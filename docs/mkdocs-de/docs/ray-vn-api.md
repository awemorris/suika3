Ray VN API Referenz
====================

Die `VN API (Suika.*)` ist für die Erstellung von Visual Novels konzipiert.

Jede `Suika.*` API-Funktion nimmt nur ein Argument an.
Das Argument muss ein Dictionary sein, und Optionen für eine Funktion müssen als Schlüssel-Wert-Paare im Dictionary gespeichert werden.
In diesem Dokument bedeutet "Parameter" ein Schlüssel-Wert-Paar in diesem Dictionary.

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

Lädt ein Plugin.

Nur diese API nimmt ein Nicht-Dictionary-Argument an.

### Parameter (Direkt)

| Parameter | Typ    | Beschreibung    |
|-----------|--------|-----------------|
| name      | String | Plugin-Name.    |

### Rückgabe

Keine Rückgabe.

---

## Suika.setConfig()

Setzt eine Konfiguration.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung                   |
|-----------|--------|--------------------------------|
| key       | String | Schlüssel der Konfiguration.   |
| value     | String | Wert der Konfiguration.        |

### Rückgabe

Keine Rückgabe.

---

## Suika.getConfigCount()

Ruft die Anzahl der Konfigurationsschlüssel ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der die Anzahl der Konfigurationsschlüssel darstellt.

---

## Suika.getConfigKey()

Ruft den Index eines Konfigurationsschlüssels ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                   |
|-----------|---------|--------------------------------|
| index     | Integer | Index einer Konfiguration.     |

### Rückgabe

String, der den Schlüssel der Konfiguration darstellt.

---

## Suika.isGlobalSaveConfig()

Überprüft, ob ein Konfigurationsschlüssel in globalen Speicherdaten gespeichert ist.

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Boolean, der angibt, ob die Konfiguration global gespeichert ist oder nicht.

---

## Suika.isLocalSaveConfig()

Überprüft, ob ein Konfigurationsschlüssel in lokalen Speicherdaten gespeichert ist.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Boolean, der angibt, ob die Konfiguration lokal gespeichert ist oder nicht.

---

## Suika.getConfigType()

Ruft den Wertetyp einer Konfiguration ab. ("s", "b", "i", "f")

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Einer der folgenden Strings.

| Wert       | Bedeutung                |
|------------|--------------------------|
| "s"        | Konfiguration ist String |
| "b"        | Konfiguration ist Boolean|
| "i"        | Konfiguration ist Integer|
| "f"        | Konfiguration ist Float  |

---

## Suika.getStringConfig()

Ruft einen String-Konfigurationswert ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

String-Wert der Konfiguration.

---

## Suika.getBoolConfig()

Ruft einen Boolean-Konfigurationswert ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Boolean-Wert der Konfiguration.

---

## Suika.getIntConfig()

Ruft einen Integer-Konfigurationswert ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Integer-Wert der Konfiguration.

---

## Suika.getFloatConfig()

Ruft einen Float-Konfigurationswert ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Float-Wert der Konfiguration.

---

## Suika.getConfigAsString()

Ruft einen Konfigurationswert als String ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| key       | String | Schlüsselname.         |

### Rückgabe

Stringifizierter Wert der Konfiguration.

---

## Suika.compareLocale()

Überprüft, ob das angegebene Gebietsschema dem aktuellen Gebietsschema entspricht.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung             |
|-----------|--------|--------------------------|
| locale    | String | Gebietsschemname.        |


### Rückgabe

Boolean, der angibt, ob das angegebene Gebietsschema mit dem aktuellen übereinstimmt.

---

## Suika.getMousePosX()

Ruft die X-Position der Maus ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der die aktuelle X-Koordinate der Maus darstellt.

---

## Suika.getMousePosY()

Ruft die Y-Position der Maus ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der die aktuelle Y-Koordinate der Maus darstellt.

---

## Suika.isMouseLeftPressed()

Überprüft, ob die linke Maustaste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der angibt, ob die linke Taste gerade gedrückt wird.

---

## Suika.isMouseRightPressed()

Überprüft, ob die rechte Maustaste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der angibt, ob die rechte Taste gerade gedrückt wird.

---

## Suika.isMouseLeftClicked()

Überprüft, ob die linke Maustaste gedrückt und dann freigegeben wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der angibt, ob in diesem Frame ein Linksklick aufgetreten ist.

---

## Suika.isMouseRightClicked()

Überprüft, ob die rechte Maustaste gedrückt und dann freigegeben wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der angibt, ob in diesem Frame ein Rechtsklick aufgetreten ist.

---

## Suika.isMouseDragging()

Überprüft, ob die Maus gezogen wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der angibt, ob die Maus bewegt wird, während eine Taste gedrückt ist.

---

## Suika.isReturnKeyPressed()

Überprüft, ob die Eingabetaste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isSpaceKeyPressed()

Überprüft, ob die Leertaste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isEscapeKeyPressed()

Überprüft, ob die Escape-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isUpKeyPressed()

Überprüft, ob die Pfeiltaste Nach-oben gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isDownKeyPressed()

Überprüft, ob die Pfeiltaste Nach-unten gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isLeftKeyPressed()

Überprüft, ob die Pfeiltaste Nach-links gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isRightKeyPressed()

Überprüft, ob die Pfeiltaste Nach-rechts gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isPageUpKeyPressed()

Überprüft, ob die Bild-Auf-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isPageDownKeyPressed()

Überprüft, ob die Bild-Ab-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isControlKeyPressed()

Überprüft, ob die Strg-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isSKeyPressed()

Überprüft, ob die S-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isLKeyPressed()

Überprüft, ob die L-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isHKeyPressed()

Überprüft, ob die H-Taste gedrückt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isTouchCanceled()

Überprüft, ob die Berührung abgebrochen wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isSwiped()

Überprüft, ob gewischt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.clearInputState()

Löscht die Eingabezustände, um weitere Eingabenverarbeitung im aktuellen Frame zu vermeiden.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.


---

## Suika.startCommandRepetition()

Startet eine mehrere Frames dauernde Befehlsausführung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.stopCommandRepetition()

Stoppt eine mehrere Frames dauernde Befehlsausführung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isInCommandRepetition()

Überprüft, ob wir uns in einer mehrere Frames dauernden Befehlsausführung befinden oder nicht.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.setMessageActive()

Setzt den Nachrichtenzeigemodus auf aktiv.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.clearMessageActive()

Setzt den Nachrichtenzeigemodus zurück.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isMessageActive()

Überprüft, ob der Nachrichtenzeigemodus gesetzt ist oder nicht.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.startAutoMode()

Startet den Auto-Modus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.stopAutoMode()

Stoppt den Auto-Modus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isAutoMode()

Überprüft, ob wir uns im Auto-Modus befinden oder nicht.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.startSkipMode()

Startet den Skip-Modus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.stopSkipMode()

Stoppt den Skip-Modus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isSkipMode()

Überprüft, ob wir uns im Skip-Modus befinden oder nicht.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.setSaveLoad()

Setzt die Speicher-/Ladefähigkeit.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                     |
|-----------|---------|----------------------------------|
| enable    | Boolean | Ob Speichern und Laden aktiviert |

### Rückgabe

Keine Rückgabe.

---

## Suika.isSaveLoadEnabled()

Ruft die Speicher-/Ladeeinstellung ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.setNonInterruptible()

Setzt die Einstellung für den nicht unterbrechbaren Modus.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                 |
|-----------|---------|------------------------------|
| enable    | Boolean | Nicht unterbrechbarer Modus. |

### Rückgabe

Keine Rückgabe.

---

## Suika.isNonInterruptible()

Ruft die Einstellung für den nicht unterbrechbaren Modus ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.setPenPosition()

Setzt die Stiftposition für das Textzeichnen.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung |
|-----------|---------|--------------|
| x         | Integer | X-Koordinate.|
| y         | Integer | Y-Koordinate.|

### Rückgabe

Keine Rückgabe.

---

## Suika.getPenPositionX()

Ruft die X-Position des Stiftes ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer-Wert.

---

## Suika.getPenPositionY()

