眨眼
============

眨眼图片必须存放在与角色图片相同目录中的 "eye" 资料夹内。

- happy.png（角色主图档）
- eye/
    - happy.png（眨眼档）

眨眼图片由眨眼差分的影格组成。
每个影格的大小必须与角色图片相同。
影格必须水平排列，并按照由左到右的顺序存放。实际图片请参考范例游戏。

边缘的 alpha 值必须做平滑处理。
请在影像编辑软体中使用 "Blur Selection" 与 "Delete Selection"。

眨眼间隔可以在 `config.ini` 档案中指定。
间隔会稍微随机化，有时也会出现双重眨眼。

```
#
# 眨眼间隔（秒）
#
character.eyeblink.interval=4.0

#
# 眨眼影格长度（每影格秒数）
#
character.eyeblink.frame=0.15
```
