施工中

## 遊戲設定

|名稱 |型別 |描述 |
|----------------------------|------------|---------------------------------------------------------------|
| game.title.en |字串|遊戲標題為英文。                                        |
| game.novel |布林 |啟用小說模式。                                            |
| game.locale |字串|強制使用指定語言，不採用系統區域設定。            |

### game.title.en

### game.novel

### game.local

語言設定。

- “”：使用系統設定
- “ja”：固定為日語
- “en”：固定為英語

--

## 字型檔案設定

# 字型 1
# - 預設字型
# - 您可以在訊息中加入 \f{1} 來選用此字型
font.ttf1=system/font/rounded-l-mplus-1c-bold.ttf

# 字型 2
# - 您可以在訊息中加入 \f{2} 來選用此字型
#font.ttf2=

# 字型 3
# - 您可以在訊息中加入 \f{3} 來選用此字型
#font.ttf3=

# 字型 4
# - 您可以在訊息中加入 \f{4} 來選用此字型
#font.ttf4=


############################################################
## 訊息框設定

#
# 訊息框的影象
#

msgbox.image=system/message/msgbox.png

#
＃ 動畫片
#

msgbox.anime.hide=system/message/msgbox-hide.anime
msgbox.anime.show=system/message/msgbox-show.anime

#
# 訊息框的位置
#

msgbox.x=0
msgbox.y=520

#
# 訊息框文字的邊距（以畫素為單位）
#

msgbox.margin.left=200
msgbox.margin.top=20
msgbox.margin.right=200
msgbox.margin.bottom=30

#
# 訊息框文字的行邊距（以畫素為單位）
#（包括字元高度）
#

msgbox.margin.line=40

#
# 訊息框文字的字元邊距（以畫素為單位）
#

msgbox.margin.char=0

#
# 訊息框文字的字型選擇 (1,2,3,4)
#

msgbox.font.select=1

#
# 訊息框文字的字型大小
#

msgbox.font.size=36

#
# 訊息框文字的字型顏色
#

msgbox.font.r=255
msgbox.font.g=255
msgbox.font.b=255

#
# 訊息框文字的字型輪廓寬度和顏色
#

msgbox.font.outline.width=0
msgbox.font.outline.r=0
msgbox.font.outline.g=0
msgbox.font.outline.b=0

#
# 訊息框文字的 Ruby 大小
#

msgbox.font.ruby=10

#
# 啟用訊息框文字的垂直書寫
#

msgbox.font.tategaki=false

#
# 啟用字元背景填充
#

msgbox.fill.enable=false
msgbox.fill.r=255
msgbox.fill.g=255
msgbox.fill.b=255

#
# 為訊息框文字的前面段落啟用變暗
#

msgbox.dim.enable=false

#
# 訊息框文字顏色變暗
#

msgbox.dim.r=0
msgbox.dim.g=0
msgbox.dim.b=0

#
# 調暗訊息框文字的輪廓寬度和顏色
#
msgbox.dim.outline.width=0
msgbox.dim.outline.r=0
msgbox.dim.outline.g=0
msgbox.dim.outline.b=0

#
# 啟用訊息框看到的訊息的字型顏色更改
#

msgbox.seen.enable=false

#
# 訊息框看到的訊息的字型顏色
#

msgbox.seen.r=0
msgbox.seen.g=0
msgbox.seen.b=0

#
# 訊息框看到的訊息的字型輪廓寬度和顏色
#

msgbox.seen.outline.width=0
msgbox.seen.outline.r=0
msgbox.seen.outline.g=0
msgbox.seen.outline.b=0

#
# 啟用跳過未見訊息的功能
#

msgbox.skip_unseen=false

############################################################
## 名稱框設定

#
# 啟用名稱框
# - 如果您需要全屏小說風格，請禁用名稱框
#

namebox.enable=true

#
# 名稱框的影象
#

namebox.image=system/message/namebox.png

#
＃ 動畫片
#

namebox.anime.hide=system/message/namebox-hide.anime
namebox.anime.show=system/message/namebox-show.anime

#
# 名稱框的位置
#

namebox.x=80
namebox.y=450

