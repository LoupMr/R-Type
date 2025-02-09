#ifndef ENGINE_INPUT_HPP
#define ENGINE_INPUT_HPP

#include <raylib.h>

namespace Engine {
    class Input {
    public:
        static bool IsKeyPressed(int key);
        static bool IsKeyDown(int key);
    };
}

#endif // ENGINE_INPUT_HPP
