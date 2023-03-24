
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
    spew3d_audio_sink *sink = audio_sink_CreateOutput(48000);
    if (!sink) {
        fprintf(stderr, "Failed to crate sink\n");
        return 1;
    }

    printf("Entering main loop\n");
    int notquit = 1;
    while (notquit) {
        audio_sink_MainThreadUpdate();
    }

    printf("Shutting down\n");
    return 0;
}