#
# 名稱框文字的邊距（以畫素為單位）
#

namebox.margin.top=5
namebox.margin.left=0

#
# 啟用名稱框文字居中
#

namebox.centering=true

#
# 名稱框文字的字型選擇（1,2,3,4）
#

namebox.font.select=1

#
# 名稱框文字的字型大小
#

namebox.font.size=36

#
# 名稱框文字的字型顏色
#

namebox.font.r=255
namebox.font.g=255
namebox.font.b=255

#
# 名稱框文字的字型輪廓寬度和顏色
#

namebox.font.outline.width=0
namebox.font.outline.r=255
namebox.font.outline.g=255
namebox.font.outline.b=255

#
# 名稱框文字的 Ruby 大小
#

namebox.font.ruby=10

#
# 啟用名稱框文字的垂直書寫
#

namebox.font.tategaki=false


############################################################
## 單擊動畫設定

#
# 點選動畫的位置
#

click.x=1060
click.y=660

#
# 點選動畫的間隔
#

click.interval=1.0

#
# 點選動畫的影象
#

click.image1=system/message/click1.png
click.image2=system/message/click2.png
#click.image3=
#click.image4=
#click.image5=
#click.image6=
#click.image7=
#click.image8=
#click.image9=
#click.image10=
#click.image11=
#click.image12=
#click.image13=
#click.image14=
#click.image15=
#click.image16=

#
# 啟用點選動畫的自動移動
#

click.move=false


############################################################
## 選擇框設定

#
# 選擇框文字的字型選擇
#

choose.font.select=1

#
# 選擇框文字的字型大小
#

choose.font.size=36

#
# 非尖頭選擇框文字的字型顏色
#

choose.font.idle.r=255
choose.font.idle.g=255
choose.font.idle.b=255

#
# 非尖頭選擇框文字的字型輪廓寬度和顏色
#

choose.font.idle.outline.width=0
choose.font.idle.outline.r=255
choose.font.idle.outline.g=255
choose.font.idle.outline.b=255

#
# 尖頭選擇框文字的字型顏色
#

choose.font.hover.r=255
choose.font.hover.g=0
choose.font.hover.b=0

#
# 尖頭選擇框文字的字型輪廓寬度和顏色
#

choose.font.hover.outline.width=0
choose.font.hover.outline.r=255
choose.font.hover.outline.g=255
choose.font.hover.outline.b=255

#
# 選擇框文字的 Ruby 大小
#

choose.font.ruby=10

#
# 啟用選擇框文字的垂直書寫
#

choose.font.tategaki=false

#
# 尖頭選擇框改變時的音效
#

choose.change_se=system/choose/button.ogg

#
# 選擇選擇框時的音效
#

choose.click_se=system/choose/button.ogg

#
# 選擇框 1 的設定
#

choose.box1.idle=system/choose/idle.png
choose.box1.hover=system/choose/hover.png
choose.box1.x=200
choose.box1.y=130
choose.box1.margin.top=15
choose.box1.idle_anime=system/choose/idle.anime
choose.box1.hover_anime=system/choose/hover.anime

#
# 選擇框 2 的設定
#

choose.box2.idle=system/choose/idle.png
choose.box2.hover=system/choose/hover.png
choose.box2.x=200
choose.box2.y=220
choose.box2.margin.top=15
choose.box2.idle_anime=system/choose/idle.anime
choose.box2.hover_anime=system/choose/hover.anime

#
# 選擇框 3 的設定
#

choose.box3.idle=system/choose/idle.png
choose.box3.hover=system/choose/hover.png
choose.box3.x=200
choose.box3.y=310
choose.box3.margin.top=15
choose.box3.idle_anime=system/choose/idle.anime
choose.box3.hover_anime=system/choose/hover.anime

#
# 選擇框 4 的設定
#

choose.box4.idle=system/choose/idle.png
choose.box4.hover=system/choose/hover.png
choose.box4.x=200
choose.box4.y=400
choose.box4.margin.top=15
choose.box4.idle_anime=system/choose/idle.anime
choose.box4.hover_anime=system/choose/hover.anime

#
# 選擇框5的設定
#

