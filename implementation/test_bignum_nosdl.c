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

#include <assert.h>
#include <check.h>

#define SPEW3D_OPTION_DISABLE_SDL
#define SPEW3D_IMPLEMENTATION
#include "spew3d.h"

#include "testmain.h"


START_TEST (test_bignum)
{
    int result;
    result = spew3d_bignum_CompareStrInts(
        "0", "0"
    );
    assert(result == 0);
    result = spew3d_bignum_CompareStrInts(
        "-1", "0"
    );
    assert(result == -1);
    result = spew3d_bignum_CompareStrInts(
        "0", "-0"
    );
    assert(result == 0);
    result = spew3d_bignum_CompareStrInts(
        "-0", "-0000"
    );
    assert(result == 0);
    result = spew3d_bignum_CompareStrInts(
        "12345678123456781234567812345678123456781234567812345678",
        "12345678123456781234567812345678123456781234567812345679"
    );
    assert(result == -1);
    result = spew3d_bignum_CompareStrInts(
        "12345678123456781234567812345678123456781234567812345678",
        "-12345678123456781234567812345678123456781234567812345679"
    );
    assert(result == 1);

    result = spew3d_bignum_CompareStrFloats(
        "12345678123456781234567812345678123456781234567812345678",
        "12345678123456781234567812345678123456781234567812345678.001"
    );
    assert(result == -1);
    result = spew3d_bignum_CompareStrFloats(
        "12345678123456781234567812345678123456781234567812345678.00100",
        "12345678123456781234567812345678123456781234567812345678.001"
    );
    assert(result == 0);
    result = spew3d_bignum_CompareStrFloats(
        "12345678123456781234567812345678123456781234567812345678.00100",
        "12345678123456781234567812345678123456781234567812345678.0012"
    );
    assert(result == -1);
    result = spew3d_bignum_CompareStrFloats(
        "12345678123456781234567812345678123456781234567812345678.00200",
        "12345678123456781234567812345678123456781234567812345678.001"
    );
    assert(result == 1);
    result = spew3d_bignum_CompareStrFloats(
        "12345678123456781234567812345678123456781234567812345678.00100",
        "12345678123456781234567812345678123456781234567812345678.001234"
    );
    assert(result == -1);
    result = spew3d_bignum_CompareStrFloats(
        "-12345678123456781234567812345678123456781234567812345678.00123",
        "12345678123456781234567812345678123456781234567812345678.001000"
    );
    assert(result == -1);

    result = spew3d_bignum_VerifyStrFloat(
        "-3434.344"
    );
    assert(result == 1);
    result = spew3d_bignum_VerifyStrFloat(
        "-34344"
    );
    assert(result == 1);
    result = spew3d_bignum_VerifyStrFloat(
        "-.34344"
    );
    assert(result == 0);
    result = spew3d_bignum_VerifyStrFloat(
        "-2.343.44"
    );
    assert(result == 0);
    result = spew3d_bignum_VerifyStrFloat(
        "-3."
    );
    assert(result == 0);
    result = spew3d_bignum_VerifyStrFloat(
        "-"
    );
    assert(result == 0);
    result = spew3d_bignum_VerifyStrFloat(
        "44 "
    );
    assert(result == 0);
}
END_TEST

TESTS_MAIN(test_bignum)
