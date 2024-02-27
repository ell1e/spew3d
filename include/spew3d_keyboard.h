/* Copyright (c) 2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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

#ifndef SPEW3D_KEYBOARD_H_
#define SPEW3D_KEYBOARD_H_

typedef uint16_t s3d_key_t;

enum S3D_KEY {
    S3D_KEY_INVALID = 0,
    S3D_KEY_RETURN,
    S3D_KEY_BACKSPACE,
    S3D_KEY_SPACE,
    S3D_KEY_A,
    S3D_KEY_B,
    S3D_KEY_C,
    S3D_KEY_D,
    S3D_KEY_E,
    S3D_KEY_F,
    S3D_KEY_G,
    S3D_KEY_H,
    S3D_KEY_I,
    S3D_KEY_J,
    S3D_KEY_K,
    S3D_KEY_L,
    S3D_KEY_M,
    S3D_KEY_N,
    S3D_KEY_O,
    S3D_KEY_P,
    S3D_KEY_Q,
    S3D_KEY_R,
    S3D_KEY_S,
    S3D_KEY_T,
    S3D_KEY_U,
    S3D_KEY_V,
    S3D_KEY_W,
    S3D_KEY_X,
    S3D_KEY_Y,
    S3D_KEY_Z,
    S3D_KEY_0,
    S3D_KEY_1,
    S3D_KEY_2,
    S3D_KEY_3,
    S3D_KEY_4,
    S3D_KEY_5,
    S3D_KEY_6,
    S3D_KEY_7,
    S3D_KEY_8,
    S3D_KEY_9,
    S3D_KEY_LEFT,
    S3D_KEY_UP,
    S3D_KEY_RIGHT,
    S3D_KEY_DOWN,
    S3D_KEY_ESCAPE,
    S3D_KEY_DELETE,
    S3D_KEY_F1,
    S3D_KEY_F2,
    S3D_KEY_F3,
    S3D_KEY_F4,
    S3D_KEY_F5,
    S3D_KEY_F6,
    S3D_KEY_F7,
    S3D_KEY_F8,
    S3D_KEY_F9,
    S3D_KEY_F10,
    S3D_KEY_F11,
    S3D_KEY_F12,
    S3D_KEY_LEFTCONTROL,
    S3D_KEY_RIGHTCONTROL,
    S3D_KEY_LEFTSHIFT,
    S3D_KEY_RIGHTSHIFT,
    S3D_KEY_LEFTALT,
    S3D_KEY_RIGHTALT,
    S3D_KEY_SUPER,
    S3D_KEY_LEFTPAREN,
    S3D_KEY_RIGHTPAREN,
    S3D_KEY_LEFTBRACE,
    S3D_KEY_RIGHTBRACE,
    S3D_KEY_LEFTBRACKET,
    S3D_KEY_RIGHTBRACKET,
    S3D_KEY_AMPERSAND,
    S3D_KEY_ASTERISK,
    S3D_KEY_AT,
    S3D_KEY_CARET,
    S3D_KEY_COLON,
    S3D_KEY_DOLLAR,
    S3D_KEY_EXCLAMATIONMARK,
    S3D_KEY_PERIOD,
    S3D_KEY_COMMA,
    S3D_KEY_SLASH,
    S3D_KEY_BACKSLASH,
    S3D_KEY_SEMICOLON,
    S3D_KEY_TAB,
    S3D_KEY_CAPSLOCK,
    S3D_KEY_EQUALS,
    S3D_KEY_DASH,
    S3D_KEY_SINGLEQUOTE,
    S3D_KEY_DOUBLEQUOTE,
    S3D_KEY_HOME,
    S3D_KEY_END,
    S3D_KEY_PAUSE,
    S3D_KEY_INSERT,
    S3D_KEY_BACKQUOTE,
    S3D_KEY_PAGEDOWN,
    S3D_KEY_PAGEUP,
    S3D_KEY_PRINTSCREEN,
    S3D_KEY_APPMENU,
    _INTERNAL_S3D_KEY_UPPER_BOUND  /* This must remain the last entry! */
};

S3DEXP int spew3d_keyboard_IsKeyPressed(
    uint32_t window_id, s3d_key_t key
);

S3DEXP const char *spew3d_keyboard_GetKeyDescription(
    s3d_key_t key
);

S3DEXP s3d_key_t spew3d_keyboard_FindKeyByDescription(
    const char *desc
);

#endif  // SPEW3D_KEYBOARD_H_