Ruft die Y-Position des Stiftes ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer-Wert.

---

## Suika.pushForCall()

Drückt den Rückgabepunkt auf den Aufrufarithmetik.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| file      | String  | Script-Dateiname.    |
| index     | Integer | Befehlsindex.        |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.popForReturn()

Entfernt den Rückgabepunkt vom Aufrufarithmetik.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt ein Dictionary zurück, das enthält:

* obj.file: Dateiname
* obj.index: Tag-Index

---

## Suika.readCallStack()

Liest das Call-Stack-Element bei dem angegebenen Index.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| sp        | Integer | Stack-Elementindex.  |

### Rückgabe

Gibt ein Dictionary zurück, das enthält:

* obj.file: Dateiname
* obj.index: Tag-Index

---

## Suika.writeCallStack()

Schreibt das Call-Stack-Element bei dem angegebenen Index.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| sp        | Integer | Stack-Elementindex.  |
| file      | String  | Script-Dateiname.    |
| index     | Integer | Tag-Index.           |

### Rückgabe

Keine Rückgabe.

---

## Suika.setCallArgument()

Setzt ein Aufrufiargument für GUI oder Animation.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Argumentindex.     |
| value     | String  | Argumentwert.      |

### Rückgabe

Boolean-Wert.

---

## Suika.getCallArgument()

Ruft ein Aufrufiargument ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Argumentindex.     |

### Rückgabe

String-Wert.

---

## Suika.isPageMode()

Überprüft, ob der Script-Seitenmodus aktiviert ist.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt Boolean zurück.

---

## Suika.appendBufferedMessage()

Hängt einen String an die Seitenmodus-Pufferzeichenkette an.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung |
|-----------|--------|--------------|
| message   | String | Nachricht.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.getBufferedMessage()

Ruft die Seitenmodus-Pufferzeichenkette ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen String zurück.

---

## Suika.clearBufferedMessage()

Löscht die Seitenmodus-Pufferzeichenkette.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.resetPageLine()

Setzt die Nachrichtenanzahl auf einer Seite zurück.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.incPageLine()

Erhöht die Zeilenanzahl auf einer Seite.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isPageTop()

Überprüft, ob wir uns in der ersten Zeile einer Seite befinden.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.registerBGVoice()

Registriert eine BGVoice.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung         |
|-----------|--------|----------------------|
| file      | String | BGVoice-Datei.       |

### Rückgabe

Keine Rückgabe.

---

## Suika.getBVoice()

Ruft die BGVoice ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt eine String-Dateiname zurück.

---

## Suika.setBGVoicePlaying()

Setzt den BGVoice-Status auf Wiedergabe.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung |
|-----------|---------|--------------|
| isPlaying | Boolean | Status.      |

### Rückgabe

Keine Rückgabe.

---

## Suika.isBGVoicePlaying()

Überprüft, ob die BGVoice abgespielt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt Boolean zurück.

---

## Suika.setChapterName()

Setzt den Kapitelnamen.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung |
|-----------|--------|--------------|
| name      | String | Kapitelname. |

### Rückgabe

Keine Rückgabe.

---

## Suika.getChapterName()

Ruft den Kapitelnamen ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen String zurück.

---

## Suika.setLastMessage()

Setzt die letzte Nachricht.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung           |
|-----------|---------|------------------------|
| message   | String  | Nachricht.             |
| isAppend  | Boolean | Anhängen oder ersetzen.|

### Rückgabe

Keine Rückgabe.

---

## Suika.getLastMessage()

Ruft die letzte Nachricht ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen String zurück.

---

## Suika.setTextSpeed()

Setzt die Textgeschwindigkeit.

### Parameter (Dictionary)

| Parameter | Typ   | Beschreibung |
|-----------|-------|--------------|
| speed     | Float | Textspeed.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.getTextSpeed()

Ruft die Textgeschwindigkeit ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen Float zurück.

---

## Suika.setAutoSpeed()

Setzt die Auto-Modus-Geschwindigkeit.

### Parameter (Dictionary)

| Parameter | Typ   | Beschreibung |
|-----------|-------|--------------|
| speed     | Float | Auto-Speed.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getAutoSpeed()

Ruft die Auto-Geschwindigkeit ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen Float zurück.

---

## Suika.markLastEnglishTagIndex()

Markiert den letzten englischen Index.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.getLastEnglishTagIndex()

Ruft den letzten englischen Index ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.clearLastEnglishTagIndex()

Löscht den letzten englischen Index.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.getLastTagName()

Ruft den Namen des letzten Tags ab.


### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen String zurück.

---

## Suika.createImageFromFile()

Lädt ein Bild aus einer Datei.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung              |
|-----------|--------|---------------------------|
| file      | String | Pfad zur Bilddatei.       |

### Rückgabe

Ein Bildobjekt oder null bei Fehler.

---

## Suika.createImage()

Erstellt ein neues leeres Bild.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| width     | Integer | Bildbreite.       |
| height    | Integer | Bildhöhe.         |

### Rückgabe

Ein Bildobjekt.

---

## Suika.getImageWidth()

Ruft die Breite eines Bildes ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung      |
|-----------|--------|-------------------|
| img       | Object | Bildobjekt.       |

### Rückgabe

Integer, der die Breite darstellt.

---

## Suika.getImageHeight()

Ruft die Höhe eines Bildes ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung      |
|-----------|--------|-------------------|
| image     | Object | Bildobjekt.       |

### Rückgabe

Integer, der die Höhe darstellt.

---

## Suika.destroyImage()

Zerstört ein Bild und gibt seinen Speicher frei.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung            |
|-----------|--------|-------------------------|
| image     | Object | Zu zerstörendes Bild.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.drawImage()

Kopiert ein Bild in ein anderes Bild (ohne Blending).

### Parameter (Dictionary)

| Parameter  | Typ     | Beschreibung               |
|------------|---------|----------------------------|
| dstImage   | Object  | Zielbildnis.               |
| dstLeft    | Integer | X-Koordinate im Ziel.      |
| dstTop     | Integer | Y-Koordinate im Ziel.      |
| srcImage   | Object  | Quellbild.                 |
| dstWidth   | Integer | Zeichnungsbreite.          |
| dstHeight  | Integer | Zeichnungshöhe.            |
| srcLeft    | Integer | X-Koordinate in Quelle.    |
| srcTop     | Integer | Y-Koordinate in Quelle.    |
| alpha      | Integer | 0-255                      |
| blend      | Integer | Blending-Typ.              |

### Blending-Typen

| Typ                 | Beschreibung                            |
|---------------------|-----------------------------------------|
| Suika.BLEND_COPY    | Kopieren.                               |
| Suika.BLEND_ALPHA   | Alpha-Blending.                         |
| Suika.BLEND_ADD     | Additives Blending.                     |
| Suika.BLEND_SUB     | Subtraktives Blending.                  |
| Suika.BLEND_DIM     | RGB 50% Alpha-Blending.                 |
| Suika.BLEND_GLYPH   | Alpha-Blending für normale Glyphen.     |
| Suika.BLEND_EMOJI   | Alpha-Blending für Emoji-Glyphen.      |

### Rückgabe

Keine Rückgabe.

---

## Suika.drawImage3D()

Kopiert ein Bild in ein anderes Bild mit 3D-Transformation.

### Parameter (Dictionary)

| Parameter  | Typ     | Beschreibung                  |
|------------|---------|-------------------------------|
| dstImage   | Object  | Zielbildnis.                  |
| x1         | Integer | X1-Koordinate im Ziel.        |
| y1         | Integer | Y1-Koordinate im Ziel.        |
| x2         | Integer | X2-Koordinate im Ziel.        |
| y2         | Integer | Y2-Koordinate im Ziel.        |
| x3         | Integer | X3-Koordinate im Ziel.        |
| y3         | Integer | Y3-Koordinate im Ziel.        |
| x4         | Integer | X4-Koordinate im Ziel.        |
| y5         | Integer | Y4-Koordinate im Ziel.        |
| srcImage   | Object  | Quellbild.                    |
| srcLeft    | Integer | X-Koordinate in Quelle.       |
| srcTop     | Integer | Y-Koordinate in Quelle.       |
| srcWidth   | Integer | Breite in Quelle.             |
| srcHeight  | Integer | Höhe in Quelle.               |
| alpha      | Integer | 0-255                         |
| blend      | Integer | Blending-Typ.                 |

