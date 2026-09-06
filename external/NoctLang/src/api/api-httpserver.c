/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * API: HttpServer.*
 *
 * A minimal socket layer for writing HTTP servers in Noct.
 *
 * Design notes:
 *
 *  - Handles (listener sockets, connections and pollers) are
 *    dictionaries carrying a native pointer to a control block.
 *
 *  - A poller is a stateful handle rather than a per-call handle set.
 *    Registrations are added and removed incrementally, which maps
 *    directly onto epoll_ctl()/kqueue() and avoids rebuilding the
 *    watch set on every wait. The poll(2) backend keeps a persistent
 *    pollfd array for the same reason.
 *
 *  - Readiness is level-triggered, so the poll(2) and epoll backends
 *    behave identically. Edge-triggered mode is intentionally absent.
 *
 *  - A socket belongs to at most one poller. Closing a socket detaches
 *    it automatically, so a closed descriptor can never linger in a
 *    watch set.
 *
 *  - The poller stores its socket handles in its own handle dictionary
 *    (the "socks" key), keyed by registration id. The native side only
 *    ever holds file descriptors and registration ids, never raw
 *    object pointers, so a moving GC cannot invalidate anything.
 *
 *  - Blocking calls (accept, recv, send, wait) are wrapped in
 *    noct_enter_blocking()/noct_leave_blocking() so a blocked thread
 *    never stalls a stop-the-world GC. This is also what allows a
 *    future thread-pool design: a connection handle carries no
 *    thread affinity, so the thread that accepts it and the thread
 *    that serves it may differ.
 */

#if defined(_WIN32) && !defined(_WIN32_WINNT)
/* WSAPoll is available starting with Windows Vista. */
#define _WIN32_WINNT	0x0600
#endif

#include <noct/noct.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#if defined(NOCT_TARGET_WINDOWS)
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#if !defined(NOCT_TARGET_ZEDBSD)
#include <netinet/tcp.h>
#endif
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <poll.h>
#include <signal.h>
#endif

/* Magic numbers for the handle control blocks. */
#define SOCKET_MAGIC	0x536b7431	/* 'Skt1' */
#define POLLER_MAGIC	0x506c7231	/* 'Plr1' */

/* Event flags. Must match the values documented for Noct scripts. */
#define EVENT_READ	0x1
#define EVENT_WRITE	0x2

/* Maximum bytes accepted by a single recv() call. */
#define RECV_MAX	(16 * 1024 * 1024)

#if defined(NOCT_TARGET_ZEDBSD)
/* Supplies socket constants omitted by the minimal zedBSD headers. */
#define TCP_NODELAY	1
#define SOCK_LISTEN_BACKLOG	128
#define SOCK_NUMERIC_HOST_SIZE	1025
#define SOCK_NUMERIC_SERVICE_SIZE	32
#else
#define SOCK_LISTEN_BACKLOG	SOMAXCONN
#define SOCK_NUMERIC_HOST_SIZE	NI_MAXHOST
#define SOCK_NUMERIC_SERVICE_SIZE	NI_MAXSERV
#endif

#if defined(NOCT_TARGET_WINDOWS)
#define SOCK_INVALID		INVALID_SOCKET
#define sock_close		closesocket
#define sock_poll		WSAPoll
#define SOCK_WOULDBLOCK()	(WSAGetLastError() == WSAEWOULDBLOCK)
#define SOCK_INTR()		(WSAGetLastError() == WSAEINTR)
#else
#define SOCK_INVALID		(-1)
#define sock_close		close
#define sock_poll		poll
#define SOCK_WOULDBLOCK()	(errno == EAGAIN || errno == EWOULDBLOCK)
#define SOCK_INTR()		(errno == EINTR)
#endif

#if defined(NOCT_TARGET_WINDOWS)
typedef SOCKET sock_t;
typedef int socklen_arg_t;
typedef WSAPOLLFD sock_pollfd_t;
#else
typedef int sock_t;
typedef socklen_t socklen_arg_t;
typedef struct pollfd sock_pollfd_t;
#endif

/* Socket handle control block. */
struct socket_obj {
	int magic;
	sock_t fd;

	/* Registration id in the owning poller, or -1 if not registered. */
	int reg_id;

	/* Owning poller, or NULL. */
	struct poller_obj *poller;
};

/* One poller registration. */
struct poller_entry {
	int reg_id;
	sock_t fd;
	int events;
};

/* Poller control block. */
struct poller_obj {
	int magic;

	/* Registrations, kept dense. */
	struct poller_entry *entry;
	size_t entry_count;
	size_t entry_alloc;

	/* Persistent pollfd array, rebuilt only when registrations change. */
	sock_pollfd_t *pfd;
	size_t pfd_alloc;
	bool pfd_dirty;

	/* Next registration id. */
	int next_reg_id;
};

/* One native function registration. */
struct ffi_item {
	const char *global_name;
	const char *package_name;
	const char *field_name;
	size_t param_count;
	const char *param[NOCT_ARG_MAX];
	bool (*cfunc)(NoctEnv *env);
};

static const struct ffi_item *get_ffi_items(size_t *count);
static bool get_handle_native(NoctEnv *env, NoctValue *handle, int magic, void **native);
static bool get_open_socket(NoctEnv *env, NoctValue *handle, struct socket_obj **sock);
static bool make_socket_handle(NoctEnv *env, NoctValue *handle, sock_t fd);
static void socket_finalizer(void *native_pointer);
static void poller_finalizer(void *native_pointer);
static int poller_find(struct poller_obj *poller, int reg_id);
static bool poller_add_entry(NoctEnv *env, struct poller_obj *poller, sock_t fd, int events, int *ret_reg_id);
static void poller_remove_entry(struct poller_obj *poller, int index);
static bool poller_sync(NoctEnv *env, struct poller_obj *poller);
static bool detach_socket(NoctEnv *env, NoctValue *poller_handle, struct socket_obj *sock);
static bool cfunc_HttpServer_listen(NoctEnv *env);
static bool cfunc_HttpServer_connect(NoctEnv *env);
static bool cfunc_HttpServer_accept(NoctEnv *env);
static bool cfunc_HttpServer_recv(NoctEnv *env);
static bool cfunc_HttpServer_send(NoctEnv *env);
static bool cfunc_HttpServer_close(NoctEnv *env);
static bool cfunc_HttpServer_isClosed(NoctEnv *env);
static bool cfunc_HttpServer_setBlocking(NoctEnv *env);
static bool cfunc_HttpServer_getPeer(NoctEnv *env);
static bool cfunc_HttpServer_createPoller(NoctEnv *env);
static bool cfunc_HttpServer_addToPoller(NoctEnv *env);
static bool cfunc_HttpServer_modifyPoller(NoctEnv *env);
static bool cfunc_HttpServer_removeFromPoller(NoctEnv *env);
static bool cfunc_HttpServer_waitPoller(NoctEnv *env);
static bool cfunc_HttpServer_countPoller(NoctEnv *env);

