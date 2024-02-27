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

#ifdef SPEW3D_IMPLEMENTATION

#define MAX_PRESSED_KEYS 64

typedef struct _window_keypress_entry {
    uint32_t win_id;
    s3d_key_t pressed[MAX_PRESSED_KEYS];
    uint32_t pressed_count;
} _window_keypress_entry;

s3d_mutex *_window_keypress_mutex = NULL;
uint32_t _window_keypress_map_count = 0;
_window_keypress_entry *_window_keypress_map = NULL;
char **_s3d_key_to_name_map = NULL;

S3DHID int _spew3d_keyboard_globalnameregister(
        s3d_key_t key, const char *name
        ) {
    if (_s3d_key_to_name_map[key] != NULL)
        return 1;
    _s3d_key_to_name_map[key] = strdup(name);
    return (_s3d_key_to_name_map[key] != NULL);
}

S3DHID __attribute__((constructor))
        static void _spew3d_keyboard_GlobalInit() {
    if (_window_keypress_mutex == NULL) {
        _window_keypress_mutex = mutex_Create();
        if (!_window_keypress_mutex) {
            fprintf(stderr, "spew3d_keyboard.c: error: "
                "Failed to allocate global mutex.\n");
            _exit(1);
        }
    }
    if (!_s3d_key_to_name_map) {
        _s3d_key_to_name_map = malloc(
            sizeof(*_s3d_key_to_name_map) *
            (uint64_t)_INTERNAL_S3D_KEY_UPPER_BOUND
        );
        if (!_s3d_key_to_name_map) {
            fprintf(stderr, "spew3d_keyboard.c: error: "
                "Failed to allocate global name map.\n");
            _exit(1);
        }
        int result = (
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RETURN, "Return"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_BACKSPACE, "Backspace"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_A, "A"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_B, "B"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_C, "C"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_D, "D"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_E, "E"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F, "F"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_G, "G"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_H, "H"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_I, "I"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_J, "J"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_K, "K"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_L, "L"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_M, "M"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_N, "N"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_O, "O"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_P, "P"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_Q, "Q"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_R, "R"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_S, "S"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_T, "T"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_U, "U"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_V, "V"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_W, "W"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_X, "X"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_Y, "Y"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_Z, "Z"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_0, "0"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_1, "1"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_2, "2"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_3, "3"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_4, "4"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_5, "5"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_6, "6"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_7, "7"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_8, "8"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_9, "9"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFT, "Left"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_UP, "Up"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHT, "Right"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_DOWN, "Down"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_ESCAPE, "Escape"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_DELETE, "Delete"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F1, "F1"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F2, "F2"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F3, "F3"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F4, "F4"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F5, "F5"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F6, "F6"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F7, "F7"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F8, "F8"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F9, "F9"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F10, "F10"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F11, "F11"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_F12, "F12"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTCONTROL, "Left Control"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTCONTROL, "Right Control"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_SPACE, "Space"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTSHIFT, "Left Shift"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTSHIFT, "Right Shift"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTSHIFT, "Left Alt"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTSHIFT, "Right Alt"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_SUPER, "Super"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTPAREN, "Left Parenthesis"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTPAREN, "Right Parenthesis"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTBRACE, "Left Brace"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTBRACE, "Right Brace"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_LEFTBRACKET, "Left Square Bracket"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_RIGHTBRACKET, "Right Square Bracket"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_ASTERISK, "Asterisk"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_AT, "At"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_CARET, "Caret"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_COLON, "Colon"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_DOLLAR, "Dollar"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_EXCLAMATIONMARK, "Exclamation Mark"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_PERIOD, "Period"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_COMMA, "Comma"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_SLASH, "Slash"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_BACKSLASH, "Backslash"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_SEMICOLON, "Semicolon"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_TAB, "Tab"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_CAPSLOCK, "Caps Lock"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_EQUALS, "Equals"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_DASH, "Dash"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_SINGLEQUOTE, "Single Quote"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_DOUBLEQUOTE, "Double Quote"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_HOME, "Home"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_END, "End"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_PAUSE, "Pause"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_INSERT, "Insert"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_BACKQUOTE, "Backquote"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_PAGEDOWN, "Page Down"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_PAGEUP, "Page Up"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_PRINTSCREEN, "Print Screen"
            ) &&
            _spew3d_keyboard_globalnameregister(
                S3D_KEY_APPMENU, "App Menu"
            ) &&
            1
        );
        if (!result) {
            fprintf(stderr, "spew3d_keyboard.c: error: "
                "Failed to fill in global name map.\n");
            _exit(1);
        }
    }
}

