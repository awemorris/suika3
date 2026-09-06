/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Noct Programming Language
 * Copyright (C) 2025, 2026 Awe Morris
 *
 * SPDX-License-Identifier: Zlib
 */

/*
 * The zedBSD BeUI evdev state-engine host corpus.
 */

#define NOCT_BEUI_ZEDBSD_INPUT_TEST 1
#include "../../src/api/api-beui-zedbsd.c"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition)                                                       \
	do {                                                                   \
		if (!(condition)) {                                            \
			fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, \
				__LINE__, #condition);                         \
			exit(1);                                               \
		}                                                              \
	} while (0)

static struct input_event
event_make(unsigned type, unsigned code, int value)
{
	struct input_event event;

	memset(&event, 0, sizeof(event));
	event.type = (uint16_t)type;
	event.code = (uint16_t)code;
	event.value = value;
	return event;
}

static void
capabilities_begin(struct noct_beui_zedbsd_input_capabilities *capabilities)
{
	noct_beui_zedbsd_input_capabilities_clear(capabilities);
	noct_beui_zedbsd_input_capabilities_set_event(capabilities, EV_SYN);
}

static void
bitmap_set(unsigned long *bits, unsigned code)
{
	bits[code / NOCT_BEUI_ZEDBSD_INPUT_BITS_PER_WORD] |=
	    1UL << (code % NOCT_BEUI_ZEDBSD_INPUT_BITS_PER_WORD);
}

static unsigned
feed_event(struct noct_beui_zedbsd_input *input, unsigned source, unsigned type,
	   unsigned code, int value)
{
	struct input_event event;

	event = event_make(type, code, value);
	return noct_beui_zedbsd_input_feed(input, source, &event,
					   sizeof(event));
}

static void
test_capability_classification(void)
{
	struct noct_beui_zedbsd_input_capabilities capabilities;
	struct input_absinfo information;
	unsigned roles;

	noct_beui_zedbsd_input_capabilities_clear(&capabilities);
	noct_beui_zedbsd_input_capabilities_set_key(&capabilities, KEY_A);
	CHECK(noct_beui_zedbsd_input_classify(&capabilities) ==
	      NOCT_BEUI_ZEDBSD_INPUT_ROLE_NONE);

	capabilities_begin(&capabilities);
	noct_beui_zedbsd_input_capabilities_set_key(&capabilities, KEY_A);
	roles = noct_beui_zedbsd_input_classify(&capabilities);
	CHECK(roles == NOCT_BEUI_ZEDBSD_INPUT_ROLE_KEYBOARD);

	noct_beui_zedbsd_input_capabilities_set_relative(&capabilities, REL_X);
	CHECK(noct_beui_zedbsd_input_classify(&capabilities) == roles);
	noct_beui_zedbsd_input_capabilities_set_relative(&capabilities, REL_Y);
	roles |= NOCT_BEUI_ZEDBSD_INPUT_ROLE_RELATIVE_POINTER;
	CHECK(noct_beui_zedbsd_input_classify(&capabilities) == roles);

	memset(&information, 0, sizeof(information));
	information.minimum = -100;
	information.maximum = 100;
	noct_beui_zedbsd_input_capabilities_set_absolute(&capabilities, ABS_X,
							 &information);
	CHECK(noct_beui_zedbsd_input_classify(&capabilities) == roles);
	noct_beui_zedbsd_input_capabilities_set_absolute(&capabilities, ABS_Y,
							 &information);
	roles |= NOCT_BEUI_ZEDBSD_INPUT_ROLE_ABSOLUTE_POINTER;
	CHECK(noct_beui_zedbsd_input_classify(&capabilities) == roles);
}