### Blending-Typen

| Typ                 | Beschreibung            |
|---------------------|-------------------------|
| Suika.BLEND_ALPHA   | Alpha-Blending.         |
| Suika.BLEND_ADD     | Additives Blending.     |
| Suika.BLEND_SUB     | Subtraktives Blending.  |
| Suika.BLEND_DIM     | RGB 50% Alpha-Blending. |

### Rückgabe

Keine Rückgabe.

---

## Suika.makeColor()

Erstellt einen Pixelwert aus RGBA-Komponenten.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung  |
|-----------|---------|---------------|
| r         | Integer | Rot (0-255).  |
| g         | Integer | Grün (0-255). |
| b         | Integer | Blau (0-255). |
| a         | Integer | Alpha (0-255).

### Rückgabe

Ein Pixelwert.

---

## Suika.fillImageRect()

Füllt einen rechteckigen Bereich auf einem Bild mit einer Farbe.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                          |
|-----------|---------|---------------------------------------|
| image     | Object  | Zielbild.                             |
| left      | Integer | X-Koordinate.                         |
| top       | Integer | Y-Koordinate.                         |
| width     | Integer | Breite.                               |
| height    | Integer | Höhe.                                 |
| color     | Integer | Von Suika.makeColor() erstellte Farbe.|

### Rückgabe

Keine Rückgabe.

---

## Suika.reloadStageImages()

Lädt die Bühnenbilder nach der Konfiguration neu.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.reloadStagePositions()

Lädt die Bühnenpositionen nach der Konfiguration neu.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerX()

Ruft die aktuelle Position einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Integer-Wert der Koordinate.

---

## Suika.getLayerY()

Ruft die aktuelle Position einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Integer-Wert der Koordinate.

---

## Suika.setLayerPosition()

Setzt die Position einer bestimmten Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|
| x         | Integer | X-Koordinate.         |
| y         | Integer | Y-Koordinate.         |

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerScaleX()

Ruft den X-Skalierungsfaktor einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Float-Wert der Skalierung.

---

## Suika.getLayerScaleY()

Ruft den Y-Skalierungsfaktor einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Float-Wert der Skalierung.

---

## Suika.setLayerScale()

Setzt den Skalierungsfaktor einer bestimmten Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung           |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene. |
| scale_x   | Float   | Horizontale Skalierung.|
| scale_y   | Float   | Vertikale Skalierung.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerRotate()

Ruft den Rotationswinkel einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Gibt Float zurück.

---

## Suika.setLayerRotate()

Setzt den Rotationswinkel einer bestimmten Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                    |
|-----------|---------|--------------------------------|
| layer     | Integer | Index der Bühnenebene.         |
| rot       | Float   | Rotationswinkel im Bogenmaß.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerDim()

Ruft den Dimmungsstatus einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Gibt Boolean zurück.

---

## Suika.setLayerDim()

Setzt den Dimmungsstatus einer bestimmten Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| layer     | Integer | Index der Bühnenebene.    |
| dim       | Boolean | Ob die Ebene gedimmt wird.|

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerAlpha()

Ruft die Transparenz einer bestimmten Ebene ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|

### Rückgabe

Gibt Integer zurück.

---

## Suika.setLayerAlpha()

Setzt die Transparenz einer bestimmten Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung           |
|-----------|---------|------------------------| 
| layer     | Integer | Index der Bühnenebene. |
| alpha     | Integer | Alpha-Wert (0-255).    |

### Rückgabe

Keine Rückgabe.

---

## Suika.setLayerBlend()

Setzt den Blending-Modus für eine Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| layer     | Integer | Index der Bühnenebene.            |
| blend     | Integer | Blend-Modus (Alpha, Add, Sub).    |

### Rückgabe

Keine Rückgabe.

---

## Suika.setLayerFileName()

Setzt eine anzuzeigende Datei auf einer Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|
| file_name | String  | Pfad zur Bilddatei.   |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.setLayerFrame()

Setzt den Frame-Index für Augenblinzeln und Lippensynchronisation.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| layer     | Integer | Index der Bühnenebene.|
| frame     | Integer | Frame-Index.          |

### Rückgabe

Keine Rückgabe.

---

## Suika.getLayerText()

Ruft den auf einer Textebene angezeigten String ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| index     | Integer | Index der Textebene.  |

### Rückgabe

Gibt String zurück.

---

## Suika.setLayerText()

Setzt den auf einer Textebene angezeigten String.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| index     | Integer | Index der Textebene.  |
| text      | String  | Anzusetzende Nachricht.|

### Rückgabe

Keine Rückgabe.

---

## Suika.getSysBtnIdleImage()

Ruft das Sysbtn-Ruhebildnis ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt ein Bildobjekt zurück.

---

## Suika.getSysBtnHoverImage()

Ruft das Sysbtn-Schwebebildnis ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt ein Bildobjekt zurück.

---

## Suika.clearStageBasic()

Löscht die Grundebenen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt ein Bildobjekt zurück.

---

## Suika.clearStage()

Löscht die Bühne und setzt sie in den Ausgangszustand zurück.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt ein Bildobjekt zurück.

---

## Suika.chposToLayer()

Konvertiert eine Charakterposition zu einem Bühnenebene-Index.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| chpos     | Integer | Charakterposition. |

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.chposToEyeLayer()

Konvertiert eine Charakterposition zu einem Bühnenebene-Index (Charakterauge).

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| chpos     | Integer | Charakterposition. |

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.chposToLipLayer()

Konvertiert eine Charakterposition zu einem Bühnenebene-Index (Charakterlippe).

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| chpos     | Integer | Charakterposition. |

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.layerToChpos()

Konvertiert einen Bühnenebene-Index zu einer Charakterposition.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung  |
|-----------|---------|---------------|
| layer     | Integer | Ebenenindex.  |

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.renderStage()

Rendert die Bühne mit allen Bühnenebenen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.startFade()

Startet einen Übergnafseffekt.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                        |
|-----------|---------|-------------------------------------|
| desc      | Array   | Fade-Deskriptor.                    |
| method    | String  | Fade-Methode.                       |
| time      | Float   | Dauer in Sekunden.                  |
| ruleImage | Object  | Regelbild-Objekt (optional).        |

### Rückgabe

Boolean-Wert.

---

## Suika.getShakeOffset()

Ruft den Versatz für den Shake-Befehl ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Ein Objekt, das enthält:
* x
* y

---

## Suika.setShakeOffset()

Setzt den Versatz für den Shake-Befehl.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung |
|-----------|---------|--------------|
| x         | Integer | X-Versatz.   |
| y         | Integer | Y-Versatz.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.isFadeRunning()

Überprüft, ob das Faden läuft.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.finishFade()

Beendet den Fade-Effekt sofort.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.setChNameMapping()

Gibt einen Charakternamenindex für eine Charakterposition an.

### Parameter (Dictionary)

| Parameter   | Typ     | Beschreibung              |
|-------------|---------|---------------------------|
| chpos       | Integer | Charakterposition.        |
| chNameIndex | Integer | Charakternamenindex.      |

### Rückgabe

Keine Rückgabe.

---

## Suika.getTalkingChpos()

Ruft die Position des gerade sprechenden Charakters ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.setChTalking()

Setzt den sprechenden Charakter.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung        |
|-----------|---------|---------------------|
| chpos     | Integer | Charakterposition.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getTalkingChpos()

Ruft die Position des sprechenden Charakters ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.updateChDimByTalkingCh()

Aktualisiert automatisch die Charakterdimmung basierend auf dem Sprecher.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.forceChDim()

Aktualisiert die Charakterdimmung manuell.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung        |
|-----------|---------|---------------------|
| chpos     | Integer | Charakterposition.  |
| dim       | Boolean | Dimmen oder nicht.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getChDim()

