#include "Input.hpp"

namespace Engine {
    bool Input::IsKeyPressed(int key) {
        return ::IsKeyPressed(key);
    }

    bool Input::IsKeyDown(int key) {
        return ::IsKeyDown(key);
    }
}
