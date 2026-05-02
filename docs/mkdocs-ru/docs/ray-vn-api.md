Справочник по Ray VN API
=========================

`VN API (Suika.*)` предназначен для создания визуальных новелл.

Каждая функция API `Suika.*` принимает только один аргумент.
Аргумент должен быть словарём, а параметры функции должны храниться в словаре как пары ключ-значение.
В этом документе под словом "параметр" понимается пара ключ-значение в этом словаре.

## Содержание

* Базовые
    * [Suika.loadPlugin()](#suikaloadplugin)
* Конфигурация
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
* Ввод
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
* Игра
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
* Изображение
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
    * [Suika.getImageWidth()](#suikagetimagewidth)
    * [Suika.getImageHeight()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImage()](#suikadrawimage)
    * [Suika.drawImage3D()](#suikadrawimage3d)
    * [Suika.makeColor()](#suikamakecolor)
    * [Suika.fillImageRect()](#suikafillimagerect)
* Сцена
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
* Микшер
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
* SysBtn
    * [Suika.enableSysBtn()](#suikaenablesysbtn)
    * [Suika.isSysBtnVisible()](#suikaisysbtnvisible)
    * [Suika.updateSysBtnState()](#suikaupdatesysbtnstate)
    * [Suika.isSysBtnPointed()](#suikaisysbtnpointed)
    * [Suika.isSysBtnClicked()](#suikaisysbtnclicked)
* Текст
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
* Тег
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
* Анимация
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
* Переменные
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
* Сохранение
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
* История
    * [Suika.clearHistory()](#suikaclearhistory)
    * [Suika.addHistory()](#suikaaddhistory)
    * [Suika.getHistoryCount()](#suikagethistorycount)
    * [Suika.getHistoryName()](#suikagethistoryname)
    * [Suika.getHistoryMessage()](#suikagethistorymessage)
    * [Suika.getHistoryVoice()](#suikagethistoryvoice)
* Просмотрено
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

Загружает плагин.

Только этот API принимает аргумент не в виде словаря.

### Параметры (прямые)

| Параметр | Тип    | Описание        |
|----------|--------|-----------------|
| name     | String | Имя плагина.    |

### Возврат

Возврат отсутствует.

---

## Suika.setConfig()

Устанавливает конфигурацию.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Ключ конфигурации.                         |
| value    | String | Значение конфигурации.                     |

### Возврат

Возврат отсутствует.

---

## Suika.getConfigCount()

Возвращает количество ключей конфигурации.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, показывающее количество ключей конфигурации.

---

## Suika.getConfigKey()

Возвращает ключ конфигурации по индексу.

### Параметры (словарь)

| Параметр | Тип     | Описание                                   |
|----------|---------|--------------------------------------------|
| index    | Integer | Индекс конфигурации.                       |

### Возврат

Строка, представляющая ключ конфигурации по указанному индексу.

---

## Suika.isGlobalSaveConfig()

Проверяет, хранится ли ключ конфигурации в глобальных данных сохранения.

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Булево значение, показывающее, сохранена ли конфигурация глобально.

---

## Suika.isLocalSaveConfig()

Проверяет, хранится ли ключ конфигурации в локальных данных сохранения.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Булево значение, показывающее, сохранена ли конфигурация локально.

---

## Suika.getConfigType()

Возвращает тип значения конфигурации. ("s", "b", "i", "f")

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Одна из следующих строк.

| Значение | Значение                  |
|----------|--------------------------|
| "s"      | Конфигурация - строка.    |
| "b"      | Конфигурация - логическое значение. |
| "i"      | Конфигурация - целое число. |
| "f"      | Конфигурация - число с плавающей точкой. |

---

## Suika.getStringConfig()

Возвращает строковое значение конфигурации.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Строковое значение конфигурации.

---

## Suika.getBoolConfig()

Возвращает логическое значение конфигурации.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Логическое значение конфигурации.

---

## Suika.getIntConfig()

Возвращает целочисленное значение конфигурации.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Целочисленное значение конфигурации.

---

## Suika.getFloatConfig()

Возвращает значение конфигурации с плавающей точкой.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Значение конфигурации с плавающей точкой.

---

## Suika.getConfigAsString()

Возвращает значение конфигурации в виде строки.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| key      | String | Имя ключа.                                 |

### Возврат

Строковое представление значения конфигурации.

---

## Suika.compareLocale()

Проверяет, совпадает ли указанная локаль с текущей.

### Параметры (словарь)

| Параметр | Тип    | Описание                                   |
|----------|--------|--------------------------------------------|
| locale   | String | Имя локали.                                |


### Возврат

Булево значение, показывающее, совпадает ли указанная локаль с текущей.

---

## Suika.getMousePosX()

Возвращает X-координату мыши.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее текущую X-координату мыши.

---

## Suika.getMousePosY()

Возвращает Y-координату мыши.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее текущую Y-координату мыши.

---

## Suika.isMouseLeftPressed()

Проверяет, нажата ли левая кнопка мыши.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее, удерживается ли левая кнопка.

---

## Suika.isMouseRightPressed()

Проверяет, нажата ли правая кнопка мыши.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее, удерживается ли правая кнопка.

---

## Suika.isMouseLeftClicked()

Проверяет, была ли левая кнопка мыши нажата и отпущена.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее, произошло ли левое нажатие в текущем кадре.

---

## Suika.isMouseRightClicked()

Проверяет, была ли правая кнопка мыши нажата и отпущена.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее, произошло ли правое нажатие в текущем кадре.

---

## Suika.isMouseDragging()

Проверяет, выполняется ли перетаскивание мышью.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее, перемещается ли мышь при нажатой кнопке.

---

## Suika.isReturnKeyPressed()

Проверяет, нажата ли клавиша Enter.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isSpaceKeyPressed()

Проверяет, нажата ли клавиша пробела.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isEscapeKeyPressed()

Проверяет, нажата ли клавиша Escape.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isUpKeyPressed()

Проверяет, нажата ли клавиша вверх.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isDownKeyPressed()

Проверяет, нажата ли клавиша вниз.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isLeftKeyPressed()

Проверяет, нажата ли клавиша влево.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isRightKeyPressed()

Проверяет, нажата ли клавиша вправо.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isPageUpKeyPressed()

Проверяет, нажата ли клавиша Page Up.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isPageDownKeyPressed()

Проверяет, нажата ли клавиша Page Down.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isControlKeyPressed()

Проверяет, нажата ли клавиша Control.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isSKeyPressed()

Проверяет, нажата ли клавиша S.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isLKeyPressed()

Проверяет, нажата ли клавиша L.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isHKeyPressed()

Проверяет, нажата ли клавиша H.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isTouchCanceled()

Проверяет, отменён ли сенсорный ввод.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isSwiped()

Проверяет, был ли выполнен свайп.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.clearInputState()

Сбрасывает состояние ввода, чтобы предотвратить дальнейшую обработку ввода в текущем кадре.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.


---

## Suika.startCommandRepetition()

Запускает выполнение команды на несколько кадров.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.stopCommandRepetition()

Останавливает выполнение команды на несколько кадров.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isInCommandRepetition()

Проверяет, выполняется ли команда на несколько кадров.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.setMessageActive()

Активирует состояние отображения сообщения.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.clearMessageActive()

Сбрасывает состояние отображения сообщения.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isMessageActive()

Проверяет, включено ли состояние отображения сообщения.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.startAutoMode()

Запускает авто-режим.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.stopAutoMode()

Останавливает авто-режим.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isAutoMode()

Проверяет, включён ли авто-режим.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.startSkipMode()

Запускает режим пропуска.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.stopSkipMode()

Останавливает режим пропуска.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isSkipMode()

Проверяет, включён ли режим пропуска.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.setSaveLoad()

Устанавливает разрешение на сохранение и загрузку.

### Параметры (словарь)

| Параметр | Тип     | Описание                          |
|----------|---------|-----------------------------------|
| enable   | Boolean | Включить сохранение и загрузку.   |

### Возврат

Возврат отсутствует.

---

## Suika.isSaveLoadEnabled()

Возвращает настройку разрешения сохранения и загрузки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.setNonInterruptible()

Устанавливает режим без прерывания.

### Параметры (словарь)

| Параметр | Тип     | Описание                   |
|----------|---------|----------------------------|
| enable   | Boolean | Режим без прерывания.      |

### Возврат

Возврат отсутствует.

---

## Suika.isNonInterruptible()

Возвращает настройку режима без прерывания.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.setPenPosition()

Устанавливает позицию пера для рисования текста.

### Параметры (словарь)

| Параметр | Тип     | Описание       |
|----------|---------|----------------|
| x        | Integer | X-координата.  |
| y        | Integer | Y-координата.  |

### Возврат

Возврат отсутствует.

---

## Suika.getPenPositionX()

Возвращает X-позицию пера.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число.

---

## Suika.getPenPositionY()

Возвращает Y-позицию пера.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число.

---

## Suika.pushForCall()

Помещает точку возврата в стек вызовов.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| file     | String  | Имя файла сценария. |
| index    | Integer | Индекс команды.     |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.popForReturn()

Извлекает точку возврата из стека вызовов.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает словарь, содержащий:

* obj.file: имя файла
* obj.index: индекс тега

---

## Suika.readCallStack()

Читает элемент стека вызовов по указанному индексу.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|----------------------|
| sp       | Integer | Индекс элемента стека. |

### Возврат

Возвращает словарь, содержащий:

* obj.file: имя файла
* obj.index: индекс тега

---

## Suika.writeCallStack()

Записывает элемент стека вызовов по указанному индексу.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| sp       | Integer | Индекс элемента стека. |
| file     | String  | Имя файла сценария. |
| index    | Integer | Индекс тега.        |

### Возврат

Возврат отсутствует.

---

## Suika.setCallArgument()

Устанавливает аргумент вызова для GUI или анимации.

### Параметры (словарь)

| Параметр | Тип     | Описание           |
|----------|---------|---------------------|
| index    | Integer | Индекс аргумента.   |
| value    | String  | Значение аргумента. |

### Возврат

Булево значение.

---

## Suika.getCallArgument()

Возвращает аргумент вызова.

### Параметры (словарь)

| Параметр | Тип     | Описание           |
|----------|---------|---------------------|
| index    | Integer | Индекс аргумента.   |

### Возврат

Строковое значение.

---

## Suika.isPageMode()

Проверяет, включён ли режим страниц сценария.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.appendBufferedMessage()

Добавляет строку в буфер режима страниц.

### Параметры (словарь)

| Параметр | Тип    | Описание    |
|----------|---------|------------|
| message  | String | Сообщение. |

### Возврат

Возврат отсутствует.

---

## Suika.getBufferedMessage()

Возвращает строку буфера режима страниц.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает строку.

---

## Suika.clearBufferedMessage()

Очищает строку буфера режима страниц.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.resetPageLine()

Сбрасывает счётчик строк сообщения на странице.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.incPageLine()

Увеличивает счётчик строк на странице.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isPageTop()

Проверяет, находимся ли мы на первой строке страницы.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.registerBGVoice()

Регистрирует BGVoice.

### Параметры (словарь)

| Параметр | Тип    | Описание       |
|----------|--------|----------------|
| file     | String | Файл BGVoice.  |

### Возврат

Возврат отсутствует.

---

## Suika.getBVoice()

Возвращает BGVoice.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает строку с именем файла.

---

## Suika.setBGVoicePlaying()

Устанавливает состояние воспроизведения BGVoice.

### Параметры (словарь)

| Параметр  | Тип     | Описание |
|-----------|---------|----------|
| isPlaying | Boolean | Состояние. |

### Возврат

Возврат отсутствует.

---

## Suika.isBGVoicePlaying()

Проверяет, воспроизводится ли BGVoice.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.setChapterName()

Устанавливает имя главы.

### Параметры (словарь)

| Параметр | Тип    | Описание        |
|----------|--------|-----------------|
| name     | String | Имя главы.      |

### Возврат

Возврат отсутствует.

---

## Suika.getChapterName()

Возвращает имя главы.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает строку.

---

## Suika.setLastMessage()

Устанавливает последнее сообщение.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| message  | String  | Сообщение.          |
| isAppend | Boolean | Добавить или заменить. |

### Возврат

Возврат отсутствует.

---

## Suika.getLastMessage()

Возвращает последнее сообщение.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает строку.

---

## Suika.setTextSpeed()

Устанавливает скорость текста.

### Параметры (словарь)

| Параметр | Тип   | Описание        |
|----------|-------|-----------------|
| speed    | Float | Скорость текста. |

### Возврат

Возврат отсутствует.

---

## Suika.getTextSpeed()

Возвращает скорость текста.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.setAutoSpeed()

Устанавливает скорость авто-режима.

### Параметры (словарь)

| Параметр | Тип   | Описание          |
|----------|-------|-------------------|
| speed    | Float | Скорость авто-режима. |

### Возврат

Возврат отсутствует.

---

## Suika.getAutoSpeed()

Возвращает скорость авто-режима.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.markLastEnglishTagIndex()

Помечает последний английский индекс.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.getLastEnglishTagIndex()

Возвращает последний английский индекс.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает целое число.

---

## Suika.clearLastEnglishTagIndex()

Очищает последний английский индекс.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.getLastTagName()

Возвращает имя последнего тега.


### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает строку.

---

## Suika.createImageFromFile()

Загружает изображение из файла.

### Параметры (словарь)

| Параметр | Тип    | Описание                  |
|----------|--------|---------------------------|
| file     | String | Путь к файлу изображения. |

### Возврат

Объект изображения или `null` при неудаче.

---

## Suika.createImage()

Создаёт новое пустое изображение.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|----------------------|
| width    | Integer | Ширина изображения.  |
| height   | Integer | Высота изображения.  |

### Возврат

Объект изображения.

---

## Suika.getImageWidth()

Возвращает ширину изображения.

### Параметры (словарь)

| Параметр | Тип    | Описание          |
|----------|--------|-------------------|
| img      | Object | Объект изображения. |

### Возврат

Целое число, представляющее ширину.

---

## Suika.getImageHeight()

Возвращает высоту изображения.

### Параметры (словарь)

| Параметр | Тип    | Описание          |
|----------|--------|-------------------|
| image    | Object | Объект изображения. |

### Возврат

Целое число, представляющее высоту.

---

## Suika.destroyImage()

Уничтожает изображение и освобождает память.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| image    | Object | Объект изображения для уничтожения. |

### Возврат

Возврат отсутствует.

---

## Suika.drawImage()

Копирует изображение в другое изображение без смешивания.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| dstImage | Object  | Целевое изображение.      |
| dstLeft  | Integer | X-координата в цели.      |
| dstTop   | Integer | Y-координата в цели.      |
| srcImage | Object  | Исходное изображение.     |
| dstWidth | Integer | Ширина отрисовки.         |
| dstHeight| Integer | Высота отрисовки.         |
| srcLeft  | Integer | X-координата в источнике. |
| srcTop   | Integer | Y-координата в источнике. |
| alpha    | Integer | 0-255                     |
| blend    | Integer | Тип смешивания.           |

### Типы смешивания

| Тип                | Описание                           |
|--------------------|------------------------------------|
| Suika.BLEND_COPY   | Копирование.                       |
| Suika.BLEND_ALPHA  | Альфа-смешивание.                  |
| Suika.BLEND_ADD    | Аддитивное смешивание.             |
| Suika.BLEND_SUB    | Субтрактивное смешивание.          |
| Suika.BLEND_DIM    | Альфа-смешивание RGB 50%.          |
| Suika.BLEND_GLYPH  | Альфа-смешивание для обычных глифов. |
| Suika.BLEND_EMOJI  | Альфа-смешивание для глифов-эмодзи. |

### Возврат

Возврат отсутствует.

---

## Suika.drawImage3D()

Копирует изображение в другое изображение без смешивания.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| dstImage | Object  | Целевое изображение.      |
| x1       | Integer | Координата x1 в цели.     |
| y1       | Integer | Координата y1 в цели.     |
| x2       | Integer | Координата x2 в цели.     |
| y2       | Integer | Координата y2 в цели.     |
| x3       | Integer | Координата x3 в цели.     |
| y3       | Integer | Координата y3 в цели.     |
| x4       | Integer | Координата x4 в цели.     |
| y5       | Integer | Координата y4 в цели.     |
| srcImage | Object  | Исходное изображение.     |
| srcLeft  | Integer | X-координата в источнике. |
| srcTop   | Integer | Y-координата в источнике. |
| srcWidth | Integer | Ширина в источнике.       |
| srcHeight| Integer | Высота в источнике.       |
| alpha    | Integer | 0-255                     |
| blend    | Integer | Тип смешивания.           |

### Типы смешивания

| Тип                | Описание                 |
|--------------------|--------------------------|
| Suika.BLEND_ALPHA  | Альфа-смешивание.        |
| Suika.BLEND_ADD    | Аддитивное смешивание.   |
| Suika.BLEND_SUB    | Субтрактивное смешивание.|
| Suika.BLEND_DIM    | Альфа-смешивание RGB 50%.|

### Возврат

Возврат отсутствует.

---

## Suika.drawImageAlpha()

Рисует изображение с альфа-смешиванием.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| dstImage | Object  | Целевое изображение.      |
| dstLeft  | Integer | X-координата в цели.      |
| dstTop   | Integer | Y-координата в цели.      |
| dstWidth | Integer | Ширина отрисовки.         |
| dstHeight| Integer | Высота отрисовки.         |
| srcImage | Object  | Исходное изображение.     |
| srcLeft  | Integer | X-координата в источнике. |
| srcTop   | Integer | Y-координата в источнике. |
| alpha    | Integer | Значение альфы (`0`-`255`). |

### Возврат

Возврат отсутствует.

---

## Suika.drawImageAdd()

Рисует изображение с аддитивным смешиванием.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| dstImage | Object  | Целевое изображение.      |
| dstLeft  | Integer | X-координата в цели.      |
| dstTop   | Integer | Y-координата в цели.      |
| dstWidth | Integer | Ширина отрисовки.         |
| dstHeight| Integer | Высота отрисовки.         |
| srcImage | Object  | Исходное изображение.     |
| srcLeft  | Integer | X-координата в источнике. |
| srcTop   | Integer | Y-координата в источнике. |
| alpha    | Integer | Значение альфы (`0`-`255`). |

### Возврат

Возврат отсутствует.

---

## Suika.drawImageSub()

Рисует изображение с субтрактивным смешиванием.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| dstImage | Object  | Целевое изображение.      |
| dstLeft  | Integer | X-координата в цели.      |
| dstTop   | Integer | Y-координата в цели.      |
| dstWidth | Integer | Ширина отрисовки.         |
| dstHeight| Integer | Высота отрисовки.         |
| srcImage | Object  | Исходное изображение.     |
| srcLeft  | Integer | X-координата в источнике. |
| srcTop   | Integer | Y-координата в источнике. |
| alpha    | Integer | Значение альфы (`0`-`255`). |

### Возврат

Возврат отсутствует.

---

## Suika.makeColor()

Создаёт значение пикселя из компонентов RGBA.

### Параметры (словарь)

| Параметр | Тип     | Описание        |
|----------|---------|-----------------|
| r        | Integer | Красный (0-255). |
| g        | Integer | Зелёный (0-255). |
| b        | Integer | Синий (0-255).   |
| a        | Integer | Альфа (0-255).   |

### Возврат

Значение пикселя.

---

## Suika.fillImageRect()

Заполняет прямоугольную область изображения цветом.

### Параметры (словарь)

| Параметр | Тип    | Описание                                |
|----------|--------|-----------------------------------------|
| image    | Object | Целевое изображение.                    |
| left     | Integer | X-координата.                           |
| top      | Integer | Y-координата.                           |
| width    | Integer | Ширина.                                 |
| height   | Integer | Высота.                                 |
| color    | Integer | Цвет, созданный `Suika.makeColor()`.    |

### Возврат

Возврат отсутствует.

---

## Suika.reloadStageImages()

Перезагружает изображения сцены по конфигурации.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.reloadStagePositions()

Перезагружает позиции сцены по конфигурации.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.getLayerX()

Возвращает текущую позицию указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Целочисленное значение координаты.

---

## Suika.getLayerY()

Возвращает текущую позицию указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Целочисленное значение координаты.

---

## Suika.setLayerPosition()

Устанавливает позицию указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |
| x        | Integer | X-координата.          |
| y        | Integer | Y-координата.          |

### Возврат

Возврат отсутствует.

---

## Suika.getLayerScaleX()

Возвращает коэффициент масштабирования по X для указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Число с плавающей точкой, представляющее масштаб.

---

## Suika.getLayerScaleY()

Возвращает коэффициент масштабирования по Y для указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Число с плавающей точкой, представляющее масштаб.

---

## Suika.setLayerScale()

Устанавливает коэффициент масштабирования указанного слоя.

### Параметры (словарь)

| Параметр | Тип   | Описание            |
|----------|-------|---------------------|
| layer    | Integer | Индекс слоя сцены. |
| scale_x  | Float | Горизонтальный масштаб. |
| scale_y  | Float | Вертикальный масштаб.   |

### Возврат

Возврат отсутствует.

---

## Suika.getLayerRotate()

Возвращает угол поворота указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.setLayerRotate()

Устанавливает угол поворота указанного слоя.

### Параметры (словарь)

| Параметр | Тип   | Описание                    |
|----------|-------|-----------------------------|
| layer    | Integer | Индекс слоя сцены.        |
| rot      | Float | Угол поворота в радианах.   |

### Возврат

Возврат отсутствует.

---

## Suika.getLayerDim()

Возвращает состояние затемнения указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Возвращает булево значение.

---

## Suika.setLayerDim()

Устанавливает состояние затемнения указанного слоя.

### Параметры (словарь) (Set)

| Параметр | Тип     | Описание                    |
|----------|---------|-----------------------------|
| layer    | Integer | Индекс слоя сцены.          |
| dim      | Boolean | Затемнять ли слой.          |

### Возврат

Возврат отсутствует.

---

## Suika.getLayerAlpha()

Возвращает прозрачность указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |

### Возврат

Возвращает целое число.

---

## Suika.setLayerAlpha()

Устанавливает прозрачность указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |
| alpha    | Integer | Значение альфы (0-255). |

### Возврат

Возврат отсутствует.

---

## Suika.setLayerBlend()

Устанавливает режим смешивания для слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание                        |
|----------|---------|---------------------------------|
| layer    | Integer | Индекс слоя сцены.              |
| blend    | Integer | Режим смешивания (Alpha, Add, Sub). |

### Возврат

Возврат отсутствует.

---

## Suika.setLayerFile()

Устанавливает файл, отображаемый на слое.

### Параметры (словарь)

| Параметр  | Тип    | Описание                  |
|-----------|--------|---------------------------|
| layer     | Integer | Индекс слоя сцены.        |
| file_name | String  | Путь к файлу изображения. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.setLayerFrame()

Устанавливает индекс кадра для моргания глаз и синхронизации губ.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс слоя сцены.     |
| frame    | Integer | Индекс кадра.          |

### Возврат

Возврат отсутствует.

---

## Suika.getLayerText()

Возвращает строку, отображаемую на текстовом слое.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| index    | Integer | Индекс текстового слоя. |

### Возврат

Возвращает строку.

---

## Suika.setLayerText()

Устанавливает строку, отображаемую на текстовом слое.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| index    | Integer | Индекс текстового слоя. |
| text     | String  | Устанавливаемый текст.  |

### Возврат

Возврат отсутствует.

---

## Suika.getSysBtnIdleImage()

Возвращает изображение состояния ожидания системной кнопки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает объект изображения.

---

## Suika.getSysBtnHoverImage()

Возвращает изображение состояния наведения системной кнопки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает объект изображения.

---

## Suika.clearStageBasic()

Очищает базовые слои.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает объект изображения.

---

## Suika.clearStage()

Очищает сцену и возвращает её в начальное состояние.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает объект изображения.

---

## Suika.chposToLayer()

Преобразует позицию персонажа в индекс слоя сцены.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возвращает целое число.

---

## Suika.chposToEyeLayer()

Преобразует позицию персонажа в индекс слоя сцены для глаз персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возвращает целое число.

---

## Suika.chposToLipLayer()

Преобразует позицию персонажа в индекс слоя сцены для губ персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возвращает целое число.

---

## Suika.layerToChpos()

Преобразует индекс слоя сцены в позицию персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| layer    | Integer | Индекс слоя.     |

### Возврат

Возвращает целое число.

---

## Suika.renderStage()

Отрисовывает сцену со всеми слоями.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.startFade()

Запускает эффект перехода.

### Параметры (словарь)

| Параметр  | Тип    | Описание                               |
|-----------|--------|----------------------------------------|
| desc      | Array  | Описание перехода.                     |
| method    | String | Метод перехода.                        |
| time      | Float  | Длительность в секундах.               |
| ruleImage | Object | Объект rule image (необязательно).     |

### Возврат

Булево значение.

---

## Suika.getShakeOffset()

Возвращает смещение для команды shake.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Объект, содержащий:
* x
* y

---

## Suika.setShakeOffset()

Устанавливает смещение для команды shake.

### Параметры (словарь)

| Параметр | Тип     | Описание    |
|----------|---------|------------|
| x        | Integer | Смещение по X. |
| y        | Integer | Смещение по Y. |

### Возврат

Возврат отсутствует.

---

## Suika.isFadeRunning()

Проверяет, выполняется ли переход.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.finishFade()

Немедленно завершает эффект перехода.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.setChNameMapping()

Задаёт индекс имени персонажа для позиции персонажа.

### Параметры (словарь)

| Параметр    | Тип     | Описание               |
|-------------|---------|------------------------|
| chpos       | Integer | Позиция персонажа.     |
| chNameIndex | Integer | Индекс имени персонажа. |

### Возврат

Возврат отсутствует.

---

## Suika.getTalkingChpos()

Возвращает позицию персонажа, который сейчас говорит.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает целое число.

---

## Suika.setChTalking()

Устанавливает говорящего персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возврат отсутствует.

---

## Suika.getTalkingChpos()

Возвращает позицию говорящего персонажа.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает целое число.

---

## Suika.updateChDimByTalkingCh()

Автоматически обновляет затемнение персонажей в зависимости от того, кто говорит.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.forceChDim()

Обновляет затемнение персонажей вручную.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |
| dim      | Boolean | Затемнять или нет.    |

### Возврат

Возврат отсутствует.

---

## Suika.getChDim()

Возвращает состояние затемнения.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возвращает булево значение.

---

## Suika.fillNameBox()

Заполняет имябокс изображением имени.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.getNameBoxRect()

Возвращает позицию и размер name box.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Объект.

* x
* y
* w
* h

---

## Suika.showNameBox()

Показывает или скрывает name box.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| show     | Boolean | Показать или скрыть. |

### Возврат

Возврат отсутствует.

---

## Suika.fillMessageBox()

Заполняет message box изображением message box.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.showMessageBox()

Показывает или скрывает message box.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| show     | Boolean | Показывать ли окно. |

### Возврат

Возврат отсутствует.

---

## Suika.getMessageBoxRect()

Возвращает прямоугольник message box.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Объект, содержащий:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClickPosition()

Устанавливает позицию анимации клика.

### Параметры (словарь)

| Параметр | Тип     | Описание       |
|----------|---------|----------------|
| x        | Integer | Позиция по X.  |
| y        | Integer | Позиция по Y.  |

### Возврат

Возврат отсутствует.

---

## Suika.showClick()

Показывает или скрывает анимацию клика.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| show     | Boolean | Показать или скрыть. |

### Возврат

Возврат отсутствует.

---

## Suika.setClickIndex()

Устанавливает индекс кадра анимации клика.

### Параметры (словарь)

| Параметр | Тип     | Описание       |
|----------|---------|----------------|
| index    | Integer | Индекс кадра.  |

### Возврат

Возврат отсутствует.

---

## Suika.getClickRect()

Возвращает прямоугольник анимации клика.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Объект, содержащий:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

Заполняет idle-слой choice box изображением состояния idle.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| index    | Integer | Индекс choice box.    |

### Возврат

Возврат отсутствует.

---

## Suika.fillChooseBoxHoverImage()

Заполняет hover-слой choice box изображением состояния hover.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| index    | Integer | Индекс choice box.    |

### Возврат

Возврат отсутствует.

---

## Suika.showChoosebox()

Показывает или скрывает choice box.

### Параметры (словарь)

| Параметр | Тип     | Описание                  |
|----------|---------|---------------------------|
| index    | Integer | Индекс choice box. (`0`-`7`) |
| showIdle | Boolean | Показывать состояние idle. |
| showHover| Boolean | Показывать состояние hover. |

### Возврат

Возврат отсутствует.

---

## Suika.getChooseBoxRect()

Возвращает прямоугольник choice box.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Объект, содержащий:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

Показывает или скрывает баннер авто-режима.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| show     | Boolean | Показать или скрыть. |

### Возврат

Возврат отсутствует.

---

## Suika.showSkipModeBanner()

Показывает или скрывает баннер режима пропуска.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| show     | Boolean | Показать или скрыть. |

### Возврат

Возврат отсутствует.

---

## Suika.renderImage()

Выполняет прямую отрисовку изображения на экран.

Учтите, что для обычной отрисовки лучше использовать слои сцены.
Этот API полезен для эффектов.

### Параметры (словарь)

| Параметр | Omissible | Тип     | Описание                           |
|----------|-----------|---------|------------------------------------|
| dstLeft  | No        | Integer | X-координата левого верхнего угла назначения. |
| dstTop   | No        | Integer | Y-координата левого верхнего угла назначения. |
| image    | No        | Object  | Изображение.                       |
| srcLeft  | No        | Integer | X-координата левого верхнего угла источника. |
| srcTop   | No        | Integer | Y-координата левого верхнего угла источника. |
| srcWidth | No        | Integer | Ширина источника.                  |
| srcHeight| No        | Integer | Высота источника.                  |
| alpha    | No        | Integer | Значение альфы (`0`-`255`).       |
| blend    | No        | Integer | Тип смешивания.                   |

### Типы смешивания

| Name              | Description         |
|-------------------|---------------------|
| Suika.BLEND_ALPHA | Альфа-смешивание.   |
| Suika.BLEND_ADD   | Аддитивное смешивание. |
| Suika.BLEND_SUB   | Субтрактивное смешивание. |

### Возврат

Возврат отсутствует.

---

## Suika.renderImage3d()

Выполняет прямую отрисовку изображения на экран с 3D-преобразованием.

Учтите, что для обычной отрисовки лучше использовать слои сцены.
Этот API полезен для эффектов.

### Параметры (словарь)

| Параметр | Omissible | Тип     | Описание                           |
|----------|-----------|---------|------------------------------------|
| x1       | No        | Integer | Позиция X вершины назначения 1.    |
| y1       | No        | Integer | Позиция Y вершины назначения 1.    |
| x2       | No        | Integer | Позиция X вершины назначения 2.    |
| y2       | No        | Integer | Позиция Y вершины назначения 2.    |
| x3       | No        | Integer | Позиция X вершины назначения 3.    |
| y3       | No        | Integer | Позиция Y вершины назначения 3.    |
| x4       | No        | Integer | Позиция X вершины назначения 4.    |
| y4       | No        | Integer | Позиция Y вершины назначения 4.    |
| tex      | No        | Object  | Изображение.                       |
| srcLeft  | No        | Integer | X-координата левого верхнего угла источника. |
| srcTop   | No        | Integer | Y-координата левого верхнего угла источника. |
| srcWidth | No        | Integer | Ширина источника.                  |
| srcHeight| No        | Integer | Высота источника.                  |
| alpha    | No        | Integer | Значение альфы (`0`-`255`).       |

### Возврат

Возврат отсутствует.

---

## Suika.startKirakira()

Запускает эффект Kirakira.

Эффект Kirakira - это анимация, которая показывается в той точке экрана, где был щёлкнут курсор мыши.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.renderKirakira()

Отрисовывает эффект Kirakira.

---

## Suika.setMixerInputFile()

Воспроизводит звуковой файл на указанной дорожке микшера.

### Параметры (словарь)

| Параметр | Omissible   | Тип     | Описание                         |
|----------|-------------|---------|----------------------------------|
| track    | No          | String  | Имя дорожки микшера.             |
| file     | No          | String  | Путь к звуковому файлу.          |
| isLooped | Yes(`false`)| Boolean | Зацикливать ли воспроизведение.  |

### Имена дорожек

| Name   | Description                  |
|--------|------------------------------|
| bgm    | Дорожка фоновой музыки.      |
| se     | Дорожка звуковых эффектов.   |
| voice  | Дорожка голоса персонажа.    |
| sys    | Дорожка системных звуков.    |

### Возврат

Булево значение, показывающее, удалось ли открыть файл.

---

## Suika.setMixerVolume()

Устанавливает громкость указанной дорожки микшера.

### Параметры (словарь)

| Параметр | Тип    | Описание                              |
|----------|--------|---------------------------------------|
| track    | String | Имя дорожки микшера.                  |
| vol      | Float  | Уровень громкости (0.0 to 1.0).       |
| span     | Float  | Длительность плавного изменения в секундах. |

### Имена дорожек

| Name   | Description                  |
|--------|------------------------------|
| bgm    | Дорожка фоновой музыки.      |
| se     | Дорожка звуковых эффектов.   |
| voice  | Дорожка голоса персонажа.    |
| sys    | Дорожка системных звуков.    |

### Возврат

Возврат отсутствует.

---

## Suika.getMixerVolume()

Возвращает громкость указанной дорожки микшера.

### Параметры (словарь)

| Параметр | Тип    | Описание                              |
|----------|--------|---------------------------------------|
| track    | String | Имя дорожки микшера.                  |
| volume   | Float  | Уровень громкости (0.0 to 1.0).       |
| span     | Float  | Длительность плавного изменения в секундах. |

### Имена дорожек

| Name   | Description                  |
|--------|------------------------------|
| bgm    | Дорожка фоновой музыки.      |
| se     | Дорожка звуковых эффектов.   |
| voice  | Дорожка голоса персонажа.    |
| sys    | Дорожка системных звуков.    |

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.setMasterVolume()

Устанавливает общую громкость для всех дорожек.

### Параметры (словарь)

| Параметр | Тип   | Описание                          |
|----------|-------|-----------------------------------|
| volume   | Float | Общая громкость (0.0 to 1.0).     |

### Возврат

Возврат отсутствует.

---

## Suika.getMasterVolume()

Возвращает общую громкость, влияющую на все дорожки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.setMixerGlobalVolume()

Устанавливает глобальную громкость для дорожки, часто используется для настроек конфигурации.

### Параметры (словарь)

| Параметр | Тип    | Описание                   |
|----------|--------|----------------------------|
| track    | String | Имя дорожки микшера.       |
| vol      | Float  | Уровень глобальной громкости. |

### Имена дорожек

| Name   | Description                  |
|--------|------------------------------|
| bgm    | Дорожка фоновой музыки.      |
| se     | Дорожка звуковых эффектов.   |
| voice  | Дорожка голоса персонажа.    |
| sys    | Дорожка системных звуков.    |

### Возврат

Возврат отсутствует.

---

## Suika.getMixerGlobalVolume()

Возвращает глобальную громкость для дорожки, часто используемую в настройках конфигурации.

### Параметры (словарь)

| Параметр | Тип    | Описание                   |
|----------|--------|----------------------------|
| track    | String | Имя дорожки микшера.       |

### Имена дорожек

| Name   | Description                  |
|--------|------------------------------|
| bgm    | Дорожка фоновой музыки.      |
| se     | Дорожка звуковых эффектов.   |
| voice  | Дорожка голоса персонажа.    |
| sys    | Дорожка системных звуков.    |

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.setCharacterVolume()

Устанавливает громкость голоса конкретного персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| index    | Integer | Индекс имени персонажа. |
| vol      | Float   | Уровень громкости.     |

### Возврат

Возврат отсутствует.

---

## Suika.getCharacterVolume()

Возвращает громкость голоса конкретного персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| ch_index | Integer | Индекс имени персонажа. |

### Возврат

Возвращает число с плавающей точкой.

---

## Suika.isMixerSoundFinished()

Проверяет, завершилось ли воспроизведение на указанной дорожке.

### Параметры (словарь)

| Параметр | Тип     | Описание                |
|----------|---------|-------------------------|
| track    | Integer | Индекс дорожки микшера. |

### Возврат

Булево значение.

---

## Suika.getTrackFileName()

Возвращает имя файла звука, который сейчас загружен на дорожку.

### Параметры (словарь)

| Параметр | Тип     | Описание                |
|----------|---------|-------------------------|
| track    | Integer | Индекс дорожки микшера. |

### Возврат

Строка, представляющая путь к файлу.

---

## Suika.applyCharacterVolume()

Применяет индивидуальную громкость персонажа к дорожке VOICE.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| ch       | Integer | Индекс имени персонажа. |

### Возврат

Возврат отсутствует.

---

## Suika.enableSysBtn()

Управляет системной кнопкой.

### Параметры (словарь)

| Параметр  | Тип     | Описание                     |
|-----------|---------|------------------------------|
| isEnabled | Boolean | Включать ли системную кнопку. |

### Возврат

Возврат отсутствует.

---

## Suika.isSysBtnEnabled()

Проверяет, включена ли системная кнопка.

### Параметры

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.updateSysBtnState()

Обновляет отслеживание мыши для системной кнопки.

### Параметры

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isSysBtnPointed()

Проверяет, наведён ли курсор на системную кнопку.

### Параметры

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.isSysBtnClicked()

Проверяет, нажата ли системная кнопка.

### Параметры

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.drawTextOnLayer()

Рисует текст на указанном слое.

### Параметры (словарь)

| Параметр    | Тип     | Описание                     |
|-------------|---------|------------------------------|
| layer       | Integer | Индекс целевого слоя сцены.  |
| fontType    | Integer | Индекс выбора шрифта.        |
| fontSize    | Integer | Размер шрифта.               |
| color       | Integer | Цвет.                        |
| outlineWidth| Integer | Толщина контура.             |
| outlineColor| Integer | Цвет контура.                |
| lineMargin  | Integer | Отступ между строками.       |
| charMargin  | Integer | Отступ между символами.      |
| x           | Integer | Позиция X рамки.             |
| y           | Integer | Позиция Y рамки.             |
| width       | Integer | Ширина рамки.                |
| height      | Integer | Высота рамки.                |
| text        | String  | Текст.                       |

### Возврат

Возврат отсутствует.

---

## Suika.getStringWidth()

Возвращает общую ширину UTF-8 строки.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| fontType | Integer | Индекс выбора шрифта.  |
| fontSize | Integer | Размер шрифта.         |
| text     | String  | Текст.                 |

### Возврат

Целочисленная ширина в пикселях.

---

## Suika.getStringHeight()

Возвращает общую высоту UTF-8 строки.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| fontType | Integer | Индекс выбора шрифта.  |
| fontSize | Integer | Размер шрифта.         |
| text     | String  | Текст.                 |

### Возврат

Целочисленная высота в пикселях.

---

## Suika.drawGlyph()

Рисует один глиф на изображении.

### Параметры (словарь)

| Параметр      | Тип     | Описание                           |
|---------------|---------|------------------------------------|
| img           | Object  | Целевое изображение.               |
| font_type     | Integer | Индекс выбора шрифта.              |
| font_size     | Integer | Размер шрифта при отрисовке.       |
| base_font_size| Integer | Базовый размер шрифта для метрик.  |
| outline_size  | Integer | Толщина контура.                   |
| x             | Integer | X-координата.                      |
| y             | Integer | Y-координата.                      |
| color         | Pixel   | Основной цвет текста.              |
| outline_color | Pixel   | Цвет контура.                      |
| codepoint     | Integer | Кодовая точка UTF-32.              |
| is_dim        | Boolean | Применять ли затемнение.           |

### Возврат

Булево значение, показывающее успех.

---

## Suika.createDrawMsg()

Создаёт контекст сложной отрисовки сообщения для высокоуровневого текстового рендеринга.

### Параметры (словарь)

| Параметр      | Тип     | Описание                    |
|---------------|---------|-----------------------------|
| image         | Integer | Целевое изображение.        |
| text          | String  | Сообщение для отрисовки.    |
| fontType      | Integer | Выбор шрифта.               |
| fontSize      | Integer | Размер шрифта.              |
| baseFontSize  | Integer | Базовый размер шрифта.      |
| rubySize      | Integer | Размер ruby.                |
| outlineSize   | Integer | Толщина контура.            |
| penX          | Integer | Позиция пера по X.          |
| penY          | Integer | Позиция пера по Y.          |
| areaWidth     | Integer | Ширина области отрисовки.   |
| areaHeight    | Integer | Высота области отрисовки.   |
| leftMargin    | Integer | Левый отступ.               |
| rightMargin   | Integer | Правый отступ.              |
| topMargin     | Integer | Верхний отступ.             |
| bottomMargin  | Integer | Нижний отступ.              |
| lineMargin    | Integer | Отступ между строками.      |
| charMargin    | Integer | Отступ между символами.     |
| color         | Integer | Цвет.                       |
| outlineColor  | Integer | Цвет контура.               |
| bgColor       | Integer | Цвет фона.                  |
| fillBg        | Boolean | Заполнять фон?              |
| dim           | Boolean | Затемнять?                  |
| ignoreLF      | Boolean | Игнорировать LF?            |
| ignoreFont    | Boolean | Игнорировать смену шрифта?  |
| ignoreOutline | Boolean | Игнорировать контур?        |
| ignoreColor   | Boolean | Игнорировать цвет?          |
| ignoreSize    | Boolean | Игнорировать размер?        |
| ignorePosition| Boolean | Игнорировать позицию курсора? |
| ignoreRuby    | Boolean | Игнорировать ruby?          |
| ignoreWait    | Boolean | Игнорировать встроенную паузу? |
| inlineWaitHook| Function | Хук встроенной паузы.       |
| tategaki      | Boolean | Использовать tategaki?      |

### Возврат

Объект контекста отрисовки сообщения.

---

## Suika.destroyDrawMsg()

Уничтожает контекст отрисовки сообщения.

### Параметры (словарь)

| Параметр | Тип    | Описание                 |
|----------|--------|--------------------------|
| context  | Object | Контекст отрисовки сообщения. |

### Возврат

Возврат отсутствует.

---

## Suika.countDrawMsgChars()

Считает оставшиеся символы без учёта escape-последовательностей.

### Параметры (словарь)

| Параметр | Тип    | Описание                 |
|----------|--------|--------------------------|
| context  | Object | Контекст отрисовки сообщения. |

### Возврат

Возвращает целое число.

---

## Suika.drawMessage()

Рисует символы сообщения до значения `maxChars`.

### Параметры (словарь)

| Параметр | Тип     | Описание                 |
|----------|---------|--------------------------|
| context  | Object  | Контекст отрисовки сообщения. |
| maxChars | Integer | Максимальное число символов. |

### Возврат

Возвращает целое число, показывающее количество символов, отрисованных за вызов.

---

## Suika.getDrawMsgPenPosition()

Возвращает текущую позицию пера из контекста отрисовки.

### Параметры (словарь)

| Параметр | Тип    | Описание            |
|----------|--------|---------------------|
| context  | Object | Контекст отрисовки. |

### Возврат

Объект, содержащий `x` и `y`.

---

## Suika.isEscapeSequenceChar()

Проверяет, является ли символ частью escape-последовательности.

### Параметры (словарь)

| Параметр | Type   | Description                                |
|----------|--------|--------------------------------------------|
| c        | String | Символ для проверки.                       |

### Возврат

Булево значение.

---

## Suika.moveToTagFile()

Загружает новый файл тегов и переводит точку выполнения в его начало.

### Параметры (словарь)

| Параметр | Тип    | Описание                          |
|----------|--------|-----------------------------------|
| file     | String | Путь к файлу `.novel` или файлу сценария. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.getTagCount()

Возвращает общее количество тегов в текущем файле сценария.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее количество тегов.

---

## Suika.moveToTagIndex()

Перемещает указатель выполнения к указанному индексу тега.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| index    | Integer | Целевой индекс тега. |

### Возврат

Булево значение.

---

## Suika.moveToNextTag()

Перемещает указатель выполнения к следующему тегу.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.moveToLabelTag()

Переходит к указанной метке.

### Параметры (словарь)

| Параметр | Тип    | Описание           |
|----------|--------|--------------------|
| name     | String | Имя целевой метки. |

### Возврат

Булево значение.

---

## Suika.moveToMacroTag()

Переходит к указанному макросу по имени.

### Параметры (словарь)

| Параметр | Тип    | Описание            |
|----------|--------|---------------------|
| name     | String | Имя целевого макроса. |

### Возврат

Булево значение.

---

## Suika.moveToElseTag()

Переходит к соответствующему тегу `else`/`elseif`/`endif`.

### Параметры (словарь)

| Параметр | Тип    | Описание            |
|----------|--------|---------------------|
| name     | String | Имя целевого макроса. |

### Возврат

Булево значение.

---

## Suika.moveToEndIfTag()

Переходит к соответствующему тегу `endif`.

### Параметры (словарь)

| Параметр | Тип    | Описание             |
|----------|--------|----------------------|
| name     | String | Имя целевого макротега. |

### Возврат

Булево значение.

---

## Suika.moveToEndMacroTag()

Переходит к соответствующему тегу `endmacro`.

### Параметры (словарь)

| Параметр | Тип    | Описание             |
|----------|--------|----------------------|
| name     | String | Имя целевого макротега. |

### Возврат

Булево значение.

---

## Suika.getTagFileName()

Возвращает имя файла сценария для текущего тега.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Строка, представляющая имя файла.

---

## Suika.getTagName()

Возвращает имя текущего тега.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Строка, представляющая имя тега.

---

## Suika.getTagPropertyCount()

Возвращает количество свойств текущего тега.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Строка, представляющая имя или значение.

---

## Suika.getTagPropertyName()

Перебирает и возвращает свойства (аргументы) текущего тега.

### Параметры (словарь)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| index    | Integer | Индекс свойства. |

### Возврат

Строка, представляющая имя.

---

## Suika.getTagPropertyValue()

Перебирает и возвращает свойства (аргументы) текущего тега.

### Параметры (словарь) (для PropertyName/Value)

| Параметр | Тип     | Описание         |
|----------|---------|------------------|
| index    | Integer | Индекс свойства. |

### Возврат

Строка, представляющая значение.

---

## Suika.getTagArgBool()

Возвращает конкретный аргумент текущего тега с поддержкой значений по умолчанию и необязательности.

### Параметры (словарь)

| Параметр  | Тип     | Описание                          |
|-----------|---------|-----------------------------------|
| name      | String  | Имя аргумента.                    |
| omissible | Boolean | Является ли аргумент необязательным. |
| defVal    | Boolean | Значение по умолчанию, если отсутствует. |

### Возврат

Значение аргумента в запрошенном типе.

---

## Suika.getTagArgInt()

Возвращает конкретный аргумент текущего тега с поддержкой значений по умолчанию и необязательности.

### Параметры (словарь)

| Параметр  | Тип     | Описание                          |
|-----------|---------|-----------------------------------|
| name      | String  | Имя аргумента.                    |
| omissible | Boolean | Является ли аргумент необязательным. |
| defVal    | Integer | Значение по умолчанию, если отсутствует. |

### Возврат

Значение аргумента в запрошенном типе.

---

## Suika.getTagArgFloat()

Возвращает конкретный аргумент текущего тега с поддержкой значений по умолчанию и необязательности.

### Параметры (словарь)

| Параметр  | Тип   | Описание                          |
|-----------|-------|-----------------------------------|
| name      | String | Имя аргумента.                    |
| omissible | Boolean | Является ли аргумент необязательным. |
| defVal    | Float | Значение по умолчанию, если отсутствует. |

### Возврат

Значение аргумента в запрошенном типе.

---

## Suika.getTagArgString()

Возвращает конкретный аргумент текущего тега с поддержкой значений по умолчанию и необязательности.

### Параметры (словарь)

| Параметр  | Тип    | Описание                          |
|-----------|--------|-----------------------------------|
| name      | String | Имя аргумента.                    |
| omissible | Boolean | Является ли аргумент необязательным. |
| defVal    | String | Значение по умолчанию, если отсутствует. |

### Возврат

Значение аргумента в запрошенном типе.

---

## Suika.evaluateTag()

Вычисляет значения свойств текущего тега, чтобы раскрыть встроенные переменные. (`${varname}` form)

Вызов этого API обновляет кэш значений свойств.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.pushTagStackIf()

Управляет внутренним стеком для условных блоков `[if]`.

Этот API помечает позицию блока `if` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.popTagStackIf()

Управляет внутренним стеком для условных блоков `if`.

Этот API помечает конец блока `if` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.pushTagStackWhile()

Управляет внутренним стеком для циклов (`while`).

Этот API помечает блок `while` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.popTagStackWhile()

Управляет внутренним стеком для циклов (`while`).

Этот API помечает конец блока `while` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.pushTagStackFor()

Управляет внутренним стеком для циклов (`for`).

Этот API помечает блок `for` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.popTagStackFor()

Управляет внутренним стеком для циклов (`for`).

Этот API помечает конец блока `for` для обработки вложенных блоков.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.loadAnimeFromFile()

Загружает определение анимации из файла и регистрирует его.

### Параметры (словарь)

| Параметр | Тип    | Описание                       |
|----------|--------|--------------------------------|
| file     | String | Путь к файлу анимации.         |
| reg_name | String | Имя регистрации анимации.      |

### Возврат

Массив булевых значений, показывающий, загружен ли каждый слой.

---

## Suika.newAnimeSequence()

Начинает описание новой последовательности анимации для указанного слоя.
Этот API используется для анимаций, созданных вручную.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс целевого слоя сцены. |

### Возврат

Булево значение, показывающее успех.

---

## Suika.addAnimeSequencePropertyF()

Добавляет свойство с плавающей точкой, например позицию или alpha, в текущую последовательность анимации.
Этот API используется для анимаций, созданных вручную.

### Параметры (словарь)

| Параметр | Тип   | Описание                         |
|----------|-------|----------------------------------|
| key      | String | Ключ свойства (например, "x", "y", "a"). |
| val      | Float | Целевое значение.                |

### Возврат

Булево значение.

---

## Suika.addAnimeSequencePropertyI()

Добавляет целочисленное свойство, например позицию или alpha, в текущую последовательность анимации.
Этот API используется для анимаций, созданных вручную.

### Параметры (словарь)

| Параметр | Тип     | Описание                         |
|----------|---------|----------------------------------|
| key      | String  | Ключ свойства (например, "x", "y", "a"). |
| val      | Integer | Целевое значение.                |

### Возврат

Булево значение.

---

## Suika.startLayerAnime()

Запускает зарегистрированную последовательность анимации для указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс целевого слоя сцены. |

### Возврат

Булево значение.

---

## Suika.isAnimeRunning()

Проверяет общее состояние анимации.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isAnimeFinishedForLayer()

Проверяет, закончилась ли анимация указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс целевого слоя сцены. |

### Возврат

Булево значение.

---

## Suika.updateAnimeFrame()

Обновляет состояние кадра анимации. Обычно вызывается один раз за кадр.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.loadEyeImageIfExists()

Управляет изображением и анимацией моргания глаз для позиции персонажа.

### Параметры (словарь)

| Параметр | Тип    | Описание                           |
|----------|--------|------------------------------------|
| chpos    | Integer | Позиция персонажа (Left, Center и т.д.). |
| file     | String | Путь к файлу изображения глаз.     |

### Возврат

Булево значение.

---

## Suika.reloadEyeAnime()

Перезапускает анимацию моргания глаз для позиции персонажа.

### Параметры (словарь)

| Параметр | Тип     | Описание                           |
|----------|---------|------------------------------------|
| chpos    | Integer | Позиция персонажа (Left, Center и т.д.). |

### Возврат

Булево значение.

---

## Suika.runLipAnime()

Запускает анимацию синхронизации губ по содержимому сообщения для персонажа.

### Параметры (словарь)

| Параметр | Тип    | Описание                     |
|----------|--------|------------------------------|
| chpos    | Integer | Позиция персонажа.           |
| text     | String | Текст сообщения для синхронизации. |

### Возврат

Возврат отсутствует.

---

## Suika.stopLipAnime()

Останавливает анимацию синхронизации губ.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| chpos    | Integer | Позиция персонажа.    |

### Возврат

Возврат отсутствует.

---

## Suika.clearLayerAnimeSequence()

Очищает последовательности анимации для указанного слоя.

### Параметры (словарь)

| Параметр | Тип     | Описание               |
|----------|---------|------------------------|
| layer    | Integer | Индекс целевого слоя сцены. |

### Возврат

Возврат отсутствует.

---

## Suika.clearAllAnimeSequence()

Очищает последовательности анимации для всех слоёв.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.setVariableInt()

Устанавливает значение локальной или глобальной переменной.

### Параметры (словарь)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| name     | String  | Имя переменной.       |
| value    | Integer | Устанавливаемое значение. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.setVariableFloat()

Устанавливает значение локальной или глобальной переменной.

### Параметры (словарь)

| Параметр | Тип   | Описание              |
|----------|-------|-----------------------|
| name     | String | Имя переменной.       |
| value    | Float  | Устанавливаемое значение. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.setVariableString()

Устанавливает значение локальной или глобальной переменной.

### Параметры (словарь)

| Параметр | Тип    | Описание              |
|----------|--------|-----------------------|
| name     | String | Имя переменной.       |
| value    | String | Устанавливаемое значение. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.getVariableInt()

Возвращает текущее значение переменной.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| name     | String | Имя переменной.         |

### Возврат

Значение переменной в виде целого числа.

---

## Suika.getVariableFloat()

Возвращает текущее значение переменной.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| name     | String | Имя переменной.         |

### Возврат

Значение переменной в виде числа с плавающей точкой.

---

## Suika.getVariableString()

Возвращает текущее значение переменной.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| name     | String | Имя переменной.         |

### Возврат

Значение переменной в виде строки.

---

## Suika.unsetVariable()

Удаляет указанную переменную.

### Параметры (словарь)

| Параметр | Тип    | Описание                     |
|----------|--------|------------------------------|
| name     | String | Имя переменной для удаления. |

### Возврат

Возврат отсутствует.

---

## Suika.unsetLocalVariables()

Удаляет все локальные переменные.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.makeVariableGlobal()

Делает переменную глобальной, то есть сохраняемой между сохранениями.

### Параметры (словарь)

| Параметр  | Тип     | Описание                 |
|-----------|---------|--------------------------|
| name      | String  | Имя переменной.          |
| is_global | Boolean | Делать ли её глобальной. |

### Возврат

Булево значение.

---

## Suika.isGlobalVariable()

Проверяет глобальный статус переменной.

### Параметры (словарь)

| Параметр | Тип    | Описание                |
|----------|--------|-------------------------|
| name     | String | Имя переменной.         |

### Возврат

Булево значение.

---

## Suika.getVariableCount()

Возвращает количество переменных.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее количество.

---

## Suika.getVariableName()

Перебирает зарегистрированные переменные.

### Параметры (словарь) (для getVariableName)

| Параметр | Тип     | Описание              |
|----------|---------|-----------------------|
| index    | Integer | Индекс переменной.    |

### Возврат

Строка с именем.

---

## Suika.checkVariableExists()

Проверяет, существует ли переменная с указанным именем.

### Параметры (словарь)

| Параметр | Тип    | Описание         |
|----------|--------|------------------|
| name     | String | Имя для проверки. |

### Возврат

Булево значение.

---

## Suika.executeSaveGlobal()

Выполняет глобальное сохранение.
Глобальные данные обычно включают постоянные настройки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.executeLoadGlobal()

Выполняет глобальную загрузку.
Глобальные данные обычно включают постоянные настройки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.executeSaveLocal()

Сохраняет прогресс игры в указанный слот.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.executeLoadLocal()

Загружает прогресс игры из указанного слота.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.checkSaveExists()

Проверяет, существуют ли данные сохранения для указанного индекса слота.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Булево значение.

---

## Suika.deleteLocalSave()

Удаляет локальный слот сохранения.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Возврат отсутствует.

---

## Suika.deleteGlobalSave()

Удаляет все глобальные данные сохранения.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.checkRightAfterLoad()

Проверяет, следует ли текущий кадр сразу после успешной операции загрузки.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.getSaveTimestamp()

Возвращает временную метку Unix, когда были созданы данные сохранения.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Целое число (временная метка).

---

## Suika.getLatestSaveIndex()

Возвращает индекс наиболее недавно обновлённого слота сохранения.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее индекс слота.

---

## Suika.getSaveChapterName()

Возвращает название главы, сохранённое в слоте сохранения.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Строка, представляющая название главы.

---

## Suika.getSaveLastMessage()

Возвращает последнее отображённое сообщение, сохранённое в слоте.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Строка, представляющая сообщение.

---

## Suika.getSaveThumbnail()

Возвращает миниатюру, связанную со слотом сохранения.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| index    | Integer | Индекс слота сохранения. |

### Возврат

Объект изображения.

---

## Suika.clearHistory()

Очищает все сообщения из истории (backlog).

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.addHistory()

Добавляет новую запись в историю.

### Параметры (словарь)

| Параметр         | Тип     | Описание                   |
|------------------|---------|----------------------------|
| name             | String  | Имя персонажа.             |
| msg              | String  | Текст сообщения.           |
| voice            | String  | Путь к файлу голоса.       |
| bodyColor        | Integer | Цвет тела.                 |
| bodyOutlineColor | Integer | Цвет контура тела.         |
| nameColor        | Integer | Цвет имени.                |
| nameOutlineColor | Integer | Цвет контура имени.        |

### Возврат

Булево значение, показывающее успех.

---

## Suika.getHistoryCount()

Возвращает общее количество записей в истории.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число, представляющее количество записей в истории.

---

## Suika.getHistoryName()

Возвращает имя по указанному индексу истории.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| index    | Integer | Индекс в истории.   |

### Возврат

Строковое значение.

---

## Suika.getHistoryMessage()

Возвращает сообщение по указанному индексу истории.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| index    | Integer | Индекс в истории.   |

### Возврат

Строковое значение.

---

## Suika.getHistoryVoice()

Возвращает путь к голосу по указанному индексу истории.

### Параметры (словарь)

| Параметр | Тип     | Описание            |
|----------|---------|---------------------|
| index    | Integer | Индекс в истории.   |

### Возврат

Строковое значение.

---

## Suika.loadSeen()

Загружает флаги просмотренного (прочитанного) для текущего файла сценария.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее успех.

---

## Suika.saveSeen()

Сохраняет флаги просмотренного (прочитанного) для текущего файла сценария.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение, показывающее успех.

---

## Suika.getSeenFlags()

Возвращает статус просмотра для текущего тега.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает целое число.

Для тега `[text]` `0` означает непрочитано, а `1` означает прочитано.

Для тега `[choose]` каждый бит показывает, был ли вариант выбран ранее.

---

## Suika.setSeenFlags()

Устанавливает статус просмотра для текущего тега.

### Параметры (словарь)

| Параметр | Тип     | Описание             |
|----------|---------|----------------------|
| flag     | Integer | Флаг статуса просмотра. |

### Возврат

Возврат отсутствует.

---

## Suika.loadGUIFile()

Загружает файл описания GUI и подготавливает его к выполнению.

### Параметры (словарь)

| Параметр | Тип     | Описание                              |
|----------|---------|---------------------------------------|
| file     | String  | Путь к файлу `.gui`.                  |
| sys      | Boolean | Является ли это системным GUI (Save/Load и т.д.). |

### Что такое System GUI

System GUI обычно вызывается, когда выполняется `[text]` или `[choose]`,
и управление возвращается к прерванному тегу.

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.startGUI()

Запускает загруженный GUI.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.stopGUI()

Останавливает текущий работающий GUI.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isGUIRunning()

Проверяет, активен ли GUI в данный момент.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.isGUIFinished()

Проверяет, завершил ли GUI свою работу.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.getGUIResultLabel()

Возвращает метку кнопки, выбранной для завершения GUI.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Строка, представляющая метку результата.

---

## Suika.isGUIResultTitle()

Проверяет, был ли GUI закрыт действием "back to title".

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.checkIfSavedInGUI()

Проверяет, была ли выполнена операция сохранения, пока GUI был активен.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.checkIfLoadedInGUI()

Проверяет, была ли выполнена операция загрузки, пока GUI был активен.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.checkRightAfterSysGUI()

Проверяет, следует ли текущий кадр сразу после возврата из системного GUI.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.getMillisec()

Возвращает прошедшее время с момента отсчёта в миллисекундах.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Целое число в миллисекундах.

---

## Suika.checkFileExists()

Проверяет, существует ли файл.

### Параметры (словарь)

| Параметр | Тип    | Описание             |
|----------|--------|----------------------|
| file     | String | Путь к файлу.        |

### Возврат

Возвращает булево значение.

---

## Suika.readFileContent()

Читает содержимое файла целиком.

### Параметры (словарь) (для readFileContent)

| Параметр | Тип    | Описание             |
|----------|--------|----------------------|
| file     | String | Путь к файлу.        |

### Возврат

Возвращает строку.

---

## Suika.writeSaveData()

Непосредственно записывает сырые данные сохранения, связанные с ключом.

### Параметры (словарь)

| Параметр | Тип    | Описание               |
|----------|--------|------------------------|
| key      | String | Уникальный ключ данных. |
| data     | String | Данные для записи/чтения. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.readSaveData()

Непосредственно читает сырые данные сохранения, связанные с ключом.

### Параметры (словарь)

| Параметр | Тип    | Описание               |
|----------|--------|------------------------|
| key      | String | Уникальный ключ данных. |

### Возврат

Булево значение, показывающее успех или неудачу.

---

## Suika.playVideo()

Управляет воспроизведением видео.

### Параметры (словарь) (для playVideo)

| Параметр     | Тип     | Описание                         |
|--------------|---------|----------------------------------|
| file         | String  | Путь к видеофайлу.               |
| is_skippable | Boolean | Можно ли пропустить видео.       |

### Возврат

`Play` возвращает Boolean; `IsPlaying` возвращает Boolean.

---

## Suika.stopVideo()

Останавливает воспроизведение видео.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.isVideoPlaying()

Проверяет, воспроизводится ли видео.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возвращает булево значение.

---

## Suika.isFullScreenSupported()

Проверяет поддержку полноэкранного режима.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Булево значение.

---

## Suika.enterFullScreenMode()

Переходит в полноэкранный режим.

### Параметры (словарь)

Параметры отсутствуют.

### Возврат

Возврат отсутствует.

---

## Suika.speakText()

Выполняет синтез речи (TTS) для указанного сообщения.

### Параметры (словарь)

| Параметр | Тип    | Описание            |
|----------|--------|---------------------|
| msg      | String | Текст для озвучивания. |

### Возврат

Возврат отсутствует.