Ruft den Dimmungsstatus ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung        |
|-----------|---------|---------------------|
| chpos     | Integer | Charakterposition.  |

### Rückgabe

Gibt einen Boolean zurück.

---

## Suika.fillNameBox()

Füllt das Namensfeld mit dem Namensfeldbildnis.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.getNameBoxRect()

Ruft die Position und Größe des Namensfelds ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Objekt.

* x
* y
* w
* h

---

## Suika.showNameBox()

Zeigt oder versteckt das Namensfeld.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung   |
|-----------|---------|----------------|
| show      | Boolean | Anzeigen oder verstecken.|

### Rückgabe

Keine Rückgabe.

---

## Suika.fillMessageBox()

Füllt das Nachrichtenfeld mit dem Nachrichtenfeldbildnis.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.showMessageBox()

Zeigt das Nachrichtenfeld an oder versteckt es.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung           |
|-----------|---------|------------------------|
| show      | Boolean | Ob das Feld angezeigt  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getMessageBoxRect()

Ruft das Nachrichtenfeld-Rechteck ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Ein Objekt, das enthält:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClickPosition()

Setzt die Position der Klick-Animation.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung |
|-----------|---------|--------------|
| x         | Integer | X-Position.  |
| y         | Integer | Y-Position.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.showClick()

Zeigt die Klick-Animation an oder versteckt sie.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| show      | Boolean | Anzeigen oder verstecken. |

### Rückgabe

Keine Rückgabe.

---

## Suika.setClickIndex()

Setzt den Index des Klick-Animations-Frames.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung  |
|-----------|---------|---------------|
| index     | Integer | Frame-Index.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getClickRect()

Ruft das Klick-Animations-Rechteck ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Ein Objekt, das enthält:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

Füllt eine Auswahlfeld-Ruheebene mit dem Auswahlfeld-Ruhebildnis.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Auswahlfeld-Index. |

### Rückgabe

Keine Rückgabe.

---

## Suika.fillChooseBoxHoverImage()

Füllt eine Auswahlfeld-Schwebeebene mit dem Auswahlfeld-Schwebebildnis.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Auswahlfeld-Index. |

### Rückgabe

Keine Rückgabe.

---

## Suika.showChoosebox()

Zeigt eine Auswahlfeld an oder versteckt es.

### Parameter (Dictionary)

| Parameter  | Typ     | Beschreibung            |
|------------|---------|-------------------------|
| index      | Integer | Auswahlfeld-Index (0-7) |
| showIdle   | Boolean | Ruhestate anzeigen.     |
| showHover  | Boolean | Schwebestate anzeigen.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getChooseBoxRect()

Ruft das Auswahlfeld-Rechteck ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Ein Objekt, das enthält:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

Zeigt das Auto-Modus-Banner an oder versteckt es.

### Parameter (Dictionary)

| Parameter  | Typ     | Beschreibung              |
|------------|---------|---------------------------|
| show       | Boolean | Anzeigen oder verstecken. |

### Rückgabe

Keine Rückgabe.

---

## Suika.showSkipModeBanner()

Zeigt das Skip-Modus-Banner an oder versteckt es.

### Parameter (Dictionary)

| Parameter  | Typ     | Beschreibung              |
|------------|---------|---------------------------|
| show       | Boolean | Anzeigen oder verstecken. |

### Rückgabe

Keine Rückgabe.

---

## Suika.renderImage()

Führt das direkte Rendern eines Bildes auf den Bildschirm aus.

Hinweis: Sie sollten in Betracht ziehen, die Bühnenebenen für normales Rendern zu verwenden.
Diese API ist nützlich für Effekte.

### Parameter (Dictionary)

| Parameter | Auslassbar | Typ     | Beschreibung                        |
|-----------|------------|---------|-------------------------------------|
| dstLeft   | Nein       | Integer | Ziel-obere-linke X-Position.        |
| dstTop    | Nein       | Integer | Ziel-obere-linke Y-Position.        |
| image     | Nein       | Object  | Bild.                               |
| srcLeft   | Nein       | Integer | Quell-obere-linke X-Position.       |
| srcTop    | Nein       | Integer | Quell-obere-linke Y-Position.       |
| srcWidth  | Nein       | Integer | Quellenbreite.                      |
| srcHeight | Nein       | Integer | Quellhöhe.                          |
| alpha     | Nein       | Integer | Alpha-Wert (0-255).                 |
| blend     | Nein       | Integer | Blend-Typ.                          |

### Blend-Typen

| Name                 | Beschreibung      |
|----------------------|-------------------|
| Suika.BLEND_ALPHA    | Alpha-Blending.   |
| Suika.BLEND_ADD      | Add-Blending.     |
| Suika.BLEND_SUB      | Sub-Blending.     |

### Rückgabe

Keine Rückgabe.

---

## Suika.renderImage3d()

Führt das direkte Rendern eines Bildes auf den Bildschirm mit 3D-Transformation aus.

Hinweis: Sie sollten in Betracht ziehen, die Bühnenebenen für normales Rendern zu verwenden.
Diese API ist nützlich für Effekte.

### Parameter (Dictionary)

| Parameter | Auslassbar | Typ     | Beschreibung                   |
|-----------|------------|---------|--------------------------------|
| x1        | Nein       | Integer | Ziel-Vertex-1-X-Position.      |
| y1        | Nein       | Integer | Ziel-Vertex-1-Y-Position.      |
| x2        | Nein       | Integer | Ziel-Vertex-2-X-Position.      |
| y2        | Nein       | Integer | Ziel-Vertex-2-Y-Position.      |
| x3        | Nein       | Integer | Ziel-Vertex-3-X-Position.      |
| y3        | Nein       | Integer | Ziel-Vertex-3-Y-Position.      |
| x4        | Nein       | Integer | Ziel-Vertex-4-X-Position.      |
| y4        | Nein       | Integer | Ziel-Vertex-4-Y-Position.      |
| tex       | Nein       | Object  | Bild.                           |
| srcLeft   | Nein       | Integer | Quell-obere-linke X-Position.   |
| srcTop    | Nein       | Integer | Quell-obere-linke Y-Position.   |
| srcWidth  | Nein       | Integer | Quellenbreite.                  |
| srcHeight | Nein       | Integer | Quellhöhe.                      |
| alpha     | Nein       | Integer | Alpha-Wert (0-255).             |

### Rückgabe

Keine Rückgabe.

---

## Suika.startKirakira()

Startet den Kirakira-Effekt.

Kirakira-Effekt ist eine Animation, die an der Bildschirmposition angezeigt wird, wo der Mauszeiger angeklickt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.renderKirakira()

Rendert den Kirakira-Effekt.

---

## Suika.setMixerInputFile()

Spielt eine Sounddatei auf einem bestimmten Mixer-Track ab.

### Parameter (Dictionary)

| Parameter | Auslassbar    | Typ     | Beschreibung                        |
|-----------|---------------|---------|-------------------------------------|
| track     | Nein          | String  | Mixer-Track-Name.                   |
| file      | Nein          | String  | Pfad zur Sounddatei.                |
| isLooped  | Ja (`false`)  | Boolean | Ob die Wiedergabe schleifen soll.   |

### Track-Namen

| Name   | Beschreibung                |
|--------|------------------------------|
| bgm    | Hintergrundmusik-Track.     |
| se     | Sound-Effects-Track.        |
| voice  | Sprachtrack des Charakters. |
| sys    | Systemsound-Track.          |

### Rückgabe

Boolean, der angibt, ob die Datei erfolgreich geöffnet wurde.

---

## Suika.setMixerVolume()

Setzt die Lautstärke für einen bestimmten Mixer-Track.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                       |
|-----------|---------|-----------------------------------|
| track     | String  | Mixer-Track-Name.                  |
| vol       | Float   | Lautstärkepegel (0,0 bis 1,0).     |
| span      | Float   | Fade-Dauer in Sekunden.            |

### Track-Namen

| Name   | Beschreibung                |
|--------|------------------------------|
| bgm    | Hintergrundmusik-Track.     |
| se     | Sound-Effects-Track.        |
| voice  | Sprachtrack des Charakters. |
| sys    | Systemsound-Track.          |

