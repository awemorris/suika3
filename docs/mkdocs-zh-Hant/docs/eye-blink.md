眨眼
============

眨眼圖片必須存放在與角色圖片相同目錄中的 "eye" 資料夾內。

- happy.png（角色主圖檔）
- eye/
    - happy.png（眨眼檔）

眨眼圖片由眨眼差分的影格組成。
每個影格的大小必須與角色圖片相同。
影格必須水平排列，並按照由左到右的順序存放。實際圖片請參考範例遊戲。

邊緣的 alpha 值必須做平滑處理。
請在影像編輯軟體中使用 "Blur Selection" 與 "Delete Selection"。

眨眼間隔可以在 `config.ini` 檔案中指定。
間隔會稍微隨機化，有時也會出現雙重眨眼。

```
#
# 眨眼間隔（秒）
#
character.eyeblink.interval=4.0

#
# 眨眼影格長度（每影格秒數）
#
character.eyeblink.frame=0.15
```
