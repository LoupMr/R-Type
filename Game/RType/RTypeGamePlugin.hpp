#ifndef RTYPE_GAME_PLUGIN_HPP
#define RTYPE_GAME_PLUGIN_HPP

#include "IGame.hpp"
#include "RTypeTypes.hpp"
#include <unordered_map>
#include <vector>

class RTypeGamePlugin : public IGame {
public:
    RTypeGamePlugin();
    virtual ~RTypeGamePlugin() override;
    void onStart() override;
    void onPlayerInput(const PlayerInputPayload& input) override;
    void onUpdate(float dt) override;
    GameState getGameState() override;
private:
    void spawnWave();
    bool checkCollision(float x1, float y1, float x2, float y2, float radius = 20.f) const;
    std::unordered_map<int32_t, Player> players;
    std::vector<Enemy> enemies;
    std::vector<Bullet> bullets;
    float waveTimer;
    int waveIndex;
    const float spawnInterval;
    const int spawnCount;
    const float baseEnemySpeed;
    const float baseEnemyShootTime;
    const float bulletSpeed;
    int32_t nextEnemyID;
    int32_t nextBulletID;
};

extern "C" {
    IGame* createGame();
}

#endif // RTYPE_GAME_PLUGIN_HPP
