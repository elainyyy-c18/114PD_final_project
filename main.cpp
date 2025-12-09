#include "game.h"
#include <conio.h>
#include <iostream>
#include <cstdlib>

int main() {
    while (true) {
        Game game("NTU_Student");
        game.run();   // 玩一局，裡面會印結束畫面

        std::cout << "\nPress 'R' to restart, or any other key to quit...\n";
        int c = _getch();
        if (c != 'r' && c != 'R') {
            break;    // 離開程式
        }
        system("cls");    // 要重開新的一局才清畫面
    }
    return 0;
}
