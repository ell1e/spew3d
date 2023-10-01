/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.

Alternatively, at your option, this file is offered under the Apache 2
license, see accompanied LICENSE.md.
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
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
#define fseek64 _fseeki64
#define ftell64 _ftelli64
#else
#define fdopen64 fdopen
#ifndef fseek64
#define fseek64 fseeko64
#endif
#ifndef ftell64
#define ftell64 ftello64
#endif
#endif

#include <stdint.h>
#include <unistd.h>
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#ifndef FSCTL_GET_REPARSE_POINT
// Needed for MinGW:
#include <winioctl.h>
#endif
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
S3DEXP char *AS_U8_FROM_U16(const uint16_t *s);
S3DEXP uint16_t *AS_U16(const char *s);

S3DHID int _spew3d_fs_RemoveFileEx(
    const char *path, int *error,
    int allowdirs
);

S3DEXP int spew3d_fs_IsObviouslyInvalidPath(const char *p) {
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

#if defined(_WIN32) || defined(_WIN64)
S3DHID int _spew3d_IsHandleSymlinkOrJunction(HANDLE fhandle) {
    DWORD written = 0;
    char reparse_buf[MAXIMUM_REPARSE_DATA_BUFFER_SIZE] = {0};
    struct reparse_tag {
        ULONG tag;
    };
    if (!DeviceIoControl(
            fhandle, FSCTL_GET_REPARSE_POINT, NULL, 0,
            (LPVOID)reparse_buf, sizeof(reparse_buf),
            &written, 0
            )) {
        // Assume there is no reparse point.
    } else {
        struct reparse_tag *tagcheck = (
            (struct reparse_tag *)reparse_buf
        );
        if (tagcheck->tag == IO_REPARSE_TAG_SYMLINK ||
                tagcheck->tag == IO_REPARSE_TAG_MOUNT_POINT) {
            CloseHandle(fhandle);
            return 1;
        }
    }
    return 0;
}
#endif

S3DHID h64filehandle spew3d_fs_OpenFromPathAsOSHandleEx(
        const char *path, const char *mode, int flags, int *err
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: "
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
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: debug: "
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
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: "
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
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: "
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
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: "
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
        if (_spew3d_IsHandleSymlinkOrJunction(fhandle)) {
            #if defined(DEBUG_SPEW3D_FS)
            fprintf(stderr,
                "spew3d_filesystem.h: debug: "
                "spew3d_fs_OpenFromPathAsOSHandleEx "
                "-> is symlink and that was disabled\n");
            #endif
            *err = FSERR_SYMLINKSWEREEXCLUDED;
            return H64_NOFILE;
        }
    }
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: debug: "
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

S3DEXP int filesys_IsLink(
        const char *path, int *result
        ) {
    #if defined(_WIN32) || defined(_WIN64)
    int err = 0;
    h64filehandle h = spew3d_fs_OpenFromPathAsOSHandleEx(
        path, "rb", 0, &err
    );
    if (h == H64_NOFILE)
        return 0;
    *result = _spew3d_IsHandleSymlinkOrJunction(h);
    CloseHandle(h);
    return 1;
    #else
    struct stat buf;
    int statresult = lstat(path, &buf);
    if (statresult < 0)
        return 0;
    if (result)
        *result = S_ISLNK(buf.st_mode);
    return 1;
    #endif
}

S3DEXP FILE *spew3d_fs_OpenFromPath(
        const char *path, const char *mode, int *err
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        *err = FSERR_NOSUCHTARGET;
        return NULL;
    }

    int innererr = 0;
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: spew3d_fs_OpenFromPath(\"%s\", "
        "\"%s\", %p) -> spew3d_fs_OpenFromPathAsOSHandleEx\n",
        path, mode, err);
    #endif
    h64filehandle os_f = spew3d_fs_OpenFromPathAsOSHandleEx(
        path, mode, 0, &innererr
    );
    if (os_f == H64_NOFILE) {
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: "
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
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: debug: using _open_osfhandle with "
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
        #if defined(DEBUG_SPEW3D_FS)
        fprintf(stderr,
            "spew3d_fs.c: debug: _open_osfhandle failed\n");
        #endif
        *err = FSERR_OTHERERROR;
        CloseHandle(os_f);
        return NULL;
    }
    os_f = NULL;  // now owned by 'filedescr'
    errno = 0;
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: debug: using _fdopen with mode=%s\n",
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
    #if defined(DEBUG_SPEW3D_FS)
    fprintf(stderr,
        "spew3d_fs.c: debug: using fdopen64 with mode=%s\n",
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

S3DEXP int spew3d_fs_GetSize(
        const char *path, uint64_t *size, int *err
        ) {
    #if defined(ANDROID) || defined(__ANDROID__) || \
        defined(__unix__) || defined(__linux__) || \
        defined(__APPLE__) || defined(__OSX__)
    struct stat64 statbuf;

    if (stat64(path, &statbuf) == -1) {
        *err = FSERR_OTHERERROR;
        if (errno == ENOENT)
            *err = FSERR_NOSUCHTARGET;
        else if (errno == EACCES || errno == EPERM)
            *err = FSERR_NOPERMISSION;
        return 0;
    }
    if (!S_ISREG(statbuf.st_mode)) {
        *err = FSERR_TARGETNOTAFILE;
        return 0;
    }
    *size = (uint64_t)statbuf.st_size;
    return 1;
    #else
    wchar_t *wpath = AS_U16(path);
    if (!wpath)
        return 0;
    WIN32_FILE_ATTRIBUTE_DATA fad;
    if (!GetFileAttributesExW(wpath, GetFileExInfoStandard, &fad)) {
        free(wpath);
        uint32_t werror = GetLastError();
        *err = FSERR_OTHERERROR;
        if (werror == ERROR_ACCESS_DENIED ||
                werror == ERROR_SHARING_VIOLATION) {
            *err = FSERR_NOPERMISSION;
        } else if (werror == ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND ||
                werror == ERROR_INVALID_NAME ||
                werror == ERROR_INVALID_DRIVE) {
            *err = FSERR_NOSUCHTARGET;
        }
        return 0;
    }
    free(wpath);
    LARGE_INTEGER v;
    v.HighPart = fad.nFileSizeHigh;
    v.LowPart = fad.nFileSizeLow;
    *size = (uint64_t)v.QuadPart;
    return 1;
    #endif
}

S3DEXP int spew3d_fs_GetComponentCount(const char *path) {
    int i = 0;
    int component_count = 0;
    #if defined(_WIN32) || defined(_WIN64)
    if (path[0] != '/' && path[0] != '\\' &&
            path[1] == ':' && (path[2] == '/' ||
            path[2] == '\\'))
        i = 2;
    #endif
    while (i < (int)strlen(path)) {
        if (i > 0 && path[i] != '/' &&
                #if defined(_WIN32) || defined(_WIN64)
                path[i] != '\\' &&
                #endif
                (path[i - 1] == '/'
                #if defined(_WIN32) || defined(_WIN64)
                || path[i - 1] == '\\'
                #endif
                )) {
            component_count++;
        }
        i++;
    }
    return component_count;
}

S3DEXP char *spew3d_fs_RemoveDoubleSlashes(const char *path) {
    if (!path)
        return NULL;
    char *p = strdup(path);
    if (!p)
        return NULL;

    // Remove double slashes:
    int lastwassep = 0;
    int i = 0;
    while (i < (int)strlen(p)) {
        if (p[i] == '/'
                #if defined(_WIN32) || defined(_WIN64)
                || p[i] == '\\'
                #endif
                ) {
            #if defined(_WIN32) || defined(_WIN64)
            p[i] = '\\';
            #endif
            if (!lastwassep) {
                lastwassep = 1;
            } else {
                memmove(
                    p + i, p + i + 1,
                    strlen(p) - i
                );
                continue;
            }
        } else {
            lastwassep = 0;
        }
        i++;
    }
    if (strlen(p) > 1 && (
            p[strlen(p) - 1] == '/'
            #if defined(_WIN32) || defined(_WIN64)
            || p[strlen(p) - 1] == '\\'
            #endif
            )) {
        p[strlen(p) - 1] = '\0';
    }
    return p;
}

S3DEXP char *spew3d_fs_Normalize(const char *path) {
    return spew3d_fs_NormalizeEx(path, 0, 0,
        #if defined(_WIN32) || defined(_WIN64)
        '\\'
        #else
        '/'
        #endif
    );
}

S3DEXP char *spew3d_fs_NormalizeEx(
        const char *path, int always_allow_windows_separator,
        int never_allow_windows_separator,
        char unified_separator_to_use
        ) {
    assert(!always_allow_windows_separator ||
        !never_allow_windows_separator);
    char *result = spew3d_fs_RemoveDoubleSlashes(path);
    if (!result)
        return NULL;

    int allow_windows_separator = (
        #if defined(_WIN32) || defined(_WIN64)
        !never_allow_windows_separator
        #else
        always_allow_windows_separator
        #endif
    );

    // Remove all unnecessary ../ and ./ inside the path:
    int last_component_start = -1;
    int i = 0;
    while (i < (int)strlen(result)) {
        if ((result[i] == '/' ||
                (allow_windows_separator && result[i] == '\\')) &&
                result[i + 1] == '.' &&
                result[i + 2] == '.' && (
                result[i + 3] == '/' ||
                (allow_windows_separator && result[i + 3] == '\\') ||
                result[i + 3] == '\0'
                ) && i > last_component_start && i > 0 &&
                (result[last_component_start + 1] != '.' ||
                 result[last_component_start + 2] != '.' ||
                 (result[last_component_start + 3] != '/' &&
                  (allow_windows_separator &&
                   result[last_component_start + 3] != '\\')
                 )
                )) {
            // Collapse ../ into previous component:
            int movelen = 4;
            if (result[i + 3] == '\0')
                movelen = 3;
            memmove(result + last_component_start + 1,
                    result + (i + movelen),
                    strlen(result) - (i + movelen) + 1);
            // Start over from beginning:
            i = 0;
            last_component_start = 0;
            continue;
        } else if ((result[i] == '/' ||
                (allow_windows_separator && result[i] == '\\')
                ) && result[i + 1] == '.' && (
                result[i + 2] == '/' ||
                (allow_windows_separator && result[i + 1] == '\\')
                )) {
            // Collapse unncessary ./ away:
            last_component_start = i;
            memmove(result + i, result + (i + 2),
                    strlen(result) - (i - 2) + 1);
            continue;
        } else if (result[i] == '/' ||
                (allow_windows_separator && result[i] == '\\')
                ) {
            last_component_start = i;
            // Collapse all double slashes away:
            while (result[i + 1] == '/' ||
                    (allow_windows_separator && result[i + 1] == '\\')
                    ) {
                memmove(result + i, result + (i + 1),
                        strlen(result) - (i - 1) + 1);
            }
        }
        i++;
    }

    // Remove leading ./ instances:
    while (strlen(result) >= 2 && result[0] == '.' && (
            result[1] == '/' ||
            (allow_windows_separator && result[1] == '\\')
            )) {
        memmove(result, result + 2, strlen(result) + 1 - 2);
    }

    // Unify path separators:
    i = 0;
    while (i < (int)strlen(result)) {
        if (result[i] == '/' ||
                (allow_windows_separator && result[i] == '\\')) {
            result[i] = unified_separator_to_use;
        }
        i++;
    }

    // Remove trailing path separators:
    while (strlen(result) > 0) {
        if (result[strlen(result) - 1] == unified_separator_to_use) {
            result[strlen(result) - 1] = '\0';
        } else {
            break;
        }
    }
    return result;
}

S3DEXP int spew3d_fs_TargetExists(const char *path, int *exists) {
    return (spew3d_fs_TargetExistsEx(path, exists, 0));
}

S3DEXP int spew3d_fs_TargetExistsEx(
        const char *path, int *exists, int noperms_as_ioerror
        ) {
    #if defined(ANDROID) || defined(__ANDROID__) || \
        defined(__unix__) || defined(__linux__) || \
        defined(__APPLE__) || defined(__OSX__)
    struct stat sb;
    if (stat(path, &sb) != 0) {
        if (errno == ENOENT || errno == ENOTDIR ||
                (!noperms_as_ioerror && errno == EACCES)) {
            *exists = 0;
            return 1;
        }
        return 0;
    }
    *exists = 1;
    return 1;
    #elif defined(_WIN32) || defined(_WIN64)
    wchar_t *wpath = AS_U16(path);
    if (!wpath)
        return 0;
    DWORD dwAttrib = GetFileAttributesW(wpath);
    free(wpath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        uint32_t werror = GetLastError();
        if (werror == ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND ||
                werror == ERROR_INVALID_NAME ||
                werror == ERROR_INVALID_DRIVE ||
                (!noperms_as_ioerror && werror ==
                ERROR_ACCESS_DENIED)) {
            *exists = 0;
            return 1;
        }
        return 0;
    }
    *exists = 1;
    return 1;
    #else
    #error "unsupported platform"
    #endif
}

S3DEXP int spew3d_fs_IsDirectory(const char *path, int *result) {
    #if defined(ANDROID) || defined(__ANDROID__) || defined(__unix__) || \
            defined(__linux__) || defined(__APPLE__) || defined(__OSX__)
    struct stat sb;
    if (stat(path, &sb) < 0) {
        if (errno == ENOENT) {
            *result = 0;
            return 1;
        }
        return 0;
    }
    *result = (S_ISDIR(sb.st_mode));
    return 1;
    #elif defined(_WIN32) || defined(_WIN64)
    wchar_t *wpath = AS_U16(path);
    if (!wpath)
        return 0;
    DWORD dwAttrib = GetFileAttributesW(wpath);
    free(wpath);
    if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
        uint32_t werror = GetLastError();
        if (werror == ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND ||
                werror == ERROR_INVALID_NAME ||
                werror == ERROR_INVALID_DRIVE) {
            *result = 0;
            return 1;
        }
        return 0;
    }
    *result = (dwAttrib & FILE_ATTRIBUTE_DIRECTORY);
    return 1;
    #else
    #error "unsupported platform"
    #endif
}

S3DEXP int spew3d_fs_CreateDirectoryEx(
        const char *path, int onlyuserreadable
        ) {
    #if defined(ANDROID) || defined(__ANDROID__) || \
        defined(__unix__) || defined(__linux__) || \
        defined(__APPLE__) || defined(__OSX__)
    return (
        mkdir(path, (onlyuserreadable ? 0700 : 0755)) == 0
    );
    #elif defined(_WIN32) || defined(_WIN64)
    // FIXME: implement only user readable permission
    wchar_t *pathw = AS_U16(path);
    if (!pathw) return 0;
    int result = (CreateDirectoryW(pathw, NULL) != 0);
    free(pathw);
    return result;
    #else
    #error "unsupported platform"
    #endif
}

S3DEXP int spew3d_fs_CreateDirectory(const char *path) {
    return spew3d_fs_CreateDirectoryEx(path, 0);
}

S3DEXP void spew3d_fs_FreeFolderList(char **list) {
    int i = 0;
    while (list[i]) {
        free(list[i]);
        i++;
    }
    free(list);
}

S3DEXP int spew3d_fs_ListFolderEx(
        const char *path, char ***contents,
        int returnFullPath, int allowsymlink,
        int *error
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        *error = FSERR_TARGETNOTADIRECTORY;
        return 0;
    }

    // Hack for "" referring to cwd:
    static char dotfolder[] = {'.', '\0'};
    if (strlen(path) == 0) {
        path = dotfolder;
    }

    // Start scanning the files:
    #if defined(_WIN32) || defined(_WIN64)
    assert(sizeof(wchar_t) == sizeof(uint16_t));
    wchar_t *folderpath = (wchar_t*)AS_U16(path);
    if (!folderpath) {
        return 0;
    }
    wchar_t *p = malloc(wcslen(folderpath) * sizeof(*p) + 3);
    if (!p) {
        free(folderpath);
        *error = FSERR_OUTOFMEMORY;
        return 0;
    }
    memcpy(p, folderpath, wcslen(folderpath) + 1);
    if (p[wcslen(p) - 1] != '\\') {
        p[wcslen(p) + 1] = '\0';
        p[wcslen(p)] = '\\';
    }
    p[wcslen(p) + 1] = '\0';
    p[wcslen(p)] = '*';
    free((void *)folderpath);
    folderpath = NULL;
    LPWIN32_FIND_DATAW ffd = {0};
    int isfirst = 1;
    HANDLE hFind = FindFirstFileW(p, ffd);
    // FIXME: implement allowsymlink for windows
    if (hFind == INVALID_HANDLE_VALUE) {
        free(p);
        uint64_t werror = GetLastError();
        if (werror == ERROR_NO_MORE_FILES) {
            // Special case, empty directory.
            *contents = malloc(sizeof(*contents) * 1);
            if (!*contents) {
                free(*contents);
                *error = FSERR_OUTOFMEMORY;
                return 0;
            }
            (*contents)[0] = NULL;
            *error = FSERR_SUCCESS;
            return 1;
        }
        *error = FSERR_OTHERERROR;
        if (werror == ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND)
            *error = FSERR_NOSUCHTARGET;
        else if (werror == ERROR_INVALID_PARAMETER ||
                werror == ERROR_INVALID_NAME ||
                werror == ERROR_INVALID_DRIVE ||
                werror == ERROR_DIRECTORY_NOT_SUPPORTED ||
                werror == ERROR_DIRECTORY)
            *error = FSERR_TARGETNOTADIRECTORY;
        else if (werror == ERROR_ACCESS_DENIED ||
                werror == ERROR_SHARING_VIOLATION)
            *error = FSERR_NOPERMISSION;
        else if (werror == ERROR_NOT_ENOUGH_MEMORY)
            *error = FSERR_OUTOFMEMORY;
        else if (werror == ERROR_TOO_MANY_OPEN_FILES)
            *error = FSERR_OUTOFFDS;
        return 0;
    }
    free(p);
    #else
    char *p = strdup(path);
    errno = 0;
    DIR *d = NULL;
    if (allowsymlink) {
        // Allow using regular mechanism:
        d = opendir(p);
    } else {
        // Open as fd first, so we can avoid symlinks.
        errno = 0;
        int dirfd = open64(
            p, O_RDONLY | O_NOFOLLOW | O_LARGEFILE | O_NOCTTY
        );
        if (dirfd < 0) {
            free(p);
            *error = FSERR_OTHERERROR;
            if (errno == EMFILE || errno == ENFILE)
                *error = FSERR_OUTOFFDS;
            else if (errno == ENOMEM)
                *error = FSERR_OUTOFMEMORY;
            else if (errno == ELOOP)
                *error = FSERR_SYMLINKSWEREEXCLUDED;
            else if (errno == EACCES)
                *error = FSERR_NOPERMISSION;
            return 0;
        }
        d = fdopendir(dirfd);
    }
    free(p);
    if (!d) {
        *error = FSERR_OTHERERROR;
        if (errno == EMFILE || errno == ENFILE)
            *error = FSERR_OUTOFFDS;
        else if (errno == ENOMEM)
            *error = FSERR_OUTOFMEMORY;
        else if (errno == ENOTDIR || errno == ENOENT)
            *error = FSERR_TARGETNOTADIRECTORY;
        else if (errno == EACCES)
            *error = FSERR_NOPERMISSION;
        return 0;
    }
    #endif
    // Now, get all the files entries and put them into the list:
    char **list = malloc(sizeof(*list));
    if (!list) {
        free(list);
        *error = FSERR_OUTOFMEMORY;
        return 0;
    }
    list[0] = NULL;
    char **fullPathList = NULL;
    int entriesSoFar = 0;
    while (1) {
        #if defined(_WIN32) || defined(_WIN64)
        if (isfirst) {
            isfirst = 0;
        } else {
            if (FindNextFileW(hFind, ffd) == 0) {
                uint32_t werror = GetLastError();
                if (werror != ERROR_NO_MORE_FILES) {
                    *error = FSERR_OTHERERROR;
                    if (werror == ERROR_NOT_ENOUGH_MEMORY)
                        *error = FSERR_OUTOFMEMORY;
                    goto errorquit;
                }
                break;
            }
        }
        const wchar_t *entryNameWide = ffd->cFileName;
        char *entryName = AS_U8_FROM_U16(
            (uint16_t *)entryNameWide
        );
        if (!entryName) {
            *error = FSERR_OUTOFMEMORY;
            goto errorquit;
        }
        #else
        errno = 0;
        struct dirent *entry = readdir(d);
        if (!entry && errno != 0) {
            *error = FSERR_OUTOFMEMORY;
            goto errorquit;
        }
        if (!entry)
            break;
        if (strcmp(entry->d_name, ".") == 0 ||
                strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        char *entryName = strdup(entry->d_name);
        if (!entryName) {
            *error = FSERR_OUTOFMEMORY;
            goto errorquit;
        }
        #endif
        char **nlist = realloc(
            list, sizeof(*nlist) * (entriesSoFar + 2)
        );
        if (nlist)
            list = nlist;
        if (!nlist) {
            *error = FSERR_OUTOFMEMORY;
            free(entryName);
            errorquit: ;
            #if defined(_WIN32) || defined(_WIN64)
            if (hFind != INVALID_HANDLE_VALUE)
                FindClose(hFind);
            #else
            if (d)
                closedir(d);
            #endif
            if (list) {
                int k = 0;
                while (k < entriesSoFar) {
                    if (list[k])
                        free(list[k]);
                    else
                        break;
                    k++;
                }
                free(list);
            }
            if (fullPathList) {
                int k = 0;
                while (fullPathList[k]) {
                    free(fullPathList[k]);
                    k++;
                }
                free(fullPathList);
            }
            return 0;
        }
        list = nlist;
        entriesSoFar++;
        list[entriesSoFar] = NULL;
        list[entriesSoFar - 1] = entryName;
    }

    // Ok, done with extracting all names:
    #if defined(_WIN32) || defined(_WIN64)
    FindClose(hFind);
    hFind = INVALID_HANDLE_VALUE;
    #else
    closedir(d);
    d = NULL;
    #endif

    // Convert names to full path if requested:
    if (!returnFullPath) {
        // No conversion needed:
        *contents = list;
        *error = FSERR_SUCCESS;
    } else {
        // Ok, allocate new array for conversion:
        fullPathList = malloc(sizeof(*fullPathList) * (entriesSoFar + 1));
        if (!fullPathList) {
            *error = FSERR_OUTOFMEMORY;
            goto errorquit;
        }
        memset(fullPathList, 0, sizeof(*fullPathList) * (entriesSoFar + 1));
        int pathlen = strlen(path);
        int k = 0;
        while (k < entriesSoFar) {
            fullPathList[k] = malloc(
                (pathlen + 1 + strlen(list[k]) + 1) * sizeof(*list[k])
            );
            if (!fullPathList[k]) {
                *error = FSERR_OUTOFMEMORY;
                goto errorquit;
            }
            memcpy(fullPathList[k], path, sizeof(*path) * pathlen);
            int lensofar = strlen(path);
            #if defined(_WIN32) || defined(_WIN64)
            if (pathlen > 0 && fullPathList[k][pathlen - 1] != '/' &&
                    fullPathList[k][pathlen - 1] != '\\') {
                fullPathList[k][pathlen] = '\\';
                lensofar++;
            }
            #else
            if (pathlen > 0 && fullPathList[k][pathlen - 1] != '/') {
                fullPathList[k][pathlen] = '/';
                lensofar++;
            }
            #endif
            memcpy(
                fullPathList[k] + lensofar,
                list[k], (strlen(list[k]) + 1) * sizeof(*list[k])
            );
            k++;
        }
        fullPathList[entriesSoFar] = NULL;
        k = 0;
        while (k < entriesSoFar && list[k]) {  // free orig data
            free(list[k]);
            k++;
        }
        free(list);
        // Return the full path arrays:
        *contents = fullPathList;
        *error = FSERR_SUCCESS;
    }
    return 1;
}

S3DEXP int spew3d_fs_ListFolder(
        const char *path, char ***contents,
        int returnFullPath, int *error
        ) {
    return spew3d_fs_ListFolderEx(
        path, contents, returnFullPath, 1, error
    );
}

S3DEXP char *spew3d_fs_GetCurrentDirectory() {
    #if defined(_WIN32) || defined(_WIN64)
    DWORD size = GetCurrentDirectory(0, NULL);
    char *s = malloc(size + 1);
    if (!s)
        return NULL;
    if (GetCurrentDirectory(size, s) != 0) {
        s[size - 1] = '\0';
        return s;
    }
    free(s);
    return NULL;
    #else
    int allocsize = 32;
    while (1) {
        allocsize *= 2;
        char *s = malloc(allocsize);
        if (!s)
            return NULL;
        char *result = getcwd(s, allocsize - 1);
        if (result == NULL) {
            free(s);
            if (errno == ERANGE) {
                continue;
            }
            return NULL;
        }
        s[allocsize - 1] = '\0';
        return s;
    }
    #endif
}

S3DEXP char *spew3d_fs_Join(
        const char *path1, const char *path2_orig
        ) {
    // Quick result paths:
    if (path2_orig && (
            strcmp(path2_orig, ".") == 0 ||
            strcmp(path2_orig, "") == 0
            ))
        path2_orig = NULL;
    if (!path1 && path2_orig)
        return strdup(path2_orig);
    if (path1 && !path2_orig)
        return strdup(path1);
    if (!path1 && !path2_orig)
        return NULL;

    // Clean up path2 for merging:
    char *path2 = strdup(path2_orig);
    if (!path2)
        return NULL;
    while (strlen(path2) >= 2 && path2[0] == '.' &&
            (path2[1] == '/'
            #if defined(_WIN32) || defined(_WIN64)
            || path2[1] == '\\'
            #endif
            )) {
        memmove(path2, path2 + 2, strlen(path2) + 1 - 2);
        if (strcmp(path2, "") == 0 || strcmp(path2, ".") == 0) {
            free(path2);
            return strdup(path1);
        }
    }

    // Do actual merging:
    char *presult = malloc(strlen(path1) + 1 + strlen(path2) + 1);
    if (!presult) {
        free(path2);
        return NULL;
    }
    memcpy(presult, path1, strlen(path1) + 1);
    presult[strlen(path1)] = '\0';
    if (strlen(path1) > 0) {
        presult[strlen(path1) + 1] = '\0';
        #if defined(_WIN32) || defined(_WIN64)
        if (path1[strlen(path1) - 1] != '\\' &&
                path1[strlen(path1) - 1] != '/' &&
                (strlen(path2) == 0 || path2[0] != '\\' ||
                 path2[0] != '/'))
            presult[strlen(path1)] = '\\';
        #else
        if ((path1[strlen(path1) - 1] != '/') &&
                (strlen(path2) == 0 || path2[0] != '/'))
            presult[strlen(path1)] = '/';
        #endif
        memcpy(presult + strlen(presult), path2, strlen(path2) + 1);
    } else {
        #if defined(_WIN32) || defined(_WIN64)
        if (strlen(path2) == 0 ||
                path2[0] == '/' ||
                path2[0] == '\\')
            memcpy(presult + strlen(presult),
                   path2 + 1, strlen(path2));
        else
            memcpy(presult + strlen(presult), path2,
                   strlen(path2) + 1);
        #else
        if (strlen(path2) == 0 ||
                path2[0] == '/')
            memcpy(presult + strlen(presult),
                   path2 + 1, strlen(path2));
        else
            memcpy(presult + strlen(presult), path2,
                   strlen(path2) + 1);
        #endif
    }
    free(path2);
    return presult;
}

S3DEXP int spew3d_fs_IsAbsolutePath(const char *path) {
    if (strlen(path) > 0 && path[0] == '.')
        return 0;
    #if (!defined(_WIN32) && !defined(_WIN64))
    if (strlen(path) > 0 && path[0] == '/')
        return 1;
    #endif
    #if defined(_WIN32) || defined(_WIN64)
    if (strlen(path) > 2 && (
            path[1] == ':' || path[1] == '\\'))
        return 1;
    #endif
    return 0;
}

S3DEXP char *spew3d_fs_Basename(const char *path) {
    int i = 0;
    while ((int)strlen(path) > i &&
            path[strlen(path) - i - 1] != '/'
            #if defined(_WIN32) || defined(_WIN64)
            &&
            path[strlen(path) - i - 1] != '\\'
            #endif
            )
        i++;
    char *result = malloc(i + 1);
    if (!result)
        return result;
    memcpy(result, path + strlen(path) - i, i + 1);
    return result;
}

S3DEXP char *spew3d_fs_ParentdirOfItem(const char *path) {
    if (!path)
        return NULL;
    char *p = strdup(path);
    if (!p)
        return NULL;

    // Strip trailing duplicate separators if any:
    #if defined(_WIN32) || defined(_WIN64)
    while (strlen(p) > 1 && (
            p[strlen(p) - 1] == '/' ||
            p[strlen(p) - 1] == '\\') &&
            (p[strlen(p) - 2] == '/' ||
            p[strlen(p) - 2] == '\\'))
        p[strlen(p) - 1] = '\0';
    #else
    while (strlen(p) > 1 && (
            p[strlen(p) - 1] == '/') &&
            (p[strlen(p) - 2] == '/'))
        p[strlen(p) - 1] = '\0';
    #endif

    // If this is already shortened to absolute path root, abort:
    #if defined(_WIN32) || defined(_WIN64)
    if (strlen(path) >= 2 && strlen(path) <= 3 &&
            path[1] == ':' && (strlen(path) == 2 ||
            path[2] == '/' || path[2] == '\\') &&
            ((path[0] >= 'a' && path[0] <= 'z') ||
              (path[0] >= 'A' && path[0] <= 'Z')))
        return p;
    #else
    if (strlen(path) == 1 && path[0] == '/')
        return p;
    #endif

    // Strip trailing ALL trailing separators, then go back to previous:
    #if defined(_WIN32) || defined(_WIN64)
    while (strlen(p) > 0 && (
            p[strlen(p) - 1] == '/' ||
            p[strlen(p) - 1] == '\\'))
        p[strlen(p) - 1] = '\0';
    while (strlen(p) > 0 &&
            p[strlen(p) - 1] != '/' &&
            p[strlen(p) - 1] != '\\')
        p[strlen(p) - 1] = '\0';
    #else
    while (strlen(p) > 0 &&
            p[strlen(p) - 1] == '/')
        p[strlen(p) - 1] = '\0';
    while (strlen(p) > 0 &&
            p[strlen(p) - 1] != '/')
        p[strlen(p) - 1] = '\0';
    #endif
    return p;
}

S3DEXP char *spew3d_fs_GetOwnExecutablePath() {
    #if defined(_WIN32) || defined(_WIN64)
    wchar_t fp[MAX_PATH * 2 + 1];
    GetModuleFileNameW(NULL, fp, MAX_PATH * 2 + 1);
    fp[MAX_PATH] = '\0';
    char *result = AS_U8_FROM_U16(fp);
    return result;
    #else
    int alloc = 16;
    char *fpath = malloc(alloc);
    if (!fpath)
        return NULL;
    while (1) {
        fpath[0] = '\0';
        #if defined(APPLE) || defined(__APPLE__)
        int i = alloc;
        if (_NSGetExecutablePath(fpath, &i) != 0) {
            free(fpath);
            fpath = malloc(i + 1);
            if (_NSGetExecutablePath(fpath, &i) != 0) {
                free(fpath);
                return NULL;
            }
            return fpath;
        }
        #else
        int written = readlink("/proc/self/exe", fpath, alloc);
        if (written >= alloc) {
            alloc *= 2;
            free(fpath);
            fpath = malloc(alloc);
            if (!fpath)
                return NULL;
            continue;
        } else if (written <= 0) {
            free(fpath);
            return NULL;
        }
        fpath[written] = '\0';
        return fpath;
        #endif
    }
    #endif
}

S3DEXP char *spew3d_fs_ToAbsolutePath(const char *path) {
    if (spew3d_fs_IsAbsolutePath(path))
        return strdup(path);
    char *cwd = spew3d_fs_GetCurrentDirectory();
    if (!cwd)
        return NULL;
    char *result = spew3d_fs_Join(cwd, path);
    free(cwd);
    return result;
}

S3DEXP int spew3d_fs_LaunchExecutable(
        const char *path, int argcount, const char **_orig_argv
        ) {
    int argc = 0;
    char **argv = malloc(sizeof(*argv) * 2);
    if (argv) {
        argv[0] = strdup(path);
        argv[1] = NULL;
        if (argv[0]) {
            argc++;
        } else {
            free(argv);
            argv = NULL;
        }
    }
    int i = 0;
    while (i < argcount) {
        char *val = strdup(_orig_argv[i]);
        if (!val)
            goto dumpargv;
        char **newargv = malloc(sizeof(*argv) * (argc + 2));
        int k = 0;
        if (newargv) {
            k = 0;
            while (k < argc) {
                newargv[k] = argv[k];
                k++;
            }
            free(argv);
            argv = newargv;
            argv[argc] = val;
            argv[argc + 1] = NULL;
            argc++;
        } else {
            dumpargv: ;
            free(val);
            k = 0;
            while (k < argc) {
                free(argv[k]);
                k++;
            }
            free(argv);
            argv = NULL;
            return 0;
        }
        i++;
    }
    if (!argv)
        return 0;
    int success = 1;
    #if defined(_WIN32) || defined(_WIN64)
    char *cmd = strdup("");
    if (!cmd) {
        success = 0;
        goto ending;
    }
    int j = 0;
    while (j < argc) {
        char *newcmd = realloc(
            cmd, strlen(cmd) + 3 + strlen(argv[j]) * 2 + 1
        );
        if (!newcmd) {
            free(cmd);
            success = 0;
            goto ending;
        }
        cmd = newcmd;
        if (j > 0) {
            cmd[strlen(cmd) + 1] = '\0';
            cmd[strlen(cmd)] = ' ';
        }
        cmd[strlen(cmd) + 1] = '\0';
        cmd[strlen(cmd)] = '\"';
        int k = 0;
        while (k < (int)strlen(argv[j])) {
            char c = argv[j][k];
            if (c == '"' || c == '\\') {
                cmd[strlen(cmd) + 1] = '\0';
                cmd[strlen(cmd)] = '\\';
            }
            cmd[strlen(cmd) + 1] = '\0';
            cmd[strlen(cmd)] = c;
            k++;
        }
        cmd[strlen(cmd) + 1] = '\0';
        cmd[strlen(cmd)] = '\"';
        j++;
    }
    STARTUPINFOW info;
    memset(&info, 0, sizeof(info));
    PROCESS_INFORMATION pinfo;
    memset(&pinfo, 0, sizeof(pinfo));
    success = 0;
    wchar_t *pathw = AS_U16(path);
    if (!pathw) {
        free(cmd);
        success = 0;
        goto ending;
    }
    wchar_t *cmdw = AS_U16(cmd);
    if (!cmdw) {
        free(cmd);
        free(pathw);
        success = 0;
        goto ending;
    }
    if (CreateProcessW(pathw, cmdw, NULL, NULL,
            FALSE, CREATE_DEFAULT_ERROR_MODE | DETACHED_PROCESS |
            CREATE_NEW_PROCESS_GROUP, NULL,
            NULL, &info, &pinfo)) {
        success = 1;
        CloseHandle(info.hStdInput);
        CloseHandle(info.hStdOutput);
        CloseHandle(info.hStdError);
        CloseHandle(pinfo.hProcess);
        CloseHandle(pinfo.hThread);
    }
    free(pathw);
    free(cmdw);
    free(cmd);
    #else
    if (fork() == 0) {
        execvp(path, argv);
        // This should never be reached:
        exit(1);
        return 0;
    }
    #endif
    #if defined(_WIN32) || defined(_WIN64)
    ending: ;
    #endif
    int z = 0;
    while (z < argc) {
        free(argv[z]);
        z++;
    }
    free(argv);
    argv = NULL;
    return success;
}

S3DEXP char *spew3d_fs_GetSysTempdir() {
    #if defined(_WIN32) || defined(_WIN64)
    int tempbufwsize = 512;
    wchar_t *tempbufw = malloc(tempbufwsize * sizeof(wchar_t));
    assert(
        sizeof(wchar_t) == sizeof(uint16_t)
        // should be true for windows
    );
    if (!tempbufw)
        return NULL;
    unsigned int rval = 0;
    while (1) {
        rval = GetTempPathW(
            tempbufwsize - 1, tempbufw
        );
        if (rval >= (unsigned int)tempbufwsize - 2) {
            tempbufwsize *= 2;
            free(tempbufw);
            tempbufw = malloc(tempbufwsize * sizeof(wchar_t));
            if (!tempbufw)
                return NULL;
            continue;
        }
        if (rval == 0)
            return NULL;
        tempbufw[rval] = '\0';
        break;
    }
    assert(wcslen(tempbufw) < (uint64_t)tempbufwsize - 2);
    if (tempbufw[wcslen(tempbufw) - 1] != '\\') {
        tempbufw[wcslen(tempbufw) + 1] = '\0';
        tempbufw[wcslen(tempbufw)] = '\\';
    }
    int _wasoom = 0;
    int64_t tempbuffill = 0;
    char *tempbuf = AS_U8_FROM_U16(tempbufw);
    free(tempbufw);
    if (!tempbuf)
        return NULL;
    return tempbuf;
    #else
    char *tempbufu8 = NULL;
    tempbufu8 = strdup("/tmp/");
    if (!tempbufu8) {
        return NULL;
    }
    return tempbufu8;
    #endif
}

S3DEXP int spew3d_fs_RemoveFolderRecursively(
        const char *path, int *error
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        *error = FSERR_TARGETNOTADIRECTORY;
        return 0;
    }

    int final_error = FSERR_SUCCESS;
    const char *scan_next = path;
    int operror = 0;
    int64_t queue_scan_index = 0;
    char **removal_queue = NULL;
    int64_t queue_len = 0;

    char **contents = NULL;
    int firstitem = 1;
    while (1) {
        if (!scan_next) {
            firstitem = 0;
            if (queue_scan_index < queue_len) {
                scan_next = removal_queue[queue_scan_index];
                queue_scan_index++;
                continue;
            } else {
                break;
            }
        }
        assert(scan_next != NULL);
        int listingworked = spew3d_fs_ListFolderEx(
            scan_next, &contents, 1,
            0,  // fail on symlinks!!! (no delete-descend INTO those!!)
            &operror
        );
        if (!listingworked) {
            if (operror == FSERR_TARGETNOTADIRECTORY ||
                    operror == FSERR_SYMLINKSWEREEXCLUDED ||
                    (firstitem && operror ==
                        FSERR_NOSUCHTARGET)) {
                // It's a file or symlink.
                // If it's a file, and this is our first item, error:
                if (firstitem && operror ==
                        FSERR_TARGETNOTADIRECTORY) {
                    // We're advertising recursive DIRECTORY deletion,
                    // so resuming here would be unexpected.
                    *error = FSERR_TARGETNOTADIRECTORY;
                    assert(!contents && queue_len == 0);
                    return 0;
                } else if (firstitem &&
                        operror == FSERR_NOSUCHTARGET) {
                    *error = FSERR_NOSUCHTARGET;
                    assert(!contents && queue_len == 0);
                    return 0;
                }
                // Instantly remove it instead:
                if (operror != FSERR_NOSUCHTARGET &&
                        !_spew3d_fs_RemoveFileEx(
                        scan_next, &operror, 1
                        )) {
                    if (operror == FSERR_NOSUCHTARGET) {
                        // Maybe it was removed in parallel?
                        // ignore this.
                    } else if (final_error == FSERR_SUCCESS) {
                        // No error yet, so take this one.
                        final_error = operror;
                    }
                }
                if (queue_scan_index > 0 &&
                        queue_scan_index < queue_len &&
                        memcmp(
                            removal_queue[queue_scan_index - 1],
                            scan_next,
                            sizeof(*scan_next) * (strlen(scan_next) + 1)
                        ) == 0) {
                    free(removal_queue[queue_scan_index - 1]);
                    memmove(
                        &removal_queue[queue_scan_index - 1],
                        &removal_queue[queue_scan_index],
                        sizeof(*removal_queue) * (
                            queue_len - queue_scan_index
                        )
                    );
                    queue_len--;
                    queue_scan_index--;
                }
                scan_next = NULL;
                firstitem = 0;
                continue;
            }
            // Another error. Consider it for returning at the end:
            if (final_error == FSERR_SUCCESS) {
                // No error yet, so take this one.
                final_error = operror;
            }
        } else if (contents[0]) {  // one new item or more
            int64_t addc = 0;
            while (contents[addc])
                addc++;
            char **new_removal_queue = realloc(
                removal_queue, sizeof(*removal_queue) * (
                    queue_len + addc
                )
            );
            if (new_removal_queue)
                removal_queue = new_removal_queue;
            if (!new_removal_queue) {
                *error = FSERR_OUTOFMEMORY;
                int64_t k = 0;
                while (k < queue_len) {
                    free(removal_queue[k]);
                    k++;
                }
                free(removal_queue);
                spew3d_fs_FreeFolderList(contents);
                contents = NULL;
                return 0;
            }
            memcpy(
                &removal_queue[queue_len],
                contents,
                sizeof(*contents) * addc
            );
            queue_len += addc;
            free(contents);  // we copied contents, so only free outer
        } else {
            spew3d_fs_FreeFolderList(contents);
        }
        contents = NULL;
        scan_next = NULL;
        firstitem = 0;
    }
    // Now remove everything left in the queue, since we should have
    // gotten rid of all the files. However, there might still be nested
    // directories, so let's go through the queue BACKWARDS (from inner
    // to outer):
    int64_t k = queue_len - 1;
    while (k >= 0) {
        if (!_spew3d_fs_RemoveFileEx(
                removal_queue[k],
                &operror, 1
                )) {
            if (operror == FSERR_NOSUCHTARGET) {
                // Maybe it was removed in parallel?
                // ignore this.
            } else if (final_error == FSERR_SUCCESS) {
                // No error yet, so take this one.
                final_error = operror;
            }
        }
        free(removal_queue[k]);
        k--;
    }
    free(removal_queue);
    if (!_spew3d_fs_RemoveFileEx(
            path, &operror, 1
            )) {
        if (operror == FSERR_NOSUCHTARGET) {
            // Maybe it was removed in parallel?
            // ignore this.
        } else if (final_error == FSERR_SUCCESS) {
            // No error yet, so take this one.
            final_error = operror;
        }
    }
    if (final_error != FSERR_SUCCESS) {
        *error = final_error;
        return 0;
    }
    return 1;
}