static void
test_partial_multiple_and_relative(void)
{
	struct noct_beui_zedbsd_input_capabilities keyboard;
	struct noct_beui_zedbsd_input_capabilities pointer;
	struct noct_beui_zedbsd_input input;
	struct noct_beui_pointer_event pointer_event;
	struct input_event events[5];
	struct input_event key;
	unsigned update;
	int pointer_slot;
	int keyboard_slot;

	capabilities_begin(&pointer);
	noct_beui_zedbsd_input_capabilities_set_relative(&pointer, REL_X);
	noct_beui_zedbsd_input_capabilities_set_relative(&pointer, REL_Y);
	noct_beui_zedbsd_input_capabilities_set_key(&pointer, BTN_LEFT);
	capabilities_begin(&keyboard);
	noct_beui_zedbsd_input_capabilities_set_key(&keyboard, KEY_A);
	noct_beui_zedbsd_input_capabilities_set_key(&keyboard, KEY_RIGHTSHIFT);

	noct_beui_zedbsd_input_init(&input, 100, 80);
	/* Reverse the usual event numbering: pointer is attached first. */
	pointer_slot = noct_beui_zedbsd_input_attach(&input, &pointer);
	keyboard_slot = noct_beui_zedbsd_input_attach(&input, &keyboard);
	CHECK(pointer_slot == 0);
	CHECK(keyboard_slot == 1);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &pointer_event) == 1);
	CHECK(pointer_event.x == 50 && pointer_event.y == 40);

	key = event_make(EV_KEY, KEY_A, 1);
	update = noct_beui_zedbsd_input_feed(&input, (unsigned)keyboard_slot,
					     &key, sizeof(key) / 2U);
	CHECK(update == NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 0);
	update = noct_beui_zedbsd_input_feed(&input, (unsigned)keyboard_slot,
					     (const unsigned char *)&key +
						 sizeof(key) / 2U,
					     sizeof(key) - sizeof(key) / 2U);
	CHECK(update == NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 0);
	update =
	    feed_event(&input, (unsigned)keyboard_slot, EV_SYN, SYN_REPORT, 0);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_KEY) != 0U);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 1);

	/* Repeat is still down and is committed only at SYN_REPORT. */
	CHECK(feed_event(&input, (unsigned)keyboard_slot, EV_KEY, KEY_A, 2) ==
	      NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	CHECK(feed_event(&input, (unsigned)keyboard_slot, EV_SYN, SYN_REPORT,
			 0) == NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 1);

	events[0] = event_make(EV_MSC, 7, 1234); /* unknown: ignored */
	events[1] = event_make(EV_REL, REL_X, 10);
	events[2] = event_make(EV_REL, REL_Y, -1000);
	events[3] = event_make(EV_KEY, BTN_LEFT, 1);
	events[4] = event_make(EV_SYN, SYN_REPORT, 0);
	update = noct_beui_zedbsd_input_feed(&input, (unsigned)pointer_slot,
					     events, sizeof(events));
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_POINTER) != 0U);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &pointer_event) == 1);
	CHECK(pointer_event.x == 60 && pointer_event.y == 0);
	CHECK(pointer_event.buttons == NOCT_BEUI_BUTTON_LEFT);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &pointer_event) == 0);

	CHECK(feed_event(&input, (unsigned)keyboard_slot, EV_KEY,
			 KEY_RIGHTSHIFT,
			 1) == NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	(void)feed_event(&input, (unsigned)keyboard_slot, EV_SYN, SYN_REPORT,
			 0);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, NOCT_BEUI_KEY_SHIFT) ==
	      1);
	(void)feed_event(&input, (unsigned)keyboard_slot, EV_KEY, KEY_A, 0);
	(void)feed_event(&input, (unsigned)keyboard_slot, EV_KEY,
			 KEY_RIGHTSHIFT, 0);
	update =
	    feed_event(&input, (unsigned)keyboard_slot, EV_SYN, SYN_REPORT, 0);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_KEY) != 0U);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 0);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, NOCT_BEUI_KEY_SHIFT) ==
	      0);
}

static void
test_absolute_scaling(void)
{
	struct noct_beui_zedbsd_input_capabilities capabilities;
	struct noct_beui_zedbsd_input input;
	struct noct_beui_pointer_event event;
	struct input_absinfo x;
	struct input_absinfo y;
	struct input_event packet[3];
	int source;

	memset(&x, 0, sizeof(x));
	x.minimum = 0;
	x.maximum = 1000;
	x.value = 0;
	memset(&y, 0, sizeof(y));
	y.minimum = -100;
	y.maximum = 100;
	y.value = 0;
	capabilities_begin(&capabilities);
	noct_beui_zedbsd_input_capabilities_set_absolute(&capabilities, ABS_X,
							 &x);
	noct_beui_zedbsd_input_capabilities_set_absolute(&capabilities, ABS_Y,
							 &y);
	noct_beui_zedbsd_input_capabilities_set_key(&capabilities, BTN_RIGHT);
	noct_beui_zedbsd_input_init(&input, 101, 51);
	source = noct_beui_zedbsd_input_attach(&input, &capabilities);
	CHECK(source >= 0);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 0 && event.y == 25);

	packet[0] = event_make(EV_ABS, ABS_X, 500);
	packet[1] = event_make(EV_ABS, ABS_Y, 100);
	packet[2] = event_make(EV_SYN, SYN_REPORT, 0);
	CHECK((noct_beui_zedbsd_input_feed(&input, (unsigned)source, packet,
					   sizeof(packet)) &
	       NOCT_BEUI_ZEDBSD_INPUT_UPDATE_POINTER) != 0U);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 50 && event.y == 50);

	noct_beui_zedbsd_input_set_display(&input, 10, 10);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 9 && event.y == 9);
}

