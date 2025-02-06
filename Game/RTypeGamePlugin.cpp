// File: RTypeGamePlugin.cpp

#include "IGame.hpp"
#include <unordered_map>
#include <vector>
#include <cstring>
#include <cmath>

// Internal data structures for simulation:
struct Player {
    int32_t id;
    float x;
    float y;
    int32_t health;
};

struct Enemy {
    int32_t enemyID;
    float x, y;
    float vx;
    int32_t health;
    bool active;
    float shootTimer;
};

struct Bullet {
    int32_t bulletID;
    float x, y;
    float vx, vy;
    int32_t ownerID; // >= 0: player bullet, < 0: enemy bullet
    bool active;
};

class RTypeGamePlugin : public IGame {
public:
    RTypeGamePlugin()
    : waveTimer(0.0f), waveIndex(0),
      spawnInterval(10.0f), spawnCount(3),
      enemySpeed(-50.0f), enemyShootTime(2.0f), bulletSpeed(200.f),
      nextEnemyID(1), nextBulletID(1000)
    {
    }

    virtual ~RTypeGamePlugin() {}

    // Called once when the game starts (e.g. when the first client is ready).
    void onStart() override {
        // Reset the entire game state.
        players.clear();
        enemies.clear();
        bullets.clear();
        waveTimer = 0.0f;
        waveIndex = 0;
        nextEnemyID = 1;
        nextBulletID = 1000;
    }

    // Called when a client sends input.
    void onPlayerInput(const PlayerInputPayload& input) override {
        // If the player doesn't exist yet, create a new one.
        if (players.find(input.netID) == players.end()) {
            Player p;
            p.id = input.netID;
            p.x = 400.0f;  // starting x
            p.y = 300.0f;  // starting y
            p.health = 3;
            players[input.netID] = p;
        }
        // Update the player's position based on input.
        Player& p = players[input.netID];
        const float moveSpeed = 150.0f * 0.09f; // approximate movement step (adjust as needed)
        if (input.up)    p.y -= moveSpeed;
        if (input.down)  p.y += moveSpeed;
        if (input.left)  p.x -= moveSpeed;
        if (input.right) p.x += moveSpeed;
        if (input.shoot) {
            // Spawn a player bullet.
            Bullet b;
            b.bulletID = nextBulletID++;
            b.x = p.x;
            b.y = p.y;
            b.vx = bulletSpeed;
            b.vy = 0.f;
            b.ownerID = p.id;
            b.active = true;
            bullets.push_back(b);
        }
    }

    // Called periodically to update the game simulation.
    void onUpdate(float dt) override {
        // Update the wave timer; if enough time has passed, spawn a new wave.
        waveTimer += dt;
        if (waveTimer >= spawnInterval) {
            waveTimer = 0.f;
            spawnWave();
        }

        // Update enemies.
        for (auto& enemy : enemies) {
            if (!enemy.active) continue;
            enemy.x += enemy.vx * dt;
            if (enemy.x < -100.f) {
                enemy.active = false;
            }
            enemy.shootTimer -= dt;
            if (enemy.shootTimer <= 0.f && enemy.active) {
                // Enemy shoots: spawn an enemy bullet.
                Bullet b;
                b.bulletID = nextBulletID++;
                b.x = enemy.x;
                b.y = enemy.y;
                b.vx = -bulletSpeed;
                b.vy = 0.f;
                b.ownerID = -enemy.enemyID; // negative to denote enemy bullet
                b.active = true;
                bullets.push_back(b);
                enemy.shootTimer = enemyShootTime;
            }
        }

        // Update bullets.
        for (auto& b : bullets) {
            if (!b.active) continue;
            b.x += b.vx * dt;
            b.y += b.vy * dt;
            if (b.x < -50.f || b.x > 850.f || b.y < 0.f || b.y > 600.f) {
                b.active = false;
            } else {
                // Check for collisions.
                if (b.ownerID >= 0) {
                    // Player bullet: check collision with enemies.
                    for (auto& enemy : enemies) {
                        if (!enemy.active) continue;
                        if (checkCollision(b.x, b.y, enemy.x, enemy.y)) {
                            enemy.health--;
                            if (enemy.health <= 0) {
                                enemy.active = false;
                            }
                            b.active = false;
                            break;
                        }
                    }
                } else {
                    // Enemy bullet: check collision with players.
                    for (auto& kv : players) {
                        Player& p = kv.second;
                        if (p.health <= 0) continue;
                        if (checkCollision(b.x, b.y, p.x, p.y)) {
                            p.health--;
                            b.active = false;
                            break;
                        }
                    }
                }
            }
        }
        // (Optionally, you can periodically remove inactive bullets and enemies.)
    }

