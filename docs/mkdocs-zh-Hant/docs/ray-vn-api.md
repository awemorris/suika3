Ray VN API 參考
====================

《VN API (Suika.*)》是為視覺小說創作而設計的。

每個 `Suika.*` API 函數都只接受一個參數。
該參數必須是一個字典, 並且函數的選項必須作為鍵值對存儲在字典中。
在本文檔中，「參數」是指該字典中的鍵值對。

## 索引

* 基礎
    * [Suika.loadPlugin()](#suikaloadplugin)
* 配置
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
* 輸入
    * [Suika.getMousePosX()](#suikagetmouseposx)
    * [Suika.getMousePosY()](#suikagetmouseposy)
    * [Suika.isMouseLeftPressed()](#suikaismouseleftpressed)
    * [Suika.isMouseRightPressed()](#suikaismouserightpressed)
    * [Suika.isMouseLeftClicked()](#suikaismouseleftclicked)
    * [Suika.isMouseRightClicked()](#suikaismouserightclicked)
    * [Suika.isMouseDragging()](#suikaismousedragging)
    * [Suika.isReturnKeyPressed()](#suikaisreturnkeypressed)
    * [Suika.isSpaceKeyPressed()](#suikaisspaceKeypressed)
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
* 遊戲
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
* 圖像
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
    * [Suika.getImage寬度()](#suikagetimagewidth)
    * [Suika.getImage高度()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImage()](#suikadrawimage)
    * [Suika.drawImage3D()](#suikadrawimage3d)
    * [Suika.makeColor()](#suikamakecolor)
    * [Suika.fillImageRect()](#suikafillimagerect)
* 舞台
    * [Suika.reloadStageImages()](#suikareloadstageimages)
    * [Suika.reloadStage位置s()](#suikareloadstagepositions)
    * [Suika.getLayerX()](#suikagetlayerx)
    * [Suika.getLayerY()](#suikagetlayery)
    * [Suika.setLayer位置()](#suikasetlayerposition)
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
    * [Suika.setLayerFileName()](#suikasetlayerfile名稱)
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
    * [Suika.setChNameMapping()](#suikasetch名稱mapping)
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
    * [Suika.fillNameBox()](#suikafill名稱box)
    * [Suika.showNameBox()](#suikashow名稱box)
    * [Suika.getNameBoxRect()](#suikaget名稱boxrect)
    * [Suika.showChoosebox()](#suikashowchoosebox)
    * [Suika.showAutoModeBanner()](#suikashowautomodebanner)
    * [Suika.showSkipModeBanner()](#suikashowskipmodebanner)
    * [Suika.renderImage()](#suikarenderimage)
    * [Suika.renderImage3d()](#suikarenderimage3d)
    * [Suika.setClick位置()](#suikasetclickposition)
    * [Suika.showClick()](#suikashowclick)
    * [Suika.setClickIndex()](#suikasetclick索引]
    * [Suika.getClickRect()](#suikagetclickrect)
    * [Suika.fillChooseBoxIdleImage()](#suikafillchooseboxidleimage)
    * [Suika.fillChooseBoxHoverImage()](#suikafillchooseboxhoverimage)
    * [Suika.getChooseBoxRect()](#suikagetchooseboxrect)
    * [Suika.startKirakira()](#suikastartkirakira)
    * [Suika.renderKirakira()](#suikarenderkirakira)
* 混音器
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
    * [Suika.getTrackFileName()](#suikagettrackfile名稱)
    * [Suika.applyCharacterVolume()](#suikaapplycharactervolume)
* 系統按鈕
    * [Suika.enableSysBtn()](#suikaenablesysbtn)
    * [Suika.isSysBtnVisible()](#suikaisysbtnvisible)
    * [Suika.updateSysBtnState()](#suikaupdatesysbtnstate)
    * [Suika.isSysBtnPointed()](#suikaisysbtnpointed)
    * [Suika.isSysBtnClicked()](#suikaisysbtnclicked)
* 文本
    * [Suika.drawTextOnLayer()](#suikadrawtextonlayer)
    * [Suika.getString寬度()](#suikagetstringwidth)
    * [Suika.getString高度()](#suikagetstringheight)
    * [Suika.drawGlyph()](#suikadrawglyph)
    * [Suika.createDrawMsg()](#suikacreatedraw消息)
    * [Suika.destroyDrawMsg()](#suikadestroydraw消息)
    * [Suika.countDrawMsgChars()](#suikacountdraw消息chars)
    * [Suika.drawMessage()](#suikadraw消息common)
    * [Suika.getDrawMsgPenPosition()](#suikagetpenpositioncommon)
    * [Suika.isEscapeSequenceChar()](#suikaisescapesequencechar)
* 標籤
    * [Suika.getTagCount()](#suikagettagcount)
    * [Suika.moveToTagFile()](#suikamovetotagfile)
    * [Suika.moveToTagIndex()](#suikamovetotagindex)
    * [Suika.moveToNextTag()](#suikamovetonexttag)
    * [Suika.moveToLabelTag()](#suikamovetolabeltag)
    * [Suika.moveToMacroTag()](#suikamovetomacrotag)
    * [Suika.moveToElseTag()](#suikamovetoelsetag)
    * [Suika.moveToEndIfTag()](#suikamovetoendiftag)
    * [Suika.moveToEndMacroTag()](#suikamovetoendmacrotag)
    * [Suika.getTagFileName()](#suikagettagfile名稱)
    * [Suika.getTagName()](#suikagettagname)
    * [Suika.getTagPropertyCount()](#suikagettagpropertycount)
    * [Suika.getTagPropertyName()](#suikagettagproperty名稱)
    * [Suika.getTagPropertyValue()](#suikagettagproperty值)
    * [Suika.getTagArgBool()](#suikagettagargbool)
    * [Suika.getTagArgInt()](#suikagettagargint)
    * [Suika.getTagArgFloat()](#suikagettagargfloat)
    * [Suika.getTagArgString()](#suikagettagargstring)
    * [Suika.evaluateTag()](#suikaevaluatetag)
    * [Suika.pushTagStackIf()](#suikapushtagstackif)
    * [Suika.popTagStackIf()](#suikapoptagstackif)
    * [Suika.pushTagStackWhile()](#suikapushtagstackwhile)
    * [Suika.pushTagStackFor()](#suikapushtagstackfor)
* 動畫
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
* 變量
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
    * [Suika.getVariableName()](#suikagetvariable名稱)
    * [Suika.checkVariableExists()](#suikacheckvariableexists)
    * [Suika.expandStringWithVariable()](#suikaexpandstringwithvariable)
* 保存
    * [Suika.executeSaveGlobal()](#suikaexecutesaveglobal)
    * [Suika.executeLoadGlobal()](#suikaexecuteloadglobal)
    * [Suika.executeSaveLocal()](#suikaexecutesavelocal)
    * [Suika.executeLoadLocal()](#suikaexecuteloadlocal)
    * [Suika.checkSaveExists()](#suikachecksaveexists)
    * [Suika.deleteLocalSave()](#suikadeletelocalsave)
    * [Suika.deleteGlobalSave()](#suikadeleteglobalsave)
    * [Suika.checkRightAfterLoad()](#suikacheckrightafterload)
    * [Suika.getSaveTimestamp()](#suikagetsavetimestamp)
    * [Suika.getLatestSaveIndex()](#suikagetlatestsave索引)
    * [Suika.getSaveChapterName()](#suikagetsavechaptername)
    * [Suika.getSaveLastMessage()](#suikagetsavelastmessage)
    * [Suika.getSaveThumbnail()](#suikagetsavethumbnail)
* 歷史
    * [Suika.clearHistory()](#suikaclearhistory)
    * [Suika.addHistory()](#suikaaddhistory)
    * [Suika.getHistoryCount()](#suikagethistorycount)
    * [Suika.getHistoryName()](#suikagethistory名稱)
    * [Suika.getHistoryMessage()](#suikagethistorymessage)
    * [Suika.getHistoryVoice()](#suikagethistoryvoice)
* 已讀
    * [Suika.loadSeen()](#suikaloadseen)
    * [Suika.saveSeen()](#suikasaveseen)
    * [Suika.getSeenFlags()](#suikagetseenflags)
    * [Suika.setSeenFlags()](#suikasetseenflags)
* 圖形用戶界面
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

加載插件。

只有這個 API 接受非字典參數。

### 參數（直接）

| 參數 | 類型 | 說明 |
|-----------|--------|-----------------|
| 名稱      | String | 插件名稱。    |

### 返回

無返回。

---

## Suika.setConfig()

設置配置。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 配置的鍵。                         |
| 值     | String | 配置的值。                       |

### 返回

無返回。

---

## Suika.getConfigCount()

獲取配置鍵的數量。

### 參數（字典）

無參數。

### 返回

代表配置鍵數量的整數。

---

## Suika.getConfigKey()

獲取配置鍵的索引。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 索引     | Integer | 配置的索引。                         |

### 返回

代表指定索引處配置鍵的字符串。

---

## Suika.isGlobalSaveConfig()

檢查配置鍵是否存儲到全局保存數據。

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

表示配置是否全局保存的布爾值。

---

## Suika.isLocalSaveConfig()

檢查配置鍵是否存儲到本地保存數據。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

表示配置是否本地保存的布爾值。

---

## Suika.getConfigType()

獲取配置值類型。("s", "b", "i", "f")

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

以下字符串之一。

| 值 | 含義 |
|------------|--------------------------|
| "s" | 配置是字符串。 |
| "b" | 配置是布爾值。 |
| "i" | 配置是整數。 |
| "f" | 配置是浮點數。 |

---

## Suika.getStringConfig()

獲取字符串配置值。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

配置的字符串值。

---

## Suika.getBoolConfig()

獲取布爾配置值。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

配置的布爾值。

---

## Suika.getIntConfig()

獲取整數配置值。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

配置的整數值。

---

## Suika.getFloatConfig()

獲取浮點配置值。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

配置的浮點值。

---

## Suika.getConfigAsString()

Get a config 值 as a string.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | 鍵名。                                  |

### 返回

Stringified 值 of the config.

---

## Suika.compareLocale()

檢查指定的語言環境是否與當前語言環境相同。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| locale    | String | Locale 名稱.                               |


### 返回

表示指定語言環境是否匹配的布爾值
current one.

---

## Suika.getMousePosX()

獲取鼠標 X 位置。

### 參數（字典）

無參數。

### 返回

代表當前鼠標 X 座標的整數。

---

## Suika.getMousePosY()

獲取鼠標 Y 位置。

### 參數（字典）

無參數。

### 返回

代表當前鼠標 Y 座標的整數。

---

## Suika.isMouseLeftPressed()

檢查鼠標左按鈕是否被按下。

### 參數（字典）

無參數。

### 返回

表示左按鈕是否當前被按住的布爾值.

---

## Suika.isMouseRightPressed()

檢查鼠標右按鈕是否被按下。

### 參數（字典）

無參數。

### 返回

表示右按鈕是否當前被按住的布爾值n.

---

## Suika.isMouseLeftClicked()

Check if mouse left button is pressed then released.

### 參數（字典）

無參數。

### 返回

Boolean that represents whether a left-click occurred in the current frame.

---

## Suika.isMouseRightClicked()

Check if mouse right button is pressed then released.

### 參數（字典）

無參數。

### 返回

Boolean that represents whether a right-click occurred in the current frame.

---

## Suika.isMouseDragging()

Check if mouse is dragging.

### 參數（字典）

無參數。

### 返回

Boolean that represents whether the mouse is being moved while a button is pressed.

---

## Suika.isReturnKeyPressed()

Check if return 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isSpaceKeyPressed()

Check if space 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isEscapeKeyPressed()

Check if escape 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isUpKeyPressed()

Check if up 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isDownKeyPressed()

Check if down 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isLeftKeyPressed()

Check if left 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isRightKeyPressed()

Check if right 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isPageUpKeyPressed()

Check if pageup 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isPageDownKeyPressed()

Check if pagedown 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isControlKeyPressed()

Check if control 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isSKeyPressed()

Check if S 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isLKeyPressed()

Check if L 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isHKeyPressed()

Check if H 鍵 is pressed.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isTouchCanceled()

檢查觸摸是否被取消。

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isSwiped()

Check if swiped.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.clearInputState()

Clear input states to avoid further input processing in the current frame.

### 參數（字典）

無參數。

### 返回

無返回。


---

## Suika.startCommandRepetition()

Start a multiple-frame command execution.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.stopCommandRepetition()

Stop a multiple-frame command execution.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isInCommandRepetition()

Check whether we are in a multiple-frame command execution or not.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.setMessageActive()

Set the message showing state to active.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.clearMessageActive()

Reset the message showing state.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isMessageActive()

Check whether the message showing state is set or not.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.startAutoMode()

Start the auto-mode.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.stopAutoMode()

Stop the auto-mode.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isAutoMode()

Check whether we are in the auto-mode or not.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.startSkipMode()

Start the skip-mode.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.stopSkipMode()

停止跳過模式。

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isSkipMode()

檢查我們是否在跳過模式中。

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.setSaveLoad()

設置保存/加載啟用設置。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|-----------------------------------|
| enable    | Boolean | 是否啟用保存和加載。  |

### 返回

無返回。

---

## Suika.isSaveLoadEnabled()

獲取保存/加載啟用設置。

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.setNonInterruptible()

Set the non-interruptible mode setting.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| enable    | Boolean | Non-interruptible mode.    |

### 返回

無返回。

---

## Suika.isNonInterruptible()

Get the non-interruptible mode setting.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.setPenPosition()

Set the pen position for text drawing.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| x         | Integer | X coordinate.         |
| y         | Integer | Y coordinate.         |

### 返回

無返回。

---

## Suika.getPenPositionX()

Get the pen X position.

### 參數（字典）

無參數。

### 返回

Integer 值.

---

## Suika.getPenPositionY()

Get the pen Y position.

### 參數（字典）

無參數。

### 返回

Integer 值.

---

## Suika.pushForCall()

Push the return point to the call stack.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| file      | String  | Script file 名稱.     |
| 索引     | Integer | Command 索引.        |

### 返回

Boolean that represents success or failure.

---

## Suika.popForReturn()

Pop the return point from the call stack.

### 參數（字典）

無參數。

### 返回

Returns a dictionary that contains:

* obj.file: File 名稱
* obj.索引: Tag 索引

---

## Suika.readCallStack()

Read the call stack element at the specified 索引.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| sp        | Integer | Stack element 索引.  |

### 返回

Returns a dictionary that contains:

* obj.file: File 名稱
* obj.索引: Tag 索引

---

## Suika.writeCallStack()

Write the call stack element at the specified 索引.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| sp        | Integer | Stack element 索引.  |
| file      | String  | Script file 名稱.     |
| 索引     | Integer | Tag 索引.            |

### 返回

無返回。

---

## Suika.setCallArgument()

Set a calling argument for GUI or anime.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| 索引     | Integer | 參數索引。       |
| 值     | String  | 參數值。       |

### 返回

Boolean 值.

---

## Suika.getCallArgument()

Get a calling argument.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| 索引     | Integer | 參數索引。       |

### 返回

String 值.

---

## Suika.isPageMode()

Check if the script page mode is enabled.

### 參數（字典）

無參數。

### 返回

返回布爾值。

---

## Suika.appendBufferedMessage()

Append a string to the page mode buffer string.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| message   | String  | Message.              |

### 返回

無返回。

---

## Suika.getBufferedMessage()

Get the page mode buffer string.

### 參數（字典）

無參數。

### 返回

Returns a string.

---

## Suika.clearBufferedMessage()

Clear the page mode buffer string.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.resetPageLine()

Reset the message line count in a page.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.incPageLine()

Increment the line count in a page.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isPageTop()

Check if we are at the first line in a page.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.registerBGVoice()

Register a BGVoice.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| file      | String  | BGVoice file.         |

### 返回

無返回。

---

## Suika.getBVoice()

Get the BGVoice.

### 參數（字典）

無參數。

### 返回

Returns a file 名稱 string.

---

## Suika.setBGVoicePlaying()

Set the BGVoice state playing.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| isPlaying | Boolean | State.                |

### 返回

無返回。

---

## Suika.isBGVoicePlaying()

Check if the BGVoice is playing.

### 參數（字典）

無參數。

### 返回

返回布爾值。

---

## Suika.setChapterName()

Set the chapter 名稱.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| 名稱      | String  | 章節名稱。         |

### 返回

無返回。

---

## Suika.getChapterName()

Get the chapter 名稱.

### 參數（字典）

無參數。

### 返回

Returns a string.

---

## Suika.setLastMessage()

設置最後一條消息。

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| message   | String  | Message.              |
| isAppend  | Boolean | Append or replace.    |

### 返回

無返回。

---

## Suika.getLastMessage()

獲取最後一條消息。

### 參數（字典）

無參數。

### 返回

Returns a string.

---

## Suika.setTextSpeed()

Set the text speed.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| speed     | Float   | Text speed.           |

### 返回

無返回。

---

## Suika.getTextSpeed()

Get the text speed.

### 參數（字典）

無參數。

### 返回

Returns a float.

---

## Suika.setAutoSpeed()

Set the auto mode speed.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| speed     | Float   | Auto speed.           |

### 返回

無返回。

---

## Suika.getAutoSpeed()

Get the auto speed.

### 參數（字典）

無參數。

### 返回

Returns a float.

---

## Suika.markLastEnglishTagIndex()

Mark the last English 索引.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.getLastEnglishTagIndex()

Get the last English 索引.

### 參數（字典）

無參數。

### 返回

Returns an integer.

---

## Suika.clearLastEnglishTagIndex()

Clear the last English 索引.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.getLastTagName()

Get the last tag 名稱.


### 參數（字典）

無參數。

### 返回

Returns a string.

---

## Suika.createImageFromFile()

Load an image from a file.

### 參數（字典）

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| file      | String | Path to the image file.       |

### 返回

An image object, or null on failure.

---

## Suika.createImage()

Create a new blank image.

### 參數（字典）

| Parameter | Type    | Description                   |
|-----------|---------|-------------------------------|
| width     | Integer | 寬度 of the image.           |
| height    | Integer | 高度 of the image.          |

### 返回

An image object.

---

## Suika.getImage寬度()

獲取圖像的寬度。

### 參數（字典）

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| img       | Object | Image object.                 |

### 返回

Integer that represents the width.

---

## Suika.getImage高度()

獲取圖像的高度。

### 參數（字典）

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| image     | Object | Image object.                 |

### 返回

Integer that represents the height.

---

## Suika.destroyImage()

Destroy an image and free its memory.

### 參數（字典）

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| image     | Object | Image object to destroy.      |

### 返回

無返回。

---

## Suika.drawImage()

Copy an image to another image (no blending).

### 參數（字典）

| Parameter  | Type    | Description                      |
|------------|---------|----------------------------------|
| dstImage   | Object  | Destination image.               |
| dstLeft    | Integer | X coordinate in destination.     |
| dstTop     | Integer | Y coordinate in destination.     |
| srcImage   | Object  | Source image.                    |
| dst寬度   | Integer | 寬度 to draw.                   |
| dst高度  | Integer | 高度 to draw.                  |
| srcLeft    | Integer | X coordinate in source.          |
| srcTop     | Integer | Y coordinate in source.          |
| alpha      | Integer | 0-255                            |
| blend      | Integer | Blending type.                   |

### Blending Types

| Type                | Description                        |
|---------------------|------------------------------------|
| Suika.BLEND_COPY    | Copy.                              |
| Suika.BLEND_ALPHA   | Alpha blending.                    |
| Suika.BLEND_ADD     | Add blending.                      |
| Suika.BLEND_SUB     | Sub blending.                      |
| Suika.BLEND_DIM     | RGB 50% alpha blending.            |
| Suika.BLEND_GLYPH   | Alpha blending for normal glyphs.  |
| Suika.BLEND_EMOJI   | Alpha blending for emoji glyphs.   |

### 返回

無返回。

---

## Suika.drawImage3D()

Copy an image to another image (no blending).

### 參數（字典）

| Parameter  | Type    | Description                      |
|------------|---------|----------------------------------|
| dstImage   | Object  | Destination image.               |
| x1         | Integer | x1 coordinate in destination.    |
| y1         | Integer | y1 coordinate in destination.    |
| x2         | Integer | x2 coordinate in destination.    |
| y2         | Integer | y2 coordinate in destination.    |
| x3         | Integer | x3 coordinate in destination.    |
| y3         | Integer | y3 coordinate in destination.    |
| x4         | Integer | x4 coordinate in destination.    |
| y5         | Integer | y4 coordinate in destination.    |
| srcImage   | Object  | Source image.                    |
| srcLeft    | Integer | X coordinate in source.          |
| srcTop     | Integer | Y coordinate in source.          |
| src寬度   | Integer | 寬度 in source.                 |
| src高度  | Integer | 高度 in source.                |
| alpha      | Integer | 0-255                            |
| blend      | Integer | Blending type.                   |

### Blending Types

| Type                | Description                        |
|---------------------|------------------------------------|
| Suika.BLEND_ALPHA   | Alpha blending.                    |
| Suika.BLEND_ADD     | Add blending.                      |
| Suika.BLEND_SUB     | Sub blending.                      |
| Suika.BLEND_DIM     | RGB 50% alpha blending.            |

### 返回

無返回。

---

## Suika.drawImageAlpha()

Draw an image with alpha blending.

### 參數（字典）

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dst寬度  | Integer | 寬度 to draw.                   |
| dst高度 | Integer | 高度 to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha 值 (`0`-`255`).         |

### 返回

無返回。

---

## Suika.drawImageAdd()

Draw an image with additive blending.

### 參數（字典）

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dst寬度  | Integer | 寬度 to draw.                   |
| dst高度 | Integer | 高度 to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha 值 (`0`-`255`).         |

### 返回

無返回。

---

## Suika.drawImageSub()

Draw an image with subtractive blending.

### 參數（字典）

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dst寬度  | Integer | 寬度 to draw.                   |
| dst高度 | Integer | 高度 to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha 值 (`0`-`255`).         |

### 返回

無返回。

---

## Suika.makeColor()

Create a pixel 值 from RGBA components.

### 參數（字典）

| Parameter | Type    | Description      |
|-----------|---------|------------------|
| r         | Integer | Red (0-255).     |
| g         | Integer | Green (0-255).   |
| b         | Integer | Blue (0-255).    |
| a         | Integer | Alpha (0-255).   |

### 返回

A pixel 值.

---

## Suika.fillImageRect()

Fill a rectangular area on an image with a color.

### 參數（字典）

| Parameter | Type    | Description                         |
|-----------|---------|-------------------------------------|
| image     | Object  | Target image.                       |
| left      | Integer | X coordinate.                       |
| top       | Integer | Y coordinate.                       |
| width     | Integer | 寬度.                              |
| height    | Integer | 高度.                             |
| color     | Integer | Color created by Suika.makeColor(). |

### 返回

無返回。

---

## Suika.reloadStageImages()

Reload the stage images by the config.

### 參數（字典）

無參數。

### 返回

Boolean that represents success or failure.

---

## Suika.reloadStage位置s()

Reload the stage positions by the config.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.getLayerX()

Get the current position of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

Integer 值 of the coordinate.

---

## Suika.getLayerY()

Get the current position of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

Integer 值 of the coordinate.

---

## Suika.setLayer位置()

Set the position of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| x         | Integer | X coordinate.              |
| y         | Integer | Y coordinate.              |

### 返回

無返回。

---

## Suika.getLayerScaleX()

Get the X scaling factor of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

Float 值 of the scale.

---

## Suika.getLayerScaleY()

Get the Y scaling factor of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

Float 值 of the scale.

---

## Suika.setLayerScale()

Set the scaling factor of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| scale_x   | Float   | Horizontal scale.          |
| scale_y   | Float   | Vertical scale.            |

### 返回

無返回。

---

## Suika.getLayerRotate()

Get the rotation angle of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

返回浮點數。

---

## Suika.setLayerRotate()

Set the rotation angle of a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| rot       | Float   | 旋轉角度 in radians. |

### 返回

無返回。

---

## Suika.getLayerDim()

獲取特定圖層的昏暗狀態。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

返回布爾值。

---

## Suika.setLayerDim()

設置特定圖層的昏暗狀態。

### 參數（字典） (Set)

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| dim       | Boolean | 是否昏暗圖層。 |

### 返回

無返回。

---

## Suika.getLayerAlpha()

獲取特定圖層的透明度。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |

### 返回

返回整數。

---

## Suika.setLayerAlpha()

設置特定圖層的透明度。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| alpha     | Integer | Alpha 值 (0-255).       |

### 返回

無返回。

---

## Suika.setLayerBlend()

Set the blending mode for a layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| blend     | Integer | Blend mode (Alpha, Add, Sub). |

### 返回

無返回。

---

## Suika.setLayerFile()

Set a file to be displayed on a layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| file_名稱 | String  | Path to the image file.    |

### 返回

Boolean that represents success or failure.

---

## Suika.setLayerFrame()

Set the frame 索引 for eye blinking and lip synchronization.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | 舞台圖層的索引。 |
| frame     | Integer | Frame 索引.               |

### 返回

無返回。

---

## Suika.getLayerText()

Get the string displayed on a text layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the text layer.   |

### 返回

返回字符串。

---

## Suika.setLayerText()

Set the string displayed on a text layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the text layer.   |
| text      | String  | Text message to set.       |

### 返回

無返回。

---

## Suika.getSysBtnIdleImage()

Get the sysbtn idle image.

### 參數（字典）

無參數。

### 返回

Returns an image object.

---

## Suika.getSysBtnHoverImage()

Get the sysbtn hover image.

### 參數（字典）

無參數。

### 返回

Returns an image object.

---

## Suika.clearStageBasic()

Clear the basic layers.

### 參數（字典）

無參數。

### 返回

Returns an image object.

---

## Suika.clearStage()

Clear the stage and make it initial state.

### 參數（字典）

無參數。

### 返回

Returns an image object.

---

## Suika.chposToLayer()

Convert a character position to a stage layer 索引.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | 字符位置。   |

### 返回

Returns an integer.

---

## Suika.chposToEyeLayer()

Convert a character position to a stage layer 索引 (character eye).

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | 字符位置。   |

### 返回

Returns an integer.

---

## Suika.chposToLipLayer()

Convert a character position to a stage layer 索引 (character lip).

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | 字符位置。   |

### 返回

Returns an integer.

---

## Suika.layerToChpos()

Convert a stage layer 索引 to a character position.

### 參數（字典）

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| layer     | Integer | 圖層索引。          |

### 返回

Returns an integer.

---

## Suika.renderStage()

Render the stage with all stage layers.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.startFade()

Start a transition effect.

### 參數（字典）

| Parameter | Type    | Description                                  |
|-----------|---------|----------------------------------------------|
| desc      | Array   | Fade descriptor.                             |
| method    | String  | Fading method.                               |
| time      | Float   | Duration in seconds.                         |
| ruleImage | Object  | Rule image object (optional).                |

### 返回

Boolean 值.

---

## Suika.getShakeOffset()

Get the offset for the shake command.

### 參數（字典）

無參數。

### 返回

An object that contains:
* x
* y

---

## Suika.setShakeOffset()

Set the offset for the shake command.

### 參數（字典）

| Parameter | Type    | Description    |
|-----------|---------|----------------|
| x         | Integer | X offset.      |
| y         | Integer | Y offset.      |

### 返回

無返回。

---

## Suika.isFadeRunning()

Check if the fading is running.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.finishFade()

Immediately end the fading effect.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.setChNameMapping()

Specify a character 名稱 索引 for a character position.

### 參數（字典）

| Parameter   | Type    | Description                |
|-------------|---------|----------------------------|
| chpos       | Integer | 字符位置。        |
| chNameIndex | Integer | Character 名稱 索引.      |

### 返回

無返回。

---

## Suika.getTalkingChpos()

Get the position of the character currently speaking.

### 參數（字典）

無參數。

### 返回

Returns an integer.

---

## Suika.setChTalking()

設置正在說話的字符。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| chpos     | Integer | 字符位置。        |

### 返回

無返回。

---

## Suika.getTalkingChpos()

Get the talker character position.

### 參數（字典）

無參數。

### 返回

Returns an integer.

---

## Suika.updateChDimByTalkingCh()

Automatically update character dimming based on who is speaking.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.forceChDim()

Update the character dimming manually.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| chpos     | Integer | 字符位置。        |
| dim       | Boolean | Dim or not.                |

### 返回

無返回。

---

## Suika.getChDim()

Get the dimming state.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| chpos     | Integer | 字符位置。        |

### 返回

Returns a boolean.

---

## Suika.fillNameBox()

Fill the 名稱 box by the 名稱 box image.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.getNameBoxRect()

Get the 名稱 box position and size.

### 參數（字典）

無參數。

### 返回

Object.

* x
* y
* w
* h

---

## Suika.showNameBox()

Show or hides the 名稱 box.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| show      | Boolean | Show or hide.              |

### 返回

無返回。

---

## Suika.fillMessageBox()

Fill the message box by the message box image.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.showMessageBox()

Show or hide the message box.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| show      | Boolean | Whether to show the box.   |

### 返回

無返回。

---

## Suika.getMessageBoxRect()

Get the message box rect.

### 參數（字典）

無參數。

### 返回

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClick位置()

Set the click animation position.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| x | Integer | X 位置。 |
| y | Integer | Y 位置。 |

### 返回

無返回。

---

## Suika.showClick()

Show or hide the click animation.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| show      | Boolean | Show or hide.              |

### 返回

無返回。

---

## Suika.setClickIndex()

Set the 索引 of the click animation frame.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Frame 索引.               |

### 返回

無返回。

---

## Suika.getClickRect()

Get the click animation rect.

### 參數（字典）

無參數。

### 返回

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

Fill a choose box idle layer by the choose box idle image.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Choose box 索引.          |

### 返回

無返回。

---

## Suika.fillChooseBoxHoverImage()

Fill a choose box hover layer by the choose box hover image.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Choose box 索引.          |

### 返回

無返回。

---

## Suika.showChoosebox()

Show or hide a choice box.

### 參數（字典）

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| 索引      | Integer | Choice box 索引. (`0`-`7`) |
| showIdle   | Boolean | Show idle state.            |
| showHover  | Boolean | Show hover state.           |

### 返回

無返回。

---

## Suika.getChooseBoxRect()

Get the choose box rect.

### 參數（字典）

無參數。

### 返回

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

Show or hide the auto mode banner.

### 參數（字典）

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| show       | Boolean | Show or hide.               |

### 返回

無返回。

---

## Suika.showSkipModeBanner()

Show or hide the skip mode banner.

### 參數（字典）

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| show       | Boolean | Show or hide.               |

### 返回

無返回。

---

## Suika.renderImage()

Perform direct rendering of an image to the screen.

Note that you should consider using the stage layers for normal rendering.
This API is useful for effects.

### 參數（字典）

| Parameter | Omissible    | Type    | Description                                |
|-----------|--------------|---------|--------------------------------------------|
| dstLeft   | No           | Integer | Destination top-left X position.           |
| dstTop    | No           | Integer | Destination top-left Y position.           |
| image     | No           | Object  | Image.                                     |
| srcLeft   | No           | Integer | Source top-left X position.                |
| srcTop    | No           | Integer | Source top-left Y position.                |
| src寬度  | No           | Integer | Source width.                              |
| src高度 | No           | Integer | Source height.                             |
| alpha     | No           | Integer | Alpha 值. (`0`-`255`)                   |
| blend     | No           | Integer | Blend type.                                |

### Blend Types

| Name                 | Description       |
|----------------------|-------------------|
| Suika.BLEND_ALPHA    | Alpha blending.   |
| Suika.BLEND_ADD      | Add blending.     |
| Suika.BLEND_SUB      | Sub blending.     |

### 返回

無返回。

---

## Suika.renderImage3d()

Perform direct rendering of an image to the screen with 3D transformation.

Note that you should consider using the stage layers for normal rendering.
This API is useful for effects.

### 參數（字典）

| Parameter | Omissible    | Type    | Description                                |
|-----------|--------------|---------|--------------------------------------------|
| x1        | No           | Integer | Destination vertex 1 X position.           |
| y1        | No           | Integer | Destination vertex 1 Y position.           |
| x2        | No           | Integer | Destination vertex 2 X position.           |
| y2        | No           | Integer | Destination vertex 2 Y position.           |
| x3        | No           | Integer | Destination vertex 3 X position.           |
| y3        | No           | Integer | Destination vertex 3 Y position.           |
| x4        | No           | Integer | Destination vertex 4 X position.           |
| y4        | No           | Integer | Destination vertex 4 Y position.           |
| tex       | No           | Object  | Image.                                     |
| srcLeft   | No           | Integer | Source top-left X position.                |
| srcTop    | No           | Integer | Source top-left Y position.                |
| src寬度  | No           | Integer | Source width.                              |
| src高度 | No           | Integer | Source height.                             |
| alpha     | No           | Integer | Alpha 值. (`0`-`255`)                   |

### 返回

無返回。

---

## Suika.startKirakira()

Start Kirakira effect.

Kirakira effect is an animation that is shown at the screen position where the mouse cursor is clicked.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.renderKirakira()

Render Kirakira effect.

---

## Suika.setMixerInputFile()

Play a sound file on a specific mixer track.

### 參數（字典）

| Parameter | Omissible    | Type    | Description                                |
|-----------|--------------|---------|--------------------------------------------|
| track     | No           | String  | Mixer track 名稱.                          |
| file      | No           | String  | Path to the sound file.                    |
| isLooped  | Yes(`false`) | Boolean | Whether to loop the playback.              |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### 返回

Boolean that represents whether the file was opened successfully.

---

## Suika.setMixerVolume()

Set the volume for a specific mixer track.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track 名稱.                          |
| vol       | Float   | 音量級別 (0.0 to 1.0).                 |
| span      | Float   | Fade duration in seconds.                  |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### 返回

無返回。

---

## Suika.getMixerVolume()

Get the volume for a specific mixer track.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track 名稱.                          |
| volume    | Float   | 音量級別 (0.0 to 1.0).                 |
| span      | Float   | Fade duration in seconds.                  |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### 返回

返回浮點數。

---

## Suika.setMasterVolume()

Set the master volume affecting all tracks.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| volume    | Float   | 主音量 level (0.0 to 1.0).          |

### 返回

無返回。

---

## Suika.getMasterVolume()

Get the master volume affecting all tracks.

### 參數（字典）

無參數。

### 返回

返回浮點數。

---

## Suika.setMixerGlobalVolume()

Set the global volume for a track (often used for config settings).

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track 名稱.                          |
| vol       | Float   | 全局音量 level.                       |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### 返回

無返回。

---

## Suika.getMixerGlobalVolume()

Get the global volume for a track (often used for config settings).

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track 名稱.                          |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### 返回

返回浮點數。

---

## Suika.setCharacterVolume()

Set the volume for a specific character's voice.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 索引     | Integer | Character 名稱 索引.                      |
| vol       | Float   | 音量級別.                              |

### 返回

無返回。

---

## Suika.getCharacterVolume()

獲取特定字符聲音的音量。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| ch_索引  | Integer | Character 名稱 索引.                      |

### 返回

返回浮點數。

---

## Suika.isMixerSoundFinished()

檢查特定軌道上的播放是否已完成。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track 索引.                         |

### 返回

Boolean 值.

---

## Suika.getTrackFileName()

Get the file 名稱 of the sound currently loaded in a track.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track 索引.                         |

### 返回

String representing the file path.

---

## Suika.applyCharacterVolume()

Apply a character's specific volume setting to the VOICE track.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| ch        | Integer | Character 名稱 索引.                      |

### 返回

無返回。

---

## Suika.enableSysBtn()

Control the system button.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| isEnabled | Boolean | Enable the system button or not.           |

### 返回

無返回。

---

## Suika.isSysBtnEnabled()

Check if the system button is enabled.

### Parameters

無參數。

### 返回

Returns a boolean 值. 

---

## Suika.updateSysBtnState()

Update the mouse tracking for the system button.

### Parameters

無參數。

### 返回

無返回。

---

## Suika.isSysBtnPointed()

Check if the system button is pointed.

### Parameters

無參數。

### 返回

Returns a boolean 值.

---

## Suika.isSysBtnClicked()

Check if the system button is clicked.

### Parameters

無參數。

### 返回

Returns a boolean 值.

---

## Suika.drawTextOnLayer()

Draw a text on a specified layer.

### 參數（字典）

| Parameter    | Type    | Description               |
|--------------|---------|---------------------------|
| layer        | Integer | Target stage layer 索引. |
| fontType     | Integer | Font selection 索引.     |
| fontSize     | Integer | Size of the font.         |
| color        | Integer | Color.                    |
| outline寬度 | Integer | Outline width.            |
| outlineColor | Integer | Outline color.            |
| lineMargin   | Integer | Line margin.              |
| charMargin   | Integer | Character margin.         |
| x            | Integer | Bounding box X position.  |
| y            | Integer | Bounding box Y position.  |
| width        | Integer | Bounding box width.       |
| height       | Integer | Bounding box height.      |
| text         | String  | Text.                     |

### 返回

無返回。

---

## Suika.getString寬度()

Get the total width of a UTF-8 string.

### 參數（字典）

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| fontType  | Integer | Font selection 索引.  |
| fontSize  | Integer | Size of the font.      |
| text      | String  | Text.                  |

### 返回

Integer 值 of the width in pixels.

---

## Suika.getString高度()

Get the total height of a UTF-8 string.

### 參數（字典）

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| fontType  | Integer | Font selection 索引.  |
| fontSize  | Integer | Size of the font.      |
| text      | String  | Text.                  |

### 返回

Integer 值 of the height in pixels.

---

## Suika.drawGlyph()

繪製單個字形 onto an image.

### 參數（字典）

| Parameter     | Type    | Description                                |
|---------------|---------|--------------------------------------------|
| img           | Object  | Target image.                              |
| font_type     | Integer | Font selection 索引.                      |
| font_size     | Integer | Rendering font size.                       |
| base_font_size| Integer | Base font size for metrics.                |
| outline_size  | Integer | 寬度 of the outline.                      |
| x             | Integer | X coordinate.                              |
| y             | Integer | Y coordinate.                              |
| color         | Pixel   | Main text color.                           |
| outline_color | Pixel   | Outline color.                             |
| codepoint     | Integer | UTF-32 code point.                         |
| is_dim        | Boolean | Whether to apply dimming.                  |

### 返回

Boolean that represents success.

---

## Suika.createDrawMsg()

Create a complex message drawing context for high-level text rendering.

### 參數（字典）

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| image          | Integer  | Destination image.     |
| text           | String   | Message to draw.       |
| fontType       | Integer  | Font selection.        |
| fontSize       | Integer  | Font size.             |
| baseFontSize   | Integer  | Base font size.        |
| rubySize       | Integer  | Ruby size.             |
| outlineSize    | Integer  | Outline width.         |
| penX           | Integer  | Pen X position.        |
| penY           | Integer  | Pen Y position.        |
| area寬度      | Integer  | Draw area width.       |
| area高度     | Integer  | Draw area height.      |
| leftMargin     | Integer  | Left margin.           |
| rightMargin    | Integer  | Right margin.          |
| topMargin      | Integer  | Top margin.            |
| bottomMargin   | Integer  | Bottom margin.         |
| lineMargin     | Integer  | Line margin.           |
| charMargin     | Integer  | Character margin.      |
| color          | Integer  | Color.                 |
| outlineColor   | Integer  | Outline color.         |
| bgColor        | Integer  | Background color.      |
| fillBg         | Boolean  | Fill background?       |
| dim            | Boolean  | Dim?                   |
| ignoreLF       | Boolean  | Ignore LF?             |
| ignoreFont     | Boolean  | Ignore font change?    |
| ignoreOutline  | Boolean  | Ignore outline change? |
| ignoreColor    | Boolean  | Ignore color change?   |
| ignoreSize     | Boolean  | Ignore size change?    |
| ignore位置 | Boolean  | Ignore cursor change?  |
| ignoreRuby     | Boolean  | Ignore ruby?           |
| ignoreWait     | Boolean  | Ignore inline wait?    |
| inlineWaitHook | Function | Inline wait hook.      |
| tategaki       | Boolean  | Use tategaki?          |

### 返回

A message drawing context object.

---

## Suika.destroyDrawMsg()

Destroy a message drawing context.

### 參數（字典）

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |

### 返回

無返回。

---

## Suika.countDrawMsgChars()

Count the remaining characters excluding escape sequences.

### 參數（字典）

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |

### 返回

Returns an integer.

---

## Suika.drawMessage()

Draw characters in a message up to (maxChars) characters.

### 參數（字典）

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |
| maxChars       | Integer  | Max chars.             |

### 返回

Returns an integer that indicates the count of characters drawn in the call.

---

## Suika.getDrawMsgPenPosition()

Get the current pen position from a drawing context.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| context   | Object  | Drawing context.                           |

### 返回

An object containing `x` and `y`.

---

## Suika.isEscapeSequenceChar()

Check if a character is part of an escape sequence.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| c         | String | Character to check.                        |

### 返回

Boolean 值.

---

## Suika.moveToTagFile()

Load a new tag file and move the execution point to its beginning.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the .novel or script file. |

### 返回

Boolean that represents success or failure.

---

## Suika.getTagCount()

Get the total number of tags in the current script file.

### 參數（字典）

無參數。

### 返回

Integer representing the tag count.

---

## Suika.moveToTagIndex()

Move the execution pointer to a specific tag 索引.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Target tag 索引. |

### 返回

Boolean 值.

---

## Suika.moveToNextTag()

Move the execution pointer to the very next tag.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.moveToLabelTag()

Jump to a specific label.

### 參數（字典）

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| 名稱      | String | Target label 名稱.      |

### 返回

Boolean 值.

---

## Suika.moveToMacroTag()

Jump to a specific macro by 名稱.

### 參數（字典）

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| 名稱      | String | Target macro 名稱.      |

### 返回

Boolean 值.

---

## Suika.moveToElseTag()

Jump to a correspoinding else/elseif/endif tag.

### 參數（字典）

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| 名稱      | String | Target macro 名稱.      |

### 返回

Boolean 值.

---

## Suika.moveToEndIfTag()

Jump to a correspoinding endif tag.

### 參數（字典）

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| 名稱      | String | Target macro 名稱.      |

### 返回

Boolean 值.

---

## Suika.moveToEndMacroTag()

Jump to a correspoinding endmacro tag.

### 參數（字典）

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| 名稱      | String | Target macro 名稱.      |

### 返回

Boolean 值.

---

## Suika.getTagFileName()

Get the current script file 名稱 current tag.

### 參數（字典）

無參數。

### 返回

String representing the file 名稱.

---

## Suika.getTagName()

獲取當前標籤的名稱。

### 參數（字典）

無參數。

### 返回

String representing the tag 名稱.

---

## Suika.getTagPropertyCount()

Get the number of the properties of the current tag.

### 參數（字典）

無參數。

### 返回

String representing the 名稱 or 值.

---

## Suika.getTagPropertyName()

Iterate through and retrieve the properties (arguments) of the current
tag.

### 參數（字典）

| Parameter | Type    | Description       |
|-----------|---------|-------------------|
| 索引     | Integer | Property 索引.   |

### 返回

String representing the 名稱.

---

## Suika.getTagPropertyValue()

Iterate through and retrieve the properties (arguments) of the current
tag.

### 參數（字典） (for PropertyName/Value)

| Parameter | Type    | Description       |
|-----------|---------|-------------------|
| 索引     | Integer | Property 索引.   |

### 返回

String representing the 值.

---

## Suika.getTagArgBool()

Get a specific argument of the current tag, with support for default
值s and optionality.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|-----------------------------------|
| 名稱      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Boolean | Default 值 if missing.         |

### 返回

The 值 of the argument in the requested type.

---

## Suika.getTagArgInt()

Get a specific argument of the current tag, with support for default
值s and optionality.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|-----------------------------------|
| 名稱      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Integer | Default 值 if missing.         |

### 返回

The 值 of the argument in the requested type.

---

## Suika.getTagArgFloat()

Get a specific argument of the current tag, with support for default
值s and optionality.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|-----------------------------------|
| 名稱      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Float   | Default 值 if missing.         |

### 返回

The 值 of the argument in the requested type.

---

## Suika.getTagArgString()

Get a specific argument of the current tag, with support for default
值s and optionality.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|-----------------------------------|
| 名稱      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | String  | Default 值 if missing.         |

### 返回

The 值 of the argument in the requested type.

---

## Suika.evaluateTag()

Evaluate the property 值s of the current tag to expand inline
variables. (`${var名稱}` form)

Calling this API updates the cache for the property 值s.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.pushTagStackIf()

Manage the internal stack for `[if]` conditional blocks.

This API marks the `if` block position for nested block processing.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.popTagStackIf()

Manage the internal stack for `if` conditional blocks.

This API marks the end of `if` block for nested block processing.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.pushTagStackWhile()

Manage the internal stack for loops (`while`).

This API marks the `while` block for nested block processing.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.popTagStackWhile()

Manage the internal stack for loops (`while`).

This API marks the end of `while` block for nested block processing.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.pushTagStackFor()

Manage the internal stack for loops (`for`).

This API marks the `for` block for nested block processing.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.popTagStackFor()

Manage the internal stack for loops (`for`).

This API marks the end of `for` block for nested block processing.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.loadAnimeFromFile()

Load an animation definition from a file and register it.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| file      | String  | Path to the anime file.                    |
| reg_名稱  | String  | Registration 名稱 for the anime.           |

### 返回

An array of boolean that indicate each layer is loaded or not.

---

## Suika.newAnimeSequence()

Begin describing a new animation sequence for a specific layer.
This API is used for manually generated animations.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer 索引.                  |

### 返回

Boolean that represents success.

---

## Suika.addAnimeSequencePropertyF()

Add a float property (e.g., position, alpha) to the current anime sequence.
This API is used for manually generated animations.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 鍵       | String  | Property 鍵 (e.g., "x", "y", "a").        |
| val       | Float   | Target 值.                              |

### 返回

Boolean 值.

---

## Suika.addAnimeSequencePropertyI()

Add an integer property (e.g., position, alpha) to the current anime sequence.
This API is used for manually generated animations.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 鍵       | String  | Property 鍵 (e.g., "x", "y", "a").        |
| val       | Integer | Target 值.                              |

### 返回

Boolean 值.

---

## Suika.startLayerAnime()

Start the registered animation sequence for a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer 索引.                  |

### 返回

Boolean 值.

---

## Suika.isAnimeRunning()

Check the overall animation status.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isAnimeFinishedForLayer()

Check if a specific layer's animation has ended.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer 索引.  |

### 返回

Boolean 值.

---

## Suika.updateAnimeFrame()

Update the animation frame state. Usually called once per frame.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.loadEyeImageIfExists()

Manage eye-blinking (eye-patch) image and animation for a character position.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position (Left, Center, etc.).   |
| file      | String  | Path to the eye image file.                |

### 返回

Boolean 值.

---

## Suika.reloadEyeAnime()

Restart the eye-blinking (eye-patch) animation for a character position.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position (Left, Center, etc.).   |

### 返回

Boolean 值.

---

## Suika.runLipAnime()

Start lip-sync animation based on the message content for a character.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | 字符位置。                        |
| text      | String  | The message text to sync with.             |

### 返回

無返回。

---

## Suika.stopLipAnime()

Stop lip-sync animation.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | 字符位置。                        |

### 返回

無返回。

---

## Suika.clearLayerAnimeSequence()

Clear animation sequences for a specific layer.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer 索引.  |

### 返回

無返回。

---

## Suika.clearAllAnimeSequence()

Clear animation sequences for all layers.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.setVariableInt()

Set a 值 to a local or global variable.

### 參數（字典）

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| 名稱      | String  | Name of the variable.  |
| 值     | Integer | Value to set           |

### 返回

Boolean that represents success or failure.

---

## Suika.setVariableFloat()

Set a 值 to a local or global variable.

### 參數（字典）

| Parameter | Type    | Description             |
|-----------|---------|-------------------------|
| 名稱      | String  | Name of the variable.   |
| 值     | Float   | Value to set            |

### 返回

Boolean that represents success or failure.

---

## Suika.setVariableString()

Set a 值 to a local or global variable.

### 參數（字典）

| Parameter | Type    | Description             |
|-----------|---------|-------------------------|
| 名稱      | String  | Name of the variable.   |
| 值     | String  | Value to set            |

### 返回

Boolean that represents success or failure.

---

## Suika.getVariableInt()

Get the current 值 of a variable.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 名稱      | String | Name of the variable.                      |

### 返回

The 值 of the variable in integer.

---

## Suika.getVariableFloat()

Get the current 值 of a variable.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 名稱      | String | Name of the variable.                      |

### 返回

The 值 of the variable in float.

---

## Suika.getVariableString()

Get the current 值 of a variable.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 名稱      | String | Name of the variable.                      |

### 返回

The 值 of the variable in string

---

## Suika.unsetVariable()

取消設置 (delete) a specific variable.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 名稱      | String | Name of the variable to unset.             |

### 返回

無返回。

---

## Suika.unsetLocalVariables()

取消設置 (delete) all local variables.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.makeVariableGlobal()

Set a variable to be global (persistent across saves).

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 名稱      | String  | Name of the variable.                      |
| is_global | Boolean | Whether to make it global.                 |

### 返回

Boolean 值.

---

## Suika.isGlobalVariable()

Check the variable's global status.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| 名稱      | String  | Name of the variable.                      |

### 返回

Boolean 值.

---

## Suika.getVariableCount()

Get the number of variables.

### 參數（字典）

無參數。

### 返回

Integer for count.

---

## Suika.getVariableName()

Iterate through the registered variables.

### 參數（字典） (for getVariableName)

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the variable.     |

### 返回

String for 名稱.

---

## Suika.checkVariableExists()

Check if a variable with the specified 名稱 exists.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 名稱      | String | Name to check.                             |

### 返回

Boolean 值.

---

## Suika.executeSaveGlobal()

Execute a global save.
Global data typically includes persistent settings.

### 參數（字典）

無參數。

### 返回

Boolean that represents success or failure.

---

## Suika.executeLoadGlobal()

Execute a global load.
Global data typically includes persistent settings.

### 參數（字典）

無參數。

### 返回

Boolean that represents success or failure.

---

## Suika.executeSaveLocal()

Save the game progress to a specific slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

Boolean that represents success or failure.

---

## Suika.executeLoadLocal()

Load game progress from a specific slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

Boolean that represents success or failure.

---

## Suika.checkSaveExists()

Check if the save data exists for the specified slot 索引.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

Boolean 值.

---

## Suika.deleteLocalSave()

Delete a local save slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

無返回。

---

## Suika.deleteGlobalSave()

Delete the entire global save data.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.checkRightAfterLoad()

Check if the current frame is immediately following a successful load operation.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.getSaveTimestamp()

Get the timestamp (Unix time) when the save data was created.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

Integer (timestamp).

---

## Suika.getLatestSaveIndex()

Get the 索引 of the most recently updated save slot.

### 參數（字典）

無參數。

### 返回

Integer representing the slot 索引.

---

## Suika.getSaveChapterName()

Retrieve the chapter title stored in a save slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

String representing the chapter 名稱.

---

## Suika.getSaveLastMessage()

Retrieve the last displayed message stored in a save slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

String representing the message.

---

## Suika.getSaveThumbnail()

Get the thumbnail image associated with a save slot.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index of the save slot.    |

### 返回

An image object.

---

## Suika.clearHistory()

Clear all messages from the history (backlog).

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.addHistory()

Add a new entry to the history.

### 參數（字典）

| Parameter        | Type    | Description                                |
|------------------|---------|--------------------------------------------|
| 名稱             | String  | Character 名稱.                            |
| 消息              | String  | 消息文本。                              |
| voice            | String  | Path to the voice file.                    |
| bodyColor        | Integer | Body color.                                |
| bodyOutlineColor | Integer | Body outline color.                        |
| 名稱Color        | Integer | Name color.                                |
| 名稱OutlineColor | Integer | Name outline color.                        |

### 返回

Boolean that represents success.

---

## Suika.getHistoryCount()

Get the total number of entries currently in the history.

### 參數（字典）

無參數。

### 返回

Integer representing the history count.

---

## Suika.getHistoryName()

Retrieve the 名稱 at a specific history 索引.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index in the history.      |

### 返回

String 值.

---

## Suika.getHistoryMessage()

Retrieve the message at a specific history 索引.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index in the history.      |

### 返回

String 值.

---

## Suika.getHistoryVoice()

Retrieve the voice path at a specific history 索引.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| 索引     | Integer | Index in the history.      |

### 返回

String 值.

---

## Suika.loadSeen()

Load the seen (read) flags for the current script file.

### 參數（字典）

無參數。

### 返回

Boolean that represents success.

---

## Suika.saveSeen()

Save the seen (read) flags for the current script file.

### 參數（字典）

無參數。

### 返回

Boolean that represents success.

---

## Suika.getSeenFlags()

Get the seen status for the current tag.

### 參數（字典）

無參數。

### 返回

Get returns Integer.

For a `[text]` tag, `0` means unread and `1` means read.

For a `[choose]` tag, each bit indicates the option is selected before.

---

## Suika.setSeenFlags()

Set the seen status for the current tag.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|----------------------------|
| flag      | Integer | Seen status flag.          |

### 返回

無返回。

---

## Suika.loadGUIFile()

Load a GUI definition file and prepare it for execution.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|---------|--------------------------------------------|
| file      | String  | Path to the .gui file.                     |
| sys       | Boolean | Whether it's a system GUI (Save/Load/etc). |

### What is System GUI

System GUI is typically called when `[text]` or `[choose]` is running,
and the control will return to the interrupted tag.

### 返回

Boolean that represents success or failure.

---

## Suika.startGUI()

Start the loaded GUI.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.stopGUI()

Stop the currently running GUI.

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isGUIRunning()

Check if a GUI is currently active.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.isGUIFinished()

Check if a GUI has completed its operation.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.getGUIResultLabel()

Get the label of the button that was selected to finish the GUI.

### 參數（字典）

無參數。

### 返回

String representing the result label.

---

## Suika.isGUIResultTitle()

Check if the GUI was closed with a "back to title" action.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.checkIfSavedInGUI()

Check if a save operation was performed while the GUI was active.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.checkIfLoadedInGUI()

Check if a load operation was performed while the GUI was active.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.checkRightAfterSysGUI()

Check if the current frame is immediately following a return from a system GUI.

### 參數（字典）

無參數。

### 返回

Boolean 值.

---

## Suika.getMillisec()

Get a lap time since the time origin in milliseconds.

### 參數（字典）

無參數。

### 返回

Integer in milliseconds.

---

## Suika.checkFileExists()

Check if a file exists.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the file.                          |

### 返回

返回布爾值。

---

## Suika.readFileContent()

Read an entire file content.

### 參數（字典） (for readFileContent)

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the file.                          |

### 返回

Returns a string.

---

## Suika.writeSaveData()

Directly write raw save data associated with a 鍵.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | Unique 鍵 for the data.                   |
| data      | String | Data to write/read.                        |

### 返回

Boolean that represents success or failure.

---

## Suika.readSaveData()

Directly read raw save data associated with a 鍵.

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 鍵       | String | Unique 鍵 for the data.                   |

### 返回

Boolean that represents success or failure.

---

## Suika.playVideo()

Control video playback.

### 參數（字典） (for playVideo)

| Parameter    | Type    | Description                          |
|--------------|---------|--------------------------------------|
| file         | String  | Path to the video file.              |
| is_skippable | Boolean | Whether the user can skip the video. |

### 返回

Play 返回布爾值；IsPlaying 返回布爾值。

---

## Suika.stopVideo()

停止視頻播放。

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.isVideoPlaying()

檢查視頻是否正在播放。

### 參數（字典）

無參數。

### 返回

返回布爾值。

---

## Suika.isFullScreenSupported()

檢查全屏模式能力。

### 參數（字典）

無參數。

### 返回

布爾值。

---

## Suika.enterFullScreenMode()

進入全屏模式。

### 參數（字典）

無參數。

### 返回

無返回。

---

## Suika.speakText()

執行給定消息的文本到語音(TTS)。

### 參數（字典）

| 參數 | 類型 | 說明 |
|-----------|--------|--------------------------------------------|
| 消息       | String | 要朗讀的文本。                         |

### 返回

無返回。
