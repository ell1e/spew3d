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

#ifndef SPEW3D_TEXTURE_H_
#define SPEW3D_TEXTURE_H_

#include <stdint.h>
#include <SDL2/SDL.h>

typedef uint64_t spew3d_texture_t;

typedef struct spew3d_texture_info {
    char *idstring, *diskpath;
    uint8_t loaded, correspondstofile;
    uint32_t width, height;
    char *pixels;

    void *_internal;
} spew3d_texture_info;


spew3d_texture_t spew3d_texture_FromFile(
    const char *path
);

spew3d_texture_info *spew3d_texinfo(
    spew3d_texture_t id
);

spew3d_texture_t spew3d_texture_NewWritable(
    const char *name, uint32_t w, uint32_t h
);

spew3d_texture_t spew3d_texture_NewWritableFromFile(
    const char *name, const char *original_path
);

void spew3d_texture_Destroy(spew3d_texture_t tid);

int spew3d_texture_Draw(
    spew3d_texture_t tid,
    int32_t x, int32_t y, double scale, double angle,
    double tint_red, double tint_white, double tint_blue,
    double transparency,
    int withalphachannel
);

#endif  // SPEW3D_TEXTURE_H_