/*
 * Registers the HttpServer API.
 */
NOCT_DLL
bool
noct_register_api_httpserver(
	NoctEnv *env)
{
	NoctValue srv_dict;
	NoctValue tmp;
	NoctValue funcval;
	const struct ffi_item *ffi_items;
	const char *param[NOCT_ARG_MAX];
	size_t ffi_item_count;
	size_t i;
	size_t j;
#if defined(NOCT_TARGET_WINDOWS)
	WSADATA wsa;
	static bool initialized;
	int startup_result;
#endif

	/* Pin the values that survive registration calls. */
	memset(&srv_dict, 0, sizeof(NoctValue));
	memset(&tmp, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &srv_dict, &tmp);

#if defined(NOCT_TARGET_WINDOWS)
	/* Initialize Winsock once for this process. */
	if (!initialized) {
		startup_result = WSAStartup(MAKEWORD(2, 2), &wsa);

		/* Report a failed Winsock initialization. */
		if (startup_result != 0) {
			noct_error(env, N_TR("Cannot initialize the socket library."));
			return false;
		}

		/* Records the completed process-wide initialization. */
		initialized = true;
	}
#else
	/* Do not die on a write to a closed connection. */
#if defined(NOCT_TARGET_ZEDBSD)
	signal(SIGPIPE, (sighandler_t)(uintptr_t)SIG_IGN);
#else
	signal(SIGPIPE, SIG_IGN);
#endif
#endif

	/* Create the global HttpServer dictionary. */
	if (!noct_make_empty_dict(env, &srv_dict))
		return false;

	/* Publish the global HttpServer dictionary. */
	if (!noct_set_global(env, "HttpServer", &srv_dict))
		return false;

	/* Register every HttpServer function in declaration order. */
	ffi_items = get_ffi_items(&ffi_item_count);
	for (i = 0; i < ffi_item_count; i++) {
		/* Clear the temporary function value. */
		memset(&funcval, 0, sizeof(NoctValue));

		/* Copies the immutable parameter names for the registration API. */
		for (j = 0; j < NOCT_ARG_MAX; j++)
			param[j] = ffi_items[i].param[j];

		/* Register the native function globally. */
		if (!noct_register_cfunc(
			env,
			ffi_items[i].global_name,
			ffi_items[i].param_count,
			param,
			ffi_items[i].cfunc,
			NULL)) {
			return false;
		}

		/* Fetch the registered function value. */
		if (!noct_get_global(env, ffi_items[i].global_name, &funcval))
			return false;

		/* Publish the function in the HttpServer dictionary. */
		if (!noct_set_dict_elem_cstr(env, &srv_dict, ffi_items[i].field_name, &funcval))
			return false;
	}

	/* Publish the readable event flag. */
	if (!noct_set_dict_elem_make_int(env, &srv_dict, "READ", &tmp, EVENT_READ))
		return false;

	/* Publish the writable event flag. */
	if (!noct_set_dict_elem_make_int(env, &srv_dict, "WRITE", &tmp, EVENT_WRITE))
		return false;

	/* Release the registration roots. */
	noct_unpin_local(env, 2, &srv_dict, &tmp);

	/* Report successful API registration. */
	return true;
}

/* Return the immutable native-function table. */
static const struct ffi_item *
get_ffi_items(
	size_t *count)
{
	static const struct ffi_item ffi_items[] = {
		{
			"HttpServer.listen", "HttpServer", "listen", 2,
			{"host", "port"}, cfunc_HttpServer_listen
		},
		{
			"HttpServer.connect", "HttpServer", "connect", 2,
			{"host", "port"}, cfunc_HttpServer_connect
		},
		{
			"HttpServer.accept", "HttpServer", "accept", 1,
			{"server"}, cfunc_HttpServer_accept
		},
		{
			"HttpServer.recv", "HttpServer", "recv", 2,
			{"conn", "maxBytes"}, cfunc_HttpServer_recv
		},
		{
			"HttpServer.send", "HttpServer", "send", 2,
			{"conn", "data"}, cfunc_HttpServer_send
		},
		{
			"HttpServer.close", "HttpServer", "close", 1,
			{"sock"}, cfunc_HttpServer_close
		},
		{
			"HttpServer.isClosed", "HttpServer", "isClosed", 1,
			{"sock"}, cfunc_HttpServer_isClosed
		},
		{
			"HttpServer.setBlocking", "HttpServer", "setBlocking", 2,
			{"sock", "blocking"}, cfunc_HttpServer_setBlocking
		},
		{
			"HttpServer.getPeer", "HttpServer", "getPeer", 1,
			{"conn"}, cfunc_HttpServer_getPeer
		},
		{
			"HttpServer.createPoller", "HttpServer", "createPoller", 0,
			{NULL}, cfunc_HttpServer_createPoller
		},
		{
			"HttpServer.addToPoller", "HttpServer", "addToPoller", 3,
			{"poller", "sock", "events"}, cfunc_HttpServer_addToPoller
		},
		{
			"HttpServer.modifyPoller", "HttpServer", "modifyPoller", 3,
			{"poller", "sock", "events"}, cfunc_HttpServer_modifyPoller
		},
		{
			"HttpServer.removeFromPoller", "HttpServer", "removeFromPoller", 2,
			{"poller", "sock"}, cfunc_HttpServer_removeFromPoller
		},
		{
			"HttpServer.waitPoller", "HttpServer", "waitPoller", 2,
			{"poller", "timeout"}, cfunc_HttpServer_waitPoller
		},
		{
			"HttpServer.countPoller", "HttpServer", "countPoller", 1,
			{"poller"}, cfunc_HttpServer_countPoller
		}
	};

	assert(count != NULL);

	/* Publish the immutable table size. */
	*count = sizeof(ffi_items) / sizeof(ffi_items[0]);

	/* Return the immutable registration table. */
	return ffi_items;
}

/* Get a control block from a handle dictionary with a magic check. */
static bool
get_handle_native(
	NoctEnv *env,
	NoctValue *handle,
	int magic,
	void **native)
{
	void (*finalizer)(void *);

	/* Retrieves the native control block from the handle. */
	if (!noct_get_dict_native_pointer(env, handle, native, &finalizer))
		return false;

	/* Rejects a missing control block or a handle of another kind. */
	if (*native == NULL || *(int *)*native != magic) {
		noct_error(env, N_TR("Invalid handle."));
		return false;
	}

	/* Reports a valid native handle. */
	return true;
}

/* Get a socket control block that is still open. */
static bool
get_open_socket(
	NoctEnv *env,
	NoctValue *handle,
	struct socket_obj **sock)
{
	/* Retrieves the socket control block. */
	if (!get_handle_native(env, handle, SOCKET_MAGIC, (void **)sock))
		return false;

