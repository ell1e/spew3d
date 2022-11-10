
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <math.h>
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdint.h>


int main(int argc, const char **argv) {
    printf("Initializing Spew3D with window and renderer\n");
    SDL_Window *window = NULL;
    SDL_Renderer * renderer = NULL;
    spew3d_ctx *ctx = spew3d_CreateSDLWindowForMe(
        "Spew 3D Cube Example", 0
    );
    if (!ctx) {
        fprintf(stderr, "Spew3D initialization failed\n");
        return 1;
    }
    spew3d_ctx_GetSDLWindowAndRenderer(ctx, &window, &renderer);

    printf("Creating a cube\n");
    spew3d_geometry *cube = spew3d_geometry_Create();
    if (cube) {
        if (!spew3d_geometry_AddCubeSimple(
                cube, 1.0,
                spew3d_texture_FromFile("numberone.jpg", 0), 0
                )) {
            spew3d_geometry_Destroy(cube);
            cube = NULL;
        }
    }
    if (!cube) {
        fprintf(stderr, "Failed to create geometry\n");
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
        SDL_RenderPresent(renderer);
    }

    printf("Shutting down\n");
    return 0;
}

