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


#ifndef _SPEW3D_DUPFHANDLE_H_
#define _SPEW3D_DUPFHANDLE_H_

#include <stdio.h>
#include <unistd.h>


static FILE *_dupfhandle(FILE *f, const char* mode) {
    int fd = -1;
    int fd2 = -1;
    #if defined(_WIN32) || defined(_WIN64)
    fd = _fileno(f);
    fd2 = _dup(fd);
    #else
    fd = fileno(f);
    fd2 = dup(fd);
    #endif
    if (fd2 < 0)
        return NULL;
    FILE *f2 = fdopen(fd2, mode);
    if (!f2) {
        #if defined(_WIN32) || defined(_WIN64)
        _close(fd2);
        #else
        close(fd2);
        #endif
        return NULL;
    }
}

#endif  // _SPEW3D_DUPFHANDLE_H_

