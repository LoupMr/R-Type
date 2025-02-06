#include "Core.hpp"
#include <raylib.h>

namespace Engine {
    float GetDeltaTime() {
        // In raylib, GetFrameTime() returns the delta time
        return GetFrameTime();
    }
}
