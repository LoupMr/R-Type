#include <iostream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdlib>

#include <raylib.h>

#include "Engine/GraphicalLibrary.hpp"
#include "Game/Components/Components.hpp"
#include "Game/Systems/Systems.hpp"
#include "Network/System/NetworkSystem.hpp"
#include "Network/Protocol/Protocol.hpp"

static std::string GetAssetPath(const std::string &assetName) {
    return (std::filesystem::current_path() / "assets" / assetName).string();
}

enum class GameScene {
    MENU,
    GAME
};

void networkThreadFunc(NetworkSystem &netSys,
                       Engine::EntityManager &em,
                       Engine::ComponentManager &cm,
                       std::atomic<bool> &stopFlag,
                       std::atomic<bool> &gameStartedFlag)
{
    constexpr auto frameDuration = std::chrono::milliseconds(16);
    while (!stopFlag.load()) {
        netSys.update(0.016f, em, cm);
        if (netSys.isGameStarted()) {
            gameStartedFlag.store(true);
        }
        std::this_thread::sleep_for(frameDuration);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <host> <server-port> <client-port>\n";
        return EXIT_FAILURE;
    }
    std::string serverIP = argv[1];
    int serverPort = std::stoi(argv[2]);
    int clientPort = std::stoi(argv[3]);

    constexpr int screenWidth  = 800;
    constexpr int screenHeight = 600;
    const std::string windowTitle = "Snake Game Client";

    const std::string playerAsset       = "player.png";
    const std::string remotePlayerAsset = "player.png";
    const std::string enemyAsset        = "enemy.png";
    const std::string bulletAsset       = "bullet.png";
    const std::string beepAsset         = "beep.wav";

    Engine::Window::Initialize(screenWidth, screenHeight, windowTitle);
    Engine::Audio::InitializeAudioDevice();

    Sound beepSound = Engine::Audio::LoadAudioFromFile(GetAssetPath(beepAsset));
    Texture2D playerTexture       = Engine::Texture::LoadTextureFromFile(GetAssetPath(playerAsset));
    Texture2D remotePlayerTexture = Engine::Texture::LoadTextureFromFile(GetAssetPath(remotePlayerAsset));
    Texture2D enemyTexture        = Engine::Texture::LoadTextureFromFile(GetAssetPath(enemyAsset));
    Texture2D bulletTexture       = Engine::Texture::LoadTextureFromFile(GetAssetPath(bulletAsset));

    if (!playerTexture.id || !remotePlayerTexture.id || !enemyTexture.id || !bulletTexture.id) {
        std::cerr << "[Error] Missing textures.\n";
        return EXIT_FAILURE;
    }

    Engine::EntityManager entityManager;
    Engine::ComponentManager componentManager;

    Engine::Entity localPlayer = entityManager.createEntity();
    componentManager.addComponent(localPlayer, Position{100.f, 300.f});
    componentManager.addComponent(localPlayer, Velocity{0.f, 0.f});
    componentManager.addComponent(localPlayer, Health{3, 3});
    componentManager.addComponent(localPlayer, Sprite{playerTexture, playerTexture.width, playerTexture.height});
    componentManager.addComponent(localPlayer, KeyboardControl{});

    RenderSystem renderSystem;
    InputSystem inputSystem;
    AudioSystem audioSystem(beepSound);
    NetworkSystem networkSystem(serverIP, serverPort, clientPort);

    componentManager.setGlobalTexture("player", playerTexture);
    componentManager.setGlobalTexture("remotePlayer", remotePlayerTexture);
    componentManager.setGlobalTexture("enemy", enemyTexture);
    componentManager.setGlobalTexture("bullet", bulletTexture);

    std::atomic<bool> stopNetThread{false};
    std::atomic<bool> hasGameStarted{false};
    std::thread netThread(networkThreadFunc,
                          std::ref(networkSystem),
                          std::ref(entityManager),
                          std::ref(componentManager),
                          std::ref(stopNetThread),
                          std::ref(hasGameStarted));

    GameScene scene = GameScene::MENU;
    bool sentReady = false;
    while (!Engine::Window::ShouldCloseWindow()) {
        float dt = GetFrameTime();

        if (scene == GameScene::MENU) {
            Engine::Window::StartDrawing();
            Engine::Window::ClearScreen(BLACK);
            DrawText("PRESS ENTER WHEN READY", 220, 280, 20, WHITE);
            uint8_t total = 0, ready = 0;
            networkSystem.getLobbyStatus(total, ready);
            std::string info = "Players: " + std::to_string(total) + "  Ready: " + std::to_string(ready);
            DrawText(info.c_str(), 270, 320, 20, WHITE);
            Engine::Window::StopDrawing();

            if (IsKeyPressed(KEY_ENTER) && !sentReady) {
                networkSystem.sendPacket(static_cast<uint8_t>(MessageType::READY), nullptr, 0, true);
                sentReady = true;
            }
            if (hasGameStarted.load()) {
                scene = GameScene::GAME;
            }
        }
        else if (scene == GameScene::GAME) {
            if (auto health = componentManager.getComponent<Health>(localPlayer)) {
                if (health->current > 1) {
                    inputSystem.handleInput(entityManager, componentManager);
                    inputSystem.update(dt, entityManager, componentManager);
                }
            }
            PlayerInputPayload inputPayload{};
            inputPayload.netID = networkSystem.getLocalNetworkID();
            inputPayload.up    = IsKeyDown(KEY_UP);
            inputPayload.down  = IsKeyDown(KEY_DOWN);
            inputPayload.left  = IsKeyDown(KEY_LEFT);
            inputPayload.right = IsKeyDown(KEY_RIGHT);
            inputPayload.shoot = false;
            networkSystem.sendPacket(static_cast<uint8_t>(MessageType::PLAYER_INPUT), &inputPayload, sizeof(inputPayload), false);

            audioSystem.update(dt, entityManager, componentManager);

            Engine::Window::StartDrawing();
            Engine::Window::ClearScreen(RAYWHITE);
            renderSystem.update(dt, entityManager, componentManager);
            float latency = networkSystem.getLatency();
            uint32_t packetLoss = networkSystem.getPacketLoss();
            std::string latencyStr = "Latency: " + std::to_string(static_cast<int>(latency)) + " ms";
            std::string packetLossStr = "Packet Loss: " + std::to_string(packetLoss);
            int textWidthLatency = MeasureText(latencyStr.c_str(), 20);
            int textWidthPacketLoss = MeasureText(packetLossStr.c_str(), 20);
            DrawText(latencyStr.c_str(), screenWidth - textWidthLatency - 10, 10, 20, RED);
            DrawText(packetLossStr.c_str(), screenWidth - textWidthPacketLoss - 10, 40, 20, RED);
            Engine::Window::StopDrawing();

            if (auto health = componentManager.getComponent<Health>(localPlayer)) {
                if (health->current <= -15) {
                    std::cout << "Local player is dead. Returning to lobby...\n";
                    scene = GameScene::MENU;
                    sentReady = false;
                    entityManager.destroyEntity(localPlayer);
                    localPlayer = entityManager.createEntity();
                    componentManager.addComponent(localPlayer, Position{100.f, 300.f});
                    componentManager.addComponent(localPlayer, Velocity{0.f, 0.f});
                    componentManager.addComponent(localPlayer, Health{3, 3});
                    componentManager.addComponent(localPlayer, Sprite{componentManager.getGlobalTexture("player"),
                                                                      playerTexture.width, playerTexture.height});
                    componentManager.addComponent(localPlayer, KeyboardControl{});
                }
            }
        }
    }

    stopNetThread.store(true);
    if (netThread.joinable())
        netThread.join();

    Engine::Audio::ReleaseAudio(beepSound);
    Engine::Audio::ShutdownAudioDevice();
    Engine::Texture::ReleaseTexture(playerTexture);
    Engine::Texture::ReleaseTexture(remotePlayerTexture);
    Engine::Texture::ReleaseTexture(enemyTexture);
    Engine::Texture::ReleaseTexture(bulletTexture);
    Engine::Window::Shutdown();

    return EXIT_SUCCESS;
}
