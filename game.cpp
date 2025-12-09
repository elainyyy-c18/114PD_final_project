#include "game.h"

#include <conio.h>
#include <windows.h>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <fstream>
#include <stdexcept>

static HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

// 真正清空整個 console，用在：一局開始前、結束畫面
static void clearScreen() {
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    DWORD count;
    DWORD cellCount;
    COORD homeCoords = { 0, 0 };

    if (!GetConsoleScreenBufferInfo(hConsole, &csbi)) return;
    cellCount = csbi.dwSize.X * csbi.dwSize.Y;

    // 把整個 buffer 填空白
    FillConsoleOutputCharacter(hConsole, ' ', cellCount, homeCoords, &count);
    FillConsoleOutputAttribute(hConsole, csbi.wAttributes, cellCount, homeCoords, &count);
    SetConsoleCursorPosition(hConsole, homeCoords);
}

// 不清畫面，只把游標移到左上角，用在每一 frame 畫面更新
static void moveCursorHome() {
    COORD coord = { 0, 0 };
    SetConsoleCursorPosition(hConsole, coord);
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

// ===== Character =====

Character::Character(const std::string& n, int hp)
    : name(n), health(hp) {}

Character::~Character() {}

std::string Character::getName() const { return name; }
int Character::getHealth() const { return health; }
void Character::setHealth(int hp) { health = hp; }

// ===== Player =====

Player::Player(const std::string& n)
    : Character(n, 100), level(1), experience(0), x(0), y(0), score(0) {}

int Player::getX() const { return x; }
int Player::getY() const { return y; }
int Player::getScore() const { return score; }

void Player::setPosition(int px, int py) { x = px; y = py; }

void Player::moveLeft() { if (x > 0) x--; }

void Player::moveRight(int width) { if (x < width - 1) x++; }

void Player::draw(std::vector<std::string>& buffer) const {
    if (y >= 0 && y < (int)buffer.size() &&
        x >= 0 && x < (int)buffer[0].size()) {
        buffer[y][x] = 'S';
    }
}

void Player::gainScore(int s) { score += s; }

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

// ===== Enemy =====

Enemy::Enemy(const std::string& n, int hp, int x, int y, char sym)
    : Character(n, hp), x(x), y(y), symbol(sym), active(true) {}

Enemy::~Enemy() {}

int Enemy::getX() const { return x; }
int Enemy::getY() const { return y; }
char Enemy::getSymbol() const { return symbol; }

void Enemy::setY(int ny) { y = ny; }

bool Enemy::isActive() const { return active; }
void Enemy::deactivate() { active = false; }

void Enemy::update() { y++; }

void Enemy::attack(Character& target) {
    int newHp = target.getHealth() - 5;
    target.setHealth(newHp);
}

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

// ===== Item =====

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
    p.gainScore(10);
    p += 10;
    deactivate();
}

Umbrella::Umbrella(int x, int y)
    : Item("Umbrella", x, y, 'U') {}

void Umbrella::apply(Player& p) {
    p.gainScore(5);
    p += 5;
    deactivate();
}

// ===== Game =====

Game::Game(const std::string& playerName, int w, int h, int limitSec)
    : player(playerName),
      isRaining(false),
      gameOver(false),
      success(false),
      width(w),
      height(h),
      timeLimit(limitSec),
      nextRainStart(0),
      rainEndTime(-1),
      lastRainDamageSecond(-1) {

    std::srand((unsigned)std::time(nullptr));
    player.setPosition(width / 2, height - 1);
    nextRainStart = 10 + std::rand() % 11;
}

Game::~Game() {
    for (auto e : enemies) delete e;
    for (auto it : items) delete it;
}

