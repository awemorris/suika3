#ifndef _WINDOWS_H
#define _WINDOWS_H

#define WINAPI __stdcall

typedef unsigned long DWORD;
typedef unsigned long long SIZE_T;
typedef void *LPVOID;
typedef const void *LPCVOID;
typedef DWORD *PDWORD;
typedef char BOOL;
typedef void *HANDLE;

#define PAGE_READWRITE		0x04
#define PAGE_EXECUTE_READ	0x20

#define MEM_COMMIT		0x00001000
#define MEM_RELEASE		0x00008000

WINAPI LPVOID VirtualAlloc(
  [in, optional] LPVOID lpAddress,
  [in]           SIZE_T dwSize,
  [in]           DWORD  flAllocationType,
  [in]           DWORD  flProtect
);

WINAPI BOOL VirtualProtect(
  [in]  LPVOID lpAddress,
  [in]  SIZE_T dwSize,
  [in]  DWORD  flNewProtect,
  [out] PDWORD lpflOldProtect
);

WINAPI BOOL VirtualFree(
  [in] LPVOID lpAddress,
  [in] SIZE_T dwSize,
  [in] DWORD  dwFreeType
);

WINAPI BOOL FlushInstructionCache(
  [in] HANDLE  hProcess,
  [in] LPCVOID lpBaseAddress,
  [in] SIZE_T  dwSize
);

WINAPI HANDLE GetCurrentProcess(void);

#endif
