口型同步
===================

口型同步图片必须存放在与角色图片相同目录中的 "lip" 资料夹内。

* happy.png（角色主图档）
* lip/
    * happy.png（口型同步档）

口型同步图片由口型同步差分的影格组成。
每个影格的大小必须与角色图片相同。
影格必须水平排列，并按照由左到右的顺序存放。

边缘的 alpha 值必须做平滑处理。
请在影像编辑软体中使用 "Blur Selection" 与 "Delete Selection"。

口型同步间隔可以在 `project.txt` 档案中指定。

```
#
# 口型同步影格长度（每影格秒数）
#
character.lipsync.frame=0.3

#
# 每个字元的口型同步次数
#
character.lipsync.chars=3
```
