# 1. 編譯器設定
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++11 -O2 -Iinclude

# 2. 資料夾路徑 (確保沒有斜線開頭)
SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
OUT_DIR = output

# 3. 檔案設定
SRCS = $(wildcard $(SRC_DIR)/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJ_DIR)/%.o, $(SRCS))

# 4. 最終執行檔 (加上 .\ 確保是在當前目錄)
TARGET = $(BIN_DIR)\ntu_game.exe

# 5. 編譯規則
all: directories $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 建立資料夾
directories:
	@if not exist $(OBJ_DIR) mkdir $(OBJ_DIR)
	@if not exist $(BIN_DIR) mkdir $(BIN_DIR)
	@if not exist $(OUT_DIR) mkdir $(OUT_DIR)

clean:
	@if exist $(OBJ_DIR) rmdir /S /Q $(OBJ_DIR)
	@if exist $(BIN_DIR) rmdir /S /Q $(BIN_DIR)
	@echo Cleaned successfully!

.PHONY: all directories clean