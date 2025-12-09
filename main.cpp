#include "game.h"
#include <conio.h>
#include <iostream>
#include <cstdlib>
#include <windows.h>

// test change

int main() {
    // 取得標準輸入的 handle，用來清空鍵盤 buffer
    HANDLE hIn = GetStdHandle(STD_INPUT_HANDLE);

    while (true) {
        Game game("NTU_Student");
        game.run();   // 玩一局，裡面會印結束畫面 (GAME OVER / SUCCESS)

        // 🔥 清空所有舊的按鍵，避免上一局的輸入影響這裡
        FlushConsoleInputBuffer(hIn);

        std::cout << "\nPress 'R' to restart, or any other key to quit...\n";

        int c = _getch();   // 這裡一定會「真的等你按一次」
        if (c != 'r' && c != 'R') {
            break;          // 不是 r/R 就離開 while，程式結束
        }

        system("cls");      // 要重開新的一局才清畫面
    }

    return 0;
}