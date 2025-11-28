// game.h
#ifndef GAME_H
#define GAME_H

#include <string>
#include <vector>
#include <iostream>
using namespace std;

// =========================
// 1. Character（純虛擬基底）
// =========================

class Character {
protected:
    string name;
    int health;

public:
    Character(const string& n, int hp);
    virtual ~Character();

    string getName() const;
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
    Player(const string& n);

    int getX() const;
    int getY() const;
    int getScore() const;

    void setPosition(int px, int py);

    void moveLeft();
    void moveRight(int width);          // 需要知道畫面寬度
    void draw(vector<string>& buffer) const;

    void gainScore(int s);

    // Operator overloading：經驗值累加
    Player& operator+=(int exp);

    // 之後如果要真的做戰鬥可以用 attack
    void attack(Character& target) override;
};

// 輸出 Player 狀態（Operator Overloading）
ostream& operator<<(ostream& os, const Player& p);

// =========================
// 3. Enemy（敵人／障礙物）
// =========================

class Enemy : public Character {
protected:
    int x, y;
    char symbol;
    bool active;

public:
    Enemy(const string& n, int hp, int x, int y, char sym);
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
    string name;
    int x, y;
    char symbol;
    bool active;

public:
    Item(const string& n, int x, int y, char sym);
    virtual ~Item();

    string getName() const;
    int getX() const;
    int getY() const;
    char getSymbol() const;

    void setY(int ny);
    bool isActive() const;
    void deactivate();

    // 使用物品對玩家產生效果（Polymorphism）
    virtual void apply(Player& p) = 0;
};

// 雞排：補血 + 加分 + 給一點經驗
class ChickenCutlet : public Item {
public:
    ChickenCutlet(int x, int y);
    void apply(Player& p) override;
};

// 雨傘：放晴 + 小補血 + 加分
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
    vector<Enemy*> enemies;
    vector<Item*> items;

    bool isRaining;
    bool gameOver;
    bool success;

    int width;
    int height;
    int timeLimit;

public:
    Game(const string& playerName, int w = 40, int h = 20, int limitSec = 60);
    ~Game();

    void run();   // 主遊戲迴圈

private:
    void spawnObjects(int elapsedSec);
    void updateObjects();
    void handleCollisions();
    void removeInactive();
    void drawFrame(int remainingSec);
    void drawRain(vector<string>& buffer);
    void saveResult() const;  // File I/O + Exception Handling
};

#endif // GAME_H