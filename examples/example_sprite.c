
#include <math.h>
#include <SDL2/SDL.h>
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <stdint.h>
#include <stdio.h>


int main(int argc, const char **argv) {
    printf("Initializing SDL2\n");
    int result = SDL_Init(SDL_INIT_EVERYTHING);
    if (result != 0) {
        fprintf(stderr, "SDL_Init() failed\n");
        return 1;
    }

    printf("Initializing Spew3D with window and renderer\n");
    SDL_Window *window = NULL;
    SDL_Renderer * renderer = NULL;
    if (!spew3d_Init(
            "Spew 3D Sprite Example", 0,
            &window, &renderer)) {
        fprintf(stderr, "Spew3D initialization failed\n");
        return 1;
    }

    printf("Loading sprite texture\n");
    spew3d_texture_t spritetex = spew3d_texture_FromFile(
        "helloworld.png"
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

        int32_t cvwidth = spew3d_window_CanvasWidth();
        int32_t cvheight = spew3d_window_CanvasHeight();
        int32_t imagewidth = spew3d_texinfo(spritetex)->width;
        int32_t imageheight = spew3d_texinfo(spritetex)->height;
        spew3d_texture_Draw(
            spritetex,
            cvwidth / 2 - imagewidth / 2,
            cvheight / 2 - imageheight / 2,
            fmin((double)cvwidth, (double)cvheight) /
                (double)imagewidth,
            90, 1.0, 1.0, 1.0, 1.0, 1
        );

        SDL_RenderPresent(renderer);
    }

    printf("Shutting down\n");
    return 0;
}

