
//#define DEBUG_SPEW3D_AUDIO_DECODE
//#define DEBUG_SPEW3D_AUDIO_DECODE_DATA
#define DEBUG_SPEW3D_AUDIO_SINK
#define DEBUG_SPEW3D_AUDIO_SINK_DATA

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
    spew3d_audio_sink *sink = spew3d_audio_sink_CreateStereoOutput(48000);
    if (!sink) {
        fprintf(stderr, "Failed to crate sink\n");
        return 1;
    }
    printf("Opening file to directly feed into output sink\n");
    /*s3daudiodecoder *decoder = audiodecoder_NewFromFile(
        "test.mp3"
    );
    if (!decoder || !spew3d_audio_sink_FeedFromDecoder(
            sink, decoder
            )) {
        fprintf(stderr, "Failed to set up decoded file\n");
        return 1;
    }*/

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

