/* Spew3D is Copyright 2022 ell1e et al.

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or
sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall
be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS
OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef SPEW3D_IMPLEMENTATION

#define _FILE_OFFSET_BITS 64
#ifndef __USE_LARGEFILE64
#define __USE_LARGEFILE64 1
#endif
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#define _LARGEFILE_SOURCE
#if defined(_WIN32) || defined(_WIN64)
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#define fdopen64 fdopen
#else
#define fseek64 fseeko64
#define ftell64 ftello64
#define fdopen64 fdopen
#endif

#include <stdint.h>
#include <unistd.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#define h64filehandle HANDLE
#define H64_NOFILE (INVALID_HANDLE_VALUE)
#else
typedef int h64filehandle;
#define H64_NOFILE ((int)-1)
#endif
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(_WIN32) || defined(_WIN64)
#define _O_RDONLY 0x0000
#define _O_WRONLY 0x0001
#define _O_RDWR 0x0002
#define _O_APPEND 0x0008
#include <malloc.h>
#include <shlobj.h>
#if defined(__MING32__) || defined(__MINGW64__) || defined(_WIN32) || defined(_WIN64)
#ifndef ERROR_DIRECTORY_NOT_SUPPORTED
#define ERROR_DIRECTORY_NOT_SUPPORTED (0x150)
#endif
#endif
__declspec(dllimport) int _open_osfhandle(intptr_t osfhandle, int flags);
#else
#if !defined(ANDROID) && !defined(__ANDROID__)
#include <pwd.h>
#endif
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#endif
#if defined(__FreeBSD__) || defined(__FREEBSD__)
#include <sys/sysctl.h>
#endif


enum {
    _WIN32_OPEN_LINK_ITSELF = 0x1,
    _WIN32_OPEN_DIR = 0x2,
    OPEN_ONLY_IF_NOT_LINK = 0x4
};
char *AS_U8_FROM_U16(const uint16_t *s);
uint16_t *AS_U16(const char *s);


int spew3d_fs_IsObviouslyInvalidPath(const char *p) {
    int64_t i = 0;
    const int64_t plen = strlen(p);
    while (i < plen) {
        if (p[i] == '\0')
            return 1;
        #if defined(_WIN32) || defined(_WIN64)
        if (p[i] == '*' || p[i] == '%' ||
                (p[i] == ':' && i != 1) ||
                p[i] == '"' || p[i] == '?' ||
                p[i] == '|' || p[i] == '>' ||
                p[i] == '<')
            return 1;
        #endif
        i++;
    }
    return 0;
}


h64filehandle spew3d_fs_OpenFromPathAsOSHandleEx(
        const char *path, const char *mode, int flags, int *err
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: "
            "spew3d_fs_OpenFromPathAsOSHandleEx "
            "got invalid path: \"%s\"\n",
            path);
        #endif
        *err = FSERR_NOSUCHTARGET;
        return H64_NOFILE;
    }

    // Hack for "" referring to cwd:
    const char _cwdbuf[] = {'.', '\0'};
    if (strlen(path) == 0) {
        path = _cwdbuf;
    }

    int mode_read = (
        strstr(mode, "r") || strstr(mode, "a") || strstr(mode, "w+")
    );
    int mode_write = (
        strstr(mode, "w") || strstr(mode, "a") || strstr(mode, "r+")
    );
    int mode_append = strstr(mode, "r+") || strstr(mode, "a");
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: debug: "
        "spew3d_fs_OpenFromPathAsOSHandleEx(\"%s\", "
        "\"%s\", %d, %p) -> mode_read=%d mode_write=%d "
        "mode_append=%d\n",
        path, mode, flags, err, mode_read, mode_write,
        mode_append);
    #endif

    #if defined(_WIN32) || defined(_WIN64)
    assert(sizeof(uint16_t) == sizeof(wchar_t));
    uint16_t *pathw = AS_U16(path);
    if (!pathw) {
        *err = FSERR_OUTOFMEMORY;
        return H64_NOFILE;
    }
    HANDLE fhandle = INVALID_HANDLE_VALUE;
    if (!mode_write && !mode_append &&
            (flags & _WIN32_OPEN_DIR) != 0) {
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: "
            "spew3d_spew3d_fs_OpenFromPathAsOSHandleEx "
            "-> trying opening as dir\n");
        #endif
        fhandle = CreateFileW(
            pathw,
            GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, 0,
            OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS |
             // ^ first try: this is needed for dirs
            ((flags & (OPEN_ONLY_IF_NOT_LINK | _WIN32_OPEN_LINK_ITSELF)) ?
             FILE_FLAG_OPEN_REPARSE_POINT : 0),
            0
        );
        if (fhandle != INVALID_HANDLE_VALUE) {
            // If this is not a directory, throw away handle again:
            BY_HANDLE_FILE_INFORMATION finfo = {0};
            if (!GetFileInformationByHandle(fhandle, &finfo)) {
                uint32_t werror = GetLastError();
                *err = FSERR_OTHERERROR;
                if (werror == ERROR_ACCESS_DENIED ||
                        werror == ERROR_SHARING_VIOLATION)
                    *err = FSERR_NOPERMISSION;
                return H64_NOFILE;
            }
            if ((finfo.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) {
                // Oops, not a dir. We shouldn't open this like we have.
                // (We want to reopen without FILE_FLAG_BACKUP_SEMANTICS)
                CloseHandle(fhandle);
                fhandle = INVALID_HANDLE_VALUE;
            }
        }
    }
    if (fhandle == INVALID_HANDLE_VALUE) {
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: "
            "spew3d_fs_OpenFromPathAsOSHandleEx "
            "-> trying opening as file\n");
        #endif
        fhandle = CreateFileW(
            (LPCWSTR)pathw,
            0 | (mode_read ? GENERIC_READ : 0)
            | (mode_write ? GENERIC_WRITE : 0),
            (mode_write ? 0 : FILE_SHARE_READ),
            NULL,
            ((mode_write && !mode_append) ?
                CREATE_ALWAYS : OPEN_EXISTING),
            ((flags & (OPEN_ONLY_IF_NOT_LINK | _WIN32_OPEN_LINK_ITSELF)) ?
            FILE_FLAG_OPEN_REPARSE_POINT : 0),
            NULL
        );
    }
    free(pathw);
    pathw = NULL;
    if (fhandle == INVALID_HANDLE_VALUE) {
        uint32_t werror = GetLastError();
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: "
            "spew3d_fs_OpenFromPathAsOSHandleEx "
            "-> werror %d\n", (int)werror);
        #endif
        if (werror == ERROR_SHARING_VIOLATION ||
                werror == ERROR_ACCESS_DENIED) {
            *err = FSERR_NOPERMISSION;
        } else if (werror== ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND) {
            *err = FSERR_NOSUCHTARGET;
        } else if (werror == ERROR_TOO_MANY_OPEN_FILES) {
            *err = FSERR_OUTOFFDS;
        } else if (werror == ERROR_INVALID_NAME ||
                werror == ERROR_LABEL_TOO_LONG ||
                werror == ERROR_BUFFER_OVERFLOW ||
                werror == ERROR_FILENAME_EXCED_RANGE
                ) {
            *err = FSERR_INVALIDNAME;
        } else {
            *err = FSERR_OTHERERROR;
        }
        return H64_NOFILE;
    }
    if ((flags & OPEN_ONLY_IF_NOT_LINK) != 0) {
        if (_check_if_symlink_or_junction(fhandle)) {
            #if defined(DEBUG_FS)
            fprintf(stderr,
                "spew3d_filesystem.h: debug: "
                "spew3d_fs_OpenFromPathAsOSHandleEx "
                "-> is symlink and that was disabled\n");
            #endif
            *err = FSERR_SYMLINKSWEREEXCLUDED;
            return H64_NOFILE;
        }
    }
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: debug: "
        "spew3d_fs_OpenFromPathAsOSHandleEx "
        "-> success\n");
    #endif
    return fhandle;
    #else
    assert(mode_read || mode_write);
    int open_options = (
        ((mode_read && !mode_read && !mode_write) ? O_RDONLY : 0) |
        (((mode_write || mode_append) && !mode_read) ? O_WRONLY : 0) |
        (((mode_write || mode_append) && mode_read) ? O_RDWR : 0) |
        (mode_append ? O_APPEND : 0) |
        ((mode_write && !mode_append) ? O_CREAT : 0) |
        ((flags & OPEN_ONLY_IF_NOT_LINK) != 0 ? O_NOFOLLOW : 0) |
        O_LARGEFILE | O_NOCTTY |
        ((mode_write && !mode_append) ? O_TRUNC : 0)
    );
    int fd = -1;
    if ((open_options & O_CREAT) != 0) {
        fd = open64(path, open_options, 0664);
    } else {
        fd = open64(path, open_options);
    }
    if (fd < 0) {
        *err = FSERR_OTHERERROR;
        if (errno == ENOENT)
            *err = FSERR_NOSUCHTARGET;
        else if (errno == EMFILE || errno == ENFILE)
            *err = FSERR_OUTOFFDS;
        else if (errno == EACCES || errno == EPERM)
            *err = FSERR_NOPERMISSION;
        return H64_NOFILE;
    }
    return fd;
    #endif
}


FILE *spew3d_fs_OpenFromPath(
        const char *path, const char *mode, int *err
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        *err = FSERR_NOSUCHTARGET;
        return NULL;
    }

    int innererr = 0;
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: spew3d_fs_OpenFromPath(\"%s\", "
        "\"%s\", %p) -> spew3d_fs_OpenFromPathAsOSHandleEx\n",
        path, mode, err);
    #endif
    h64filehandle os_f = spew3d_fs_OpenFromPathAsOSHandleEx(
        path, mode, 0, &innererr
    );
    if (os_f == H64_NOFILE) {
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: "
            "spew3d_fs_OpenFromPathAsOSHandleEx "
            "says H64_NOFILE\n");
        #endif
        *err = innererr;
        return NULL;
    }

    #if defined(_WIN32) || defined(_WIN64)
    int mode_read = (
        strstr(mode, "r") || strstr(mode, "a") || strstr(mode, "w+")
    );
    int mode_write = (
        strstr(mode, "w") || strstr(mode, "a") || strstr(mode, "r+")
    );
    int mode_append = strstr(mode, "r+") || strstr(mode, "a");
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: debug: using _open_osfhandle with "
        "mode_read=%d, mode_write=%d, mode_append=%d\n",
        mode_read, mode_write, mode_append);
    #endif
    int filedescr = _open_osfhandle(
        (intptr_t)os_f,
        ((mode_read && !mode_read && !mode_write) ? _O_RDONLY : 0) |
        (((mode_write || mode_append) && !mode_read) ? _O_WRONLY : 0) |
        (((mode_write || mode_append) && mode_read) ? _O_RDWR : 0) |
        (mode_append ? _O_APPEND : 0)
    );
    if (filedescr < 0) {
        #if defined(DEBUG_FS)
        fprintf(stderr,
            "spew3d_filesystem.c: debug: _open_osfhandle failed\n");
        #endif
        *err = FSERR_OTHERERROR;
        CloseHandle(os_f);
        return NULL;
    }
    os_f = NULL;  // now owned by 'filedescr'
    errno = 0;
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: debug: using _fdopen with mode=%s\n",
        mode);
    #endif
    FILE *fresult = _fdopen(filedescr, mode);
    if (!fresult) {
        *err = FSERR_OTHERERROR;
        _close(filedescr);
        return NULL;
    }
    return fresult;
    #else
    #if defined(DEBUG_FS)
    fprintf(stderr,
        "spew3d_filesystem.c: debug: using fdopen64 with mode=%s\n",
        mode);
    #endif
    FILE *f = fdopen64(os_f, mode);
    if (!f) {
        *err = FSERR_OTHERERROR;
        if (errno == ENOENT)
            *err = FSERR_NOSUCHTARGET;
        else if (errno == EMFILE || errno == ENFILE)
            *err = FSERR_OUTOFFDS;
        else if (errno == EACCES || errno == EPERM)
            *err = FSERR_NOPERMISSION;
        return NULL;
    }
    return f;
    #endif
}


#endif  // SPEW3D_IMPLEMENTATION
