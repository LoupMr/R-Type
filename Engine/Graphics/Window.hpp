#ifndef ENGINE_WINDOW_HPP
#define ENGINE_WINDOW_HPP

#include <raylib.h>
#include <string>

namespace Engine {
    class Window {
    public:
        static void Initialize(int screenWidth, int screenHeight, const std::string& title);
        static void Shutdown();
        static bool ShouldCloseWindow();
        static void StartDrawing();
        static void StopDrawing();
        static void ClearScreen(Color color);
    };
}

#endif // ENGINE_WINDOW_HPP
