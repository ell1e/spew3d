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

#include <stdint.h>
#include <stdio.h>


S3DEXP void spew3d_stringutil_FreeArray(unsigned char **array) {
    if (!array)
        return;
    int i = 0;
    while (array[i]) {
        free(array[i]);
        i++;
    }
    free(array);
}

S3DEXP unsigned char **spew3d_stringutil_ArrayFromLines(
        const char *filepath, int vfsflags, int64_t *output_len
        ) {
    // Helper variables to hold result array and current line:
    unsigned char **result = malloc(sizeof(void*));
    int64_t resultlen = 0;
    if (!result) {
        return NULL;
    }
    unsigned char _linebufshort[256] = "";
    unsigned char *linebuf = _linebufshort;
    int linebufalloc = 256;
    int linebufonheap = 0;

    // Open the target file:
    SPEW3DVFS_FILE *f = spew3d_vfs_fopen(
        filepath, "rb", vfsflags
    );
    if (!f) {
        free(result);
        return NULL;
    }

    // Loop through file line by line:
    int last_was_ascii_r = 0;
    while (1) {
        int c = spew3d_vfs_fgetc(f);

        // Skip over the second byte of any \r\n windows line break:
        if (last_was_ascii_r && c == '\n') {
            last_was_ascii_r = 0;
            continue;
        }

        // See if we are at a point that completes a line:
        if (c < 0 || c == '\r' || c == '\n') {
            if (strlen(linebuf) > 0) {  // We got a non-empty line:
                unsigned char *linedup = strdup(linebuf);
                unsigned char **newresult = realloc(result,
                    sizeof(void*) * (resultlen + 2));
                if (!newresult || !linedup) {
                    if (newresult)
                        result = newresult;
                    free(linedup);
                    result[resultlen] = NULL;

                    // Generic error bail code path:
                    errorquit: ;
                    if (linebufonheap)
                        free(linebuf);
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
            last_was_ascii_r = (c== '\r');
            continue;
        }

        // If we arrive here, this extends the current line by one.
        last_was_ascii_r = 0;
        if (c == '\0')  // Don't allow null bytes.
            c == ' ';

        // Resize line buffer if it's too small:
        if (strlen(linebuf) + 1 > linebufalloc) {
            int newalloc = linebufalloc * 2;
            char *linebufnew = NULL;
            if (linebufonheap)
                linebufnew = realloc(linebuf, newalloc);
            else
                linebufnew = malloc(newalloc);
            if (!linebufnew)
                goto errorquit;
            if (!linebufonheap)
                memcpy(linebufnew, linebuf, linebufalloc);
            linebuf = linebufnew;
            linebufalloc = newalloc;
            linebufonheap = 1;
        }
        assert(strlen(linebuf) + 1 < linebufalloc);
        linebuf[strlen(linebuf) + 1] = '\0';
        linebuf[strlen(linebuf)] = (unsigned char)c;
    }

    // This is the success end case, we made it through.
    spew3d_vfs_fclose(f);
    if (output_len)
        *output_len = resultlen;
    result[resultlen] = NULL;
    if (linebufonheap)
        free(linebuf);
    return result;
}

#endif  // SPEW3D_IMPLEMENTATION