### Rückgabe

Keine Rückgabe.

---

## Suika.getMixerVolume()

Ruft die Lautstärke für einen bestimmten Mixer-Track ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                       |
|-----------|---------|-----------------------------------|
| track     | String  | Mixer-Track-Name.                  |
| volume    | Float   | Lautstärkepegel (0,0 bis 1,0).     |
| span      | Float   | Fade-Dauer in Sekunden.            |

### Track-Namen

| Name   | Beschreibung                |
|--------|------------------------------|
| bgm    | Hintergrundmusik-Track.     |
| se     | Sound-Effects-Track.        |
| voice  | Sprachtrack des Charakters. |
| sys    | Systemsound-Track.          |

### Rückgabe

Gibt Float zurück.

---

## Suika.setMasterVolume()

Setzt die Master-Lautstärke, die alle Tracks beeinflusst.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                        |
|-----------|---------|-------------------------------------|
| volume    | Float   | Master-Lautstärkepegel (0,0-1,0).  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getMasterVolume()

Ruft die Master-Lautstärke ab, die alle Tracks beeinflusst.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt Float zurück.

---

## Suika.setMixerGlobalVolume()

Setzt die globale Lautstärke für einen Track (oft für Konfigurationseinstellungen verwendet).

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                   |
|-----------|---------|--------------------------------|
| track     | String  | Mixer-Track-Name.              |
| vol       | Float   | Globale Lautstärkepegel.       |

### Track-Namen

| Name   | Beschreibung                |
|--------|------------------------------|
| bgm    | Hintergrundmusik-Track.     |
| se     | Sound-Effects-Track.        |
| voice  | Sprachtrack des Charakters. |
| sys    | Systemsound-Track.          |

### Rückgabe

Keine Rückgabe.

---

## Suika.getMixerGlobalVolume()

Ruft die globale Lautstärke für einen Track ab (oft für Konfigurationseinstellungen verwendet).

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung           |
|-----------|---------|------------------------|
| track     | String  | Mixer-Track-Name.      |

### Track-Namen

| Name   | Beschreibung                |
|--------|------------------------------|
| bgm    | Hintergrundmusik-Track.     |
| se     | Sound-Effects-Track.        |
| voice  | Sprachtrack des Charakters. |
| sys    | Systemsound-Track.          |

### Rückgabe

Gibt Float zurück.

---

## Suika.setCharacterVolume()

Setzt die Lautstärke für die Stimme eines bestimmten Charakters.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| index     | Integer | Charakternamenindex.      |
| vol       | Float   | Lautstärkepegel.          |

### Rückgabe

Keine Rückgabe.

---

## Suika.getCharacterVolume()

Ruft die Lautstärke für die Stimme eines bestimmten Charakters ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| ch_index  | Integer | Charakternamenindex.      |

### Rückgabe

Gibt Float zurück.

---

## Suika.isMixerSoundFinished()

Überprüft, ob die Wiedergabe auf einem bestimmten Track beendet ist.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                 |
|-----------|---------|------------------------------|
| track     | Integer | Mixer-Track-Index.           |

### Rückgabe

Boolean-Wert.

---

## Suika.getTrackFileName()

Ruft den Dateinamen des Sounds ab, der derzeit auf einem Track geladen ist.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                 |
|-----------|---------|------------------------------|
| track     | Integer | Mixer-Track-Index.           |

### Rückgabe

String, der den Dateipfad darstellt.

---

## Suika.applyCharacterVolume()

Wendet die spezifische Lautstärkeeinstellung eines Charakters auf den Voice-Track an.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| ch        | Integer | Charakternamenindex.      |

### Rückgabe

Keine Rückgabe.

---

## Suika.enableSysBtn()

Steuert die Systemschaltfläche.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                        |
|-----------|---------|-------------------------------------|
| isEnabled | Boolean | Die Systemschaltfläche aktivieren.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.isSysBtnEnabled()

Überprüft, ob die Systemschaltfläche aktiviert ist.

### Parameter

Keine Parameter.

### Rückgabe

Gibt einen Boolean-Wert zurück.

---

## Suika.updateSysBtnState()

Aktualisiert die Mausverfolgung für die Systemschaltfläche.

### Parameter

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isSysBtnPointed()

Überprüft, ob auf die Systemschaltfläche zeigt.

### Parameter

Keine Parameter.

### Rückgabe

Gibt einen Boolean-Wert zurück.

---

## Suika.isSysBtnClicked()

Überprüft, ob die Systemschaltfläche angeklickt wird.

### Parameter

Keine Parameter.

### Rückgabe

Gibt einen Boolean-Wert zurück.

---

## Suika.drawTextOnLayer()

Zeichnet Text auf eine bestimmte Ebene.

### Parameter (Dictionary)

| Parameter    | Typ     | Beschreibung           |
|--------------|---------|------------------------|
| layer        | Integer | Ziel-Bühnenebene-Index.|
| fontType     | Integer | Schriftartauswahlindex.|
| fontSize     | Integer | Schriftgröße.          |
| color        | Integer | Farbe.                 |
| outlineWidth | Integer | Outline-Breite.        |
| outlineColor | Integer | Outline-Farbe.         |
| lineMargin   | Integer | Zeilenabstand.         |
| charMargin   | Integer | Zeichenabstand.        |
| x            | Integer | Begrenzungsbox X-Pos.  |
| y            | Integer | Begrenzungsbox Y-Pos.  |
| width        | Integer | Begrenzungsbox Breite. |
| height       | Integer | Begrenzungsbox Höhe.   |
| text         | String  | Text.                  |

### Rückgabe

Keine Rückgabe.

---

## Suika.getStringWidth()

Ruft die Gesamtbreite einer UTF-8-Zeichenkette ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| fontType  | Integer | Schriftartauswahlindex.   |
| fontSize  | Integer | Schriftgröße.             |
| text      | String  | Text.                     |

### Rückgabe

Integer-Wert der Breite in Pixeln.

---

## Suika.getStringHeight()

Ruft die Gesamthöhe einer UTF-8-Zeichenkette ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| fontType  | Integer | Schriftartauswahlindex.   |
| fontSize  | Integer | Schriftgröße.             |
| text      | String  | Text.                     |

### Rückgabe

Integer-Wert der Höhe in Pixeln.

---

## Suika.drawGlyph()

Zeichnet eine einzelne Glyphe auf ein Bild.

### Parameter (Dictionary)

| Parameter      | Typ     | Beschreibung                  |
|----------------|---------|-------------------------------|
| img            | Object  | Zielbild.                     |
| font_type      | Integer | Schriftartauswahlindex.       |
| font_size      | Integer | Rendering-Schriftgröße.       |
| base_font_size | Integer | Basis-Schriftgröße für Metriken|
| outline_size   | Integer | Outline-Breite.               |
| x              | Integer | X-Koordinate.                 |
| y              | Integer | Y-Koordinate.                 |
| color          | Pixel   | Haupttextfarbe.               |
| outline_color  | Pixel   | Outline-Farbe.                |
| codepoint      | Integer | UTF-32-Codepunkt.             |
| is_dim         | Boolean | Ob Dimmen angewendet werden.  |

### Rückgabe

Boolean, der Erfolg darstellt.

---

## Suika.createDrawMsg()

Erstellt einen komplexen Nachrichtenzeichnungs-Kontext für hochrangiges Text-Rendering.

### Parameter (Dictionary)

