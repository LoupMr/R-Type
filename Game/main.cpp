#include "Library/GraphicalLibrary.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ComponentManager.hpp"
#include "ECS/SystemManager.hpp"
#include "Graphics/Window.hpp"
#include "Input/Input.hpp"
#include "Audio/Audio.hpp"
#include "Components/Components.hpp"
#include "Systems/Systems.hpp"
#include <iostream>
#include <cmath>
#include <filesystem>  // For std::filesystem::path

// Function to resolve asset paths
std::string GetAssetPath(const std::string& assetName) {
    std::filesystem::path assetPath = std::filesystem::current_path() / "assets" / assetName;
    std::cout << "[DEBUG] Resolved asset path: " << assetPath.string() << std::endl; // Debug print
    return assetPath.string();
}

int main() {
    const int screenWidth = 800;
    const int screenHeight = 600;

    std::cout << "[DEBUG] Initializing window..." << std::endl; // Debug print
    GraphicalLibrary::Window::Initialize(screenWidth, screenHeight, "R-Type Game");

    std::cout << "[DEBUG] Initializing audio device..." << std::endl; // Debug print
    GraphicalLibrary::Audio::InitializeAudioDevice();
    Sound beepSound = GraphicalLibrary::Audio::LoadAudioFromFile(GetAssetPath("beep.wav").c_str());
    if (beepSound.frameCount == 0) {
        std::cerr << "[ERROR] Failed to load beep.wav\n";
    } else {
        std::cout << "[INFO] beep.wav loaded successfully.\n";
    }

    // Load textures
    Texture2D playerTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("player.png").c_str());
    Texture2D enemyTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("enemy.png").c_str());
    Texture2D bulletTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("bullet.png").c_str());

    if (playerTexture.id == 0 || enemyTexture.id == 0 || bulletTexture.id == 0) {
        std::cerr << "[ERROR] Failed to load one or more textures\n";
        return -1;
    } else {
        std::cout << "[INFO] All textures loaded successfully.\n";
    }

    // ECS-related managers
    EntityManager em;
    ComponentManager cm;

    std::cout << "[DEBUG] Creating player entity..." << std::endl; // Debug print
    Entity player = em.createEntity();
    cm.addComponent(player, Position{100.0f, 300.0f});
    cm.addComponent(player, Velocity{0.0f, 0.0f});
    cm.addComponent(player, KeyboardControl{});
    cm.addComponent(player, Health{3, 3});
    cm.addComponent(player, Sprite{playerTexture, playerTexture.width, playerTexture.height});

    std::cout << "[DEBUG] Creating systems..." << std::endl; // Debug print
    RenderSystem renderSystem;
    MovementSystem movementSystem;
    InputSystem inputSystem;
    ShootingSystem shootingSystem;
    EnemySystem enemySystem;
    CollisionSystem collisionSystem;
    AudioSystem audioSystem(beepSound);

    // Store textures for reuse
    cm.setGlobalTexture("player", playerTexture);
    cm.setGlobalTexture("enemy", enemyTexture);
    cm.setGlobalTexture("bullet", bulletTexture);

    std::cout << "[DEBUG] Entering main loop..." << std::endl; // Debug print
    while (!GraphicalLibrary::Window::ShouldCloseWindow()) {
        float dt = GetFrameTime();

        inputSystem.handleInput(em, cm);
        inputSystem.update(dt, em, cm);
        movementSystem.update(dt, em, cm);
        shootingSystem.update(dt, em, cm);
        enemySystem.update(dt, em, cm);
        collisionSystem.update(dt, em, cm);
        audioSystem.update(dt, em, cm);

        GraphicalLibrary::Window::StartDrawing();
        GraphicalLibrary::Window::ClearScreen(RAYWHITE);
        renderSystem.update(dt, em, cm);
        GraphicalLibrary::Window::StopDrawing();
    }

    std::cout << "[DEBUG] Cleaning up..." << std::endl; // Debug print
    GraphicalLibrary::Audio::ReleaseAudio(beepSound);
    GraphicalLibrary::Audio::ShutdownAudioDevice();
    GraphicalLibrary::Texture::ReleaseTexture(playerTexture);
    GraphicalLibrary::Texture::ReleaseTexture(enemyTexture);
    GraphicalLibrary::Texture::ReleaseTexture(bulletTexture);
    GraphicalLibrary::Window::Shutdown();

    std::cout << "[DEBUG] Exiting program..." << std::endl; // Debug print
    return 0;
}