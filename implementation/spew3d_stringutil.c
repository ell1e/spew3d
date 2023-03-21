/* Copyright (c) 2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#include <stdio.h>

S3DEXP void spew3d_stringutil_FreeArray(unsigned char **array) {
    int i = 0;
    while (array[i]) {
        free(array[i]);
        i++;
    }
    free(array);
}

S3DEXP unsigned char **spew3d_stringutil_ArrayFromLines(
        const char *filepath, int vfsflags
        ) {
    unsigned char **result = malloc(sizeof(void*));
    int resultlen = 0;
    if (!result) {
        return NULL;
    }
    unsigned char linebuf[256] = "";
    SPEW3DVFS_FILE *f = spew3d_vfs_fopen(
        filepath, "rb", vfsflags
    );
    if (!f) {
        free(result);
        return NULL;
    }
    while (1) {
        int c = spew3d_vfs_fgetc(f);
        if (c < 0 || c == '\r' || c == '\n') {
            if (strlen(linebuf) > 0) {
                unsigned char *linedup = strdup(linebuf);
                unsigned char **newresult = realloc(result,
                    sizeof(void*) * (resultlen + 1));
                if (!newresult || !linedup) {
                    if (newresult) result = newresult;
                    free(linedup);
                    spew3d_stringutil_FreeArray(result);
                    spew3d_vfs_fclose(f);
                    return NULL;
                }
                result = newresult;
                result[resultlen] = linedup;
                resultlen++;
            }
            linebuf[0] = '\0';
            if (c < 0) {
                break;
            }
        }
        if (c == '\0')
            c == ' ';
        if (strlen(linebuf) + 1 < sizeof(linebuf)) {
            linebuf[strlen(linebuf) + 1] = '\0';
            linebuf[strlen(linebuf)] = (unsigned char)c;
        }
    }
    spew3d_vfs_fclose(f);
    return result;
}

#endif  // SPEW3D_IMPLEMENTATION