S3DHID int _spew3d_fs_RemoveFileEx(
        const char *path, int *error,
        int allowdirs
        ) {
    #if defined(_WIN32) || defined(_WIN64)
    wchar_t *path16 = (wchar_t*)AS_U16(path);
    if (!path16) {
        *error = FSERR_OUTOFMEMORY;
        return 0;
    }
    if (DeleteFileW(path16) != TRUE) {
        uint32_t werror = GetLastError();
        *error = FSERR_OTHERERROR;
        if (werror == ERROR_DIRECTORY_NOT_SUPPORTED ||
                werror == ERROR_DIRECTORY) {
            if (!allowdirs) {
                *error = FSERR_TARGETNOTAFILE;
                return 0;
            }
            if (RemoveDirectoryW(path16) != TRUE) {
                free(path16);
                werror = GetLastError();
                *error = FSERR_OTHERERROR;
                if (werror == ERROR_PATH_NOT_FOUND ||
                        werror == ERROR_FILE_NOT_FOUND ||
                        werror == ERROR_INVALID_PARAMETER ||
                        werror == ERROR_INVALID_NAME ||
                        werror == ERROR_INVALID_DRIVE)
                    *error = FSERR_NOSUCHTARGET;
                else if (werror == ERROR_ACCESS_DENIED ||
                        werror == ERROR_WRITE_PROTECT ||
                        werror == ERROR_SHARING_VIOLATION)
                    *error = FSERR_NOPERMISSION;
                else if (werror == ERROR_NOT_ENOUGH_MEMORY)
                    *error = FSERR_OUTOFMEMORY;
                else if (werror == ERROR_TOO_MANY_OPEN_FILES)
                    *error = FSERR_OUTOFFDS;
                else if (werror == ERROR_PATH_BUSY ||
                        werror == ERROR_BUSY ||
                        werror == ERROR_CURRENT_DIRECTORY)
                    *error = FSERR_DIRISBUSY;
                else if (werror == ERROR_DIR_NOT_EMPTY)
                    *error = FSERR_NONEMPTYDIRECTORY;
                return 0;
            }
            free(path16);
            *error = FSERR_SUCCESS;
            return 1;
        }
        free(path16);
        if (werror == ERROR_PATH_NOT_FOUND ||
                werror == ERROR_FILE_NOT_FOUND ||
                werror == ERROR_INVALID_PARAMETER ||
                werror == ERROR_INVALID_NAME ||
                werror == ERROR_INVALID_DRIVE)
            *error = FSERR_NOSUCHTARGET;
        else if (werror == ERROR_ACCESS_DENIED ||
                werror == ERROR_WRITE_PROTECT ||
                werror == ERROR_SHARING_VIOLATION)
            *error = FSERR_NOPERMISSION;
        else if (werror == ERROR_NOT_ENOUGH_MEMORY)
            *error = FSERR_OUTOFMEMORY;
        else if (werror == ERROR_TOO_MANY_OPEN_FILES)
            *error = FSERR_OUTOFFDS;
        else if (werror == ERROR_PATH_BUSY ||
                werror == ERROR_BUSY)
            *error = FSERR_DIRISBUSY;
        return 0;
    }
    free(path16);
    *error = FSERR_SUCCESS;
    return 1;
    #else
    #if defined(_WIN32) || defined(_WIN64)
    int result;
    if (allowdirs) {
        result = remove(path);
    } else { 
        result = unlink(path);
    }
    if (result < 0) {
        *error = FSERR_OTHERERROR;
        if (errno == EACCES || errno == EPERM ||
                errno == EROFS) {
            *error = FSERR_NOPERMISSION;
        } else if (errno == EISDIR) {
            *error = FSERR_TARGETNOTAFILE;
        } else if (errno == ENOTEMPTY) {
            *error = FSERR_NONEMPTYDIRECTORY;
        } else if (errno == ENOENT || errno == ENAMETOOLONG ||
                errno == ENOTDIR) {
            *error = FSERR_NOSUCHTARGET;
        } else if (errno == EBUSY) {
            *error = FSERR_DIRISBUSY;
        }
        return 0;
    }
    #endif
    *error = FSERR_SUCCESS;
    return 1;
    #endif
}