    // Returns the current game state to be sent to clients.
    GameState getGameState() override {
        GameState state;
        // Zero out the payload.
        std::memset(&state.payload, 0, sizeof(GameStatePayload));

        // Fill player information (up to 4 players).
        int pCount = 0;
        for (const auto& kv : players) {
            if (pCount >= 4) break;
            const Player& p = kv.second;
            if (p.health > 0) {  // Only include living players.
                state.payload.players[pCount].playerID = p.id;
                state.payload.players[pCount].x = p.x;
                state.payload.players[pCount].y = p.y;
                state.payload.players[pCount].health = p.health;
                pCount++;
            }
        }
        state.payload.numPlayers = static_cast<uint8_t>(pCount);

        // Fill enemy information (up to 32 enemies).
        int eCount = 0;
        for (const auto& enemy : enemies) {
            if (!enemy.active) continue;
            if (eCount >= 32) break;
            state.payload.enemies[eCount].enemyID = enemy.enemyID;
            state.payload.enemies[eCount].x = enemy.x;
            state.payload.enemies[eCount].y = enemy.y;
            state.payload.enemies[eCount].health = enemy.health;
            eCount++;
        }
        state.payload.numEnemies = static_cast<uint8_t>(eCount);

        // Fill bullet information (up to 32 bullets).
        int bCount = 0;
        for (const auto& b : bullets) {
            if (!b.active) continue;
            if (bCount >= 32) break;
            state.payload.bullets[bCount].bulletID = b.bulletID;
            state.payload.bullets[bCount].x = b.x;
            state.payload.bullets[bCount].y = b.y;
            state.payload.bullets[bCount].vx = b.vx;
            state.payload.bullets[bCount].vy = b.vy;
            state.payload.bullets[bCount].ownerID = b.ownerID;
            bCount++;
        }
        state.payload.numBullets = static_cast<uint8_t>(bCount);

        return state;
    }

private:
    // Spawns a wave of enemies.
    void spawnWave() {
        for (int i = 0; i < spawnCount; i++) {
            Enemy e;
            e.enemyID = nextEnemyID++;
            e.x = 850.f;
            e.y = 100.f + i * 80.f;
            e.vx = enemySpeed;
            e.health = 3;
            e.active = true;
            e.shootTimer = enemyShootTime;
            enemies.push_back(e);
        }
        waveIndex++;
    }

    // Simple circular collision check.
    bool checkCollision(float x1, float y1, float x2, float y2, float radius = 20.f) {
        float dx = x2 - x1;
        float dy = y2 - y1;
        return (dx * dx + dy * dy) < (radius * radius);
    }

    // Internal simulation state.
    std::unordered_map<int32_t, Player> players;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;

    // Wave and simulation parameters.
    float waveTimer;
    int waveIndex;
    const float spawnInterval;    // e.g., 10 seconds between waves
    const int spawnCount;         // e.g., spawn 3 enemies per wave
    const float enemySpeed;       // e.g., -50 (moving left)
    const float enemyShootTime;   // e.g., enemy fires every 2 seconds
    const float bulletSpeed;      // e.g., 200 pixels/second

    int32_t nextEnemyID;
    int32_t nextBulletID;
};

// The shared library must expose a C-style factory function.
extern "C" {
    IGame* createGame() {
        return new RTypeGamePlugin();
    }
}
