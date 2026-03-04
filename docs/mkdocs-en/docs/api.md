Suika3 API Reference
====================

## Index

* Config
    * [Suika.setConfig()](#suikasetconfig)
    * [Suika.getConfigCount()](#suikagetconfigcount)
    * [Suika.getConfigKey()](#suikagetconfigkey)
    * [Suika.isConfigGlobal()](#suikaisconfigglobal)
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
    * [Suika.setCallArgument()](#suikasetcallargument)
    * [Suika.getCallArgument()](#suikagetcallargument)
* Image
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
    * [Suika.getImageWidth()](#suikagetimagewidth)
    * [Suika.getImageHeight()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImageCopy()](#suikadrawimagecopy)
    * [Suika.drawImageAlpha()](#suikadrawimagealpha)
    * [Suika.drawImageAdd()](#suikadrawimageadd)
    * [Suika.drawImageSub()](#suikadrawimagesub)
    * [Suika.makePixel()](#suikam akepixel)
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
    * [Suika.renderStage()](#suikarenderstage)
    * [Suika.startFade()](#suikastartfade)
    * [Suika.isFadeRunning()](#suikaisfaderunning)
    * [Suika.finishFade()](#suikafinishfade)
    * [Suika.setChTalking()](#suikasetchtalking)
    * [Suika.getTalkingChpos()](#suikagettalkingchpos)
    * [Suika.updateChDimByTalkingCh()](#suikaupdatechdimbytalkingch)
    * [Suika.showMsgbox()](#suikashowmsgbox)
    * [Suika.showNamebox()](#suikashownamebox)
    * [Suika.showChoosebox()](#suikashowchoosebox)
    * [Suika.renderImage()](#suikarenderimage)
    * [Suika.renderImage3d()](#suikarenderimage3d)
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
* Text
    * [Suika.utf8ToUtf32()](#suikautf8toutf32)
    * [Suika.countUtf8Chars()](#suikacountutf8chars)
    * [Suika.getGlyphWidth()](#suikagetglyphwidth)
    * [Suika.getGlyphHeight()](#suikagetglyphheight)
    * [Suika.getStringWidth()](#suikagetstringwidth)
    * [Suika.getStringHeight()](#suikagetstringheight)
    * [Suika.drawGlyph()](#suikadrawglyph)
    * [Suika.constructDrawMsgContext()](#suikaconstructdrawmsgcontext)
    * [Suika.drawMsgCommon()](#suikadrawmsgcommon)
    * [Suika.getPenPositionCommon()](#suikagetpenpositioncommon)
    * [Suika.isEscapeSequenceChar()](#suikaisescapesequencechar)
* Tag
    * [Suika.getTagCount()](#suikagettagcount)
    * [Suika.moveToTagFile()](#suikamovetotagfile)
    * [Suika.moveToTagIndex()](#suikamovetotagindex)
    * [Suika.moveToNextTag()](#suikamovetonexttag)
    * [Suika.moveToLabelTag()](#suikamovetolabeltag)
    * [Suika.moveToMacroTag()](#suikamovetomacrotag)
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
* Variable
    * [Suika.setVariableInt()](#suikasetvariableint)
    * [Suika.setVariableFloat()](#suikasetvariablefloat)
    * [Suika.setVariableString()](#suikasetvariablestring)
    * [Suika.getVariableInt()](#suikagetvariablestring)
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
    * [Suika.loadGuhUIFile()](#suikaloadguifile)
    * [Suika.startGUI())](#suikastartgui)
    * [Suika.stopGui()](#suikastopgui)
    * [Suika.isGUIRunning()](#suikaisguirunning)
    * [Suika.isGuiFinished()](#suikaisguifinished)
    * [Suika.getGUIResultLabel()](#suikagetguiresultlabel)
    * [Suika.isGUIResultTitle()](#suikaisguiresulttitle)
    * [Suika.checkIfSavedInGUI()](#suikacheckifloadedingui)
    * [Suika.checkIfLoadedInGui()](#suikacheckifloadedingui)
    * [Suika.checkRightAfterSysGUI()](#suikacheckrightaftersysgui)
* HAL
    * [Suika.resetLapTimer()](#suikaresetlaptimer)
    * [Suika.getLapTimerMillisec()](#suikagetlaptimermillisec)
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

## Suika.setConfig()

Set a config.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key of the config.                         |
| val       | String | Value of the config.                       |

### Return

No return.

---

## Suika.getConfigCount()

Get the number of the config keys.

### Parameters

No parameters.

### Return

Integer that represents a number of config keys.

---

## Suika.getConfigKey()

Get a config key for index.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| index     | Integer | Index of a config.                         |

### Return

String that represents a key of the config at the specified index.

---

## Suika.isConfigGlobal()

Check if config key is stored to global save data.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Boolean that represents whether the config is global or not.

---

## Suika.getConfigType()

Get a config value type. ("s", "b", "i", "f")

### Parameters

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

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

String value of the config.

---

## Suika.getBoolConfig()

Get a boolean config value.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Boolean value of the config.

---

## Suika.getIntConfig()

Get an integer config value.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Integer value of the config.

---

## Suika.getFloatConfig()

Get a float config value.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Float value of the config.

---

## Suika.getConfigAsString()

Get a config value as a string.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Key name.                                  |

### Return

Stringified value of the config.

---

## Suika.compareLocale()

Check if the specified locale is same as the current locale.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| locale    | String | Locale name.                               |


### Return

Bolean that represents whether the specified locale is matched to the
current one.

---

## Suika.getMousePosX()

Get the mouse X position.

### Parameters

No parameters.

### Return

Integer that represents the current mouse X coordinate.

---

## Suika.getMousePosY()

Get the mouse Y position.

### Parameters

No parameters.

### Return

Integer that represents the current mouse Y coordinate.

---

## Suika.isMouseLeftPressed()

Check if mouse left button is pressed.

### Parameters

No parameters.

### Return

Boolean that represents whether the left button is currently held down.

---

## Suika.isMouseRightPressed()

Check if mouse right button is pressed.

### Parameters

No parameters.

### Return

Boolean that represents whether the right button is currently held down.

---

## Suika.isMouseLeftClicked()

Check if mouse left button is pressed then released.

### Parameters

No parameters.

### Return

Boolean that represents whether a left-click occurred in the current frame.

---

## Suika.isMouseRightClicked()

Check if mouse right button is pressed then released.

### Parameters

No parameters.

### Return

Boolean that represents whether a right-click occurred in the current frame.

---

## Suika.isMouseDragging()

Check if mouse is dragging.

### Parameters

No parameters.

### Return

Boolean that represents whether the mouse is being moved while a button is pressed.

---

## Suika.isReturnKeyPressed()

Check if return key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isSpaceKeyPressed()

Check if space key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isEscapeKeyPressed()

Check if escape key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isUpKeyPressed()

Check if up key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isDownKeyPressed()

Check if down key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isLeftKeyPressed()

Check if left key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isRightKeyPressed()

Check if right key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isPageUpKeyPressed()

Check if pageup key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isPageDownKeyPressed()

Check if pagedown key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isControlKeyPressed()

Check if control key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isSKeyPressed()

Check if S key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isLKeyPressed()

Check if L key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isHKeyPressed()

Check if H key is pressed.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isTouchCanceled()

Check if touch is canceled.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.isSwiped()

Check if swiped.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.clearInputState()

Clear input states to avoid further input processing in the current frame.

### Parameters

No parameters.

### Return

No return.


---

## Suika.startCommandRepetition()

Start a multiple-frame command execution.

### Parameters

No parameters.

### Return

No return.

---

## Suika.stopCommandRepetition()

Stop a multiple-frame command execution.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isInCommandRepetition()

Check whether we are in a multiple-frame command execution or not.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.setMessageActive()

Set the message showing state to active.

### Parameters

No parameters.

### Return

No return.

---

## Suika.clearMessageActive()

Reset the message showing state.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isMessageActive()

Check whether the message showing state is set or not.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.startAutoMode()

Start the auto-mode.

### Parameters

No parameters.

### Return

No return.

---

## Suika.stopAutoMode()

Stop the auto-mode.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isAutoMode()

Check whether we are in the auto-mode or not.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.startSkipMode()

Start the skip-mode.

### Parameters

No parameters.

### Return

No return.

---

## Suika.stopSkipMode()

Stop the skip-mode.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isSkipMode()

Check whether we are in the skip-mode or not.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.setSaveLoad()

Set the save/load enable setting.

### Parameters

| Parameter | Type    | Description                       |
|-----------|---------|-----------------------------------|
| enable    | Boolean | Whether to enable save and load.  |

### Return

No return.

---

## Suika.isSaveLoadEnabled()

Get the save/load enable setting.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.setNonInterruptible()

Set the non-interruptible mode setting.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| mode      | Boolean | Non-interruptible mode.    |

### Return

No return.

---

## Suika.isNonInterruptible()

Get the non-interruptible mode setting.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.setPenPosition()

Set the pen position for text drawing.

### Parameters

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| x         | Integer | X coordinate.         |
| y         | Integer | Y coordinate.         |

### Return

No return.

---

## Suika.getPenPositionX()

Get the pen X position.

### Parameters

No parameters.

### Return

Integer value.

---

## Suika.getPenPositionY()

Get the pen Y position.

### Parameters

No parameters.

### Return

Integer value.

---

## Suika.pushForCall()

Push the return point to the call stack.

### Parameters

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| file      | String  | Script file name.     |
| index     | Integer | Command index.        |

### Return

Boolean that represents success or failure.

---

## Suika.popForReturn()

Pop the return point from the call stack.

### Parameters

No parameters.

### Return

An object containing `file` (String) and `index` (Integer), or null if the stack is empty.

---

## Suika.setCallArgument()

Set a calling argument for GUI or anime.

### Parameters

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| index     | Integer | Argument index.       |
| val       | String  | Argument value.       |

### Return

Boolean value.

---

## Suika.getCallArgument()

Get a calling argument.

### Parameters

| Parameter | Type    | Description           |
|-----------|---------|-----------------------|
| index     | Integer | Argument index.       |

### Return

String value.

---

## Suika.createImageFromFile()

Load an image from a file.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the image file.       |

### Return

An image object, or null on failure.

---

## Suika.createImage()

Create a new blank image.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| width     | Integer | Width of the image.           |
| height    | Integer | Height of the image.          |

### Return

An image object.

---

## Suika.getImageWidth()

Get the width of an image.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| img       | Object | Image object.                 |

### Return

Integer that represents the width.

---

## Suika.getImageHeight()

Get the height of an image.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| img       | Object | Image object.                 |

### Return

Integer that represents the height.

---

## Suika.destroyImage()

Destroy an image and free its memory.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| img       | Object | Image object to destroy.      |

### Return

No return.

---

## Suika.drawImageCopy()

Copy an image to another image (no blending).

### Parameters

| Parameter  | Type    | Description                                   |
|------------|---------|-----------------------------------------------|
| dst        | Object  | Destination image.               |
| dst_left   | Integer | X coordinate in destination.     |
| dst_top    | Integer | Y coordinate in destination.     |
| src        | Object  | Source image.                    |
| dst_width  | Integer | Width to draw.                   |
| dst_height | Integer | Height to draw.                  |
| src_left   | Integer | X coordinate in source.          |
| src_top    | Integer | Y coordinate in source.          |

### Return

No return.

---

## Suika.drawImageAlpha()

Draw an image with alpha blending.

### Parameters

| Parameter  | Type    | Description                                   |
|------------|---------|-----------------------------------------------|
| dst        | Object  | Destination image.               |
| dst_left   | Integer | X coordinate in destination.     |
| dst_top    | Integer | Y coordinate in destination.     |
| dst_width  | Integer | Width to draw.                   |
| dst_height | Integer | Height to draw.                  |
| src        | Object  | Source image.                    |
| src_left   | Integer | X coordinate in source.          |
| src_top    | Integer | Y coordinate in source.          |
| alpha      | Integer | Alpha value (0-255).             |

### Return

No return.

---

## Suika.drawImageAdd()

Draw an image with additive blending.

### Parameters

(Same as Suika.drawImageAlpha)

---

## Suika.drawImageSub()

Draw an image with subtractive blending.

### Parameters

(Same as Suika.drawImageAlpha)

---

## Suika.makePixel()

Create a pixel value from RGBA components.

### Parameters

| Parameter | Type    | Description                   |
|-----------|---------|-------------------------------|
| r         | Integer | Red (0-255).     |
| g         | Integer | Green (0-255).   |
| b         | Integer | Blue (0-255).    |
| a         | Integer | Alpha (0-255).   |

### Return

A pixel value.

---

## Suika.fillImageRect()

Fill a rectangular area on an image with a color.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| img       | Object  | Target image.                 |
| left      | Integer | X coordinate.                 |
| top       | Integer | Y coordinate.                 |
| width     | Integer | Width.                        |
| height    | Integer | Height.                       |
| color     | Pixel   | Color created by makePixel(). |

### Return

No return.

---


## Suika.reloadStageImages()

Reload the stage images by the config.

### Parameters

No parameters.

### Return

Boolean that represents success or failure.

---

## Suika.reloadStagePositions()

Reload the stage positions by the config.

### Parameters

No parameters.

### Return

No return.

---

## Suika.getLayerX() / Suika.getLayerY()

Get the current position of a specific layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Integer value of the coordinate.

---

## Suika.setLayerPosition()

Set the position of a specific layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| x         | Integer | X coordinate.              |
| y         | Integer | Y coordinate.              |

### Return

No return.

---

## Suika.getLayerScaleX() / Suika.getLayerScaleY()

Get the scaling factor of a specific layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |

### Return

Float value of the scale.

---

## Suika.setLayerScale()

Set the scaling factor of a specific layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| scale_x   | Float   | Horizontal scale.          |
| scale_y   | Float   | Vertical scale.            |

### Return

No return.

---

## Suika.getLayerRotate() / Suika.setLayerRotate()

Get or set the rotation angle of a specific layer.

### Parameters (Set)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| rot       | Float   | Rotation angle in radians. |

### Return

Get returns Float; Set returns No return.

---

## Suika.getLayerDim() / Suika.setLayerDim()

Get or set the dimming state of a specific layer.

### Parameters (Set)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| dim       | Boolean | Whether to dim the layer.  |

### Return

Get returns Boolean; Set returns No return.

---

## Suika.getLayerAlpha() / Suika.setLayerAlpha()

Get or set the transparency of a specific layer.

### Parameters (Set)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| alpha     | Integer | Alpha value (0-255).       |

### Return

Get returns Integer; Set returns No return.

---

## Suika.setLayerBlend()

Set the blending mode for a layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| blend     | Integer | Blend mode (Alpha, Add, Sub). |

### Return

No return.

---

## Suika.setLayerFileName()

Set a file to be displayed on a layer.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| file_name | String  | Path to the image file.    |

### Return

Boolean that represents success or failure.

---

## Suika.setLayerFrame()

Set the frame index for eye blinking and lip synchronization.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Index of the stage layer.  |
| frame     | Integer | Frame index.               |

### Return

No return.

---

## Suika.getLayerText() / Suika.setLayerText()

Get or set the string displayed on a text layer.

### Parameters (Set)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the text layer.   |
| msg       | String  | Text message to set.       |

### Return

Get returns String; Set returns Boolean.

---

## Suika.renderStage()

Render the stage with all stage layers.

### Parameters

No parameters.

### Return

No return.

---

## Suika.startFade()

Start a transition effect.

### Parameters

| Parameter | Type    | Description                                  |
|-----------|---------|----------------------------------------------|
| desc      | Object  | Fade descriptor (s3_fade_desc).              |
| method    | Integer | Fading method (Normal, Rule, Melt).          |
| t         | Float   | Duration in seconds.                         |
| rule_img  | Object  | Rule image object (optional).                |

### Return

Boolean value.

---

## Suika.isFadeRunning()

Check if the fading is running.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.finishFade()

Immediately end the fading effect.

### Parameters

No parameters.

### Return

No return.

---

## Suika.setChTalking() / Suika.getTalkingChpos()

Set the talking character index or get the position of the character currently speaking.

### Return

Set returns No return; Get returns Integer.

---

## Suika.updateChDimByTalkingCh()

Automatically update character dimming based on who is speaking.

### Parameters

No parameters.

### Return

No return.

---

## Suika.showMsgbox() / Suika.showNamebox()

Show or hide the message box or name box.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| show      | Boolean | Whether to show the box.   |

### Return

No return.

---

## Suika.showChoosebox()

Show or hide a choice box.

### Parameters

| Parameter  | Type    | Description                |
|------------|---------|----------------------------|
| index      | Integer | Choice box index.          |
| show_idle  | Boolean | Show idle state.           |
| show_hover | Boolean | Show hover state.          |

### Return

No return.

---

## Suika.renderImage() / Suika.renderImage3d()

Perform direct rendering of an image or a 3D transformed image to the stage.

### Return

No return.

---

## Suika.setMixerInputFile()

Play a sound file on a specific mixer track.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index (BGM, VOICE, SE, SYS).   |
| file      | String  | Path to the sound file.                    |
| is_looped | Boolean | Whether to loop the playback.              |

### Return

Boolean that represents whether the file was opened successfully.

---

## Suika.setMixerVolume() / Suika.getMixerVolume()

Set or get the volume for a specific mixer track.

### Parameters (Set)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |
| vol       | Float   | Volume level (0.0 to 1.0).                 |
| span      | Float   | Fade duration in seconds.                  |

### Return

Get returns Float; Set returns No return.

---

## Suika.setMasterVolume() / Suika.getMasterVolume()

Set or get the master volume affecting all tracks.

### Parameters (Set)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| vol       | Float   | Master volume level (0.0 to 1.0).          |

### Return

Get returns Float; Set returns No return.

---

## Suika.setMixerGlobalVolume() / Suika.getMixerGlobalVolume()

Set or get the global volume for a track (often used for config settings).

### Parameters (Set)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |
| vol       | Float   | Global volume level.                       |

### Return

Get returns Float; Set returns No return.

---

## Suika.setCharacterVolume() / Suika.getCharacterVolume()

Set or get the volume for a specific character's voice.

### Parameters (Set)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| ch_index  | Integer | Character name index.                      |
| vol       | Float   | Volume level.                              |

### Return

Get returns Float; Set returns No return.

---

## Suika.isMixerSoundFinished()

Check if the playback on a specific track is finished.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |

### Return

Boolean value.

---

## Suika.getTrackFileName()

Get the file name of the sound currently loaded in a track.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| track     | Integer | Mixer track index.                         |

### Return

String representing the file path.

---

## Suika.applyCharacterVolume()

Apply a character's specific volume setting to the VOICE track.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| ch        | Integer | Character name index.                      |

### Return

No return.

---

## Suika.utf8ToUtf32()

Convert the top character of a UTF-8 string to UTF-32.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| mbs       | String | Multi-byte string (UTF-8).                  |

### Return

An object containing `wc` (Integer, UTF-32 code point) and the number of bytes consumed.

---

## Suika.countUtf8Chars()

Count the number of characters in a UTF-8 string.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| mbs       | String | Multi-byte string (UTF-8).                  |

### Return

Integer that represents the character count.

---

## Suika.getGlyphWidth() / Suika.getGlyphHeight()

Get the width or height of a specific glyph.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| font_type | Integer | Font selection index.                      |
| font_size | Integer | Size of the font.                          |
| codepoint | Integer | UTF-32 code point.                         |

### Return

Integer value of the dimension.

---

## Suika.getStringWidth() / Suika.getStringHeight()

Get the total width or height of a UTF-8 string.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| font_type | Integer | Font selection index.                      |
| font_size | Integer | Size of the font.                          |
| mbs       | String  | Multi-byte string.                         |

### Return

Integer value of the dimension.

---

## Suika.drawGlyph()

Draw a single glyph onto an image.

### Parameters

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

## Suika.constructDrawMsgContext()

Initialize a complex message drawing context for high-level text rendering.

### Parameters

This function takes a large configuration object including:
- **dst**: Destination image.
- **msg**: The message string.
- **font/font_size**: Font settings.
- **margins**: Left, right, top, bottom, line, and char margins.
- **colors**: Text, outline, and background colors.
- **flags**: Booleans to ignore font, outline, color, size, ruby, etc.
- **use_tategaki**: Boolean for vertical text mode.

### Return

A message drawing context object.

---

## Suika.drawMsgCommon()

Draw characters in a message up to a specified maximum count using a context.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| context   | Object  | Context from constructDrawMsgContext().    |
| max_chars | Integer | Maximum characters to draw in this call.   |

### Return

Integer representing the number of characters actually drawn.

---

## Suika.getPenPositionCommon()

Get the current pen position from a drawing context.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| context   | Object  | Drawing context.                           |

### Return

An object containing `pen_x` and `pen_y`.

---

## Suika.isEscapeSequenceChar()

Check if a character is part of an escape sequence.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| c         | String | Character to check.                        |

### Return

Boolean value.

---

## Suika.getTagCount()

Get the total number of tags in the current script file.

### Parameters

No parameters.

### Return

Integer representing the tag count.

---

## Suika.moveToTagFile()

Load a new script file and move the execution point to its beginning.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| file      | String | Path to the .novel or script file. |

### Return

Boolean that represents success or failure.

---

## Suika.moveToTagIndex() / Suika.moveToNextTag()

Move the execution pointer to a specific tag index or the very next tag.

### Parameters (for moveToTagIndex)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Target tag index. |

### Return

Boolean value.

---

## Suika.moveToLabelTag() / Suika.moveToMacroTag()

Jump to a specific label or call a macro by name.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| label/name| String | Target label or macro name.      |

### Return

Boolean value.

---

## Suika.getTagFileName() / Suika.getTagName()

Get the current script file name or the name of the current tag (command).

### Parameters

No parameters.

### Return

String representing the file or tag name.

---

## Suika.getTagPropertyCount() / Suika.getTagPropertyName() / Suika.getTagPropertyValue()

Iterate through and retrieve the properties (arguments) of the current tag.

### Parameters (for PropertyName/Value)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Property index.   |

### Return

String representing the name or value.

---

## Suika.getTagArgBool() / Suika.getTagArgInt() / Suika.getTagArgFloat() / Suika.getTagArgString()

Get a specific argument of the current tag, with support for default values and optionality.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Name of the argument.            |
| omissible | Boolean | Whether the argument is optional.|
| def_val   | Mixed   | Default value if missing.        |

### Return

The value of the argument in the requested type.

---

## Suika.evaluateTag()

Evaluate the property values of the current tag (process variables and expressions).

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.pushTagStackIf() / Suika.popTagStackIf()

Manage the internal stack for `if` conditional blocks.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.pushTagStackWhile() / Suika.pushTagStackFor()

Manage the internal stack for loops (`while` or `for`).

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.loadAnimeFromFile()

Load an animation definition from a file and register it.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| fname     | String  | Path to the anime file.                    |
| reg_name  | String  | Registration name for the anime.           |

### Return

An object containing success status and whether the anime uses specific layers.

---

## Suika.newAnimeSequence()

Begin describing a new animation sequence for a specific layer.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer index.                  |

### Return

Boolean that represents success.

---

## Suika.addAnimeSequencePropertyF() / Suika.addAnimeSequencePropertyI()

Add a float or integer property (e.g., position, alpha) to the current anime sequence.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| key       | String  | Property key (e.g., "x", "y", "a").        |
| val       | Mixed   | Target value (Float or Integer).           |

### Return

Boolean value.

---

## Suika.startLayerAnime()

Start the registered animation sequence for a specific layer.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| layer     | Integer | Target stage layer index.                  |

### Return

Boolean value.

---

## Suika.isAnimeRunning() / Suika.isAnimeFinishedForLayer()

Check the overall animation status or if a specific layer's animation has ended.

### Parameters (for FinishedForLayer)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer index.  |

### Return

Boolean value.

---

## Suika.updateAnimeFrame()

Update the animation frame state. Usually called once per frame.

### Parameters

No parameters.

### Return

No return.

---

## Suika.loadEyeImageIfExists() / Suika.reloadEyeAnime()

Manage eye-blinking (eye-patch) images and animations for a character position.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position (Left, Center, etc.).   |
| fname     | String  | Path to the eye image file.                |

### Return

Boolean value.

---

## Suika.runLipAnime() / Suika.stopLipAnime()

Start or stop lip-sync animation based on the message content for a character.

### Parameters (for Run)

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| chpos     | Integer | Character position.                        |
| msg       | String  | The message text to sync with.             |

### Return

No return.

---

## Suika.clearLayerAnimeSequence() / Suika.clearAllAnimeSequence()

Clear animation sequences for a specific layer or all layers.

### Parameters (for Layer)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| layer     | Integer | Target stage layer index.  |

### Return

No return.

---

## Suika.setVariableInt() / Suika.setVariableFloat() / Suika.setVariableString()

Set a value to a local or global variable.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Name of the variable.                      |
| val       | Mixed   | Value to set (Integer, Float, or String).  |

### Return

Boolean that represents success or failure.

---

## Suika.getVariableInt() / Suika.getVariableFloat() / Suika.getVariableString()

Get the current value of a variable.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable.                      |

### Return

The value of the variable in the requested type.

---

## Suika.unsetVariable() / Suika.unsetLocalVariables()

Unset (delete) a specific variable or clear all local variables.

### Parameters (for unsetVariable)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name of the variable to unset.             |

### Return

Boolean value for single unset; No return for clearing all.

---

## Suika.makeVariableGlobal() / Suika.isGlobalVariable()

Set a variable to be global (persistent across saves) or check its global status.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Name of the variable.                      |
| is_global | Boolean | Whether to make it global.                 |

### Return

Boolean value.

---

## Suika.getVariableCount() / Suika.getVariableName()

Iterate through the registered variables.

### Parameters (for getVariableName)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the variable.     |

### Return

Integer for count; String for name.

---

## Suika.checkVariableExists()

Check if a variable with the specified name exists.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| name      | String | Name to check.                             |

### Return

Boolean value.

---

## Suika.expandStringWithVariable()

Expand a string that contains variable references (e.g., "Hello, ${name}!").

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| msg       | String | String containing variable markers.        |

### Return

String with all variable references replaced by their values.

---

## Suika.executeSaveGlobal() / Suika.executeLoadGlobal()

Execute a global save or load. Global data typically includes persistent settings and seen flags.

### Parameters

No parameters.

### Return

Boolean that represents success or failure.

---

## Suika.executeSaveLocal() / Suika.executeLoadLocal()

Save or load game progress to/from a specific slot.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Boolean that represents success or failure.

---

## Suika.checkSaveExists()

Check if the save data exists for the specified slot index.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

Boolean value.

---

## Suika.deleteLocalSave() / Suika.deleteGlobalSave()

Delete a local save slot or the entire global save data.

### Parameters (for deleteLocalSave)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

No return.

---

## Suika.checkRightAfterLoad()

Check if the current frame is immediately following a successful load operation.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.getSaveTimestamp()

Get the timestamp (Unix time) when the save data was created.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

64-bit unsigned integer (timestamp).

---

## Suika.getLatestSaveIndex()

Get the index of the most recently updated save slot.

### Parameters

No parameters.

### Return

Integer representing the slot index.

---

## Suika.getSaveChapterName() / Suika.getSaveLastMessage()

Retrieve the chapter title or the last displayed message stored in a save slot.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

String representing the chapter name or message.

---

## Suika.getSaveThumbnail()

Get the thumbnail image associated with a save slot.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index of the save slot.    |

### Return

An image object.

---

## Suika.clearHistory()

Clear all messages from the history (backlog).

### Parameters

No parameters.

### Return

No return.

---

## Suika.addHistory()

Add a new entry to the history.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| name      | String  | Character name.                            |
| msg       | String  | Message text.                              |
| voice     | String  | Path to the voice file.                    |
| colors... | Pixel   | Body and name colors/outlines.             |

### Return

Boolean that represents success.

---

## Suika.getHistoryCount()

Get the total number of entries currently in the history.

### Parameters

No parameters.

### Return

Integer representing the history count.

---

## Suika.getHistoryName() / Suika.getHistoryMessage() / Suika.getHistoryVoice()

Retrieve the name, message, or voice path at a specific history index.

### Parameters

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| index     | Integer | Index in the history.      |

### Return

String value.

---

## Suika.loadSeen() / Suika.saveSeen()

Load or save the seen (read) flags for the current script file.

### Parameters

No parameters.

### Return

Boolean that represents success.

---

## Suika.getSeenFlags() / Suika.setSeenFlags()

Get or set the seen status for the current tag.

### Parameters (for setSeenFlags)

| Parameter | Type    | Description                |
|-----------|---------|----------------------------|
| flag      | Integer | Seen status flag.          |

### Return

Get returns Integer; Set returns No return.

---

## Suika.loadGuiFile()

Load a GUI definition file and prepare it for execution.

### Parameters

| Parameter | Type    | Description                                |
|-----------|---------|--------------------------------------------|
| file      | String  | Path to the .gui file.                     |
| sys       | Boolean | Whether it's a system GUI (Save/Load/etc). |

### Return

Boolean that represents success or failure.

---

## Suika.startGui() / Suika.stopGui()

Start the loaded GUI or stop the currently running GUI.

### Parameters

No parameters.

### Return

No return.

---

## Suika.isGuiRunning() / Suika.isGuiFinished()

Check if a GUI is currently active or if it has completed its operation.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.getGuiResultLabel()

Get the label of the button that was selected to finish the GUI.

### Parameters

No parameters.

### Return

String representing the result label.

---

## Suika.isGuiResultTitle()

Check if the GUI was closed with a "back to title" action.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.checkIfSavedInGui() / Suika.checkIfLoadedInGui()

Check if a save or load operation was performed while the GUI was active.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.checkRightAfterSysGui()

Check if the current frame is immediately following a return from a system GUI.

### Parameters

No parameters.

### Return

Boolean value.

---

## Suika.resetLapTimer() / Suika.getLapTimerMillisec()

Reset a lap timer or get the elapsed time in milliseconds.

### Parameters (for getLapTimerMillisec)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| origin    | Object | Pointer or reference to the timer origin.  |

### Return

Get returns 64-bit Integer (milliseconds); Reset returns No return.

---

## Suika.checkFileExists() / Suika.readFileContent()

Check if a file exists or read its entire content into a buffer.

### Parameters (for readFileContent)

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| fname     | String | Path to the file.                          |

### Return

Check returns Boolean; Read returns an object with `buf` (String/Data) and `len`.

---

## Suika.writeSaveData() / Suika.readSaveData()

Directly write or read raw save data associated with a key.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| key       | String | Unique key for the data.                   |
| data      | Object | Buffer or data to write/read.              |

### Return

Boolean that represents success or failure.

---

## Suika.installApi() / Suika.installTag()

Register a new custom API function or a new script tag (command) handler.

### Parameters

| Parameter | Type     | Description                                |
|-----------|----------|--------------------------------------------|
| name      | String   | Name of the API or tag.                    |
| func      | Function | Callback function in the script language.  |

### Return

Boolean value.

---

## Suika.getVmInt() / Suika.setVmInt() / Suika.callVmFunction()

Interact directly with the Suika VM's internal variables and functions.

### Return

Boolean value indicating success of the VM operation.

---

## Suika.playVideo() / Suika.stopVideo() / Suika.isVideoPlaying()

Control video playback.

### Parameters (for playVideo)

| Parameter    | Type    | Description                          |
|--------------|---------|--------------------------------------|
| file         | String  | Path to the video file.              |
| is_skippable | Boolean | Whether the user can skip the video. |

### Return

Play returns Boolean; IsPlaying returns Boolean.

---

## Suika.isFullScreenSupported() / Suika.enterFullScreenMode()

Check for and control full-screen mode display.

### Return

Boolean for support/status; No return for enter/leave.

---

## Suika.logInfo() / Suika.logWarn() / Suika.logError()

Output messages to the system log or debug console.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| msg       | String | The message to log.                        |

### Return

No return.

---

## Suika.speakText()

Execute Text-to-Speech (TTS) for the given message.

### Parameters

| Parameter | Type   | Description                                |
|-----------|--------|--------------------------------------------|
| msg       | String | Text to be spoken.                         |

### Return

No return.
