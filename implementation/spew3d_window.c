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
int _global_win_registry_fill =  0;
int _global_win_registry_alloc = 0;

typedef struct s3d_window {
    uint32_t id;
    uint32_t flags;
    char *title;
    int32_t canvaswidth, canvasheight;
    s3dnum_t dpiscale;
    int wasclosed;
    #ifndef SPEW3D_OPTION_DISABLE_SDL
    int owns_sdl_window;
    SDL_Window *_sdl_outputwindow;
    int owns_sdl_renderer;
    SDL_Renderer *_sdl_outputrenderer;
    #endif
    struct virtualwin {
        s3d_texture_t canvas;
    } virtualwin;
    int32_t width, height;
} s3d_window;

S3DHID __attribute__((constructor)) void _ensure_winid_mutex() {
    if (_win_id_mutex != NULL)
        return;
    _win_id_mutex = mutex_Create();
    if (!_win_id_mutex) {
        fprintf(stderr, "spew3d_window.c: error: "
            "Failed to allocate event queues.\n");
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

S3DHID static s3d_window *spew3d_window_NewExEx(
        const char *title, uint32_t flags,
        int dontinitactualwindow, int32_t width, int32_t height
        ) {
    s3dequeue *eq = _s3devent_GetInternalQueue();
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

    win->id = _spew3d_window_MakeNewID_nolock();
    if (width <= 0) width = 800;
    if (height <= 0) height = 500;
    win->width = width;
    win->height = height;
    win->flags = flags;
    win->title = strdup(title);
    if (!win->title) {
        free(win);
        mutex_Release(_win_id_mutex);
        return NULL;
    }

    if (!dontinitactualwindow) {
        s3devent e = {0};
        e.kind = S3DEV_INTERNAL_CMD_WIN_OPEN;
        e.window.win_id = win->id;
        if (!s3devent_q_Insert(eq, &e)) {
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
        if (_global_win_registry[i]->_sdl_outputwindow != NULL &&
                sdlid == SDL_GetWindowID(
                _global_win_registry[i]->_sdl_outputwindow)) {
            mutex_Release(_win_id_mutex);
            return _global_win_registry[i];
        }
        i++;
    }
    mutex_Release(_win_id_mutex);
    return NULL;
}
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DHID int _spew3d_window_HandleSDLEvent(SDL_Event *e) {
    thread_MarkAsMainThread();

    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    s3dequeue *eq = _s3devent_GetInternalQueue();
    assert(eq != NULL);
    s3dequeue *equser = s3devent_GetMainQueue();
    assert(equser != NULL);

    if (e->type == SDL_QUIT) {
        s3devent e2 = {0};
        e2.kind = S3DEV_APP_QUIT_REQUEST;
        _s3devent_q_InsertForce(equser, &e2);
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
            _s3devent_q_InsertForce(equser, &e2);
            i++;
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->type == SDL_WINDOWEVENT) {
        s3d_window *win = spew3d_window_GetBySDLWindowID(e->window.windowID);
        if (win != NULL && e->window.event == SDL_WINDOWEVENT_CLOSE) {
            s3devent e2 = {0};
            e2.kind = S3DEV_WINDOW_USER_CLOSE_REQUEST;
            e2.window.win_id = win->id;
            _s3devent_q_InsertForce(equser, &e2);
        }
        return 1;
    }
    return 0;
}
#endif

S3DHID int _spew3d_window_ProcessWinOpenReq(s3devent *ev);

S3DHID int _spew3d_window_ProcessWinDrawFillReq(s3devent *ev);

S3DHID int _spew3d_window_ProcessWinUpdateCanvasReq(s3devent *ev);

S3DHID int _spew3d_window_ProcessWinCloseReq(s3devent *ev);

S3DHID int _spew3d_window_ProcessWinDestroyReq(s3devent *ev);

S3DEXP int spew3d_window_MainThreadProcessEvent(s3devent *e) {
    thread_MarkAsMainThread();

    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    s3dequeue *eq = _s3devent_GetInternalQueue();

    mutex_Lock(_win_id_mutex);
    if (e->kind == S3DEV_INTERNAL_CMD_WIN_OPEN) {
        if (!_spew3d_window_ProcessWinOpenReq(e)) {
            mutex_Release(_win_id_mutex);
            _s3devent_q_InsertForce(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_WIN_UPDATECANVAS) {
        if (!_spew3d_window_ProcessWinUpdateCanvasReq(e)) {
            mutex_Release(_win_id_mutex);
            s3devent_q_Insert(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_WIN_CLOSE) {
        if (!_spew3d_window_ProcessWinCloseReq(e)) {
            mutex_Release(_win_id_mutex);
            _s3devent_q_InsertForce(eq, e);
        }
        mutex_Release(_win_id_mutex);
        return 1;
    } else if (e->kind == S3DEV_INTERNAL_CMD_DRAWPRIMITIVE_WINFILL) {
        if (!_spew3d_window_ProcessWinDrawFillReq(e)) {
            mutex_Release(_win_id_mutex);
            s3devent_q_Insert(eq, e);
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

S3DHID int _spew3d_window_ProcessWinOpenReq(s3devent *ev) {
    _ensure_winid_mutex();
    assert(_win_id_mutex != NULL);
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;
    s3dequeue *eq = _s3devent_GetInternalQueue();
    assert(eq != NULL);

    assert(mutex_IsLocked(_win_id_mutex));
    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win) {
        return 1;
    }

    uint32_t flags = win->flags;
    if ((flags & SPEW3D_WINDOW_FLAG_FORCE_HIDDEN_VIRTUAL) == 0) {
        #ifndef SPEW3D_OPTION_DISABLE_SDL
        int isfullscreen = (
            (flags & SPEW3D_WINDOW_FLAG_FULLSCREEN) != 0
        );

        // Try to create a 3d accelerated OpenGL window:
        SDL_Window *window = SDL_CreateWindow(
            win->title, SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, win->width, win->height,
            SDL_WINDOW_RESIZABLE|
            (isfullscreen ? SDL_WINDOW_FULLSCREEN:0)|
            (((flags &
                    SPEW3D_WINDOW_FLAG_FORCE_NO_3D_ACCEL) == 0) ?
                SDL_WINDOW_OPENGL:0)|
            SDL_WINDOW_ALLOW_HIGHDPI);
        SDL_Renderer *renderer = NULL;
        if (window) {  // Try to create 3d accel OpenGL renderer:
            renderer = SDL_CreateRenderer(window, -1,
                SDL_RENDERER_PRESENTVSYNC|
                (((flags &
                        SPEW3D_WINDOW_FLAG_FORCE_NO_3D_ACCEL) == 0) ?
                    SDL_RENDERER_ACCELERATED:
                    SDL_RENDERER_SOFTWARE));
            if (!renderer) {
                SDL_DestroyWindow(window);
                window = NULL;
                // Failed. Go on and retry non-3d window...
            } else {
                win->owns_sdl_window = 1;
                win->_sdl_outputwindow = window;
                win->owns_sdl_renderer = 1;
                win->_sdl_outputrenderer = renderer;
                spew3d_window_UpdateGeometryInfo(win);
                return 1;
            }
        }

        // Try to create a non-OpenGL, likely unaccelerated window:
        window = SDL_CreateWindow(
            win->title, SDL_WINDOWPOS_UNDEFINED,
            SDL_WINDOWPOS_UNDEFINED, win->width, win->height,
            SDL_WINDOW_RESIZABLE|
            (isfullscreen ? SDL_WINDOW_FULLSCREEN:0)|
            SDL_WINDOW_ALLOW_HIGHDPI);
        if (!window) {
            return 0;
        }
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_SOFTWARE);
        if (!renderer) {
            SDL_DestroyWindow(window);
            window = NULL;
            return 0;
        }

        assert(window != NULL && renderer != NULL);
        win->owns_sdl_window = 1;
        win->_sdl_outputwindow = window;
        win->owns_sdl_renderer = 1;
        win->_sdl_outputrenderer = renderer;
        spew3d_window_UpdateGeometryInfo(win);
        return 1;
        #endif  // ifndef SPEW3D_OPTION_DISABLE_SDL
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
    s3dequeue *eq = _s3devent_GetInternalQueue();
    if (!eq)
        return;

    s3devent e = {0};
    e.kind = S3DEV_INTERNAL_CMD_WIN_UPDATECANVAS;
    e.window.win_id = win->id;
    if (!s3devent_q_Insert(eq, &e))
        return;
}

S3DHID int _spew3d_window_ProcessWinUpdateCanvasReq(s3devent *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win)
        return 1;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->_sdl_outputrenderer != NULL) {
        SDL_RenderPresent(win->_sdl_outputrenderer);
        return 1;
    }
    #endif

    return 1;
}

S3DEXP void spew3d_window_Destroy(s3d_window *win) {
    s3dequeue *eq = _s3devent_GetInternalQueue();
    if (!eq)
        return;

    if (win->wasclosed)
        return;

    s3devent e = {0};
    e.kind = S3DEV_INTERNAL_CMD_WIN_CLOSE;
    e.window.win_id = win->id;
    _s3devent_q_InsertForce(eq, &e);
    e.kind = S3DEV_INTERNAL_CMD_WIN_DESTROY;
    e.window.win_id = win->id;
    _s3devent_q_InsertForce(eq, &e);
}

S3DHID int _spew3d_window_ProcessWinCloseReq(s3devent *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(ev->window.win_id);
    if (!win)
        return 1;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->_sdl_outputrenderer != NULL && win->owns_sdl_renderer) {
        SDL_DestroyRenderer(win->_sdl_outputrenderer);
        return 1;
    }
    win->_sdl_outputrenderer = NULL;
    if (win->_sdl_outputwindow != NULL && win->owns_sdl_window) {
        SDL_DestroyWindow(win->_sdl_outputwindow);
        return 1;
    }
    win->_sdl_outputwindow = NULL;
    #endif

    win->wasclosed = 1;
    return 1;
}

S3DHID int _spew3d_window_ProcessWinDrawFillReq(s3devent *ev) {
    if (!_internal_spew3d_InitSDLGraphics())
        return 0;

    s3d_window *win = _spew3d_window_GetByIDLocked(
        ev->drawprimitive.win_id);
    if (!win)
        return 1;

    #ifndef SPEW3D_OPTION_DISABLE_SDL
    if (win->_sdl_outputrenderer != NULL) {
        double redv = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.red * 256.0));
        double greenv = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.green * 256.0));
        double bluev = fmax(0.0, fmin(255.0,
            (double)ev->drawprimitive.blue * 256.0));
        SDL_SetRenderDrawColor(
            win->_sdl_outputrenderer, redv, greenv, bluev, 255
        );
        SDL_RenderClear(win->_sdl_outputrenderer);
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
    s3dequeue *eq = _s3devent_GetInternalQueue();
    if (!eq)
        return;

    mutex_Lock(_win_id_mutex);

    if (win->wasclosed) {
        mutex_Release(_win_id_mutex);
        return;
    }

    s3devent e = {0};
    e.kind = S3DEV_INTERNAL_CMD_DRAWPRIMITIVE_WINFILL;
    e.drawprimitive.win_id = win->id;
    e.drawprimitive.red = red;
    e.drawprimitive.green = green;
    e.drawprimitive.blue = blue;
    _s3devent_q_InsertForce(eq, &e);
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
    if (out_w) *out_w = win->_sdl_outputwindow;
    if (out_r) *out_r = win->_sdl_outputrenderer;
}
#endif

#ifndef SPEW3D_OPTION_DISABLE_SDL
S3DEXP void spew3d_window_GetSDLWindowAndRenderer(
        s3d_window *win, SDL_Window **out_w,
        SDL_Renderer **out_r
        ) {
    mutex_Lock(_win_id_mutex);
    if (out_w) *out_w = win->_sdl_outputwindow;
    if (out_r) *out_r = win->_sdl_outputrenderer;
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
    if (win->_sdl_outputwindow != NULL) {
        SDL_Renderer *renderer = NULL;
        SDL_Window *swin = win->_sdl_outputwindow;
        spew3d_window_GetSDLWindowAndRenderer_nolock(
            win, NULL, &renderer);
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
                s3devent_UpdateMainThread();
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

