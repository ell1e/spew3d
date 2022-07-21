
#include <SDL2/SDL.h>
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION


int main(int argc, const char **argv) {
    int result = SDL_Init(SDL_INIT_EVERYTHING);
    if (result != 0) {
        fprintf(stderr, "SDL_Init() failed\n");
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow(
        "Spew 3D Cube Example", SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, 800, 500,
        SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL|
        SDL_WINDOW_ALLOW_HIGHDPI);
    SDL_Renderer *renderer = NULL;
    if (window != NULL) {
        renderer = SDL_CreateRenderer(window, -1,
            SDL_RENDERER_PRESENTVSYNC|SDL_RENDERER_ACCELERATED);
    }
    if (!window || !renderer) {
        fprintf(stderr, "Window or renderer creation failed\n");
        return 1;
    }

    spew3d_geometry *cube = spew3d_geometry_Create();
    if (cube) {
        if (!spew3d_geometry_AddCubeSimple(
                cube, 1.0, spew3d_texture_ByFile("numberone.jpg")
                )) {
            spew3d_geometry_Destroy(cube);
            cube = NULL;
        }
    }
    if (!cube) {
        fprintf(stderr, "Failed to create geometry\n");
    }

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

    return 0;
}

