口型同步
===================

口型同步圖片必須存放在與角色圖片相同目錄中的 "lip" 資料夾內。

* happy.png（角色主圖檔）
* lip/
    * happy.png（口型同步檔）

口型同步圖片由口型同步差分的影格組成。
每個影格的大小必須與角色圖片相同。
影格必須水平排列，並按照由左到右的順序存放。

邊緣的 alpha 值必須做平滑處理。
請在影像編輯軟體中使用 "Blur Selection" 與 "Delete Selection"。

口型同步間隔可以在 `project.txt` 檔案中指定。

```
#
# 口型同步影格長度（每影格秒數）
#
character.lipsync.frame=0.3

#
# 每個字元的口型同步次數
#
character.lipsync.chars=3
```
