#ifndef GRAPHICAL_LIBRARY_CORE_HPP
#define GRAPHICAL_LIBRARY_CORE_HPP

#include <raylib.h>
#include <string>

namespace GraphicalLibrary {
    // Forward declarations
    class Window;
    class Texture;
    class Audio;
    class Input;

    // Common utility functions
    float GetDeltaTime(); // Renamed from GetFrameTime
}

#endif // GRAPHICAL_LIBRARY_CORE_HPP