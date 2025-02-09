#ifndef SNAKE_GAME_PLUGIN_HPP
#define SNAKE_GAME_PLUGIN_HPP

#include "IGame.hpp"
#include "SnakeTypes.hpp"
#include <unordered_map>
#include <vector>

class SnakeGamePlugin : public IGame {
public:
    SnakeGamePlugin();
    virtual ~SnakeGamePlugin() override;

    void onStart() override;
    void onPlayerInput(const PlayerInputPayload& input) override;
    void onUpdate(float dt) override;
    GameState getGameState() override;

private:
    void updateSnake(Snake &snake);
    void spawnFood();
    bool isCellOccupied(int x, int y) const;

    std::unordered_map<int, Snake> snakes;
    std::vector<Food> foods;
    float moveAccumulator;
    const float moveInterval;
    const int gridWidth;
    const int gridHeight;
    const int cellSize;
    int nextFoodID;
};

extern "C" {
    IGame* createGame();
}

#endif // SNAKE_GAME_PLUGIN_HPP
