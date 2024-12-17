/* Copyright (c) 2020-2022, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>


//#define _SPEW3D_GLOBDEBUG

S3DEXP int spew3d_wildcard_MatchBuf(
        const uint8_t *pattern, size_t patternlen,
        const uint8_t *value, size_t valuelen,
        int doublestar_for_paths, int backslash_paths,
        int *result) {
    #ifdef _SPEW3D_GLOBDEBUG
    char _patternstrbuf[64];
    char _valuestrbuf[64];
    {
        int i = 0;
        while (i < patternlen && i < 63) {
            _patternstrbuf[i] = (
                (pattern[i] != '\0' &&
                pattern[i] >= 32) ? pattern[i] : '?'
            );
            i += 1;
        }
        _patternstrbuf[i] = '\0';
        i = 0;
        while (i < valuelen && i < 63) {
            _valuestrbuf[i] = (
                (value[i] != '\0' &&
                value[i] >= 32) ? value[i] : '?'
            );
            i += 1;
        }
        _valuestrbuf[i] = '\0';
        printf("spew3d_wildcard.c: debug: "
            "spew3d_wildcard_MatchBuf(\"%s\", %d, "
            "\"%s\", %d, ...) called\n",
            _patternstrbuf, (int)patternlen,
            _valuestrbuf, (int)valuelen);
    }
    #endif
    // First, enforce glob limit:
    int globcount = 0;
    int is_escaped = 0;
    int i = 0;
    while (i < patternlen) {
        if (pattern[i] == '^' && !is_escaped) {
            is_escaped = 1;
            i++;
            continue;
        }
        if (!is_escaped && pattern[i] == '*') {
            globcount++;
            i++;
            while (i < patternlen &&
                    pattern[i] == '*')
                i++;
            continue;
        }
        is_escaped = 0;
        i++;
    }
    if (globcount > SPEW3D_MAX_ASTERISKS) {
        #ifdef _SPEW3D_GLOBDEBUG
        printf("spew3d_wildcard.c: debug: "
            "spew3d_wildcard_MatchBuf() mismatch, "
            "pattern exceeds asterisk limit\n");
        #endif
        return 0;
    }

    // Now do matching:
    is_escaped = 0;
    int i2 = 0;
    i = 0;
    while (i < patternlen && i2 < valuelen) {
        #ifdef _SPEW3D_GLOBDEBUG
        printf("spew3d_wildcard.c: debug: "
            "spew3d_wildcard_MatchBuf() comparing "
            "'%c' and '%c'\n",
            pattern[i], value[i2]);
        #endif
        if (pattern[i] == '^' && !is_escaped) {
            is_escaped = 1;
            i++;
            continue;
        }
        if (!is_escaped && pattern[i] == '?' &&
                (!doublestar_for_paths ||
                (value[i2] != '/' &&
                (!backslash_paths || value[i2] != '\\')))) {
            i++;
            i2++;
            continue;
        }
        if (!is_escaped && pattern[i] == '*') {
            int atdoublestar = 0;
            while (i + 1 < patternlen && pattern[i + 1] == '*') {
                atdoublestar = 1;
                i++;
            }
            const uint8_t *remainingpattern = NULL;
            size_t remainingpatternlen = 0;
            if (i + 1 >= patternlen) {  // Trailing last '*':
                if (!doublestar_for_paths) {  // Matches everything, done!
                    *result = 1;
                    return 1;
                }
                // Make sure remaining value has no path separator:
                while (i2 < valuelen) {
                    if (value[i2] == '/' ||
                            (backslash_paths && value[i2] == '\\')) {
                        #ifdef _SPEW3D_GLOBDEBUG
                        printf("spew3d_wildcard.c: debug: "
                            "spew3d_wildcard_MatchBuf() mismatch at "
                            "pattern position %d\n", (int)i);
                        #endif
                        *result = 0;
                        return 1;
                    }
                    i2++;
                }
                #ifdef _SPEW3D_GLOBDEBUG
                printf("spew3d_wildcard.c: debug: "
                    "spew3d_wildcard_MatchBuf() matched!\n");
                #endif
                *result = 1;
                return 1;
            }
            // We got still stuff left in our pattern, match it:
            remainingpattern = &pattern[i + 1];
            remainingpatternlen = patternlen - (i + 1);
            while (i2 < valuelen) {
                if (doublestar_for_paths && !atdoublestar &&
                        (value[i2] == '/' ||
                        (backslash_paths && value[i2] == '\\'))) {
                    // Encountered a path sep in our pattern.
                    // This is a hard boundary where we need to stop
                    // trying, unless our pattern also has a path sep
                    // right next:
                    if (remainingpatternlen > 0 && (
                            remainingpattern[0] == '/' ||
                            (backslash_paths &&
                            remainingpattern[0] == '\\'))) {
                        remainingpattern++;
                        remainingpatternlen--;
                        i2++;
                        continue;
                    }
                    if (remainingpatternlen > 1 &&
                            remainingpattern[0] == '^' && (
                            remainingpattern[1] == '/' ||
                            (backslash_paths &&
                            remainingpattern[1] == '\\'))) {
                        remainingpattern++;
                        remainingpatternlen--;
                        i2++;
                        continue;
                    }
                    #ifdef _SPEW3D_GLOBDEBUG
                    printf("spew3d_wildcard.c: debug: "
                        "spew3d_wildcard_MatchBuf() mismatch at "
                        "pattern position %d\n", (int)i);
                    #endif
                    *result = 0;
                    return 1;
                }
                // Try to match remaining substring:
                int innerresult = 0;
                int validpattern = spew3d_wildcard_MatchBuf(
                    remainingpattern, remainingpatternlen,
                    &value[i2], valuelen - i2,
                    doublestar_for_paths, backslash_paths,
                    &innerresult
                );
                assert(validpattern);
                if (innerresult) {
                    #ifdef _SPEW3D_GLOBDEBUG
                    printf("spew3d_wildcard.c: debug: "
                        "spew3d_wildcard_MatchBuf() matched via "
                        "recursion!\n");
                    #endif
                    *result = 1;
                    return 1;
                }
                i2++;
            }
            #ifdef _SPEW3D_GLOBDEBUG
            printf("spew3d_wildcard.c: debug: "
                "spew3d_wildcard_MatchBuf() mismatch at "
                "pattern position %d\n", (int)i);
            #endif
            *result = 0;
            return 1;
        }
        if (pattern[i] != value[i2] && (
                !backslash_paths ||
                (pattern[i] != '/' && pattern[i] != '\\') ||
                (value[i2] != '/' && value[i2] != '\\'))) {
            #ifdef _SPEW3D_GLOBDEBUG
            printf("spew3d_wildcard.c: debug: "
                "spew3d_wildcard_MatchBuf() mismatch at "
                "pattern position %d\n", (int)i);
            #endif
            *result = 0;
            return 1;
        }
        is_escaped = 0;
        i++;
        i2++;
    }
    #ifdef _SPEW3D_GLOBDEBUG
    printf("spew3d_wildcard.c: debug: "
        "spew3d_wildcard_MatchBuf() is past "
        "the loop, i=%d, i2=%d\n", (int)i, (int)i2);
    #endif
    if (i2 >= valuelen && (i >= patternlen ||
            (i == patternlen - 1 && !is_escaped &&
            pattern[i] == '*'))) {
        #ifdef _SPEW3D_GLOBDEBUG
        printf("spew3d_wildcard.c: debug: "
            "spew3d_wildcard_MatchBuf() matched!\n");
        #endif
        *result = 1;
        return 1;
    }
    #ifdef _SPEW3D_GLOBDEBUG
    printf("spew3d_wildcard.c: debug: "
        "spew3d_wildcard_MatchBuf() mismatch at "
        "pattern position %d\n", (int)i);
    #endif
    *result = 0;
    return 1;
}


S3DEXP int spew3d_wildcard_Match(
        const char *pattern, const char *value,
        int doublestar_for_paths, int backslash_paths,
        int *result
        ) {
    return spew3d_wildcard_MatchBuf(
        pattern, strlen(pattern),
        value, strlen(value),
        doublestar_for_paths, backslash_paths,
        result
    );
}

#endif  // SPEW3D_IMPLEMENTATION

