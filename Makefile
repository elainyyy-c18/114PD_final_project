# 1. 編譯器與參數設定
CXX = g++
# -Iinclude 是告訴編譯器：遇到 #include "xxx.h" 時，自動去 include 資料夾找
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -Iinclude

# 2. 資料夾設定
SRC_DIR = src
INC_DIR = include
OBJ_DIR = obj
OUT_DIR = output

# 3. 自動抓取檔案
# 找出 src/ 底下所有的 .cpp 檔
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
# 將 src/xxx.cpp 名稱替換成 obj/xxx.o
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# 4. 最終執行檔名稱 (輸出在根目錄)
TARGET = ntu_game.exe

# 5. 編譯規則
all: directories $(TARGET)

# 連結所有的 .o 檔產生 .exe
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# 將 src/ 底下的 .cpp 編譯成 obj/ 底下的 .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 確保 obj 與 output 資料夾存在 (Windows 語法)
directories:
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	@if not exist $(OUT_DIR) mkdir $(OUT_DIR)

# 清除編譯產生的檔案 (輸入 make clean)
clean:
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	@if exist $(TARGET) del /Q $(TARGET)
	@echo Cleaned successfully!

.PHONY: all directories clean