	/* Rejects a descriptor that was already closed. */
	if ((*sock)->fd == SOCK_INVALID) {
		noct_error(env, N_TR("Socket is already closed."));
		return false;
	}

	/* Reports an open socket. */
	return true;
}

/* Make a socket handle dictionary for an open descriptor. */
static bool
make_socket_handle(
	NoctEnv *env,
	NoctValue *handle,
	sock_t fd)
{
	struct socket_obj *obj;

	/* Allocates the socket control block. */
	obj = noct_malloc(sizeof(struct socket_obj));
	if (obj == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the socket control block. */
	memset(obj, 0, sizeof(struct socket_obj));
	obj->magic = SOCKET_MAGIC;
	obj->fd = fd;
	obj->reg_id = -1;
	obj->poller = NULL;

	/* Creates the socket handle dictionary. */
	if (!noct_make_empty_dict(env, handle)) {
		noct_free(obj);
		return false;
	}

	/* Attaches the control block to the handle dictionary. */
	if (!noct_set_dict_native_pointer(env, handle, obj, socket_finalizer)) {
		noct_free(obj);
		return false;
	}

	/* Reports a completed socket handle. */
	return true;
}

/* Finalizes a socket control block. */
static void
socket_finalizer(
	void *native_pointer)
{
	struct socket_obj *obj;

	/* Recovers the socket control block. */
	obj = (struct socket_obj *)native_pointer;

	/* Ignores an empty native pointer. */
	if (obj == NULL)
		return;

	/*
	 * A socket that is still registered keeps its handle dictionary
	 * alive through the poller, so reaching the finalizer means the
	 * registration is already gone.
	 */
	if (obj->fd != SOCK_INVALID)
		sock_close(obj->fd);

	/* Releases the socket control block. */
	noct_free(obj);
}

/* Finalizes a poller control block. */
static void
poller_finalizer(
	void *native_pointer)
{
	struct poller_obj *obj;

	/* Recovers the poller control block. */
	obj = (struct poller_obj *)native_pointer;

	/* Ignores an empty native pointer. */
	if (obj == NULL)
		return;

	/* Releases the poller-owned storage and control block. */
	noct_free(obj->entry);
	noct_free(obj->pfd);
	noct_free(obj);
}

/* Find a registration by id. Returns the index, or -1. */
static int
poller_find(
	struct poller_obj *poller,
	int reg_id)
{
	size_t i;

	/* Searches the dense registration table. */
	for (i = 0; i < poller->entry_count; i++) {
		/* Returns the matching registration index. */
		if (poller->entry[i].reg_id == reg_id)
			return (int)i;
	}

	/* Reports that the registration does not exist. */
	return -1;
}

/* Append a registration. */
static bool
poller_add_entry(
	NoctEnv *env,
	struct poller_obj *poller,
	sock_t fd,
	int events,
	int *ret_reg_id)
{
	size_t new_alloc;
	struct poller_entry *new_entry;

	/* Grows the dense registration table when it is full. */
	if (poller->entry_count == poller->entry_alloc) {
		new_alloc = poller->entry_alloc == 0 ? 16 : poller->entry_alloc * 2;
		new_entry = noct_realloc(
			poller->entry,
			new_alloc * sizeof(struct poller_entry));

		/* Reports a failed registration-table allocation. */
		if (new_entry == NULL) {
			noct_out_of_memory(env);
			return false;
		}

		/* Publishes the grown registration-table storage. */
		poller->entry = new_entry;
		poller->entry_alloc = new_alloc;
	}

	/* Appends the new registration and publishes its identifier. */
	poller->entry[poller->entry_count].reg_id = poller->next_reg_id++;
	poller->entry[poller->entry_count].fd = fd;
	poller->entry[poller->entry_count].events = events;
	*ret_reg_id = poller->entry[poller->entry_count].reg_id;
	poller->entry_count++;

	/* Marks the platform poll array for reconstruction. */
	poller->pfd_dirty = true;

	/* Reports a completed registration. */
	return true;
}

/* Remove a registration by index. */
static void
poller_remove_entry(
	struct poller_obj *poller,
	int index)
{
	/* Ignores an index outside the registration table. */
	if (index < 0 || (size_t)index >= poller->entry_count)
		return;

	/* Moves the last entry over a removed interior entry. */
	if ((size_t)index != poller->entry_count - 1) {
		poller->entry[index] = poller->entry[poller->entry_count - 1];
	}

	/* Shrinks the table and schedules poll-array reconstruction. */
	poller->entry_count--;
	poller->pfd_dirty = true;
}

/* Rebuild the pollfd array if the registrations changed. */
static bool
poller_sync(
	NoctEnv *env,
	struct poller_obj *poller)
{
	size_t i;
	size_t new_alloc;
	sock_pollfd_t *new_pfd;

	/* Reuses an up-to-date platform poll array. */
	if (!poller->pfd_dirty)
		return true;

	/* Grows the platform poll array for the current registrations. */
	if (poller->entry_count > poller->pfd_alloc) {
		/* Finds the next sufficient geometric allocation size. */
		new_alloc = poller->pfd_alloc == 0 ? 16 : poller->pfd_alloc;
		while (new_alloc < poller->entry_count)
			new_alloc *= 2;

		/* Reallocates the platform poll array. */
		new_pfd = noct_realloc(
			poller->pfd,
			new_alloc * sizeof(sock_pollfd_t));

		/* Reports a failed poll-array allocation. */
		if (new_pfd == NULL) {
			noct_out_of_memory(env);
			return false;
		}

		/* Publishes the grown platform poll-array storage. */
		poller->pfd = new_pfd;
		poller->pfd_alloc = new_alloc;
	}

	/* Rebuilds each platform registration from the native table. */
	for (i = 0; i < poller->entry_count; i++) {
		poller->pfd[i].fd = poller->entry[i].fd;
		poller->pfd[i].events = 0;

		/* Maps readable interest to the platform poll flag. */
		if (poller->entry[i].events & EVENT_READ)
			poller->pfd[i].events |= POLLIN;

		/* Maps writable interest to the platform poll flag. */
		if (poller->entry[i].events & EVENT_WRITE)
			poller->pfd[i].events |= POLLOUT;

		/* Clears events left by the previous platform poll operation. */
		poller->pfd[i].revents = 0;
	}

	/* Marks the reconstructed platform poll array as current. */
	poller->pfd_dirty = false;

	/* Reports a synchronized poller. */
	return true;
}

/* Detaches a socket from its poller. */
static bool
detach_socket(
	NoctEnv *env,
	NoctValue *poller_handle,
	struct socket_obj *sock)
{
	NoctValue socks;
	NoctValue key;
	char key_s[32];
	int index;

	/* Ignores a socket that is not registered. */
	if (sock->poller == NULL)
		return true;

	/* Pins the values used while removing the registration. */
	memset(&socks, 0, sizeof(NoctValue));
	memset(&key, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &socks, &key);

	/* Removes the native registration. */
	index = poller_find(sock->poller, sock->reg_id);
	poller_remove_entry(sock->poller, index);

	/* Removes the GC-owned handle reference when its poller is available. */
	if (poller_handle != NULL) {
		/* Retrieves the poller's GC-owned socket dictionary. */
		if (!noct_get_dict_elem_check_dict(env, poller_handle, "socks", &socks))
			return false;

		/* Formats the socket's registration key. */
		snprintf(key_s, sizeof(key_s), "%d", sock->reg_id);

		/* Removes the handle at the formatted registration key. */
		if (!noct_remove_dict_elem_cstr(env, &socks, key_s))
			return false;
	}

	/* Clears the socket's poller ownership. */
	sock->poller = NULL;
	sock->reg_id = -1;

	/* Releases the temporary roots. */
	noct_unpin_local(env, 2, &socks, &key);

	/* Reports a detached socket. */
	return true;
}

/* Implements HttpServer.listen(). */
static bool
cfunc_HttpServer_listen(
	NoctEnv *env)
{
	NoctValue host;
	NoctValue port;
	NoctValue handle;
	const char *host_s;
	char port_s[16];
	int port_i;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *ai;
	sock_t fd;
	int on;

	/* Pins the argument and result values. */
	memset(&host, 0, sizeof(NoctValue));
	memset(&port, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &host, &port, &handle);

	/* Reads the listening host argument. */
	if (!noct_get_arg_check_string(env, 0, &host, &host_s))
		return false;

	/* Reads the listening port argument. */
	if (!noct_get_arg_check_int(env, 1, &port, &port_i))
		return false;

	/* Rejects a port outside the socket address range. */
	if (port_i < 0 || port_i > 65535) {
		noct_error(env, N_TR("Port number is out-of-range."));
		return false;
	}

	/* Formats the service name for address resolution. */
	snprintf(port_s, sizeof(port_s), "%d", port_i);

	/* Resolves the requested passive listening address. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	if (getaddrinfo(
		host_s[0] == '\0' ? NULL : host_s,
		port_s,
		&hints,
		&res) != 0) {
		noct_error(env, N_TR("Cannot resolve the address %s."), host_s);
		return false;
	}

	/* Binds the first usable resolved address. */
	fd = SOCK_INVALID;
	for (ai = res; ai != NULL; ai = ai->ai_next) {
		/* Opens a socket for this address candidate. */
		fd = socket(
			ai->ai_family,
			ai->ai_socktype,
			ai->ai_protocol);

		/* Skips an address whose socket could not be opened. */
		if (fd == SOCK_INVALID)
			continue;

		/* Enables immediate reuse of the listening address. */
		on = 1;
		setsockopt(
			fd,
			SOL_SOCKET,
			SO_REUSEADDR,
			(const char *)&on,
			sizeof(on));

		/* Stops after binding a usable address. */
		if (bind(
			fd,
			ai->ai_addr,
			(socklen_arg_t)ai->ai_addrlen) == 0) {
			break;
		}

		/* Closes a socket that could not bind its candidate address. */
		sock_close(fd);
		fd = SOCK_INVALID;
	}

	/* Releases the resolved address list. */
	freeaddrinfo(res);

	/* Reports that no resolved address could be bound. */
	if (fd == SOCK_INVALID) {
		noct_error(
			env,
			N_TR("Cannot bind to %s:%d. (%s)"),
			host_s,
			port_i,
			strerror(errno));
		return false;
	}

	/* Starts listening for connections. */
	if (listen(fd, SOCK_LISTEN_BACKLOG) != 0) {
		sock_close(fd);
		noct_error(env, N_TR("Cannot listen on %s:%d."), host_s, port_i);
		return false;
	}

	/* Wraps the listening descriptor in a managed handle. */
	if (!make_socket_handle(env, &handle, fd)) {
		sock_close(fd);
		return false;
	}

	/* Publishes the listening socket handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &host, &port, &handle);

	/* Reports a successful listen operation. */
	return true;
}

/* Implements HttpServer.connect(). */
static bool
cfunc_HttpServer_connect(
	NoctEnv *env)
{
	NoctValue host;
	NoctValue port;
	NoctValue handle;
	const char *host_s;
	char port_s[16];
	char host_buf[256];
	int port_i;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *ai;
	sock_t fd;
	int on;

	/* Pins the argument and result values. */
	memset(&host, 0, sizeof(NoctValue));
	memset(&port, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &host, &port, &handle);

	/* Reads the remote host argument. */
	if (!noct_get_arg_check_string(env, 0, &host, &host_s))
		return false;

	/* Reads the remote port argument. */
	if (!noct_get_arg_check_int(env, 1, &port, &port_i))
		return false;

	/* Rejects a port outside the connectable range. */
	if (port_i < 1 || port_i > 65535) {
		noct_error(env, N_TR("Port number is out-of-range."));
		return false;
	}

	/* Formats the service name for address resolution. */
	snprintf(port_s, sizeof(port_s), "%d", port_i);

	/* Copy the host name: the resolve and connect below park this thread. */
	strncpy(host_buf, host_s, sizeof(host_buf) - 1);
	host_buf[sizeof(host_buf) - 1] = '\0';

	/* Prepares a stream-socket address query. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	/* Resolves and connects without blocking the VM. */
	fd = SOCK_INVALID;
	noct_enter_blocking(env);

	/* Tries each resolved address when resolution succeeds. */
	if (getaddrinfo(host_buf, port_s, &hints, &res) == 0) {
		/* Connects the first usable resolved address. */
		for (ai = res; ai != NULL; ai = ai->ai_next) {
			/* Opens a socket for this address candidate. */
			fd = socket(
				ai->ai_family,
				ai->ai_socktype,
				ai->ai_protocol);

			/* Skips an address whose socket could not be opened. */
			if (fd == SOCK_INVALID)
				continue;

			/* Stops after connecting a usable address. */
			if (connect(
				fd,
				ai->ai_addr,
				(socklen_arg_t)ai->ai_addrlen) == 0) {
				break;
			}

			/* Closes a socket that could not connect. */
			sock_close(fd);
			fd = SOCK_INVALID;
		}

		/* Releases the resolved address list. */
		freeaddrinfo(res);
	}

	/* Returns this thread to VM execution. */
	noct_leave_blocking(env);

	/* Reports that no resolved address could be connected. */
	if (fd == SOCK_INVALID) {
		noct_error(env, N_TR("Cannot connect to %s:%d."), host_buf, port_i);
		return false;
	}

	/* Disables Nagle buffering for request and response traffic. */
	on = 1;
	setsockopt(
		fd,
		IPPROTO_TCP,
		TCP_NODELAY,
		(const char *)&on,
		sizeof(on));

	/* Wraps the connected descriptor in a managed handle. */
	if (!make_socket_handle(env, &handle, fd)) {
		sock_close(fd);
		return false;
	}

	/* Publishes the connected socket handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &host, &port, &handle);

	/* Reports a successful connection. */
	return true;
}

/* Implements HttpServer.accept(). */
static bool
cfunc_HttpServer_accept(
	NoctEnv *env)
{
	NoctValue server;
	NoctValue handle;
	struct socket_obj *sock;
	sock_t fd;
	sock_t conn_fd;
	int on;

	/* Pins the argument and result values. */
	memset(&server, 0, sizeof(NoctValue));
	memset(&handle, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &server, &handle);

	/* Reads the listener handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &server))
		return false;

	/* Retrieves the open listener control block. */
	if (!get_open_socket(env, &server, &sock))
		return false;

	/* Copies the descriptor before entering the blocking region. */
	fd = sock->fd;

	/*
	 * Accepts a connection without blocking the VM. The descriptor was
	 * read out of the control block before parking.
	 */
	noct_enter_blocking(env);

	/* Retries an interrupted accept operation. */
	for (;;) {
		conn_fd = accept(fd, NULL, NULL);

		/* Stops after accepting a connection. */
		if (conn_fd != SOCK_INVALID)
			break;

		/* Retries an interrupted system call. */
		if (SOCK_INTR())
			continue;

		/* Stops after a non-interruption failure. */
		break;
	}

	/* Returns this thread to VM execution. */
	noct_leave_blocking(env);

	/* Handles a failed or non-blocking accept operation. */
	if (conn_fd == SOCK_INVALID) {
		/* Reports no pending connection on a non-blocking listener. */
		if (SOCK_WOULDBLOCK()) {
			/* Publishes the empty accept result. */
			if (!noct_set_return_make_int(env, &handle, 0))
				return false;

			/* Releases the listener and result roots. */
			noct_unpin_local(env, 2, &server, &handle);

			/* Reports a successful empty accept operation. */
			return true;
		}

		/* Reports a failed accept operation. */
		noct_error(env, N_TR("Cannot accept a connection."));
		return false;
	}

	/* Disable Nagle: responses are written in one shot. */
	on = 1;
	setsockopt(
		conn_fd,
		IPPROTO_TCP,
		TCP_NODELAY,
		(const char *)&on,
		sizeof(on));

	/* Wraps the accepted descriptor in a managed handle. */
	if (!make_socket_handle(env, &handle, conn_fd)) {
		sock_close(conn_fd);
		return false;
	}

	/* Publishes the accepted socket handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the listener and result roots. */
	noct_unpin_local(env, 2, &server, &handle);

	/* Reports a successful accept operation. */
	return true;
}

/* Implements HttpServer.recv(). */
static bool
cfunc_HttpServer_recv(
	NoctEnv *env)
{
	NoctValue conn;
	NoctValue max_bytes;
	NoctValue ret;
	struct socket_obj *sock;
	sock_t fd;
	size_t max_n;
	char *buf;
	long received;

	/* Pins the argument and result values. */
	memset(&conn, 0, sizeof(NoctValue));
	memset(&max_bytes, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &conn, &max_bytes, &ret);

	/* Reads the connection handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &conn))
		return false;

	/* Reads the maximum receive size argument. */
	if (!noct_get_arg_check_int_long(env, 1, &max_bytes, &max_n))
		return false;

	/* Rejects an empty or excessive receive size. */
	if (max_n == 0 || max_n > RECV_MAX) {
		noct_error(env, N_TR("Receive size is out-of-range."));
		return false;
	}

	/* Retrieves the open connection control block. */
	if (!get_open_socket(env, &conn, &sock))
		return false;

	/* Copies the descriptor before entering the blocking region. */
	fd = sock->fd;

	/* Allocates a stable native buffer before parking this thread. */
	buf = noct_malloc(max_n + 1);
	if (buf == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Receives data without blocking the VM. */
	noct_enter_blocking(env);

	/* Retries an interrupted receive operation. */
	for (;;) {
		received = (long)recv(fd, buf, max_n, 0);

		/* Stops after receiving data or an end-of-stream result. */
		if (received >= 0)
			break;

		/* Retries an interrupted system call. */
		if (SOCK_INTR())
			continue;

		/* Stops after a non-interruption failure. */
		break;
	}

	/* Returns this thread to VM execution. */
	noct_leave_blocking(env);

	/* Handles a failed or non-blocking receive operation. */
	if (received < 0) {
		/* Releases the unused receive buffer. */
		noct_free(buf);

		/* Reports no available data on a non-blocking connection. */
		if (SOCK_WOULDBLOCK()) {
			/* Publishes the empty receive result. */
			if (!noct_set_return_make_string(env, &ret, ""))
				return false;

			/* Releases the argument and result roots. */
			noct_unpin_local(env, 3, &conn, &max_bytes, &ret);

			/* Reports a successful empty receive operation. */
			return true;
		}

		/* Reports a failed receive operation. */
		noct_error(env, N_TR("Cannot receive from the connection."));
		return false;
	}

	/*
	 * Publishes the data as a string. A zero-length result means the
	 * peer closed the connection; use HttpServer.isClosed() or the
	 * length to tell the two apart.
	 */
	buf[received] = '\0';
	if (!noct_set_return_make_string(env, &ret, buf)) {
		noct_free(buf);
		return false;
	}

	/* Releases the native receive buffer. */
	noct_free(buf);

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &conn, &max_bytes, &ret);

	/* Reports a successful receive operation. */
	return true;
}

/* Implements HttpServer.send(). */
static bool
cfunc_HttpServer_send(
	NoctEnv *env)
{
	NoctValue conn;
	NoctValue data;
	NoctValue ret;
	struct socket_obj *sock;
	sock_t fd;
	const char *data_s;
	char *buf;
	size_t len;
	size_t sent;
	long n;
	bool failed;

	/* Pins the argument and result values. */
	memset(&conn, 0, sizeof(NoctValue));
	memset(&data, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &conn, &data, &ret);

	/* Reads the connection handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &conn))
		return false;

	/* Reads the payload string argument. */
	if (!noct_get_arg_check_string(env, 1, &data, &data_s))
		return false;

	/* Retrieves the open connection control block. */
	if (!get_open_socket(env, &conn, &sock))
		return false;

	/* Copies the descriptor before entering the blocking region. */
	fd = sock->fd;

	/* Handles an empty payload without entering a blocking region. */
	len = strlen(data_s);
	if (len == 0) {
		/* Publishes the empty send result. */
		if (!noct_set_return_make_int_long(env, &ret, 0))
			return false;

		/* Releases the argument and result roots. */
		noct_unpin_local(env, 3, &conn, &data, &ret);

		/* Reports a successful empty send operation. */
		return true;
	}

	/*
	 * Copy the payload out of the VM heap: send() parks this thread,
	 * during which a GC may move the string.
	 */
	buf = noct_malloc(len);
	if (buf == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Copies the movable VM string into stable native storage. */
	memcpy(buf, data_s, len);

	/* Sends as much of the payload as possible without blocking the VM. */
	failed = false;
	sent = 0;
	noct_enter_blocking(env);

	/* Advances through every successfully transmitted byte. */
	while (sent < len) {
		n = (long)send(fd, buf + sent, len - sent, 0);

		/* Handles an interrupted or failed send operation. */
		if (n < 0) {
			/* Retries an interrupted system call. */
			if (SOCK_INTR())
				continue;

			/* Records the terminal send failure. */
			failed = true;
			break;
		}

		/* Advances past the bytes written by this system call. */
		sent += (size_t)n;
	}

	/* Returns this thread to VM execution. */
	noct_leave_blocking(env);

	/* Releases the stable payload buffer. */
	noct_free(buf);

	/* Reports a terminal failure that wrote no bytes. */
	if (failed &&
	    sent == 0 &&
	    !SOCK_WOULDBLOCK()) {
		noct_error(env, N_TR("Cannot send to the connection."));
		return false;
	}

	/* Publishes the number of bytes actually written. */
	if (!noct_set_return_make_int_long(env, &ret, sent))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &conn, &data, &ret);

	/* Reports a successful or partial send operation. */
	return true;
}

/* Implements HttpServer.close(). */
static bool
cfunc_HttpServer_close(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	struct socket_obj *sock;

	/* Pins the argument and result values. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &handle, &ret);

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the socket control block, including a closed one. */
	if (!get_handle_native(env, &handle, SOCKET_MAGIC, (void **)&sock))
		return false;

	/*
	 * A registered socket cannot be closed directly: the poller
	 * would be left watching a dead descriptor. Removing it first
	 * is the caller's job, but doing it here keeps the invariant
	 * unconditional.
	 */
	if (sock->poller != NULL) {
		noct_error(env, N_TR("Socket is registered to a poller. Remove it first."));
		return false;
	}

	/* Closes an open descriptor exactly once. */
	if (sock->fd != SOCK_INVALID) {
		sock_close(sock->fd);
		sock->fd = SOCK_INVALID;
	}

	/* Publishes a successful close result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 2, &handle, &ret);

	/* Reports a successful close operation. */
	return true;
}

/* Implements HttpServer.isClosed(). */
static bool
cfunc_HttpServer_isClosed(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	struct socket_obj *sock;

	/* Pins the argument and result values. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &handle, &ret);

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the socket control block, including a closed one. */
	if (!get_handle_native(env, &handle, SOCKET_MAGIC, (void **)&sock))
		return false;

	/* Publishes whether the descriptor is closed. */
	if (!noct_set_return_make_int(env, &ret, sock->fd == SOCK_INVALID ? 1 : 0))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 2, &handle, &ret);

	/* Reports a successful socket-state query. */
	return true;
}