choose.box5.idle=system/choose/idle.png
choose.box5.hover=system/choose/hover.png
choose.box5.x=200
choose.box5.y=490
choose.box5.margin.top=15
choose.box5.idle_anime=system/choose/idle.anime
choose.box5.hover_anime=system/choose/hover.anime

#
# 選擇框 6 的設定
#

choose.box6.idle=system/choose/idle.png
choose.box6.hover=system/choose/hover.png
choose.box6.x=200
choose.box6.y=580
choose.box6.margin.top=15
choose.box6.idle_anime=system/choose/idle.anime
choose.box6.hover_anime=system/choose/hover.anime

#
# 選擇框7的設定
#

choose.box7.idle=system/choose/idle.png
choose.box7.hover=system/choose/hover.png
choose.box7.x=200
choose.box7.y=670
choose.box7.margin.top=15
choose.box7.idle_anime=system/choose/idle.anime
choose.box7.hover_anime=system/choose/hover.anime

#
# 選擇框8的設定
#

choose.box8.idle=system/choose/idle.png
choose.box8.hover=system/choose/hover.png
choose.box8.x=200
choose.box8.y=760
choose.box8.margin.top=15
choose.box8.idle_anime=system/choose/idle.anime
choose.box8.hover_anime=system/choose/hover.anime


############################################################
## 儲存資料設定
############################################################

#
# 儲存縮圖的大小
#

save.thumb.width=213
save.thumb.height=120

#
#“新”標記影象
#

#save.new_image=system/save/new.png


############################################################
## 系統按鈕設定

#
# 啟用系統按鈕
#

sysbtn.enable=true

#
# 系統按鈕的影象
#

sysbtn.idle=system/sysbtn/sysbtn-idle.png
sysbtn.hover=system/sysbtn/sysbtn-hover.png

#
＃ 日本動畫片
#

# 隱藏。
sysbtn.anime.out=system/sysbtn/anime-out.anime

# 出現。
sysbtn.anime.fadein=system/sysbtn/anime-fadein.anime

# 出現但未指出。
sysbtn.anime.appear=system/sysbtn/anime-appear.anime

# 懸停。
sysbtn.anime.hover=system/sysbtn/anime-hover.anime

# 消失。
sysbtn.anime.fadeout=system/sysbtn/anime-fadeout.anime

#
# 系統按鈕的位置
#

sysbtn.x=1183
sysbtn.y=48
sysbtn.width=100
sysbtn.height=100

#
# 系統按鈕音效
#

#sysbtn.enter_se=se/click.ogg
#sysbtn.leave_se=se/click.ogg
#sysbtn.click_se=se/click.ogg


############################################################
## 自動模式設定

#
# 自動模式橫幅影象
#

automode.image=system/message/auto.png

#
＃ 動畫片
#

automode.anime.hide=system/message/auto-hide.anime
automode.anime.show=system/message/auto-show.anime

#
# 自動模式橫幅的位置
#

automode.x=0
automode.y=126

#
# 進入自動模式時的音效
#

#automode.enter_se=

#
# 退出自動模式時的音效
#

#automode.leave_se=


############################################################
## 跳過模式設定

#
# 跳過模式橫幅的影象
#

skipmode.image=system/message/skip.png

#
＃ 動畫片
#

skipmode.anime.hide=system/message/skip-hide.anime
skipmode.anime.show=system/message/skip-show.anime

#
# 自動模式橫幅的位置
#

skipmode.x=0
skipmode.y=126

#
# 進入跳過模式時的音效
#

#skipmode.enter_se=

#
# 退出自動模式時的音效
#

#skipmode.leave_se=


############################################################
## 圖形使用者介面設定

#
# save/load GUI 項的槽索引文字的字型選擇
#
gui.save.index.font.select=1
gui.save.index.font.size=30
gui.save.index.font.r=255
gui.save.index.font.g=200
gui.save.index.font.b=200
gui.save.index.font.outline.width=0
gui.save.index.font.outline.r=0
gui.save.index.font.outline.g=0
gui.save.index.font.outline.b=0
gui.save.index.font.ruby=10
gui.save.index.font.tategaki=false
gui.save.index.margin.char=3

