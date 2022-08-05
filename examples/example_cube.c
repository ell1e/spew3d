
#include <SDL2/SDL.h>
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION


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
            "Spew 3D Cube Example", 0,
            &window, &renderer)) {
        fprintf(stderr, "Spew3D initialization failed\n");
        return 1;
    }

    printf("Creating a cube\n");
    spew3d_geometry *cube = spew3d_geometry_Create();
    if (cube) {
        if (!spew3d_geometry_AddCubeSimple(
                cube, 1.0,
                spew3d_texture_NewFromFile("numberone.jpg"), 0
                )) {
            spew3d_geometry_Destroy(cube);
            cube = NULL;
        }
    }
    if (!cube) {
        fprintf(stderr, "Failed to create geometry\n");
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