| Parameter      | Typ      | Beschreibung              |
|----------------|----------|---------------------------|
| image          | Integer  | Zielbild.                 |
| text           | String   | Zu zeichnende Nachricht.  |
| fontType       | Integer  | Schriftartauswahl.        |
| fontSize       | Integer  | Schriftgröße.             |
| baseFontSize   | Integer  | Basis-Schriftgröße.       |
| rubySize       | Integer  | Ruby-Größe.               |
| outlineSize    | Integer  | Outline-Breite.           |
| penX           | Integer  | Stift-X-Position.         |
| penY           | Integer  | Stift-Y-Position.         |
| areaWidth      | Integer  | Zeichenbereich-Breite.    |
| areaHeight     | Integer  | Zeichenbereich-Höhe.      |
| leftMargin     | Integer  | Linker Rand.              |
| rightMargin    | Integer  | Rechter Rand.             |
| topMargin      | Integer  | Oberer Rand.              |
| bottomMargin   | Integer  | Unterer Rand.             |
| lineMargin     | Integer  | Zeilenabstand.            |
| charMargin     | Integer  | Zeichenabstand.           |
| color          | Integer  | Farbe.                    |
| outlineColor   | Integer  | Outline-Farbe.            |
| bgColor        | Integer  | Hintergrundfarbe.         |
| fillBg         | Boolean  | Hintergrund füllen?       |
| dim            | Boolean  | Dimmen?                   |
| ignoreLF       | Boolean  | LF ignorieren?            |
| ignoreFont     | Boolean  | Schriftart ignorieren?    |
| ignoreOutline  | Boolean  | Outline ignorieren?       |
| ignoreColor    | Boolean  | Farbe ignorieren?         |
| ignoreSize     | Boolean  | Größe ignorieren?         |
| ignorePosition | Boolean  | Cursor ignorieren?        |
| ignoreRuby     | Boolean  | Ruby ignorieren?          |
| ignoreWait     | Boolean  | Inline-Wait ignorieren?   |
| inlineWaitHook | Function | Inline-Wait-Hook.         |
| tategaki       | Boolean  | Tategaki verwenden?       |

### Rückgabe

Ein Nachrichtenzeichnungs-Kontext-Objekt.

---

## Suika.destroyDrawMsg()

Zerstört einen Nachrichtenzeichnungs-Kontext.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung              |
|-----------|--------|---------------------------|
| context   | Object | Zeichnen-Nachricht-Kontext|

### Rückgabe

Keine Rückgabe.

---

## Suika.countDrawMsgChars()

Zählt die verbleibenden Zeichen ohne Escape-Sequenzen.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung              |
|-----------|--------|---------------------------|
| context   | Object | Zeichnen-Nachricht-Kontext|

### Rückgabe

Gibt einen Integer zurück.

---

## Suika.drawMessage()

Zeichnet Zeichen in einer Nachricht bis zu (maxChars) Zeichen.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| context   | Object  | Zeichnen-Nachricht-Kontext|
| maxChars  | Integer | Maximale Zeichen.         |

### Rückgabe

Gibt einen Integer zurück, der die Anzahl der in diesem Aufruf gezeichneten Zeichen angibt.

---

## Suika.getDrawMsgPenPosition()

Ruft die aktuelle Stiftposition aus einem Zeichnungskontext ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung        |
|-----------|--------|---------------------|
| context   | Object | Zeichnungskontext.  |

### Rückgabe

Ein Objekt mit `x` und `y`.

---

## Suika.isEscapeSequenceChar()

Überprüft, ob ein Zeichen Teil einer Escape-Sequenz ist.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung         |
|-----------|--------|----------------------|
| c         | String | Zu prüfendes Zeichen.|

### Rückgabe

Boolean-Wert.

---

## Suika.moveToTagFile()

Lädt eine neue Tag-Datei und verschiebt den Ausführungspunkt zu deren Beginn.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung                      |
|-----------|--------|-----------------------------------|
| file      | String | Pfad zur .novel- oder Script-Datei|

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.getTagCount()

Ruft die Gesamtanzahl der Tags in der aktuellen Script-Datei ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der die Tag-Anzahl darstellt.

---

## Suika.moveToTagIndex()

Verschiebt den Ausführungszeiger zu einem bestimmten Tag-Index.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Ziel-Tag-Index.    |

### Rückgabe

Boolean-Wert.

---

## Suika.moveToNextTag()

Verschiebt den Ausführungszeiger zum nächsten Tag.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.moveToLabelTag()

Springt zu einer bestimmten Bezeichnung.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung         |
|-----------|--------|----------------------|
| name      | String | Zielbezeichnungsname.|

### Rückgabe

Boolean-Wert.

---

## Suika.moveToMacroTag()

Springt zu einem bestimmten Makro nach Name.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Ziel-Makro-Name.   |

### Rückgabe

Boolean-Wert.

---

## Suika.moveToElseTag()

Springt zu einem entsprechenden else/elseif/endif-Tag.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Ziel-Makro-Name.   |

### Rückgabe

Boolean-Wert.

---

## Suika.moveToEndIfTag()

Springt zu einem entsprechenden endif-Tag.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Ziel-Makro-Name.   |

### Rückgabe

Boolean-Wert.

---

## Suika.moveToEndMacroTag()

Springt zu einem entsprechenden endmacro-Tag.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Ziel-Makro-Name.   |

### Rückgabe

Boolean-Wert.

---

## Suika.getTagFileName()

Ruft den aktuellen Script-Dateinamen für das aktuelle Tag ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

String, der den Dateinamen darstellt.

---

## Suika.getTagName()

Ruft den Namen des aktuellen Tags ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

String, der den Tag-Namen darstellt.

---

## Suika.getTagPropertyCount()

Ruft die Anzahl der Eigenschaften des aktuellen Tags ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

String, der den Namen oder Wert darstellt.

---

## Suika.getTagPropertyName()

Durchläuft und ruft die Eigenschaften (Argumente) des aktuellen Tags ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung      |
|-----------|---------|-------------------|
| index     | Integer | Eigenschaftsindex.|

### Rückgabe

String, der den Namen darstellt.

---

## Suika.getTagPropertyValue()

Durchläuft und ruft die Eigenschaften (Argumente) des aktuellen Tags ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung      |
|-----------|---------|-------------------|
| index     | Integer | Eigenschaftsindex.|

### Rückgabe

String, der den Wert darstellt.

---

## Suika.getTagArgBool()

Ruft ein bestimmtes Argument des aktuellen Tags ab, mit Unterstützung für Standardwerte und Optionalität.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| name      | String  | Name des Arguments.               |
| omissible | Boolean | Ob das Argument optional ist.     |
| defVal    | Boolean | Standardwert bei Fehlen.          |

### Rückgabe

Der Wert des Arguments im angeforderten Typ.

---

## Suika.getTagArgInt()

Ruft ein bestimmtes Argument des aktuellen Tags ab, mit Unterstützung für Standardwerte und Optionalität.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| name      | String  | Name des Arguments.               |
| omissible | Boolean | Ob das Argument optional ist.     |
| defVal    | Integer | Standardwert bei Fehlen.          |

### Rückgabe

Der Wert des Arguments im angeforderten Typ.

---

## Suika.getTagArgFloat()

Ruft ein bestimmtes Argument des aktuellen Tags ab, mit Unterstützung für Standardwerte und Optionalität.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| name      | String  | Name des Arguments.               |
| omissible | Boolean | Ob das Argument optional ist.     |
| defVal    | Float   | Standardwert bei Fehlen.          |

### Rückgabe

Der Wert des Arguments im angeforderten Typ.

---

## Suika.getTagArgString()

Ruft ein bestimmtes Argument des aktuellen Tags ab, mit Unterstützung für Standardwerte und Optionalität.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| name      | String  | Name des Arguments.               |
| omissible | Boolean | Ob das Argument optional ist.     |
| defVal    | String  | Standardwert bei Fehlen.          |

### Rückgabe

Der Wert des Arguments im angeforderten Typ.

---

## Suika.evaluateTag()

Wertet die Eigenschaftswerte des aktuellen Tags aus, um Inline-Variablen zu erweitern. (Form `${varname}`)

Das Aufrufen dieser API aktualisiert den Cache für die Eigenschaftswerte.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.pushTagStackIf()

Verwaltet den internen Stack für `[if]` Bedingungsblöcke.

Diese API markiert die `if` Block-Position für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.popTagStackIf()

Verwaltet den internen Stack für `if` Bedingungsblöcke.

Diese API markiert das Ende des `if` Block für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.pushTagStackWhile()

Verwaltet den internen Stack für Schleifen (`while`).