/* Implements HttpServer.setBlocking(). */
static bool
cfunc_HttpServer_setBlocking(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue blocking;
	NoctValue ret;
	struct socket_obj *sock;
	int blocking_i;
#if defined(NOCT_TARGET_WINDOWS)
	u_long mode;
#else
	int flags;
#endif

	/* Pins the argument and result values. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&blocking, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &handle, &blocking, &ret);

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Reads the requested blocking-mode argument. */
	if (!noct_get_arg_check_int(env, 1, &blocking, &blocking_i))
		return false;

	/* Retrieves the open socket control block. */
	if (!get_open_socket(env, &handle, &sock))
		return false;

#if defined(NOCT_TARGET_WINDOWS)
	/* Changes the socket mode through Winsock. */
	mode = blocking_i ? 0 : 1;
	if (ioctlsocket(sock->fd, FIONBIO, &mode) != 0) {
		noct_error(env, N_TR("Cannot change the socket mode."));
		return false;
	}
#else
	/* Reads the current descriptor flags. */
	flags = fcntl(sock->fd, F_GETFL, 0);
	if (flags == -1) {
		noct_error(env, N_TR("Cannot change the socket mode."));
		return false;
	}

	/* Applies the requested blocking-mode flag. */
	if (blocking_i) {
		flags &= ~O_NONBLOCK;
	} else {
		flags |= O_NONBLOCK;
	}

	/* Publishes the updated descriptor flags. */
	if (fcntl(sock->fd, F_SETFL, flags) == -1) {
		noct_error(env, N_TR("Cannot change the socket mode."));
		return false;
	}
#endif

	/* Publishes a successful mode-change result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &handle, &blocking, &ret);

	/* Reports a successful mode change. */
	return true;
}

