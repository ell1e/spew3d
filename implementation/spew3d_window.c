/* Copyright (c) 2020-2024, ellie/@ell1e & Spew3D Team (see AUTHORS.md).

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
#include <string.h>
#ifndef SPEW3D_OPTION_DISABLE_SDL
#include <SDL2/SDL.h>
#endif
#include <unistd.h>

uint32_t _last_window_id = 0;
s3d_mutex *_win_id_mutex = NULL;
s3d_window **_global_win_registry = NULL;
uint32_t _last_keyboard_focus_window_id = 0;
uint32_t _last_mouse_hover_window_id = 0;
int _global_win_registry_fill = 0;
int _global_win_registry_alloc = 0;

typedef struct s3d_window {
    uint32_t id;
    s3d_backend_windowing *backend;
    s3d_backend_windowing_wininfo *backend_winfo;
    uint32_t flags;
    char *title;
    int wasclosed;
    int32_t width, height;
    int mouse_lock_mode;
    int focused;

    int32_t canvaswidth, canvasheight;
    s3dnum_t dpiscale;
    struct virtualwin {
        s3d_texture_t canvas;
    } virtualwin;
    int mouseseeninwindow, fingerseeninwindow;
    uint64_t lastlegitmouseseen_ts;
    s3dnum_t lastmousex, lastmousey,
        lastfingerx, lastfingery, mouse_warp_target_x,
        mouse_warp_target_y;
    uint64_t ignore_mouse_motion_until_ts;
} s3d_window;

S3DHID s3d_key_t _spew3d_keyboard_SDL_Key_To_S3D_Key(
    SDL_Keycode sym, SDL_Scancode scancode
);
S3DHID void _spew3d_keyboard_win_lose_keyboard(
    uint32_t window_id,
    void (*was_pressed_cb)(uint32_t window_id, s3d_key_t key,
        void *userdata),
    void *userdata
);
S3DHID void _spew3d_keyboard_register_pressed_down(
    uint32_t window_id, s3d_key_t key
);
S3DHID void _spew3d_keyboard_register_release(
    uint32_t window_id, s3d_key_t key
);
S3DHID void _s3d_sdl_GetWindowSDLRef(
    s3d_window *win,
    s3d_backend_windowing_wininfo *backend_winfo,
    SDL_Window **out_win,
    SDL_Renderer **out_renderer
);

S3DHID __attribute__((constructor)) void _ensure_winid_mutex() {
    if (_win_id_mutex != NULL)
        return;
    _win_id_mutex = mutex_Create();
    if (!_win_id_mutex) {
        fprintf(stderr, "spew3d_window.c: error: "
            "Failed to allocate global mutex.\n");
        _exit(1);
    }
}

S3DHID void _spew3d_window_ExtractCanvasSize_nolock(
        s3d_window *win, uint32_t *out_w, uint32_t *out_h) {
    *out_w = win->canvaswidth;
    *out_h = win->canvasheight;
}

S3DHID uint32_t _spew3d_window_MakeNewID_nolock() {
    _last_window_id += 1;
    uint32_t result = _last_window_id;
    return result;
}

S3DHID uint32_t spew3d_window_MakeNewID() {
    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    mutex_Lock(_win_id_mutex);
    uint32_t result = _spew3d_window_MakeNewID_nolock();
    mutex_Release(_win_id_mutex);
    return result;
}

S3DHID void _spew3d_window_FreeContents(s3d_window *win) {
    if (win == NULL)
        return;

    if (win->virtualwin.canvas != 0) {
        // FIXME. Clean up here.
    }
    free(win->title);
}

#if !defined(SPEW3D_OPTION_DISABLE_SDL) &&\
        defined(SPEW3D_OPTION_DISABLE_SDL_HEADER)
// This won't be in the header, so define it here:
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
    s3d_window *win, SDL_Window **out_w,
    SDL_Renderer **out_r
);
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DEXP void spew3d_window_GetSDLWindowAndRenderer_nolock(
        s3d_window *win, SDL_Window **out_w,
        SDL_Renderer **out_r
        ) {
    assert(win->backend->kind == S3D_BACKEND_WINDOWING_SDL);
    _s3d_sdl_GetWindowSDLRef(
        win, win->backend_winfo,
        out_w, out_r
    );
}
#endif

S3DHID static s3d_window *spew3d_window_NewExEx(
        const char *title, uint32_t flags,
        int dontinitactualwindow, int32_t width, int32_t height
        ) {
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);
    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);

    mutex_Lock(_win_id_mutex);
    if (_global_win_registry_fill + 1 >
            _global_win_registry_alloc) {
        int newalloc = _global_win_registry_alloc * 2 + 256;
        s3d_window **new_registry = realloc(
            _global_win_registry,
            sizeof(*_global_win_registry) * newalloc
        );
        if (!new_registry) {
            mutex_Release(_win_id_mutex);
            return NULL;
        }
        _global_win_registry = new_registry;
        _global_win_registry_alloc = newalloc;
    }

    s3d_window *win = malloc(sizeof(*win));
    if (!win) {
        mutex_Release(_win_id_mutex);
        return NULL;
    }
    memset(win, 0, sizeof(*win));
    win->backend = spew3d_backend_windowing_GetDefault();
    if (!win->backend) {
        free(win);
        mutex_Release(_win_id_mutex);
        return NULL;
    }
    win->backend_winfo = win->backend->CreateWinInfo(
        win->backend, win
    );
    if (!win->backend_winfo) {
        free(win);
        mutex_Release(_win_id_mutex);
        return NULL;
    }

    win->id = _spew3d_window_MakeNewID_nolock();
    if (width <= 0) width = 800;
    if (height <= 0) height = 500;
    win->width = width;
    win->height = height;
    win->flags = flags;
    win->title = strdup(title);
    if (!win->title) {
        win->backend->DestroyWinInfo(
            win->backend, win, win->backend_winfo
        );
        free(win);
        mutex_Release(_win_id_mutex);
        return NULL;
    }

    if (!dontinitactualwindow) {
        s3d_event e = {0};
        e.kind = S3DEV_INTERNAL_CMD_WIN_OPEN;
        e.window.win_id = win->id;
        if (!spew3d_event_q_Insert(eq, &e)) {
            win->backend->DestroyWinInfo(
                win->backend, win, win->backend_winfo
            );
            free(win->title);
            free(win);
            mutex_Release(_win_id_mutex);
            return NULL;
        }
    }

    if ((flags & SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL) != 0) {
        win->canvaswidth = win->width;
        win->canvasheight = win->height;
        win->dpiscale = 1.0f;
    }

    _global_win_registry[_global_win_registry_fill] = win;
    _global_win_registry_fill++;
    mutex_Release(_win_id_mutex);

    return win;
}

S3DEXP uint32_t spew3d_window_GetID(s3d_window *w) {
    return w->id;
}

S3DHID s3d_window *_spew3d_window_GetByIDLocked(uint32_t id) {
    int i = 0;
    while (i < _global_win_registry_fill) {
        if (_global_win_registry[i]->id == id) {
            return _global_win_registry[i];
        }
        i++;
    }
    return NULL;
}

S3DEXP s3d_window *spew3d_window_GetByID(uint32_t id) {
    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);

    mutex_Lock(_win_id_mutex);
    int i = 0;
    while (i < _global_win_registry_fill) {
        if (_global_win_registry[i]->id == id) {
            mutex_Release(_win_id_mutex);
            return _global_win_registry[i];
        }
        i++;
    }
    mutex_Release(_win_id_mutex);
    return NULL;
}

S3DHID void thread_MarkAsMainThread(void);  // Used below.

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID s3d_window *spew3d_window_GetBySDLWindowID(uint32_t sdlid) {
    mutex_Lock(_win_id_mutex);
    int i = 0;
    while (i < _global_win_registry_fill) {
        SDL_Window *sdlwin = NULL;
        spew3d_window_GetSDLWindowAndRenderer_nolock(
            _global_win_registry[i], &sdlwin, NULL
        );
        if (sdlwin != NULL &&
                sdlid == SDL_GetWindowID(sdlwin)) {
            mutex_Release(_win_id_mutex);
            return _global_win_registry[i];
        }
        i++;
    }
    mutex_Release(_win_id_mutex);
    return NULL;
}
#endif

S3DHID void _spew3d_window_HandleUnpressedKeyOnUnfocus(
        uint32_t window_id, s3d_key_t key,
        void *unused
        ) {
    mutex_Lock(_win_id_mutex);
    s3d_equeue *equser = spew3d_event_GetMainQueue();
    assert(equser != NULL);
    s3d_event e2 = {0};
    e2.kind = S3DEV_KEY_UP;
    e2.key.win_id = window_id;
    e2.key.key = key;
    mutex_Release(_win_id_mutex);
    spew3d_event_q_Insert(equser, &e2);
}

static int _graphics_backend_hidden_mouse_lock_mode = 0;
void spew3d_window_Update_nolock(s3d_window *win) {
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->id == _last_mouse_hover_window_id) {
        if (win->mouse_lock_mode != S3D_MOUSE_LOCK_DISABLED &&
                win->backend->WasWinObjCreated(
                    win->backend, win, win->backend_winfo
                ) &&
                win->mouseseeninwindow &&
                win->focused
                ) {
            if (_graphics_backend_hidden_mouse_lock_mode !=
                    win->mouse_lock_mode) {

                win->backend->ResetMouseGrab(
                    win->backend, win, win->backend_winfo
                );

                _graphics_backend_hidden_mouse_lock_mode = (
                    win->mouse_lock_mode
                );

                if (win->mouse_lock_mode ==
                        S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE) {
                    win->backend->SetMouseGrabInvisibleRelative(
                        win->backend, win, win->backend_winfo
                    );
                    win->mouse_warp_target_x = (
                        floor((s3dnum_t)win->width / (s3dnum_t)2.0)
                    );
                    win->mouse_warp_target_y = (
                        floor((s3dnum_t)win->height / (s3dnum_t)2.0)
                    );
                    win->backend->WarpMouse(
                        win->backend, win, win->backend_winfo,
                        win->mouse_warp_target_x,
                        win->mouse_warp_target_y
                    );
                    win->ignore_mouse_motion_until_ts = (
                        spew3d_time_Ticks() + 500
                    );
                } else {
                    win->backend->SetMouseGrabConstrained(
                        win->backend, win, win->backend_winfo
                    );
                }
            }
        } else if (win->mouse_lock_mode == S3D_MOUSE_LOCK_DISABLED ||
                !win->focused || win->wasclosed ||
                !win->backend->WasWinObjCreated(
                    win->backend, win, win->backend_winfo
                )) {
            if (_graphics_backend_hidden_mouse_lock_mode !=
                    S3D_MOUSE_LOCK_DISABLED) {
                _graphics_backend_hidden_mouse_lock_mode = (
                    S3D_MOUSE_LOCK_DISABLED
                );
                win->backend->ResetMouseGrab(
                    win->backend, win, win->backend_winfo
                );
            }
        }
    }
    #endif
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
static uint64_t _last_focus_update_ts = 0;
S3DHID void _spew3d_window_UpdateSDLFocus() {
    uint32_t previously_focused = 0;
    mutex_Lock(_win_id_mutex);
    uint64_t now = spew3d_time_Ticks();
    if (_last_focus_update_ts > 0 &&
            _last_focus_update_ts + 1000 > now) {
        mutex_Release(_win_id_mutex);
        return;
    }

    int i = 0;
    while (i < _global_win_registry_fill) {
        SDL_Window *sdlwin = NULL;
        spew3d_window_GetSDLWindowAndRenderer_nolock(
            _global_win_registry[i], &sdlwin, NULL
        );
        if (!sdlwin) {
            i++;
            continue;
        }
        uint32_t flags = SDL_GetWindowFlags(sdlwin);
        if ((flags & SDL_WINDOW_INPUT_FOCUS) != 0 ||
                _last_keyboard_focus_window_id == 0) {
            previously_focused = _last_keyboard_focus_window_id;
            _last_keyboard_focus_window_id = (
                _global_win_registry[i]->id
            );
            assert(_last_keyboard_focus_window_id > 0);
            break;
        }
        i++;
    }
    if (previously_focused != 0 &&
            previously_focused != _last_keyboard_focus_window_id
            ) {
        mutex_Release(_win_id_mutex);
        _spew3d_keyboard_win_lose_keyboard(
            previously_focused,
            _spew3d_window_HandleUnpressedKeyOnUnfocus,
            NULL
        );
    } else {
        mutex_Release(_win_id_mutex);
    }
}
#endif

S3DHID static void _force_unkeyboardfocus_win(
        s3d_window *win
        ) {
    mutex_Lock(_win_id_mutex);
    uint32_t previously_focused = 0;
    if (_last_keyboard_focus_window_id == win->id) {
        previously_focused = _last_keyboard_focus_window_id;
        _last_keyboard_focus_window_id = 0;
    }
    mutex_Release(_win_id_mutex);
    if (previously_focused != 0) {
        _spew3d_keyboard_win_lose_keyboard(
            previously_focused,
            _spew3d_window_HandleUnpressedKeyOnUnfocus,
            NULL
        );
    }
}

S3DHID void spew3d_window_MarkAsFocused(
        s3d_window *win
        ) {
    s3d_window *unfocused_list[32];
    uint32_t unfocused_list_fill = 0;
    mutex_Lock(_win_id_mutex);
    uint32_t i = 0;
    while (i < _global_win_registry_fill) {
        if (win != NULL && _global_win_registry[i]->id == win->id) {
            win->focused = 1;
        } else {
            if (_global_win_registry[i]->focused ||
                    _last_keyboard_focus_window_id ==
                    _global_win_registry[i]->id) {
                if (unfocused_list_fill < 32) {
                    unfocused_list[unfocused_list_fill] =
                        _global_win_registry[i];
                    unfocused_list_fill++;
                }
            }
            _global_win_registry[i]->focused = 0;
        }
        i++;
    }
    mutex_Release(_win_id_mutex);
    i = 0;
    while (i < unfocused_list_fill) {
        _force_unkeyboardfocus_win(unfocused_list[i]);
        unfocused_list[i]->focused = 0;
        i++;
    }
}

void spew3d_window_ProcessMouseMotion(
        s3d_window *win, s3d_equeue *equser, int mx, int my
        ) {
    mutex_Lock(_win_id_mutex);
    uint64_t now = spew3d_time_Ticks();

    int is_in_window = (
        mx >= 0 && mx < win->width &&
        my >= 0 && my < win->height
    );
    if (is_in_window) {
        _last_mouse_hover_window_id = win->id;
    }
    if (win->ignore_mouse_motion_until_ts > now) {
        if (!is_in_window) {
            win->mouseseeninwindow = 0;
        } else {
            if (win->mouse_lock_mode !=
                    S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE ||
                    !win->focused) {
                win->mouseseeninwindow = 1;
                win->lastmousex = (int)mx;
                win->lastmousey = (int)my;
            } else {
                assert(win->mouse_lock_mode ==
                    S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE);
                int wx = win->mouse_warp_target_x;
                int wy = win->mouse_warp_target_y;
                if (wx != mx || wy != my) {
                    win->backend->WarpMouse(
                        win->backend, win, win->backend_winfo,
                        wx, wy
                    );
                }
                win->mouseseeninwindow = 1;
                win->lastmousex = wx;
                win->lastmousey = wy;
            }
        }
        mutex_Release(_win_id_mutex);
        return;
    }

    int ignore_relative = (
        win->focused &&
        (win->mouse_lock_mode !=
            _graphics_backend_hidden_mouse_lock_mode)
    );
    int can_warp = (
        win->focused &&
        win->mouse_lock_mode ==
            _graphics_backend_hidden_mouse_lock_mode
    );
    s3dnum_t x = mx;
    s3dnum_t y = my;
    s3d_event e2 = {0};
    e2.kind = S3DEV_MOUSE_MOVE;
    e2.mouse.win_id = _last_keyboard_focus_window_id;
    e2.mouse.x = x;
    e2.mouse.y = y;
    int bogus_event = 0;
    if (win->mouseseeninwindow &&
            win->lastlegitmouseseen_ts + 200 < now &&
            (fabs(win->lastmousex - x) > 10 ||
            fabs(win->lastmousey - y) > 10)) {
        // XXX / Note: workaround for bugs like this one:
        // https://github.com/libsdl-org/SDL/issues/9156
        // (Basically, this doesn't look like a legit movement.)
        bogus_event = 1;
        #if defined(DEBUG_SPEW3D_EVENT) || \
                defined(DEBUG_SPEW3D_WARNING_BOGUS_EVENT)
        printf("spew3d_event.c: warning: "
            "Ignored likely faulty mouse event at "
            "mx/my %d, %d, delivered from windowing backend "
            "kind=%d.\n",
            (int)mx, (int)my,
            (int)win->backend->kind);
        #endif
    }
    if (win->mouseseeninwindow && is_in_window && !ignore_relative &&
            !bogus_event) {
        e2.mouse.rel_x = (
            x - win->lastmousex
        );
        e2.mouse.rel_y = (
            y - win->lastmousey
        );
    }
    win->mouseseeninwindow = is_in_window;
    if (is_in_window) {
        if (can_warp && win->mouse_lock_mode ==
                S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE) {
            int wx = win->mouse_warp_target_x;
            int wy = win->mouse_warp_target_y;
            if (mx != wx || my != wy) {
                win->backend->WarpMouse(
                    win->backend, win, win->backend_winfo,
                    wx, wy
                );
            }
            e2.mouse.x = 0;
            e2.mouse.y = 0;
            win->lastmousex = wx;
            win->lastmousey = wy;
            if (!bogus_event) {
                win->lastlegitmouseseen_ts = now;
            }
        } else {
            win->lastmousex = x;
            win->lastmousey = y;
            if (!bogus_event) {
                win->lastlegitmouseseen_ts = now;
            }
        }
    }
    mutex_Release(_win_id_mutex);
    spew3d_event_q_Insert(equser, &e2);
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID int _spew3d_window_HandleSDLEvent(SDL_Event *e) {
    thread_MarkAsMainThread();

    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);
    s3d_equeue *equser = spew3d_event_GetMainQueue();
    assert(equser != NULL);
    _spew3d_window_UpdateSDLFocus();

    if (e->type == SDL_QUIT) {
        s3d_event e2 = {0};
        e2.kind = S3DEV_APP_QUIT_REQUEST;
        _spew3d_event_q_InsertForce(equser, &e2);
        mutex_Lock(_win_id_mutex);
        int i = 0;
        while (i < _global_win_registry_fill) {
            if (_global_win_registry[i]->wasclosed) {
                i++;
                continue;
            }
            memset(&e2, 0, sizeof(e2));
            e2.kind = S3DEV_WINDOW_USER_CLOSE_REQUEST;
            e2.window.win_id = _global_win_registry[i]->id;
            _spew3d_event_q_InsertForce(equser, &e2);
            i++;
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->type == SDL_KEYDOWN) {
        mutex_Lock(_win_id_mutex);
        if (_last_keyboard_focus_window_id <= 0) {
            mutex_Release(_win_id_mutex);
            return 1;
        }
        s3d_key_t k = _spew3d_keyboard_SDL_Key_To_S3D_Key(
            e->key.keysym.sym, e->key.keysym.scancode
        );
        if (k == S3D_KEY_INVALID) {
            mutex_Release(_win_id_mutex);
            return 1;
        }
        if (!spew3d_keyboard_IsKeyPressed(
                _last_keyboard_focus_window_id, k
                )) {
            _spew3d_keyboard_register_pressed_down(
                _last_keyboard_focus_window_id, k
            );
            assert(spew3d_keyboard_IsKeyPressed(
                _last_keyboard_focus_window_id, k
            ));
            s3d_event e2 = {0};
            e2.kind = S3DEV_KEY_DOWN;
            e2.key.win_id = _last_keyboard_focus_window_id;
            e2.key.key = k;
            mutex_Release(_win_id_mutex);
            spew3d_event_q_Insert(equser, &e2);
        } else {
            mutex_Release(_win_id_mutex);
        }
        return 1;
    } else if (e->type == SDL_KEYUP) {
        mutex_Lock(_win_id_mutex);
        if (_last_keyboard_focus_window_id <= 0) {
            mutex_Release(_win_id_mutex);
            return 1;
        }
        s3d_key_t k = _spew3d_keyboard_SDL_Key_To_S3D_Key(
            e->key.keysym.sym, e->key.keysym.scancode
        );
        if (k == S3D_KEY_INVALID) {
            mutex_Release(_win_id_mutex);
            return 1;
        }
        _spew3d_keyboard_register_release(
            _last_keyboard_focus_window_id, k
        );
        s3d_event e2 = {0};
        e2.kind = S3DEV_KEY_UP;
        e2.key.win_id = _last_keyboard_focus_window_id;
        e2.key.key = k;
        mutex_Release(_win_id_mutex);
        spew3d_event_q_Insert(equser, &e2);
        return 1;
    } else if ((e->type == SDL_MOUSEBUTTONDOWN ||
            e->type == SDL_MOUSEBUTTONUP) &&
            (e->button.button == SDL_BUTTON_LEFT ||
            e->button.button == SDL_BUTTON_RIGHT ||
            e->button.button == SDL_BUTTON_MIDDLE)) {
        s3d_window *win = spew3d_window_GetBySDLWindowID(
            e->button.windowID
        );
        mutex_Lock(_win_id_mutex);
        if (win != NULL && e->motion.which != SDL_TOUCH_MOUSEID) {
            s3dnum_t x = (s3dnum_t)win->width * e->button.x;
            s3dnum_t y = (s3dnum_t)win->height * e->button.y;
            s3d_event e2 = {0};
            e2.kind = (e->type == SDL_MOUSEBUTTONDOWN ?
                S3DEV_MOUSE_BUTTON_DOWN : S3DEV_MOUSE_BUTTON_UP);
            e2.mouse.win_id = _last_keyboard_focus_window_id;
            e2.mouse.x = x;
            e2.mouse.y = y;
            e2.mouse.button = S3DEV_MOUSE_BUTTON_PRIMARY;
            if (e->button.button == SDL_BUTTON_RIGHT)
                e2.mouse.button = S3DEV_MOUSE_BUTTON_SECONDARY;
            else if (e->button.button == SDL_BUTTON_MIDDLE)
                e2.mouse.button = S3DEV_MOUSE_BUTTON_MIDDLE;
            mutex_Release(_win_id_mutex);
            spew3d_event_q_Insert(equser, &e2);
        } else {
            mutex_Release(_win_id_mutex);
        }
        return 1;
    } else if (e->type == SDL_MOUSEMOTION) {
        s3d_window *win = spew3d_window_GetBySDLWindowID(
            e->motion.windowID
        );
        mutex_Lock(_win_id_mutex);
        if (win != NULL && e->motion.which != SDL_TOUCH_MOUSEID) {
            mutex_Release(_win_id_mutex);
            spew3d_window_ProcessMouseMotion(
                win, equser, (int)e->motion.x, (int)e->motion.y
            );
        } else {
            mutex_Release(_win_id_mutex);
        }
        return 1;
    } else if (e->type == SDL_FINGERMOTION ||
            e->type == SDL_FINGERDOWN ||
            e->type == SDL_FINGERUP) {
        // Multi touch handling:
        // FIXME: implement this here.

        // Fake mouse cursor handling:
        s3d_window *win = spew3d_window_GetBySDLWindowID(
            e->tfinger.windowID
        );
        mutex_Lock(_win_id_mutex);
        if (win != NULL) {
            SDL_Finger *finger = SDL_GetTouchFinger(
                e->tfinger.touchId, 0
            );
            if (finger->id != e->tfinger.fingerId) {
                // This isn't the first finger. Ignore it.
                mutex_Release(_win_id_mutex);
                return 1;
            }
            s3dnum_t x = (s3dnum_t)win->width * e->tfinger.x;
            s3dnum_t y = (s3dnum_t)win->height * e->tfinger.y;
            s3d_event e2 = {0};
            e2.kind = S3DEV_MOUSE_MOVE;
            e2.mouse.win_id = _last_keyboard_focus_window_id;
            e2.mouse.x = x;
            e2.mouse.y = y;
            if (win->fingerseeninwindow) {
                e2.mouse.rel_x = (
                    x - win->lastfingerx
                );
                e2.mouse.rel_y = (
                    y - win->lastfingery
                );
            }
            if (e->type != SDL_FINGERUP) {
                win->fingerseeninwindow = 1;
                win->lastfingerx = x;
                win->lastfingery = y;
            } else {
                win->fingerseeninwindow = 0;
                win->lastfingerx = 0;
                win->lastfingery = 0;
            }
            spew3d_event_q_Insert(equser, &e2);
            if (e->type == SDL_FINGERDOWN) {
                s3d_event e2 = {0};
                e2.kind = S3DEV_MOUSE_BUTTON_DOWN;
                e2.mouse.win_id = _last_keyboard_focus_window_id;
                e2.mouse.x = x;
                e2.mouse.y = y;
                e2.mouse.button = S3DEV_MOUSE_BUTTON_PRIMARY;
                spew3d_event_q_Insert(equser, &e2);
            } else if (e->type == SDL_FINGERUP) {
                s3d_event e2 = {0};
                e2.kind = S3DEV_MOUSE_BUTTON_UP;
                e2.mouse.win_id = _last_keyboard_focus_window_id;
                e2.mouse.x = x;
                e2.mouse.y = y;
                e2.mouse.button = S3DEV_MOUSE_BUTTON_PRIMARY;
                spew3d_event_q_Insert(equser, &e2);
            }
            mutex_Release(_win_id_mutex);
        } else {
            mutex_Release(_win_id_mutex);
        }
        return 1;
    } else if (e->type == SDL_WINDOWEVENT) {
        s3d_window *win = spew3d_window_GetBySDLWindowID(
            e->window.windowID
        );
        if (win != NULL) {
            if (e->window.event == SDL_WINDOWEVENT_CLOSE) {
                s3d_event e2 = {0};
                e2.kind = S3DEV_WINDOW_USER_CLOSE_REQUEST;
                e2.window.win_id = win->id;
                _spew3d_event_q_InsertForce(equser, &e2);
            } else if (e->window.event == SDL_WINDOWEVENT_FOCUS_GAINED) {
                spew3d_window_MarkAsFocused(win);
            } else if (e->window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
                spew3d_window_MarkAsFocused(NULL);
            } else if (e->window.event == SDL_WINDOWEVENT_LEAVE) {
                win->lastmousex = 0;
                win->lastmousey = 0;
                win->mouseseeninwindow = 0;
            } else if (e->window.event == SDL_WINDOWEVENT_HIDDEN ||
                    e->window.event == SDL_WINDOWEVENT_MINIMIZED) {
                win->lastmousex = 0;
                win->lastmousey = 0;
                win->lastfingerx = 0;
                win->lastfingery = 0;
                win->fingerseeninwindow = 0;
                win->mouseseeninwindow = 0;
            } else if (e->window.event == SDL_WINDOWEVENT_RESIZED) {
                mutex_Lock(_win_id_mutex);
                win->width = e->window.data1;
                win->height = e->window.data2;
                spew3d_window_UpdateGeometryInfo(win);
                mutex_Release(_win_id_mutex);
                s3d_event e3 = {0};
                e3.kind = S3DEV_WINDOW_RESIZED;
                e3.window.win_id = win->id;
                spew3d_event_q_Insert(equser, &e3);
            }
        }
        return 1;
    }
    return 0;
}
#endif

S3DHID int _spew3d_window_ProcessWinOpenReq(s3d_event *ev);

S3DHID int _spew3d_window_ProcessWinDrawFillReq(s3d_event *ev);

S3DHID int _spew3d_window_ProcessWinUpdateCanvasReq(s3d_event *ev);

S3DHID int _spew3d_window_ProcessWinCloseReq(s3d_event *ev);

S3DHID int _spew3d_window_ProcessWinDestroyReq(s3d_event *ev);

S3DEXP void spew3d_window_InternalMainThreadUpdate() {
    thread_MarkAsMainThread();

    _ensure_winid_mutex();
    mutex_Lock(_win_id_mutex);
    uint32_t i = 0;
    while (i < _global_win_registry_fill) {
        spew3d_window_Update_nolock(
            _global_win_registry[i]
        );
        i++;
    }
    mutex_Release(_win_id_mutex);
}

S3DEXP int spew3d_window_InternalMainThreadProcessEvent(
        s3d_event *e
        ) {
    thread_MarkAsMainThread();

    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();

    mutex_Lock(_win_id_mutex);
    if (e->kind == S3DEV_INTERNAL_CMD_WIN_OPEN) {
        if (!_spew3d_window_ProcessWinOpenReq(e)) {
            mutex_Release(_win_id_mutex);
            _spew3d_event_q_InsertForce(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_WIN_UPDATECANVAS) {
        if (!_spew3d_window_ProcessWinUpdateCanvasReq(e)) {
            mutex_Release(_win_id_mutex);
            spew3d_event_q_Insert(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_WIN_CLOSE) {
        if (!_spew3d_window_ProcessWinCloseReq(e)) {
            mutex_Release(_win_id_mutex);
            _spew3d_event_q_InsertForce(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_DRAWPRIMITIVE_WINFILL) {
        if (!_spew3d_window_ProcessWinDrawFillReq(e)) {
            mutex_Release(_win_id_mutex);
            spew3d_event_q_Insert(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_WIN_DESTROY) {
        while (!_spew3d_window_ProcessWinCloseReq(e)) {
            mutex_Release(_win_id_mutex);
            spew3d_time_Sleep(10);
            mutex_Lock(_win_id_mutex);
        }
        int i = 0;
        while (i < _global_win_registry_fill) {
            if (_global_win_registry[i]->id == e->window.win_id) {
                _spew3d_window_FreeContents(
                    _global_win_registry[i]);
                free(_global_win_registry[i]);
                if (i + 1 < _global_win_registry_fill)
                    memmove(
                        &_global_win_registry[i],
                        &_global_win_registry[i + 1],
                        sizeof(*_global_win_registry) *
                            (_global_win_registry_fill - 2)
                    );
                _global_win_registry_fill--;
                continue;
            }
            i++;
        }
        mutex_Release(_win_id_mutex);
        return 1;
    }
    mutex_Release(_win_id_mutex);
    return 0;
}

S3DHID int _spew3d_window_ProcessWinOpenReq(s3d_event *ev) {
    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    assert(eq != NULL);

    assert(mutex_IsLocked(_win_id_mutex));
    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win) {
        return 1;
    }

    uint32_t flags = win->flags;
    if ((flags & SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL) == 0) {
        if (!win->backend->CreateWinObj(
                win->backend, win, win->backend_winfo,
                win->flags, win->title, win->width, win->height)) {
            return 0;
        }
        spew3d_window_UpdateGeometryInfo(win);
        return 1;
    }

    // Create a virtual window:
    win->virtualwin.canvas = spew3d_texture_NewWritable(
        NULL, win->width, win->height
    );
    if (win->virtualwin.canvas == 0) {
        return 0;
    }
    win->dpiscale = 1.0;
    win->width = win->width;
    win->height = win->height;

    return 1;
}

S3DEXP void spew3d_window_PresentToScreen(s3d_window *win) {
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    if (!eq)
        return;

    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_WIN_UPDATECANVAS;
    e.window.win_id = win->id;
    if (!spew3d_event_q_Insert(eq, &e))
        return;
}

S3DHID int _spew3d_window_ProcessWinUpdateCanvasReq(s3d_event *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win)
        return 1;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Renderer *sdlrenderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer_nolock(
        win, NULL, &sdlrenderer
    );
    if (sdlrenderer != NULL) {
        SDL_RenderPresent(sdlrenderer);
        return 1;
    }
    #endif

    return 1;
}

S3DEXP void spew3d_window_Destroy(s3d_window *win) {
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    if (!eq)
        return;

    if (win->wasclosed)
        return;

    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_WIN_CLOSE;
    e.window.win_id = win->id;
    _spew3d_event_q_InsertForce(eq, &e);
    e.kind = S3DEV_INTERNAL_CMD_WIN_DESTROY;
    e.window.win_id = win->id;
    _spew3d_event_q_InsertForce(eq, &e);
}

S3DHID void _spew3d_window_ActuallyDestroy(s3d_window *win) {
    if (!win)
        return;

    if (win->backend_winfo != NULL) {
        win->backend->DestroyWinInfo(
            win->backend, win, win->backend_winfo
        );
    }
    free(win);
}

S3DHID int _spew3d_window_ProcessWinCloseReq(s3d_event *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win)
        return 1;

    win->wasclosed = 1;
    int result = spew3d_Deletion_Queue(DELETION_WINDOW, win);

    return 1;
}

S3DHID int _spew3d_window_ProcessWinDrawFillReq(s3d_event *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(
        ev->drawprimitive.win_id);
    if (!win)
        return 1;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Window *swin = NULL;
    SDL_Renderer *sdlrenderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer_nolock(
        win, &swin, &sdlrenderer
    );
    if (sdlrenderer != NULL) {
        double redv = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.red * 256.0));
        double greenv = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.green * 256.0));
        double bluev = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.blue * 256.0));
        SDL_SetRenderDrawColor(
            sdlrenderer, redv, greenv, bluev, 255
        );
        SDL_RenderClear(sdlrenderer);
        return 1;
    }
    #endif
    assert(win->virtualwin.canvas != 0);
    // FIXME: implement this later for virtual windows.
    return 1;
}

S3DEXP void spew3d_window_FillWithColor(
        s3d_window *win,
        s3dnum_t red, s3dnum_t green, s3dnum_t blue
        ) {
    s3d_equeue *eq = _spew3d_event_GetInternalQueue();
    if (!eq)
        return;

    mutex_Lock(_win_id_mutex);

    if (win->wasclosed) {
        mutex_Release(_win_id_mutex);
        return;
    }

    s3d_event e = {0};
    e.kind = S3DEV_INTERNAL_CMD_DRAWPRIMITIVE_WINFILL;
    e.drawprimitive.win_id = win->id;
    e.drawprimitive.red = red;
    e.drawprimitive.green = green;
    e.drawprimitive.blue = blue;
    _spew3d_event_q_InsertForce(eq, &e);
    mutex_Release(_win_id_mutex);
}

S3DEXP s3d_window *spew3d_window_New(
        const char *title, uint32_t flags
        ) {
    return spew3d_window_NewExEx(title, flags, 0, 0, 0);
}

S3DEXP s3d_window *spew3d_window_NewEx(
        const char *title, uint32_t flags,
        int32_t width, int32_t height
        ) {
    return spew3d_window_NewExEx(
        title, flags, 0, width, height);
}

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
        s3d_window *win, SDL_Window **out_w,
        SDL_Renderer **out_r
        ) {
    mutex_Lock(_win_id_mutex);
    spew3d_window_GetSDLWindowAndRenderer_nolock(
        win, out_w, out_r
    );
    mutex_Release(_win_id_mutex);
}
#endif

S3DEXP s3d_point spew3d_window_GetWindowSize(
        s3d_window *win
        ) {
    mutex_Lock(_win_id_mutex);
    s3d_point result;
    result.x = ((s3dnum_t)win->width);
    result.y = ((s3dnum_t)win->height);
    mutex_Release(_win_id_mutex);
    return result;
}

S3DEXP const char *spew3d_window_GetTitle(
        s3d_window *win
        ) {
    return win->title;
}

S3DEXP void spew3d_window_PointToCanvasDrawPixels(
        s3d_window *win, s3d_point point,
        int32_t *x, int32_t *y
        ) {
    mutex_Lock(_win_id_mutex);
    _spew3d_window_WaitForCanvasInfo(win);
    *x = round((double)point.x * win->dpiscale);
    *y = round((double)point.y * win->dpiscale);
    mutex_Release(_win_id_mutex);
}

S3DHID void spew3d_window_UpdateGeometryInfo(s3d_window *win) {
    assert(mutex_IsLocked(_win_id_mutex));

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    SDL_Window *swin = NULL;
    SDL_Renderer *sdlrenderer = NULL;
    spew3d_window_GetSDLWindowAndRenderer_nolock(
        win, &swin, &sdlrenderer
    );
    if (swin != NULL) {
        SDL_Renderer *renderer =sdlrenderer;
        if (!renderer) {
            win->dpiscale = 1;
            int w = win->width;
            int h = win->height;
            if (w < 1)
                w = 1;
            if (h < 1)
                h = 1;
            win->canvaswidth = win->width;
            win->canvasheight = win->height;
            return;
        }
        int w, h;
        SDL_RenderGetLogicalSize(renderer, &w, &h);
        if (w == 0 && h == 0) {
            if (SDL_GetRendererOutputSize(
                    renderer, &w, &h
                    ) != 0) {
                w = 1;
                h = 1;
            }
        }
        if (w < 1)
            w = 1;
        if (h < 1)
            h = 1;
        win->canvaswidth = w;
        win->canvasheight = h;
        int ww, wh;
        SDL_GetWindowSize(swin, &ww, &wh);
        if (ww < 1)
            ww = 1;
        if (wh < 1)
            wh = 1;
        win->width = ww;
        win->height = wh;
    } else {
        if (win->width < 1) win->width = 1;
        if (win->height < 1) win->height = 1;
        if (win->canvaswidth < 1) win->canvaswidth = 1;
        if (win->canvasheight < 1) win->canvasheight = 1;
    }
    #endif  // #ifndef SPEW3D_OPTION_DISABLE_SDL
    win->dpiscale = (double)(((double)win->width) /
        (double)win->canvaswidth);
}

S3DHID void _spew3d_window_WaitForCanvasInfo(s3d_window *win) {
    assert(mutex_IsLocked(_win_id_mutex));
    int showedwarning = 0;
    uint64_t waitstart = spew3d_time_Ticks();
    if (win->canvaswidth == 0 || win->dpiscale == 0) {
        while (1) {
            mutex_Release(_win_id_mutex);
            if (thread_InMainThread())
                spew3d_event_UpdateMainThread();
            if (!showedwarning &&
                    spew3d_time_Ticks() > waitstart + 2000) {
                printf("spew3d_window.c: warning: "
                    "Stuck waiting for canvas info for more than "
                    "2 seconds, regarding window with id %d "
                    "(main thread: %s)\n",
                    (int)win->id, (thread_InMainThread() ? "yes" : "no"));
                showedwarning = 1;
            }
            spew3d_time_Sleep(10);
            mutex_Lock(_win_id_mutex);
            if (win->canvaswidth != 0 && win->dpiscale != 0) {
                break;
            }
        }
    }
}

S3DEXP void spew3d_window_SetMouseLockMode(
        s3d_window *win, int mode
        ) {
    if (mode != S3D_MOUSE_LOCK_DISABLED &&
            mode != S3D_MOUSE_LOCK_CONFINED_ABSOLUTE_MODE &&
            mode != S3D_MOUSE_LOCK_INVISIBLE_RELATIVE_MODE) {
        return;
    }
    mutex_Lock(_win_id_mutex);
    win->mouse_lock_mode = mode;
    mutex_Release(_win_id_mutex);
}

S3DEXP int32_t spew3d_window_GetCanvasDrawWidth(s3d_window *win) {
    mutex_Lock(_win_id_mutex);
    _spew3d_window_WaitForCanvasInfo(win);
    mutex_Release(_win_id_mutex);
    return win->canvaswidth;
}

S3DEXP int32_t spew3d_window_GetCanvasDrawHeight(s3d_window *win) {
    mutex_Lock(_win_id_mutex);
    _spew3d_window_WaitForCanvasInfo(win);
    mutex_Release(_win_id_mutex);
    return win->canvasheight;
}

#endif  // SPEW3D_IMPLEMENTATION

