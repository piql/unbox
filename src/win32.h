#ifndef WIN32_H
#define WIN32_H

#include <stdint.h>
typedef struct {
  uint32_t nLength;
  void *lpSecurityDescriptor;
  int32_t bInheritHandle;
} SECURITY_ATTRIBUTES;

typedef union {
  struct {
    uint32_t LowPart;
    int32_t HighPart;
  };
  int64_t QuadPart;
} LARGE_INTEGER;

typedef struct {
  uint32_t dwLowDateTime;
  uint32_t dwHighDateTime;
} FILETIME;

#define MAX_PATH 260

typedef struct {
  uint32_t dwFileAttributes;
  FILETIME ftCreationTime;
  FILETIME ftLastAccessTime;
  FILETIME ftLastWriteTime;
  uint32_t nFileSizeHigh;
  uint32_t nFileSizeLow;
  uint32_t dwReserved0;
  uint32_t dwReserved1;
  char cFileName[MAX_PATH];
  char cAlternateFileName[14];
} WIN32_FIND_DATAA;

#define WINBASEAPI __declspec(dllimport)
#define WINAPI __stdcall

typedef uint32_t(WINAPI *LPTHREAD_START_ROUTINE)(void *lpThreadParameter);

WINBASEAPI void *WINAPI CreateFileA(
    const char *lpFileName, uint32_t dwDesiredAccess, uint32_t dwShareMode,
    SECURITY_ATTRIBUTES *lpSecurityAttributes, uint32_t dwCreationDisposition,
    uint32_t dwFlagsAndAttributes, void *hTemplateFile);
WINBASEAPI int32_t WINAPI GetFileSizeEx(void *hFile, LARGE_INTEGER *lpFileSize);
WINBASEAPI uint32_t WINAPI GetCurrentDirectoryA(uint32_t nBufferLength, char *lpBuffer);
WINBASEAPI int32_t WINAPI CloseHandle(void *hObject);
WINBASEAPI void *WINAPI
CreateFileMappingA(void *hFile, SECURITY_ATTRIBUTES *lpFileMappingAttributes,
                   uint32_t flProtect, uint32_t dwMaximumSizeHigh,
                   uint32_t dwMaximumSizeLow, const char *lpName);
WINBASEAPI void *WINAPI MapViewOfFileFromApp(void *hFileMappingObject,
                                             uint32_t DesiredAccess,
                                             uint64_t FileOffset,
                                             uintptr_t NumberOfBytesToMap);
WINBASEAPI int32_t WINAPI UnmapViewOfFile(void *lpBaseAddress);
WINBASEAPI uint32_t WINAPI GetFileAttributesA(const char *lpFileName);
WINBASEAPI uint32_t WINAPI WaitForSingleObject(void *hHandle,
                                               uint32_t dwMilliseconds);
WINBASEAPI void *WINAPI CreateMutexA(SECURITY_ATTRIBUTES *lpMutexAttributes,
                                     int32_t bInitialOwner, const char *lpName);
WINBASEAPI int32_t WINAPI ReleaseMutex(void *hMutex);
WINBASEAPI void *WINAPI CreateThread(SECURITY_ATTRIBUTES *lpThreadAttributes,
                                     uintptr_t dwStackSize,
                                     LPTHREAD_START_ROUTINE lpStartAddress,
                                     void *lpParameter,
                                     uint32_t dwCreationFlags,
                                     uint32_t *lpThreadId);
WINBASEAPI uint32_t WINAPI WaitForMultipleObjects(uint32_t nCount,
                                                  const void **lpHandles,
                                                  int32_t bWaitAll,
                                                  uint32_t dwMilliseconds);
WINBASEAPI void *WINAPI FindFirstFileA(const char *lpFileName,
                                       WIN32_FIND_DATAA *lpFindFileData);
WINBASEAPI int32_t WINAPI FindNextFileA(void *hFindFile,
                                        WIN32_FIND_DATAA *lpFindFileData);
WINBASEAPI int32_t WINAPI FindClose(void *hFindFile);
WINBASEAPI int32_t WINAPI SetConsoleOutputCP(unsigned int wCodePageID);

#define GENERIC_READ (0x80000000L)
#define GENERIC_WRITE (0x40000000L)
#define GENERIC_EXECUTE (0x20000000L)
#define GENERIC_ALL (0x10000000L)

