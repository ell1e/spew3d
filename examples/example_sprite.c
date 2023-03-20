
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
    SDL_Window *window = NULL;
    SDL_Renderer * renderer = NULL;
    spew3d_ctx *ctx = spew3d_CreateSDLWindowForMe(
        "Spew 3D Sprite Example", 0
    );
    if (!ctx) {
        fprintf(stderr, "Spew3D initialization failed\n");
        return 1;
    }
    spew3d_ctx_GetSDLWindowAndRenderer(ctx, &window, &renderer);

    printf("Loading sprite texture\n");
    spew3d_texture_t spritetex = spew3d_texture_FromFile(
        "./hello_world.png", 0
    );
    if (!spritetex) {
        fprintf(stderr, "Failed to load sprite\n");
        return 1;
    }

    printf("Entering main loop\n");
    int notquit = 1;
    while (notquit) {
        SDL_Event e = {0};
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                notquit = 0;
                break;
            } else if (e.type == SDL_WINDOWEVENT) {
                if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
                    notquit = 0;
                    break;
                }
            }
        }
        SDL_SetRenderDrawColor(renderer, 255, 225, 255, 255);
        SDL_RenderClear(renderer);

        int32_t cvwidth = spew3d_window_CanvasDrawWidth(ctx);
        int32_t cvheight = spew3d_window_CanvasDrawHeight(ctx);
        int32_t imagewidth, imageheight;
        if (spew3d_texture_GetSize(
                spritetex, &imagewidth, &imageheight
                )) {
            assert(imagewidth != 0 && imageheight != 0);
            double scale = (
                fmin((double)cvwidth, (double)cvheight) /
                    (double)imagewidth);
            int result = spew3d_texture_Draw(
                ctx, spritetex,
                cvwidth / 2, cvheight / 2,
                1, scale,
                45, 1.0, 1.0, 1.0, 1.0, 1
            );
            assert(result != 0);
        }
        SDL_RenderPresent(renderer);
    }

    printf("Shutting down\n");
    return 0;
}

