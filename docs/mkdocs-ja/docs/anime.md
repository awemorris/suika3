アニメ
=====

アニメは、animeタグを使ってレイヤーベースのアニメーションを再生する機能です。

## アニメファイル

アニメファイルは、レイヤー変換のシーケンスを記述するテキストファイルです。

## シーケンス

メッセージボックスを1秒で右へ100px移動するには、アニメファイルに次のシーケンスを記述します。

```
# ブロックはアニメーションのシーケンスを表します。
# ブロック名は任意で、動作には影響しません。
move {
    # レイヤー指定子です。
    layer: msg;

    # 時刻指定子です。（秒単位）
    start: 0.0;
    end: 1.0;

    # 開始位置指定子です。'r0' は相対値 '0' を意味します。
    from-x: r0;
    from-y: r0;

    # 開始アルファ指定子です。
    from-a: 255;

    # 終了位置指定子です。'r100' は相対値 '100' を意味します。
    to-x: r100;
    to-y: r0;

    # 終了アルファ指定子です。
    to-a: 255;
}
```

## レイヤー構造

次に示すのは、下から上への順に並べたレイヤー構造です。

| レイヤー名       | 説明                                       |
|------------------|--------------------------------------------|
| bg               | 背景                                       |
| bg2              | 2つ目の背景（シームレススクロール用）     |
| efb1             | 背面エフェクト1                           |
| efb2             | 背面エフェクト2                           |
| efb3             | 背面エフェクト3                           |
| efb4             | 背面エフェクト4                           |
| chb              | 奥中央のキャラクター                      |
| chb-eye          | 奥中央のキャラクター                      |
| chb-lip          | 奥中央のキャラクター                      |
| chl              | 左のキャラクター                          |
| chl-eye          | 左のキャラクター                          |
| chl-lip          | 左のキャラクター                          |
| chlc             | 左中央のキャラクター                      |
| chlc-eye         | 左中央のキャラクター                      |
| chlc-lip         | 左中央のキャラクター                      |
| chr              | 右のキャラクター                          |
| chr-eye          | 右のキャラクター                          |
| chr-lip          | 右のキャラクター                          |
| chrc             | 右中央のキャラクター                      |
| chrc-eye         | 右中央のキャラクター                      |
| chrc-lip         | 右中央のキャラクター                      |
| eff1             | 前面エフェクト1                           |
| eff2             | 前面エフェクト2                           |
| eff3             | 前面エフェクト3                           |
| eff4             | 前面エフェクト4                           |
| msgbox           | メッセージボックス                        |
| namebox          | 名前ボックス                              |
| choose1-idle     | 選択肢ボックス1（待機）                   |
| choose1-hover    | 選択肢ボックス1（ホバー）                 |
| choose2-idle     | 選択肢ボックス2（待機）                   |
| choose2-hover    | 選択肢ボックス2（ホバー）                 |
| choose3-idle     | 選択肢ボックス3（待機）                   |
| choose3-hover    | 選択肢ボックス3（ホバー）                 |
| choose4-idle     | 選択肢ボックス4（待機）                   |
| choose4-hover    | 選択肢ボックス4（ホバー）                 |
| choose5-idle     | 選択肢ボックス5（待機）                   |
| choose5-hover    | 選択肢ボックス5（ホバー）                 |
| choose6-idle     | 選択肢ボックス6（待機）                   |
| choose6-hover    | 選択肢ボックス6（ホバー）                 |
| choose7-idle     | 選択肢ボックス7（待機）                   |
| choose7-hover    | 選択肢ボックス7（ホバー）                 |
| choose8-idle     | 選択肢ボックス8（待機）                   |
| choose8-hover    | 選択肢ボックス8（ホバー）                 |
| chf              | キャラクターの顔                          |
| chf-eye          | キャラクターの顔                          |
| chf-lip          | キャラクターの顔                          |
| click            | クリックアニメーション                    |
| auto             | オートモードバナー                        |
| skip             | スキップモードバナー                      |
| text1            | テキストレイヤー1                         |
| text2            | テキストレイヤー2                         |
| text3            | テキストレイヤー3                         |
| text4            | テキストレイヤー4                         |
| text5            | テキストレイヤー5                         |
| text6            | テキストレイヤー6                         |
| text7            | テキストレイヤー7                         |
| text8            | テキストレイヤー8                         |
| gui-button-1     | GUIボタンID 1                             |
| gui-button-2     | GUIボタンID 2                             |
| gui-button-3     | GUIボタンID 3                             |
| gui-button-4     | GUIボタンID 4                             |
| gui-button-5     | GUIボタンID 5                             |
| gui-button-6     | GUIボタンID 6                             |
| gui-button-7     | GUIボタンID 7                             |
| gui-button-8     | GUIボタンID 8                             |
| gui-button-9     | GUIボタンID 9                             |
| gui-button-10    | GUIボタンID 10                            |
| gui-button-11    | GUIボタンID 11                            |
| gui-button-12    | GUIボタンID 12                            |
| gui-button-13    | GUIボタンID 13                            |
| gui-button-14    | GUIボタンID 14                            |
| gui-button-15    | GUIボタンID 15                            |
| gui-button-16    | GUIボタンID 16                            |
| gui-button-17    | GUIボタンID 17                            |
| gui-button-18    | GUIボタンID 18                            |
| gui-button-19    | GUIボタンID 19                            |
| gui-button-20    | GUIボタンID 20                            |
| gui-button-21    | GUIボタンID 21                            |
| gui-button-22    | GUIボタンID 22                            |
| gui-button-23    | GUIボタンID 23                            |
| gui-button-24    | GUIボタンID 24                            |
| gui-button-25    | GUIボタンID 25                            |
| gui-button-26    | GUIボタンID 26                            |
| gui-button-27    | GUIボタンID 27                            |
| gui-button-28    | GUIボタンID 28                            |
| gui-button-29    | GUIボタンID 29                            |
| gui-button-30    | GUIボタンID 30                            |
| gui-button-31    | GUIボタンID 31                            |
| gui-button-32    | GUIボタンID 32                            |

## 拡大縮小と回転

```
# `effect1` レイヤーを3秒で2.0倍に拡大し、360度回転します。
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

    # 拡大縮小と回転の原点
    center-x: 600;
    center-y: 100;

    # 拡大縮小率
    from-scale-x: 1.0;
    from-scale-y: 1.0;
    to-scale-x: 2.0;
    to-scale-y: 2.0;
 
    # 回転（+ は右、- は左）
    from-rotate: 0.0;
    to-rotate: -360;
}

# 逆方向。
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
