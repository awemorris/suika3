Ray VN API リファレンス
======================

`VN (Suika.*)` の API はビジュアルノベル制作向けに設計されています。

すべての `Suika.*` API 関数はたった 1 つの引数を受け取ります。
その引数は辞書オブジェクトでなければならず、API 関数に渡すそれぞれのオプションは、キーと値のペアとして辞書に格納する必要があります。
便宜上、そのオプションのことを、このドキュメントではパラメータと表現しています。

## 目次

* 基本
    * [Suika.loadPlugin()](#suikaloadplugin)
* 設定
    * [Suika.setConfig()](#suikasetconfig)
    * [Suika.getConfigCount()](#suikagetconfigcount)
    * [Suika.getConfigKey()](#suikagetconfigkey)
    * [Suika.isGlobalSaveConfig()](#suikaisglobalsaveconfig)
    * [Suika.isLocalSaveConfig()](#suikaislocalsaveconfig)
    * [Suika.getConfigType()](#suikagetconfigtype)
    * [Suika.getStringConfig()](#suikagetstringconfig)
    * [Suika.getBoolConfig()](#suikagetboolconfig)
    * [Suika.getIntConfig()](#suikagetintconfig)
    * [Suika.getFloatConfig()](#suikagetfloatconfig)
    * [Suika.getConfigAsString()](#suikagetconfigasstring)
    * [Suika.compareLocale()](#suikacomparelocale)
* 入力
    * [Suika.getMousePosX()](#suikagetmouseposx)
    * [Suika.getMousePosY()](#suikagetmouseposy)
    * [Suika.isMouseLeftPressed()](#suikaismouseleftpressed)
    * [Suika.isMouseRightPressed()](#suikaismouserightpressed)
    * [Suika.isMouseLeftClicked()](#suikaismouseleftclicked)
    * [Suika.isMouseRightClicked()](#suikaismouserightclicked)
    * [Suika.isMouseDragging()](#suikaismousedragging)
    * [Suika.isReturnKeyPressed()](#suikaisreturnkeypressed)
    * [Suika.isSpaceKeyPressed()](#suikaisspacekeypressed)
    * [Suika.isEscapeKeyPressed()](#suikaisescapekeypressed)
    * [Suika.isUpKeyPressed()](#suikaisupkeypressed)
    * [Suika.isDownKeyPressed()](#suikaisdownkeypressed)
    * [Suika.isLeftKeyPressed()](#suikaisleftkeypressed)
    * [Suika.isRightKeyPressed()](#suikaisrightkeypressed)
    * [Suika.isPageUpKeyPressed()](#suikaispageupkeypressed)
    * [Suika.isPageDownKeyPressed()](#suikaispagedownkeypressed)
    * [Suika.isControlKeyPressed()](#suikaiscontrolkeypressed)
    * [Suika.isSKeyPressed()](#suikaisskeypressed)
    * [Suika.isLKeyPressed()](#suikaislkeypressed)
    * [Suika.isHKeyPressed()](#suikaishkeypressed)
    * [Suika.isTouchCanceled()](#suikaistouchcanceled)
    * [Suika.isSwiped()](#suikaisswiped)
    * [Suika.clearInputState()](#suikaclearinputstate)
* ゲーム
    * [Suika.startCommandRepetition()](#suikastartcommandrepetition)
    * [Suika.stopCommandRepetition()](#suikastopcommandrepetition)
    * [Suika.isInCommandRepetition()](#suikaisincommandrepetition)
    * [Suika.setMessageActive()](#suikasetmessageactive)
    * [Suika.clearMessageActive()](#suikaclearmessageactive)
    * [Suika.isMessageActive()](#suikaismessageactive)
    * [Suika.startAutoMode()](#suikastartautomode)
    * [Suika.stopAutoMode()](#suikastopautomode)
    * [Suika.isAutoMode()](#suikaisautomode)
    * [Suika.startSkipMode()](#suikastartskipmode)
    * [Suika.stopSkipMode()](#suikastopskipmode)
    * [Suika.isSkipMode()](#suikaisskipmode)
    * [Suika.setSaveLoad()](#suikasetsaveload)
    * [Suika.isSaveLoadEnabled()](#suikaissaveloadenabled)
    * [Suika.setNonInterruptible()](#suikasetnoninterruptible)
    * [Suika.isNonInterruptible()](#suikaisnoninterruptible)
    * [Suika.setPenPosition()](#suikasetpenposition)
    * [Suika.getPenPositionX()](#suikagetpenpositionx)
    * [Suika.getPenPositionY()](#suikagetpenpositiony)
    * [Suika.pushForCall()](#suikapushforcall)
    * [Suika.popForReturn()](#suikapopforreturn)
    * [Suika.readCallStack()](#suikareadcallstack)
    * [Suika.writeCallStack()](#suikawritecallstack)
    * [Suika.setCallArgument()](#suikasetcallargument)
    * [Suika.getCallArgument()](#suikagetcallargument)
    * [Suika.isPageMode()](#suikaispagemode)
    * [Suika.appendBufferedMessage()](#suikaappendbufferedmessage)
    * [Suika.getBufferedMessage()](#suikagetbufferedmessage)
    * [Suika.clearBufferedMessage()](#suikaclearbufferedmessage)
    * [Suika.resetPageLine()](#suikaresetpageline)
    * [Suika.incPageLine()](#suikaincpageline)
    * [Suika.isPageTop()](#suikaispagetop)
    * [Suika.registerBGVoice()](#registerbgvoice)
    * [Suika.getBVoice()](#suikagetbgvoice)
    * [Suika.setBGVoicePlaying()](#suikasetbgvoiceplaying]
    * [Suika.isBGVoicePlaying()](#suikaisbgvoiceplaying)
    * [Suika.setChapterName()](#suikasetchaptername)
    * [Suika.getChapterName()](#suikagetchaptername)
    * [Suika.setLastMessage()](#suikasetlastmessage)
    * [Suika.getLastMessage()](#suikagetlastmessage)
    * [Suika.setTextSpeed()](#suikasettextspeed)
    * [Suika.getTextSpeed()](#suikagettextspeed)
    * [Suika.setAutoSpeed()](#suikasetautospeed)
    * [Suika.getAutoSpeed()](#suikagetautospeed)
    * [Suika.markLastEnglishTagIndex()](#suikamarklastenglishtagindex)
    * [Suika.getLastEnglishTagIndex()](#suikagetlastenglishtagindex)
    * [Suika.clearLastEnglishTagIndex()](#suikaclearlastenglishtagindex)
    * [Suika.getLastTagName()](#suikagetlasttagname)
* 画像
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
    * [Suika.getImageWidth()](#suikagetimagewidth)
    * [Suika.getImageHeight()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImage()](#suikadrawimage)
    * [Suika.drawImage3D()](#suikadrawimage3d)
    * [Suika.makeColor()](#suikamakecolor)
    * [Suika.fillImageRect()](#suikafillimagerect)
* ステージ
    * [Suika.reloadStageImages()](#suikareloadstageimages)
    * [Suika.reloadStagePositions()](#suikareloadstagepositions)
    * [Suika.getLayerX()](#suikagetlayerx)
    * [Suika.getLayerY()](#suikagetlayery)
    * [Suika.setLayerPosition()](#suikasetlayerposition)
    * [Suika.getLayerScaleX()](#suikagetlayerscalex)
    * [Suika.getLayerScaleY()](#suikagetlayerscaley)
    * [Suika.setLayerScale()](#suikasetlayerscale)
    * [Suika.getLayerRotate()](#suikagetlayerrotate)
    * [Suika.setLayerRotate()](#suikasetlayerrotate)
    * [Suika.getLayerDim()](#suikagetlayerdim)
    * [Suika.setLayerDim()](#suikasetlayerdim)
    * [Suika.getLayerAlpha()](#suikagetlayeralpha)
    * [Suika.setLayerAlpha()](#suikasetlayeralpha)
    * [Suika.setLayerBlend()](#suikasetlayerblend)
    * [Suika.setLayerFileName()](#suikasetlayerfilename)
    * [Suika.setLayerFrame()](#suikasetlayerframe)
    * [Suika.getLayerText()](#suikagetlayertext)
    * [Suika.setLayerText()](#suikasetlayertext)
    * [Suika.getSysBtnIdleImage()](#suikagetsysbtnidleimage)
    * [Suika.getSysBtnHoverImage()](#suikagetsysbtnhoverimage)
    * [Suika.clearStageBasic()](#suikaclearstagebasic)
    * [Suika.clearStage()](#suikaclearstage)
    * [Suika.chposToLayer()](#suikachpostolayer)
    * [Suika.chposToEyeLayer()](#suikachpostoeyelayer)
    * [Suika.chposToLipLayer()](#suikachpostoliplayer)
    * [Suika.layerToChpos()](#suikalayertochpos)
    * [Suika.renderStage()](#suikarenderstage)
    * [Suika.drawStageToThumb()](#suikadrawstagetothumb)
    * [Suika.getThumbImage()](#suikagetthumbimage())
    * [Suika.getFadeMethod()](#suikagetfademethod)
    * [Suika.getAccelMethod()](#suikagetaccelmethod)
    * [Suika.startFade()](#suikastartfade)
    * [Suika.getShakeOffset()](#suikagetshakeoffset)
    * [Suika.setShakeOffset()](#suikasetshakeoffset)
    * [Suika.isFadeRunning()](#suikaisfaderunning)
    * [Suika.finishFade()](#suikafinishfade)
    * [Suika.setChNameMapping()](#suikasetchnamemapping)
    * [Suika.getTalkingChpos()](#suikagettalkingchpos)
    * [Suika.setChTalking()](#suikasetchtalking)
    * [Suika.getTalkingChpos()](#suikagettalkingchpos)
    * [Suika.updateChDimByTalkingCh()](#suikaupdatechdimbytalkingch)
    * [Suika.forceChDim()](#suikaforcechdim)
    * [Suika.getChDim()](#suikagetchdim)
    * [Suika.updateChDimByTalkingCh()](#suikaupdatechdimbytalkingch)
    * [Suika.fillMessageBox()](#suikafillmessagebox)
    * [Suika.showMessageBox()](#suikashowmessagebox)
    * [Suika.getMessageBoxRect()](#suikagetmessageboxrect]
    * [Suika.fillNameBox()](#suikafillnamebox)
    * [Suika.showNameBox()](#suikashownamebox)
    * [Suika.getNameBoxRect()](#suikagetnameboxrect)
    * [Suika.showChoosebox()](#suikashowchoosebox)
    * [Suika.showAutoModeBanner()](#suikashowautomodebanner)
    * [Suika.showSkipModeBanner()](#suikashowskipmodebanner)
    * [Suika.renderImage()](#suikarenderimage)
    * [Suika.renderImage3d()](#suikarenderimage3d)
    * [Suika.setClickPosition()](#suikasetclickposition)
    * [Suika.showClick()](#suikashowclick)
    * [Suika.setClickIndex()](#suikasetclickindex]
    * [Suika.getClickRect()](#suikagetclickrect)
    * [Suika.fillChooseBoxIdleImage()](#suikafillchooseboxidleimage)
    * [Suika.fillChooseBoxHoverImage()](#suikafillchooseboxhoverimage)
    * [Suika.getChooseBoxRect()](#suikagetchooseboxrect)
    * [Suika.startKirakira()](#suikastartkirakira)
    * [Suika.renderKirakira()](#suikarenderkirakira)
* ミキサー
    * [Suika.setMixerInputFile()](#suikasetmixerinputfile)
    * [Suika.setMixerVolume()](#suikasetmixervolume)
    * [Suika.getMixerVolume()](#suikagetmixervolume)
    * [Suika.setMasterVolume()](#suikasetmastervolume)
    * [Suika.getMasterVolume()](#suikagetmastervolume)
    * [Suika.setMixerGlobalVolume()](#suikasetmixerglobalvolume)
    * [Suika.getMixerGlobalVolume()](#suikagetmixerglobalvolume)
    * [Suika.setCharacterVolume()](#suikasetcharactervolume)
    * [Suika.getCharacterVolume()](#suikagetcharactervolume)
    * [Suika.isMixerSoundFinished()](#suikaismixersoundfinished)
    * [Suika.getTrackFileName()](#suikagettrackfilename)
    * [Suika.applyCharacterVolume()](#suikaapplycharactervolume)
* システムボタン
    * [Suika.enableSysBtn()](#suikaenablesysbtn)
    * [Suika.isSysBtnVisible()](#suikaisysbtnvisible)
    * [Suika.updateSysBtnState()](#suikaupdatesysbtnstate)
    * [Suika.isSysBtnPointed()](#suikaisysbtnpointed)
    * [Suika.isSysBtnClicked()](#suikaisysbtnclicked)
* テキスト
    * [Suika.drawTextOnLayer()](#suikadrawtextonlayer)
    * [Suika.getStringWidth()](#suikagetstringwidth)
    * [Suika.getStringHeight()](#suikagetstringheight)
    * [Suika.drawGlyph()](#suikadrawglyph)
    * [Suika.createDrawMsg()](#suikacreatedrawmsg)
    * [Suika.destroyDrawMsg()](#suikadestroydrawmsg)
    * [Suika.countDrawMsgChars()](#suikacountdrawmsgchars)
    * [Suika.drawMessage()](#suikadrawmsgcommon)
    * [Suika.getDrawMsgPenPosition()](#suikagetpenpositioncommon)
    * [Suika.isEscapeSequenceChar()](#suikaisescapesequencechar)
* タグ
    * [Suika.getTagCount()](#suikagettagcount)
    * [Suika.moveToTagFile()](#suikamovetotagfile)
    * [Suika.moveToTagIndex()](#suikamovetotagindex)
    * [Suika.moveToNextTag()](#suikamovetonexttag)
    * [Suika.moveToLabelTag()](#suikamovetolabeltag)
    * [Suika.moveToMacroTag()](#suikamovetomacrotag)
    * [Suika.moveToElseTag()](#suikamovetoelsetag)
    * [Suika.moveToEndIfTag()](#suikamovetoendiftag)
    * [Suika.moveToEndMacroTag()](#suikamovetoendmacrotag)
    * [Suika.getTagFileName()](#suikagettagfilename)
    * [Suika.getTagName()](#suikagettagname)
    * [Suika.getTagPropertyCount()](#suikagettagpropertycount)
    * [Suika.getTagPropertyName()](#suikagettagpropertyname)
    * [Suika.getTagPropertyValue()](#suikagettagpropertyvalue)
    * [Suika.getTagArgBool()](#suikagettagargbool)
    * [Suika.getTagArgInt()](#suikagettagargint)
    * [Suika.getTagArgFloat()](#suikagettagargfloat)
    * [Suika.getTagArgString()](#suikagettagargstring)
    * [Suika.evaluateTag()](#suikaevaluatetag)
    * [Suika.pushTagStackIf()](#suikapushtagstackif)
    * [Suika.popTagStackIf()](#suikapoptagstackif)
    * [Suika.pushTagStackWhile()](#suikapushtagstackwhile)
    * [Suika.pushTagStackFor()](#suikapushtagstackfor)
* アニメーション
    * [Suika.loadAnimeFromFile()](#suikaloadanimefromfile)
    * [Suika.newAnimeSequence()](#suikanewanimesequence)
    * [Suika.addAnimeSequencePropertyF()](#suikaaddanimesequencepropertyf)
    * [Suika.addAnimeSequencePropertyI()](#suikaaddanimesequencepropertyi)
    * [Suika.startLayerAnime()](#suikastartlayeranime)
    * [Suika.isAnimeRunning()](#suikaisanimerunning)
    * [Suika.isAnimeFinishedForLayer()](#suikaisanimefinishedforlayer)
    * [Suika.updateAnimeFrame()](#suikaupdateanimeframe)
    * [Suika.loadEyeImageIfExists()](#suikaloadeyeimageifexists)
    * [Suika.reloadEyeAnime()](#suikareloadeyeanime)
    * [Suika.runLipAnime()](#suikarunlipanime)
    * [Suika.stopLipAnime()](#suikastoplipanime)
    * [Suika.clearLayerAnimeSequence()](#suikaclearlayeranimesequence)
    * [Suika.clearAllAnimeSequence()](#suikaclearallanimesequnce)
* 変数
    * [Suika.setVariableInt()](#suikasetvariableint)
    * [Suika.setVariableFloat()](#suikasetvariablefloat)
    * [Suika.setVariableString()](#suikasetvariablestring)
    * [Suika.getVariableInt()](#suikagetvariableint)
    * [Suika.getVariableFloat()](#suikagetvariablefloat)
    * [Suika.getVariableString()](#suikagetvariablestring)
    * [Suika.unsetVariable()](#suikaunsetvariable)
    * [Suika.unsetLocalVariables()](#suikaunsetlocalvariables)
    * [Suika.makeVariableGlobal()](#suikamakevariableglobal)
    * [Suika.isGlobalVariable()](#suikaisglobalvariable)
    * [Suika.getVariableCount()](#suikagetvariablecount)
    * [Suika.getVariableName()](#suikagetvariablename)
    * [Suika.checkVariableExists()](#suikacheckvariableexists)
    * [Suika.expandStringWithVariable()](#suikaexpandstringwithvariable)
* セーブ
    * [Suika.executeSaveGlobal()](#suikaexecutesaveglobal)
    * [Suika.executeLoadGlobal()](#suikaexecuteloadglobal)
    * [Suika.executeSaveLocal()](#suikaexecutesavelocal)
    * [Suika.executeLoadLocal()](#suikaexecuteloadlocal)
    * [Suika.checkSaveExists()](#suikachecksaveexists)
    * [Suika.deleteLocalSave()](#suikadeletelocalsave)
    * [Suika.deleteGlobalSave()](#suikadeleteglobalsave)
    * [Suika.checkRightAfterLoad()](#suikacheckrightafterload)
    * [Suika.getSaveTimestamp()](#suikagetsavetimestamp)
    * [Suika.getLatestSaveIndex()](#suikagetlatestsaveindex)
    * [Suika.getSaveChapterName()](#suikagetsavechaptername)
    * [Suika.getSaveLastMessage()](#suikagetsavelastmessage)
    * [Suika.getSaveThumbnail()](#suikagetsavethumbnail)
* 履歴
    * [Suika.clearHistory()](#suikaclearhistory)
    * [Suika.addHistory()](#suikaaddhistory)
    * [Suika.getHistoryCount()](#suikagethistorycount)
    * [Suika.getHistoryName()](#suikagethistoryname)
    * [Suika.getHistoryMessage()](#suikagethistorymessage)
    * [Suika.getHistoryVoice()](#suikagethistoryvoice)
* 既読
    * [Suika.loadSeen()](#suikaloadseen)
    * [Suika.saveSeen()](#suikasaveseen)
    * [Suika.getSeenFlags()](#suikagetseenflags)
    * [Suika.setSeenFlags()](#suikasetseenflags)
* GUI
    * [Suika.loadGUIFile()](#suikaloadguifile)
    * [Suika.startGUI()](#suikastartgui)
    * [Suika.stopGUI()](#suikastopgui)
    * [Suika.isGUIRunning()](#suikaisguirunning)
    * [Suika.isGUIFinished()](#suikaisguifinished)
    * [Suika.getGUIResultLabel()](#suikagetguiresultlabel)
    * [Suika.isGUIResultTitle()](#suikaisguiresulttitle)
    * [Suika.checkIfSavedInGUI()](#suikacheckifsavedingui)
    * [Suika.checkIfLoadedInGui()](#suikacheckifloadedingui)
    * [Suika.checkRightAfterSysGUI()](#suikacheckrightaftersysgui)
* HAL
    * [Suika.getMillisec()](#suikagetmillisec)
    * [Suika.checkFileExists()](#suikacheckfileexists)
    * [Suika.readFileContent()](#suikareadfilecontent)
    * [Suika.writeSaveData()](#suikawritesavedata)
    * [Suika.readSaveData()](#suikareadsavedata)
    * [Suika.installAPI()](#suikainstallapi)
    * [Suika.installTag()](#suikainstalltag)
    * [Suika.getVmInt()](#suikagetvmint)
    * [Suika.setVmInt()](#suikasetvmint)
    * [Suika.callVmFunction()](#suikacallvmfunction)
    * [Suika.playVideo()](#suikaplayvideo)
    * [Suika.stopVideo()](#suikastopvideo)
    * [Suika.isVideoPlaying()](#suikaisvideoplaying)
    * [Suika.isFullScreenSupported()](#suikaisfullscreensupported)
    * [Suika.enterFullScreenMode()](#suikaenterfullscreenmode)
    * [Suika.logInfo()](#suikaloginfo)
    * [Suika.logWarn()](#suikalogwarn)
    * [Suika.logError()](#suikalogerror)
    * [Suika.speakText()](#suikaspeaktext)

---

## Suika.loadPlugin()

プラグインを読み込みます。

この API だけは辞書ではない引数を受け取ります。

### パラメータ (直接)

| パラメータ | 型   | 説明     |
|-----------|--------|-----------------|
| name      | String | プラグイン名。    |

### 戻り値

戻り値なし。

---

## Suika.setConfig()

設定をセットします。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | 設定のキー。                         |
| value     | String | 設定の値。                       |

### 戻り値

戻り値なし。

---

## Suika.getConfigCount()

設定キーの数を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

設定キーの数を表す整数。

---

## Suika.getConfigKey()

設定キーのインデックスを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| index     | Integer | 設定のインデックス。                         |

### 戻り値

指定したインデックスにある設定キーを表す文字列。

---

## Suika.isGlobalSaveConfig()

設定キーがグローバルセーブデータに保存されるかどうかを確認します。

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定がグローバルセーブされるかどうかを表す真偽値。

---

## Suika.isLocalSaveConfig()

設定キーがローカルセーブデータに保存されるかどうかを確認します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定がローカルセーブされるかどうかを表す真偽値。

---

## Suika.getConfigType()

設定値の型を取得します。("s", "b", "i", "f")

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

次のいずれかの文字列。

| 値      | 意味                  |
|------------|--------------------------|
| "s"        | 設定は文字列です。        |
| "b"        | 設定は真偽値です。       |
| "i"        | 設定は整数です。       |
| "f"        | 設定は浮動小数点数です。         |

---

## Suika.getStringConfig()

文字列の設定値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定の文字列値。

---

## Suika.getBoolConfig()

真偽値の設定値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定の真偽値。

---

## Suika.getIntConfig()

整数の設定値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定の整数値。

---

## Suika.getFloatConfig()

浮動小数点数の設定値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

設定の浮動小数点値。

---

## Suika.getConfigAsString()

設定値を文字列として取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | キー名。                                  |

### 戻り値

文字列化された設定値。

---

## Suika.compareLocale()

指定したロケールが現在のロケールと同じか確認します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| locale    | String | ロケール名。                               |


### 戻り値

指定したロケールが現在のロケールと一致するかどうかを表す真偽値。

---

## Suika.getMousePosX()

マウスの X 位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

現在のマウス X 座標を表す整数。

---

## Suika.getMousePosY()

マウスの Y 位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

現在のマウス Y 座標を表す整数。

---

## Suika.isMouseLeftPressed()

マウスの左ボタンが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

左ボタンが現在押し下げられているかどうかを表す真偽値。

---

## Suika.isMouseRightPressed()

マウスの右ボタンが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

右ボタンが現在押し下げられているかどうかを表す真偽値。

---

## Suika.isMouseLeftClicked()

マウスの左ボタンが押されてから離されたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

現在のフレームで左クリックが発生したかどうかを表す真偽値。

---

## Suika.isMouseRightClicked()

マウスの右ボタンが押されてから離されたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

現在のフレームで右クリックが発生したかどうかを表す真偽値。

---

## Suika.isMouseDragging()

マウスがドラッグ中か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

ボタンが押されている間にマウスが移動しているかどうかを表す真偽値。

---

## Suika.isReturnKeyPressed()

Return キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isSpaceKeyPressed()

Space キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isEscapeKeyPressed()

Escape キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isUpKeyPressed()

上キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isDownKeyPressed()

下キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isLeftKeyPressed()

左キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isRightKeyPressed()

右キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isPageUpKeyPressed()

PageUp キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isPageDownKeyPressed()

PageDown キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isControlKeyPressed()

Control キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isSKeyPressed()

S キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isLKeyPressed()

L キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isHKeyPressed()

H キーが押されているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isTouchCanceled()

タッチがキャンセルされたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isSwiped()

スワイプされたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.clearInputState()

現在のフレームで以降の入力処理を避けるため、入力状態をクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。


---

## Suika.startCommandRepetition()

複数フレームにまたがるコマンド実行を開始します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.stopCommandRepetition()

複数フレームにまたがるコマンド実行を停止します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isInCommandRepetition()

複数フレームにまたがるコマンド実行中かどうかを確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.setMessageActive()

メッセージ表示状態をアクティブにします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.clearMessageActive()

メッセージ表示状態をリセットします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isMessageActive()

メッセージ表示状態がセットされているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.startAutoMode()

オートモードを開始します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.stopAutoMode()

オートモードを停止します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isAutoMode()

オートモード中かどうかを確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.startSkipMode()

スキップモードを開始します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.stopSkipMode()

スキップモードを停止します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isSkipMode()

スキップモード中かどうかを確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.setSaveLoad()

セーブ/ロードの有効設定をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                       |
|-----------|---------|-----------------------------------|
| enable    | Boolean | セーブとロードを有効にするかどうか。  |

### 戻り値

戻り値なし。

---

## Suika.isSaveLoadEnabled()

セーブ/ロードの有効設定を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.setNonInterruptible()

割り込み不可モード設定をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| enable    | Boolean | 割り込み不可モード。    |

### 戻り値

戻り値なし。

---

## Suika.isNonInterruptible()

割り込み不可モード設定を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.setPenPosition()

テキスト描画用のペン位置をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| x         | Integer | X 座標。         |
| y         | Integer | Y 座標。         |

### 戻り値

戻り値なし。

---

## Suika.getPenPositionX()

ペンの X 位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数値。

---

## Suika.getPenPositionY()

ペンの Y 位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数値。

---

## Suika.pushForCall()

戻り先をコールスタックにプッシュします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| file      | String  | スクリプトファイル名。     |
| index     | Integer | コマンドインデックス。        |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.popForReturn()

戻り先をコールスタックからポップします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

次の要素を含む辞書を返します:

* obj.file: ファイル名
* obj.index: タグインデックス

---

## Suika.readCallStack()

指定したインデックスのコールスタック要素を読み取ります。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| sp        | Integer | スタック要素インデックス。  |

### 戻り値

次の要素を含む辞書を返します:

* obj.file: ファイル名
* obj.index: タグインデックス

---

## Suika.writeCallStack()

指定したインデックスのコールスタック要素を書き込みます。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| sp        | Integer | スタック要素インデックス。  |
| file      | String  | スクリプトファイル名。     |
| index     | Integer | タグインデックス。            |

### 戻り値

戻り値なし。

---

## Suika.setCallArgument()

GUI またはアニメーション用の呼び出し引数をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| index     | Integer | 引数インデックス。       |
| value     | String  | 引数の値。       |

### 戻り値

真偽値。

---

## Suika.getCallArgument()

呼び出し引数を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| index     | Integer | 引数インデックス。       |

### 戻り値

文字列値。

---

## Suika.isPageMode()

スクリプトのページモードが有効か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値を返します。

---

## Suika.appendBufferedMessage()

ページモードのバッファ文字列に文字列を追加します。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| message   | String  | メッセージ。              |

### 戻り値

戻り値なし。

---

## Suika.getBufferedMessage()

ページモードのバッファ文字列を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

文字列を返します。

---

## Suika.clearBufferedMessage()

ページモードのバッファ文字列をクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.resetPageLine()

ページ内のメッセージ行数をリセットします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.incPageLine()

ページ内の行数を増やします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isPageTop()

ページの先頭行にいるか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.registerBGVoice()

BGVoice を登録します。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| file      | String  | BGVoice ファイル。         |

### 戻り値

戻り値なし。

---

## Suika.getBVoice()

BGVoice を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

ファイル名文字列を返します。

---

## Suika.setBGVoicePlaying()

BGVoice の再生状態をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| isPlaying | Boolean | 状態。                |

### 戻り値

戻り値なし。

---

## Suika.isBGVoicePlaying()

BGVoice が再生中か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値を返します。

---

## Suika.setChapterName()

チャプター名をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| name      | String  | チャプター名。         |

### 戻り値

戻り値なし。

---

## Suika.getChapterName()

チャプター名を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

文字列を返します。

---

## Suika.setLastMessage()

最後のメッセージをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| message   | String  | メッセージ。              |
| isAppend  | Boolean | 追加または置換。    |

### 戻り値

戻り値なし。

---

## Suika.getLastMessage()

最後のメッセージを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

文字列を返します。

---

## Suika.setTextSpeed()

テキスト速度をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| speed     | Float   | テキスト速度。           |

### 戻り値

戻り値なし。

---

## Suika.getTextSpeed()

テキスト速度を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

浮動小数点数を返します。

---

## Suika.setAutoSpeed()

オートモード速度をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| speed     | Float   | オート速度。           |

### 戻り値

戻り値なし。

---

## Suika.getAutoSpeed()

オート速度を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

浮動小数点数を返します。

---

## Suika.markLastEnglishTagIndex()

最後の English インデックスを記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.getLastEnglishTagIndex()

最後の English インデックスを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数を返します。

---

## Suika.clearLastEnglishTagIndex()

最後の English インデックスをクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.getLastTagName()

最後のタグ名を取得します。


### パラメータ (辞書)

パラメータなし。

### 戻り値

文字列を返します。

---

## Suika.createImageFromFile()

ファイルから画像を読み込みます。

### パラメータ (辞書)

| パラメータ | 型   | 説明                   |
|-----------|--------|-------------------------------|
| file      | String | 画像ファイルへのパス。       |

### 戻り値

画像オブジェクト。失敗時は null。

---

## Suika.createImage()

新しい空の画像を作成します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                   |
|-----------|---------|-------------------------------|
| width     | Integer | 画像の幅。           |
| height    | Integer | 画像の高さ。          |

### 戻り値

画像オブジェクト。

---

## Suika.getImageWidth()

画像の幅を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                   |
|-----------|--------|-------------------------------|
| img       | Object | 画像オブジェクト。                 |

### 戻り値

幅を表す整数。

---

## Suika.getImageHeight()

画像の高さを取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                   |
|-----------|--------|-------------------------------|
| image     | Object | 画像オブジェクト。                 |

### 戻り値

高さを表す整数。

---

## Suika.destroyImage()

画像を破棄し、そのメモリを解放します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                   |
|-----------|--------|-------------------------------|
| image     | Object | 破棄する画像オブジェクト。      |

### 戻り値

戻り値なし。

---

## Suika.drawImage()

画像を別の画像へコピーします (ブレンドなし)。

### パラメータ (辞書)

| パラメータ  | 型    | 説明                      |
|------------|---------|----------------------------------|
| dstImage   | Object  | 描画先画像。               |
| dstLeft    | Integer | 描画先の X 座標。     |
| dstTop     | Integer | 描画先の Y 座標。     |
| srcImage   | Object  | 描画元画像。                    |
| dstWidth   | Integer | 描画する幅。                   |
| dstHeight  | Integer | 描画する高さ。                  |
| srcLeft    | Integer | 描画元の X 座標。          |
| srcTop     | Integer | 描画元の Y 座標。          |
| alpha      | Integer | 0-255                            |
| blend      | Integer | ブレンド種別。                   |

### ブレンド種別

| 型                | 説明                        |
|---------------------|------------------------------------|
| Suika.BLEND_COPY    | コピー。                              |
| Suika.BLEND_ALPHA   | アルファブレンド。                    |
| Suika.BLEND_ADD     | 加算ブレンド。                      |
| Suika.BLEND_SUB     | 減算ブレンド。                      |
| Suika.BLEND_DIM     | RGB 50% アルファブレンド。            |
| Suika.BLEND_GLYPH   | 通常グリフ用のアルファブレンド。  |
| Suika.BLEND_EMOJI   | 絵文字グリフ用のアルファブレンド。   |

### 戻り値

戻り値なし。

---

## Suika.drawImage3D()

画像を別の画像へコピーします (ブレンドなし)。

### パラメータ (辞書)

| パラメータ  | 型    | 説明                      |
|------------|---------|----------------------------------|
| dstImage   | Object  | 描画先画像。               |
| x1         | Integer | 描画先の x1 座標。    |
| y1         | Integer | 描画先の y1 座標。    |
| x2         | Integer | 描画先の x2 座標。    |
| y2         | Integer | 描画先の y2 座標。    |
| x3         | Integer | 描画先の x3 座標。    |
| y3         | Integer | 描画先の y3 座標。    |
| x4         | Integer | 描画先の x4 座標。    |
| y5         | Integer | 描画先の y4 座標。    |
| srcImage   | Object  | 描画元画像。                    |
| srcLeft    | Integer | 描画元の X 座標。          |
| srcTop     | Integer | 描画元の Y 座標。          |
| srcWidth   | Integer | 描画元の幅。                 |
| srcHeight  | Integer | 描画元の高さ。                |
| alpha      | Integer | 0-255                            |
| blend      | Integer | ブレンド種別。                   |

### ブレンド種別

| 型                | 説明                        |
|---------------------|------------------------------------|
| Suika.BLEND_ALPHA   | アルファブレンド。                    |
| Suika.BLEND_ADD     | 加算ブレンド。                      |
| Suika.BLEND_SUB     | 減算ブレンド。                      |
| Suika.BLEND_DIM     | RGB 50% アルファブレンド。            |

### 戻り値

戻り値なし。

---

## Suika.drawImageAlpha()

アルファブレンドで画像を描画します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | 描画先画像。               |
| dstLeft   | Integer | 描画先の X 座標。     |
| dstTop    | Integer | 描画先の Y 座標。     |
| dstWidth  | Integer | 描画する幅。                   |
| dstHeight | Integer | 描画する高さ。                  |
| srcImage  | Object  | 描画元画像。                    |
| srcLeft   | Integer | 描画元の X 座標。          |
| srcTop    | Integer | 描画元の Y 座標。          |
| alpha     | Integer | アルファ値 (`0`-`255`)。         |

### 戻り値

戻り値なし。

---

## Suika.drawImageAdd()

加算ブレンドで画像を描画します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | 描画先画像。               |
| dstLeft   | Integer | 描画先の X 座標。     |
| dstTop    | Integer | 描画先の Y 座標。     |
| dstWidth  | Integer | 描画する幅。                   |
| dstHeight | Integer | 描画する高さ。                  |
| srcImage  | Object  | 描画元画像。                    |
| srcLeft   | Integer | 描画元の X 座標。          |
| srcTop    | Integer | 描画元の Y 座標。          |
| alpha     | Integer | アルファ値 (`0`-`255`)。         |

### 戻り値

戻り値なし。

---

## Suika.drawImageSub()

減算ブレンドで画像を描画します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | 描画先画像。               |
| dstLeft   | Integer | 描画先の X 座標。     |
| dstTop    | Integer | 描画先の Y 座標。     |
| dstWidth  | Integer | 描画する幅。                   |
| dstHeight | Integer | 描画する高さ。                  |
| srcImage  | Object  | 描画元画像。                    |
| srcLeft   | Integer | 描画元の X 座標。          |
| srcTop    | Integer | 描画元の Y 座標。          |
| alpha     | Integer | アルファ値 (`0`-`255`)。         |

### 戻り値

戻り値なし。

---

## Suika.makeColor()

RGBA 成分からピクセル値を作成します。

### パラメータ (辞書)

| パラメータ | 型    | 説明      |
|-----------|---------|------------------|
| r         | Integer | 赤 (0-255)。     |
| g         | Integer | 緑 (0-255)。   |
| b         | Integer | 青 (0-255)。    |
| a         | Integer | アルファ (0-255)。   |

### 戻り値

ピクセル値。

---

## Suika.fillImageRect()

画像上の矩形領域を色で塗りつぶします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                         |
|-----------|---------|-------------------------------------|
| image     | Object  | 対象画像。                       |
| left      | Integer | X 座標。                       |
| top       | Integer | Y 座標。                       |
| width     | Integer | 幅。                              |
| height    | Integer | 高さ。                             |
| color     | Integer | Suika.makeColor() で作成した色。 |

### 戻り値

戻り値なし。

---

## Suika.reloadStageImages()

設定に基づいてステージ画像を再読み込みします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.reloadStagePositions()

設定に基づいてステージ位置を再読み込みします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.getLayerX()

指定したレイヤーの現在位置を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

座標の整数値。

---

## Suika.getLayerY()

指定したレイヤーの現在位置を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

座標の整数値。

---

## Suika.setLayerPosition()

指定したレイヤーの位置をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| x         | Integer | X 座標。              |
| y         | Integer | Y 座標。              |

### 戻り値

戻り値なし。

---

## Suika.getLayerScaleX()

指定したレイヤーの X スケール係数を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

スケールの浮動小数点値。

---

## Suika.getLayerScaleY()

指定したレイヤーの Y スケール係数を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

スケールの浮動小数点値。

---

## Suika.setLayerScale()

指定したレイヤーのスケール係数をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| scale_x   | Float   | 水平スケール。          |
| scale_y   | Float   | 垂直スケール。            |

### 戻り値

戻り値なし。

---

## Suika.getLayerRotate()

指定したレイヤーの回転角を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

浮動小数点数を返します。

---

## Suika.setLayerRotate()

指定したレイヤーの回転角をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| rot       | Float   | ラジアン単位の回転角。 |

### 戻り値

戻り値なし。

---

## Suika.getLayerDim()

指定したレイヤーの暗転状態を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

真偽値を返します。

---

## Suika.setLayerDim()

指定したレイヤーの暗転状態をセットします。

### パラメータ (辞書) (設定)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| dim       | Boolean | レイヤーを暗くするかどうか。  |

### 戻り値

戻り値なし。

---

## Suika.getLayerAlpha()

指定したレイヤーの透明度を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |

### 戻り値

整数を返します。

---

## Suika.setLayerAlpha()

指定したレイヤーの透明度をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| alpha     | Integer | アルファ値 (0-255)。       |

### 戻り値

戻り値なし。

---

## Suika.setLayerBlend()

レイヤーのブレンドモードをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| blend     | Integer | ブレンドモード (Alpha、Add、Sub)。 |

### 戻り値

戻り値なし。

---

## Suika.setLayerFile()

レイヤーに表示するファイルをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| file_name | String  | 画像ファイルへのパス。    |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.setLayerFrame()

目パチとリップシンク用のフレームインデックスをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | ステージレイヤーのインデックス。  |
| frame     | Integer | フレームインデックス。               |

### 戻り値

戻り値なし。

---

## Suika.getLayerText()

テキストレイヤーに表示されている文字列を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | テキストレイヤーのインデックス。   |

### 戻り値

文字列を返します。

---

## Suika.setLayerText()

テキストレイヤーに表示する文字列をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | テキストレイヤーのインデックス。   |
| text      | String  | 設定するテキストメッセージ。       |

### 戻り値

戻り値なし。

---

## Suika.getSysBtnIdleImage()

システムボタンの通常時画像を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

画像オブジェクトを返します。

---

## Suika.getSysBtnHoverImage()

システムボタンのホバー時画像を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

画像オブジェクトを返します。

---

## Suika.clearStageBasic()

基本レイヤーをクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

画像オブジェクトを返します。

---

## Suika.clearStage()

ステージをクリアして初期状態にします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

画像オブジェクトを返します。

---

## Suika.chposToLayer()

キャラクター位置をステージレイヤーインデックスに変換します。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| chpos     | Integer | キャラクター位置。   |

### 戻り値

整数を返します。

---

## Suika.chposToEyeLayer()

キャラクター位置をステージレイヤーインデックスに変換します (キャラクターの目)。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| chpos     | Integer | キャラクター位置。   |

### 戻り値

整数を返します。

---

## Suika.chposToLipLayer()

キャラクター位置をステージレイヤーインデックスに変換します (キャラクターの口)。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| chpos     | Integer | キャラクター位置。   |

### 戻り値

整数を返します。

---

## Suika.layerToChpos()

ステージレイヤーインデックスをキャラクター位置に変換します。

### パラメータ (辞書)

| パラメータ | 型    | 説明           |
|-----------|---------|-----------------------|
| layer     | Integer | レイヤーインデックス。          |

### 戻り値

整数を返します。

---

## Suika.renderStage()

すべてのステージレイヤーでステージを描画します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.startFade()

トランジション効果を開始します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                  |
|-----------|---------|----------------------------------------------|
| desc      | Array   | フェード記述子。                             |
| method    | String  | フェード方式。                               |
| time      | Float   | 秒単位の時間。                         |
| ruleImage | Object  | ルール画像オブジェクト (任意)。                |

### 戻り値

真偽値。

---

## Suika.getShakeOffset()

shake コマンド用のオフセットを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

次の要素を含むオブジェクト:
* x
* y

---

## Suika.setShakeOffset()

shake コマンド用のオフセットをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明    |
|-----------|---------|----------------|
| x         | Integer | X オフセット。      |
| y         | Integer | Y オフセット。      |

### 戻り値

戻り値なし。

---

## Suika.isFadeRunning()

フェードが実行中か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.finishFade()

フェード効果を即座に終了します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.setChNameMapping()

キャラクター位置に対するキャラクター名インデックスを指定します。

### パラメータ (辞書)

| パラメータ   | 型    | 説明                |
|-------------|---------|----------------------------|
| chpos       | Integer | キャラクター位置。        |
| chNameIndex | Integer | キャラクター名インデックス。      |

### 戻り値

戻り値なし。

---

## Suika.getTalkingChpos()

現在話しているキャラクターの位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数を返します。

---

## Suika.setChTalking()

話者キャラクターをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| chpos     | Integer | キャラクター位置。        |

### 戻り値

戻り値なし。

---

## Suika.getTalkingChpos()

話者キャラクター位置を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数を返します。

---

## Suika.updateChDimByTalkingCh()

話者に基づいてキャラクターの暗転を自動更新します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.forceChDim()

キャラクターの暗転を手動で更新します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| chpos     | Integer | キャラクター位置。        |
| dim       | Boolean | 暗くするかどうか。                |

### 戻り値

戻り値なし。

---

## Suika.getChDim()

暗転状態を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| chpos     | Integer | キャラクター位置。        |

### 戻り値

真偽値を返します。

---

## Suika.fillNameBox()

名前ボックス画像で名前ボックスを塗りつぶします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.getNameBoxRect()

名前ボックスの位置とサイズを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

オブジェクト。

* x
* y
* w
* h

---

## Suika.showNameBox()

名前ボックスを表示または非表示にします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| show      | Boolean | 表示または非表示。              |

### 戻り値

戻り値なし。

---

## Suika.fillMessageBox()

メッセージボックス画像でメッセージボックスを塗りつぶします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.showMessageBox()

メッセージボックスを表示または非表示にします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| show      | Boolean | ボックスを表示するかどうか。   |

### 戻り値

戻り値なし。

---

## Suika.getMessageBoxRect()

メッセージボックスの矩形を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

次の要素を含むオブジェクト:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClickPosition()

クリックアニメーションの位置をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| x         | Integer | X 位置。                |
| y         | Integer | Y 位置。                |

### 戻り値

戻り値なし。

---

## Suika.showClick()

クリックアニメーションを表示または非表示にします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| show      | Boolean | 表示または非表示。              |

### 戻り値

戻り値なし。

---

## Suika.setClickIndex()

クリックアニメーションフレームのインデックスをセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | フレームインデックス。               |

### 戻り値

戻り値なし。

---

## Suika.getClickRect()

クリックアニメーションの矩形を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

次の要素を含むオブジェクト:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

選択肢ボックスの通常時画像で通常時レイヤーを塗りつぶします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 選択肢ボックスのインデックス。          |

### 戻り値

戻り値なし。

---

## Suika.fillChooseBoxHoverImage()

選択肢ボックスのホバー時画像でホバー時レイヤーを塗りつぶします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 選択肢ボックスのインデックス。          |

### 戻り値

戻り値なし。

---

## Suika.showChoosebox()

選択肢ボックスを表示または非表示にします。

### パラメータ (辞書)

| パラメータ  | 型    | 説明                 |
|------------|---------|-----------------------------|
| index      | Integer | 選択肢ボックスのインデックス (`0`-`7`)。 |
| showIdle   | Boolean | 通常状態を表示します。            |
| showHover  | Boolean | ホバー状態を表示します。           |

### 戻り値

戻り値なし。

---

## Suika.getChooseBoxRect()

選択肢ボックスの矩形を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

次の要素を含むオブジェクト:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

オートモードバナーを表示または非表示にします。

### パラメータ (辞書)

| パラメータ  | 型    | 説明                 |
|------------|---------|-----------------------------|
| show       | Boolean | 表示または非表示。               |

### 戻り値

戻り値なし。

---

## Suika.showSkipModeBanner()

スキップモードバナーを表示または非表示にします。

### パラメータ (辞書)

| パラメータ  | 型    | 説明                 |
|------------|---------|-----------------------------|
| show       | Boolean | 表示または非表示。               |

### 戻り値

戻り値なし。

---

## Suika.renderImage()

画像を画面へ直接描画します。

通常の描画ではステージレイヤーの使用を検討してください。
この API はエフェクトに便利です。

### パラメータ (辞書)

| パラメータ | 省略可    | 型    | 説明                                |
|-----------|--------------|---------|--------------------------------------------|
| dstLeft   | いいえ           | Integer | 描画先左上の X 位置。           |
| dstTop    | いいえ           | Integer | 描画先左上の Y 位置。           |
| image     | いいえ           | Object  | 画像。                                     |
| srcLeft   | いいえ           | Integer | 描画元左上の X 位置。                |
| srcTop    | いいえ           | Integer | 描画元左上の Y 位置。                |
| srcWidth  | いいえ           | Integer | 描画元の幅。                              |
| srcHeight | いいえ           | Integer | 描画元の高さ。                             |
| alpha     | いいえ           | Integer | アルファ値 (`0`-`255`)。                   |
| blend     | いいえ           | Integer | ブレンド種別。                                |

### ブレンド種別

| 名前                 | 説明       |
|----------------------|-------------------|
| Suika.BLEND_ALPHA    | アルファブレンド。   |
| Suika.BLEND_ADD      | 加算ブレンド。     |
| Suika.BLEND_SUB      | 減算ブレンド。     |

### 戻り値

戻り値なし。

---

## Suika.renderImage3d()

3D 変換を使って画像を画面へ直接描画します。

通常の描画ではステージレイヤーの使用を検討してください。
この API はエフェクトに便利です。

### パラメータ (辞書)

| パラメータ | 省略可    | 型    | 説明                                |
|-----------|--------------|---------|--------------------------------------------|
| x1        | いいえ           | Integer | 描画先頂点 1 の X 位置。           |
| y1        | いいえ           | Integer | 描画先頂点 1 の Y 位置。           |
| x2        | いいえ           | Integer | 描画先頂点 2 の X 位置。           |
| y2        | いいえ           | Integer | 描画先頂点 2 の Y 位置。           |
| x3        | いいえ           | Integer | 描画先頂点 3 の X 位置。           |
| y3        | いいえ           | Integer | 描画先頂点 3 の Y 位置。           |
| x4        | いいえ           | Integer | 描画先頂点 4 の X 位置。           |
| y4        | いいえ           | Integer | 描画先頂点 4 の Y 位置。           |
| tex       | いいえ           | Object  | 画像。                                     |
| srcLeft   | いいえ           | Integer | 描画元左上の X 位置。                |
| srcTop    | いいえ           | Integer | 描画元左上の Y 位置。                |
| srcWidth  | いいえ           | Integer | 描画元の幅。                              |
| srcHeight | いいえ           | Integer | 描画元の高さ。                             |
| alpha     | いいえ           | Integer | アルファ値 (`0`-`255`)。                   |

### 戻り値

戻り値なし。

---

## Suika.startKirakira()

キラキラ効果を開始します。

キラキラ効果は、マウスカーソルがクリックされた画面位置に表示されるアニメーションです。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.renderKirakira()

キラキラ効果を描画します。

---

## Suika.setMixerInputFile()

指定したミキサートラックで音声ファイルを再生します。

### パラメータ (辞書)

| パラメータ | 省略可    | 型    | 説明                                |
|-----------|--------------|---------|--------------------------------------------|
| track     | いいえ           | String  | ミキサートラック名。                          |
| file      | いいえ           | String  | 音声ファイルへのパス。                    |
| isLooped  | はい (`false`) | Boolean | 再生をループするかどうか。              |

### トラック名

| 名前   | 説明              |
|--------|--------------------------|
| bgm    | BGM トラック。  |
| se     | 効果音トラック。      |
| voice  | キャラクターボイストラック。   |
| sys    | システム音トラック。      |

### 戻り値

ファイルを正常に開けたかどうかを表す真偽値。

---

## Suika.setMixerVolume()

指定したミキサートラックの音量をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | String  | ミキサートラック名。                          |
| vol       | Float   | 音量レベル (0.0 から 1.0)。                 |
| span      | Float   | 秒単位のフェード時間。                  |

### トラック名

| 名前   | 説明              |
|--------|--------------------------|
| bgm    | BGM トラック。  |
| se     | 効果音トラック。      |
| voice  | キャラクターボイストラック。   |
| sys    | システム音トラック。      |

### 戻り値

戻り値なし。

---

## Suika.getMixerVolume()

指定したミキサートラックの音量を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | String  | ミキサートラック名。                          |
| volume    | Float   | 音量レベル (0.0 から 1.0)。                 |
| span      | Float   | 秒単位のフェード時間。                  |

### トラック名

| 名前   | 説明              |
|--------|--------------------------|
| bgm    | BGM トラック。  |
| se     | 効果音トラック。      |
| voice  | キャラクターボイストラック。   |
| sys    | システム音トラック。      |

### 戻り値

浮動小数点数を返します。

---

## Suika.setMasterVolume()

全トラックに影響するマスター音量をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| volume    | Float   | マスター音量レベル (0.0 から 1.0)。          |

### 戻り値

戻り値なし。

---

## Suika.getMasterVolume()

全トラックに影響するマスター音量を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

浮動小数点数を返します。

---

## Suika.setMixerGlobalVolume()

トラックのグローバル音量をセットします (設定でよく使用されます)。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | String  | ミキサートラック名。                          |
| vol       | Float   | グローバル音量レベル。                       |

### トラック名

| 名前   | 説明              |
|--------|--------------------------|
| bgm    | BGM トラック。  |
| se     | 効果音トラック。      |
| voice  | キャラクターボイストラック。   |
| sys    | システム音トラック。      |

### 戻り値

戻り値なし。

---

## Suika.getMixerGlobalVolume()

トラックのグローバル音量を取得します (設定でよく使用されます)。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | String  | ミキサートラック名。                          |

### トラック名

| 名前   | 説明              |
|--------|--------------------------|
| bgm    | BGM トラック。  |
| se     | 効果音トラック。      |
| voice  | キャラクターボイストラック。   |
| sys    | システム音トラック。      |

### 戻り値

浮動小数点数を返します。

---

## Suika.setCharacterVolume()

指定したキャラクターボイスの音量をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| index     | Integer | キャラクター名インデックス。                      |
| vol       | Float   | 音量レベル。                              |

### 戻り値

戻り値なし。

---

## Suika.getCharacterVolume()

指定したキャラクターボイスの音量を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| ch_index  | Integer | キャラクター名インデックス。                      |

### 戻り値

浮動小数点数を返します。

---

## Suika.isMixerSoundFinished()

指定したトラックの再生が終了したか確認します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | ミキサートラックインデックス。                         |

### 戻り値

真偽値。

---

## Suika.getTrackFileName()

トラックに現在読み込まれている音声のファイル名を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | ミキサートラックインデックス。                         |

### 戻り値

ファイルパスを表す文字列。

---

## Suika.applyCharacterVolume()

キャラクター固有の音量設定を VOICE トラックに適用します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| ch        | Integer | キャラクター名インデックス。                      |

### 戻り値

戻り値なし。

---

## Suika.enableSysBtn()

システムボタンを制御します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| isEnabled | Boolean | システムボタンを有効にするかどうか。           |

### 戻り値

戻り値なし。

---

## Suika.isSysBtnEnabled()

システムボタンが有効か確認します。

### パラメータ

パラメータなし。

### 戻り値

真偽値を返します。 

---

## Suika.updateSysBtnState()

システムボタンのマウストラッキングを更新します。

### パラメータ

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isSysBtnPointed()

システムボタンがポイントされているか確認します。

### パラメータ

パラメータなし。

### 戻り値

真偽値を返します。

---

## Suika.isSysBtnClicked()

システムボタンがクリックされたか確認します。

### パラメータ

パラメータなし。

### 戻り値

真偽値を返します。

---

## Suika.drawTextOnLayer()

指定したレイヤーにテキストを描画します。

### パラメータ (辞書)

| パラメータ    | 型    | 説明               |
|--------------|---------|---------------------------|
| layer        | Integer | 対象ステージレイヤーのインデックス。 |
| fontType     | Integer | フォント選択インデックス。     |
| fontSize     | Integer | フォントサイズ。         |
| color        | Integer | 色。                    |
| outlineWidth | Integer | アウトライン幅。            |
| outlineColor | Integer | アウトライン色。            |
| lineMargin   | Integer | 行間。              |
| charMargin   | Integer | 文字間。         |
| x            | Integer | 境界ボックスの X 位置。  |
| y            | Integer | 境界ボックスの Y 位置。  |
| width        | Integer | 境界ボックスの幅。       |
| height       | Integer | 境界ボックスの高さ。      |
| text         | String  | テキスト。                     |

### 戻り値

戻り値なし。

---

## Suika.getStringWidth()

UTF-8 文字列の合計幅を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明            |
|-----------|---------|------------------------|
| fontType  | Integer | フォント選択インデックス。  |
| fontSize  | Integer | フォントサイズ。      |
| text      | String  | テキスト。                  |

### 戻り値

ピクセル単位の幅の整数値。

---

## Suika.getStringHeight()

UTF-8 文字列の合計高さを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明            |
|-----------|---------|------------------------|
| fontType  | Integer | フォント選択インデックス。  |
| fontSize  | Integer | フォントサイズ。      |
| text      | String  | テキスト。                  |

### 戻り値

ピクセル単位の高さの整数値。

---

## Suika.drawGlyph()

画像に単一グリフを描画します。

### パラメータ (辞書)

| パラメータ     | 型    | 説明                                |
|---------------|---------|--------------------------------------------|
| img           | Object  | 対象画像。                              |
| font_type     | Integer | フォント選択インデックス。                      |
| font_size     | Integer | 描画フォントサイズ。                       |
| base_font_size| Integer | メトリクス用の基準フォントサイズ。                |
| outline_size  | Integer | アウトラインの幅。                      |
| x             | Integer | X 座標。                              |
| y             | Integer | Y 座標。                              |
| color         | Pixel   | 本文の色。                           |
| outline_color | Pixel   | アウトライン色。                             |
| codepoint     | Integer | UTF-32 コードポイント。                         |
| is_dim        | Boolean | 暗転を適用するかどうか。                  |

### 戻り値

成功を表す真偽値。

---

## Suika.createDrawMsg()

高レベルのテキスト描画用に複合メッセージ描画コンテキストを作成します。

### パラメータ (辞書)

| パラメータ      | 型     | 説明            |
|----------------|----------|------------------------|
| image          | Integer  | 描画先画像。     |
| text           | String   | 描画するメッセージ。       |
| fontType       | Integer  | フォント選択。        |
| fontSize       | Integer  | フォントサイズ。             |
| baseFontSize   | Integer  | 基準フォントサイズ。        |
| rubySize       | Integer  | ルビサイズ。             |
| outlineSize    | Integer  | アウトライン幅。         |
| penX           | Integer  | ペンの X 位置。        |
| penY           | Integer  | ペンの Y 位置。        |
| areaWidth      | Integer  | 描画領域の幅。       |
| areaHeight     | Integer  | 描画領域の高さ。      |
| leftMargin     | Integer  | 左余白。           |
| rightMargin    | Integer  | 右余白。          |
| topMargin      | Integer  | 上余白。            |
| bottomMargin   | Integer  | 下余白。         |
| lineMargin     | Integer  | 行間。           |
| charMargin     | Integer  | 文字間。      |
| color          | Integer  | 色。                 |
| outlineColor   | Integer  | アウトライン色。         |
| bgColor        | Integer  | 背景色。      |
| fillBg         | Boolean  | 背景を塗りつぶすか。       |
| dim            | Boolean  | 暗くするか。                   |
| ignoreLF       | Boolean  | 改行を無視するか。             |
| ignoreFont     | Boolean  | フォント変更を無視するか。    |
| ignoreOutline  | Boolean  | アウトライン変更を無視するか。 |
| ignoreColor    | Boolean  | 色変更を無視するか。   |
| ignoreSize     | Boolean  | サイズ変更を無視するか。    |
| ignorePosition | Boolean  | カーソル変更を無視するか。  |
| ignoreRuby     | Boolean  | ルビを無視するか。           |
| ignoreWait     | Boolean  | インライン待機を無視するか。    |
| inlineWaitHook | Function | インライン待機フック。      |
| tategaki       | Boolean  | 縦書きを使用するか。          |

### 戻り値

メッセージ描画コンテキストオブジェクト。

---

## Suika.destroyDrawMsg()

メッセージ描画コンテキストを破棄します。

### パラメータ (辞書)

| パラメータ      | 型     | 説明            |
|----------------|----------|------------------------|
| context        | Object   | メッセージ描画コンテキスト。  |

### 戻り値

戻り値なし。

---

## Suika.countDrawMsgChars()

エスケープシーケンスを除いた残り文字数を数えます。

### パラメータ (辞書)

| パラメータ      | 型     | 説明            |
|----------------|----------|------------------------|
| context        | Object   | メッセージ描画コンテキスト。  |

### 戻り値

整数を返します。

---

## Suika.drawMessage()

メッセージ内の文字を最大 (maxChars) 文字まで描画します。

### パラメータ (辞書)

| パラメータ      | 型     | 説明            |
|----------------|----------|------------------------|
| context        | Object   | メッセージ描画コンテキスト。  |
| maxChars       | Integer  | 最大文字数。             |

### 戻り値

この呼び出しで描画した文字数を示す整数を返します。

---

## Suika.getDrawMsgPenPosition()

描画コンテキストから現在のペン位置を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| context   | Object  | 描画コンテキスト。                           |

### 戻り値

`x` と `y` を含むオブジェクト。

---

## Suika.isEscapeSequenceChar()

文字がエスケープシーケンスの一部か確認します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| c         | String | 確認する文字。                        |

### 戻り値

真偽値。

---

## Suika.moveToTagFile()

新しいタグファイルを読み込み、実行位置を先頭へ移動します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| file      | String | .novel またはスクリプトファイルへのパス。 |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.getTagCount()

現在のスクリプトファイル内のタグ総数を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

タグ数を表す整数。

---

## Suika.moveToTagIndex()

実行ポインタを指定したタグインデックスへ移動します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 対象タグインデックス。 |

### 戻り値

真偽値。

---

## Suika.moveToNextTag()

実行ポインタを次のタグへ移動します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.moveToLabelTag()

指定したラベルへジャンプします。

### パラメータ (辞書)

| パラメータ | 型   | 説明             |
|-----------|--------|-------------------------|
| name      | String | 対象ラベル名。      |

### 戻り値

真偽値。

---

## Suika.moveToMacroTag()

名前で指定したマクロへジャンプします。

### パラメータ (辞書)

| パラメータ | 型   | 説明             |
|-----------|--------|-------------------------|
| name      | String | 対象マクロ名。      |

### 戻り値

真偽値。

---

## Suika.moveToElseTag()

対応する else/elseif/endif タグへジャンプします。

### パラメータ (辞書)

| パラメータ | 型   | 説明             |
|-----------|--------|-------------------------|
| name      | String | 対象マクロ名。      |

### 戻り値

真偽値。

---

## Suika.moveToEndIfTag()

対応する endif タグへジャンプします。

### パラメータ (辞書)

| パラメータ | 型   | 説明             |
|-----------|--------|-------------------------|
| name      | String | 対象マクロ名。      |

### 戻り値

真偽値。

---

## Suika.moveToEndMacroTag()

対応する endmacro タグへジャンプします。

### パラメータ (辞書)

| パラメータ | 型   | 説明             |
|-----------|--------|-------------------------|
| name      | String | 対象マクロ名。      |

### 戻り値

真偽値。

---

## Suika.getTagFileName()

現在タグのスクリプトファイル名を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

ファイル名を表す文字列。

---

## Suika.getTagName()

現在のタグ名を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

タグ名を表す文字列。

---

## Suika.getTagPropertyCount()

現在のタグのプロパティ数を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

名前または値を表す文字列。

---

## Suika.getTagPropertyName()

現在のタグのプロパティ (引数) を反復して取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明       |
|-----------|---------|-------------------|
| index     | Integer | プロパティインデックス。   |

### 戻り値

名前を表す文字列。

---

## Suika.getTagPropertyValue()

現在のタグのプロパティ (引数) を反復して取得します。

### パラメータ (辞書) (PropertyName/Value 用)

| パラメータ | 型    | 説明       |
|-----------|---------|-------------------|
| index     | Integer | プロパティインデックス。   |

### 戻り値

値を表す文字列。

---

## Suika.getTagArgBool()

現在のタグの指定した引数を取得します。デフォルト値と省略可能性に対応します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                       |
|-----------|---------|-----------------------------------|
| name      | String  | 引数名。             |
| omissible | Boolean | 引数を省略可能にするかどうか。 |
| defVal    | Boolean | 未指定時のデフォルト値。         |

### 戻り値

要求した型での引数値。

---

## Suika.getTagArgInt()

現在のタグの指定した引数を取得します。デフォルト値と省略可能性に対応します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                       |
|-----------|---------|-----------------------------------|
| name      | String  | 引数名。             |
| omissible | Boolean | 引数を省略可能にするかどうか。 |
| defVal    | Integer | 未指定時のデフォルト値。         |

### 戻り値

要求した型での引数値。

---

## Suika.getTagArgFloat()

現在のタグの指定した引数を取得します。デフォルト値と省略可能性に対応します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                       |
|-----------|---------|-----------------------------------|
| name      | String  | 引数名。             |
| omissible | Boolean | 引数を省略可能にするかどうか。 |
| defVal    | Float   | 未指定時のデフォルト値。         |

### 戻り値

要求した型での引数値。

---

## Suika.getTagArgString()

現在のタグの指定した引数を取得します。デフォルト値と省略可能性に対応します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                       |
|-----------|---------|-----------------------------------|
| name      | String  | 引数名。             |
| omissible | Boolean | 引数を省略可能にするかどうか。 |
| defVal    | String  | 未指定時のデフォルト値。         |

### 戻り値

要求した型での引数値。

---

## Suika.evaluateTag()

現在のタグのプロパティ値を評価し、インライン変数 (`${varname}` 形式) を展開します。

この API を呼び出すと、プロパティ値のキャッシュが更新されます。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.pushTagStackIf()

`[if]` 条件ブロック用の内部スタックを管理します。

この API はネストしたブロック処理のために `if` ブロック位置を記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.popTagStackIf()

`if` 条件ブロック用の内部スタックを管理します。

この API はネストしたブロック処理のために `if` ブロックの終端を記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.pushTagStackWhile()

ループ (`while`) 用の内部スタックを管理します。

この API はネストしたブロック処理のために `while` ブロックを記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.popTagStackWhile()

ループ (`while`) 用の内部スタックを管理します。

この API はネストしたブロック処理のために `while` ブロックの終端を記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.pushTagStackFor()

ループ (`for`) 用の内部スタックを管理します。

この API はネストしたブロック処理のために `for` ブロックを記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.popTagStackFor()

ループ (`for`) 用の内部スタックを管理します。

この API はネストしたブロック処理のために `for` ブロックの終端を記録します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.loadAnimeFromFile()

ファイルからアニメーション定義を読み込み、登録します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| file      | String  | アニメーションファイルへのパス。                    |
| reg_name  | String  | アニメーションの登録名。           |

### 戻り値

各レイヤーが読み込まれたかどうかを示す真偽値の配列。

---

## Suika.newAnimeSequence()

指定したレイヤー用の新しいアニメーションシーケンスの記述を開始します。
この API は手動生成アニメーションに使用します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | 対象ステージレイヤーのインデックス。                  |

### 戻り値

成功を表す真偽値。

---

## Suika.addAnimeSequencePropertyF()

現在のアニメーションシーケンスに浮動小数点プロパティ (位置、アルファなど) を追加します。
この API は手動生成アニメーションに使用します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| key       | String  | プロパティキー (例: "x", "y", "a")。        |
| val       | Float   | 対象値。                              |

### 戻り値

真偽値。

---

## Suika.addAnimeSequencePropertyI()

現在のアニメーションシーケンスに整数プロパティ (位置、アルファなど) を追加します。
この API は手動生成アニメーションに使用します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| key       | String  | プロパティキー (例: "x", "y", "a")。        |
| val       | Integer | 対象値。                              |

### 戻り値

真偽値。

---

## Suika.startLayerAnime()

指定したレイヤーで登録済みアニメーションシーケンスを開始します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | 対象ステージレイヤーのインデックス。                  |

### 戻り値

真偽値。

---

## Suika.isAnimeRunning()

全体のアニメーション状態を確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isAnimeFinishedForLayer()

指定したレイヤーのアニメーションが終了したか確認します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | 対象ステージレイヤーのインデックス。  |

### 戻り値

真偽値。

---

## Suika.updateAnimeFrame()

アニメーションフレーム状態を更新します。通常はフレームごとに 1 回呼び出します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.loadEyeImageIfExists()

キャラクター位置の目パチ (eye-patch) 画像とアニメーションを管理します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | キャラクター位置 (Left、Center など)。   |
| file      | String  | 目パチ画像ファイルへのパス。                |

### 戻り値

真偽値。

---

## Suika.reloadEyeAnime()

キャラクター位置の目パチ (eye-patch) アニメーションを再開します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | キャラクター位置 (Left、Center など)。   |

### 戻り値

真偽値。

---

## Suika.runLipAnime()

キャラクターに対して、メッセージ内容に基づくリップシンクアニメーションを開始します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | キャラクター位置。                        |
| text      | String  | 同期するメッセージテキスト。             |

### 戻り値

戻り値なし。

---

## Suika.stopLipAnime()

リップシンクアニメーションを停止します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | キャラクター位置。                        |

### 戻り値

戻り値なし。

---

## Suika.clearLayerAnimeSequence()

指定したレイヤーのアニメーションシーケンスをクリアします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| layer     | Integer | 対象ステージレイヤーのインデックス。  |

### 戻り値

戻り値なし。

---

## Suika.clearAllAnimeSequence()

すべてのレイヤーのアニメーションシーケンスをクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.setVariableInt()

ローカルまたはグローバル変数に値をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明            |
|-----------|---------|------------------------|
| name      | String  | 変数名。  |
| value     | Integer | セットする値           |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.setVariableFloat()

ローカルまたはグローバル変数に値をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明             |
|-----------|---------|-------------------------|
| name      | String  | 変数名。   |
| value     | Float   | セットする値            |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.setVariableString()

ローカルまたはグローバル変数に値をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明             |
|-----------|---------|-------------------------|
| name      | String  | 変数名。   |
| value     | String  | セットする値            |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.getVariableInt()

変数の現在値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| name      | String | 変数名。                      |

### 戻り値

整数としての変数値。

---

## Suika.getVariableFloat()

変数の現在値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| name      | String | 変数名。                      |

### 戻り値

浮動小数点数としての変数値。

---

## Suika.getVariableString()

変数の現在値を取得します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| name      | String | 変数名。                      |

### 戻り値

文字列としての変数値。

---

## Suika.unsetVariable()

指定した変数を解除 (削除) します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| name      | String | 解除する変数名。             |

### 戻り値

戻り値なし。

---

## Suika.unsetLocalVariables()

すべてのローカル変数を解除 (削除) します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.makeVariableGlobal()

変数をグローバルにします (セーブをまたいで永続化)。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| name      | String  | 変数名。                      |
| is_global | Boolean | グローバルにするかどうか。                 |

### 戻り値

真偽値。

---

## Suika.isGlobalVariable()

変数のグローバル状態を確認します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| name      | String  | 変数名。                      |

### 戻り値

真偽値。

---

## Suika.getVariableCount()

変数の数を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

個数を表す整数。

---

## Suika.getVariableName()

登録済み変数を反復処理します。

### パラメータ (辞書) (getVariableName 用)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 変数のインデックス。     |

### 戻り値

名前を表す文字列。

---

## Suika.checkVariableExists()

指定した名前の変数が存在するか確認します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| name      | String | 確認する名前。                             |

### 戻り値

真偽値。

---

## Suika.executeSaveGlobal()

グローバルセーブを実行します。
グローバルデータには通常、永続設定が含まれます。

### パラメータ (辞書)

パラメータなし。

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.executeLoadGlobal()

グローバルロードを実行します。
グローバルデータには通常、永続設定が含まれます。

### パラメータ (辞書)

パラメータなし。

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.executeSaveLocal()

ゲーム進行状況を指定したスロットに保存します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.executeLoadLocal()

指定したスロットからゲーム進行状況を読み込みます。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.checkSaveExists()

指定したスロットインデックスのセーブデータが存在するか確認します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

真偽値。

---

## Suika.deleteLocalSave()

ローカルセーブスロットを削除します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

戻り値なし。

---

## Suika.deleteGlobalSave()

グローバルセーブデータ全体を削除します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.checkRightAfterLoad()

現在のフレームが、ロード成功直後か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.getSaveTimestamp()

セーブデータが作成された日時 (Unix 時刻) を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

整数 (タイムスタンプ)。

---

## Suika.getLatestSaveIndex()

直近に更新されたセーブスロットのインデックスを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

スロットインデックスを表す整数。

---

## Suika.getSaveChapterName()

セーブスロットに保存されたチャプタータイトルを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

チャプター名を表す文字列。

---

## Suika.getSaveLastMessage()

セーブスロットに保存された最後の表示メッセージを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

メッセージを表す文字列。

---

## Suika.getSaveThumbnail()

セーブスロットに関連付けられたサムネイル画像を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | セーブスロットのインデックス。    |

### 戻り値

画像オブジェクト。

---

## Suika.clearHistory()

履歴 (バックログ) からすべてのメッセージをクリアします。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.addHistory()

履歴に新しいエントリを追加します。

### パラメータ (辞書)

| パラメータ        | 型    | 説明                                |
|------------------|---------|--------------------------------------------|
| name             | String  | キャラクター名。                            |
| msg              | String  | メッセージテキスト。                              |
| voice            | String  | ボイスファイルへのパス。                    |
| bodyColor        | Integer | 本文色。                                |
| bodyOutlineColor | Integer | 本文アウトライン色。                        |
| nameColor        | Integer | 名前色。                                |
| nameOutlineColor | Integer | 名前アウトライン色。                        |

### 戻り値

成功を表す真偽値。

---

## Suika.getHistoryCount()

現在の履歴内のエントリ総数を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

履歴数を表す整数。

---

## Suika.getHistoryName()

指定した履歴インデックスの名前を取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 履歴内のインデックス。      |

### 戻り値

文字列値。

---

## Suika.getHistoryMessage()

指定した履歴インデックスのメッセージを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 履歴内のインデックス。      |

### 戻り値

文字列値。

---

## Suika.getHistoryVoice()

指定した履歴インデックスのボイスパスを取得します。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| index     | Integer | 履歴内のインデックス。      |

### 戻り値

文字列値。

---

## Suika.loadSeen()

現在のスクリプトファイルの既読フラグを読み込みます。

### パラメータ (辞書)

パラメータなし。

### 戻り値

成功を表す真偽値。

---

## Suika.saveSeen()

現在のスクリプトファイルの既読フラグを保存します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

成功を表す真偽値。

---

## Suika.getSeenFlags()

現在のタグの既読状態を取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

整数を返します。

`[text]` タグでは、`0` は未読、`1` は既読を意味します。

`[choose]` タグでは、各ビットがその選択肢が以前に選ばれたことを示します。

---

## Suika.setSeenFlags()

現在のタグの既読状態をセットします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                |
|-----------|---------|----------------------------|
| flag      | Integer | 既読状態フラグ。          |

### 戻り値

戻り値なし。

---

## Suika.loadGUIFile()

GUI 定義ファイルを読み込み、実行準備をします。

### パラメータ (辞書)

| パラメータ | 型    | 説明                                |
|-----------|---------|--------------------------------------------|
| file      | String  | .gui ファイルへのパス。                     |
| sys       | Boolean | システム GUI (セーブ/ロードなど) かどうか。 |

### システム GUI とは

システム GUI は通常、`[text]` または `[choose]` の実行中に呼び出され、制御は中断されたタグへ戻ります。

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.startGUI()

読み込んだ GUI を開始します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.stopGUI()

現在実行中の GUI を停止します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isGUIRunning()

GUI が現在アクティブか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.isGUIFinished()

GUI の処理が完了したか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.getGUIResultLabel()

GUI 終了時に選択されたボタンのラベルを取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

結果ラベルを表す文字列。

---

## Suika.isGUIResultTitle()

GUI が「タイトルへ戻る」操作で閉じられたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.checkIfSavedInGUI()

GUI がアクティブな間にセーブ操作が実行されたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.checkIfLoadedInGUI()

GUI がアクティブな間にロード操作が実行されたか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.checkRightAfterSysGUI()

現在のフレームがシステム GUI から戻った直後か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.getMillisec()

時間原点からの経過時間をミリ秒で取得します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

ミリ秒単位の整数。

---

## Suika.checkFileExists()

ファイルが存在するか確認します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| file      | String | ファイルへのパス。                          |

### 戻り値

真偽値を返します。

---

## Suika.readFileContent()

ファイル内容全体を読み込みます。

### パラメータ (辞書) (readFileContent 用)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| file      | String | ファイルへのパス。                          |

### 戻り値

文字列を返します。

---

## Suika.writeSaveData()

キーに関連付けられた生のセーブデータを直接書き込みます。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | データの一意なキー。                   |
| data      | String | 書き込み/読み込みするデータ。                        |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.readSaveData()

キーに関連付けられた生のセーブデータを直接読み込みます。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| key       | String | データの一意なキー。                   |

### 戻り値

成功または失敗を表す真偽値。

---

## Suika.playVideo()

動画再生を制御します。

### パラメータ (辞書) (playVideo 用)

| パラメータ    | 型    | 説明                          |
|--------------|---------|--------------------------------------|
| file         | String  | 動画ファイルへのパス。              |
| is_skippable | Boolean | ユーザーが動画をスキップできるかどうか。 |

### 戻り値

Play は真偽値を返し、IsPlaying も真偽値を返します。

---

## Suika.stopVideo()

動画再生を停止します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.isVideoPlaying()

動画が再生中か確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値を返します。

---

## Suika.isFullScreenSupported()

フルスクリーンモードに対応しているか確認します。

### パラメータ (辞書)

パラメータなし。

### 戻り値

真偽値。

---

## Suika.enterFullScreenMode()

フルスクリーンモードに入ります。

### パラメータ (辞書)

パラメータなし。

### 戻り値

戻り値なし。

---

## Suika.speakText()

指定したメッセージに対して Text-to-Speech (TTS) を実行します。

### パラメータ (辞書)

| パラメータ | 型   | 説明                                |
|-----------|--------|--------------------------------------------|
| msg       | String | 読み上げるテキスト。                         |

### 戻り値

戻り値なし。