#
# save/load GUI 專案的日期文字的字型選擇
#
gui.save.date.font.select=1
gui.save.date.font.size=30
gui.save.date.font.r=255
gui.save.date.font.g=255
gui.save.date.font.b=255
gui.save.date.font.outline.width=0
gui.save.date.font.outline.r=0
gui.save.date.font.outline.g=0
gui.save.date.font.outline.b=0
gui.save.date.font.ruby=10
gui.save.date.font.tategaki=false
gui.save.date.margin.char=5

#
# save/load GUI 專案的章節文字的字型選擇
#
gui.save.chapter.font.select=1
gui.save.chapter.font.size=32
gui.save.chapter.font.r=255
gui.save.chapter.font.g=255
gui.save.chapter.font.b=255
gui.save.chapter.font.outline.width=2
gui.save.chapter.font.outline.r=32
gui.save.chapter.font.outline.g=32
gui.save.chapter.font.outline.b=32
gui.save.chapter.font.ruby=10
gui.save.chapter.font.tategaki=false
gui.save.chapter.margin.char=5

#
# save/load GUI 項的訊息文字的字型選擇
#
gui.save.msg.font.select=1
gui.save.msg.font.size=22
gui.save.msg.font.r=255
gui.save.msg.font.g=255
gui.save.msg.font.b=255
gui.save.msg.font.outline.width=1
gui.save.msg.font.outline.r=32
gui.save.msg.font.outline.g=32
gui.save.msg.font.outline.b=32
gui.save.msg.font.ruby=10
gui.save.msg.font.tategaki=false
gui.save.msg.margin.line=30
gui.save.msg.margin.char=0
gui.save.msg.multiline=true

#
# 歷史 GUI 專案的名稱和文字的字型選擇
#

gui.history.name.font.select=1
gui.history.name.font.size=34
gui.history.name.font.r=255
gui.history.name.font.g=245
gui.history.name.font.b=245
gui.history.name.font.outline.width=1
gui.history.name.font.outline.r=32
gui.history.name.font.outline.g=32
gui.history.name.font.outline.b=32
gui.history.name.font.ruby=10
gui.history.name.font.tategaki=false
gui.history.name.margin.line=40
gui.history.name.margin.char=0

gui.history.text.font.select=1
gui.history.text.font.size=32
gui.history.text.font.r=255
gui.history.text.font.g=255
gui.history.text.font.b=255
gui.history.text.font.outline.width=1
gui.history.text.font.outline.r=32
gui.history.text.font.outline.g=32
gui.history.text.font.outline.b=32
gui.history.text.font.ruby=10
gui.history.text.font.tategaki=false
gui.history.text.margin.line=40
gui.history.text.margin.char=0

#
#歷史GUI專案文字的行引用
#

gui.history.quote.name_separator=\n
gui.history.quote.start="
gui.history.quote.end="

#
# 隱藏最後一條歷史記錄
#

gui.history.hide_last=false

#
# 文字預覽 GUI 項的文字字型選擇 (1,2,3,4)
#

gui.preview.font.select=1

#
# 文字預覽 GUI 項的文字字型大小
#

gui.preview.font.size=36

#
# 文字預覽 GUI 項的文字的字型顏色
#

gui.preview.font.r=255
gui.preview.font.g=255
gui.preview.font.b=255

#
# 文字預覽 GUI 項的文字的字型輪廓寬度和顏色
#

gui.preview.font.outline.width=0
gui.preview.font.outline.r=255
gui.preview.font.outline.g=255
gui.preview.font.outline.b=255

#
# 文字預覽 GUI 專案的文字的 Ruby 大小
#

gui.preview.font.ruby=10

#
# 啟用文字預覽 GUI 項的文字垂直書寫
#

gui.preview.font.tategaki=false


############################################################
## 聲音設定

#
# BGM曲目音量的初始值
#

sound.vol.bgm=1.0

#
# 音軌音量初始值
#

sound.vol.voice=1.0

#
# SE軌道音量的初始值
#

sound.vol.se=1.0

#
# 每個字元體積的初始值
#

sound.vol.per_character=1.0


############################################################
## 角色設定