S3DHID s3d_key_t _spew3d_keyboard_SDL_Key_To_S3D_Key(
            SDL_Keycode sym, SDL_Scancode scancode
            ) {
    if (sym == SDLK_RETURN) {
        return S3D_KEY_RETURN;
    } else if (sym == SDLK_BACKSPACE) {
        return S3D_KEY_BACKSPACE;
    } else if (sym == SDLK_SPACE) {
        return S3D_KEY_SPACE;
    } else if (sym >= SDLK_a &&
            sym <= SDLK_z) {
        return ((uint16_t)(sym - SDLK_a) +
            (uint16_t)S3D_KEY_A);
    } else if (sym >= SDLK_0 &&
            sym <= SDLK_9) {
        return ((uint16_t)(sym - SDLK_0) +
            (uint16_t)S3D_KEY_0);
    } else if (sym == SDLK_LEFT) {
        return S3D_KEY_LEFT;
    } else if (sym == SDLK_UP) {
        return S3D_KEY_UP;
    } else if (sym == SDLK_RIGHT) {
        return S3D_KEY_RIGHT;
    } else if (sym == SDLK_DOWN) {
        return S3D_KEY_DOWN;
    } else if (sym == SDLK_ESCAPE) {
        return S3D_KEY_ESCAPE;
    } else if (sym == SDLK_DELETE) {
        return S3D_KEY_DELETE;
    } else if (sym >= SDLK_F1 &&
            sym <= SDLK_F12) {
        return ((uint16_t)(sym - SDLK_F1) +
            (uint16_t)S3D_KEY_F1);
    } else if (sym == SDLK_LCTRL) {
        return S3D_KEY_LEFTCONTROL;
    } else if (sym == SDLK_RCTRL) {
        return S3D_KEY_RIGHTCONTROL;
    } else if (sym == SDLK_LSHIFT) {
        return S3D_KEY_LEFTSHIFT;
    } else if (sym == SDLK_RSHIFT) {
        return S3D_KEY_RIGHTSHIFT;
    } else if (sym == SDLK_LALT) {
        return S3D_KEY_LEFTALT;
    } else if (sym == SDLK_RALT) {
        return S3D_KEY_RIGHTALT;
    } else if (sym == SDLK_LGUI) {
        return S3D_KEY_SUPER;
    } else if (sym == SDLK_LEFTPAREN) {
        return S3D_KEY_LEFTPAREN;
    } else if (sym == SDLK_RIGHTPAREN) {
        return S3D_KEY_RIGHTPAREN;
    } else if (sym == SDLK_LEFTBRACKET) {
        return S3D_KEY_LEFTBRACKET;
    } else if (sym == SDLK_RIGHTBRACKET) {
        return S3D_KEY_RIGHTBRACKET;
    } else if (sym == SDLK_AMPERSAND) {
        return S3D_KEY_AMPERSAND;
    } else if (sym == SDLK_ASTERISK) {
        return S3D_KEY_ASTERISK;
    } else if (sym == SDLK_AT) {
        return S3D_KEY_AT;
    } else if (sym == SDLK_CARET) {
        return S3D_KEY_CARET;
    } else if (sym == SDLK_COLON) {
        return S3D_KEY_COLON;
    } else if (sym == SDLK_DOLLAR) {
        return S3D_KEY_DOLLAR;
    } else if (sym == SDLK_EXCLAIM) {
        return S3D_KEY_EXCLAMATIONMARK;
    } else if (sym == SDLK_PERIOD) {
        return S3D_KEY_PERIOD;
    } else if (sym == SDLK_COMMA) {
        return S3D_KEY_COMMA;
    } else if (sym == SDLK_SLASH) {
        return S3D_KEY_SLASH;
    } else if (sym == SDLK_BACKSLASH) {
        return S3D_KEY_BACKSLASH;
    } else if (sym == SDLK_SEMICOLON) {
        return S3D_KEY_SEMICOLON;
    } else if (sym == SDLK_TAB) {
        return S3D_KEY_TAB;
    } else if (sym == SDLK_CAPSLOCK) {
        return S3D_KEY_CAPSLOCK;
    } else if (sym == SDLK_EQUALS) {
        return S3D_KEY_EQUALS;
    } else if (sym == SDLK_MINUS) {
        return S3D_KEY_DASH;
    } else if (sym == SDLK_QUOTE) {
        return S3D_KEY_SINGLEQUOTE;
    } else if (sym == SDLK_HOME) {
        return S3D_KEY_HOME;
    } else if (sym == SDLK_END) {
        return S3D_KEY_END;
    } else if (sym == SDLK_PAUSE) {
        return S3D_KEY_PAUSE;
    } else if (sym == SDLK_INSERT) {
        return S3D_KEY_INSERT;
    } else if (sym == SDLK_BACKQUOTE) {
        return S3D_KEY_BACKQUOTE;
    } else if (sym == SDLK_PAGEDOWN) {
        return S3D_KEY_PAGEDOWN;
    } else if (sym == SDLK_PAGEUP) {
        return S3D_KEY_PAGEUP;
    } else if (sym == SDLK_PRINTSCREEN) {
        return S3D_KEY_PRINTSCREEN;
    } else if (sym == SDLK_APPLICATION) {
        return S3D_KEY_APPMENU;
    }
    return S3D_KEY_INVALID;
}

S3DEXP const char *spew3d_keyboard_GetKeyDescription(
        s3d_key_t key
        ) {
    if (key <= S3D_KEY_INVALID ||
            key >= _INTERNAL_S3D_KEY_UPPER_BOUND)
        return NULL;
    return _s3d_key_to_name_map[key];
}

