Ray High Level API Reference
============================

The `High Level API (Engine.*)` is designed for Visual Novel creation.

Every API function takes one parameter.
The parameter must be a dictionary and arguments must be stored as key-and-value pairs.

## Index

* Fundamental
    * [Suika.loadPlugin()](#suikaloadplugin)
* Config
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
* Input
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
* Game
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
* Image
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
    * [Suika.getImageWidth()](#suikagetimagewidth)
    * [Suika.getImageHeight()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImage()](#suikadrawimage)
    * [Suika.drawImage3D()](#suikadrawimage3d)
    * [Suika.makeColor()](#suikamakecolor)
    * [Suika.fillImageRect()](#suikafillimagerect)
* Stage
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
* Mixer
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
* Text
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
* Tag
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
* Anime
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
* Variable
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
* Save
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
* History
    * [Suika.clearHistory()](#suikaclearhistory)
    * [Suika.addHistory()](#suikaaddhistory)
    * [Suika.getHistoryCount()](#suikagethistorycount)
    * [Suika.getHistoryName()](#suikagethistoryname)
    * [Suika.getHistoryMessage()](#suikagethistorymessage)
    * [Suika.getHistoryVoice()](#suikagethistoryvoice)
* Seen
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

Loads a plugin.

Only this API takes a non-dictionary argument.

### Parameters (Direct)

| Parameter | Type   | Description     |
|-----------|--------|-----------------|
| name      | String | Plugin name.    |

### Return

No return.

---

## Suika.setConfig()

Set a config.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key of the config.                         |
| value     | String | Value of the config.                       |

### Return

No return.

---

## Suika.getConfigCount()

Get the number of the config keys.

### Parameters (Dictionary)

No parameters.

### Return

Integer that represents a number of config keys.

---

## Suika.getConfigKey()

Get a config key's index.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| index     | Integer | Index of a config.                         |

### Return

String that represents a key of the config at the specified index.

---

## Suika.isGlobalSaveConfig()

Check if a config key is stored to global save data.

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Boolean that represents whether the config is global-saved or not.

---

## Suika.isLocalSaveConfig()

Check if a config key is stored to local save data.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Boolean that represents whether the config is local-saved or not.

---

## Suika.getConfigType()

Get a config value type. ("s", "b", "i", "f")

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

One of the following strings.

| Value      | Meaning                  |
|------------|--------------------------|
| "s"        | Config is string.        |
| "b"        | Config is boolean.       |
| "i"        | Config is integer.       |
| "f"        | Config is float.         |

---

## Suika.getStringConfig()

Get a string config value.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

String value of the config.

---

## Suika.getBoolConfig()

Get a boolean config value.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Boolean value of the config.

---

## Suika.getIntConfig()

Get an integer config value.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Integer value of the config.

---

## Suika.getFloatConfig()

Get a float config value.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Float value of the config.

---

## Suika.getConfigAsString()

Get a config value as a string.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Stringified value of the config.

---

## Suika.compareLocale()

Check if the specified locale is same as the current locale.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| locale    | String | Locale name.                               |


### Return

Bolean that represents whether the specified locale is matched to the
current one.

---

## Suika.getMousePosX()

Get the mouse X position.

### Parameters (Dictionary)

No parameters.

### Return

Integer that represents the current mouse X coordinate.

---

## Suika.getMousePosY()

Get the mouse Y position.

### Parameters (Dictionary)

No parameters.

### Return

Integer that represents the current mouse Y coordinate.

---

## Suika.isMouseLeftPressed()

Check if mouse left button is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents whether the left button is currently held down.

---

## Suika.isMouseRightPressed()

Check if mouse right button is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents whether the right button is currently held down.

---

## Suika.isMouseLeftClicked()

Check if mouse left button is pressed then released.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents whether a left-click occurred in the current frame.

---

## Suika.isMouseRightClicked()

Check if mouse right button is pressed then released.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents whether a right-click occurred in the current frame.

---

## Suika.isMouseDragging()

Check if mouse is dragging.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents whether the mouse is being moved while a button is pressed.

---

## Suika.isReturnKeyPressed()

Check if return key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isSpaceKeyPressed()

Check if space key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isEscapeKeyPressed()

Check if escape key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isUpKeyPressed()

Check if up key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isDownKeyPressed()

Check if down key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isLeftKeyPressed()

Check if left key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isRightKeyPressed()

Check if right key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isPageUpKeyPressed()

Check if pageup key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isPageDownKeyPressed()

Check if pagedown key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isControlKeyPressed()

Check if control key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isSKeyPressed()

Check if S key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isLKeyPressed()

Check if L key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isHKeyPressed()

Check if H key is pressed.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isTouchCanceled()

Check if touch is canceled.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isSwiped()

Check if swiped.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.clearInputState()

Clear input states to avoid further input processing in the current frame.

### Parameters (Dictionary)

No parameters.

### Return

No return.


---

## Suika.startCommandRepetition()

Start a multiple-frame command execution.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.stopCommandRepetition()

Stop a multiple-frame command execution.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isInCommandRepetition()

Check whether we are in a multiple-frame command execution or not.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.setMessageActive()

Set the message showing state to active.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.clearMessageActive()

Reset the message showing state.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isMessageActive()

Check whether the message showing state is set or not.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.startAutoMode()

Start the auto-mode.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.stopAutoMode()

Stop the auto-mode.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isAutoMode()

Check whether we are in the auto-mode or not.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.startSkipMode()

Start the skip-mode.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.stopSkipMode()

Stop the skip-mode.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isSkipMode()

Check whether we are in the skip-mode or not.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.setSaveLoad()

Set the save/load enable setting.

### Parameters (Dictionary)

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| enable    | Boolean | Whether to enable save and load.  |

### Return

No return.

---

## Suika.isSaveLoadEnabled()

Get the save/load enable setting.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.setNonInterruptible()

Set the non-interruptible mode setting.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| enable    | Boolean | Non-interruptible mode.    |

### Return

No return.

---

## Suika.isNonInterruptible()

Get the non-interruptible mode setting.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.setPenPosition()

Set the pen position for text drawing.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| x         | Integer | X coordinate.         |
| y         | Integer | Y coordinate.         |

### Return

No return.

---

## Suika.getPenPositionX()

Get the pen X position.

### Parameters (Dictionary)

No parameters.

### Return

Integer value.

---

## Suika.getPenPositionY()

Get the pen Y position.

### Parameters (Dictionary)

No parameters.

### Return

Integer value.

---

## Suika.pushForCall()

Push the return point to the call stack.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| file      | String  | Script file name.     |
| index     | Integer | Command index.        |

### Return

Boolean that represents success or failure.

---

## Suika.popForReturn()

Pop the return point from the call stack.

### Parameters (Dictionary)

No parameters.

### Return

Returns a dictionary that contains:

* obj.file: File name
* obj.index: Tag index

---

## Suika.readCallStack()

Read the call stack element at the specified index.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| sp        | Integer | Stack element index.  |

### Return

Returns a dictionary that contains:

* obj.file: File name
* obj.index: Tag index

---

## Suika.writeCallStack()

Write the call stack element at the specified index.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| sp        | Integer | Stack element index.  |
| file      | String  | Script file name.     |
| index     | Integer | Tag index.            |

### Return

No return.

---

## Suika.setCallArgument()

Set a calling argument for GUI or anime.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| index     | Integer | Argument index.       |
| value     | String  | Argument value.       |

### Return

Boolean value.

---

## Suika.getCallArgument()

Get a calling argument.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| index     | Integer | Argument index.       |

### Return

String value.

---

## Suika.isPageMode()

Check if the script page mode is enabled.

### Parameters (Dictionary)

No parameters.

### Return

Returns boolean.

---

## Suika.appendBufferedMessage()

Append a string to the page mode buffer string.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| message   | String  | Message.              |

### Return

No return.

---

## Suika.getBufferedMessage()

Get the page mode buffer string.

### Parameters (Dictionary)

No parameters.

### Return

Returns a string.

---

## Suika.clearBufferedMessage()

Clear the page mode buffer string.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.resetPageLine()

Reset the message line count in a page.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.incPageLine()

Increment the line count in a page.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isPageTop()

Check if we are at the first line in a page.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.registerBGVoice()

Register a BGVoice.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| file      | String  | BGVoice file.         |

### Return

No return.

---

## Suika.getBVoice()

Get the BGVoice.

### Parameters (Dictionary)

No parameters.

### Return

Returns a file name string.

---

## Suika.setBGVoicePlaying()

Set the BGVoice state playing.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| isPlaying | Boolean | State.                |

### Return

No return.

---

## Suika.isBGVoicePlaying()

Check if the BGVoice is playing.

### Parameters (Dictionary)

No parameters.

### Return

Returns boolean.

---

## Suika.setChapterName()

Set the chapter name.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| name      | String  | Chapter name.         |

### Return

No return.

---

## Suika.getChapterName()

Get the chapter name.

### Parameters (Dictionary)

No parameters.

### Return

Returns a string.

---

## Suika.setLastMessage()

Set the last message.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| message   | String  | Message.              |
| isAppend  | Boolean | Append or replace.    |

### Return

No return.

---

## Suika.getLastMessage()

Get the last message.

### Parameters (Dictionary)

No parameters.

### Return

Returns a string.

---

## Suika.setTextSpeed()

Set the text speed.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| speed     | Float   | Text speed.           |

### Return

No return.

---

## Suika.getTextSpeed()

Get the text speed.

### Parameters (Dictionary)

No parameters.

### Return

Returns a float.

---

## Suika.setAutoSpeed()

Set the auto mode speed.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| speed     | Float   | Auto speed.           |

### Return

No return.

---

## Suika.getAutoSpeed()

Get the auto speed.

### Parameters (Dictionary)

No parameters.

### Return

Returns a float.

---

## Suika.markLastEnglishTagIndex()

Mark the last English index.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.getLastEnglishTagIndex()

Get the last English index.

### Parameters (Dictionary)

No parameters.

### Return

Returns an integer.

---

## Suika.clearLastEnglishTagIndex()

Clear the last English index.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.getLastTagName()

Get the last tag name.


### Parameters (Dictionary)

No parameters.

### Return

Returns a string.

---

## Suika.createImageFromFile()

Load an image from a file.

### Parameters (Dictionary)

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| file      | String | Path to the image file.       |

### Return

An image object, or null on failure.

---

## Suika.createImage()

Create a new blank image.

### Parameters (Dictionary)

| Parameter | Type    | Description                   |
|-----------|---------|-------------------------------|
| width     | Integer | Width of the image.           |
| height    | Integer | Height of the image.          |

### Return

An image object.

---

## Suika.getImageWidth()

Get the width of an image.

### Parameters (Dictionary)

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| img       | Object | Image object.                 |

### Return

Integer that represents the width.

---

## Suika.getImageHeight()

Get the height of an image.

### Parameters (Dictionary)

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| image     | Object | Image object.                 |

### Return

Integer that represents the height.

---

## Suika.destroyImage()

Destroy an image and free its memory.

### Parameters (Dictionary)

| Parameter | Type   | Description                   |
|-----------|--------|-------------------------------|
| image     | Object | Image object to destroy.      |

### Return

No return.

---

## Suika.drawImage()

Copy an image to another image (no blending).

### Parameters (Dictionary)

| Parameter  | Type    | Description                      |
|------------|---------|----------------------------------|
| dstImage   | Object  | Destination image.               |
| dstLeft    | Integer | X coordinate in destination.     |
| dstTop     | Integer | Y coordinate in destination.     |
| srcImage   | Object  | Source image.                    |
| dstWidth   | Integer | Width to draw.                   |
| dstHeight  | Integer | Height to draw.                  |
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

### Return

No return.

---

## Suika.drawImage3D()

Copy an image to another image (no blending).

### Parameters (Dictionary)

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
| srcWidth   | Integer | Width in source.                 |
| srcHeight  | Integer | Height in source.                |
| alpha      | Integer | 0-255                            |
| blend      | Integer | Blending type.                   |

### Blending Types

| Type                | Description                        |
|---------------------|------------------------------------|
| Suika.BLEND_ALPHA   | Alpha blending.                    |
| Suika.BLEND_ADD     | Add blending.                      |
| Suika.BLEND_SUB     | Sub blending.                      |
| Suika.BLEND_DIM     | RGB 50% alpha blending.            |

### Return

No return.

---

## Suika.drawImageAlpha()

Draw an image with alpha blending.

### Parameters (Dictionary)

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dstWidth  | Integer | Width to draw.                   |
| dstHeight | Integer | Height to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha value (`0`-`255`).         |

### Return

No return.

---

## Suika.drawImageAdd()

Draw an image with additive blending.

### Parameters (Dictionary)

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dstWidth  | Integer | Width to draw.                   |
| dstHeight | Integer | Height to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha value (`0`-`255`).         |

### Return

No return.

---

## Suika.drawImageSub()

Draw an image with subtractive blending.

### Parameters (Dictionary)

| Parameter | Type    | Description                      |
|-----------|---------|----------------------------------|
| dstImage  | Object  | Destination image.               |
| dstLeft   | Integer | X coordinate in destination.     |
| dstTop    | Integer | Y coordinate in destination.     |
| dstWidth  | Integer | Width to draw.                   |
| dstHeight | Integer | Height to draw.                  |
| srcImage  | Object  | Source image.                    |
| srcLeft   | Integer | X coordinate in source.          |
| srcTop    | Integer | Y coordinate in source.          |
| alpha     | Integer | Alpha value (`0`-`255`).         |

### Return

No return.

---

## Suika.makeColor()

Create a pixel value from RGBA components.

### Parameters (Dictionary)

| Parameter | Type    | Description      |
|-----------|---------|------------------|
| r         | Integer | Red (0-255).     |
| g         | Integer | Green (0-255).   |
| b         | Integer | Blue (0-255).    |
| a         | Integer | Alpha (0-255).   |

### Return

A pixel value.

---

## Suika.fillImageRect()

Fill a rectangular area on an image with a color.

### Parameters (Dictionary)

| Parameter | Type    | Description                         |
|-----------|---------|-------------------------------------|
| image     | Object  | Target image.                       |
| left      | Integer | X coordinate.                       |
| top       | Integer | Y coordinate.                       |
| width     | Integer | Width.                              |
| height    | Integer | Height.                             |
| color     | Integer | Color created by Suika.makeColor(). |

### Return

No return.

---

## Suika.reloadStageImages()

Reload the stage images by the config.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents success or failure.

---

## Suika.reloadStagePositions()

Reload the stage positions by the config.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.getLayerX()

Get the current position of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Integer value of the coordinate.

---

## Suika.getLayerY()

Get the current position of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Integer value of the coordinate.

---

## Suika.setLayerPosition()

Set the position of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| x         | Integer | X coordinate.              |
| y         | Integer | Y coordinate.              |

### Return

No return.

---

## Suika.getLayerScaleX()

Get the X scaling factor of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Float value of the scale.

---

## Suika.getLayerScaleY()

Get the Y scaling factor of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Float value of the scale.

---

## Suika.setLayerScale()

Set the scaling factor of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| scale_x   | Float   | Horizontal scale.          |
| scale_y   | Float   | Vertical scale.            |

### Return

No return.

---

## Suika.getLayerRotate()

Get the rotation angle of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Returns float.

---

## Suika.setLayerRotate()

Set the rotation angle of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| rot       | Float   | Rotation angle in radians. |

### Return

No return.

---

## Suika.getLayerDim()

Get the dimming state of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Returns boolean.

---

## Suika.setLayerDim()

Set the dimming state of a specific layer.

### Parameters (Dictionary) (Set)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| dim       | Boolean | Whether to dim the layer.  |

### Return

No return.

---

## Suika.getLayerAlpha()

Get the transparency of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Returns integer.

---

## Suika.setLayerAlpha()

Set the transparency of a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| alpha     | Integer | Alpha value (0-255).       |

### Return

No return.

---

## Suika.setLayerBlend()

Set the blending mode for a layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| blend     | Integer | Blend mode (Alpha, Add, Sub). |

### Return

No return.

---

## Suika.setLayerFile()

Set a file to be displayed on a layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| file_name | String  | Path to the image file.    |

### Return

Boolean that represents success or failure.

---

## Suika.setLayerFrame()

Set the frame index for eye blinking and lip synchronization.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| frame     | Integer | Frame index.               |

### Return

No return.

---

## Suika.getLayerText()

Get the string displayed on a text layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the text layer.   |

### Return

Returns string.

---

## Suika.setLayerText()

Set the string displayed on a text layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the text layer.   |
| text      | String  | Text message to set.       |

### Return

No return.

---

## Suika.getSysBtnIdleImage()

Get the sysbtn idle image.

### Parameters (Dictionary)

No parameters.

### Return

Returns an image object.

---

## Suika.getSysBtnHoverImage()

Get the sysbtn hover image.

### Parameters (Dictionary)

No parameters.

### Return

Returns an image object.

---

## Suika.clearStageBasic()

Clear the basic layers.

### Parameters (Dictionary)

No parameters.

### Return

Returns an image object.

---

## Suika.clearStage()

Clear the stage and make it initial state.

### Parameters (Dictionary)

No parameters.

### Return

Returns an image object.

---

## Suika.chposToLayer()

Convert a character position to a stage layer index.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | Character position.   |

### Return

Returns an integer.

---

## Suika.chposToEyeLayer()

Convert a character position to a stage layer index (character eye).

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | Character position.   |

### Return

Returns an integer.

---

## Suika.chposToLipLayer()

Convert a character position to a stage layer index (character lip).

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| chpos     | Integer | Character position.   |

### Return

Returns an integer.

---

## Suika.layerToChpos()

Convert a stage layer index to a character position.

### Parameters (Dictionary)

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| layer     | Integer | Layer index.          |

### Return

Returns an integer.

---

## Suika.renderStage()

Render the stage with all stage layers.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.startFade()

Start a transition effect.

### Parameters (Dictionary)

| Parameter | Type    | Description                                  |
|-----------|---------|----------------------------------------------|
| desc      | Array   | Fade descriptor.                             |
| method    | String  | Fading method.                               |
| time      | Float   | Duration in seconds.                         |
| ruleImage | Object  | Rule image object (optional).                |

### Return

Boolean value.

---

## Suika.getShakeOffset()

Get the offset for the shake command.

### Parameters (Dictionary)

No parameters.

### Return

An object that contains:
* x
* y

---

## Suika.setShakeOffset()

Set the offset for the shake command.

### Parameters (Dictionary)

| Parameter | Type    | Description    |
|-----------|---------|----------------|
| x         | Integer | X offset.      |
| y         | Integer | Y offset.      |

### Return

No return.

---

## Suika.isFadeRunning()

Check if the fading is running.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.finishFade()

Immediately end the fading effect.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.setChNameMapping()

Specify a character name index for a character position.

### Parameters (Dictionary)

| Parameter   | Type    | Description                |
|-------------|---------|----------------------------|
| chpos       | Integer | Character position.        |
| chNameIndex | Integer | Character name index.      |

### Return

No return.

---

## Suika.getTalkingChpos()

Get the position of the character currently speaking.

### Parameters (Dictionary)

No parameters.

### Return

Returns an integer.

---

## Suika.setChTalking()

Set the talking character.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| chpos     | Integer | Character position.        |

### Return

No return.

---

## Suika.getTalkingChpos()

Get the talker character position.

### Parameters (Dictionary)

No parameters.

### Return

Returns an integer.

---

## Suika.updateChDimByTalkingCh()

Automatically update character dimming based on who is speaking.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.forceChDim()

Update the character dimming manually.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| chpos     | Integer | Character position.        |
| dim       | Boolean | Dim or not.                |

### Return

No return.

---

## Suika.getChDim()

Get the dimming state.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| chpos     | Integer | Character position.        |

### Return

Returns a boolean.

---

## Suika.fillNameBox()

Fill the name box by the name box image.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.getNameBoxRect()

Get the name box position and size.

### Parameters (Dictionary)

No parameters.

### Return

Object.

* x
* y
* w
* h

---

## Suika.showNameBox()

Show or hides the name box.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| show      | Boolean | Show or hide.              |

### Return

No return.

---

## Suika.fillMessageBox()

Fill the message box by the message box image.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.showMessageBox()

Show or hide the message box.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| show      | Boolean | Whether to show the box.   |

### Return

No return.

---

## Suika.getMessageBoxRect()

Get the message box rect.

### Parameters (Dictionary)

No parameters.

### Return

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClickPosition()

Set the click animation position.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| x         | Integer | X position.                |
| y         | Integer | Y position.                |

### Return

No return.

---

## Suika.showClick()

Show or hide the click animation.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| show      | Boolean | Show or hide.              |

### Return

No return.

---

## Suika.setClickIndex()

Set the index of the click animation frame.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Frame index.               |

### Return

No return.

---

## Suika.getClickRect()

Get the click animation rect.

### Parameters (Dictionary)

No parameters.

### Return

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

Fill a choose box idle layer by the choose box idle image.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Choose box index.          |

### Return

No return.

---

## Suika.fillChooseBoxHoverImage()

Fill a choose box hover layer by the choose box hover image.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Choose box index.          |

### Return

No return.

---

## Suika.showChoosebox()

Show or hide a choice box.

### Parameters (Dictionary)

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| index      | Integer | Choice box index. (`0`-`7`) |
| showIdle   | Boolean | Show idle state.            |
| showHover  | Boolean | Show hover state.           |

### Return

No return.

---

## Suika.getChooseBoxRect()

Get the choose box rect.

### Parameters (Dictionary)

No parameters.

### Return

An object that contains:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

Show or hide the auto mode banner.

### Parameters (Dictionary)

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| show       | Boolean | Show or hide.               |

### Return

No return.

---

## Suika.showSkipModeBanner()

Show or hide the skip mode banner.

### Parameters (Dictionary)

| Parameter  | Type    | Description                 |
|------------|---------|-----------------------------|
| show       | Boolean | Show or hide.               |

### Return

No return.

---

## Suika.renderImage()

Perform direct rendering of an image to the screen.

Note that you should consider using the stage layers for normal rendering.
This API is useful for effects.

### Parameters (Dictionary)

| Parameter | Omissible    | Type    | Description                                |
|-----------|--------------|---------|--------------------------------------------|
| dstLeft   | No           | Integer | Destination top-left X position.           |
| dstTop    | No           | Integer | Destination top-left Y position.           |
| image     | No           | Object  | Image.                                     |
| srcLeft   | No           | Integer | Source top-left X position.                |
| srcTop    | No           | Integer | Source top-left Y position.                |
| srcWidth  | No           | Integer | Source width.                              |
| srcHeight | No           | Integer | Source height.                             |
| alpha     | No           | Integer | Alpha value. (`0`-`255`)                   |
| blend     | No           | Integer | Blend type.                                |

### Blend Types

| Name                 | Description       |
|----------------------|-------------------|
| Suika.BLEND_ALPHA    | Alpha blending.   |
| Suika.BLEND_ADD      | Add blending.     |
| Suika.BLEND_SUB      | Sub blending.     |

### Return

No return.

---

## Suika.renderImage3d()

Perform direct rendering of an image to the screen with 3D transformation.

Note that you should consider using the stage layers for normal rendering.
This API is useful for effects.

### Parameters (Dictionary)

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
| srcWidth  | No           | Integer | Source width.                              |
| srcHeight | No           | Integer | Source height.                             |
| alpha     | No           | Integer | Alpha value. (`0`-`255`)                   |

### Return

No return.

---

## Suika.startKirakira()

Start Kirakira effect.

Kirakira effect is an animation that is shown at the screen position where the mouse cursor is clicked.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.renderKirakira()

Render Kirakira effect.

---

## Suika.setMixerInputFile()

Play a sound file on a specific mixer track.

### Parameters (Dictionary)

| Parameter | Omissible    | Type    | Description                                |
|-----------|--------------|---------|--------------------------------------------|
| track     | No           | String  | Mixer track name.                          |
| file      | No           | String  | Path to the sound file.                    |
| isLooped  | Yes(`false`) | Boolean | Whether to loop the playback.              |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### Return

Boolean that represents whether the file was opened successfully.

---

## Suika.setMixerVolume()

Set the volume for a specific mixer track.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track name.                          |
| vol       | Float   | Volume level (0.0 to 1.0).                 |
| span      | Float   | Fade duration in seconds.                  |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### Return

No return.

---

## Suika.getMixerVolume()

Get the volume for a specific mixer track.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track name.                          |
| volume    | Float   | Volume level (0.0 to 1.0).                 |
| span      | Float   | Fade duration in seconds.                  |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### Return

Returns float.

---

## Suika.setMasterVolume()

Set the master volume affecting all tracks.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| volume    | Float   | Master volume level (0.0 to 1.0).          |

### Return

No return.

---

## Suika.getMasterVolume()

Get the master volume affecting all tracks.

### Parameters (Dictionary)

No parameters.

### Return

Returns float.

---

## Suika.setMixerGlobalVolume()

Set the global volume for a track (often used for config settings).

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track name.                          |
| vol       | Float   | Global volume level.                       |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### Return

No return.

---

## Suika.getMixerGlobalVolume()

Get the global volume for a track (often used for config settings).

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | String  | Mixer track name.                          |

### Track Names

| Name   | Description              |
|--------|--------------------------|
| bgm    | Background music track.  |
| se     | Sound effect track.      |
| voice  | Character voice track.   |
| sys    | System sound track.      |

### Return

Returns float.

---

## Suika.setCharacterVolume()

Set the volume for a specific character's voice.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| index     | Integer | Character name index.                      |
| vol       | Float   | Volume level.                              |

### Return

No return.

---

## Suika.getCharacterVolume()

Get the volume for a specific character's voice.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| ch_index  | Integer | Character name index.                      |

### Return

Get returns float.

---

## Suika.isMixerSoundFinished()

Check if the playback on a specific track is finished.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |

### Return

Boolean value.

---

## Suika.getTrackFileName()

Get the file name of the sound currently loaded in a track.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |

### Return

String representing the file path.

---

## Suika.applyCharacterVolume()

Apply a character's specific volume setting to the VOICE track.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| ch        | Integer | Character name index.                      |

### Return

No return.

---

## Suika.enableSysBtn()

Control the system button.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| isEnabled | Boolean | Enable the system button or not.           |

### Return

No return.

---

## Suika.isSysBtnEnabled()

Check if the system button is enabled.

### Parameters

No parameters.

### Return

Returns a boolean value. 

---

## Suika.updateSysBtnState()

Update the mouse tracking for the system button.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isSysBtnPointed()

Check if the system button is pointed.

### Parameters

No parameters.

### Return

Returns a boolean value.

---

## Suika.isSysBtnClicked()

Check if the system button is clicked.

### Parameters

No parameters.

### Return

Returns a boolean value.

---

## Suika.drawTextOnLayer()

Draw a text on a specified layer.

### Parameters (Dictionary)

| Parameter    | Type    | Description               |
|--------------|---------|---------------------------|
| layer        | Integer | Target stage layer index. |
| fontType     | Integer | Font selection index.     |
| fontSize     | Integer | Size of the font.         |
| color        | Integer | Color.                    |
| outlineWidth | Integer | Outline width.            |
| outlineColor | Integer | Outline color.            |
| lineMargin   | Integer | Line margin.              |
| charMargin   | Integer | Character margin.         |
| x            | Integer | Bounding box X position.  |
| y            | Integer | Bounding box Y position.  |
| width        | Integer | Bounding box width.       |
| height       | Integer | Bounding box height.      |
| text         | String  | Text.                     |

### Return

No return.

---

## Suika.getStringWidth()

Get the total width of a UTF-8 string.

### Parameters (Dictionary)

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| fontType  | Integer | Font selection index.  |
| fontSize  | Integer | Size of the font.      |
| text      | String  | Text.                  |

### Return

Integer value of the width in pixels.

---

## Suika.getStringHeight()

Get the total height of a UTF-8 string.

### Parameters (Dictionary)

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| fontType  | Integer | Font selection index.  |
| fontSize  | Integer | Size of the font.      |
| text      | String  | Text.                  |

### Return

Integer value of the height in pixels.

---

## Suika.drawGlyph()

Draw a single glyph onto an image.

### Parameters (Dictionary)

| Parameter     | Type    | Description                                |
|---------------|---------|--------------------------------------------|
| img           | Object  | Target image.                              |
| font_type     | Integer | Font selection index.                      |
| font_size     | Integer | Rendering font size.                       |
| base_font_size| Integer | Base font size for metrics.                |
| outline_size  | Integer | Width of the outline.                      |
| x             | Integer | X coordinate.                              |
| y             | Integer | Y coordinate.                              |
| color         | Pixel   | Main text color.                           |
| outline_color | Pixel   | Outline color.                             |
| codepoint     | Integer | UTF-32 code point.                         |
| is_dim        | Boolean | Whether to apply dimming.                  |

### Return

Boolean that represents success.

---

## Suika.createDrawMsg()

Create a complex message drawing context for high-level text rendering.

### Parameters (Dictionary)

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
| areaWidth      | Integer  | Draw area width.       |
| areaHeight     | Integer  | Draw area height.      |
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
| ignorePosition | Boolean  | Ignore cursor change?  |
| ignoreRuby     | Boolean  | Ignore ruby?           |
| ignoreWait     | Boolean  | Ignore inline wait?    |
| inlineWaitHook | Function | Inline wait hook.      |
| tategaki       | Boolean  | Use tategaki?          |

### Return

A message drawing context object.

---

## Suika.destroyDrawMsg()

Destroy a message drawing context.

### Parameters (Dictionary)

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |

### Return

No return.

---

## Suika.countDrawMsgChars()

Count the remaining characters excluding escape sequences.

### Parameters (Dictionary)

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |

### Return

Returns an integer.

---

## Suika.drawMessage()

Draw characters in a message up to (maxChars) characters.

### Parameters (Dictionary)

| Parameter      | Type     | Description            |
|----------------|----------|------------------------|
| context        | Object   | Draw message context.  |
| maxChars       | Integer  | Max chars.             |

### Return

Returns an integer that indicates the count of characters drawn in the call.

---

## Suika.getDrawMsgPenPosition()

Get the current pen position from a drawing context.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| context   | Object  | Drawing context.                           |

### Return

An object containing `x` and `y`.

---

## Suika.isEscapeSequenceChar()

Check if a character is part of an escape sequence.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| c         | String | Character to check.                        |

### Return

Boolean value.

---

## Suika.moveToTagFile()

Load a new tag file and move the execution point to its beginning.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the .novel or script file. |

### Return

Boolean that represents success or failure.

---

## Suika.getTagCount()

Get the total number of tags in the current script file.

### Parameters (Dictionary)

No parameters.

### Return

Integer representing the tag count.

---

## Suika.moveToTagIndex()

Move the execution pointer to a specific tag index.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Target tag index. |

### Return

Boolean value.

---

## Suika.moveToNextTag()

Move the execution pointer to the very next tag.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.moveToLabelTag()

Jump to a specific label.

### Parameters (Dictionary)

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| name      | String | Target label name.      |

### Return

Boolean value.

---

## Suika.moveToMacroTag()

Jump to a specific macro by name.

### Parameters (Dictionary)

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| name      | String | Target macro name.      |

### Return

Boolean value.

---

## Suika.moveToElseTag()

Jump to a correspoinding else/elseif/endif tag.

### Parameters (Dictionary)

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| name      | String | Target macro name.      |

### Return

Boolean value.

---

## Suika.moveToEndIfTag()

Jump to a correspoinding endif tag.

### Parameters (Dictionary)

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| name      | String | Target macro name.      |

### Return

Boolean value.

---

## Suika.moveToEndMacroTag()

Jump to a correspoinding endmacro tag.

### Parameters (Dictionary)

| Parameter | Type   | Description             |
|-----------|--------|-------------------------|
| name      | String | Target macro name.      |

### Return

Boolean value.

---

## Suika.getTagFileName()

Get the current script file name current tag.

### Parameters (Dictionary)

No parameters.

### Return

String representing the file name.

---

## Suika.getTagName()

Get the name of the current tag.

### Parameters (Dictionary)

No parameters.

### Return

String representing the tag name.

---

## Suika.getTagPropertyCount()

Get the number of the properties of the current tag.

### Parameters (Dictionary)

No parameters.

### Return

String representing the name or value.

---

## Suika.getTagPropertyName()

Iterate through and retrieve the properties (arguments) of the current
tag.

### Parameters (Dictionary)

| Parameter | Type    | Description       |
|-----------|---------|-------------------|
| index     | Integer | Property index.   |

### Return

String representing the name.

---

## Suika.getTagPropertyValue()

Iterate through and retrieve the properties (arguments) of the current
tag.

### Parameters (Dictionary) (for PropertyName/Value)

| Parameter | Type    | Description       |
|-----------|---------|-------------------|
| index     | Integer | Property index.   |

### Return

String representing the value.

---

## Suika.getTagArgBool()

Get a specific argument of the current tag, with support for default
values and optionality.

### Parameters (Dictionary)

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| name      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Boolean | Default value if missing.         |

### Return

The value of the argument in the requested type.

---

## Suika.getTagArgInt()

Get a specific argument of the current tag, with support for default
values and optionality.

### Parameters (Dictionary)

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| name      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Integer | Default value if missing.         |

### Return

The value of the argument in the requested type.

---

## Suika.getTagArgFloat()

Get a specific argument of the current tag, with support for default
values and optionality.

### Parameters (Dictionary)

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| name      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | Float   | Default value if missing.         |

### Return

The value of the argument in the requested type.

---

## Suika.getTagArgString()

Get a specific argument of the current tag, with support for default
values and optionality.

### Parameters (Dictionary)

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| name      | String  | Name of the argument.             |
| omissible | Boolean | Whether the argument is optional. |
| defVal    | String  | Default value if missing.         |

### Return

The value of the argument in the requested type.

---

## Suika.evaluateTag()

Evaluate the property values of the current tag to expand inline
variables. (`${varname}` form)

Calling this API updates the cache for the property values.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.pushTagStackIf()

Manage the internal stack for `[if]` conditional blocks.

This API marks the `if` block position for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.popTagStackIf()

Manage the internal stack for `if` conditional blocks.

This API marks the end of `if` block for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.pushTagStackWhile()

Manage the internal stack for loops (`while`).

This API marks the `while` block for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.popTagStackWhile()

Manage the internal stack for loops (`while`).

This API marks the end of `while` block for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.pushTagStackFor()

Manage the internal stack for loops (`for`).

This API marks the `for` block for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.popTagStackFor()

Manage the internal stack for loops (`for`).

This API marks the end of `for` block for nested block processing.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.loadAnimeFromFile()

Load an animation definition from a file and register it.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| file      | String  | Path to the anime file.                    |
| reg_name  | String  | Registration name for the anime.           |

### Return

An array of boolean that indicate each layer is loaded or not.

---

## Suika.newAnimeSequence()

Begin describing a new animation sequence for a specific layer.
This API is used for manually generated animations.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer index.                  |

### Return

Boolean that represents success.

---

## Suika.addAnimeSequencePropertyF()

Add a float property (e.g., position, alpha) to the current anime sequence.
This API is used for manually generated animations.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| key       | String  | Property key (e.g., "x", "y", "a").        |
| val       | Float   | Target value.                              |

### Return

Boolean value.

---

## Suika.addAnimeSequencePropertyI()

Add an integer property (e.g., position, alpha) to the current anime sequence.
This API is used for manually generated animations.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| key       | String  | Property key (e.g., "x", "y", "a").        |
| val       | Integer | Target value.                              |

### Return

Boolean value.

---

## Suika.startLayerAnime()

Start the registered animation sequence for a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer index.                  |

### Return

Boolean value.

---

## Suika.isAnimeRunning()

Check the overall animation status.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isAnimeFinishedForLayer()

Check if a specific layer's animation has ended.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer index.  |

### Return

Boolean value.

---

## Suika.updateAnimeFrame()

Update the animation frame state. Usually called once per frame.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.loadEyeImageIfExists()

Manage eye-blinking (eye-patch) image and animation for a character position.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position (Left, Center, etc.).   |
| file      | String  | Path to the eye image file.                |

### Return

Boolean value.

---

## Suika.reloadEyeAnime()

Restart the eye-blinking (eye-patch) animation for a character position.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position (Left, Center, etc.).   |

### Return

Boolean value.

---

## Suika.runLipAnime()

Start lip-sync animation based on the message content for a character.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position.                        |
| text      | String  | The message text to sync with.             |

### Return

No return.

---

## Suika.stopLipAnime()

Stop lip-sync animation.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position.                        |

### Return

No return.

---

## Suika.clearLayerAnimeSequence()

Clear animation sequences for a specific layer.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer index.  |

### Return

No return.

---

## Suika.clearAllAnimeSequence()

Clear animation sequences for all layers.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.setVariableInt()

Set a value to a local or global variable.

### Parameters (Dictionary)

| Parameter | Type    | Description            |
|-----------|---------|------------------------|
| name      | String  | Name of the variable.  |
| value     | Integer | Value to set           |

### Return

Boolean that represents success or failure.

---

## Suika.setVariableFloat()

Set a value to a local or global variable.

### Parameters (Dictionary)

| Parameter | Type    | Description             |
|-----------|---------|-------------------------|
| name      | String  | Name of the variable.   |
| value     | Float   | Value to set            |

### Return

Boolean that represents success or failure.

---

## Suika.setVariableString()

Set a value to a local or global variable.

### Parameters (Dictionary)

| Parameter | Type    | Description             |
|-----------|---------|-------------------------|
| name      | String  | Name of the variable.   |
| value     | String  | Value to set            |

### Return

Boolean that represents success or failure.

---

## Suika.getVariableInt()

Get the current value of a variable.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable.                      |

### Return

The value of the variable in integer.

---

## Suika.getVariableFloat()

Get the current value of a variable.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable.                      |

### Return

The value of the variable in float.

---

## Suika.getVariableString()

Get the current value of a variable.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable.                      |

### Return

The value of the variable in string

---

## Suika.unsetVariable()

Unset (delete) a specific variable.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable to unset.             |

### Return

No return.

---

## Suika.unsetLocalVariables()

Unset (delete) all local variables.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.makeVariableGlobal()

Set a variable to be global (persistent across saves).

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Name of the variable.                      |
| is_global | Boolean | Whether to make it global.                 |

### Return

Boolean value.

---

## Suika.isGlobalVariable()

Check the variable's global status.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Name of the variable.                      |

### Return

Boolean value.

---

## Suika.getVariableCount()

Get the number of variables.

### Parameters (Dictionary)

No parameters.

### Return

Integer for count.

---

## Suika.getVariableName()

Iterate through the registered variables.

### Parameters (Dictionary) (for getVariableName)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the variable.     |

### Return

String for name.

---

## Suika.checkVariableExists()

Check if a variable with the specified name exists.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name to check.                             |

### Return

Boolean value.

---

## Suika.executeSaveGlobal()

Execute a global save.
Global data typically includes persistent settings.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents success or failure.

---

## Suika.executeLoadGlobal()

Execute a global load.
Global data typically includes persistent settings.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents success or failure.

---

## Suika.executeSaveLocal()

Save the game progress to a specific slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Boolean that represents success or failure.

---

## Suika.executeLoadLocal()

Load game progress from a specific slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Boolean that represents success or failure.

---

## Suika.checkSaveExists()

Check if the save data exists for the specified slot index.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Boolean value.

---

## Suika.deleteLocalSave()

Delete a local save slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

No return.

---

## Suika.deleteGlobalSave()

Delete the entire global save data.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.checkRightAfterLoad()

Check if the current frame is immediately following a successful load operation.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.getSaveTimestamp()

Get the timestamp (Unix time) when the save data was created.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Integer (timestamp).

---

## Suika.getLatestSaveIndex()

Get the index of the most recently updated save slot.

### Parameters (Dictionary)

No parameters.

### Return

Integer representing the slot index.

---

## Suika.getSaveChapterName()

Retrieve the chapter title stored in a save slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

String representing the chapter name.

---

## Suika.getSaveLastMessage()

Retrieve the last displayed message stored in a save slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

String representing the message.

---

## Suika.getSaveThumbnail()

Get the thumbnail image associated with a save slot.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

An image object.

---

## Suika.clearHistory()

Clear all messages from the history (backlog).

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.addHistory()

Add a new entry to the history.

### Parameters (Dictionary)

| Parameter        | Type    | Description                                |
|------------------|---------|--------------------------------------------|
| name             | String  | Character name.                            |
| msg              | String  | Message text.                              |
| voice            | String  | Path to the voice file.                    |
| bodyColor        | Integer | Body color.                                |
| bodyOutlineColor | Integer | Body outline color.                        |
| nameColor        | Integer | Name color.                                |
| nameOutlineColor | Integer | Name outline color.                        |

### Return

Boolean that represents success.

---

## Suika.getHistoryCount()

Get the total number of entries currently in the history.

### Parameters (Dictionary)

No parameters.

### Return

Integer representing the history count.

---

## Suika.getHistoryName()

Retrieve the name at a specific history index.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index in the history.      |

### Return

String value.

---

## Suika.getHistoryMessage()

Retrieve the message at a specific history index.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index in the history.      |

### Return

String value.

---

## Suika.getHistoryVoice()

Retrieve the voice path at a specific history index.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index in the history.      |

### Return

String value.

---

## Suika.loadSeen()

Load the seen (read) flags for the current script file.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents success.

---

## Suika.saveSeen()

Save the seen (read) flags for the current script file.

### Parameters (Dictionary)

No parameters.

### Return

Boolean that represents success.

---

## Suika.getSeenFlags()

Get the seen status for the current tag.

### Parameters (Dictionary)

No parameters.

### Return

Get returns Integer.

For a `[text]` tag, `0` means unread and `1` means read.

For a `[choose]` tag, each bit indicates the option is selected before.

---

## Suika.setSeenFlags()

Set the seen status for the current tag.

### Parameters (Dictionary)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| flag      | Integer | Seen status flag.          |

### Return

No return.

---

## Suika.loadGUIFile()

Load a GUI definition file and prepare it for execution.

### Parameters (Dictionary)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| file      | String  | Path to the .gui file.                     |
| sys       | Boolean | Whether it's a system GUI (Save/Load/etc). |

### What is System GUI

System GUI is typically called when `[text]` or `[choose]` is running,
and the control will return to the interrupted tag.

### Return

Boolean that represents success or failure.

---

## Suika.startGUI()

Start the loaded GUI.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.stopGUI()

Stop the currently running GUI.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isGUIRunning()

Check if a GUI is currently active.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.isGUIFinished()

Check if a GUI has completed its operation.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.getGUIResultLabel()

Get the label of the button that was selected to finish the GUI.

### Parameters (Dictionary)

No parameters.

### Return

String representing the result label.

---

## Suika.isGUIResultTitle()

Check if the GUI was closed with a "back to title" action.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.checkIfSavedInGUI()

Check if a save operation was performed while the GUI was active.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.checkIfLoadedInGUI()

Check if a load operation was performed while the GUI was active.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.checkRightAfterSysGUI()

Check if the current frame is immediately following a return from a system GUI.

### Parameters (Dictionary)

No parameters.

### Return

Boolean value.

---

## Suika.getMillisec()

Get a lap time since the time origin in milliseconds.

### Parameters (Dictionary)

No parameters.

### Return

Integer in milliseconds.

---

## Suika.checkFileExists()

Check if a file exists.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the file.                          |

### Return

Returns boolean.

---

## Suika.readFileContent()

Read an entire file content.

### Parameters (Dictionary) (for readFileContent)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the file.                          |

### Return

Returns a string.

---

## Suika.writeSaveData()

Directly write raw save data associated with a key.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Unique key for the data.                   |
| data      | String | Data to write/read.                        |

### Return

Boolean that represents success or failure.

---

## Suika.readSaveData()

Directly read raw save data associated with a key.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Unique key for the data.                   |

### Return

Boolean that represents success or failure.

---

## Suika.playVideo()

Control video playback.

### Parameters (Dictionary) (for playVideo)

| Parameter    | Type    | Description                          |
|--------------|---------|--------------------------------------|
| file         | String  | Path to the video file.              |
| is_skippable | Boolean | Whether the user can skip the video. |

### Return

Play returns Boolean; IsPlaying returns Boolean.

---

## Suika.stopVideo()

Stop the video playback.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.isVideoPlaying()

Check if a video is playing back.

### Parameters (Dictionary)

No parameters.

### Return

Returns boolean.

---

## Suika.isFullScreenSupported()

Check for full-screen mode ability.

### Parameters (Dictionary)

No parameters.

### Return

Boolean.

---

## Suika.enterFullScreenMode()

Enter the full-screen mode.

### Parameters (Dictionary)

No parameters.

### Return

No return.

---

## Suika.speakText()

Execute Text-to-Speech (TTS) for the given message.

### Parameters (Dictionary)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| msg       | String | Text to be spoken.                         |

### Return

No return.
