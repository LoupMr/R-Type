#ifndef GRAPHICAL_LIBRARY_INPUT_HPP
#define GRAPHICAL_LIBRARY_INPUT_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Input {
    public:
        static bool IsKeyPressed(int key); // Renamed from IsKeyDown
    };
}

#endif // GRAPHICAL_LIBRARY_INPUT_HPP