動畫
=====

Anime 是一個透過 anime 標籤播放以圖層為基礎的動畫功能。

## Anime 檔案

Anime 檔案是一種文字檔，用來描述圖層轉換的序列。

## 序列

若要讓訊息視窗在一秒內向右移動 100px，請在 anime 檔案中寫入以下序列。

```
# 一個區塊描述一段動畫序列。
# 區塊名稱可以隨意命名，不會影響任何事。
move {
    # 這是圖層指定子。
    layer: msg;

    # 這些是時間指定子。（單位：秒）
    start: 0.0;
    end: 1.0;

    # 這些是起始位置指定子。`r0` 代表相對位置 0。
    from-x: r0;
    from-y: r0;

    # 這是起始 alpha 指定子。
    from-a: 255;

    # 這些是結束位置指定子。`r100` 代表相對位置 100。
    to-x: r100;
    to-y: r0;

    # 這是結束 alpha 指定子。
    to-a: 255;
}
```

## 圖層結構

以下是我們由下到上的圖層結構。

| 圖層名稱         | 說明                                   |
|------------------|----------------------------------------|
| bg               | 背景                                   |
| bg2              | 第二背景（用於無縫捲動）               |
| efb1             | 背景特效 1                             |
| efb2             | 背景特效 2                             |
| efb3             | 背景特效 3                             |
| efb4             | 背景特效 4                             |
| chb              | 後方中央角色                           |
| chb-eye          | 後方中央角色                           |
| chb-lip          | 後方中央角色                           |
| chl              | 左側角色                               |
| chl-eye          | 左側角色                               |
| chl-lip          | 左側角色                               |
| chlc             | 左中角色                               |
| chlc-eye         | 左中角色                               |
| chlc-lip         | 左中角色                               |
| chr              | 右側角色                               |
| chr-eye          | 右側角色                               |
| chr-lip          | 右側角色                               |
| chrc             | 右中角色                               |
| chrc-eye         | 右中角色                               |
| chrc-lip         | 右中角色                               |
| eff1             | 前景特效 1                             |
| eff2             | 前景特效 2                             |
| eff3             | 前景特效 3                             |
| eff4             | 前景特效 4                             |
| msgbox           | 訊息視窗                               |
| namebox          | 名稱框                                 |
| choose1-idle     | 選項框 1（閒置）                       |
| choose1-hover    | 選項框 1（滑入）                       |
| choose2-idle     | 選項框 2（閒置）                       |
| choose2-hover    | 選項框 2（滑入）                       |
| choose3-idle     | 選項框 3（閒置）                       |
| choose3-hover    | 選項框 3（滑入）                       |
| choose4-idle     | 選項框 4（閒置）                       |
| choose4-hover    | 選項框 4（滑入）                       |
| choose5-idle     | 選項框 5（閒置）                       |
| choose5-hover    | 選項框 5（滑入）                       |
| choose6-idle     | 選項框 6（閒置）                       |
| choose6-hover    | 選項框 6（滑入）                       |
| choose7-idle     | 選項框 7（閒置）                       |
| choose7-hover    | 選項框 7（滑入）                       |
| choose8-idle     | 選項框 8（閒置）                       |
| choose8-hover    | 選項框 8（滑入）                       |
| chf              | 角色臉部                               |
| chf-eye          | 角色臉部                               |
| chf-lip          | 角色臉部                               |
| click            | 點選動畫                               |
| auto             | 自動模式橫幅                           |
| skip             | 跳過模式橫幅                           |
| text1            | 文字圖層 1                             |
| text2            | 文字圖層 2                             |
| text3            | 文字圖層 3                             |
| text4            | 文字圖層 4                             |
| text5            | 文字圖層 5                             |
| text6            | 文字圖層 6                             |
| text7            | 文字圖層 7                             |
| text8            | 文字圖層 8                             |
| gui-button-1     | GUI 按鈕 ID 1                          |
| gui-button-2     | GUI 按鈕 ID 2                          |
| gui-button-3     | GUI 按鈕 ID 3                          |
| gui-button-4     | GUI 按鈕 ID 4                          |
| gui-button-5     | GUI 按鈕 ID 5                          |
| gui-button-6     | GUI 按鈕 ID 6                          |
| gui-button-7     | GUI 按鈕 ID 7                          |
| gui-button-8     | GUI 按鈕 ID 8                          |
| gui-button-9     | GUI 按鈕 ID 9                          |
| gui-button-10    | GUI 按鈕 ID 10                         |
| gui-button-11    | GUI 按鈕 ID 11                         |
| gui-button-12    | GUI 按鈕 ID 12                         |
| gui-button-13    | GUI 按鈕 ID 13                         |
| gui-button-14    | GUI 按鈕 ID 14                         |
| gui-button-15    | GUI 按鈕 ID 15                         |
| gui-button-16    | GUI 按鈕 ID 16                         |
| gui-button-17    | GUI 按鈕 ID 17                         |
| gui-button-18    | GUI 按鈕 ID 18                         |
| gui-button-19    | GUI 按鈕 ID 19                         |
| gui-button-20    | GUI 按鈕 ID 20                         |
| gui-button-21    | GUI 按鈕 ID 21                         |
| gui-button-22    | GUI 按鈕 ID 22                         |
| gui-button-23    | GUI 按鈕 ID 23                         |
| gui-button-24    | GUI 按鈕 ID 24                         |
| gui-button-25    | GUI 按鈕 ID 25                         |
| gui-button-26    | GUI 按鈕 ID 26                         |
| gui-button-27    | GUI 按鈕 ID 27                         |
| gui-button-28    | GUI 按鈕 ID 28                         |
| gui-button-29    | GUI 按鈕 ID 29                         |
| gui-button-30    | GUI 按鈕 ID 30                         |
| gui-button-31    | GUI 按鈕 ID 31                         |
| gui-button-32    | GUI 按鈕 ID 32                         |

## 縮放與旋轉

```
# 將 `effect1` 圖層在 3 秒內放大到 2.0 倍並旋轉 360 度。
test1 {
    layer: effect1;
 
    start: 0.0;
    end: 3.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    # 縮放與旋轉的中心點
    center-x: 600;
    center-y: 100;

    # 縮放係數
    from-scale-x: 1.0;
    from-scale-y: 1.0;
    to-scale-x: 2.0;
    to-scale-y: 2.0;
 
    # 旋轉（正值向右，負值向左）
    from-rotate: 0.0;
    to-rotate: -360;
}

# 反向。
test2 {
    layer: effect1;

    start: 3.0;
    end: 6.0;

    from-x: 0;
    from-y: 400;
    from-a: 255;

    to-x: 0;
    to-y: 400;
    to-a: 255;

    center-x: 600;
    center-y: 100;

    from-scale-x: 2.0;
    to-scale-x: 1.0;

    from-scale-y: 2.0;
    to-scale-y: 1.0;

    from-rotate: -360;
    to-rotate: 0;
}
```
