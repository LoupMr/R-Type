#include "Library/GraphicalLibrary.hpp"
#include "ECS/EntityManager.hpp"
#include "ECS/ComponentManager.hpp"
#include "Graphics/Window.hpp"
#include "Input/Input.hpp"
#include "Audio/Audio.hpp"
#include "Components/Components.hpp"
#include "Systems/Systems.hpp"
#include "Systems/NetworkSystem.hpp"
#include "Systems/Protocol.hpp"

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <raylib.h>
#include <thread>
#include <chrono>
#include <atomic>

std::string GetAssetPath(const std::string& assetName) {
    return (std::filesystem::current_path() / "assets" / assetName).string();
}

enum GameScene { MENU, GAME };

void networkThreadFunc(NetworkSystem &netSys,
                       EntityManager &em,
                       ComponentManager &cm,
                       std::atomic<bool> &stopFlag,
                       std::atomic<bool> &gameStartedFlag)
{
    // 60 FPS => ~16ms
    while (!stopFlag.load()) {
        netSys.update(0.016f, em, cm);
        if (netSys.isGameStarted()) {
            gameStartedFlag.store(true);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <serverIP> <serverPort> <clientPort>\n";
        return 1;
    }
    std::string serverIP = argv[1];
    int serverPort = std::atoi(argv[2]);
    int clientPort = std::atoi(argv[3]);

    const int screenWidth  = 800;
    const int screenHeight = 600;

    // Initialize
    GraphicalLibrary::Window::Initialize(screenWidth, screenHeight, "R-Type Client");
    GraphicalLibrary::Audio::InitializeAudioDevice();

    // Load assets
    Sound beepSound = GraphicalLibrary::Audio::LoadAudioFromFile(GetAssetPath("beep.wav").c_str());
    Texture2D playerTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("player.png").c_str());
    Texture2D remotePlayerTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("player.png").c_str());
    Texture2D enemyTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("enemy.png").c_str());
    Texture2D bulletTexture = GraphicalLibrary::Texture::LoadTextureFromFile(GetAssetPath("bullet.png").c_str());

    if (!playerTexture.id || !remotePlayerTexture.id || !enemyTexture.id || !bulletTexture.id) {
        std::cerr << "[Error] Missing textures.\n";
        return -1;
    }

    // ECS
    EntityManager em;
    ComponentManager cm;

    // Local player entity
    Entity localPlayer = em.createEntity();
    cm.addComponent(localPlayer, Position{100.0f, 300.0f});
    cm.addComponent(localPlayer, Velocity{0,0});
    cm.addComponent(localPlayer, Health{3,3});
    cm.addComponent(localPlayer, Sprite{playerTexture, playerTexture.width, playerTexture.height});

    // Systems
    RenderSystem renderSys;
    InputSystem inputSys;
    AudioSystem audioSys(beepSound);

    // Network
    NetworkSystem networkSystem(serverIP, serverPort, clientPort);
    cm.setGlobalTexture("player", playerTexture);
    cm.setGlobalTexture("remotePlayer", remotePlayerTexture);
    cm.setGlobalTexture("enemy", enemyTexture);
    cm.setGlobalTexture("bullet", bulletTexture);

    std::atomic<bool> stopNetThread(false);
    std::atomic<bool> hasGameStarted(false);

    // Start networking thread
    std::thread netThread(networkThreadFunc,
                          std::ref(networkSystem),
                          std::ref(em),
                          std::ref(cm),
                          std::ref(stopNetThread),
                          std::ref(hasGameStarted));

    GameScene scene = MENU;
    bool sentReady = false;

    while (!GraphicalLibrary::Window::ShouldCloseWindow()) {
        float dt = GetFrameTime();

        if (scene == MENU) {
            // Draw menu
            GraphicalLibrary::Window::StartDrawing();
            GraphicalLibrary::Window::ClearScreen(BLACK);
            DrawText("PRESS ENTER WHEN READY", 220, 280, 20, WHITE);

            uint8_t total, ready;
            networkSystem.getLobbyStatus(total, ready);
            std::string info = "Players: " + std::to_string(total) + "  Ready: " + std::to_string(ready);
            DrawText(info.c_str(), 270, 320, 20, WHITE);

            GraphicalLibrary::Window::StopDrawing();

            // If enter => send MSG_READY
            if (IsKeyPressed(KEY_ENTER) && !sentReady) {
                networkSystem.sendPacket(MSG_READY, nullptr, 0, true);
                sentReady = true;
            }

            // Move to game if server says so
            if (hasGameStarted.load()) {
                scene = GAME;
            }
        }
        else if (scene == GAME) {
            // Gather input
            inputSys.handleInput(em, cm);

            PlayerInputPayload inp;
            inp.netID = networkSystem.getLocalNetworkID();
            inp.up    = IsKeyDown(KEY_UP);
            inp.down  = IsKeyDown(KEY_DOWN);
            inp.left  = IsKeyDown(KEY_LEFT);
            inp.right = IsKeyDown(KEY_RIGHT);
            // Only press shoot once
            inp.shoot = IsKeyPressed(KEY_SPACE);

            networkSystem.sendPacket(MSG_PLAYER_INPUT, &inp, sizeof(inp), false);

            // Update local audio
            audioSys.update(dt, em, cm);

            // Render
            GraphicalLibrary::Window::StartDrawing();
            GraphicalLibrary::Window::ClearScreen(RAYWHITE);

            renderSys.update(dt, em, cm);

            // Show net stats
            float lat = networkSystem.getLatency();
            uint32_t loss = networkSystem.getPacketLoss();
            std::string latStr = "Latency: " + std::to_string((int)lat) + " ms";
            std::string losStr = "Packet Loss: " + std::to_string(loss);

            int lwLat  = MeasureText(latStr.c_str(), 20);
            int lwLoss = MeasureText(losStr.c_str(), 20);

            DrawText(latStr.c_str(), screenWidth - lwLat - 10, 10, 20, RED);
            DrawText(losStr.c_str(), screenWidth - lwLoss - 10, 40, 20, RED);

            GraphicalLibrary::Window::StopDrawing();
        }
    }

    // Cleanup
    stopNetThread.store(true);
    netThread.join();

    GraphicalLibrary::Audio::ReleaseAudio(beepSound);
    GraphicalLibrary::Audio::ShutdownAudioDevice();
    GraphicalLibrary::Texture::ReleaseTexture(playerTexture);
    GraphicalLibrary::Texture::ReleaseTexture(remotePlayerTexture);
    GraphicalLibrary::Texture::ReleaseTexture(enemyTexture);
    GraphicalLibrary::Texture::ReleaseTexture(bulletTexture);
    GraphicalLibrary::Window::Shutdown();

    return 0;
}
