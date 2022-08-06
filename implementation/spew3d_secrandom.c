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

#ifdef SPEW3D_IMPLEMENTATION

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>
#include <wincrypt.h>
#else
#include <errno.h>
#if !defined(ANDROID) && !defined(__ANDROID__)
#include <sys/random.h>
#endif
#endif


// Global state for when direct file handle to /dev/urandom is used:
#if defined(ANDROID) || defined(__ANDROID__)
static FILE *urandom = NULL;
#endif


#if !defined(_WIN32) && !defined(_WIN64)
// Warning: this function uses /dev/urandom.
// This is probably good enough for most uses, but for generation of
// strong GPG key and such it is a bit undesirable.
int _spew3d_secrandom_GetBytesUpTo256(char *ptr, size_t amount) {
    if (amount == 0)
        return 1;

    int attempts = 0;
    while (attempts < 100) {
        attempts += 1;

        #if defined(ANDROID) || defined(__ANDROID__)
        // Android doesn't have the wrappers until in late versions,
        // so we have to read /dev/urandom manually:
        if (!urandom)
            urandom = fopen("/dev/urandom", "rb");
        size_t bytesread = 0;
        char *p = ptr;
        do {
            int result = (int)fread(
                p, 1,
                amount - bytesread, urandom
            );
            if (result <= 0)
                return 0;
            p += result;
            bytesread += result;
        } while (bytesread < amount);
        return 1;
        #else
        // We'll use getentropy if not on android:
        int result = getentropy(ptr, amount);
        if (result != 0) {
            if (errno == EIO && attempts < 1000) {
                usleep(20);  // wait a short amount of time, retry
                continue;
            }
            return 0;
        }
        return 1;
        #endif
    }
    return 0;
}
#else
static HCRYPTPROV hProvider = 0;
static void _spew3d_secrandom_initProvider() {
    if (hProvider)
        return;
    int result = CryptAcquireContext(
        &hProvider, NULL, NULL, PROV_RSA_FULL, 0
    );
    if (result == FALSE) {
        if (GetLastError() == (unsigned long int)NTE_BAD_KEYSET) {
            result = CryptAcquireContext(
                &hProvider, NULL, NULL, PROV_RSA_FULL, CRYPT_NEWKEYSET
            );
        }
    }
    if (result == FALSE) {
        LPSTR s = NULL;
        FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_ARGUMENT_ARRAY | FORMAT_MESSAGE_ALLOCATE_BUFFER,
            NULL, GetLastError(), 0, (LPSTR)&s, 0, NULL
        );
        char buf[1024];
        snprintf(
            buf, sizeof(buf) - 1,
            "Couldn't acquire HCRYPTPROV: %s", s
        );
        if (s)
            LocalFree(s);
        fprintf(
            stderr, "spew3d_secrandom.c: error: %s\n", buf
        );
        int msgboxID = MessageBox(
            NULL, buf, "spew3d_secrandom.c ERROR",
            MB_OK|MB_ICONSTOP
        );
        exit(1);
    }
}
__attribute__((constructor)) static void _ensureInit() {
    _spew3d_secrandom_initProvider();
}
#endif


int spew3d_secrandom_GetBytes(char *ptr, size_t amount) {
    #if !defined(_WIN32) && !defined(_WIN64)
    char *p = ptr;
    size_t fetched = 0;
    while (fetched < amount) {
        size_t next_bytes = amount - fetched;
        if (next_bytes > 256)
            next_bytes = 256;
        if (!_spew3d_secrandom_GetBytesUpTo256(p, next_bytes))
            return 0;
        fetched += next_bytes;
        p += next_bytes;
    }
    return 1;
    #else
    if (!hProvider)
        _spew3d_secrandom_initProvider();
    memset(ptr, 0, amount);
    return (CryptGenRandom(hProvider, amount,
        (uint8_t *)ptr) != FALSE);
    #endif
}


#endif  // SPEW3D_IMPLEMENTATION