S3DEXP int spew3d_fs_RemoveFile(const char *target, int *err) {
    return _spew3d_fs_RemoveFileEx(target, err, 0);
}

S3DEXP FILE *_spew3d_fs_TempFile_SingleTry(
        int subfolder, int folderonly,
        const char *prefix, const char *suffix,
        char **folder_path, char **path,
        int *do_retry
        ) {
    assert(!folderonly || subfolder);
    assert(folderonly || path);
    *do_retry = 0;
    if (path)  // may be NULL for folderonly=1
        *path = NULL;
    *folder_path = NULL;

    // Get the folder path for system temp location:
    char *tempbuf = spew3d_fs_GetSysTempdir();
    if (!tempbuf)
        return NULL;

    // Random bytes we use in the name:
    uint64_t v[4];
    if (!spew3d_secrandom_GetBytes(
            (char*)&v, sizeof(v)
            )) {
        free(tempbuf);
        return NULL;
    }

    // The secure random part to be inserted as a string:
    char randomu8[512];
    snprintf(
        randomu8, sizeof(randomu8) - 1,
        "%" PRIu64 "%" PRIu64 "%" PRIu64 "%" PRIu64,
        v[0], v[1], v[2], v[3]
    );
    randomu8[
        sizeof(randomu8) - 1
    ] = '\0';
    char *combined_path = NULL;
    if (subfolder) {  // Create the subfolder:
        combined_path = malloc(
            sizeof(*combined_path) * (strlen(tempbuf) +
            strlen(prefix) + strlen(randomu8) + 2)
        );
        if (!combined_path) {
            free(tempbuf);
            return NULL;
        }
        memcpy(combined_path, tempbuf,
               sizeof(*combined_path) * strlen(tempbuf));
        if (prefix && strlen(prefix) > 0)
            memcpy(combined_path + strlen(tempbuf), prefix,
               sizeof(*prefix) * strlen(prefix));
        memcpy(combined_path + strlen(tempbuf) + strlen(prefix), randomu8,
               sizeof(*randomu8) * strlen(randomu8));
        #if defined(_WIN32) || defined(_WIN64)
        combined_path[
            strlen(tempbuf) + strlen(prefix) + strlen(randomu8)
        ] = '\\';
        #else
        combined_path[
            strlen(tempbuf) + strlen(prefix) + strlen(randomu8)
        ] = '/';
        #endif
        combined_path[
            strlen(tempbuf) + strlen(prefix) + strlen(randomu8)
        ] = '\0';
        free(tempbuf);
        tempbuf = NULL;

        int _mkdirerror = 0;
        if ((_mkdirerror = spew3d_fs_CreateDirectoryEx(
                combined_path, 1
                )) < 0) {
            if (_mkdirerror == FSERR_TARGETALREADYEXISTS) {
                // Oops, somebody was faster/we hit an existing
                // folder out of pure luck. Retry.
                *do_retry = 1;
            }
            free(combined_path);
            return NULL;
        }
        *folder_path = combined_path;
        if (folderonly) {
            if (path)  // may be NULL for folderonly=1
                *path = NULL;
            return NULL;
        }
    } else {
        combined_path = tempbuf;
        tempbuf = NULL;
    }

    // Compose the path to the file to create:
    char *file_path = malloc(sizeof(*file_path) * (
        strlen(combined_path) + (prefix ? strlen(prefix) : 0) +
        strlen(randomu8) + (suffix ? strlen(suffix) : 0) + 1
    ));
    if (!file_path) {
        int error = 0;
        if (subfolder) {
            _spew3d_fs_RemoveFileEx(
                *folder_path, &error, 1
            );
        }
        free(combined_path);
        *folder_path = NULL;  // already free'd through combined_path
        return NULL;
    }
    memcpy(
        file_path, combined_path,
        sizeof(*file_path) * strlen(combined_path)
    );
    if (prefix && strlen(prefix) > 0)
        memcpy(
            file_path + strlen(combined_path), prefix,
            sizeof(*prefix) * strlen(prefix)
        );
    memcpy(
        file_path + strlen(combined_path) +
        (prefix ? strlen(prefix) : 0),
        randomu8, sizeof(*randomu8) * strlen(randomu8)
    );
    if (suffix && strlen(suffix) > 0)
        memcpy(
            file_path + strlen(combined_path) +
            (prefix ? strlen(prefix) : 0) + strlen(randomu8),
            suffix, sizeof(*suffix) * strlen(suffix)
        );
    file_path[(
        strlen(combined_path) +
        (prefix ? strlen(prefix) : 0) + strlen(randomu8) +
        (suffix ? strlen(suffix) : 0)
    )] = '\0';
    if (!subfolder) {
        // (this was otherwise set as *folder_path)
        free(combined_path);
    }
    combined_path = NULL;

    // Create file and return result:
    int _innererr = 0;
    FILE *f = spew3d_fs_OpenFromPath(
        file_path, "wb", &_innererr
    );
    if (!f) {
        if (subfolder) {
            int error = 0;
            spew3d_fs_RemoveFolderRecursively(
                *folder_path, &error
            );
            free(*folder_path);
            *folder_path = NULL;
        }
        free(file_path);
        return NULL;
    }
    // Note: folderonly=0 is guaranteed here.
    *path = file_path;
    return f;
}

S3DEXP FILE *spew3d_fs_TempFile(
        int subfolder, int folderonly,
        const char *prefix, const char *suffix,
        char **folder_path, char **path
        ) {
    assert(folderonly || path);
    while (1) {
        int retry = 0;
        *folder_path = NULL;
        FILE *f = _spew3d_fs_TempFile_SingleTry(
            subfolder, folderonly, prefix,
            suffix, folder_path,
            path, &retry
        );
        if (f)
            return f;
        if (subfolder && folderonly && *folder_path != NULL) {
            if (path)  // may be NULL if folderonly=1
                *path = NULL;
            return NULL;
        }
        if (!f && !retry) {
            *folder_path = NULL;
            if (path)  // may be NULL if folderonly=1
                *path = NULL;
            return NULL;
        }
    }
}

#endif  // SPEW3D_IMPLEMENTATION
