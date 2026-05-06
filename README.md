# NTU Student Survival Game 

這是一個基於 C++ 開發的 Windows Console 動作生存遊戲。玩家扮演一名台大學生，在限定時間內躲避掉落物並應對惡劣天氣，考驗反應能力與資源管理。

## 🛠 技術亮點 (Technical Highlights)

*   **物件導向設計 (OOP)**：
    *   **繼承架構**：以 `Character` 為基底類別，衍生出 `Player` 與 `Enemy`[cite: 4]；`Item` 基類則管理 `ChickenCutlet` 與 `Umbrella` 等不同道具。
    *   **運算子多載**：實作 `operator+=` 處理經驗值與等級邏輯，以及 `operator<<` 簡化除錯與狀態輸出。
    *   **多型應用**：透過虛擬函式處理不同物件的碰撞偵測與行為更新，展現良好的程式擴充性。
*   **Windows 系統整合**：
    *   **Console 畫面優化**：調用 `Windows.h` API 隱藏游標、非同步偵測按鍵 (`GetAsyncKeyState`)，確保遊戲操作靈敏且流暢。
    *   **視覺回饋**：實作螢幕閃爍特效 (`flashScreen`) 與控制台屬性切換，提升受傷或特殊事件的臨場感。
*   **現代 C++ 特性**：
    *   使用 `<chrono>` 庫進行高精度遊戲時間管理與動態雨天週期運算。
    *   實作泛型模板函數 `clampValue` 處理生命值與座標的邊界限制。
*   **資料持久化**：
    *   透過 `fstream` 實作自動紀錄功能，將遊戲結果（含時間、得分、勝敗狀態）自動存檔至 `output/results.txt`。

## 🎮 遊戲規則

*   **生存目標**：在 **30 秒** 的時間限制內存活並獲取最高分。
*   **操作方式**：
    *   使用 `方向鍵 (LEFT/RIGHT)` 或 `A/D` 進行左右移動。
*   **物件說明**：
    *   `S` (Player)：玩家代表符號[cite: 2]。
    *   `C` (Coconut Leaf)：椰子葉，擊中會減少生命值 (HP)。
    *   `G` (Chicken Cutlet)：雞排，增加分數與經驗值。
    *   `U` (Umbrella)：雨傘，雨天時拾取可**立即停止下雨**。
*   **天氣系統**：
    *   系統會隨機觸發下雨狀態，下雨期間玩家會持續失血，必須優先尋找雨傘躲避。

## 📂 專案結構

本專案採用模組化架構，將介面定義與實作邏輯分離：
*   `src/`：存放所有核心實作原始碼 (`.cpp`)。
*   `include/`：存放類別定義與標頭檔 (`.h`)。
*   `output/`：存放遊戲紀錄檔 `results.txt`。
*   `obj/`：編譯過程中產生的目的檔。

## 🚀 編譯與執行

### 環境要求
*   **作業系統**：Windows
*   **編譯器**：MinGW-w64 (g++)

### 編譯指令
在專案根目錄下使用內附的 `Makefile` 進行編譯：
```bash
# 自動建立資料夾並編譯專案
make

# 執行遊戲
./bin/ntu_game.exe

# 清除編譯產物
make clean
```

## ✅ 執行截圖
### 1. 遊戲開始頁面
<img width="1378" height="395" alt="image" src="https://github.com/user-attachments/assets/5cf44dbd-8ab7-4051-8976-85fe1a5962f3" />

### 2. 放晴遊戲操作頁面
<img width="1370" height="602" alt="image" src="https://github.com/user-attachments/assets/1850177a-b976-4c7e-997d-e7526cf6f61e" />

### 3. 下雨遊戲操作頁面
<img width="1373" height="590" alt="image" src="https://github.com/user-attachments/assets/dc3be4a6-8b61-44a6-a40a-1e9abaa56a3e" />

### 4. result 輸出畫面
<img width="1153" height="634" alt="image" src="https://github.com/user-attachments/assets/a11c80fe-9000-49b6-8484-25defb9af97b" />

## 🎬 專案展示、回饋影片
https://www.youtube.com/watch?v=ckHAiTAwTuU

https://www.youtube.com/watch?v=26mP2644kj8