S3DEXP s3d_key_t spew3d_keyboard_FindKeyByDescription(
        const char *desc
        ) {
    uint32_t i = 0;
    while (i < (uint32_t)_INTERNAL_S3D_KEY_UPPER_BOUND) {
        if (i == S3D_KEY_INVALID ||
                _s3d_key_to_name_map[i] == NULL) {
            i++;
            continue;
        }
        if (s3dstrcasecmp(_s3d_key_to_name_map[i], desc) == 0) {
            return (s3d_key_t)i;
        }
        i++;
        continue;
    }
    return S3D_KEY_INVALID;
}

S3DHID static _window_keypress_entry *
        _spew3d_keyboard_GetMap_nolock(
        uint32_t window_id
        ) {
    assert(window_id > 0);
    uint32_t i = 0;
    while (i < _window_keypress_map_count) {
        if (_window_keypress_map[i].win_id == window_id) {
            return &_window_keypress_map[i];
        }
        i += 1;
    }
    _window_keypress_entry *new_map = realloc(
        _window_keypress_map, sizeof(*new_map) *
        (_window_keypress_map_count + 1)
    );
    if (!new_map)
        return NULL;
    _window_keypress_entry *new_entry = (
        &new_map[_window_keypress_map_count]
    );
    _window_keypress_map_count += 1;
    memset(new_entry, 0, sizeof(*new_entry));
    new_entry->win_id = window_id;

    _window_keypress_map = new_map;
    return new_entry;
}

S3DEXP int spew3d_keyboard_IsKeyPressed(
        uint32_t window_id, s3d_key_t key
        ) {
    mutex_Lock(_window_keypress_mutex);
    _window_keypress_entry *e =
        _spew3d_keyboard_GetMap_nolock(window_id);
    if (e) {
        uint32_t i = 0;
        while (i < MAX_PRESSED_KEYS && i < e->pressed_count) {
            if (e->pressed[i] == key) {
                mutex_Release(_window_keypress_mutex);
                return 1;
            }
            i++;
        }
    }
    mutex_Release(_window_keypress_mutex);
    return 0;
}

S3DHID void _spew3d_keyboard_win_lose_keyboard(
        uint32_t window_id,
        void (*was_pressed_cb)(uint32_t window_id, s3d_key_t key,
            void *userdata),
        void *userdata
        ) {
    s3d_key_t was_pressed[MAX_PRESSED_KEYS];
    uint32_t was_pressed_count = 0;
    mutex_Lock(_window_keypress_mutex);
    _window_keypress_entry *e =
        _spew3d_keyboard_GetMap_nolock(window_id);
    if (e) {
        memcpy(&was_pressed, &e->pressed,
            sizeof(s3d_key_t) * MAX_PRESSED_KEYS);
        was_pressed_count = e->pressed_count;
        e->pressed_count = 0;
    }
    mutex_Release(_window_keypress_mutex);
    uint32_t i = 0;
    while (i < was_pressed_count) {
        if (was_pressed[i] != S3D_KEY_INVALID) {
            was_pressed_cb(window_id, was_pressed[i], userdata);
        }
        i++;
    }
}

S3DHID void _spew3d_keyboard_register_pressed_down(
        uint32_t window_id, s3d_key_t key
        ) {
    if (key == S3D_KEY_INVALID || window_id == 0)
        return;
    mutex_Lock(_window_keypress_mutex);
    _window_keypress_entry *e =
        _spew3d_keyboard_GetMap_nolock(window_id);
    if (e) {
        int32_t free_slot = -1;
        uint32_t i = 0;
        while (i < MAX_PRESSED_KEYS && i < e->pressed_count) {
            if (e->pressed[i] == key) {
                mutex_Release(_window_keypress_mutex);
                return;
            } else if (e->pressed[i] == S3D_KEY_INVALID) {
                free_slot = i;
            }
            i++;
        }
        if (free_slot >= 0) {
            e->pressed[free_slot] = key;
        } else if (e->pressed_count < MAX_PRESSED_KEYS) {
            e->pressed[e->pressed_count] = key;
            e->pressed_count++;
        }
    }
    mutex_Release(_window_keypress_mutex);
}

S3DHID void _spew3d_keyboard_register_release(
        uint32_t window_id, s3d_key_t key
        ) {
    if (key == S3D_KEY_INVALID)
        return;
    mutex_Lock(_window_keypress_mutex);
    _window_keypress_entry *e =
        _spew3d_keyboard_GetMap_nolock(window_id);
    if (e) {
        uint32_t i = 0;
        while (i < MAX_PRESSED_KEYS && i < e->pressed_count) {
            if (e->pressed[i] == key) {
                e->pressed[i] = S3D_KEY_INVALID;
                if (i == e->pressed_count - 1) {
                    e->pressed_count--;
                }
            }
            i++;
        }
    }
    mutex_Release(_window_keypress_mutex);
}

#endif  // SPEW3D_IMPLEMENTATION

