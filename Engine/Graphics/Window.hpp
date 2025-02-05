#ifndef GRAPHICAL_LIBRARY_WINDOW_HPP
#define GRAPHICAL_LIBRARY_WINDOW_HPP

#include "Core/Core.hpp"

namespace GraphicalLibrary {
    class Window {
    public:
        static void Initialize(int screenWidth, int screenHeight, const std::string& title); // Renamed from Init
        static void Shutdown();
        static bool ShouldCloseWindow();
        static void StartDrawing();
        static void StopDrawing();
        static void ClearScreen(Color color);
    };
}

#endif // GRAPHICAL_LIBRARY_WINDOW_HPP