static void
test_drop_resync_detach_and_slots(void)
{
	struct noct_beui_zedbsd_input_capabilities combined;
	struct noct_beui_zedbsd_input input;
	struct noct_beui_pointer_event event;
	unsigned long queried[NOCT_BEUI_ZEDBSD_INPUT_KEY_WORDS];
	unsigned update;
	unsigned i;
	int source;

	capabilities_begin(&combined);
	noct_beui_zedbsd_input_capabilities_set_key(&combined, KEY_A);
	noct_beui_zedbsd_input_capabilities_set_key(&combined, BTN_LEFT);
	noct_beui_zedbsd_input_capabilities_set_relative(&combined, REL_X);
	noct_beui_zedbsd_input_capabilities_set_relative(&combined, REL_Y);
	noct_beui_zedbsd_input_init(&input, 64, 64);
	source = noct_beui_zedbsd_input_attach(&input, &combined);
	CHECK(source == 0);
	(void)noct_beui_zedbsd_input_poll_pointer(&input, &event);
	(void)feed_event(&input, 0, EV_KEY, KEY_A, 1);
	(void)feed_event(&input, 0, EV_KEY, BTN_LEFT, 1);
	(void)feed_event(&input, 0, EV_REL, REL_X, 10);
	(void)feed_event(&input, 0, EV_REL, REL_Y, -5);
	(void)feed_event(&input, 0, EV_SYN, SYN_REPORT, 0);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 1);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 42 && event.y == 27);

	update = feed_event(&input, 0, EV_SYN, SYN_DROPPED, 0);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_RESET) != 0U);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_KEY) != 0U);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_POINTER) != 0U);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 0);
	CHECK(input.pointer_buttons == 0U);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 32 && event.y == 32 && event.buttons == 0U);
	/* Events are ignored until a packet boundary asks the adapter to query.
	 */
	CHECK(feed_event(&input, 0, EV_KEY, KEY_A, 1) ==
	      NOCT_BEUI_ZEDBSD_INPUT_UPDATE_NONE);
	update = feed_event(&input, 0, EV_SYN, SYN_REPORT, 0);
	CHECK(update == NOCT_BEUI_ZEDBSD_INPUT_UPDATE_RESYNC);

	memset(queried, 0, sizeof(queried));
	bitmap_set(queried, KEY_A);
	bitmap_set(queried, BTN_LEFT);
	update = noct_beui_zedbsd_input_resync(&input, 0, queried,
					       sizeof(queried), NULL, NULL);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_KEY) != 0U);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_POINTER) != 0U);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == 1);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.buttons == NOCT_BEUI_BUTTON_LEFT);

	update = noct_beui_zedbsd_input_detach(&input, 0);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_KEY) != 0U);
	CHECK((update & NOCT_BEUI_ZEDBSD_INPUT_UPDATE_POINTER) != 0U);
	CHECK(input.pointer_buttons == 0U);
	CHECK(noct_beui_zedbsd_input_is_key_down(&input, 'a') == -1);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 1);
	CHECK(event.x == 32 && event.y == 32 && event.buttons == 0U);
	CHECK(noct_beui_zedbsd_input_poll_pointer(&input, &event) == 0);

	/* Slot use is bounded and detach makes exactly one slot reusable. */
	for (i = 0; i < NOCT_BEUI_ZEDBSD_INPUT_MAX_SOURCES; i++)
		CHECK(noct_beui_zedbsd_input_attach(&input, &combined) ==
		      (int)i);
	CHECK(noct_beui_zedbsd_input_attach(&input, &combined) == -1);
	(void)noct_beui_zedbsd_input_detach(&input, 7);
	CHECK(noct_beui_zedbsd_input_attach(&input, &combined) == 7);

	/* A partial record owned by a detached source cannot leak on reuse. */
	{
		struct input_event partial;

		partial = event_make(EV_KEY, KEY_A, 1);
		(void)noct_beui_zedbsd_input_feed(&input, 7, &partial, 1);
		(void)noct_beui_zedbsd_input_detach(&input, 7);
		CHECK(noct_beui_zedbsd_input_attach(&input, &combined) == 7);
		CHECK(input.sources[7].partial_event_size == 0U);
	}
}

int
main(void)
{
	test_capability_classification();
	test_partial_multiple_and_relative();
	test_absolute_scaling();
	test_drop_resync_detach_and_slots();
	puts("BeUI zedBSD input tests: OK");
	return 0;
}
