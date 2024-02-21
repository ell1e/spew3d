
#define DEBUG_SPEW3D_EVENT
#define SPEW3D_DEBUG_OUTPUT
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <math.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>

int main(int argc, const char **argv) {
    printf("Initializing Spew3D with window and renderer\n");
    s3d_window *win = spew3d_window_New(
        "Spew 3D Sprite Example", 0
    );
    if (!win) {
        fprintf(stderr, "Spew3D initialization failed\n");
        return 1;
    }

    // First, ensure we're in the right folder:
    int _exists = 0;
    if (!spew3d_fs_TargetExists("hello_world.png", &_exists) || !_exists) {
        fprintf(stderr, "You didn't run this in 'examples' folder, "
            "or there was an I/O error.\n");
        return 1;
    }

    printf("Loading sprite texture\n");
    s3d_texture_t spritetex = spew3d_texture_FromFile(
        "hello_world.png", 0
    );
    if (!spritetex) {
        fprintf(stderr, "Failed to create a sprite\n");
        return 1;
    }

    printf("Entering main loop\n");
    uint64_t start_ts = spew3d_time_Ticks();
    int notquit = 1;
    while (notquit && spew3d_time_Ticks() < start_ts + 5000) {
        s3devent_UpdateMainThread();

        s3devent e = {0};
        while (s3devent_q_Pop(s3devent_GetMainQueue(), &e)) {
            if (e.kind == S3DEV_WINDOW_USER_CLOSE_REQUEST ||
                    e.kind == S3DEV_APP_QUIT_REQUEST) {
                notquit = 0;
                break;
            }
        }
        spew3d_window_FillWithColor(win, 0.9, 0.5, 0.8);

        s3d_point p = spew3d_window_GetWindowSize(win);
        p.x /= 2.0;
        p.y /= 2.0;
        int32_t imagewidth, imageheight;
        if (spew3d_texture_GetSize(
                spritetex, &imagewidth, &imageheight
                )) {
            assert(imagewidth != 0 && imageheight != 0);
            double scale = (
                fmin((double)p.x * 2, (double)p.y * 2) /
                    (double)imagewidth);
            int result = spew3d_texture_Draw(
                win, spritetex, p,
                1, scale,
                45, 1.0, 1.0, 1.0, 1.0, 1
            );
            assert(result != 0);
        }
        spew3d_window_PresentToScreen(win);
    }
    spew3d_window_Destroy(win);

    printf("Shutting down\n");
    return 0;
}

