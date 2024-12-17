/* Copyright (c) 2020-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#undef MZ_FILE
#define MZ_FILE FILE
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

#define UNCACHED_FILE_SIZE (1024 * 5)

typedef struct spew3darchive {
    spew3darchivetype archive_type;
    union {
        struct {  // archive_type == SPEW3DARCHIVE_TYPE_ZIP
            uint8_t in_writing_mode;
            int64_t cached_entry_count;
            char **cached_entry;
            mz_zip_archive zip_archive;
            char *_last_returned_name;
        };
    };
    SPEW3DVFS_FILE *f;
    struct {
        int extract_cache_count;
        char **extract_cache_temp_path;
        char **extract_cache_temp_folder;
        char **extract_cache_orig_name;
        char *_read_buf;
    };
    uint8_t is_case_insensitive;
} spew3darchive;

S3DEXP int64_t spew3d_archive_GetEntryCount(
        spew3darchive *a
        ) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        if (a->in_writing_mode)
            return a->cached_entry_count;
        return mz_zip_reader_get_num_files(&a->zip_archive);
    } else {
        return -1;
    }
}

S3DEXP int spew3d_archive_GetEntryIsDir(
        spew3darchive *a, uint64_t entry
        ) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        mz_zip_archive_file_stat stat = {0};
        mz_bool result = mz_zip_reader_file_stat(
            &a->zip_archive, entry, &stat
        );
        if (!result)
            return -1;
        return (stat.m_is_directory != 0);
    } else {
        return -1;
    }
}

S3DEXP int64_t spew3d_archive_GetEntrySize(
        spew3darchive *a, uint64_t entry
        ) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        mz_zip_archive_file_stat stat = {0};
        mz_bool result = mz_zip_reader_file_stat(
            &a->zip_archive, entry, &stat
        );
        if (!result)
            return -1;
        if (stat.m_is_directory)
            return 0;
        if (stat.m_uncomp_size > (uint64_t)INT64_MAX)
            return -1;
        return stat.m_uncomp_size;
    }
    return -1;
}

S3DEXP char *spew3d_archive_NormalizeName(const char *name) {
    char *nname = strdup(name);
    if (!nname)
        return NULL;
    uint64_t i = 0;
    while (i < strlen(nname)) {
        if (nname[i] == '\\')
            nname[i] = '/';
        if (nname[i] == '/' &&
                i + 1 < strlen(nname) &&
                nname[i + 1] == '/') {
            memcpy(
                &nname[i], &nname[i + 1],
                strlen(nname) - i
            );
            continue;  // no i++ here!
        }
        i++;
    }
    if (strlen(nname) > 0 && name[strlen(nname) - 1] == '/')
        nname[strlen(nname) - 1] = '\0';
    return nname;
}

S3DEXP int spew3d_archive_GetEntryIndex(
        spew3darchive *a, const char *filename, int64_t *index,
        int *existsasfolder
        ) {
    *existsasfolder = 0;
    char *cleanname = spew3d_archive_NormalizeName(filename);
    int64_t entry_count = spew3d_archive_GetEntryCount(a);
    int64_t i2 = 0;
    while (i2 < entry_count) {
        const char *e = spew3d_archive_GetEntryName(a, i2);
        if (!e) {
            free(cleanname);
            return 0;
        }
        char *eclean = spew3d_archive_NormalizeName(e);
        if (!eclean) {
            free(cleanname);
            return 0;
        }
        if (strcmp(eclean, cleanname) == 0 || (
                a->is_case_insensitive &&
                s3dstrcasecmp(eclean, cleanname) == 0)) {
            free(eclean);
            free(cleanname);
            *index = i2;
            return 1;
        } else if (strlen(eclean) > strlen(cleanname) && (
                cleanname[strlen(cleanname)] == '/' ||
                cleanname[strlen(cleanname)] == '\\')) {
            *existsasfolder = 1;
        }
        free(eclean);
        i2++;
    }
    free(cleanname);
    *index = -1;
    return 1;
}

S3DEXP const char *spew3d_archive_GetEntryName(
        spew3darchive *a, uint64_t entry
        ) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        if (a->in_writing_mode) {
            if (entry >= (uint64_t)a->cached_entry_count)
                return NULL;
            assert(a->cached_entry != NULL);
            return a->cached_entry[entry];
        }
        if (a->_last_returned_name) {
            free(a->_last_returned_name);
            a->_last_returned_name = NULL;
        }
        if (entry >= mz_zip_reader_get_num_files(&a->zip_archive))
            return NULL;
        uint64_t neededlen = mz_zip_reader_get_filename(
            &a->zip_archive, entry, NULL, 0
        );
        a->_last_returned_name = malloc(neededlen + 1);
        if (!a->_last_returned_name) {
            return NULL;
        }
        uint64_t written = mz_zip_reader_get_filename(
            &a->zip_archive, entry, a->_last_returned_name, neededlen + 1
        );
        if (written > neededlen)
            written = 0;
        a->_last_returned_name[written] = '\0';
        return a->_last_returned_name;
    } else {
        return NULL;
    }
}

S3DHID int _spew3d_archive_ReadFileToMemDirectly(
        spew3darchive *a, int64_t entry, char *buf, size_t buflen
        ) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        mz_bool result = mz_zip_reader_extract_to_mem(
            &a->zip_archive, entry, buf, buflen, 0
        );
        if (!result)
            return 0;
        return 1;
    }
    return 0;
}

S3DHID int _spew3d_archive_ReadFileToFPtr(
        spew3darchive *a, int64_t entry, FILE *f
        ) {
    const char *e = spew3d_archive_GetEntryName(a, entry);
    if (!e)
        return 0;
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        mz_bool result = mz_zip_reader_extract_file_to_cfile(
            &a->zip_archive, e, f, 0
        );
        if (!result)
            return 0;
        return 1;
    }
    return 0;
}

S3DEXP const char *_spew3d_archive_GetFileCachePath(
        spew3darchive *a, int64_t entry
        ) {
    const char *e = spew3d_archive_GetEntryName(a, entry);
    if (!e)
        return NULL;
    int64_t i = 0;
    while (i < a->extract_cache_count) {
        if (strcmp(a->extract_cache_orig_name[i], e) == 0) {
            return a->extract_cache_temp_path[i];
        }
        i++;
    }
    char **orig_name_new = realloc(
        a->extract_cache_orig_name,
        sizeof(*a->extract_cache_orig_name) *
            (a->extract_cache_count + 1)
    );
    if (!orig_name_new)
        return NULL;
    a->extract_cache_orig_name = orig_name_new;
    char **temp_folder_new = realloc(
        a->extract_cache_temp_path,
        sizeof(*a->extract_cache_temp_path) *
            (a->extract_cache_count + 1)
    );
    if (!temp_folder_new) {
        return NULL;
    }
    a->extract_cache_temp_path = temp_folder_new;
    char **temp_path_new = realloc(
        a->extract_cache_temp_folder,
        sizeof(*a->extract_cache_temp_folder) *
            (a->extract_cache_count + 1)
    );
    if (!temp_path_new) {
        return NULL;
    }
    a->extract_cache_temp_folder = temp_path_new;

    char *spew3darchive_s = strdup("spew3darchive-");
    if (!spew3darchive_s) {
        return NULL;
    }
    char *folder_path = NULL;
    char *full_path = NULL;
    int64_t full_path_len = 0;
    FILE *f = spew3d_fs_TempFile(
        1, 0, spew3darchive_s, NULL,
        &folder_path, &full_path
    );
    free(spew3darchive_s);
    if (!f) {
        return NULL;
    }
    if (!_spew3d_archive_ReadFileToFPtr(a, entry, f)) {
        free(full_path);
        free(folder_path);
        fclose(f);
        return NULL;
    }
    fclose(f);
    a->extract_cache_orig_name[
        a->extract_cache_count
    ] = strdup(e);
    if (!a->extract_cache_orig_name[
            a->extract_cache_count
            ]) {
        int error = 0;
        spew3d_fs_RemoveFile(
            full_path, &error
        );
        spew3d_fs_RemoveFolderRecursively(
            folder_path, &error
        );
        free(full_path);
        free(folder_path);
        return NULL;
    }
    a->extract_cache_temp_path[
        a->extract_cache_count
    ] = full_path;
    a->extract_cache_temp_folder[
        a->extract_cache_count
    ] = folder_path;
    a->extract_cache_count++;
    return full_path;
}

S3DEXP int spew3d_archive_ReadFileByteSlice(
        spew3darchive *a, int64_t entry,
        uint64_t offset, char *buf, size_t readlen
        ) {
    int64_t fsize = spew3d_archive_GetEntrySize(a, entry);
    if (fsize < 0)
        return 0;
    if (offset + readlen > (uint64_t)fsize)
        return 0;
    if (fsize < UNCACHED_FILE_SIZE) {
        if (!a->_read_buf) {
            a->_read_buf = malloc(UNCACHED_FILE_SIZE);
            if (!a->_read_buf)
                return 0;
        }
        int result = _spew3d_archive_ReadFileToMemDirectly(
            a, entry, a->_read_buf, fsize
        );
        if (!result)
            return 0;
        memcpy(
            buf, a->_read_buf + offset, readlen
        );
        return 1;
    }
    const char *file_path = (
        _spew3d_archive_GetFileCachePath(
            a, entry
        ));
    if (!file_path)
        return 0;
    int innererr = 0;
    FILE *f = spew3d_fs_OpenFromPath(
        file_path, "rb", &innererr
    );
    if (!f)
        return 0;
    if (fseek64(f, offset, SEEK_SET) != 0) {
        fclose(f);
        return 0;
    }
    size_t result = fread(buf, 1, readlen, f);
    fclose(f);
    if (result == readlen)
        return 1;
    return 0;
}

S3DHID int _spew3d_archive_EnableWriting(spew3darchive *a) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        if (a->in_writing_mode)
            return 1;
        if (a->cached_entry) {
            int64_t i = 0;
            while (i < a->cached_entry_count) {
                free(a->cached_entry[i]);
                i++;
            }
            free(a->cached_entry);
        }
        a->cached_entry_count = spew3d_archive_GetEntryCount(a);
        a->cached_entry = malloc(
            sizeof(a->cached_entry) * (
                a->cached_entry_count > 0 ?
                a->cached_entry_count : 1
            )
        );
        if (!a->cached_entry) {
            a->cached_entry_count = 0;
            return 0;
        }
        int64_t i = 0;
        while (i < a->cached_entry_count) {
            const char *e = spew3d_archive_GetEntryName(
                a, i
            );
            if (e)
                a->cached_entry[i] = strdup(e);
            if (!a->cached_entry[i]) {
                int64_t k = 0;
                while (k < i) {
                    free(a->cached_entry[k]);
                    k++;
                }
                free(a->cached_entry);
                a->cached_entry = NULL;
                a->cached_entry_count = 0;
                return 0;
            }
            i++;
        }
        mz_bool result = mz_zip_writer_init_from_reader_v2(
            &a->zip_archive, NULL,
            MZ_ZIP_FLAG_WRITE_ZIP64 |
            MZ_ZIP_FLAG_CASE_SENSITIVE |
            MZ_ZIP_FLAG_WRITE_ALLOW_READING
        );
        if (!result) {
            return 0;
        }
        a->in_writing_mode = 1;
        return 1;
    }
    return 1;
}

S3DHID int _spew3d_archive_AddSingleDirEntry(
        spew3darchive *a, const char *dirname
        ) {
    if (!a)
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    if (!_spew3d_archive_EnableWriting(a)) {
        return SPEW3DARCHIVE_ADDERROR_IOERROR;
    }

    char *cleanname = spew3d_archive_NormalizeName(dirname);
    if (!cleanname)
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    char *dir = malloc(strlen(cleanname) + 2);
    if (!dir) {
        free(cleanname);
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    }
    memcpy(dir, cleanname, strlen(cleanname) + 1);
    free(cleanname);
    cleanname = NULL;
    while (strlen(dir) > 0 && dir[strlen(dir) - 1] == '/')
        dir[strlen(dir) - 1] = '\0';
    if (strlen(dir) == 0) {
        free(dir);
        return SPEW3DARCHIVE_ADDERROR_DUPLICATENAME;
    }

    int i = 0;
    while (i < a->cached_entry_count) {
        if (strcmp(a->cached_entry[i], dir) == 0) {
            free(dir);
            return SPEW3DARCHIVE_ADDERROR_DUPLICATENAME;
        }
        i++;
    }

    dir[strlen(dir) + 1] = '\0';
    dir[strlen(dir)] = '/';
    mz_bool result2 = mz_zip_writer_add_mem(
        &a->zip_archive, dir, NULL, 0,
        MZ_BEST_COMPRESSION
    );
    if (!result2) {
        free(dir);
        return SPEW3DARCHIVE_ADDERROR_IOERROR;
    }
    a->cached_entry[a->cached_entry_count] = dir;
    a->cached_entry_count++;
    return SPEW3DARCHIVE_ADDERROR_SUCCESS;
}

S3DEXP int spew3d_archive_AddDir(spew3darchive *a,
        const char *dirname) {
    if (!a)
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    if (!_spew3d_archive_EnableWriting(a)) {
        return SPEW3DARCHIVE_ADDERROR_IOERROR;
    }

    unsigned int i = 0;
    while (i < strlen(dirname)) {
        if (!starts_with_valid_utf8_char(
                (uint8_t *)(dirname + i), strlen(dirname + i)
                )) {
            return SPEW3DARCHIVE_ADDERROR_INVALIDNAME;
        }
        i += utf8_char_len((uint8_t *)(dirname + i));
    }

    char *cleanname = spew3d_archive_NormalizeName(dirname);
    if (!cleanname)
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    char *dir = malloc(strlen(cleanname) + 2);
    if (!dir) {
        free(cleanname);
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    }
    memcpy(dir, cleanname, strlen(cleanname) + 1);
    free(cleanname);
    cleanname = NULL;
    while (strlen(dir) > 0 && dir[strlen(dir) - 1] == '/')
        dir[strlen(dir) - 1] = '\0';
    if (strlen(dir) == 0) {
        free(dir);
        return SPEW3DARCHIVE_ADDERROR_DUPLICATENAME;
    }

    int k = 0;
    while (k < (int)strlen(dir)) {
        if (dir[k] == '/' && k > 0 && dir[k - 1] != '/') {
            char *component = strdup(dir);
            if (!component) {
                free(dir);
                return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
            }
            component[k] = '\0';
            int result = _spew3d_archive_AddSingleDirEntry(
                a, component
            );
            free(component);
            if (result != SPEW3DARCHIVE_ADDERROR_SUCCESS) {
                free(dir);
                return result;
            }
        }
        k++;
    }
    int result = _spew3d_archive_AddSingleDirEntry(a, dir);
    if (result != SPEW3DARCHIVE_ADDERROR_SUCCESS) {
        free(dir);
        return result;
    }
    free(dir);
    return SPEW3DARCHIVE_ADDERROR_SUCCESS;
}

S3DEXP int _spew3d_archive_AddFile_CheckName(
        spew3darchive *a, const char *filename,
        char **cleaned_name
        ) {
    if (strlen(filename) == 0 ||
            filename[strlen(filename) - 1] == '/' ||
            filename[strlen(filename) - 1] == '\\' ||
            filename[0] == '/' || filename[0] == '\\'
            )
        return SPEW3DARCHIVE_ADDERROR_INVALIDNAME;
    unsigned int i = 0;
    while (i < strlen(filename)) {
        if (!starts_with_valid_utf8_char(
                (uint8_t *)(filename + i), strlen(filename + i)
                )) {
            return SPEW3DARCHIVE_ADDERROR_INVALIDNAME;
        }
        i += utf8_char_len((uint8_t *)(filename + i));
    }
    char *clean_name = spew3d_archive_NormalizeName(filename);
    if (!clean_name)
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    int64_t entry_count = spew3d_archive_GetEntryCount(a);
    int64_t i2 = 0;
    while (i2 < entry_count) {
        const char *e = spew3d_archive_GetEntryName(a, i2);
        if (!e) {
            free(clean_name);
            return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
        }
        char *eclean = spew3d_archive_NormalizeName(e);
        if (!eclean) {
            free(clean_name);
            return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
        }
        if (strcmp(eclean, clean_name) == 0) {
            free(eclean);
            free(clean_name);
            return SPEW3DARCHIVE_ADDERROR_DUPLICATENAME;
        }
        free(eclean);
        i2++;
    }
    *cleaned_name = clean_name;
    return SPEW3DARCHIVE_ADDERROR_SUCCESS;
}

S3DEXP int spew3d_archive_AddFileFromMem(
        spew3darchive *a, const char *filename,
        const char *bytes, uint64_t byteslen
        ) {
    char *cleaned_name = NULL;
    int result = _spew3d_archive_AddFile_CheckName(
        a, filename, &cleaned_name
    );
    if (result != SPEW3DARCHIVE_ADDERROR_SUCCESS)
        return result;
    if (strlen(cleaned_name) > 0 &&
            cleaned_name[strlen(cleaned_name) - 1] == '/') {
        return SPEW3DARCHIVE_ADDERROR_INVALIDNAME;
    }
    if (!_spew3d_archive_EnableWriting(a)) {
        free(cleaned_name);
        return SPEW3DARCHIVE_ADDERROR_IOERROR;
    }

    // First, ensure all sub-directories exist:
    int k = 0;
    while (k < (int)strlen(cleaned_name)) {
        if (cleaned_name[k] == '/' && k > 0 &&
                cleaned_name[k - 1] != '/') {
            cleaned_name[k] = '\0';
            int result = spew3d_archive_AddDir(a, cleaned_name);
            cleaned_name[k] = '/';
            if (result != SPEW3DARCHIVE_ADDERROR_SUCCESS &&
                    result != SPEW3DARCHIVE_ADDERROR_DUPLICATENAME) {
                free(cleaned_name);
                return result;
            }
        }
        k++;
    }

    // Then, add file itself:
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        char **_expanded_names = realloc(
            a->cached_entry,
            sizeof(*a->cached_entry) * (a->cached_entry_count + 1)
        );
        if (!_expanded_names) {
            free(cleaned_name);
            return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
        }
        a->cached_entry = _expanded_names;
        mz_bool result2 = mz_zip_writer_add_mem(
            &a->zip_archive, cleaned_name, bytes, byteslen,
            MZ_BEST_COMPRESSION
        );
        if (!result2) {
            free(cleaned_name);
            return SPEW3DARCHIVE_ADDERROR_IOERROR;
        }
        a->cached_entry[a->cached_entry_count] = cleaned_name;
        a->cached_entry_count++;
        return SPEW3DARCHIVE_ADDERROR_SUCCESS;
    } else {
        free(cleaned_name);
        return SPEW3DARCHIVE_ADDERROR_OUTOFMEMORY;
    }
}

S3DEXP void spew3d_archive_Close(spew3darchive *a) {
    if (a->archive_type == SPEW3DARCHIVE_TYPE_ZIP) {
        if (a->in_writing_mode) {
            mz_zip_writer_finalize_archive(&a->zip_archive);
            mz_zip_writer_end(&a->zip_archive);
        }
        mz_zip_reader_end(&a->zip_archive);
        if (a->cached_entry) {
            int64_t i = 0;
            while (i < a->cached_entry_count) {
                free(a->cached_entry[i]);
                i++;
            }
            free(a->cached_entry);
        }
        free(a->_last_returned_name);
    }
    spew3d_vfs_fclose(a->f);
    {
        int64_t i = 0;
        while (i < a->extract_cache_count) {
            int error = 0;
            spew3d_fs_RemoveFile(
                a->extract_cache_temp_path[i],
                &error
            );
            spew3d_fs_RemoveFolderRecursively(
                a->extract_cache_temp_folder[i],
                &error
            );
            free(a->extract_cache_temp_path[i]);
            free(a->extract_cache_temp_folder[i]);
            free(a->extract_cache_orig_name[i]);
            i++;
        }
        free(a->extract_cache_orig_name);
        free(a->extract_cache_temp_path);
        free(a->extract_cache_temp_folder);
    }
    free(a);
}

S3DHID static size_t miniz_read_spew3darchive(
        void *pOpaque, mz_uint64 file_ofs, void *pBuf, size_t n
        ) {
    spew3darchive *a = (spew3darchive *)pOpaque;
    assert(a != NULL);
    if (spew3d_vfs_fseek(a->f, file_ofs) != 0)
        return 0;
    size_t result = spew3d_vfs_fread(pBuf, 1, n, a->f);
    return result;
}

S3DHID static size_t miniz_write_spew3darchive(
        void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n
        ) {
    spew3darchive *a = (spew3darchive *)pOpaque;
    if (spew3d_vfs_fseek(a->f, file_ofs) != 0)
        return 0;
    size_t result = spew3d_vfs_fwrite(pBuf, 1, n, a->f);
    return result;
}

S3DEXP spew3darchive *spew3d_archive_FromVFSHandleEx(
        SPEW3DVFS_FILE *f, int createifmissing, spew3darchivetype type
        ) {
    if (!f)
        return NULL;
    SPEW3DVFS_FILE *fnew = spew3d_vfs_fdup(f);
    if (!fnew)
        return NULL;
    spew3darchive *a = malloc(sizeof(*a));
    if (!a) {
        spew3d_vfs_fclose(fnew);
        return NULL;
    }
    memset(a, 0, sizeof(*a));
    a->f = fnew;
    if (!spew3d_vfs_fseektoend(a->f)) {
        spew3d_vfs_fclose(fnew);
        free(a);
        return NULL;
    }
    int64_t size = spew3d_vfs_ftell(a->f);
    if (spew3d_vfs_fseek(a->f, 0) != 0) {
        spew3d_vfs_fclose(fnew);
        free(a);
        return NULL;
    }
    
    // If file is empty, don't allow autodetect or opening without create:
    if ((type == SPEW3DARCHIVE_TYPE_AUTODETECT ||
            !createifmissing) && size == 0) {
        spew3d_vfs_fclose(fnew);
        free(a);
        return NULL;
    }

    // Otherwise, try to open as zip:
    if (type == SPEW3DARCHIVE_TYPE_AUTODETECT)
        type = SPEW3DARCHIVE_TYPE_ZIP;  // FIXME: Update for other formats.

    // Handle the various archive types:
    a->archive_type = type;
    if (type == SPEW3DARCHIVE_TYPE_ZIP) {
        a->zip_archive.m_pRead = miniz_read_spew3darchive;
        a->zip_archive.m_pWrite = miniz_write_spew3darchive;
        a->zip_archive.m_pIO_opaque = a;
        if (size == 0 && createifmissing) {
            // Create new archive from scratch:
            mz_bool result = mz_zip_writer_init_v2(
                &a->zip_archive, size,
                MZ_ZIP_FLAG_WRITE_ZIP64 |
                MZ_ZIP_FLAG_CASE_SENSITIVE |
                MZ_ZIP_FLAG_WRITE_ALLOW_READING
            );
            if (!result) {
                spew3d_vfs_fclose(fnew);
                free(a);
                return NULL;
            }
            if (!mz_zip_writer_finalize_archive(&a->zip_archive)) {
                mz_zip_writer_end(&a->zip_archive);
                spew3d_vfs_fclose(fnew);
                free(a);
                return NULL;
            }
            mz_zip_writer_end(&a->zip_archive);
            // Update our archive size (which we pass to the reader below):
            if (!spew3d_vfs_fseektoend(a->f)) {
                spew3d_vfs_fclose(fnew);
                free(a);
                return NULL;
            }
            size = spew3d_vfs_ftell(a->f);
            if (size <= 0) {
                spew3d_vfs_fclose(fnew);
                free(a);
                return NULL;
            }
            // Seek to beginning again:
            if (spew3d_vfs_fseek(a->f, 0) != 0) {
                spew3d_vfs_fclose(fnew);
                free(a);
                return NULL;
            }
        }
        // Get a reader.
        mz_bool result = mz_zip_reader_init(
            &a->zip_archive, size,
            MZ_ZIP_FLAG_WRITE_ZIP64 |
            MZ_ZIP_FLAG_CASE_SENSITIVE |
            MZ_ZIP_FLAG_WRITE_ALLOW_READING
        );
        if (!result) {
            spew3d_vfs_fclose(fnew);
            free(a);
            return NULL;
        }
        return a;
    } else {
        // Unsupported archive.
        spew3d_vfs_fclose(fnew);
        free(a);
        return NULL;
    }
}

S3DEXP spew3darchive *spew3d_archive_FromVFSHandle(
        SPEW3DVFS_FILE *f,  spew3darchivetype type
        ) {
    return spew3d_archive_FromVFSHandleEx(f, 0, type);
}

S3DEXP spew3darchive *spew3d_archive_FromFilePathSlice(
        const char *path,
        uint64_t fileoffset, uint64_t maxlen,
        int createifmissing, int vfsflags,
        spew3darchivetype type
        ) {
    // Get the VFS file:
    errno = 0;
    SPEW3DVFS_FILE *f = spew3d_vfs_fopen(
        path, "r+b", vfsflags
    );
    if (!f && errno == ENOENT && createifmissing) {
        f = spew3d_vfs_fopen(
            path,
            "w+b", VFSFLAG_NO_VIRTUALPAK_ACCESS
        );
    }
    if (f) {
        // Make sure the offset parameters are applied & valid:
        if (fileoffset > 0 || maxlen > 0) {
            if (!spew3d_vfs_fseektoend(f)) {
                spew3d_vfs_fclose(f);
                return NULL;
            }
            int64_t end = spew3d_vfs_ftell(f);
            if (end < 0 || fileoffset + maxlen > (uint64_t)end ||
                    fileoffset >= (uint64_t)end) {
                spew3d_vfs_fclose(f);
                return NULL;
            }
            if (maxlen == 0) {
                maxlen = (uint64_t)end - fileoffset;
            }
            if (!spew3d_vfs_flimitslice(f, fileoffset, maxlen)) {
                spew3d_vfs_fclose(f);
                return NULL;
            }
        }
        // Now load archive:
        spew3darchive *a = spew3d_archive_FromVFSHandleEx(
            f, createifmissing, type);
        spew3d_vfs_fclose(f);
        return a;
    }
    spew3d_vfs_fclose(f);
    return NULL;
}

S3DEXP spew3darchive *spew3d_archive_FromFileHandleSlice(
        FILE *origf, uint64_t fileoffset, uint64_t maxlen,
        spew3darchivetype type, int fdiswritable
        ) {
    // Get the VFS file:
    errno = 0;
    SPEW3DVFS_FILE *f = spew3d_vfs_OwnThisFD(
        origf, (fdiswritable ? "r+b" : "rb")
    );
    if (f) {
        // Make sure the offset parameters are applied & valid:
        if (fileoffset > 0 || maxlen > 0) {
            if (!spew3d_vfs_fseektoend(f)) {
                spew3d_vfs_DetachFD(f);
                spew3d_vfs_fclose(f);
                return NULL;
            }
            int64_t end = spew3d_vfs_ftell(f);
            if (end < 0 || fileoffset + maxlen > (uint64_t)end ||
                    fileoffset >= (uint64_t)end) {
                spew3d_vfs_DetachFD(f);
                spew3d_vfs_fclose(f);
                return NULL;
            }
            if (maxlen == 0) {
                maxlen = (uint64_t)end - fileoffset;
            }
            if (!spew3d_vfs_flimitslice(f, fileoffset, maxlen)) {
                spew3d_vfs_DetachFD(f);
                spew3d_vfs_fclose(f);
                return NULL;
            }
        }
        // Now load archive:
        spew3darchive *a = spew3d_archive_FromVFSHandleEx(
            f, 0, type
        );
        spew3d_vfs_DetachFD(f);
        spew3d_vfs_fclose(f);
        return a;
    }
    spew3d_vfs_DetachFD(f);
    spew3d_vfs_fclose(f);
    return NULL;
}

S3DEXP spew3darchive *spew3d_archive_FromFilePath(
        const char *path,
        int createifmissing, int vfsflags,
        spew3darchivetype type
        ) {
    return spew3d_archive_FromFilePathSlice(
        path, 0, 0, createifmissing, vfsflags, type
    );
}

S3DEXP int spew3d_archive_IsCaseInsensitive(spew3darchive *a) {
    return a->is_case_insensitive;
}

S3DEXP void spew3d_archive_SetCaseInsensitive(
        spew3darchive *a, int enabled
        ) {
    a->is_case_insensitive = (enabled != 0);
}

#endif  // SPEW3D_IMPLEMENTATION

