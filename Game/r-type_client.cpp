// r-type_client.cpp

#include <iostream>
#include <filesystem>
#include <cstdlib>
#include <raylib.h>
#include <thread>
#include <chrono>
#include <atomic>

#include "Engine/GraphicalLibrary.hpp"        // All engine includes
#include "Game/Components/Components.hpp"     // Game components
#include "Game/Systems/Systems.hpp"           // Game systems
#include "Network/NetworkSystem.hpp"          // Network client system
#include "Network/Protocol.hpp"               // Message definitions

// Helper: Returns the full path for an asset.
std::string GetAssetPath(const std::string& assetName) {
    return (std::filesystem::current_path() / "assets" / assetName).string();
}

// Scene identifiers
enum GameScene { MENU, GAME };

//
// Network thread function that periodically updates the network system.
//
void networkThreadFunc(NetworkSystem &netSys,
                       Engine::EntityManager &em,
                       Engine::ComponentManager &cm,
                       std::atomic<bool> &stopFlag,
                       std::atomic<bool> &gameStartedFlag)
{
    // Run at roughly 60 FPS (16 ms per frame)
    while (!stopFlag.load()) {
        netSys.update(0.016f, em, cm);
        if (netSys.isGameStarted()) {
            gameStartedFlag.store(true);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

//
// Main
//
int main(int argc, char* argv[]) {
    // Expect three parameters: <serverIP> <serverPort> <clientPort>
    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <serverIP> <serverPort> <clientPort>\n";
        return 1;
    }
    std::string serverIP = argv[1];
    int serverPort = std::atoi(argv[2]);
    int clientPort = std::atoi(argv[3]);

    const int screenWidth  = 800;
    const int screenHeight = 600;

    // Initialize the engine window and audio.
    Engine::Window::Initialize(screenWidth, screenHeight, "R-Type Client");
    Engine::Audio::InitializeAudioDevice();

    // Load assets.
    Sound beepSound = Engine::Audio::LoadAudioFromFile(GetAssetPath("beep.wav"));
    Texture2D playerTexture      = Engine::Texture::LoadTextureFromFile(GetAssetPath("player.png"));
    Texture2D remotePlayerTex    = Engine::Texture::LoadTextureFromFile(GetAssetPath("player.png"));
    Texture2D enemyTexture       = Engine::Texture::LoadTextureFromFile(GetAssetPath("enemy.png"));
    Texture2D bulletTexture      = Engine::Texture::LoadTextureFromFile(GetAssetPath("bullet.png"));

    if (!playerTexture.id || !remotePlayerTex.id || !enemyTexture.id || !bulletTexture.id) {
        std::cerr << "[Error] Missing textures.\n";
        return -1;
    }

    // Set up ECS.
    Engine::EntityManager em;
    Engine::ComponentManager cm;

    // Create the local player.
    Engine::Entity localPlayer = em.createEntity();
    cm.addComponent(localPlayer, Position{100.f, 300.f});
    cm.addComponent(localPlayer, Velocity{0.f, 0.f});
    cm.addComponent(localPlayer, Health{3, 3});
    cm.addComponent(localPlayer, Sprite{playerTexture, playerTexture.width, playerTexture.height});
    cm.addComponent(localPlayer, KeyboardControl{});

    // Instantiate game systems.
    RenderSystem   renderSys;
    InputSystem    inputSys;
    AudioSystem    audioSys(beepSound);

    // Instantiate the network system.
    NetworkSystem networkSystem(serverIP, serverPort, clientPort);

    // Set global textures so that systems can use them.
    cm.setGlobalTexture("player",       playerTexture);
    cm.setGlobalTexture("remotePlayer", remotePlayerTex);
    cm.setGlobalTexture("enemy",        enemyTexture);
    cm.setGlobalTexture("bullet",       bulletTexture);

    // Launch the network thread.
    std::atomic<bool> stopNetThread(false);
    std::atomic<bool> hasGameStarted(false);
    std::thread netThread(networkThreadFunc,
                          std::ref(networkSystem),
                          std::ref(em),
                          std::ref(cm),
                          std::ref(stopNetThread),
                          std::ref(hasGameStarted));

    // The client starts in the MENU scene.
    GameScene scene = MENU;
    bool sentReady = false;

    // Main loop.
    while (!Engine::Window::ShouldCloseWindow()) {
        float dt = GetFrameTime();

        if (scene == MENU) {
            // Draw the lobby menu.
            Engine::Window::StartDrawing();
            Engine::Window::ClearScreen(BLACK);
            DrawText("PRESS ENTER WHEN READY", 220, 280, 20, WHITE);

            // Get lobby status (number of players and ready count) from the network system.
            uint8_t total, ready;
            networkSystem.getLobbyStatus(total, ready);
            std::string info = "Players: " + std::to_string(total) + "  Ready: " + std::to_string(ready);
            DrawText(info.c_str(), 270, 320, 20, WHITE);

            Engine::Window::StopDrawing();

            // On ENTER press, send MSG_READY once.
            if (IsKeyPressed(KEY_ENTER) && !sentReady) {
                networkSystem.sendPacket(MSG_READY, nullptr, 0, true);
                sentReady = true;
            }

            // Transition to GAME scene if the network system indicates the game has started.
            if (hasGameStarted.load()) {
                scene = GAME;
            }
        }
        else if (scene == GAME) {
            // Check the local player's health.
            auto health = cm.getComponent<Health>(localPlayer);

            // Only process input and shooting if the player is alive.
            if (health && health->current > 0) {
                inputSys.handleInput(em, cm);
                // Update input system (this may update velocity based on keyboard state).
                inputSys.update(dt, em, cm);
            }

            // Build a PlayerInputPayload and send it to the server.
            PlayerInputPayload inp;
            inp.netID = networkSystem.getLocalNetworkID();
            inp.up    = IsKeyDown(KEY_UP);
            inp.down  = IsKeyDown(KEY_DOWN);
            inp.left  = IsKeyDown(KEY_LEFT);
            inp.right = IsKeyDown(KEY_RIGHT);
            // Only send shoot command if health > 0.
            inp.shoot = (IsKeyPressed(KEY_SPACE) && (health && health->current > 0));

            networkSystem.sendPacket(MSG_PLAYER_INPUT, &inp, sizeof(inp), false);

            // Update audio.
            audioSys.update(dt, em, cm);

            // Render the game.
            Engine::Window::StartDrawing();
            Engine::Window::ClearScreen(RAYWHITE);

            renderSys.update(dt, em, cm);

            // Display network statistics.
            float lat = networkSystem.getLatency();
            uint32_t loss = networkSystem.getPacketLoss();
            std::string latStr = "Latency: " + std::to_string((int)lat) + " ms";
            std::string losStr = "Packet Loss: " + std::to_string(loss);
            int lwLat  = MeasureText(latStr.c_str(), 20);
            int lwLoss = MeasureText(losStr.c_str(), 20);
            DrawText(latStr.c_str(), 800 - lwLat - 10, 10, 20, RED);
            DrawText(losStr.c_str(), 800 - lwLoss - 10, 40, 20, RED);

            Engine::Window::StopDrawing();

            // If the local player's health is 0 (dead), transition back to the lobby.
            if (health && health->current <= 0) {
                std::cout << "Local player is dead. Returning to lobby...\n";
                scene = MENU;
                sentReady = false;
                // Reinitialize the local player.
                em.destroyEntity(localPlayer);
                localPlayer = em.createEntity();
                cm.addComponent(localPlayer, Position{100.f, 300.f});
                cm.addComponent(localPlayer, Velocity{0.f, 0.f});
                cm.addComponent(localPlayer, Health{3, 3});
                cm.addComponent(localPlayer, Sprite{cm.getGlobalTexture("player"), playerTexture.width, playerTexture.height});
                cm.addComponent(localPlayer, KeyboardControl{});
            }
        }
    }

    // Cleanup
    stopNetThread.store(true);
    netThread.join();

    Engine::Audio::ReleaseAudio(beepSound);
    Engine::Audio::ShutdownAudioDevice();
    Engine::Texture::ReleaseTexture(playerTexture);
    Engine::Texture::ReleaseTexture(remotePlayerTex);
    Engine::Texture::ReleaseTexture(enemyTexture);
    Engine::Texture::ReleaseTexture(bulletTexture);
    Engine::Window::Shutdown();

    return 0;
}