/* Implements HttpServer.getPeer(). */
static bool
cfunc_HttpServer_getPeer(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue ret;
	NoctValue tmp;
	struct socket_obj *sock;
	struct sockaddr_storage addr;
	socklen_arg_t addr_len;
	char host[SOCK_NUMERIC_HOST_SIZE];
	char serv[SOCK_NUMERIC_SERVICE_SIZE];

	/* Pins the argument, result, and temporary values. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	memset(&tmp, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &handle, &ret, &tmp);

	/* Reads the connection handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &handle))
		return false;

	/* Retrieves the open connection control block. */
	if (!get_open_socket(env, &handle, &sock))
		return false;

	/* Initializes empty peer fields for an unavailable address. */
	addr_len = (socklen_arg_t)sizeof(addr);
	host[0] = '\0';
	serv[0] = '\0';

	/* Formats a numeric peer address when the query succeeds. */
	if (getpeername(
		sock->fd,
		(struct sockaddr *)&addr,
		&addr_len) == 0) {
		getnameinfo(
			(struct sockaddr *)&addr,
			addr_len,
			host,
			sizeof(host),
			serv,
			sizeof(serv),
			NI_NUMERICHOST | NI_NUMERICSERV);
	}

	/* Creates the peer result dictionary. */
	if (!noct_make_empty_dict(env, &ret))
		return false;

	/* Publishes the numeric peer host. */
	if (!noct_set_dict_elem_make_string(env, &ret, "host", &tmp, host))
		return false;

	/* Publishes the numeric peer service. */
	if (!noct_set_dict_elem_make_string(env, &ret, "port", &tmp, serv))
		return false;

	/* Publishes the completed peer dictionary. */
	if (!noct_set_return(env, &ret))
		return false;

	/* Releases the argument, result, and temporary roots. */
	noct_unpin_local(env, 3, &handle, &ret, &tmp);

	/* Reports a successful peer query. */
	return true;
}

