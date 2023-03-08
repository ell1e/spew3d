/* Copyright (c) 2020-2022, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#include <assert.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>


typedef struct spew3darchive spew3darchive;
typedef struct spew3d_vfs_mount spew3d_vfs_mount;

typedef struct spew3d_vfs_mount {
    int64_t mountid;
    char *archivediskpath;
    spew3darchive *archive;
    spew3d_vfs_mount *next;
} spew3d_vfs_mount;

int64_t _spew3d_lastusedmountid = 0;
spew3d_vfs_mount *_spew3d_global_mount_list = NULL;

typedef struct SPEW3DVFS_FILE {
    uint8_t via_mount, is_limited;
    union {
        struct {
            spew3d_vfs_mount *src_mount;
            uint64_t src_entry;
        };
        FILE *diskhandle;
    };
    int64_t size;
    uint64_t offset;
    uint64_t limit_start, limit_len;
    char *mode, *path;
} SPEW3DVFS_FILE;

s3d_mutex *spew3d_vfs_mutex = NULL;

__attribute__((constructor)) static void _createVFSMutex() {
    spew3d_vfs_mutex = mutex_Create();
    if (!spew3d_vfs_mutex) {
        fprintf(stderr, "spew3d_vfs.c: error: FATAL, "
            "failed to create spew3d_vfs_mutex\n");
        _exit(1);
    }
}

int64_t spew3d_vfs_MountArchiveFromDisk(const char *path) {
    mutex_Lock(spew3d_vfs_mutex);
    char *pathcleaned = spew3d_vfs_NormalizePath(path);
    if (!pathcleaned) {
        mutex_Release(spew3d_vfs_mutex);
        return -1;
    }
    spew3darchive *archive = spew3d_archive_FromFilePath(
        path, 0,
        VFSFLAG_NO_VIRTUALPAK_ACCESS,
        SPEW3DARCHIVE_TYPE_AUTODETECT
    );
    if (!archive) {
        free(pathcleaned);
        mutex_Release(spew3d_vfs_mutex);
        return -1;
    }
    spew3d_vfs_mount *newmount = malloc(sizeof(*newmount));
    if (!newmount) {
        free(pathcleaned);
        spew3d_archive_Close(archive);
        mutex_Release(spew3d_vfs_mutex);
        return -1;
    }
    memset(newmount, 0, sizeof(*newmount));
    _spew3d_lastusedmountid++;
    newmount->mountid = _spew3d_lastusedmountid;
    newmount->archivediskpath = pathcleaned;
    newmount->archive = archive;
    newmount->next = _spew3d_global_mount_list;
    _spew3d_global_mount_list = newmount;
    mutex_Release(spew3d_vfs_mutex);
    return newmount->mountid;
}

void spew3d_vfs_fclose(SPEW3DVFS_FILE *f) {
    if (!f)
        return;
    if (!f->via_mount) {
        if (f->diskhandle)
            fclose(f->diskhandle);
    }
    free(f->mode);
    free(f->path);
    free(f);
}

int spew3d_vfs_fgetc(SPEW3DVFS_FILE *f) {
    unsigned char buf[1];
    size_t result = spew3d_vfs_fread((char *)buf, 1, 1, f);
    if (result == 1)
        return (int)buf[0];
    return -1;
}

int64_t spew3d_vfs_ftell(SPEW3DVFS_FILE *f) {
    int64_t result = -1;
    if (!f->via_mount) {
        result = ftell64(f->diskhandle);
        if (result >= 0)
            f->offset = result;
    } else {
        result = f->offset;
    }
    if (result >= 0 && f->is_limited) {
        result -= f->limit_start;
        if (result < 0 || result > (int64_t)f->limit_len)
            result = -1;
    }
    return result;
}

int spew3d_vfs_peakc(SPEW3DVFS_FILE *f) {
    int64_t byteoffset = 0;
    if (!f->via_mount)
        byteoffset = ftell(f->diskhandle);
    else
        byteoffset = f->offset;
    if (byteoffset < 0 || spew3d_vfs_feof(f) ||
            (f->via_mount && byteoffset >= f->size))
        return -1;
    unsigned char buf[1];
    size_t result = spew3d_vfs_fread((char *)buf, 1, 1, f);
    if (!f->via_mount)
        fseek(f->diskhandle, byteoffset, SEEK_SET);
    else
        f->offset = byteoffset;
    if (result == 1)
        return (int)buf[0];
    return -1;
}

size_t spew3d_vfs_fwrite(
        const char *buffer, int bytes,
        int numn, SPEW3DVFS_FILE *f
        ) {
    if (f->via_mount || (strstr(f->mode, "a") == NULL &&
            strstr(f->mode, "w") == NULL &&
            strstr(f->mode, "+") == NULL))
        return 0;
    return fwrite(buffer, bytes, numn, f->diskhandle);
}


int spew3d_vfs_feof(SPEW3DVFS_FILE *f) {
    if (f->is_limited &&
            (uint64_t)f->offset - f->limit_start >=
            (uint64_t)f->limit_len)
        return 1;
    if (!f->via_mount)
        return feof(f->diskhandle);
    if (f->size < 0)
        return 0;
    return ((uint64_t)f->offset >= (uint64_t)f->size);
}

int spew3d_vfs_fseek(SPEW3DVFS_FILE *f, uint64_t offset) {
    uint64_t startoffset = 0;
    if (f->is_limited) {
        startoffset = f->limit_start;
        if (offset > f->limit_len)
            return -1;
    }
    if (!f->via_mount) {
        if (fseek64(f->diskhandle,
                offset + startoffset, SEEK_SET) == 0) {
            f->offset = offset + startoffset;
            return 0;
        }
        return -1;
    }
    f->offset = offset + startoffset;
    return -1;
}

SPEW3DVFS_FILE *spew3d_vfs_fdup(SPEW3DVFS_FILE *f) {
    mutex_Lock(spew3d_vfs_mutex);
    SPEW3DVFS_FILE *fnew = malloc(sizeof(*fnew));
    if (!fnew) {
        mutex_Release(spew3d_vfs_mutex);
        return NULL;
    }
    memcpy(fnew, f, sizeof(*f));
    fnew->mode = strdup(f->mode);
    if (!fnew->mode) {
        free(fnew);
        mutex_Release(spew3d_vfs_mutex);
        return NULL;
    }
    fnew->path = NULL;
    if (!fnew->via_mount) {
        fnew->diskhandle = _dupfhandle(fnew->diskhandle, f->mode);
        if (!fnew->diskhandle) {
            free(fnew->mode);
            free(fnew);
            mutex_Release(spew3d_vfs_mutex);
            return NULL;
        }
    } else {
        assert(fnew->src_mount != NULL);
        fnew->path = strdup(f->path);
        if (!fnew->path) {
            free(fnew->mode);
            free(fnew);
            mutex_Release(spew3d_vfs_mutex);
            return NULL;
        }
    }
    mutex_Release(spew3d_vfs_mutex);
    return fnew;
}

int spew3d_vfs_fseektoend(SPEW3DVFS_FILE *f) {
    if (f->is_limited)
        return (spew3d_vfs_fseek(f, f->limit_len) == 0);
    if (!f->via_mount) {
        if (fseek64(f->diskhandle, 0, SEEK_END) == 0) {
            int64_t tellpos = ftell64(f->diskhandle);
            if (tellpos >= 0) {
                f->offset = tellpos;
                return 1;
            }
            // Otherwise, try to revert:
            fseek64(f->diskhandle, f->offset, SEEK_SET);
            return 0;
        }
        return 0;
    }
    if (f->size < 0)
        return 0;
    f->offset = f->size;
    return 1;
}

size_t spew3d_vfs_fread(
        char *buffer, int bytes, int numn,
        SPEW3DVFS_FILE *f
        ) {
    if (bytes <= 0 || numn <= 0)
        return 0;

    if (f->is_limited) {
        if (bytes > 1) {
            // Arbitrary chunks regular path:
            while (numn > 0 &&
                    f->offset + (int64_t)(bytes * numn) >
                    f->limit_start + f->limit_len) {
                if (bytes == 1) {
                    numn = ((f->limit_len + f->limit_start) -
                        f->offset);
                    break;
                }
                numn--;
            }
        } else {
            // Fast-path for bytes=1 numn=X:
            if ((f->offset - f->limit_start) +
                    (int64_t)numn > f->limit_len) {
                numn = (int64_t)(f->limit_len -
                    (f->offset - f->limit_start));
            }
        }
        if (numn <= 0)
            return 0;
    }

    if (!f->via_mount) {
        errno = 0;
        size_t result = fread(buffer, bytes, numn, f->diskhandle);
        if (result > 0) {
            f->offset += result;
        } else {
            if (!feof(f->diskhandle)) {
                errno = EIO;
            }
        }
        return result;
    }

    mutex_Lock(spew3d_vfs_mutex);
    if (f->offset >= f->size || f->size < 0) {
        mutex_Release(spew3d_vfs_mutex);
        return 0;
    }
    if (bytes > 1) {
        // Arbitrary chunks regular path:
        while (numn > 0 && f->size >= 0 &&
                f->offset + (int64_t)(bytes * numn) > f->size)
            numn--;
        if (numn <= 0) {
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }

        int numn_read = 0;
        while (numn > 0) {
            assert(f->offset + (int64_t)bytes <= f->size);
            int result = spew3d_archive_ReadFileByteSlice(
                f->src_mount->archive, f->src_entry,
                f->offset, buffer, bytes);
            if (result != bytes) {
                errno = EIO;
                mutex_Release(spew3d_vfs_mutex);
                return 0;
            }
            f->offset += bytes;
            buffer += bytes;
            numn_read++;
        }
        mutex_Release(spew3d_vfs_mutex);
        return numn_read;
    } else {
        // Fast-path for bytes=1 numn=X:
        assert(bytes == 1);
        if (f->offset + (int64_t)numn > f->size)
            numn = (int64_t)(f->size - f->offset);
        if (numn <= 0) {
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
        int result = spew3d_archive_ReadFileByteSlice(
            f->src_mount->archive, f->src_entry,
            f->offset, buffer, numn);
        if (result <= 0) {
            errno = EIO;
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
        f->offset += result;
        mutex_Release(spew3d_vfs_mutex);
        return result;
    }
}

int spew3d_vfs_flimitslice(
        SPEW3DVFS_FILE *f, uint64_t fileoffset,
        uint64_t maxlen
        ) {
    if (!f)
        return 0;

    // Get the old position to revert to if stuff goes wrong:
    int64_t pos = (
        (!f->via_mount) ? (int64_t)ftell64(f->diskhandle) :
        (int64_t)f->offset
    );
    if (pos < 0)
        return 0;

    // Get the file's current true size:
    int64_t size = -1;
    mutex_Lock(spew3d_vfs_mutex);
    if (!f->via_mount) {
        if (fseek64(f->diskhandle, 0, SEEK_END) != 0) {
            // at least TRY to seek back:
            fseek64(f->diskhandle, pos, SEEK_SET);
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
        size = ftell64(f->diskhandle);
        if (fseek64(f->diskhandle, pos, SEEK_SET) != 0) {  // revert back
            // ... nothing we can do?
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
    } else {
        size = spew3d_archive_GetEntrySize(
            f->src_mount->archive, f->src_entry);
    }
    if (size < 0) {
        mutex_Release(spew3d_vfs_mutex);
        return 0;
    }
    f->size = size;

    // Make sure the window applied is sane:
    if (fileoffset + maxlen > (uint64_t)size) {
        mutex_Release(spew3d_vfs_mutex);
        return 0;
    }
    int64_t newpos = pos;
    if ((uint64_t)newpos < fileoffset)
        newpos = fileoffset;
    if ((uint64_t)newpos > fileoffset + maxlen)
        newpos = fileoffset + maxlen;

    if (newpos != pos && !f->via_mount) {
        // Now, try to seek to the new position that
        // is now inside the window:
        if (fseek64(f->diskhandle, newpos, SEEK_SET) < 0) {
            // At least TRY to seek back
            fseek64(f->diskhandle, pos, SEEK_SET);
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
    }
    // Apply the new window:
    f->limit_start = fileoffset;
    f->limit_len = maxlen;
    f->offset = newpos;
    f->is_limited = 1;
    mutex_Release(spew3d_vfs_mutex);
    return 1;
}

int spew3d_vfs_Exists(
        const char *path, int vfsflags,
        int *result, int *fserr
        ) {
    if (spew3d_fs_IsObviouslyInvalidPath(path)) {
        *result = 0;
        if (fserr)
            *fserr = FSERR_NOSUCHTARGET;
        return 1;
    }
    if ((vfsflags & VFSFLAG_NO_VIRTUALPAK_ACCESS) == 0 &&
            !spew3d_fs_IsAbsolutePath(path)) {
        mutex_Lock(spew3d_vfs_mutex);
        char *pathfixed = spew3d_vfs_NormalizePath(path);
        if (!pathfixed) {
            if (fserr)
                *fserr = FSERR_OUTOFMEMORY;
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
        spew3d_vfs_mount *mount = _spew3d_global_mount_list;
        while (mount) {
            int foundasfolder = 0;
            int64_t foundidx = -1;
            if (spew3d_archive_GetEntryIndex(
                    mount->archive, pathfixed, &foundidx,
                    &foundasfolder
                    )) {
                if (fserr)
                    *fserr = FSERR_SUCCESS;
                free(pathfixed);
                *result = 1;
                mutex_Release(spew3d_vfs_mutex);
                return 1;
            }
            mount = mount->next;
        }
        free(pathfixed);
        mutex_Release(spew3d_vfs_mutex);
    }
    if ((vfsflags & VFSFLAG_NO_REALDISK_ACCESS) == 0) {
        int innerresult = 0;
        if (!spew3d_fs_TargetExists(path, &innerresult)) {
            if (fserr)
                *fserr = FSERR_IOERROR;
            return 0;
        }
        if (innerresult) {
            if (fserr)
                *fserr = FSERR_SUCCESS;
            *result = 1;
            return 1;
        }
    }
    if (fserr)
        *fserr = FSERR_NOSUCHTARGET;
    *result = 0;
    return 1;
}

SPEW3DVFS_FILE *spew3d_vfs_fopen(
        const char *path,
        const char *mode, int flags) {
    SPEW3DVFS_FILE *vfile = malloc(sizeof(*vfile));
    if (!vfile) {
        errno = ENOMEM;
        return 0;
    }
    memset(vfile, 0, sizeof(*vfile));
    vfile->mode = strdup(mode);
    if (!vfile->mode) {
        errno = ENOMEM;
        free(vfile);
        return 0;
    }
    vfile->size = -1;

    if ((flags & VFSFLAG_NO_VIRTUALPAK_ACCESS) == 0 &&
            !spew3d_fs_IsObviouslyInvalidPath(path) &&
            !spew3d_fs_IsAbsolutePath(path)) {
        mutex_Lock(spew3d_vfs_mutex);
        char *pathfixed = spew3d_vfs_NormalizePath(path);
        if (!pathfixed) {
            errno = ENOMEM;
            free(vfile->mode);
            free(vfile);
            mutex_Release(spew3d_vfs_mutex);
            return 0;
        }
        spew3d_vfs_mount *mount = _spew3d_global_mount_list;
        while (mount) {
            int foundasfolder = 0;
            int64_t foundidx = -1;
            if (spew3d_archive_GetEntryIndex(
                    mount->archive, pathfixed, &foundidx,
                    &foundasfolder
                    )) {
                int64_t _size = (
                    spew3d_archive_GetEntrySize(
                        mount->archive, foundidx)
                );;
                if (_size < 0) {
                    errno = EIO;
                    free(pathfixed);
                    free(vfile->mode);
                    free(vfile);
                    mutex_Release(spew3d_vfs_mutex);
                    return 0;
                }
                vfile->size = _size;
                vfile->via_mount = 1;
                vfile->path = pathfixed;
                vfile->src_mount = mount;
                vfile->src_entry = foundidx;
                mutex_Release(spew3d_vfs_mutex);
                return vfile;
            }
            mount = mount->next;
        }
        free(pathfixed);
        mutex_Release(spew3d_vfs_mutex);
    }
    if ((flags & VFSFLAG_NO_REALDISK_ACCESS) == 0) {
        vfile->via_mount = 0;
        errno = 0;
        int innererr = 0;
        vfile->diskhandle = spew3d_fs_OpenFromPath(
            path, mode, &innererr
        );
        if (!vfile->diskhandle) {
            errno = EIO;
            if (innererr == FSERR_NOSUCHTARGET)
                errno = ENOMEM;
            free(vfile->mode);
            free(vfile);
            return 0;
        }
        return vfile;
    }

    free(vfile->mode);
    free(vfile);
    errno = ENOENT;
    return 0;
}


SPEW3DVFS_FILE *spew3d_vfs_OwnThisFD(
        FILE *f, const char *reopenmode
        ) {
    SPEW3DVFS_FILE *vfile = malloc(sizeof(*vfile));
    if (!vfile)
        return NULL;
    memset(vfile, 0, sizeof(*vfile));
    vfile->mode = strdup(reopenmode);
    if (!vfile->mode) {
        free(vfile);
        return NULL;
    }
    vfile->size = -1;
    vfile->via_mount = 0;
    vfile->diskhandle = f;
    int64_t i = ftell64(vfile->diskhandle);
    if (i < 0) {
        free(vfile->mode);
        free(vfile);
        return NULL;
    }
    vfile->offset = i;
    return vfile;
}

void spew3d_vfs_DetachFD(SPEW3DVFS_FILE *f) {
    assert(f->via_mount == 0);
    f->diskhandle = NULL;
}

char *spew3d_vfs_NormalizePath(const char *path) {
    char *p = strdup(path);
    if (!p)
        return NULL;
    char *pnew = spew3d_fs_Normalize(p);
    free(p);
    if (!pnew)
        return NULL;
    p = pnew;
    #if defined(_WIN32) || defined(_WIN64)
    unsigned int k = 0;
    while (k < strlen(p)) {
        if (p[k] == '\\')
            p[k] = '/';
        k++;
    }
    #endif
    return p;
}

int spew3d_vfs_FileToBytes(
        const char *path, int vfsflags,
        int *out_fserr,
        char **out_bytes,
        uint64_t *out_bytes_len
        ) {
    #if defined(DEBUG_SPEW3D_VFS)
    fprintf(stderr, "spew3d_vfs.c: debug: "
        "spew3d_vfs_FileToBytes on: \"%s\" "
        "vfsflags:%d\n",
        path, vfsflags);
    #endif

    // Try to open via VFS first:
    if ((vfsflags & VFSFLAG_NO_VIRTUALPAK_ACCESS) == 0) {
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes trying to read from VFS\n");
        #endif

        // FIXME.
    }

    // Fall back to filesystem if not in VFS:
    if ((vfsflags & VFSFLAG_NO_REALDISK_ACCESS) == 0) {
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes falling back to disk\n");
        #endif
        int ferr = 0;
        FILE *f = spew3d_fs_OpenFromPath(
            path, "rb", &ferr
        ); 
        if (!f) {
            #if defined(DEBUG_SPEW3D_VFS)
            fprintf(stderr, "spew3d_vfs.c: debug: "
                "spew3d_vfs_FileToBytes: disk file open failed\n");
            #endif
            *out_fserr = ferr;
            return 0;
        }
        uint64_t filesize = 0;
        if (!spew3d_fs_GetSize(path, &filesize, &ferr)) {
            #if defined(DEBUG_SPEW3D_VFS)
            fprintf(stderr, "spew3d_vfs.c: debug: "
                "spew3d_vfs_FileToBytes: getting size failed\n");
            #endif
            fclose(f);
            *out_fserr = ferr;
            return 0;
        } 
        char *result_bytes = malloc(
            (filesize > 0ULL ? filesize : 1ULL)
        );
        if (!result_bytes) {
            #if defined(DEBUG_SPEW3D_VFS)
            fprintf(stderr, "spew3d_vfs.c: debug: "
                "spew3d_vfs_FileToBytes: allocating buffer "
                "for file contents failed\n");
            #endif
            fclose(f);
            *out_fserr = FSERR_OUTOFMEMORY;
            return 0;
        }
        char *p = result_bytes;
        uint64_t bytesread = 0;
        while (bytesread < filesize) {
            uint64_t readchunk = (filesize - bytesread);
            if (readchunk > 1024ULL * 1024ULL)
                readchunk = 1024ULL * 1024ULL;
            size_t result = fread(p, 1, readchunk, f);
            if ((uint64_t)result != readchunk) {
                #if defined(DEBUG_SPEW3D_VFS)
                fprintf(stderr, "spew3d_vfs.c: debug: "
                    "spew3d_vfs_FileToBytes: got fread() error\n");
                #endif
                fclose(f);
                free(result_bytes);
                int _err = FSERR_IOERROR;
                #if defined(_WIN32) || defined(_WIN64)
                uint64_t werror = GetLastError();
                if (werror == ERROR_NOT_ENOUGH_MEMORY)
                    _err = FSERR_OUTOFMEMORY:
                else if (werror == ERROR_ACCESS_DENIED ||
                        werror == ERROR_SHARING_VIOLATION)
                    _err = FSERR_NOPERMISSION;
                #else
                if (errno == ENOMEM)
                    _err = FSERR_OUTOFMEMORY;
                else if (errno == EACCES)
                    _err = FSERR_NOPERMISSION;
                #endif
                *out_fserr = _err;
                return 0;
            }
            p += (uint64_t)result;
            bytesread += (uint64_t)result;
        }
        *out_fserr = FSERR_SUCCESS;
        *out_bytes = result_bytes;
        *out_bytes_len = filesize;
        #if defined(DEBUG_SPEW3D_VFS)
        fprintf(stderr, "spew3d_vfs.c: debug: "
            "spew3d_vfs_FileToBytes completed successfully\n");
        #endif
        return 1;
    }

    *out_fserr = FSERR_NOSUCHTARGET;
    return 0;
}


#endif  // SPEW_IMPLEMENTATION

