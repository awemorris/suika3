动画
=====

Anime 是一个透过 anime 标签播放以图层为基础的动画功能。

## Anime 档案

Anime 档案是一种文字档，用来描述图层转换的序列。

## 序列

若要让讯息视窗在一秒内向右移动 100px，请在 anime 档案中写入以下序列。

```
# 一个区块描述一段动画序列。
# 区块名称可以随意命名，不会影响任何事。
move {
    # 这是图层指定子。
    layer: msg;

    # 这些是时间指定子。（单位：秒）
    start: 0.0;
    end: 1.0;

    # 这些是起始位置指定子。`r0` 代表相对位置 0。
    from-x: r0;
    from-y: r0;

    # 这是起始 alpha 指定子。
    from-a: 255;

    # 这些是结束位置指定子。`r100` 代表相对位置 100。
    to-x: r100;
    to-y: r0;

    # 这是结束 alpha 指定子。
    to-a: 255;
}
```

## 图层结构

以下是我们由下到上的图层结构。

| 图层名称         | 说明                                   |
|------------------|----------------------------------------|
| bg               | 背景                                   |
| bg2              | 第二背景（用于无缝卷动）               |
| efb1             | 背景特效 1                             |
| efb2             | 背景特效 2                             |
| efb3             | 背景特效 3                             |
| efb4             | 背景特效 4                             |
| chb              | 后方中央角色                           |
| chb-eye          | 后方中央角色                           |
| chb-lip          | 后方中央角色                           |
| chl              | 左侧角色                               |
| chl-eye          | 左侧角色                               |
| chl-lip          | 左侧角色                               |
| chlc             | 左中角色                               |
| chlc-eye         | 左中角色                               |
| chlc-lip         | 左中角色                               |
| chr              | 右侧角色                               |
| chr-eye          | 右侧角色                               |
| chr-lip          | 右侧角色                               |
| chrc             | 右中角色                               |
| chrc-eye         | 右中角色                               |
| chrc-lip         | 右中角色                               |
| eff1             | 前景特效 1                             |
| eff2             | 前景特效 2                             |
| eff3             | 前景特效 3                             |
| eff4             | 前景特效 4                             |
| msgbox           | 讯息视窗                               |
| namebox          | 名称框                                 |
| choose1-idle     | 选项框 1（闲置）                       |
| choose1-hover    | 选项框 1（滑入）                       |
| choose2-idle     | 选项框 2（闲置）                       |
| choose2-hover    | 选项框 2（滑入）                       |
| choose3-idle     | 选项框 3（闲置）                       |
| choose3-hover    | 选项框 3（滑入）                       |
| choose4-idle     | 选项框 4（闲置）                       |
| choose4-hover    | 选项框 4（滑入）                       |
| choose5-idle     | 选项框 5（闲置）                       |
| choose5-hover    | 选项框 5（滑入）                       |
| choose6-idle     | 选项框 6（闲置）                       |
| choose6-hover    | 选项框 6（滑入）                       |
| choose7-idle     | 选项框 7（闲置）                       |
| choose7-hover    | 选项框 7（滑入）                       |
| choose8-idle     | 选项框 8（闲置）                       |
| choose8-hover    | 选项框 8（滑入）                       |
| chf              | 角色脸部                               |
| chf-eye          | 角色脸部                               |
| chf-lip          | 角色脸部                               |
| click            | 点选动画                               |
| auto             | 自动模式横幅                           |
| skip             | 跳过模式横幅                           |
| text1            | 文字图层 1                             |
| text2            | 文字图层 2                             |
| text3            | 文字图层 3                             |
| text4            | 文字图层 4                             |
| text5            | 文字图层 5                             |
| text6            | 文字图层 6                             |
| text7            | 文字图层 7                             |
| text8            | 文字图层 8                             |
| gui-button-1     | GUI 按钮 ID 1                          |
| gui-button-2     | GUI 按钮 ID 2                          |
| gui-button-3     | GUI 按钮 ID 3                          |
| gui-button-4     | GUI 按钮 ID 4                          |
| gui-button-5     | GUI 按钮 ID 5                          |
| gui-button-6     | GUI 按钮 ID 6                          |
| gui-button-7     | GUI 按钮 ID 7                          |
| gui-button-8     | GUI 按钮 ID 8                          |
| gui-button-9     | GUI 按钮 ID 9                          |
| gui-button-10    | GUI 按钮 ID 10                         |
| gui-button-11    | GUI 按钮 ID 11                         |
| gui-button-12    | GUI 按钮 ID 12                         |
| gui-button-13    | GUI 按钮 ID 13                         |
| gui-button-14    | GUI 按钮 ID 14                         |
| gui-button-15    | GUI 按钮 ID 15                         |
| gui-button-16    | GUI 按钮 ID 16                         |
| gui-button-17    | GUI 按钮 ID 17                         |
| gui-button-18    | GUI 按钮 ID 18                         |
| gui-button-19    | GUI 按钮 ID 19                         |
| gui-button-20    | GUI 按钮 ID 20                         |
| gui-button-21    | GUI 按钮 ID 21                         |
| gui-button-22    | GUI 按钮 ID 22                         |
| gui-button-23    | GUI 按钮 ID 23                         |
| gui-button-24    | GUI 按钮 ID 24                         |
| gui-button-25    | GUI 按钮 ID 25                         |
| gui-button-26    | GUI 按钮 ID 26                         |
| gui-button-27    | GUI 按钮 ID 27                         |
| gui-button-28    | GUI 按钮 ID 28                         |
| gui-button-29    | GUI 按钮 ID 29                         |
| gui-button-30    | GUI 按钮 ID 30                         |
| gui-button-31    | GUI 按钮 ID 31                         |
| gui-button-32    | GUI 按钮 ID 32                         |

## 缩放与旋转

```
# 将 `effect1` 图层在 3 秒内放大到 2.0 倍并旋转 360 度。
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

    # 缩放与旋转的中心点
    center-x: 600;
    center-y: 100;

    # 缩放系数
    from-scale-x: 1.0;
    from-scale-y: 1.0;
    to-scale-x: 2.0;
    to-scale-y: 2.0;
 
    # 旋转（正值向右，负值向左）
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
