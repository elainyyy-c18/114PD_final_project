// game.cpp
#include "game.h"

#include <conio.h>
#include <windows.h>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <stdexcept>

// ======= 螢幕工具 =======

static void clearScreen() {
    system("cls");
}

static void flashScreen() {
    HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
    for (int i = 0; i < 2; i++) {
        SetConsoleTextAttribute(h, BACKGROUND_RED | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        Sleep(50);
        SetConsoleTextAttribute(h, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        Sleep(50);
    }
}

static void hideCursor() {
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO cursorInfo;
    GetConsoleCursorInfo(out, &cursorInfo);
    cursorInfo.bVisible = false;
    SetConsoleCursorInfo(out, &cursorInfo);
}

// =====================
// Character 實作
// =====================

Character::Character(const std::string& n, int hp)
    : name(n), health(hp) {}

Character::~Character() {}

std::string Character::getName() const { return name; }
int Character::getHealth() const { return health; }
void Character::setHealth(int hp) { health = hp; }

// =====================
// Player 實作
// =====================

Player::Player(const std::string& n)
    : Character(n, 100), level(1), experience(0), x(0), y(0), score(0) {}

int Player::getX() const { return x; }
int Player::getY() const { return y; }
int Player::getScore() const { return score; }

void Player::setPosition(int px, int py) {
    x = px;
    y = py;
}

void Player::moveLeft() {
    if (x > 0) x--;
}

void Player::moveRight(int width) {
    if (x < width - 1) x++;
}

void Player::draw(std::vector<std::string>& buffer) const {
    if (y >= 0 && y < (int)buffer.size() &&
        x >= 0 && x < (int)buffer[0].size()) {
        buffer[y][x] = 'S';
    }
}

void Player::gainScore(int s) {
    score += s;
}

// 經驗值累加（Operator Overloading 範例）
Player& Player::operator+=(int exp) {
    experience += exp;
    // 簡單：每累積 50 經驗，就升級
    while (experience >= 50) {
        experience -= 50;
        level++;
    }
    return *this;
}

// 這邊只是示意，實際攻擊邏輯你可以自己加
void Player::attack(Character& target) {
    int newHp = target.getHealth() - 10;
    target.setHealth(newHp);
}

// 輸出 Player 狀態（給 File I/O 用）
std::ostream& operator<<(std::ostream& os, const Player& p) {
    os << "Player(" << p.getName() << ") HP=" << p.getHealth();
    return os;
}

// =====================
// Enemy 實作
// =====================

Enemy::Enemy(const std::string& n, int hp, int x, int y, char sym)
    : Character(n, hp), x(x), y(y), symbol(sym), active(true) {}

Enemy::~Enemy() {}

int Enemy::getX() const { return x; }
int Enemy::getY() const { return y; }
char Enemy::getSymbol() const { return symbol; }

void Enemy::setY(int ny) { y = ny; }

bool Enemy::isActive() const { return active; }
void Enemy::deactivate() { active = false; }

void Enemy::update() {
    y++;
}

void Enemy::attack(Character& target) {
    int newHp = target.getHealth() - 5;
    target.setHealth(newHp);
}

// 椰子樹葉：碰到扣血 + 螢幕閃爍
CoconutLeaf::CoconutLeaf(int x, int y)
    : Enemy("CoconutLeaf", 1, x, y, 'C') {}

void CoconutLeaf::onCollision(Player& p) {
    int hp = p.getHealth();
    hp -= 20;
    hp = clampValue(hp, 0, 100);
    p.setHealth(hp);
    deactivate();
    flashScreen();
}

// =====================
// Item 實作
// =====================

Item::Item(const std::string& n, int x, int y, char sym)
    : name(n), x(x), y(y), symbol(sym), active(true) {}

Item::~Item() {}

std::string Item::getName() const { return name; }
int Item::getX() const { return x; }
int Item::getY() const { return y; }
char Item::getSymbol() const { return symbol; }

void Item::setY(int ny) { y = ny; }
bool Item::isActive() const { return active; }
void Item::deactivate() { active = false; }

// 雞排：補血 + 加分 + 給經驗
ChickenCutlet::ChickenCutlet(int x, int y)
    : Item("ChickenCutlet", x, y, 'G') {}

void ChickenCutlet::apply(Player& p) {
    int hp = p.getHealth();
    hp += 15;
    hp = clampValue(hp, 0, 100);
    p.setHealth(hp);

    p.gainScore(10);
    p += 10; // 使用 operator+= 加經驗

    deactivate();
}

// 雨傘：放晴 + 小補血 + 加分
Umbrella::Umbrella(int x, int y)
    : Item("Umbrella", x, y, 'U') {}

void Umbrella::apply(Player& p) {
    int hp = p.getHealth();
    hp += 5;
    hp = clampValue(hp, 0, 100);
    p.setHealth(hp);

    p.gainScore(5);
    p += 5;

    deactivate();
}

// =====================
// Game 實作
// =====================

Game::Game(const std::string& playerName, int w, int h, int limitSec)
    : player(playerName),
      isRaining(false),
      gameOver(false),
      success(false),
      width(w),
      height(h),
      timeLimit(limitSec) {
    srand((unsigned)time(NULL));

    // 把玩家放在底部中央
    player.setPosition(width / 2, height - 1);
}

Game::~Game() {
    for (auto e : enemies) delete e;
    for (auto it : items) delete it;
}

void Game::spawnObjects(int elapsedSec) {
    // 椰子樹葉：比較常出現
    if (rand() % 8 == 0) {
        int x = rand() % width;
        enemies.push_back(new CoconutLeaf(x, 0));
    }

    // 雞排
    if (rand() % 20 == 0) {
        int x = rand() % width;
        items.push_back(new ChickenCutlet(x, 0));
    }

    // 雨傘：下雨時比較容易掉
    int umbrellaChance = isRaining ? 30 : 80;
    if (rand() % umbrellaChance == 0) {
        int x = rand() % width;
        items.push_back(new Umbrella(x, 0));
    }
}

void Game::updateObjects() {
    for (auto e : enemies) {
        if (!e->isActive()) continue;
        e->update();
        if (e->getY() >= height) e->deactivate();
    }

    for (auto it : items) {
        if (!it->isActive()) continue;
        int ny = it->getY() + 1;
        it->setY(ny);
        if (ny >= height) it->deactivate();
    }
}

void Game::handleCollisions() {
    int px = player.getX();
    int py = player.getY();

    // 跟敵人碰撞
    for (auto e : enemies) {
        if (!e->isActive()) continue;
        if (e->getX() == px && e->getY() == py) {
            e->onCollision(player);
        }
    }

    // 跟物品碰撞
    for (auto it : items) {
        if (!it->isActive()) continue;
        if (it->getX() == px && it->getY() == py) {
            it->apply(player);
        }
    }
}

void Game::removeInactive() {
    for (size_t i = 0; i < enemies.size();) {
        if (!enemies[i]->isActive()) {
            delete enemies[i];
            enemies.erase(enemies.begin() + i);
        } else {
            ++i;
        }
    }
    for (size_t i = 0; i < items.size();) {
        if (!items[i]->isActive()) {
            delete items[i];
            items.erase(items.begin() + i);
        } else {
            ++i;
        }
    }
}

void Game::drawRain(std::vector<std::string>& buffer) {
    for (int i = 0; i < width / 2; i++) {
        int rx = rand() % width;
        int ry = rand() % (height - 2);
        buffer[ry][rx] = '|';
    }
}

void Game::drawFrame(int remainingSec) {
    clearScreen();

    std::cout << "HP: " << std::setw(3) << player.getHealth()
              << "   Time Left: " << std::setw(3) << remainingSec
              << "   Score: " << std::setw(4) << player.getScore()
              << "   Weather: " << (isRaining ? "RAINING" : "SUNNY")
              << "\n";

    std::cout << std::string(width, '=') << "\n";

    std::vector<std::string> buffer(height, std::string(width, ' '));

    // 畫下雨效果
    if (isRaining) drawRain(buffer);

    // 畫敵人
    for (auto e : enemies) {
        if (!e->isActive()) continue;
        int ex = e->getX();
        int ey = e->getY();
        if (ey >= 0 && ey < height && ex >= 0 && ex < width) {
            buffer[ey][ex] = e->getSymbol();
        }
    }

    // 畫物品
    for (auto it : items) {
        if (!it->isActive()) continue;
        int ix = it->getX();
        int iy = it->getY();
        if (iy >= 0 && iy < height && ix >= 0 && ix < width) {
            buffer[iy][ix] = it->getSymbol();
        }
    }

    // 畫玩家
    player.draw(buffer);

    for (int i = 0; i < height; i++) {
        std::cout << buffer[i] << "\n";
    }
}

// 存檔案：results.txt（File I/O + Exception Handling）
void Game::saveResult() const {
    std::ofstream ofs("results.txt", std::ios::app);
    if (!ofs) {
        throw std::runtime_error("Cannot open results.txt");
    }
    ofs << (success ? "SUCCESS" : "GAMEOVER") << " "
        << player.getName() << " "
        << "HP=" << player.getHealth() << " "
        << "Score=" << player.getScore() << "\n";
}

// 主遊戲迴圈
void Game::run() {
    hideCursor();
    clearScreen();

    std::cout << "====== NTU Student Survival Game ======\n";
    std::cout << "左右方向鍵 或 A/D 控制學生\n";
    std::cout << "C = 椰子樹葉(扣血), G = 雞排(補血+加分), U = 雨傘(放晴)\n";
    std::cout << "請按任意鍵開始...\n";
    _getch();

    auto startTime = std::chrono::steady_clock::now();

    while (!gameOver && !success) {
        auto now = std::chrono::steady_clock::now();
        int elapsedSec = (int)std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        int remainingSec = timeLimit - elapsedSec;

        if (remainingSec <= 0) {
            success = true;
            break;
        }

        // 血量破防開始下雨
        if (!isRaining && player.getHealth() <= 50) {
            isRaining = true;
        }

        // 下雨持續扣血（慢慢扣）
        if (isRaining && elapsedSec % 2 == 0) {
            int hp = player.getHealth();
            hp -= 1;
            hp = clampValue(hp, 0, 100);
            player.setHealth(hp);
        }

        if (player.getHealth() <= 0) {
            gameOver = true;
            break;
        }

        // 處理鍵盤輸入
        if (_kbhit()) {
            int c = _getch();
            if (c == 0 || c == 224) {
                int c2 = _getch();
                if (c2 == 75) player.moveLeft();           // 左
                else if (c2 == 77) player.moveRight(width); // 右
            } else {
                if (c == 'a' || c == 'A') player.moveLeft();
                if (c == 'd' || c == 'D') player.moveRight(width);
            }
        }

        spawnObjects(elapsedSec);
        updateObjects();
        handleCollisions();
        removeInactive();
        drawFrame(remainingSec);

        Sleep(80);
    }

    // 存檔，有錯就印錯誤（Exception Handling）
    try {
        saveResult();
    } catch (const std::exception& e) {
        std::cerr << "存檔失敗: " << e.what() << "\n";
    }

    clearScreen();
    if (gameOver) {
        std::cout << "=========== GAME OVER ===========\n";
        std::cout << "你掛了 QQ\n";
        std::cout << "Final Score: " << player.getScore() << "\n";
    } else if (success) {
        std::cout << "=========== SUCCESS! ============\n";
        std::cout << "你在時間內活了下來！\n";
        std::cout << "Final Score: " << player.getScore() << "\n";
    }
    std::cout << "按任意鍵結束...\n";
    _getch();
}