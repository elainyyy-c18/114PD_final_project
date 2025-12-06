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
    while (experience >= 50) {
        experience -= 50;
        level++;
    }
    return *this;
}

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

// 雞排：只加分 + 經驗（不補血）
ChickenCutlet::ChickenCutlet(int x, int y)
    : Item("ChickenCutlet", x, y, 'G') {}

void ChickenCutlet::apply(Player& p) {
    p.gainScore(10);
    p += 10;
    deactivate();
}

// 雨傘：只加分 + 經驗（停雨邏輯在 Game 裡做）
Umbrella::Umbrella(int x, int y)
    : Item("Umbrella", x, y, 'U') {}

void Umbrella::apply(Player& p) {
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
      timeLimit(limitSec),
      nextRainStart(0),
      rainEndTime(-1),          // 雖然不再用 timer 停雨，但保留欄位沒關係
      lastRainDamageSecond(-1) {

    srand((unsigned)time(NULL));

    // 玩家放在底部中央
    player.setPosition(width / 2, height - 1);

    // 第一場雨 10~20 秒之間
    nextRainStart = 10 + rand() % 11; // 10~20
}

Game::~Game() {
    for (auto e : enemies) delete e;
    for (auto it : items) delete it;
}

void Game::spawnObjects(int /*elapsedSec*/) {
    // 椰子樹葉：比較常出現
    if (rand() % 8 == 0) {
        int x = rand() % width;
        enemies.push_back(new CoconutLeaf(x, 0));
    }

    // 雞排：加分用
    if (rand() % 20 == 0) {
        int x = rand() % width;
        items.push_back(new ChickenCutlet(x, 0));
    }

    // ⭐ 下雨時：場上「一定」要有至少一把雨傘 U ⭐
    if (isRaining) {
        bool hasUmbrella = false;
        for (auto it : items) {
            if (it->isActive() && it->getSymbol() == 'U') {
                hasUmbrella = true;
                break;
            }
        }
        if (!hasUmbrella) {
            int x = rand() % width;
            items.push_back(new Umbrella(x, 0));
        }
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

void Game::handleCollisions(int elapsedSec) {
    int px = player.getX();
    int py = player.getY();

    // 敵人
    for (auto e : enemies) {
        if (!e->isActive()) continue;
        if (e->getX() == px && e->getY() == py) {
            e->onCollision(player);
        }
    }

    // 物品
    for (auto it : items) {
        if (!it->isActive()) continue;
        if (it->getX() == px && it->getY() == py) {
            bool isUmb = (it->getSymbol() == 'U');
            it->apply(player);

            // ⭐ 撿到雨傘：只有這個事件可以讓雨停 ⭐
            if (isUmb && isRaining) {
                isRaining = false;
                lastRainDamageSecond = -1;

                // 下一場雨一樣是 10~20 秒之後
                int remainingSec = timeLimit - elapsedSec;
                if (remainingSec > 0) {
                    nextRainStart = elapsedSec + 10 + rand() % 11;
                }
            }
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

// ★★★ 結束畫面也放在這裡：時間到 / HP 歸零 → 清螢幕 + 顯示結束頁 + exit(0)
void Game::drawFrame(int remainingSec) {
    if (remainingSec <= 0 || player.getHealth() <= 0) {
        clearScreen();   // 清掉遊戲畫面

        std::cout << "========================================\n";
        if (player.getHealth() <= 0) {
            std::cout << "=========== GAME OVER ===========\n";
            std::cout << "You died QQ\n";
        } else {
            std::cout << "=========== SUCCESS! ============\n";
            std::cout << "You survived until the time limit!\n";
        }
        std::cout << "Final Score: " << player.getScore() << "\n";
        std::cout << "========================================\n\n";
        std::cout << "Press any key to exit...\n";
        _getch();

        std::exit(0); // 直接結束程式（包含 main）
    }

    // ======= 正常遊戲畫面（還沒結束） =======
    clearScreen();

    std::cout << "HP: " << std::setw(3) << player.getHealth()
              << "   Time Left: " << std::setw(3) << remainingSec
              << "   Score: " << std::setw(4) << player.getScore()
              << "   Weather: " << (isRaining ? "RAINING" : "SUNNY")
              << "\n";

    std::cout << std::string(width, '=') << "\n";

    std::vector<std::string> buffer(height, std::string(width, ' '));

    if (isRaining) drawRain(buffer);

    for (auto e : enemies) {
        if (!e->isActive()) continue;
        int ex = e->getX();
        int ey = e->getY();
        if (ey >= 0 && ey < height && ex >= 0 && ex < width) {
            buffer[ey][ex] = e->getSymbol();
        }
    }

    for (auto it : items) {
        if (!it->isActive()) continue;
        int ix = it->getX();
        int iy = it->getY();
        if (iy >= 0 && iy < height && ix >= 0 && ix < width) {
            buffer[iy][ix] = it->getSymbol();
        }
    }

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
    std::cout << "Use LEFT/RIGHT arrow keys or A/D to move.\n";
    std::cout << "C = coconut leaf (damage)\n";
    std::cout << "G = chicken cutlet (score)\n";
    std::cout << "U = umbrella (stop rain + small score)\n";
    std::cout << "Game time: " << timeLimit << " seconds.\n";
    std::cout << "Press any key to start...\n";
    _getch();

    auto startTime = std::chrono::steady_clock::now();
    lastRainDamageSecond = -1;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        int elapsedSec = (int)std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        int remainingSec = timeLimit - elapsedSec;
        if (remainingSec < 0) remainingSec = 0;

        // ===== 控制下雨開始（每 10~20 秒一次） =====
        if (!isRaining && elapsedSec >= nextRainStart) {
            isRaining = true;
            lastRainDamageSecond = -1;
            // 不再用 rainEndTime：雨只會因為撿到 U 而停止
        }

        // ===== 下雨時每秒扣 1 HP =====
        if (isRaining && elapsedSec != lastRainDamageSecond) {
            int hp = player.getHealth();
            hp -= 5;
            hp = clampValue(hp, 0, 100);
            player.setHealth(hp);
            lastRainDamageSecond = elapsedSec;
        }

        // ===== 如果 HP 變 0，會在 drawFrame 直接跳出 GAME OVER 畫面 =====

        // ===== 處理鍵盤輸入 =====
        if (_kbhit()) {
            int c = _getch();
            if (c == 0 || c == 224) {
                int c2 = _getch();
                if (c2 == 75)      player.moveLeft();
                else if (c2 == 77) player.moveRight(width);
            } else {
                if (c == 'a' || c == 'A') player.moveLeft();
                if (c == 'd' || c == 'D') player.moveRight(width);
            }
        }

        // ===== 更新物件 & 畫面 =====
        spawnObjects(elapsedSec);
        updateObjects();
        handleCollisions(elapsedSec);
        removeInactive();
        drawFrame(remainingSec);   // 時間到 or HP 歸零，都在這裡跳到結束畫面

        Sleep(80);
    }
}
