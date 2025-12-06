// game.h
#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <iostream>

// =========================
// 1. Character（純虛擬基底）
// =========================

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

    // 純虛函式：讓這個 class 是 pure abstract
    virtual void attack(Character& target) = 0;
};

// =========================
// 2. Player（玩家）
// =========================

class Player : public Character {
private:
    int level;
    int experience;
    int x, y;   // 在主控台中的位置
    int score;

public:
    Player(const std::string& n);

    int getX() const;
    int getY() const;
    int getScore() const;

    void setPosition(int px, int py);

    void moveLeft();
    void moveRight(int width);          // 需要知道畫面寬度
    void draw(std::vector<std::string>& buffer) const;

    void gainScore(int s);

    // Operator overloading：經驗值累加
    Player& operator+=(int exp);

    // 之後如果要真的做戰鬥可以用 attack
    void attack(Character& target) override;
};

// 輸出 Player 狀態（Operator Overloading）
std::ostream& operator<<(std::ostream& os, const Player& p);

// =========================
// 3. Enemy（敵人／障礙物）
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

    // 預設行為：往下掉
    virtual void update();

    // 與玩家碰撞時的行為（Polymorphism）
    virtual void onCollision(Player& p) = 0;

    void attack(Character& target) override;
};

// 椰子樹葉：被打會扣血
class CoconutLeaf : public Enemy {
public:
    CoconutLeaf(int x, int y);
    void onCollision(Player& p) override;
};

// =========================
// 4. Item（物品）
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

    // 使用物品對玩家產生效果（Polymorphism）
    virtual void apply(Player& p) = 0;
};

// 雞排：只加分 + 經驗（不補血）
class ChickenCutlet : public Item {
public:
    ChickenCutlet(int x, int y);
    void apply(Player& p) override;
};

// 雨傘：放晴 + 小補血 + 加分（停雨邏輯在 Game 內處理）
class Umbrella : public Item {
public:
    Umbrella(int x, int y);
    void apply(Player& p) override;
};

// =========================
// 5. Template 工具函式
// =========================

template <typename T>
T clampValue(T v, T lo, T hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

// =========================
// 6. Game 類別
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
    int timeLimit;              // 總遊戲時間（秒），預設 30

    // 下雨時機控制
    int nextRainStart;          // 下一次開始下雨的時間（自遊戲開始起算的秒數）
    int rainEndTime;            // 這次下雨結束的時間（秒）
    int lastRainDamageSecond;   // 上一次扣血的秒數（讓它變成一秒扣一次）

public:
    Game(const std::string& playerName, int w = 40, int h = 20, int limitSec = 30);
    ~Game();

    void run();   // 主遊戲迴圈

private:
    void spawnObjects(int elapsedSec);
    void updateObjects();
    void handleCollisions(int elapsedSec);
    void removeInactive();
    void drawFrame(int remainingSec);
    void drawRain(std::vector<std::string>& buffer);
    void saveResult() const;  // File I/O + Exception Handling
};

#endif // GAME_H