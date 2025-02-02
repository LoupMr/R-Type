#ifndef GRAPHICAL_LIBRARY_WINDOW_HPP
#define GRAPHICAL_LIBRARY_WINDOW_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Window {
    public:
        static void Initialize(int screenWidth, int screenHeight, const std::string& title); // Renamed from Init
        static void Shutdown(); // Renamed from Close
        static bool ShouldCloseWindow(); // Renamed from ShouldClose
        static void StartDrawing(); // Renamed from BeginDrawing
        static void StopDrawing(); // Renamed from EndDrawing
        static void ClearScreen(Color color); // Renamed from ClearBackground
    };
}

#endif // GRAPHICAL_LIBRARY_WINDOW_HPP