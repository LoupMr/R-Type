#include "Window.hpp"

namespace GraphicalLibrary {
    void Window::Initialize(int screenWidth, int screenHeight, const std::string& title) {
        InitWindow(screenWidth, screenHeight, title.c_str());
        SetTargetFPS(60);
    }

    void Window::Shutdown() {
        CloseWindow();
    }

    bool Window::ShouldCloseWindow() {
        return WindowShouldClose();
    }

    void Window::StartDrawing() {
        BeginDrawing();
    }

    void Window::StopDrawing() {
        EndDrawing();
    }

    void Window::ClearScreen(Color color) {
        ClearBackground(color);
    }
}