void Game::spawnObjects(int) {
    if (std::rand() % 8 == 0) {
        int x = std::rand() % width;
        enemies.push_back(new CoconutLeaf(x, 0));
    }

    if (std::rand() % 20 == 0) {
        int x = std::rand() % width;
        items.push_back(new ChickenCutlet(x, 0));
    }

    if (isRaining) {
        bool hasUmbrella = false;
        for (auto it : items) {
            if (it->isActive() && it->getSymbol() == 'U') {
                hasUmbrella = true;
                break;
            }
        }
        if (!hasUmbrella) {
            int x = std::rand() % width;
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

    for (auto e : enemies) {
        if (!e->isActive()) continue;
        if (e->getX() == px && e->getY() == py) {
            e->onCollision(player);
        }
    }

    for (auto it : items) {
        if (!it->isActive()) continue;
        if (it->getX() == px && it->getY() == py) {
            bool isUmb = (it->getSymbol() == 'U');
            it->apply(player);

            if (isUmb && isRaining) {
                isRaining = false;
                lastRainDamageSecond = -1;

                int remainingSec = timeLimit - elapsedSec;
                if (remainingSec > 0) {
                    nextRainStart = elapsedSec + 10 + std::rand() % 11;
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
        int rx = std::rand() % width;
        int ry = std::rand() % (height - 2);
        buffer[ry][rx] = '|';
    }
}

void Game::drawFrame(int remainingSec) {
    // 不要整個 cls，只把游標移回左上角，直接蓋畫面
    moveCursorHome();

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

void Game::saveResult() const {
    std::ofstream ofs("results.txt", std::ios::app);
    if (!ofs) {
        throw std::runtime_error("Cannot open results.txt");
    }

    // 取得現在時間
    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;

    // Windows 版 localtime_s（安全版本）
    localtime_s(&localTime, &now_c);

    char timeBuf[32];
    std::strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &localTime);

    // 寫入結果
    ofs << "[" << timeBuf << "] "
        << (success ? "SUCCESS" : "GAMEOVER") << " "
        << player.getName() << " "
        << "HP=" << player.getHealth() << " "
        << "Score=" << player.getScore()
        << "\n";
}

void Game::run() {
    // 每次 run 之前重設狀態
    gameOver  = false;
    success   = false;
    isRaining = false;

    for (auto e : enemies) delete e;
    enemies.clear();
    for (auto it : items) delete it;
    items.clear();

    player.setHealth(100);
    player.setPosition(width / 2, height - 1);
    nextRainStart          = 10 + std::rand() % 11;
    lastRainDamageSecond   = -1;

    hideCursor();
    clearScreen();

    std::cout << "====== NTU Student Survival Game ======\n";
    std::cout << "Move with LEFT/RIGHT arrow keys or A/D.\n";
    std::cout << "C = coconut leaf (damage)\n";
    std::cout << "G = chicken cutlet (score only)\n";
    std::cout << "U = umbrella (stop rain + score)\n";
    std::cout << "Game time: " << timeLimit << " seconds.\n";
    std::cout << "Press any key to start...\n";
    _getch();

    auto startTime = std::chrono::steady_clock::now();

    while (true) {
        auto now = std::chrono::steady_clock::now();
        int elapsedSec =
            (int)std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count();
        int remainingSec = timeLimit - elapsedSec;
        if (remainingSec < 0) remainingSec = 0;

        // 判斷勝利 / 失敗（先判斷，不再畫下一 frame）
        if (elapsedSec >= timeLimit) {
            success = true;
            break;
        }
        if (player.getHealth() <= 0) {
            gameOver = true;
            break;
        }

        // 下雨邏輯
        if (!isRaining && elapsedSec >= nextRainStart) {
            isRaining = true;
            lastRainDamageSecond = -1;
        }

        if (isRaining && elapsedSec != lastRainDamageSecond) {
            int hp = player.getHealth();
            hp -= 5;
            hp = clampValue(hp, 0, 100);
            player.setHealth(hp);
            lastRainDamageSecond = elapsedSec;
        }

        // 鍵盤輸入：用 GetAsyncKeyState，避免 key repeat 卡住
        bool leftPressed =
            (GetAsyncKeyState(VK_LEFT)  & 0x8000) ||
            (GetAsyncKeyState('A')      & 0x8000);

        bool rightPressed =
            (GetAsyncKeyState(VK_RIGHT) & 0x8000) ||
            (GetAsyncKeyState('D')      & 0x8000);

        if (leftPressed && !rightPressed) {
            player.moveLeft();
        } else if (rightPressed && !leftPressed) {
            player.moveRight(width);
        }

        // 更新物件＋畫面
        spawnObjects(elapsedSec);
        updateObjects();
        handleCollisions(elapsedSec);
        removeInactive();
        drawFrame(remainingSec);   // 裡面用 moveCursorHome()，不會閃

        Sleep(80);
    }

    // 存結果
    try {
        saveResult();
    } catch (const std::exception& e) {
        std::cerr << "Save file failed: " << e.what() << "\n";
    }

    // 🔥 這裡一定會執行到：清畫面＋顯示純結束畫面
    clearScreen();

    std::cout << (gameOver ? "===== GAME OVER =====\n"
                           : "===== SUCCESS =====\n");

    if (gameOver) {
        std::cout << "You died QQ\n";
    } else {
        std::cout << "Congratulations! You survived! :D\n";
    }

    std::cout << "Final Score: " << player.getScore() << "\n";
}
