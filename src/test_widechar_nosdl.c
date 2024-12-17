/* Copyright (c) 2023-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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
#define SPEW3D_IMPLEMENTATION 1
#include "spew3d.h"

#include "testmain.h"


START_TEST (test_s3dstrcasecmp)
{
    ck_assert(s3dstrcasecmp(
        "a", "A"
    ) == 0);
    ck_assert(s3dstrcasecmp(
        "aB", "A"
    ) == (int)('B'));
}
END_TEST

START_TEST (test_utf8_str_to_lowercase)
{
    char input[] = "aBCtest❗Z";
    char expected[] = "abctest❗z";
    char *result = NULL;
    utf8_str_to_lowercase(
        input, strlen(input), NULL, &result,
        NULL
    );
    ck_assert(result != NULL);
    ck_assert(strlen(input) == strlen(result));
    ck_assert(memcmp(expected, result, strlen(input) + 1) == 0);
    free(result);
}
END_TEST

TESTS_MAIN(test_s3dstrcasecmp,
    test_utf8_str_to_lowercase)