Diese API markiert den `while` Block für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.popTagStackWhile()

Verwaltet den internen Stack für Schleifen (`while`).

Diese API markiert das Ende des `while` Block für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.pushTagStackFor()

Verwaltet den internen Stack für Schleifen (`for`).

Diese API markiert den `for` Block für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.popTagStackFor()

Verwaltet den internen Stack für Schleifen (`for`).

Diese API markiert das Ende des `for` Block für verschachtelte Blockverarbeitung.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.loadAnimeFromFile()

Lädt eine Animationsdefinition aus einer Datei und registriert sie.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                      |
|-----------|---------|-----------------------------------|
| file      | String  | Pfad zur Anime-Datei.             |
| reg_name  | String  | Registrierungsname für Animation. |

### Rückgabe

Ein Array von Boolean, das angibt, ob jede Ebene geladen wird oder nicht.

---

## Suika.newAnimeSequence()

Beginnt mit der Beschreibung einer neuen Animationssequenz für eine bestimmte Ebene.
Diese API wird für manuell erstellte Animationen verwendet.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung            |
|-----------|---------|-------------------------|
| layer     | Integer | Ziel-Bühnenebene-Index. |

### Rückgabe

Boolean, der Erfolg darstellt.

---

## Suika.addAnimeSequencePropertyF()

Fügt eine Float-Eigenschaft (z. B. Position, Alpha) zur aktuellen Animationssequenz hinzu.
Diese API wird für manuell erstellte Animationen verwendet.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| key       | String  | Eigenschaftsschlüssel     |
| val       | Float   | Zielwert.                 |

### Rückgabe

Boolean-Wert.

---

## Suika.addAnimeSequencePropertyI()

Fügt eine Integer-Eigenschaft (z. B. Position, Alpha) zur aktuellen Animationssequenz hinzu.
Diese API wird für manuell erstellte Animationen verwendet.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| key       | String  | Eigenschaftsschlüssel.    |
| val       | Integer | Zielwert.                 |

### Rückgabe

Boolean-Wert.

---

## Suika.startLayerAnime()

Startet die registrierte Animationssequenz für eine bestimmte Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung            |
|-----------|---------|-------------------------|
| layer     | Integer | Ziel-Bühnenebene-Index. |

### Rückgabe

Boolean-Wert.

---

## Suika.isAnimeRunning()

Überprüft den allgemeinen Animationsstatus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isAnimeFinishedForLayer()

Überprüft, ob die Animation einer bestimmten Ebene beendet ist.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung            |
|-----------|---------|-------------------------|
| layer     | Integer | Ziel-Bühnenebene-Index. |

### Rückgabe

Boolean-Wert.

---

## Suika.updateAnimeFrame()

Aktualisiert den Animations-Frame-Status. Normalerweise einmal pro Frame aufgerufen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.loadEyeImageIfExists()

Verwaltet Augenblinzeln (Augen-Patch) Bild und Animation für eine Charakterposition.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                           |
|-----------|---------|----------------------------------------|
| chpos     | Integer | Charakterposition (Links, Mitte, etc.).|
| file      | String  | Pfad zur Augenbild-Datei.              |

### Rückgabe

Boolean-Wert.

---

## Suika.reloadEyeAnime()

Startet die Augenblinzeln (Augen-Patch) Animation für eine Charakterposition neu.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                           |
|-----------|---------|----------------------------------------|
| chpos     | Integer | Charakterposition (Links, Mitte, etc.).|

### Rückgabe

Boolean-Wert.

---

## Suika.runLipAnime()

Startet die Lippensynchronisationsanimation basierend auf dem Nachrichteninhalt für einen Charakter.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung              |
|-----------|---------|---------------------------|
| chpos     | Integer | Charakterposition.        |
| text      | String  | Der mit Text zu synchende.|

### Rückgabe

Keine Rückgabe.

---

## Suika.stopLipAnime()

Stoppt die Lippensynchronisationsanimation.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung      |
|-----------|---------|-------------------|
| chpos     | Integer | Charakterposition.|

### Rückgabe

Keine Rückgabe.

---

## Suika.clearLayerAnimeSequence()

Löscht Animationssequenzen für eine bestimmte Ebene.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung            |
|-----------|---------|-------------------------|
| layer     | Integer | Ziel-Bühnenebene-Index. |

### Rückgabe

Keine Rückgabe.

---

## Suika.clearAllAnimeSequence()

Löscht Animationssequenzen für alle Ebenen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.setVariableInt()

Setzt einen Wert auf eine lokale oder globale Variable.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| name      | String  | Name der Variable.    |
| value     | Integer | Einzustellender Wert. |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.setVariableFloat()

Setzt einen Wert auf eine lokale oder globale Variable.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| name      | String  | Name der Variable.    |
| value     | Float   | Einzustellender Wert. |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.setVariableString()

Setzt einen Wert auf eine lokale oder globale Variable.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung          |
|-----------|---------|------------------------|
| name      | String  | Name der Variable.    |
| value     | String  | Einzustellender Wert. |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.getVariableInt()

Ruft den aktuellen Wert einer Variable ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Name der Variable. |

### Rückgabe

Der Wert der Variable in Integer.

---

## Suika.getVariableFloat()

Ruft den aktuellen Wert einer Variable ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Name der Variable. |

### Rückgabe

Der Wert der Variable in Float.

---

## Suika.getVariableString()

Ruft den aktuellen Wert einer Variable ab.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Name der Variable. |

### Rückgabe

Der Wert der Variable in String.

---

## Suika.unsetVariable()

Deaktiviert (löscht) eine bestimmte Variable.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Zu löschender Var. |

### Rückgabe

Keine Rückgabe.

---

## Suika.unsetLocalVariables()

Deaktiviert (löscht) alle lokalen Variablen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.makeVariableGlobal()

Setzt eine Variable als global (persistent über Speicherstellen).

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                         |
|-----------|---------|--------------------------------------|
| name      | String  | Name der Variable.                  |
| is_global | Boolean | Ob sie global gemacht werden soll.  |

### Rückgabe

Boolean-Wert.

---

## Suika.isGlobalVariable()

Überprüft den globalen Status der Variable.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| name      | String | Name der Variable. |

### Rückgabe

Boolean-Wert.

---

## Suika.getVariableCount()

Ruft die Anzahl der Variablen ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer für Anzahl.

---

## Suika.getVariableName()

Durchläuft die registrierten Variablen.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| index     | Integer | Index der Variable.  |

### Rückgabe

String für Namen.

---

## Suika.checkVariableExists()

Überprüft, ob eine Variable mit dem angegebenen Namen vorhanden ist.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung      |
|-----------|--------|-------------------|
| name      | String | Zu prüfender Name. |

### Rückgabe

Boolean-Wert.

---

## Suika.expandStringWithVariable()

Expandiert eine Zeichenkette mit Variablenwerten.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung           |
|-----------|--------|------------------------|
| text      | String | Text zum Expandieren.  |

### Rückgabe

Expandierte String-Zeichenkette.

---

## Suika.executeSaveGlobal()

Führt eine globale Speicherung aus.
Globale Daten umfassen typischerweise persistent Einstellungen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.executeLoadGlobal()

Führt ein globales Laden aus.
Globale Daten umfassen typischerweise persistent Einstellungen.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.executeSaveLocal()

Speichert den Spielfortschritt in einem bestimmten Slot.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.executeLoadLocal()

Lädt den Spielfortschritt aus einem bestimmten Slot.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.checkSaveExists()

Überprüft, ob Speicherdaten für den angegebenen Slot-Index vorhanden sind.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Boolean-Wert.

---

## Suika.deleteLocalSave()

Löscht einen lokalen Speicherslot.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Keine Rückgabe.

---

## Suika.deleteGlobalSave()

Löscht die gesamten globalen Speicherdaten.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.checkRightAfterLoad()

Überprüft, ob der aktuelle Frame unmittelbar nach einer erfolgreichen Ladetätigkeit folgt.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.getSaveTimestamp()

Ruft den Zeitstempel (Unix-Zeit) ab, wenn die Speicherdaten erstellt wurden.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Integer (Zeitstempel).

