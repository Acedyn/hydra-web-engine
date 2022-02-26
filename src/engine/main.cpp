#include <iostream>
#include "emscripten.h"

extern "C" {
    EMSCRIPTEN_KEEPALIVE
    int hello(int argc, char *argv[])
    {
        std::cout << "Hello world" << std::endl;
        return 0;
    }
}