/* Implements HttpServer.createPoller(). */
static bool
cfunc_HttpServer_createPoller(
	NoctEnv *env)
{
	NoctValue handle;
	NoctValue socks;
	struct poller_obj *obj;

	/* Pins the poller handle and its socket dictionary. */
	memset(&handle, 0, sizeof(NoctValue));
	memset(&socks, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &handle, &socks);

	/* Allocates the poller control block. */
	obj = noct_malloc(sizeof(struct poller_obj));
	if (obj == NULL) {
		noct_out_of_memory(env);
		return false;
	}

	/* Initializes the poller control block. */
	memset(obj, 0, sizeof(struct poller_obj));
	obj->magic = POLLER_MAGIC;
	obj->next_reg_id = 1;

	/* Creates the poller handle dictionary. */
	if (!noct_make_empty_dict(env, &handle)) {
		noct_free(obj);
		return false;
	}

	/* Attaches the control block to the poller handle. */
	if (!noct_set_dict_native_pointer(env, &handle, obj, poller_finalizer)) {
		noct_free(obj);
		return false;
	}

	/*
	 * Registered socket handles live here, keyed by registration id.
	 * This is what keeps them reachable for the GC while the native
	 * side only remembers descriptors.
	 */
	if (!noct_make_empty_dict(env, &socks))
		return false;

	/* Attaches the socket-root dictionary to the poller handle. */
	if (!noct_set_dict_elem_cstr(env, &handle, "socks", &socks))
		return false;

	/* Publishes the completed poller handle. */
	if (!noct_set_return(env, &handle))
		return false;

	/* Releases the poller handle and socket-dictionary roots. */
	noct_unpin_local(env, 2, &handle, &socks);

	/* Reports a successful poller creation. */
	return true;
}

