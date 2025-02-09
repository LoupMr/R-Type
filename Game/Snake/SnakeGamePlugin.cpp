#include "SnakeGamePlugin.hpp"
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <algorithm>

SnakeGamePlugin::SnakeGamePlugin()
    : moveAccumulator(0.f),
      moveInterval(0.2f),
      gridWidth(40),
      gridHeight(30),
      cellSize(20),
      nextFoodID(1)
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
}

SnakeGamePlugin::~SnakeGamePlugin() {}

void SnakeGamePlugin::onStart() {
    snakes.clear();
    foods.clear();
    moveAccumulator = 0.f;
    nextFoodID = 1;
    for (int i = 0; i < 3; ++i) {
        spawnFood();
    }
}

void SnakeGamePlugin::onPlayerInput(const PlayerInputPayload& input) {
    if (snakes.find(input.netID) == snakes.end()) {
        Snake newSnake;
        newSnake.netID = input.netID;
        newSnake.score = 0;
        newSnake.currentDirection = (input.netID % 2 == 0) ? Direction::Right : Direction::Left;
        if (input.netID % 2 == 0)
            newSnake.body.push_back({ gridWidth / 4, gridHeight / 2 });
        else
            newSnake.body.push_back({ (3 * gridWidth) / 4, gridHeight / 2 });
        GridPosition head = newSnake.body.front();
        if (newSnake.currentDirection == Direction::Right) {
            newSnake.body.push_back({ head.x - 1, head.y });
            newSnake.body.push_back({ head.x - 2, head.y });
        } else if (newSnake.currentDirection == Direction::Left) {
            newSnake.body.push_back({ head.x + 1, head.y });
            newSnake.body.push_back({ head.x + 2, head.y });
        } else if (newSnake.currentDirection == Direction::Up) {
            newSnake.body.push_back({ head.x, head.y + 1 });
            newSnake.body.push_back({ head.x, head.y + 2 });
        } else if (newSnake.currentDirection == Direction::Down) {
            newSnake.body.push_back({ head.x, head.y - 1 });
            newSnake.body.push_back({ head.x, head.y - 2 });
        }
        snakes[input.netID] = newSnake;
    }
    Snake &snake = snakes[input.netID];
    Direction newDir = snake.currentDirection;
    if (input.up)
        newDir = Direction::Up;
    else if (input.right)
        newDir = Direction::Right;
    else if (input.down)
        newDir = Direction::Down;
    else if (input.left)
        newDir = Direction::Left;
    if (!((snake.currentDirection == Direction::Up && newDir == Direction::Down) ||
          (snake.currentDirection == Direction::Down && newDir == Direction::Up) ||
          (snake.currentDirection == Direction::Left && newDir == Direction::Right) ||
          (snake.currentDirection == Direction::Right && newDir == Direction::Left))) {
        snake.currentDirection = newDir;
    }
}

void SnakeGamePlugin::onUpdate(float dt) {
    moveAccumulator += dt;
    while (moveAccumulator >= moveInterval) {
        for (auto &pair : snakes) {
            updateSnake(pair.second);
        }
        moveAccumulator -= moveInterval;
    }
    if (foods.size() < 3)
        spawnFood();
}

GameState SnakeGamePlugin::getGameState() {
    GameState state{};
    std::memset(&state.payload, 0, sizeof(GameStatePayload));
    int playerCount = 0;
    for (const auto &pair : snakes) {
        if (playerCount >= 4) break;
        const Snake &snake = pair.second;
        state.payload.players[playerCount].playerID = snake.netID;
        state.payload.players[playerCount].x = snake.body.front().x * cellSize;
        state.payload.players[playerCount].y = snake.body.front().y * cellSize;
        state.payload.players[playerCount].health = snake.score;
        playerCount++;
    }
    state.payload.numPlayers = static_cast<uint8_t>(playerCount);
    int foodCount = 0;
    for (const auto &f : foods) {
        if (foodCount >= 32) break;
        state.payload.enemies[foodCount].enemyID = f.id;
        state.payload.enemies[foodCount].x = f.pos.x * cellSize;
        state.payload.enemies[foodCount].y = f.pos.y * cellSize;
        state.payload.enemies[foodCount].health = 1;
        foodCount++;
    }
    state.payload.numEnemies = static_cast<uint8_t>(foodCount);
    int segmentCount = 0;
    for (const auto &pair : snakes) {
        const Snake &snake = pair.second;
        for (size_t i = 1; i < snake.body.size(); ++i) {
            if (segmentCount >= 32) break;
            state.payload.bullets[segmentCount].bulletID = snake.netID * 1000 + static_cast<int>(i);
            state.payload.bullets[segmentCount].x = snake.body[i].x * cellSize;
            state.payload.bullets[segmentCount].y = snake.body[i].y * cellSize;
            state.payload.bullets[segmentCount].vx = 0.f;
            state.payload.bullets[segmentCount].vy = 0.f;
            state.payload.bullets[segmentCount].ownerID = snake.netID;
            segmentCount++;
        }
    }
    state.payload.numBullets = static_cast<uint8_t>(segmentCount);
    return state;
}

void SnakeGamePlugin::updateSnake(Snake &snake) {
    GridPosition head = snake.body.front();
    GridPosition newHead = head;
    switch (snake.currentDirection) {
        case Direction::Up:    newHead.y -= 1; break;
        case Direction::Right: newHead.x += 1; break;
        case Direction::Down:  newHead.y += 1; break;
        case Direction::Left:  newHead.x -= 1; break;
    }
    if (newHead.x < 0) newHead.x = gridWidth - 1;
    if (newHead.x >= gridWidth) newHead.x = 0;
    if (newHead.y < 0) newHead.y = gridHeight - 1;
    if (newHead.y >= gridHeight) newHead.y = 0;
    bool ateFood = false;
    for (size_t i = 0; i < foods.size(); ++i) {
        if (foods[i].pos.x == newHead.x && foods[i].pos.y == newHead.y) {
            ateFood = true;
            snake.score += 1;
            foods.erase(foods.begin() + i);
            break;
        }
    }
    snake.body.insert(snake.body.begin(), newHead);
    if (!ateFood)
        snake.body.pop_back();
}

void SnakeGamePlugin::spawnFood() {
    Food newFood;
    newFood.id = nextFoodID++;
    while (true) {
        int x = std::rand() % gridWidth;
        int y = std::rand() % gridHeight;
        if (!isCellOccupied(x, y)) {
            newFood.pos = { x, y };
            break;
        }
    }
    foods.push_back(newFood);
}

bool SnakeGamePlugin::isCellOccupied(int x, int y) const {
    for (const auto &pair : snakes) {
        for (const auto &seg : pair.second.body) {
            if (seg.x == x && seg.y == y)
                return true;
        }
    }
    for (const auto &f : foods) {
        if (f.pos.x == x && f.pos.y == y)
            return true;
    }
    return false;
}

extern "C" {
    IGame* createGame() {
        return new SnakeGamePlugin();
    }
}
