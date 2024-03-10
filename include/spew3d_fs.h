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

#ifndef SPEW3D_FS_H_
#define SPEW3D_FS_H_

#include <stdint.h>
#include <stdio.h>

enum {
    FSERR_SUCCESS = 0,
    FSERR_NOPERMISSION = -1,
    FSERR_TARGETNOTADIRECTORY = -2,
    FSERR_TARGETNOTAFILE = -3,
    FSERR_NOSUCHTARGET = -4,
    FSERR_OUTOFMEMORY = -5,
    FSERR_TARGETALREADYEXISTS = -6,
    FSERR_INVALIDNAME = -7,
    FSERR_OUTOFFDS = -8,
    FSERR_PARENTSDONTEXIST = -9,
    FSERR_DIRISBUSY = -10,
    FSERR_NONEMPTYDIRECTORY = -11,
    FSERR_SYMLINKSWEREEXCLUDED = -12,
    FSERR_IOERROR = -13,
    FSERR_UNSUPPORTEDPLATFORM = -14,
    FSERR_TARGETTOOLARGE = -15,
    FSERR_OTHERERROR = -9999
};

S3DEXP FILE *spew3d_fs_OpenFromPath(
    const char *path, const char *mode, int *err
);

S3DEXP int spew3d_fs_GetSize(
    const char *path, uint64_t *size, int *err
);

S3DEXP int spew3d_fs_GetComponentCount(const char *path);

S3DEXP char *spew3d_fs_RemoveDoubleSlashes(const char *path);

S3DEXP int spew3d_fs_IsDirectory(const char *path, int *result);

S3DEXP int spew3d_fs_TargetExistsEx(
    const char *path, int *exists, int noperms_as_ioerror
);

S3DEXP char *spew3d_fs_Basename(const char *path);

S3DEXP char *spew3d_fs_ParentdirOfItem(const char *path);

S3DEXP char *spew3d_fs_GetOwnExecutablePath();

S3DEXP int spew3d_fs_TargetExists(const char *path, int *exists);

S3DEXP char *spew3d_fs_Normalize(const char *path);

S3DEXP char *spew3d_fs_NormalizeEx(
    const char *path, int always_allow_windows_separator,
    int never_allow_windows_separator,
    char unified_separator_to_use
);

S3DEXP int spew3d_fs_PathsLookEquivalentEx(
    const char *path1, const char *path2,
    const char *base_dir_as_cwd,
    int always_allow_case_insensitive,
    int never_allow_case_insensitive,
    int always_allow_windows_paths,
    int never_allow_windows_paths,
    int *result
);

S3DEXP int spew3d_fs_PathsLookEquivalent(
    const char *path1, const char *path2, int *result
);

S3DEXP int spew3d_fs_CreateDirectoryEx(
    const char *path, int onlyuserreadable
);

S3DEXP int spew3d_fs_CreateDirectory(const char *path);

S3DEXP void spew3d_fs_FreeFolderList(char **list);

S3DEXP int filesys_IsLink(
    const char *path, int *result
);

S3DEXP int spew3d_fs_ListFolderEx(
    const char *path, char ***contents,
    int returnFullPath, int allowsymlink,
    int *error
);

S3DEXP int spew3d_fs_ListFolder(
    const char *path, char ***contents,
    int returnFullPath, int *error
);

S3DEXP char *spew3d_fs_GetCurrentDirectory();

S3DEXP int spew3d_fs_IsAbsolutePath(const char *path);

S3DEXP char *spew3d_fs_ToAbsolutePath(const char *path);

S3DEXP int spew3d_fs_LaunchExecutable(
    const char *path, int argcount, const char **_orig_argv
);

S3DEXP char *spew3d_fs_Join(
    const char *path1, const char *path2_orig
);

S3DEXP char *spew3d_fs_GetSysTempdir();

S3DEXP int spew3d_fs_RemoveFile(const char *path, int *error);

S3DEXP int spew3d_fs_RemoveFolderRecursively(
    const char *path, int *error
);

S3DEXP FILE *spew3d_fs_TempFile(
    int subfolder, int folderonly,
    const char *prefix, const char *suffix,
    char **folder_path, char **path
);

#endif  // SPEW3D_FS_H_