#define PAGE_NOACCESS 0x01
#define PAGE_READONLY 0x02
#define PAGE_READWRITE 0x04
#define PAGE_WRITECOPY 0x08
#define PAGE_EXECUTE 0x10
#define PAGE_EXECUTE_READ 0x20
#define PAGE_EXECUTE_READWRITE 0x40
#define PAGE_EXECUTE_WRITECOPY 0x80
#define PAGE_GUARD 0x100
#define PAGE_NOCACHE 0x200
#define PAGE_WRITECOMBINE 0x400

#define FILE_SHARE_READ 0x00000001
#define FILE_SHARE_WRITE 0x00000002
#define FILE_SHARE_DELETE 0x00000004

#define FILE_ATTRIBUTE_READONLY 0x00000001
#define FILE_ATTRIBUTE_HIDDEN 0x00000002
#define FILE_ATTRIBUTE_SYSTEM 0x00000004
#define FILE_ATTRIBUTE_DIRECTORY 0x00000010
#define FILE_ATTRIBUTE_ARCHIVE 0x00000020
#define FILE_ATTRIBUTE_DEVICE 0x00000040
#define FILE_ATTRIBUTE_NORMAL 0x00000080
#define FILE_ATTRIBUTE_TEMPORARY 0x00000100
#define FILE_ATTRIBUTE_SPARSE_FILE 0x00000200
#define FILE_ATTRIBUTE_REPARSE_POINT 0x00000400
#define FILE_ATTRIBUTE_COMPRESSED 0x00000800
#define FILE_ATTRIBUTE_OFFLINE 0x00001000
#define FILE_ATTRIBUTE_NOT_CONTENT_INDEXED 0x00002000
#define FILE_ATTRIBUTE_ENCRYPTED 0x00004000
#define FILE_ATTRIBUTE_INTEGRITY_STREAM 0x00008000
#define FILE_ATTRIBUTE_VIRTUAL 0x00010000
#define FILE_ATTRIBUTE_NO_SCRUB_DATA 0x00020000
#define FILE_ATTRIBUTE_EA 0x00040000
#define FILE_ATTRIBUTE_PINNED 0x00080000
#define FILE_ATTRIBUTE_UNPINNED 0x00100000
#define FILE_ATTRIBUTE_RECALL_ON_OPEN 0x00040000
#define FILE_ATTRIBUTE_RECALL_ON_DATA_ACCESS 0x00400000

#define SECTION_QUERY 0x0001
#define SECTION_MAP_WRITE 0x0002
#define SECTION_MAP_READ 0x0004
#define SECTION_MAP_EXECUTE 0x0008
#define SECTION_EXTEND_SIZE 0x0010
#define SECTION_MAP_EXECUTE_EXPLICIT 0x0020

#define FILE_MAP_WRITE SECTION_MAP_WRITE
#define FILE_MAP_READ SECTION_MAP_READ
#define FILE_MAP_ALL_ACCESS SECTION_ALL_ACCESS

#define FILE_MAP_EXECUTE SECTION_MAP_EXECUTE_EXPLICIT

#define FILE_MAP_COPY 0x00000001

#define FILE_MAP_RESERVE 0x80000000
#define FILE_MAP_TARGETS_INVALID 0x40000000
#define FILE_MAP_LARGE_PAGES 0x20000000

#define CREATE_NEW 1
#define CREATE_ALWAYS 2
#define OPEN_EXISTING 3
#define OPEN_ALWAYS 4
#define TRUNCATE_EXISTING 5

#define INVALID_HANDLE_VALUE ((void *)(intptr_t)-1)

#define INVALID_FILE_SIZE ((uint32_t)0xFFFFFFFF)
#define INVALID_SET_FILE_POINTER ((uint32_t)-1)
#define INVALID_FILE_ATTRIBUTES ((uint32_t)-1)

#define STATUS_WAIT_0 ((uint32_t)0x00000000L)
#define WAIT_OBJECT_0 ((STATUS_WAIT_0) + 0)
#define INFINITE 0xFFFFFFFF

#define FALSE 0
#define TRUE 1

#define CP_ACP 0        // default to ANSI code page
#define CP_OEMCP 1      // default to OEM  code page
#define CP_MACCP 2      // default to MAC  code page
#define CP_THREAD_ACP 3 // current thread's ANSI code page
#define CP_SYMBOL 42    // SYMBOL translations

#define CP_UTF7 65000 // UTF-7 translation
#define CP_UTF8 65001 // UTF-8 translation

int __cdecl mkdir(const char *_Path);

#define mkdir(path, mode) mkdir(path)

#endif // WIN32_H
