/* Copyright (c) 2020-2023, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_TESTMAIN_H_
#define SPEW3D_TESTMAIN_H_

#include <assert.h>
#include <check.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#define TESTS_MAIN(...) \
void _custom_addtests(TCase *tc, ...) {\
    va_list vl;\
    va_start(vl, tc);\
    while (1) {\
        void *ptr = va_arg(vl, void*);\
        if (!ptr)\
            break;\
        tcase_add_test(tc, (const TTest *)ptr);\
    }\
    va_end(vl);\
}\
int main(void)\
{\
    Suite *s1 = suite_create("Core");\
    TCase *tc1_1 = tcase_create("Core");\
    SRunner *sr = srunner_create(s1);\
    srunner_set_fork_status(sr,CK_NOFORK);\
    int nf;\
\
    suite_add_tcase(s1, tc1_1);\
    _custom_addtests(tc1_1, __VA_ARGS__, NULL);\
\
    srunner_run_all(sr, CK_ENV);\
    nf = srunner_ntests_failed(sr);\
    srunner_free(sr);\
\
    return nf == 0 ? 0 : 1;\
}

#ifdef ck_assert
#undef ck_assert
#endif
#define ck_assert(x) assert(x)
static void _custom_assert_str_eq(const char *s1, const char *s2) {
    if ((s1 || s2) && (!s1 || !s2 || strcmp(s1, s2) != 0)) {
        fprintf(stderr, "testmain.h: ERROR: ck_assert_eq failed: "
            "\"%s\" != \"%s\"\n", s1, s2);
        fflush(stderr);
    }
    assert((!s1 && !s2) || (s1 && s2 && strcmp(s1, s2) == 0));
}
#ifdef ck_assert_str_eq
#undef ck_assert_str_eq
#endif
#define ck_assert_str_eq _custom_assert_str_eq

#endif  // SPEW3D_TESTMAIN_H_

