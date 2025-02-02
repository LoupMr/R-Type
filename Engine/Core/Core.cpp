#include "Core.hpp"
#include <raylib.h> // Include Raylib header

namespace GraphicalLibrary {
    float GetDeltaTime() {
        return GetFrameTime(); // Explicitly call Raylib's GetFrameTime()
    }
}