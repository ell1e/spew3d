
#define SPEW3D_DEBUG_OUTPUT
#define SPEW3D_IMPLEMENTATION
#include <spew3d.h>
#undef SPEW3D_IMPLEMENTATION
#include <math.h>
#include <SDL2/SDL.h>
#include <stdint.h>
#include <stdio.h>


int main(int argc, const char **argv) {
    printf("Initializing Spew3D audio pipeline\n");
    s3d_audio_sink *sink = spew3d_audio_sink_CreateStereoOutput(48000);
    if (!sink) {
        fprintf(stderr, "Failed to create sink\n");
        return 1;
    }

    const char playfilename[] = "audiotest.mp3";

    // First, ensure we're in the right folder:
    int _exists = 0;
    if (!spew3d_fs_TargetExists(playfilename, &_exists) || !_exists) {
        fprintf(stderr, "You didn't run this in 'examples' folder, "
            "or there was an I/O error.\n");
        return 1;
    }

    // Then open mixer to play file:
    printf("Opening mixer for managed game audio\n");
    s3d_audio_mixer *mixer = spew3d_audio_sink_GetMixer(sink);
    if (!mixer) {
        fprintf(stderr, "Failed to set up a mixer.\n");
        return 1;
    }
    spew3d_audio_mixer_PlayFile(mixer, playfilename, 1.0, 0);

    // Main event loop:
    printf("Entering main loop\n");
    int notquit = 1;
    while (notquit) {
        spew3d_audio_sink_MainThreadUpdate();
        SDL_Event e = {0};
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) {
                notquit = 0;
                break;
            }
        }
    }

    printf("Shutting down\n");
    return 0;
}