---

## Suika.getLatestSaveIndex()

Ruft den Index des zuletzt aktualisierten Speicherslots ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der den Slot-Index darstellt.

---

## Suika.getSaveChapterName()

Ruft den in einem Speicherslot gespeicherten Kapiteltitel ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

String, der den Kapitelnamen darstellt.

---

## Suika.getSaveLastMessage()

Ruft die zuletzt angezeigte Nachricht ab, die in einem Speicherslot gespeichert ist.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

String, der die Nachricht darstellt.

---

## Suika.getSaveThumbnail()

Ruft das Miniaturbildnis ab, das mit einem Speicherslot verknüpft ist.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung             |
|-----------|---------|--------------------------|
| index     | Integer | Index des Speicherslots. |

### Rückgabe

Ein Bildobjekt.

---

## Suika.clearHistory()

Löscht alle Nachrichten aus der Verlaufshistorie (Rücklog).

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.addHistory()

Fügt einen neuen Eintrag zur Geschichte hinzu.

### Parameter (Dictionary)

| Parameter        | Typ     | Beschreibung               |
|------------------|---------|----------------------------|
| name             | String  | Charaktername.             |
| msg              | String  | Nachrichtentext.           |
| voice            | String  | Pfad zur Stimmdatei.       |
| bodyColor        | Integer | Körperfarbe.               |
| bodyOutlineColor | Integer | Körper-Outline-Farbe.      |
| nameColor        | Integer | Namensfarbe.               |
| nameOutlineColor | Integer | Namen-Outline-Farbe.       |

### Rückgabe

Boolean, der Erfolg darstellt.

---

## Suika.getHistoryCount()

Ruft die Gesamtanzahl der Einträge in der Geschichte ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer, der die Verlaufsanzahl darstellt.

---

## Suika.getHistoryName()

Ruft den Namen bei einem bestimmten Verlaufsindex ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung       |
|-----------|---------|-------------------|
| index     | Integer | Index der Geschichte|

### Rückgabe

String-Wert.

---

## Suika.getHistoryMessage()

Ruft die Nachricht bei einem bestimmten Verlaufsindex ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| index     | Integer | Index in der Geschichte|

### Rückgabe

String-Wert.

---

## Suika.getHistoryVoice()

Ruft den Stimmpfad bei einem bestimmten Verlaufsindex ab.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung         |
|-----------|---------|----------------------|
| index     | Integer | Index in der Geschichte|

### Rückgabe

String-Wert.

---

## Suika.loadSeen()

Lädt die Sicht-Flaggen (gelesen) für die aktuelle Script-Datei.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der Erfolg darstellt.

---

## Suika.saveSeen()

Speichert die Sicht-Flaggen (gelesen) für die aktuelle Script-Datei.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean, der Erfolg darstellt.

---

## Suika.getSeenFlags()

Ruft den Sicht-Status für das aktuelle Tag ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt Integer zurück.

Für ein `[text]` Tag bedeutet `0` ungelesen und `1` bedeutet gelesen.

Für ein `[choose]` Tag gibt jedes Bit an, ob die Option vorher ausgewählt wurde.

---

## Suika.setSeenFlags()

Setzt den Sicht-Status für das aktuelle Tag.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung      |
|-----------|---------|-------------------|
| flag      | Integer | Sicht-Status-Flag.|

### Rückgabe

Keine Rückgabe.

---

## Suika.loadGUIFile()

Lädt eine GUI-Definitionsdatei und bereitet sie zur Ausführung vor.

### Parameter (Dictionary)

| Parameter | Typ     | Beschreibung                        |
|-----------|---------|-------------------------------------|
| file      | String  | Pfad zur .gui-Datei.                |
| sys       | Boolean | Ob es ein System-GUI ist.          |

### Was ist System GUI

System GUI wird typischerweise aufgerufen, wenn `[text]` oder `[choose]` läuft,
und die Steuerung wird zum unterbrochenen Tag zurückgegeben.

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.startGUI()

Startet die geladene GUI.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.stopGUI()

Stoppt die gerade laufende GUI.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isGUIRunning()

Überprüft, ob eine GUI gerade aktiv ist.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.isGUIFinished()

Überprüft, ob eine GUI ihre Operation abgeschlossen hat.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.getGUIResultLabel()

Ruft das Label des Buttons ab, der ausgewählt wurde, um die GUI zu beenden.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

String, der das Ergebnis-Label darstellt.

---

## Suika.isGUIResultTitle()

Überprüft, ob die GUI mit einer "Zurück zum Titel"-Aktion geschlossen wurde.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.checkIfSavedInGUI()

Überprüft, ob eine Speichertätigkeit durchgeführt wurde, während die GUI aktiv war.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.checkIfLoadedInGUI()

Überprüft, ob eine Ladetätigkeit durchgeführt wurde, während die GUI aktiv war.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.checkRightAfterSysGUI()

Überprüft, ob der aktuelle Frame unmittelbar nach einer Rückkehr von einem System-GUI folgt.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean-Wert.

---

## Suika.getMillisec()

Ruft die verstrichene Zeit seit der Zeit-Ursprungsquelle in Millisekunden ab.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Integer in Millisekunden.

---

## Suika.checkFileExists()

Überprüft, ob eine Datei vorhanden ist.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung        |
|-----------|--------|---------------------|
| file      | String | Pfad zur Datei.     |

### Rückgabe

Gibt Boolean zurück.

---

## Suika.readFileContent()

Liest einen vollständigen Dateiinhalt.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung        |
|-----------|--------|---------------------|
| file      | String | Pfad zur Datei.     |

### Rückgabe

Gibt einen String zurück.

---

## Suika.writeSaveData()

Schreibt direkt rohe Speicherdaten, die einem Schlüssel zugeordnet sind.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung               |
|-----------|--------|----------------------------|
| key       | String | Eindeutiger Schlüssel.     |
| data      | String | Zu schreibende/lesende Daten|

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.readSaveData()

Liest direkt rohe Speicherdaten, die einem Schlüssel zugeordnet sind.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung              |
|-----------|--------|---------------------------|
| key       | String | Eindeutiger Schlüssel.    |

### Rückgabe

Boolean, der Erfolg oder Fehlschlag darstellt.

---

## Suika.playVideo()

Steuert die Videowiedergabe.

### Parameter (Dictionary)

| Parameter    | Typ     | Beschreibung                         |
|--------------|---------|--------------------------------------|
| file         | String  | Pfad zur Videodatei.                 |
| is_skippable | Boolean | Ob der Benutzer das Video überspringen kann|

### Rückgabe

Play gibt Boolean zurück; IsPlaying gibt Boolean zurück.

---

## Suika.stopVideo()

Stoppt die Videowiedergabe.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.isVideoPlaying()

Überprüft, ob ein Video gerade abgespielt wird.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Gibt Boolean zurück.

---

## Suika.isFullScreenSupported()

Überprüft die Fähigkeit für Vollbildmodus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Boolean.

---

## Suika.enterFullScreenMode()

Wechselt in den Vollbildmodus.

### Parameter (Dictionary)

Keine Parameter.

### Rückgabe

Keine Rückgabe.

---

## Suika.logInfo()

Gibt eine Info-Nachricht aus.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung      |
|-----------|--------|-------------------|
| msg       | String | Info-Nachricht.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.logWarn()

Gibt eine Warn-Nachricht aus.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung      |
|-----------|--------|-------------------|
| msg       | String | Warn-Nachricht.   |

### Rückgabe

Keine Rückgabe.

---

## Suika.logError()

Gibt eine Fehler-Nachricht aus.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung       |
|-----------|--------|-------------------|
| msg       | String | Fehler-Nachricht.  |

### Rückgabe

Keine Rückgabe.

---

## Suika.speakText()

Führt Text-zu-Sprache (TTS) für die angegebene Nachricht aus.

### Parameter (Dictionary)

| Parameter | Typ    | Beschreibung        |
|-----------|--------|---------------------|
| msg       | String | Zu sprechender Text.|

### Rückgabe

Keine Rückgabe.
