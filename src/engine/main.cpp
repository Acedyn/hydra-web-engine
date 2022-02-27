#include <iostream>
#include "emscripten.h"
#include <stdio.h>
#include <SDL/SDL.h>
#include "webgpu/webgpu.h"

extern "C" {
    int main(int argc, char *argv[])
    {
        std::cout << "Hello world" << std::endl;

        SDL_Init(SDL_INIT_VIDEO);
        SDL_Surface *screen = SDL_SetVideoMode(256, 256, 32, SDL_SWSURFACE);

        if (SDL_MUSTLOCK(screen)) {SDL_LockSurface(screen);}

        for (int i = 0; i < 256; i++) {
            for (int j = 0; j < 256; j++) {
                #ifdef TEST_SDL_LOCK_OPTS
                int alpha = 255;
                #else
                int alpha = (i+j) % 255;
                #endif

                *((Uint32*)screen->pixels + i * 256 + j) = SDL_MapRGBA(screen->format, i, j, 255-i, alpha);
            }
        }

        if (SDL_MUSTLOCK(screen)) {SDL_UnlockSurface(screen);}

        SDL_Flip(screen);

        SDL_Quit();

        return 0;
    }
}
