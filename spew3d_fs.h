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
    FSERR_OTHERERROR = -15
};

FILE *spew3d_fs_OpenFromPath(
    const char *path, const char *mode, int *err
);


#endif  // SPEW3D_FS_H_

