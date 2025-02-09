#include "RTypeGamePlugin.hpp"
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <iostream>

RTypeGamePlugin::RTypeGamePlugin()
    : waveTimer(0.0f), waveIndex(0),
      spawnInterval(8.0f), spawnCount(5),
      baseEnemySpeed(-50.0f), baseEnemyShootTime(2.0f), bulletSpeed(200.f),
      nextEnemyID(1), nextBulletID(1000)
{
}

RTypeGamePlugin::~RTypeGamePlugin() {}

void RTypeGamePlugin::onStart() {
    players.clear();
    enemies.clear();
    bullets.clear();
    waveTimer = 0.0f;
    waveIndex = 0;
    nextEnemyID = 1;
    nextBulletID = 1000;
}

void RTypeGamePlugin::onPlayerInput(const PlayerInputPayload& input) {
    if (players.find(input.netID) == players.end()) {
        Player p;
        p.id = input.netID;
        p.x = 400.0f;
        p.y = 300.0f;
        p.health = 3;
        players[input.netID] = p;
    }
    Player& p = players[input.netID];
    constexpr float moveSpeed = 150.0f * 0.09f;
    if (input.up) p.y -= moveSpeed;
    if (input.down) p.y += moveSpeed;
    if (input.left) p.x -= moveSpeed;
    if (input.right) p.x += moveSpeed;
    if (input.shoot) {
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

void RTypeGamePlugin::onUpdate(float dt) {
    waveTimer += dt;
    if (waveTimer >= spawnInterval) {
        waveTimer = 0.f;
        spawnWave();
    }
    for (auto& enemy : enemies) {
        if (!enemy.active) continue;
        enemy.patternTimer += dt;
        float verticalOffset = 0.f;
        switch(enemy.type) {
            case EnemyType::Normal:
                verticalOffset = 20.0f * sinf(enemy.patternTimer * 2.0f);
                break;
            case EnemyType::Fast:
                verticalOffset = 30.0f * sinf(enemy.patternTimer * 3.0f);
                break;
            case EnemyType::Strong:
                verticalOffset = 10.0f * sinf(enemy.patternTimer * 1.5f);
                break;
        }
        enemy.x += enemy.vx * dt;
        enemy.y = enemy.baseY + verticalOffset;
        if (enemy.x < -100.f)
            enemy.active = false;
        enemy.shootTimer -= dt;
        if (enemy.shootTimer <= 0.f && enemy.active) {
            Bullet b;
            b.bulletID = nextBulletID++;
            b.x = enemy.x;
            b.y = enemy.y;
            float enemyBulletSpeed = bulletSpeed;
            if (enemy.type == EnemyType::Fast)
                enemyBulletSpeed = bulletSpeed * 1.2f;
            else if (enemy.type == EnemyType::Strong)
                enemyBulletSpeed = bulletSpeed * 0.8f;
            b.vx = -enemyBulletSpeed;
            b.vy = 0.f;
            b.ownerID = -enemy.enemyID;
            b.active = true;
            bullets.push_back(b);
            if (enemy.type == EnemyType::Normal)
                enemy.shootTimer = baseEnemyShootTime;
            else if (enemy.type == EnemyType::Fast)
                enemy.shootTimer = baseEnemyShootTime * 0.7f;
            else if (enemy.type == EnemyType::Strong)
                enemy.shootTimer = baseEnemyShootTime * 1.2f;
        }
    }
    for (auto& b : bullets) {
        if (!b.active) continue;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        if (b.x < -50.f || b.x > 850.f || b.y < 0.f || b.y > 600.f) {
            b.active = false;
        } else {
            if (b.ownerID >= 0) {
                for (auto& enemy : enemies) {
                    if (!enemy.active) continue;
                    if (checkCollision(b.x, b.y, enemy.x, enemy.y)) {
                        enemy.health--;
                        if (enemy.health <= 0)
                            enemy.active = false;
                        b.active = false;
                        break;
                    }
                }
            } else {
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
    bool anyAlive = false;
    for (const auto& kv : players) {
        if (kv.second.health > 0) {
            anyAlive = true;
            break;
        }
    }
    if (!anyAlive) {
        std::cout << "[Plugin] All players dead. Resetting game state.\n";
        onStart();
    }
}

GameState RTypeGamePlugin::getGameState() {
    GameState state{};
    std::memset(&state.payload, 0, sizeof(GameStatePayload));
    int pCount = 0;
    for (const auto& kv : players) {
        if (pCount >= 4) break;
        const Player& p = kv.second;
        state.payload.players[pCount].playerID = p.id;
        state.payload.players[pCount].x = p.x;
        state.payload.players[pCount].y = p.y;
        state.payload.players[pCount].health = (p.health < 0 ? 0 : p.health);
        pCount++;
    }
    state.payload.numPlayers = static_cast<uint8_t>(pCount);
    int eCount = 0;
    for (const auto& enemy : enemies) {
        if (!enemy.active) continue;
        if (eCount >= 32) break;
        state.payload.enemies[eCount].enemyID = enemy.enemyID;
        state.payload.enemies[eCount].x = enemy.x;
        state.payload.enemies[eCount].y = enemy.y;
        state.payload.enemies[eCount].health = enemy.health;
        state.payload.enemies[eCount].type = static_cast<uint8_t>(enemy.type);
        eCount++;
    }
    state.payload.numEnemies = static_cast<uint8_t>(eCount);
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

void RTypeGamePlugin::spawnWave() {
    Enemy boss;
    boss.enemyID = nextEnemyID++;
    boss.x = 850.f;
    boss.baseY = 200.f;
    boss.y = boss.baseY;
    boss.type = EnemyType::Strong;
    boss.vx = baseEnemySpeed * 0.5f;
    boss.health = 10;
    boss.shootTimer = baseEnemyShootTime * 1.5f;
    boss.active = true;
    boss.patternTimer = 0.f;
    enemies.push_back(boss);
    for (int i = 0; i < spawnCount; ++i) {
        Enemy e;
        e.enemyID = nextEnemyID++;
        e.x = 850.f;
        e.baseY = 100.f + i * 80.f;
        e.y = e.baseY;
        int typeSelector = rand() % 2;
        if (typeSelector == 0) {
            e.type = EnemyType::Normal;
            e.vx = baseEnemySpeed;
            e.health = 3;
            e.shootTimer = baseEnemyShootTime;
        } else {
            e.type = EnemyType::Fast;
            e.vx = baseEnemySpeed * 1.5f;
            e.health = 2;
            e.shootTimer = baseEnemyShootTime * 0.7f;
        }
        e.active = true;
        e.patternTimer = 0.f;
        enemies.push_back(e);
    }
    waveIndex++;
}

bool RTypeGamePlugin::checkCollision(float x1, float y1, float x2, float y2, float radius) const {
    float dx = x2 - x1;
    float dy = y2 - y1;
    return (dx * dx + dy * dy) < (radius * radius);
}

extern "C" {
    IGame* createGame() {
        return new RTypeGamePlugin();
    }
}
