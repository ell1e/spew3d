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

#include <assert.h>
#include <check.h>
#include <string.h>

#define SPEW3D_OPTION_DISABLE_SDL
#define SPEW3D_IMPLEMENTATION
#include "spew3d.h"

#include "testmain.h"


START_TEST (test_stringutil_ReverseBufBytes)
{
    char a[] = "";
    spew3d_stringutil_ReverseBytes(a);
    assert(strlen(a) == 0);
    char b[] = "a";
    spew3d_stringutil_ReverseBytes(b);
    assert(strlen(b) == 1);
    assert(b[0] == 'a');
    char c[] = "abc";
    spew3d_stringutil_ReverseBytes(c);
    assert(strlen(c) == 3);
    assert(c[0] == 'c');
    assert(c[1] == 'b');
    assert(c[2] == 'a');
    char d[] = "abab";
    spew3d_stringutil_ReverseBytes(d);
    assert(strlen(d) == 4);
    assert(d[0] == 'b');
    assert(d[1] == 'a');
    assert(d[2] == 'b');
    assert(d[3] == 'a');
}
END_TEST

TESTS_MAIN(test_stringutil_ReverseBufBytes)