/* Implements HttpServer.addToPoller(). */
static bool
cfunc_HttpServer_addToPoller(
	NoctEnv *env)
{
	NoctValue poller_h;
	NoctValue sock_h;
	NoctValue events;
	NoctValue socks;
	NoctValue ret;
	struct poller_obj *poller;
	struct socket_obj *sock;
	char key_s[32];
	int events_i;
	int reg_id;
	int rollback_index;

	/* Pins the arguments, socket dictionary, and result. */
	memset(&poller_h, 0, sizeof(NoctValue));
	memset(&sock_h, 0, sizeof(NoctValue));
	memset(&events, 0, sizeof(NoctValue));
	memset(&socks, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 5, &poller_h, &sock_h, &events, &socks, &ret);

	/* Reads the poller handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &poller_h))
		return false;

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 1, &sock_h))
		return false;

	/* Reads the event-mask argument. */
	if (!noct_get_arg_check_int(env, 2, &events, &events_i))
		return false;

	/* Retrieves the poller control block. */
	if (!get_handle_native(env, &poller_h, POLLER_MAGIC, (void **)&poller))
		return false;

	/* Retrieves the open socket control block. */
	if (!get_open_socket(env, &sock_h, &sock))
		return false;

	/* Rejects an empty or unknown event mask. */
	if ((events_i & ~(EVENT_READ | EVENT_WRITE)) != 0 || events_i == 0) {
		noct_error(env, N_TR("Invalid event flags."));
		return false;
	}

	/* Rejects a socket that is already owned by a poller. */
	if (sock->poller != NULL) {
		noct_error(env, N_TR("Socket is already registered to a poller."));
		return false;
	}

	/* Adds the descriptor to the native registration table. */
	if (!poller_add_entry(env, poller, sock->fd, events_i, &reg_id))
		return false;

	/* Retrieves the poller's GC-owned socket dictionary. */
	if (!noct_get_dict_elem_check_dict(env, &poller_h, "socks", &socks)) {
		rollback_index = poller_find(poller, reg_id);
		poller_remove_entry(poller, rollback_index);
		return false;
	}

	/* Formats the new registration key. */
	snprintf(key_s, sizeof(key_s), "%d", reg_id);

	/* Keeps the socket handle reachable through the poller. */
	if (!noct_set_dict_elem_cstr(env, &socks, key_s, &sock_h)) {
		rollback_index = poller_find(poller, reg_id);
		poller_remove_entry(poller, rollback_index);
		return false;
	}

	/* Records the poller ownership in the socket control block. */
	sock->poller = poller;
	sock->reg_id = reg_id;

	/* Publishes a successful registration result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the argument, dictionary, and result roots. */
	noct_unpin_local(env, 5, &poller_h, &sock_h, &events, &socks, &ret);

	/* Reports a successful poller registration. */
	return true;
}

/* Implements HttpServer.modifyPoller(). */
static bool
cfunc_HttpServer_modifyPoller(
	NoctEnv *env)
{
	NoctValue poller_h;
	NoctValue sock_h;
	NoctValue events;
	NoctValue ret;
	struct poller_obj *poller;
	struct socket_obj *sock;
	int events_i;
	int index;

	/* Pins the arguments and result. */
	memset(&poller_h, 0, sizeof(NoctValue));
	memset(&sock_h, 0, sizeof(NoctValue));
	memset(&events, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 4, &poller_h, &sock_h, &events, &ret);

	/* Reads the poller handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &poller_h))
		return false;

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 1, &sock_h))
		return false;

	/* Reads the replacement event mask. */
	if (!noct_get_arg_check_int(env, 2, &events, &events_i))
		return false;

	/* Retrieves the poller control block. */
	if (!get_handle_native(env, &poller_h, POLLER_MAGIC, (void **)&poller))
		return false;

	/* Retrieves the socket control block, including a closed one. */
	if (!get_handle_native(env, &sock_h, SOCKET_MAGIC, (void **)&sock))
		return false;

	/* Rejects an empty or unknown event mask. */
	if ((events_i & ~(EVENT_READ | EVENT_WRITE)) != 0 || events_i == 0) {
		noct_error(env, N_TR("Invalid event flags."));
		return false;
	}

	/* Rejects a socket owned by a different poller. */
	if (sock->poller != poller) {
		noct_error(env, N_TR("Socket is not registered to this poller."));
		return false;
	}

	/* Finds the native registration for the socket. */
	index = poller_find(poller, sock->reg_id);

	/* Rejects a missing native registration. */
	if (index < 0) {
		noct_error(env, N_TR("Socket is not registered to this poller."));
		return false;
	}

	/* Replaces the event mask and schedules poll-array reconstruction. */
	poller->entry[index].events = events_i;
	poller->pfd_dirty = true;

	/* Publishes a successful modification result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 4, &poller_h, &sock_h, &events, &ret);

	/* Reports a successful poller modification. */
	return true;
}

