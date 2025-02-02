#include "Input.hpp"

namespace GraphicalLibrary {
    bool Input::IsKeyPressed(int key) {
        return IsKeyDown(key);
    }
}