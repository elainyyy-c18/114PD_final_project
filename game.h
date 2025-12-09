#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <iostream>

class Character {
protected:
    std::string name;
    int health;

public:
    Character(const std::string& n, int hp);
    virtual ~Character();

    std::string getName() const;
    int getHealth() const;
    void setHealth(int hp);

    virtual void attack(Character& target) = 0;
};

// =========================
// Player
// =========================

class Player : public Character {
private:
    int level;
    int experience;
    int x, y;
    int score;

public:
    Player(const std::string& n);

    int getX() const;
    int getY() const;
    int getScore() const;

    void setPosition(int px, int py);

    void moveLeft();
    void moveRight(int width);
    void draw(std::vector<std::string>& buffer) const;

    void gainScore(int s);
    Player& operator+=(int exp);

    void attack(Character& target) override;
};

std::ostream& operator<<(std::ostream& os, const Player& p);

// =========================
// Enemy
// =========================

class Enemy : public Character {
protected:
    int x, y;
    char symbol;
    bool active;

public:
    Enemy(const std::string& n, int hp, int x, int y, char sym);
    virtual ~Enemy();

    int getX() const;
    int getY() const;
    char getSymbol() const;

    void setY(int ny);
    bool isActive() const;
    void deactivate();

    virtual void update();
    virtual void onCollision(Player& p) = 0;

    void attack(Character& target) override;
};

class CoconutLeaf : public Enemy {
public:
    CoconutLeaf(int x, int y);
    void onCollision(Player& p) override;
};

// =========================
// Item
// =========================

class Item {
protected:
    std::string name;
    int x, y;
    char symbol;
    bool active;

public:
    Item(const std::string& n, int x, int y, char sym);
    virtual ~Item();

    std::string getName() const;
    int getX() const;
    int getY() const;
    char getSymbol() const;

    void setY(int ny);
    bool isActive() const;
    void deactivate();

    virtual void apply(Player& p) = 0;
};

class ChickenCutlet : public Item {
public:
    ChickenCutlet(int x, int y);
    void apply(Player& p) override;
};

class Umbrella : public Item {
public:
    Umbrella(int x, int y);
    void apply(Player& p) override;
};

// =========================
// clamp
// =========================

template <typename T>
T clampValue(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// =========================
// Game
// =========================

class Game {
private:
    Player player;
    std::vector<Enemy*> enemies;
    std::vector<Item*> items;

    bool isRaining;
    bool gameOver;
    bool success;

    int width;
    int height;
    int timeLimit;

    int nextRainStart;
    int rainEndTime;
    int lastRainDamageSecond;

public:
    Game(const std::string& playerName, int w = 40, int h = 20, int limitSec = 30);
    ~Game();

    void run();

private:
    void spawnObjects(int elapsedSec);
    void updateObjects();
    void handleCollisions(int elapsedSec);
    void removeInactive();
    void drawFrame(int remainingSec);
    void drawRain(std::vector<std::string>& buffer);
    void saveResult() const;
};

#endif // GAME_H