/* Implements HttpServer.removeFromPoller(). */
static bool
cfunc_HttpServer_removeFromPoller(
	NoctEnv *env)
{
	NoctValue poller_h;
	NoctValue sock_h;
	NoctValue ret;
	struct poller_obj *poller;
	struct socket_obj *sock;

	/* Pins the arguments and result. */
	memset(&poller_h, 0, sizeof(NoctValue));
	memset(&sock_h, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 3, &poller_h, &sock_h, &ret);

	/* Reads the poller handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &poller_h))
		return false;

	/* Reads the socket handle argument. */
	if (!noct_get_arg_check_dict(env, 1, &sock_h))
		return false;

	/* Retrieves the poller control block. */
	if (!get_handle_native(env, &poller_h, POLLER_MAGIC, (void **)&poller))
		return false;

	/* Retrieves the socket control block, including a closed one. */
	if (!get_handle_native(env, &sock_h, SOCKET_MAGIC, (void **)&sock))
		return false;

	/* Rejects a socket owned by a different poller. */
	if (sock->poller != poller) {
		noct_error(env, N_TR("Socket is not registered to this poller."));
		return false;
	}

	/* Removes both the native registration and GC-owned handle. */
	if (!detach_socket(env, &poller_h, sock))
		return false;

	/* Publishes a successful removal result. */
	if (!noct_set_return_make_int(env, &ret, 1))
		return false;

	/* Releases the argument and result roots. */
	noct_unpin_local(env, 3, &poller_h, &sock_h, &ret);

	/* Reports a successful poller removal. */
	return true;
}

/* Implements HttpServer.waitPoller(). */
static bool
cfunc_HttpServer_waitPoller(
	NoctEnv *env)
{
	NoctValue poller_h;
	NoctValue timeout;
	NoctValue socks;
	NoctValue ret;
	NoctValue item;
	NoctValue sock_h;
	struct poller_obj *poller;
	char key_s[32];
	int timeout_i;
	int poll_ret;
	int ev;
	size_t i;
	size_t ready_count;

	/* Pins the arguments, temporary values, and result array. */
	memset(&poller_h, 0, sizeof(NoctValue));
	memset(&timeout, 0, sizeof(NoctValue));
	memset(&socks, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	memset(&item, 0, sizeof(NoctValue));
	memset(&sock_h, 0, sizeof(NoctValue));
	noct_pin_local(env, 6, &poller_h, &timeout, &socks, &ret, &item, &sock_h);

	/* Reads the poller handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &poller_h))
		return false;

	/* Reads the timeout argument. */
	if (!noct_get_arg_check_int(env, 1, &timeout, &timeout_i))
		return false;

	/* Retrieves the poller control block. */
	if (!get_handle_native(env, &poller_h, POLLER_MAGIC, (void **)&poller))
		return false;

	/* Refreshes the platform poll array when registrations changed. */
	if (!poller_sync(env, poller))
		return false;

	/* Creates the result before the thread enters a blocking region. */
	if (!noct_make_empty_array(env, &ret))
		return false;

	/* Returns immediately when the poller has no registrations. */
	if (poller->entry_count == 0) {
		/* Publishes the empty readiness array. */
		if (!noct_set_return(env, &ret))
			return false;

		/* Releases the argument, temporary, and result roots. */
		noct_unpin_local(env, 6, &poller_h, &timeout, &socks, &ret, &item, &sock_h);

		/* Reports a successful empty wait operation. */
		return true;
	}

	/* Waits for socket events without blocking the VM. */
	noct_enter_blocking(env);

	/* Retries an interrupted platform poll operation. */
	for (;;) {
		poll_ret = sock_poll(
			poller->pfd,
			(unsigned int)poller->entry_count,
			timeout_i);

		/* Stops after a successful poll operation. */
		if (poll_ret >= 0)
			break;

		/* Retries an interrupted system call. */
		if (SOCK_INTR())
			continue;

		/* Stops after a non-interruption failure. */
		break;
	}

	/* Returns this thread to VM execution. */
	noct_leave_blocking(env);

	/* Reports a failed platform poll operation. */
	if (poll_ret < 0) {
		noct_error(env, N_TR("Cannot wait for socket events."));
		return false;
	}

	/* Retrieves the GC-owned socket handles. */
	if (!noct_get_dict_elem_check_dict(env, &poller_h, "socks", &socks))
		return false;

	/* Collects each ready socket into the result array. */
	ready_count = 0;
	for (i = 0; i < poller->entry_count; i++) {
		/* Skips a registration without reported events. */
		if (poller->pfd[i].revents == 0)
			continue;

		/*
		 * Report an error or a hang-up as readable: the caller
		 * finds out by getting zero bytes from the next recv().
		 */
		ev = 0;

		/* Maps readable, error, and hang-up events to readable. */
		if (poller->pfd[i].revents & (POLLIN | POLLERR | POLLHUP | POLLNVAL))
			ev |= EVENT_READ;

		/* Maps a writable platform event to writable. */
		if (poller->pfd[i].revents & POLLOUT)
			ev |= EVENT_WRITE;

		/* Skips platform events that have no public representation. */
		if (ev == 0)
			continue;

		/* Formats the registration key for the socket dictionary. */
		snprintf(key_s, sizeof(key_s), "%d", poller->entry[i].reg_id);

		/* Retrieves the ready socket handle. */
		if (!noct_get_dict_elem_check_dict(env, &socks, key_s, &sock_h))
			return false;

		/* Creates one readiness result dictionary. */
		if (!noct_make_empty_dict(env, &item))
			return false;

		/* Publishes the ready socket in the result item. */
		if (!noct_set_dict_elem_cstr(env, &item, "socket", &sock_h))
			return false;

		/* Publishes the event mask in the result item. */
		if (!noct_set_dict_elem_make_int(env, &item, "events", &timeout, ev))
			return false;

		/* Appends the completed readiness item. */
		if (!noct_set_array_elem(env, &ret, ready_count, &item))
			return false;

		/* Advances the dense result index. */
		ready_count++;
	}

	/* Publishes the completed readiness array. */
	if (!noct_set_return(env, &ret))
		return false;

	/* Releases the argument, temporary, and result roots. */
	noct_unpin_local(env, 6, &poller_h, &timeout, &socks, &ret, &item, &sock_h);

	/* Reports a successful poller wait. */
	return true;
}

/* Implements HttpServer.countPoller(). */
static bool
cfunc_HttpServer_countPoller(
	NoctEnv *env)
{
	NoctValue poller_h;
	NoctValue ret;
	struct poller_obj *poller;

	/* Pins the poller argument and result. */
	memset(&poller_h, 0, sizeof(NoctValue));
	memset(&ret, 0, sizeof(NoctValue));
	noct_pin_local(env, 2, &poller_h, &ret);

	/* Reads the poller handle argument. */
	if (!noct_get_arg_check_dict(env, 0, &poller_h))
		return false;

	/* Retrieves the poller control block. */
	if (!get_handle_native(env, &poller_h, POLLER_MAGIC, (void **)&poller))
		return false;

	/* Publishes the current registration count. */
	if (!noct_set_return_make_int_long(env, &ret, poller->entry_count))
		return false;

	/* Releases the poller argument and result roots. */
	noct_unpin_local(env, 2, &poller_h, &ret);

	/* Reports a successful registration-count query. */
	return true;
}