#
# 角色名稱
# - 用於名稱翻譯、口型同步、自動對焦等。
#

character.name1=Midori
character.name1.en=Midori
character.name1.zh-cn=美登利
character.name1.zh-tw=美登利
character.name1.ja=みどり

character.name2=Xiaoling
character.name2.en=Xiaoling
character.name2.zh-cn=小玲
character.name2.zh-tw=小玲
character.name2.ja=シャオリン

#character.name3=
#character.name4=
#character.name5=
#character.name6=
#character.name7=
#character.name8=
#character.name9=
#character.name10=
#character.name11=
#character.name12=
#character.name13=
#character.name14=
#character.name15=
#character.name16=
#character.name17=
#character.name18=
#character.name19=
#character.name20=
#character.name21=
#character.name22=
#character.name23=
#character.name24=
#character.name25=
#character.name26=
#character.name27=
#character.name28=
#character.name29=
#character.name30=
#character.name31=
#character.name32=

#
# 角色影象子資料夾（用於唇形同步和自動對焦）
#

character.folder1=ch/midori/
character.folder2=ch/xiaoling/
#character.folder3=
#character.folder4=
#character.folder5=
#character.folder6=
#character.folder7=
#character.folder8=
#character.folder9=
#character.folder10=
#character.folder11=
#character.folder12=
#character.folder13=
#character.folder14=
#character.folder15=
#character.folder16=
#character.folder17=
#character.folder18=
#character.folder19=
#character.folder20=
#character.folder21=
#character.folder22=
#character.folder23=
#character.folder24=
#character.folder25=
#character.folder26=
#character.folder27=
#character.folder28=
#character.folder29=
#character.folder30=
#character.folder31=
#character.folder32=

#
# 眨眼間隔（以秒為單位）
#

character.eyeblink.interval=4.0

#
# 眨眼幀長度（以秒為單位）
#

character.eyeblink.frame=0.05

#
# 唇形同步幀長度（以秒為單位）
#

character.lipsync.frame=0.04

#
# 每個字元的唇形同步幀
#

character.lipsync.chars=14

############################################################
## 自動對焦設定
##
## 注意：尚未在 RC1 上執行

#
# 自動聚焦於發言者/將非發言者取消聚焦於帶有名稱的 [文字]
#
#autofocus.on_text_name=true

#
# 自動取消對[文字]上沒有名稱的所有字元的聚焦
#
#autofocus.on_text_no_name=true

#
# 自動取消對 [ch] 上非發言者的關注
#
#autofocus.on_ch=true

#
# 自動取消所有角色對[選擇]的聚焦
#
#autofocus.on_choose=true


############################################################
## 舞臺設定

#
# 字元邊距（以畫素為單位）
#

stage.ch_margin.bottom=0
stage.ch_margin.left=0
stage.ch_margin.right=0


############################################################
## Kira Kira 效果設定

#
# 啟用 Kira Kira Effect（點選效果動畫）
#

kirakira.enable=false

#
# 為 Kira Kira 效果啟用新增混合
#

kirakira.add_blend=false

#
# Kira Kira Effect 的幀長度（以秒為單位）
#

kirakira.frame=0.333

#
# 基拉基拉效應的圖片
#

#kirakira.image1=kira1.png
#kirakira.image2=kira2.png
#kirakira.image3=kira3.png
#kirakira.image4=kira4.png
#kirakira.image5=
#kirakira.image6=
#kirakira.image7=
#kirakira.image8=
#kirakira.image9=
#kirakira.image10=
#kirakira.image11=
#kirakira.image12=
#kirakira.image13=
#kirakira.image14=
#kirakira.image15=
#kirakira.image16=


############################################################
## 表情符號

#
# 表情符號（1 到 32）
#

emoji.name1=heart
emoji.image1=system/emoji/heart.png

emoji.name2=sweat
emoji.image2=system/emoji/sweat.png


############################################################
## 文字轉語音

#
# 啟用語音合成
#

tts.enable=false


############################################################
## 釋出模式（安裝應用模式）

#
# 啟用Release模式（將儲存資料寫入AppData）
#

release_mode.enable=false
