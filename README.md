# find_mymouse_byshake

Windows 版的「搖晃滑鼠找游標」：快速左右（或上下）搖晃滑鼠，游標就會像 macOS 一樣暫時放大，停下來後自動縮回。

- 單一個 exe，約 150KB，不需要安裝任何執行環境
- 常駐在系統匣，幾乎不吃 CPU（只在滑鼠移動時運算）
- 支援高 DPI、多螢幕
- 不需要系統管理員權限
- 支援 Windows 10 / 11（x64；ARM 版 Windows 也能透過模擬執行）

## 安裝

### 方法一：一行指令（推薦）

開啟 **PowerShell**，貼上：

```powershell
irm https://raw.githubusercontent.com/chriscollective/find_mymouse_byshake/main/install.ps1 | iex
```

如果是在 **命令提示字元（cmd）**：

```cmd
powershell -c "irm https://raw.githubusercontent.com/chriscollective/find_mymouse_byshake/main/install.ps1 | iex"
```

這行指令會：

1. 下載最新版到 `%LOCALAPPDATA%\find_mymouse_byshake\`
2. 在開始功能表建立捷徑
3. 設定開機自動啟動
4. 立刻在背景啟動

### 方法二：手動下載

到 [Releases](https://github.com/chriscollective/find_mymouse_byshake/releases/latest) 下載 `find_mymouse_byshake.exe`，點兩下執行即可。第一次執行會自動設定開機啟動（可在選單關掉）。

> 因為程式沒有付費的程式碼簽章，用瀏覽器下載後第一次執行時，Windows 可能跳出「Windows 已保護您的電腦」。
> 點「**其他資訊**」→「**仍要執行**」即可。用方法一安裝通常不會出現這個畫面。

## 使用

- **快速左右搖晃滑鼠** → 游標放大，停下來約 0.3 秒後俐落地縮回
- **右鍵（或左鍵）點系統匣的游標圖示** → 設定選單：

| 選項 | 說明 |
|---|---|
| 啟用搖晃放大游標 | 暫時關閉／開啟偵測 |
| 靈敏度 | 低／中／高。誤觸太多就調低，很難觸發就調高 |
| 放大倍率 | 3 倍／5 倍／7 倍 |
| 游標顏色 | 白底黑邊（預設，和 Windows 預設游標一樣）／黑底白邊 |
| 開機自動啟動 | 開關開機自動執行 |
| 試試放大效果 | 不用搖晃，直接看放大效果 |
| 結束 | 關閉程式 |

> Windows 11 預設會把新的系統匣圖示收在「^」裡面。想讓它固定顯示：
> 設定 → 個人化 → 工作列 → 其他系統匣圖示 → 打開 Find My Mouse by Shake。

設定存在 `HKEY_CURRENT_USER\Software\find_mymouse_byshake`。

## 移除

```powershell
irm https://raw.githubusercontent.com/chriscollective/find_mymouse_byshake/main/uninstall.ps1 | iex
```

會關閉程式、移除開機啟動、開始功能表捷徑、設定與程式檔案。手動下載的使用者也可以直接在選單按「結束」→ 刪掉 exe，再把「開機自動啟動」關掉（或先關再刪）。

## 已知限制

- 放大的游標是畫在最上層的透明視窗上，在「開始」功能表、工作檢視等系統介面上方可能看不到。
- 全螢幕獨佔模式的遊戲中不會顯示（多數遊戲本來就會隱藏游標）。
- 放大時顯示的固定是箭頭圖示，不會跟著變成文字游標（I 形）或手形。
- 放大期間會暫時把系統游標換成透明的（讓大箭頭「取代」游標，而不是墊在游標下面），縮回後立刻還原。
  萬一程式在放大途中被強制結束，下次啟動時會自動還原；或在「設定 → 藍牙與裝置 → 滑鼠 → 其他滑鼠設定 → 指標」按一次「套用」也能還原。
- 少數程式使用自己的游標圖案（不是系統游標），放大時小游標仍可能出現在大箭頭上。

## 從原始碼編譯

需要 mingw-w64（macOS：`brew install mingw-w64`；Ubuntu：`sudo apt install mingw-w64`）。

```sh
make test   # 在本機跑單元測試（搖晃偵測、游標繪製、動畫、設定）
make        # 產生 build/find_mymouse_byshake.exe
```

專案結構：

```
src/core/   與平台無關的邏輯（都有單元測試）
  shake.c     搖晃偵測：判斷連續、快速、夠長的來回移動
  arrow.c     用距離場畫出抗鋸齒的大游標（黑底白邊 + 陰影）
  anim.c      放大／縮小的緩動動畫
  settings.c  靈敏度、倍率預設值（依 DPI 縮放）
src/win/    Windows 介面
  main.c      進入點、Raw Input 監聽滑鼠、動畫計時器
  overlay.c   最上層、可穿透點擊的透明視窗，畫放大的游標
  tray.c      系統匣圖示與選單
  config.c    登錄檔設定、開機自動啟動
tools/make_icon.c  編譯時用同一套繪製程式產生 app.ico
```

推上 `v*` 標籤（例如 `git tag v0.1.0 && git push --tags`）後，GitHub Actions 會編譯、在真的 Windows 上跑安裝／移除的冒煙測試，通過後自動發佈 Release。

## 授權

MIT
