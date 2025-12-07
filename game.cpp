// =============================================
// ===============  game.cpp  ==================
// ====== Double Buffer (No Flickering) ========
// =============================================
// test change


#include "game.h"

#include <conio.h>
#include <windows.h>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <sstream>   // ★新增：因為 drawFrame 使用 ostringstream
#include <stdexcept>

// ======= 螢幕工具 =======

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

// 椰子樹葉
CoconutLeaf::CoconutLeaf(int x, int y)
    : Enemy("CoconutLeaf", 1, x, y, 'C') {}

void CoconutLeaf::onCollision(Player& p) {
    int hp = p.getHealth();
    hp -= 20;
    if (hp < 0) hp = 0;
    if (hp > 100) hp = 100;
    p.setHealth(hp);

    deactivate();
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

ChickenCutlet::ChickenCutlet(int x, int y)
    : Item("ChickenCutlet", x, y, 'G') {}

void ChickenCutlet::apply(Player& p) {
    int hp = p.getHealth();
    hp += 15;
    if (hp > 100) hp = 100;
    p.setHealth(hp);

    p.gainScore(10);
    p += 10;

    deactivate();
}

Umbrella::Umbrella(int x, int y)
    : Item("Umbrella", x, y, 'U') {}

void Umbrella::apply(Player& p) {
    int hp = p.getHealth();
    hp += 5;
    if (hp > 100) hp = 100;
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
      timeLimit(limitSec)
{
    srand((unsigned)time(NULL));

    player.setPosition(width / 2, height - 1);

    // ============================================
    // ★新增：建立雙緩衝 Console Screen Buffer
    // ============================================
    screenBuffer[0] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    screenBuffer[1] = CreateConsoleScreenBuffer(
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        CONSOLE_TEXTMODE_BUFFER,
        NULL
    );

    activeBuffer = 0;

    SetConsoleActiveScreenBuffer(screenBuffer[activeBuffer]);
}

// ★新增：翻面（swap buffer）
void Game::flipBuffer() {
    activeBuffer = 1 - activeBuffer;
    SetConsoleActiveScreenBuffer(screenBuffer[activeBuffer]);
}

Game::~Game() {
    for (auto e : enemies) delete e;
    for (auto it : items) delete it;
}

// 產生敵人與物品
void Game::spawnObjects(int elapsedSec) {
    if (rand() % 8 == 0) {
        int x = rand() % width;
        enemies.push_back(new CoconutLeaf(x, 0));
    }

    if (rand() % 20 == 0) {
        int x = rand() % width;
        items.push_back(new ChickenCutlet(x, 0));
    }

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

    for (auto e : enemies) {
        if (!e->isActive()) continue;
        if (e->getX() == px && e->getY() == py) {
            e->onCollision(player);
        }
    }

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

// ========================================================
// ★★★  Double Buffer 版 drawFrame（核心） ★★★
// ========================================================

void Game::drawFrame(int remainingSec) {

    // 1. 組裝畫面到字串
    std::ostringstream oss;

    oss << "HP: " << std::setw(3) << player.getHealth()
        << "   Time Left: " << std::setw(3) << remainingSec
        << "   Score: " << std::setw(4) << player.getScore()
        << "   Weather: " << (isRaining ? "RAINING" : "SUNNY")
        << "\n";

    oss << std::string(width, '=') << "\n";

    std::vector<std::string> buffer(height, std::string(width, ' '));

    if (isRaining) drawRain(buffer);

    for (auto e : enemies) {
        if (!e->isActive()) continue;
        int ex = e->getX();
        int ey = e->getY();
        if (ey>=0 && ey<height && ex>=0 && ex<width)
            buffer[ey][ex] = e->getSymbol();
    }

    for (auto it : items) {
        if (!it->isActive()) continue;
        int ix = it->getX();
        int iy = it->getY();
        if (iy>=0 && iy<height && ix>=0 && ix<width)
            buffer[iy][ix] = it->getSymbol();
    }

    player.draw(buffer);

    for (int i = 0; i < height; i++)
        oss << buffer[i] << "\n";

    std::string screenText = oss.str();

    // 2. 寫入目前 active buffer
    DWORD written;
    WriteConsoleOutputCharacter(
        screenBuffer[activeBuffer],
        screenText.c_str(),
        (DWORD)screenText.size(),
        {0, 0},
        &written
    );

    // 3. 翻面（顯示剛剛畫好的畫面）
    flipBuffer();
}

// 存檔
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

// 主遊戲迴圈（原本沒動）
void Game::run() {
    hideCursor();

    std::cout << "====== NTU Student Survival Game ======\n";
    std::cout << "Use LEFT/RIGHT arrow keys or A/D to move.\n";
    std::cout << "C = coconut leaf (damage)\n";
    std::cout << "G = chicken cutlet (heal + score)\n";
    std::cout << "U = umbrella (stop rain + small heal)\n";
    std::cout << "Press any key to start...\n";
    _getch();

    auto startTime = std::chrono::steady_clock::now();

    while (!gameOver && !success) {

        auto now = std::chrono::steady_clock::now();
        int elapsedSec = (int)std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        int remainingSec = timeLimit - elapsedSec;

        if (remainingSec <= 0) { success = true; break; }

        if (!isRaining && player.getHealth() <= 50) {
            isRaining = true;
        }

        if (isRaining && elapsedSec % 8 == 0) {
            int hp = player.getHealth();
            hp -= 1;
            if (hp<0) hp = 0;
            player.setHealth(hp);
        }

        if (player.getHealth() <= 0) { gameOver = true; break; }

        if (_kbhit()) {
            int c = _getch();
            if (c == 0 || c == 224) {
                int c2 = _getch();
                if (c2 == 75) player.moveLeft();
                else if (c2 == 77) player.moveRight(width);
            }
            else {
                if (c=='a' || c=='A') player.moveLeft();
                if (c=='d' || c=='D') player.moveRight(width);
            }
        }

        spawnObjects(elapsedSec);
        updateObjects();
        handleCollisions();
        removeInactive();
        drawFrame(remainingSec);

        Sleep(80);
    }

    try {
        saveResult();
    } catch (const std::exception& e) {
        std::cerr << "Save file failed: " << e.what() << "\n";
    }

    // 退出前切回預設 console buffer
    SetConsoleActiveScreenBuffer(GetStdHandle(STD_OUTPUT_HANDLE));

    std::cout << (gameOver ? "===== GAME OVER =====\n"
                           : "===== SUCCESS =====\n");
    std::cout << "Final Score: " << player.getScore() << "\n";
    std::cout << "Press any key to exit...\n";
    _getch();
}
