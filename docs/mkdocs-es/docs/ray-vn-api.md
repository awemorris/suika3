Referencia de la API de Ray VN
====================

El `VN API (Suika.*)` está diseñado para la creación de novelas visuales.

Cada función API `Suika.*` toma solo un argumento.
El argumento debe ser un diccionario y las opciones de una función deben almacenarse como pares clave-valor en el diccionario.
En este documento, "parámetro" significa un par clave-valor en ese diccionario.

## Índice

* Fundamentales
    * [Suika.loadPlugin()](#suikaloadplugin)
* Configuración
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
* Entrada
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
* Juego
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
* Imagen
    * [Suika.createImageFromFile()](#suikacreateimagefromfile)
    * [Suika.createImage()](#suikacreateimage)
* [Suika.getImageWidth()](#suikagetimagewidth)
    * [Suika.getImageHeight()](#suikagetimageheight)
    * [Suika.destroyImage()](#suikadestroyimage)
    * [Suika.drawImage()](#suikadrawimage)
    * [Suika.drawImage3D()](#suikadrawimage3d)
    * [Suika.makeColor()](#suikamakecolor)
    * [Suika.fillImageRect()](#suikafillimagerect)
* Etapa
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
* Mezclador
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
* Texto
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
* Etiqueta
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
* animado
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
* variables
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
* Guardar
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
* Historia
    * [Suika.clearHistory()](#suikaclearhistory)
    * [Suika.addHistory()](#suikaaddhistory)
    * [Suika.getHistoryCount()](#suikagethistorycount)
    * [Suika.getHistoryName()](#suikagethistoryname)
    * [Suika.getHistoryMessage()](#suikagethistorymessage)
    * [Suika.getHistoryVoice()](#suikagethistoryvoice)
* Visto
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
*hal
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

Carga un complemento.

Sólo esta API toma un argumento que no está en el diccionario.

### Parámetros (Directo)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-----------------|
| name | String | Nombre del complemento.    |

### Devolver

Sin retorno.

---

## Suika.setConfig()

Establecer una configuración.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Clave de la configuración.                         |
| value | String | Valor de la configuración.                       |

### Devolver

Sin retorno.

---

## Suika.getConfigCount()

Obtenga el número de claves de configuración.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Número entero que representa una cantidad de claves de configuración.

---

## Suika.getConfigKey()

Obtenga el índice de una clave de configuración.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| index | Integer | Índice de una configuración.                         |

### Devolver

Cadena que representa una clave de la configuración en el índice especificado.

---

## Suika.isGlobalSaveConfig()

Compruebe si hay una clave de configuración almacenada en los datos guardados globales.

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Booleano que representa si la configuración se guarda globalmente o no.

---

## Suika.isLocalSaveConfig()

Compruebe si una clave de configuración está almacenada en los datos guardados locales.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Booleano que representa si la configuración se guarda localmente o no.

---

## Suika.getConfigType()

Obtenga un tipo de valor de configuración. ("s", "b", "i", "f")

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Una de las siguientes cadenas.

| Value | Meaning |
|------------|--------------------------|
| "s" | La configuración es una cadena.        |
| "b" | La configuración es booleana.       |
| "yo" | La configuración es un número entero.       |
| "f" | La configuración es flotante.         |

---

## Suika.getStringConfig()

Obtenga un valor de configuración de cadena.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Valor de cadena de la configuración.

---

## Suika.getBoolConfig()

Obtenga un valor de configuración booleano.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Valor booleano de la configuración.

---

## Suika.getIntConfig()

Obtenga un valor de configuración entero.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Valor entero de la configuración.

---

## Suika.getFloatConfig()

Obtenga un valor de configuración flotante.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Valor flotante de la configuración.

---

## Suika.getConfigAsString()

Obtenga un valor de configuración como una cadena.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Nombre clave.                                  |

### Devolver

Valor stringificado de la configuración.

---

## Suika.compareLocale()

Compruebe si la configuración regional especificada es la misma que la configuración regional actual.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| locale | String | Nombre de la configuración regional.                               |


### Devolver

Bolean que representa si la configuración regional especificada coincide con el
el actual.

---

## Suika.getMousePosX()

Obtenga la posición X del mouse.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero que representa la coordenada X actual del mouse.

---

## Suika.getMousePosY()

Obtenga la posición Y del mouse.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero que representa la coordenada Y actual del mouse.

---

## Suika.isMouseLeftPressed()

Compruebe si el botón izquierdo del ratón está presionado.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa si el botón izquierdo está presionado actualmente.

---

## Suika.isMouseRightPressed()

Compruebe si se presiona el botón derecho del mouse.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa si el botón derecho está presionado actualmente.

---

## Suika.isMouseLeftClicked()

Compruebe si se presiona el botón izquierdo del mouse y luego se suelta.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa si se produjo un clic izquierdo en el fotograma actual.

---

## Suika.isMouseRightClicked()

Compruebe si se presiona el botón derecho del mouse y luego se suelta.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa si se produjo un clic derecho en el fotograma actual.

---

## Suika.isMouseDragging()

Compruebe si el mouse se arrastra.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa si el mouse se mueve mientras se presiona un botón.

---

## Suika.isReturnKeyPressed()

Compruebe si se presiona la tecla de retorno.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isSpaceKeyPressed()

Compruebe si se presiona la tecla espacio.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isEscapeKeyPressed()

Compruebe si la tecla Escape está presionada.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isUpKeyPressed()

Compruebe si se pulsa la tecla arriba.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isDownKeyPressed()

Compruebe si se presiona la tecla hacia abajo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isLeftKeyPressed()

Compruebe si se presiona la tecla izquierda.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isRightKeyPressed()

Compruebe si se presiona la tecla derecha.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isPageUpKeyPressed()

Compruebe si se ha pulsado la tecla de avance de página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isPageDownKeyPressed()

Compruebe si se ha pulsado la tecla de avance de página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isControlKeyPressed()

Compruebe si se pulsa la tecla de control.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isSKeyPressed()

Compruebe si se pulsa la tecla S.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isLKeyPressed()

Compruebe si se pulsa la tecla L.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isHKeyPressed()

Compruebe si se pulsa la tecla H.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isTouchCanceled()

Compruebe si el toque está cancelado.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isSwiped()

Compruebe si lo han robado.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.clearInputState()

Borre los estados de entrada para evitar un mayor procesamiento de entrada en el cuadro actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.


---

## Suika.startCommandRepetition()

Inicie la ejecución de un comando de varios fotogramas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.stopCommandRepetition()

Detener la ejecución de un comando de varios fotogramas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isInCommandRepetition()

Compruebe si estamos en una ejecución de comando de múltiples cuadros o no.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.setMessageActive()

Establezca el estado de visualización del mensaje en activo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.clearMessageActive()

Restablezca el mensaje que muestra el estado.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isMessageActive()

Compruebe si el mensaje que muestra el estado está configurado o no.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.startAutoMode()

Inicie el modo automático.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.stopAutoMode()

Detenga el modo automático.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isAutoMode()

Comprobar si estamos en modo automático o no.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.startSkipMode()

Inicie el modo de salto.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.stopSkipMode()

Detenga el modo de salto.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isSkipMode()

Compruebe si estamos en modo de salto o no.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.setSaveLoad()

Establezca la configuración de habilitar guardar/cargar.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------------------|
| enable | Boolean | Ya sea para habilitar guardar y cargar.  |

### Devolver

Sin retorno.

---

## Suika.isSaveLoadEnabled()

Obtenga la configuración de habilitar guardar/cargar.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.setNonInterruptible()

Establezca la configuración del modo no interrumpible.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| enable | Boolean | Modo no interrumpible.    |

### Devolver

Sin retorno.

---

## Suika.isNonInterruptible()

Obtenga la configuración del modo no interrumpible.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.setPenPosition()

Establezca la posición del lápiz para dibujar texto.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| x | Integer | Coordenada X.         |
| y | Integer | Coordenada Y.         |

### Devolver

Sin retorno.

---

## Suika.getPenPositionX()

Obtenga la posición X del lápiz.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor entero.

---

## Suika.getPenPositionY()

Obtenga la posición Y del bolígrafo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor entero.

---

## Suika.pushForCall()

Empuje el punto de retorno a la pila de llamadas.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| file | String | Nombre del archivo de secuencia de comandos.     |
| index | Integer | Índice de comando.        |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.popForReturn()

Extraiga el punto de retorno de la pila de llamadas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un diccionario que contiene:

* obj.file: nombre del archivo
* obj.index: Índice de etiquetas

---

## Suika.readCallStack()

Lea el elemento de la pila de llamadas en el índice especificado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| sp | Integer | Índice de elementos de pila.  |

### Devolver

Devuelve un diccionario que contiene:

* obj.file: nombre del archivo
* obj.index: Índice de etiquetas

---

## Suika.writeCallStack()

Escriba el elemento de la pila de llamadas en el índice especificado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| sp | Integer | Índice de elementos de pila.  |
| file | String | Nombre del archivo de secuencia de comandos.     |
| index | Integer | Índice de etiquetas.            |

### Devolver

Sin retorno.

---

## Suika.setCallArgument()

Establezca un argumento de llamada para GUI o anime.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| index | Integer | Índice de argumentos.       |
| value | String | Valor del argumento.       |

### Devolver

Valor booleano.

---

## Suika.getCallArgument()

Obtenga un argumento de llamada.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| index | Integer | Índice de argumentos.       |

### Devolver

Valor de cadena.

---

## Suika.isPageMode()

Compruebe si el modo de página de secuencia de comandos está habilitado.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve booleano.

---

## Suika.appendBufferedMessage()

Agregue una cadena a la cadena del búfer del modo de página.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| message | String | Message. |

### Devolver

Sin retorno.

---

## Suika.getBufferedMessage()

Obtenga la cadena del búfer del modo de página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve una cadena.

---

## Suika.clearBufferedMessage()

Borre la cadena del búfer del modo de página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.resetPageLine()

Restablecer el recuento de líneas de mensajes en una página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.incPageLine()

Incrementar el recuento de líneas en una página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isPageTop()

Comprobar si estamos en la primera línea de una página.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.registerBGVoice()

Registre una BGVoice.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| file | String | Archivo BGVoice.         |

### Devolver

Sin retorno.

---

## Suika.getBVoice()

Obtenga BGVoice.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve una cadena de nombre de archivo.

---

## Suika.setBGVoicePlaying()

Configure el estado de reproducción de BGVoice.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| isPlaying | Boolean | State. |

### Devolver

Sin retorno.

---

## Suika.isBGVoicePlaying()

Compruebe si BGVoice se está reproduciendo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve booleano.

---

## Suika.setChapterName()

Establezca el nombre del capítulo.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| name | String | Nombre del capítulo.         |

### Devolver

Sin retorno.

---

## Suika.getChapterName()

Obtenga el nombre del capítulo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve una cadena.

---

## Suika.setLastMessage()

Establecer el último mensaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| message | String | Message. |
| isAppend | Boolean | Agregar o reemplazar.    |

### Devolver

Sin retorno.

---

## Suika.getLastMessage()

Recibe el último mensaje.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve una cadena.

---

## Suika.setTextSpeed()

Establece la velocidad del texto.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| speed | Float | Velocidad del texto.           |

### Devolver

Sin retorno.

---

## Suika.getTextSpeed()

Obtenga la velocidad del texto.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un flotador.

---

## Suika.setAutoSpeed()

Configure la velocidad del modo automático.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| speed | Float | Velocidad automática.           |

### Devolver

Sin retorno.

---

## Suika.getAutoSpeed()

Obtenga la velocidad automática.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un flotador.

---

## Suika.markLastEnglishTagIndex()

Marque el último índice en inglés.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.getLastEnglishTagIndex()

Obtenga el último índice en inglés.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un número entero.

---

## Suika.clearLastEnglishTagIndex()

Borre el último índice en inglés.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.getLastTagName()

Obtenga el último nombre de la etiqueta.


### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve una cadena.

---

## Suika.createImageFromFile()

Cargue una imagen desde un archivo.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------------|
| file | String | Ruta al archivo de imagen.       |

### Devolver

Un objeto de imagen, o nulo en caso de error.

---

## Suika.createImage()

Crea una nueva imagen en blanco.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------------------|
| width | Integer | Ancho de la imagen.           |
| height | Integer | Altura de la imagen.          |

### Devolver

Un objeto de imagen.

---

## Suika.getImageWidth()

Obtenga el ancho de una imagen.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------------|
| img | Object | Objeto de imagen.                 |

### Devolver

Entero que representa el ancho.

---

## Suika.getImageHeight()

Obtener la altura de una imagen.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------------|
| image | Object | Objeto de imagen.                 |

### Devolver

Entero que representa la altura.

---

## Suika.destroyImage()

Destruye una imagen y libera su memoria.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------------|
| image | Object | Objeto de imagen para destruir.      |

### Devolver

Sin retorno.

---

## Suika.drawImage()

Copie una imagen a otra imagen (sin mezclar).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------|---------|----------------------------------|
| dstImage | Object | Imagen de destino.               |
| dstLeft | Integer | Coordenada X en destino.     |
| dstTop | Integer | Coordenada Y en destino.     |
| srcImage | Object | Imagen fuente.                    |
| dstWidth | Integer | Ancho para dibujar.                   |
| dstHeight | Integer | Altura para dibujar.                  |
| srcLeft | Integer | Coordenada X en la fuente.          |
| srcTop | Integer | Coordenada Y en fuente.          |
| alpha | Integer | 0-255 |
| blend | Integer | Tipo de mezcla.                   |

### Tipos de mezcla

| Tipo | Descripciﾃｳn |
|---------------------|------------------------------------|
| Suika.BLEND_COPY | Copy. |
| Suika.BLEND_ALPHA | Mezcla alfa.                    |
| Suika.BLEND_ADD | Agrega la mezcla.                      |
| Suika.BLEND_SUB | Submezcla.                      |
| Suika.BLEND_DIM | Mezcla RGB 50% alfa.            |
| Suika.BLEND_GLYPH | Mezcla alfa para glifos normales.  |
| Suika.BLEND_EMOJI | Mezcla alfa para glifos emoji.   |

### Devolver

Sin retorno.

---

## Suika.drawImage3D()

Copie una imagen a otra imagen (sin mezclar).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------|---------|----------------------------------|
| dstImage | Object | Imagen de destino.               |
| x1 | Integer | Coordenada x1 en destino.    |
| y1 | Integer | Coordenada y1 en destino.    |
| x2 | Integer | Coordenada x2 en destino.    |
| y2 | Integer | Coordenada y2 en destino.    |
| x3 | Integer | Coordenada x3 en destino.    |
| y3 | Integer | Coordenada y3 en destino.    |
| x4 | Integer | Coordenada x4 en destino.    |
| y5 | Integer | Coordenada y4 en destino.    |
| srcImage | Object | Imagen fuente.                    |
| srcLeft | Integer | Coordenada X en la fuente.          |
| srcTop | Integer | Coordenada Y en fuente.          |
| srcWidth | Integer | Ancho en fuente.                 |
| srcHeight | Integer | Altura en fuente.                |
| alpha | Integer | 0-255 |
| blend | Integer | Tipo de mezcla.                   |

### Tipos de mezcla

| Tipo | Descripciﾃｳn |
|---------------------|------------------------------------|
| Suika.BLEND_ALPHA | Mezcla alfa.                    |
| Suika.BLEND_ADD | Agrega la mezcla.                      |
| Suika.BLEND_SUB | Submezcla.                      |
| Suika.BLEND_DIM | Mezcla RGB 50% alfa.            |

### Devolver

Sin retorno.

---

## Suika.drawImageAlpha()

Dibuja una imagen con fusión alfa.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------------|
| dstImage | Object | Imagen de destino.               |
| dstLeft | Integer | Coordenada X en destino.     |
| dstTop | Integer | Coordenada Y en destino.     |
| dstWidth | Integer | Ancho para dibujar.                   |
| dstHeight | Integer | Altura para dibujar.                  |
| srcImage | Object | Imagen fuente.                    |
| srcLeft | Integer | Coordenada X en la fuente.          |
| srcTop | Integer | Coordenada Y en fuente.          |
| alpha | Integer | Valor alfa (`0`-`255`).         |

### Devolver

Sin retorno.

---

## Suika.drawImageAdd()

Dibuja una imagen con mezcla aditiva.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------------|
| dstImage | Object | Imagen de destino.               |
| dstLeft | Integer | Coordenada X en destino.     |
| dstTop | Integer | Coordenada Y en destino.     |
| dstWidth | Integer | Ancho para dibujar.                   |
| dstHeight | Integer | Altura para dibujar.                  |
| srcImage | Object | Imagen fuente.                    |
| srcLeft | Integer | Coordenada X en la fuente.          |
| srcTop | Integer | Coordenada Y en fuente.          |
| alpha | Integer | Valor alfa (`0`-`255`).         |

### Devolver

Sin retorno.

---

## Suika.drawImageSub()

Dibuja una imagen con fusión sustractiva.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------------|
| dstImage | Object | Imagen de destino.               |
| dstLeft | Integer | Coordenada X en destino.     |
| dstTop | Integer | Coordenada Y en destino.     |
| dstWidth | Integer | Ancho para dibujar.                   |
| dstHeight | Integer | Altura para dibujar.                  |
| srcImage | Object | Imagen fuente.                    |
| srcLeft | Integer | Coordenada X en la fuente.          |
| srcTop | Integer | Coordenada Y en fuente.          |
| alpha | Integer | Valor alfa (`0`-`255`).         |

### Devolver

Sin retorno.

---

## Suika.makeColor()

Cree un valor de píxel a partir de componentes RGBA.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|------------------|
| r | Integer | Rojo (0-255).     |
| g | Integer | Verde (0-255).   |
| b | Integer | Azul (0-255).    |
| a | Integer | Alfa (0-255).   |

### Devolver

Un valor de píxel.

---

## Suika.fillImageRect()

Rellena un área rectangular en una imagen con un color.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------------------------|
| image | Object | Imagen de destino.                       |
| left | Integer | Coordenada X.                       |
| top | Integer | Coordenada Y.                       |
| width | Integer | Width. |
| height | Integer | Height. |
| color | Integer | Color creado por Suika.makeColor(). |

### Devolver

Sin retorno.

---

## Suika.reloadStageImages()

Vuelva a cargar las imágenes del escenario mediante la configuración.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.reloadStagePositions()

Vuelva a cargar las posiciones del escenario mediante la configuración.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.getLayerX()

Obtenga la posición actual de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Valor entero de la coordenada.

---

## Suika.getLayerY()

Obtenga la posición actual de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Valor entero de la coordenada.

---

## Suika.setLayerPosition()

Establece la posición de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| x | Integer | Coordenada X.              |
| y | Integer | Coordenada Y.              |

### Devolver

Sin retorno.

---

## Suika.getLayerScaleX()

Obtenga el factor de escala X de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Valor flotante de la escala.

---

## Suika.getLayerScaleY()

Obtenga el factor de escala Y de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Valor flotante de la escala.

---

## Suika.setLayerScale()

Establece el factor de escala de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| scale_x | Float | Escala horizontal.          |
| scale_y | Float | Escala vertical.            |

### Devolver

Sin retorno.

---

## Suika.getLayerRotate()

Obtenga el ángulo de rotación de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Las devoluciones flotan.

---

## Suika.setLayerRotate()

Establece el ángulo de rotación de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| rot | Float | Ángulo de rotación en radianes. |

### Devolver

Sin retorno.

---

## Suika.getLayerDim()

Obtenga el estado de atenuación de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Devuelve booleano.

---

## Suika.setLayerDim()

Establece el estado de atenuación de una capa específica.

### Parámetros (Diccionario) (Establecer)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| dim | Boolean | Ya sea para atenuar la capa.  |

### Devolver

Sin retorno.

---

## Suika.getLayerAlpha()

Consigue la transparencia de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |

### Devolver

Devuelve un número entero.

---

## Suika.setLayerAlpha()

Establece la transparencia de una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| alpha | Integer | Valor alfa (0-255).       |

### Devolver

Sin retorno.

---

## Suika.setLayerBlend()

Establece el modo de fusión para una capa.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| blend | Integer | Modo de fusión (Alfa, Agregar, Sub). |

### Devolver

Sin retorno.

---

## Suika.setLayerFile()

Configure un archivo para que se muestre en una capa.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| file_name | String | Ruta al archivo de imagen.    |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.setLayerFrame()

Establezca el índice de cuadros para el parpadeo y la sincronización de labios.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de la capa del escenario.  |
| frame | Integer | Índice de cuadros.               |

### Devolver

Sin retorno.

---

## Suika.getLayerText()

Obtenga la cadena mostrada en una capa de texto.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la capa de texto.   |

### Devolver

Devuelve una cadena.

---

## Suika.setLayerText()

Establece la cadena que se muestra en una capa de texto.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la capa de texto.   |
| text | String | Mensaje de texto para configurar.       |

### Devolver

Sin retorno.

---

## Suika.getSysBtnIdleImage()

Obtenga la imagen inactiva de sysbtn.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un objeto de imagen.

---

## Suika.getSysBtnHoverImage()

Obtenga la imagen flotante de sysbtn.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un objeto de imagen.

---

## Suika.clearStageBasic()

Limpia las capas básicas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un objeto de imagen.

---

## Suika.clearStage()

Despeja el escenario y conviértelo en estado inicial.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un objeto de imagen.

---

## Suika.chposToLayer()

Convierta la posición de un carácter en un índice de capa de escenario.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| chpos | Integer | Posición del personaje.   |

### Devolver

Devuelve un número entero.

---

## Suika.chposToEyeLayer()

Convierte la posición de un personaje en un índice de capa de escenario (ojo de personaje).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| chpos | Integer | Posición del personaje.   |

### Devolver

Devuelve un número entero.

---

## Suika.chposToLipLayer()

Convierte la posición de un personaje en un índice de capa de escenario (labio del personaje).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| chpos | Integer | Posición del personaje.   |

### Devolver

Devuelve un número entero.

---

## Suika.layerToChpos()

Convierta un índice de capa de escenario en una posición de carácter.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------|
| layer | Integer | Índice de capas.          |

### Devolver

Devuelve un número entero.

---

## Suika.renderStage()

Renderiza el escenario con todas las capas del escenario.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.startFade()

Inicia un efecto de transición.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------------------------|
| desc | Array | Descriptor de desvanecimiento.                             |
| method | String | Método de desvanecimiento.                               |
| time | Float | Duración en segundos.                         |
| ruleImage | Object | Objeto de imagen de regla (opcional).                |

### Devolver

Valor booleano.

---

## Suika.getShakeOffset()

Obtenga el desplazamiento para el comando de agitación.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Un objeto que contiene:
*x
* y

---

## Suika.setShakeOffset()

Establezca el desplazamiento para el comando de agitación.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------|
| x | Integer | Desplazamiento X.      |
| y | Integer | Desplazamiento Y.      |

### Devolver

Sin retorno.

---

## Suika.isFadeRunning()

Compruebe si el desvanecimiento se está ejecutando.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.finishFade()

Termine inmediatamente el efecto de desvanecimiento.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.setChNameMapping()

Especifique un índice de nombre de carácter para una posición de carácter.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-------------|---------|----------------------------|
| chpos | Integer | Posición del personaje.        |
| chNameIndex | Integer | Índice de nombres de personajes.      |

### Devolver

Sin retorno.

---

## Suika.getTalkingChpos()

Obtén la posición del personaje que habla actualmente.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un número entero.

---

## Suika.setChTalking()

Establece el personaje parlante.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| chpos | Integer | Posición del personaje.        |

### Devolver

Sin retorno.

---

## Suika.getTalkingChpos()

Consigue la posición del personaje hablador.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve un número entero.

---

## Suika.updateChDimByTalkingCh()

Actualice automáticamente la atenuación de caracteres según quién esté hablando.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.forceChDim()

Actualice el carácter atenuando manualmente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| chpos | Integer | Posición del personaje.        |
| dim | Boolean | Oscuro o no.                |

### Devolver

Sin retorno.

---

## Suika.getChDim()

Obtenga el estado de atenuación.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| chpos | Integer | Posición del personaje.        |

### Devolver

Devuelve un valor booleano.

---

## Suika.fillNameBox()

Complete el cuadro de nombre con la imagen del cuadro de nombre.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.getNameBoxRect()

Obtenga la posición y el tamaño del cuadro de nombre.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Objeto.

*x
* y
*w
*h

---

## Suika.showNameBox()

Muestra u oculta el cuadro de nombre.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| show | Boolean | Mostrar u ocultar.              |

### Devolver

Sin retorno.

---

## Suika.fillMessageBox()

Complete el cuadro de mensaje con la imagen del cuadro de mensaje.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.showMessageBox()

Mostrar u ocultar el cuadro de mensaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| show | Boolean | Si mostrar la caja.   |

### Devolver

Sin retorno.

---

## Suika.getMessageBoxRect()

Obtenga el cuadro de mensaje correcto.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Un objeto que contiene:
* `x`
* `y`
* `w`
* `h`

---

## Suika.setClickPosition()

Establezca la posición de la animación del clic.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| x | Integer | Posición X.                |
| y | Integer | Posición Y.                |

### Devolver

Sin retorno.

---

## Suika.showClick()

Muestra u oculta la animación del clic.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| show | Boolean | Mostrar u ocultar.              |

### Devolver

Sin retorno.

---

## Suika.setClickIndex()

Establezca el índice del cuadro de animación del clic.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de cuadros.               |

### Devolver

Sin retorno.

---

## Suika.getClickRect()

Obtenga la animación de clic correcta.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Un objeto que contiene:
* `x`
* `y`
* `w`
* `h`

---

## Suika.fillChooseBoxIdleImage()

Rellene una capa inactiva del cuadro de elección con la imagen inactiva del cuadro de elección.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Elija el índice del cuadro.          |

### Devolver

Sin retorno.

---

## Suika.fillChooseBoxHoverImage()

Rellene una capa de selección de cuadro de desplazamiento con la imagen de selección de cuadro de desplazamiento.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Elija el índice del cuadro.          |

### Devolver

Sin retorno.

---

## Suika.showChoosebox()

Mostrar u ocultar un cuadro de elección.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------|---------|-----------------------------|
| index | Integer | Índice del cuadro de elección. (`0`-`7`) |
| showIdle | Boolean | Mostrar estado inactivo.            |
| showHover | Boolean | Mostrar estado de desplazamiento.           |

### Devolver

Sin retorno.

---

## Suika.getChooseBoxRect()

Obtenga el cuadro de elección correcto.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Un objeto que contiene:
* `x`
* `y`
* `w`
* `h`

---

## Suika.showAutoModeBanner()

Muestra u oculta el banner del modo automático.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------|---------|-----------------------------|
| show | Boolean | Mostrar u ocultar.               |

### Devolver

Sin retorno.

---

## Suika.showSkipModeBanner()

Muestra u oculta el banner del modo de omisión.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------|---------|-----------------------------|
| show | Boolean | Mostrar u ocultar.               |

### Devolver

Sin retorno.

---

## Suika.renderImage()

Realice la representación directa de una imagen en la pantalla.

Tenga en cuenta que debería considerar el uso de las capas del escenario para el renderizado normal.
Esta API es útil para efectos.

### Parámetros (Diccionario)

| Parﾃ｡metro | Omisible | Tipo | Descripciﾃｳn |
|-----------|--------------|---------|--------------------------------------------|
| dstLeft | No | Integer | Posición X superior izquierda de destino.           |
| dstTop | No | Integer | Posición Y superior izquierda de destino.           |
| image | No | Object | Image. |
| srcLeft | No | Integer | Fuente de la posición X superior izquierda.                |
| srcTop | No | Integer | Fuente de la posición Y superior izquierda.                |
| srcWidth | No | Integer | Ancho de fuente.                              |
| srcHeight | No | Integer | Altura de la fuente.                             |
| alpha | No | Integer | Valor alfa. (`0`-`255`) |
| blend | No | Integer | Tipo de mezcla.                                |

### Tipos de mezcla

| Nombre | Descripciﾃｳn |
|----------------------|-------------------|
| Suika.BLEND_ALPHA | Mezcla alfa.   |
| Suika.BLEND_ADD | Agrega la mezcla.     |
| Suika.BLEND_SUB | Submezcla.     |

### Devolver

Sin retorno.

---

## Suika.renderImage3d()

Realice una representación directa de una imagen en la pantalla con transformación 3D.

Tenga en cuenta que debería considerar el uso de las capas del escenario para el renderizado normal.
Esta API es útil para efectos.

### Parámetros (Diccionario)

| Parﾃ｡metro | Omisible | Tipo | Descripciﾃｳn |
|-----------|--------------|---------|--------------------------------------------|
| x1 | No | Integer | Vértice de destino 1 posición X.           |
| y1 | No | Integer | Vértice de destino 1 posición Y.           |
| x2 | No | Integer | Vértice de destino 2 posición X.           |
| y2 | No | Integer | Vértice de destino 2 Posición Y.           |
| x3 | No | Integer | Posición del vértice de destino 3 X.           |
| y3 | No | Integer | Vértice de destino 3 Posición Y.           |
| x4 | No | Integer | Posición del vértice de destino 4 X.           |
| y4 | No | Integer | Vértice de destino 4 posición Y.           |
| tex | No | Object | Image. |
| srcLeft | No | Integer | Fuente de la posición X superior izquierda.                |
| srcTop | No | Integer | Fuente de la posición Y superior izquierda.                |
| srcWidth | No | Integer | Ancho de fuente.                              |
| srcHeight | No | Integer | Altura de la fuente.                             |
| alpha | No | Integer | Valor alfa. (`0`-`255`) |

### Devolver

Sin retorno.

---

## Suika.startKirakira()

Inicia el efecto Kirakira.

El efecto Kirakira es una animación que se muestra en la posición de la pantalla donde se hace clic con el cursor del mouse.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.renderKirakira()

Renderiza el efecto Kirakira.

---

## Suika.setMixerInputFile()

Reproduce un archivo de sonido en una pista del mezclador específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Omisible | Tipo | Descripciﾃｳn |
|-----------|--------------|---------|--------------------------------------------|
| track | No | String | Nombre de la pista del mezclador.                          |
| file | No | String | Ruta al archivo de sonido.                    |
| isLooped | Yes(__PH8__) | Boolean | Ya sea para repetir la reproducción.              |

### Nombres de pistas

| Nombre | Descripciﾃｳn |
|--------|--------------------------|
| bgm | Pista de música de fondo.  |
| se | Pista de efectos de sonido.      |
| voice | Pista de voz del personaje.   |
| sys | Pista de sonido del sistema.      |

### Devolver

Booleano que representa si el archivo se abrió correctamente.

---

## Suika.setMixerVolume()

Establezca el volumen para una pista del mezclador específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | String | Nombre de la pista del mezclador.                          |
| vol | Float | Nivel de volumen (0,0 a 1,0).                 |
| span | Float | Duración del desvanecimiento en segundos.                  |

### Nombres de pistas

| Nombre | Descripciﾃｳn |
|--------|--------------------------|
| bgm | Pista de música de fondo.  |
| se | Pista de efectos de sonido.      |
| voice | Pista de voz del personaje.   |
| sys | Pista de sonido del sistema.      |

### Devolver

Sin retorno.

---

## Suika.getMixerVolume()

Obtenga el volumen de una pista del mezclador específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | String | Nombre de la pista del mezclador.                          |
| volume | Float | Nivel de volumen (0,0 a 1,0).                 |
| span | Float | Duración del desvanecimiento en segundos.                  |

### Nombres de pistas

| Nombre | Descripciﾃｳn |
|--------|--------------------------|
| bgm | Pista de música de fondo.  |
| se | Pista de efectos de sonido.      |
| voice | Pista de voz del personaje.   |
| sys | Pista de sonido del sistema.      |

### Devolver

Las devoluciones flotan.

---

## Suika.setMasterVolume()

Establece el volumen maestro que afecta a todas las pistas.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| volume | Float | Nivel de volumen maestro (0,0 a 1,0).          |

### Devolver

Sin retorno.

---

## Suika.getMasterVolume()

Obtén el volumen maestro que afecta a todas las pistas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Las devoluciones flotan.

---

## Suika.setMixerGlobalVolume()

Establece el volumen global de una pista (a menudo se utiliza para ajustes de configuración).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | String | Nombre de la pista del mezclador.                          |
| vol | Float | Nivel de volumen global.                       |

### Nombres de pistas

| Nombre | Descripciﾃｳn |
|--------|--------------------------|
| bgm | Pista de música de fondo.  |
| se | Pista de efectos de sonido.      |
| voice | Pista de voz del personaje.   |
| sys | Pista de sonido del sistema.      |

### Devolver

Sin retorno.

---

## Suika.getMixerGlobalVolume()

Obtenga el volumen global de una pista (a menudo utilizado para ajustes de configuración).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | String | Nombre de la pista del mezclador.                          |

### Nombres de pistas

| Nombre | Descripciﾃｳn |
|--------|--------------------------|
| bgm | Pista de música de fondo.  |
| se | Pista de efectos de sonido.      |
| voice | Pista de voz del personaje.   |
| sys | Pista de sonido del sistema.      |

### Devolver

Las devoluciones flotan.

---

## Suika.setCharacterVolume()

Establece el volumen de la voz de un personaje específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| index | Integer | Índice de nombres de personajes.                      |
| vol | Float | Nivel de volumen.                              |

### Devolver

Sin retorno.

---

## Suika.getCharacterVolume()

Obtén el volumen de la voz de un personaje específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| ch_index | Integer | Índice de nombres de personajes.                      |

### Devolver

Obtenga retornos flotantes.

---

## Suika.isMixerSoundFinished()

Compruebe si ha finalizado la reproducción de una pista específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | Integer | Índice de pistas del mezclador.                         |

### Devolver

Valor booleano.

---

## Suika.getTrackFileName()

Obtenga el nombre del archivo del sonido cargado actualmente en una pista.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| track | Integer | Índice de pistas del mezclador.                         |

### Devolver

Cadena que representa la ruta del archivo.

---

## Suika.applyCharacterVolume()

Aplique la configuración de volumen específica de un personaje a la pista de VOZ.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| ch | Integer | Índice de nombres de personajes.                      |

### Devolver

Sin retorno.

---

## Suika.enableSysBtn()

Controla el botón del sistema.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| isEnabled | Boolean | Habilite el botón del sistema o no.           |

### Devolver

Sin retorno.

---

## Suika.isSysBtnEnabled()

Compruebe si el botón del sistema está habilitado.

### Parámetros

Sin parámetros.

### Devolver

Devuelve un valor booleano.

---

## Suika.updateSysBtnState()

Actualice el seguimiento del mouse para el botón del sistema.

### Parámetros

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isSysBtnPointed()

Compruebe si el botón del sistema apunta.

### Parámetros

Sin parámetros.

### Devolver

Devuelve un valor booleano.

---

## Suika.isSysBtnClicked()

Compruebe si se hace clic en el botón del sistema.

### Parámetros

Sin parámetros.

### Devolver

Devuelve un valor booleano.

---

## Suika.drawTextOnLayer()

Dibuja un texto en una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|--------------|---------|---------------------------|
| layer | Integer | Índice de capa de etapa de destino. |
| fontType | Integer | Índice de selección de fuentes.     |
| fontSize | Integer | Tamaño de la fuente.         |
| color | Integer | Color. |
| outlineWidth | Integer | Ancho del contorno.            |
| outlineColor | Integer | Color del contorno.            |
| lineMargin | Integer | Margen de línea.              |
| charMargin | Integer | Margen de carácter.         |
| x | Integer | Posición X del cuadro delimitador.  |
| y | Integer | Posición Y del cuadro delimitador.  |
| width | Integer | Ancho del cuadro delimitador.       |
| height | Integer | Altura del cuadro delimitador.      |
| text | String | Text. |

### Devolver

Sin retorno.

---

## Suika.getStringWidth()

Obtenga el ancho total de una cadena UTF-8.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|------------------------|
| fontType | Integer | Índice de selección de fuentes.  |
| fontSize | Integer | Tamaño de la fuente.      |
| text | String | Text. |

### Devolver

Valor entero del ancho en píxeles.

---

## Suika.getStringHeight()

Obtenga la altura total de una cadena UTF-8.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|------------------------|
| fontType | Integer | Índice de selección de fuentes.  |
| fontSize | Integer | Tamaño de la fuente.      |
| text | String | Text. |

### Devolver

Valor entero de la altura en píxeles.

---

## Suika.drawGlyph()

Dibuja un solo glifo en una imagen.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|---------------|---------|--------------------------------------------|
| img | Object | Imagen de destino.                              |
| font_type | Integer | Índice de selección de fuentes.                      |
| font_size | Integer | Tamaño de fuente de renderizado.                       |
| base_font_size| Integer | Tamaño de fuente base para métricas.                |
| outline_size | Integer | Ancho del contorno.                      |
| x | Integer | Coordenada X.                              |
| y | Integer | Coordenada Y.                              |
| color | Pixel | Color del texto principal.                           |
| outline_color | Pixel | Color del contorno.                             |
| codepoint | Integer | Punto de código UTF-32.                         |
| is_dim | Boolean | Ya sea para aplicar atenuación.                  |

### Devolver

Booleano que representa el éxito.

---

## Suika.createDrawMsg()

Cree un contexto de dibujo de mensajes complejo para la representación de texto de alto nivel.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|----------------|----------|------------------------|
| image | Integer | Imagen de destino.     |
| text | String | Mensaje para dibujar.       |
| fontType | Integer | Selección de fuentes.        |
| fontSize | Integer | Tamaño de fuente.             |
| baseFontSize | Integer | Tamaño de fuente base.        |
| rubySize | Integer | Tamaño rubí.             |
| outlineSize | Integer | Ancho del contorno.         |
| penX | Integer | Posición del bolígrafo X.        |
| penY | Integer | Posición del bolígrafo Y.        |
| areaWidth | Integer | Dibujar el ancho del área.       |
| areaHeight | Integer | Dibujar la altura del área.      |
| leftMargin | Integer | Margen izquierdo.           |
| rightMargin | Integer | Margen derecho.          |
| topMargin | Integer | Margen superior.            |
| bottomMargin | Integer | Margen inferior.         |
| lineMargin | Integer | Margen de línea.           |
| charMargin | Integer | Margen de carácter.      |
| color | Integer | Color. |
| outlineColor | Integer | Color del contorno.         |
| bgColor | Integer | Color de fondo.      |
| fillBg | Boolean | ¿Rellenar fondo?       |
| dim | Boolean | ¿Oscuro?                   |
| ignoreLF | Boolean | ¿Ignorar LF?             |
| ignoreFont | Boolean | ¿Ignorar el cambio de fuente?    |
| ignoreOutline | Boolean | ¿Ignorar el cambio de esquema? |
| ignoreColor | Boolean | ¿Ignorar el cambio de color?   |
| ignoreSize | Boolean | ¿Ignorar el cambio de tamaño?    |
| ignorePosition | Boolean | ¿Ignorar el cambio de cursor?  |
| ignoreRuby | Boolean | ¿Ignorar a Ruby?           |
| ignoreWait | Boolean | ¿Ignorar la espera en línea?    |
| inlineWaitHook | Function | Gancho de espera en línea.      |
| tategaki | Boolean | ¿Usar tategaki?          |

### Devolver

Un objeto de contexto de dibujo de mensaje.

---

## Suika.destroyDrawMsg()

Destruir un contexto de dibujo de mensaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|----------------|----------|------------------------|
| context | Object | Dibujar el contexto del mensaje.  |

### Devolver

Sin retorno.

---

## Suika.countDrawMsgChars()

Cuente los caracteres restantes excluyendo las secuencias de escape.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|----------------|----------|------------------------|
| context | Object | Dibujar el contexto del mensaje.  |

### Devolver

Devuelve un número entero.

---

## Suika.drawMessage()

Dibuja caracteres en un mensaje hasta (maxChars) caracteres.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|----------------|----------|------------------------|
| context | Object | Dibujar el contexto del mensaje.  |
| maxChars | Integer | Máximo de caracteres.             |

### Devolver

Devuelve un número entero que indica el recuento de caracteres dibujados en la llamada.

---

## Suika.getDrawMsgPenPosition()

Obtenga la posición actual del lápiz a partir de un contexto de dibujo.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| context | Object | Contexto del dibujo.                           |

### Devolver

Un objeto que contiene `x` y `y`.

---

## Suika.isEscapeSequenceChar()

Comprueba si un personaje es parte de una secuencia de escape.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| c | String | Carácter a comprobar.                        |

### Devolver

Valor booleano.

---

## Suika.moveToTagFile()

Cargue un nuevo archivo de etiquetas y mueva el punto de ejecución al principio.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| file | String | Ruta al archivo .novel o script. |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.getTagCount()

Obtenga el número total de etiquetas en el archivo de script actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero que representa el recuento de etiquetas.

---

## Suika.moveToTagIndex()

Mueva el puntero de ejecución a un índice de etiqueta específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de etiquetas de destino. |

### Devolver

Valor booleano.

---

## Suika.moveToNextTag()

Mueva el puntero de ejecución a la siguiente etiqueta.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.moveToLabelTag()

Saltar a una etiqueta específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------|
| name | String | Nombre de la etiqueta de destino.      |

### Devolver

Valor booleano.

---

## Suika.moveToMacroTag()

Salta a una macro específica por nombre.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------|
| name | String | Nombre de la macro de destino.      |

### Devolver

Valor booleano.

---

## Suika.moveToElseTag()

Salta a la etiqueta else/elseif/endif correspondiente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------|
| name | String | Nombre de la macro de destino.      |

### Devolver

Valor booleano.

---

## Suika.moveToEndIfTag()

Salta a la etiqueta endif correspondiente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------|
| name | String | Nombre de la macro de destino.      |

### Devolver

Valor booleano.

---

## Suika.moveToEndMacroTag()

Salta a la etiqueta de macro final correspondiente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|-------------------------|
| name | String | Nombre de la macro de destino.      |

### Devolver

Valor booleano.

---

## Suika.getTagFileName()

Obtenga la etiqueta actual del nombre del archivo de script actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Cadena que representa el nombre del archivo.

---

## Suika.getTagName()

Obtenga el nombre de la etiqueta actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Cadena que representa el nombre de la etiqueta.

---

## Suika.getTagPropertyCount()

Obtenga el número de propiedades de la etiqueta actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Cadena que representa el nombre o valor.

---

## Suika.getTagPropertyName()

Iterar y recuperar las propiedades (argumentos) del actual
etiqueta.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------|
| index | Integer | Índice de propiedad.   |

### Devolver

Cadena que representa el nombre.

---

## Suika.getTagPropertyValue()

Iterar y recuperar las propiedades (argumentos) del actual
etiqueta.

### Parámetros (Diccionario) (para Nombre/Valor de propiedad)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------|
| index | Integer | Índice de propiedad.   |

### Devolver

Cadena que representa el valor.

---

## Suika.getTagArgBool()

Obtenga un argumento específico de la etiqueta actual, compatible con el valor predeterminado
Valores y opcionalidad.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------------------|
| name | String | Nombre del argumento.             |
| omissible | Boolean | Si el argumento es opcional. |
| defVal | Boolean | Valor predeterminado si falta.         |

### Devolver

El valor del argumento en el tipo solicitado.

---

## Suika.getTagArgInt()

Obtenga un argumento específico de la etiqueta actual, compatible con el valor predeterminado
Valores y opcionalidad.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------------------|
| name | String | Nombre del argumento.             |
| omissible | Boolean | Si el argumento es opcional. |
| defVal | Integer | Valor predeterminado si falta.         |

### Devolver

El valor del argumento en el tipo solicitado.

---

## Suika.getTagArgFloat()

Obtenga un argumento específico de la etiqueta actual, compatible con el valor predeterminado
Valores y opcionalidad.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------------------|
| name | String | Nombre del argumento.             |
| omissible | Boolean | Si el argumento es opcional. |
| defVal | Float | Valor predeterminado si falta.         |

### Devolver

El valor del argumento en el tipo solicitado.

---

## Suika.getTagArgString()

Obtenga un argumento específico de la etiqueta actual, compatible con el valor predeterminado
Valores y opcionalidad.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-----------------------------------|
| name | String | Nombre del argumento.             |
| omissible | Boolean | Si el argumento es opcional. |
| defVal | String | Valor predeterminado si falta.         |

### Devolver

El valor del argumento en el tipo solicitado.

---

## Suika.evaluateTag()

Evaluar los valores de propiedad de la etiqueta actual para expandirla en línea
variables. (formulario `${varname}`)

Llamar a esta API actualiza la caché de los valores de propiedad.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.pushTagStackIf()

Administre la pila interna para `[if]` bloques condicionales.

Esta API marca la posición del bloque `if` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.popTagStackIf()

Administre la pila interna para `if` bloques condicionales.

Esta API marca el final del bloque `if` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.pushTagStackWhile()

Administre la pila interna para bucles (`while`).

Esta API marca el bloque `while` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.popTagStackWhile()

Administre la pila interna para bucles (`while`).

Esta API marca el final del bloque `while` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.pushTagStackFor()

Administre la pila interna para bucles (`for`).

Esta API marca el bloque `for` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.popTagStackFor()

Administre la pila interna para bucles (`for`).

Esta API marca el final del bloque `for` para el procesamiento de bloques anidados.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.loadAnimeFromFile()

Cargue una definición de animación desde un archivo y regístrela.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| file | String | Ruta al archivo de anime.                    |
| reg_name | String | Nombre de registro del anime.           |

### Devolver

Una matriz de valores booleanos que indican que cada capa está cargada o no.

---

## Suika.newAnimeSequence()

Comience a describir una nueva secuencia de animación para una capa específica.
Esta API se utiliza para animaciones generadas manualmente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| layer | Integer | Índice de capa de etapa de destino.                  |

### Devolver

Booleano que representa el éxito.

---

## Suika.addAnimeSequencePropertyF()

Agregue una propiedad flotante (por ejemplo, posición, alfa) a la secuencia de anime actual.
Esta API se utiliza para animaciones generadas manualmente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| key | String | Clave de propiedad (por ejemplo, "x", "y", "a").        |
| val | Float | Valor objetivo.                              |

### Devolver

Valor booleano.

---

## Suika.addAnimeSequencePropertyI()

Agregue una propiedad entera (por ejemplo, posición, alfa) a la secuencia de anime actual.
Esta API se utiliza para animaciones generadas manualmente.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| key | String | Clave de propiedad (por ejemplo, "x", "y", "a").        |
| val | Integer | Valor objetivo.                              |

### Devolver

Valor booleano.

---

## Suika.startLayerAnime()

Inicie la secuencia de animación registrada para una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| layer | Integer | Índice de capa de etapa de destino.                  |

### Devolver

Valor booleano.

---

## Suika.isAnimeRunning()

Verifique el estado general de la animación.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isAnimeFinishedForLayer()

Compruebe si la animación de una capa específica ha finalizado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de capa de etapa de destino.  |

### Devolver

Valor booleano.

---

## Suika.updateAnimeFrame()

Actualiza el estado del cuadro de animación. Generalmente se llama una vez por cuadro.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.loadEyeImageIfExists()

Administre imágenes y animaciones que parpadean (parche en el ojo) para la posición de un personaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| chpos | Integer | Posición del personaje (izquierda, centro, etc.).   |
| file | String | Ruta al archivo de imagen del ojo.                |

### Devolver

Valor booleano.

---

## Suika.reloadEyeAnime()

Reinicie la animación de parpadeo (parche) para la posición de un personaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| chpos | Integer | Posición del personaje (izquierda, centro, etc.).   |

### Devolver

Valor booleano.

---

## Suika.runLipAnime()

Inicie una animación de sincronización de labios basada en el contenido del mensaje de un personaje.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| chpos | Integer | Posición del personaje.                        |
| text | String | El texto del mensaje para sincronizar.             |

### Devolver

Sin retorno.

---

## Suika.stopLipAnime()

Detener la animación de sincronización de labios.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| chpos | Integer | Posición del personaje.                        |

### Devolver

Sin retorno.

---

## Suika.clearLayerAnimeSequence()

Borrar secuencias de animación para una capa específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| layer | Integer | Índice de capa de etapa de destino.  |

### Devolver

Sin retorno.

---

## Suika.clearAllAnimeSequence()

Borrar secuencias de animación para todas las capas.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.setVariableInt()

Establezca un valor para una variable local o global.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|------------------------|
| name | String | Nombre de la variable.  |
| value | Integer | Valor a establecer |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.setVariableFloat()

Establezca un valor para una variable local o global.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------------|
| name | String | Nombre de la variable.   |
| value | Float | Valor a establecer |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.setVariableString()

Establezca un valor para una variable local o global.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|-------------------------|
| name | String | Nombre de la variable.   |
| value | String | Valor a establecer |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.getVariableInt()

Obtener el valor actual de una variable.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| name | String | Nombre de la variable.                      |

### Devolver

El valor de la variable en número entero.

---

## Suika.getVariableFloat()

Obtener el valor actual de una variable.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| name | String | Nombre de la variable.                      |

### Devolver

El valor de la variable en float.

---

## Suika.getVariableString()

Obtener el valor actual de una variable.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| name | String | Nombre de la variable.                      |

### Devolver

El valor de la variable en cadena.

---

## Suika.unsetVariable()

Desarmar (eliminar) una variable específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| name | String | Nombre de la variable a desarmar.             |

### Devolver

Sin retorno.

---

## Suika.unsetLocalVariables()

Desarmar (eliminar) todas las variables locales.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.makeVariableGlobal()

Establezca una variable para que sea global (persistente entre guardados).

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| name | String | Nombre de la variable.                      |
| is_global | Boolean | Si hacerlo global.                 |

### Devolver

Valor booleano.

---

## Suika.isGlobalVariable()

Verifique el estado global de la variable.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| name | String | Nombre de la variable.                      |

### Devolver

Valor booleano.

---

## Suika.getVariableCount()

Obtenga el número de variables.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero para contar.

---

## Suika.getVariableName()

Iterar a través de las variables registradas.

### Parámetros (Diccionario) (para getVariableName)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la variable.     |

### Devolver

Cadena para nombre.

---

## Suika.checkVariableExists()

Compruebe si existe una variable con el nombre especificado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| name | String | Nombre para comprobar.                             |

### Devolver

Valor booleano.

---

## Suika.executeSaveGlobal()

Ejecute un guardado global.
Los datos globales normalmente incluyen configuraciones persistentes.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.executeLoadGlobal()

Ejecutar una carga global.
Los datos globales normalmente incluyen configuraciones persistentes.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.executeSaveLocal()

Guarde el progreso del juego en una ranura específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.executeLoadLocal()

Carga el progreso del juego desde una ranura específica.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.checkSaveExists()

Compruebe si existen datos guardados para el índice de ranura especificado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Valor booleano.

---

## Suika.deleteLocalSave()

Elimine una ranura de guardado local.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Sin retorno.

---

## Suika.deleteGlobalSave()

Elimina todos los datos guardados globales.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.checkRightAfterLoad()

Compruebe si el cuadro actual sigue inmediatamente a una operación de carga exitosa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.getSaveTimestamp()

Obtenga la marca de tiempo (hora Unix) cuando se crearon los datos guardados.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Entero (marca de tiempo).

---

## Suika.getLatestSaveIndex()

Obtenga el índice del espacio para guardar actualizado más recientemente.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Número entero que representa el índice de ranura.

---

## Suika.getSaveChapterName()

Recupere el título del capítulo almacenado en una ranura para guardar.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Cadena que representa el nombre del capítulo.

---

## Suika.getSaveLastMessage()

Recupere el último mensaje mostrado almacenado en una ranura para guardar.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Cadena que representa el mensaje.

---

## Suika.getSaveThumbnail()

Obtenga la imagen en miniatura asociada con un espacio para guardar.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice de la ranura para guardar.    |

### Devolver

Un objeto de imagen.

---

## Suika.clearHistory()

Borrar todos los mensajes del historial (atrasos).

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.addHistory()

Añade una nueva entrada al historial.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|------------------|---------|--------------------------------------------|
| name | String | Nombre del personaje.                            |
| msg | String | Texto del mensaje.                              |
| voice | String | Ruta al archivo de voz.                    |
| bodyColor | Integer | Color del cuerpo.                                |
| bodyOutlineColor | Integer | Color del contorno del cuerpo.                        |
| nameColor | Integer | Color del nombre.                                |
| nameOutlineColor | Integer | Color del contorno del nombre.                        |

### Devolver

Booleano que representa el éxito.

---

## Suika.getHistoryCount()

Obtenga el número total de entradas actualmente en el historial.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero que representa el recuento del historial.

---

## Suika.getHistoryName()

Recupera el nombre en un índice de historial específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice en la historia.      |

### Devolver

Valor de cadena.

---

## Suika.getHistoryMessage()

Recupera el mensaje en un índice de historial específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice en la historia.      |

### Devolver

Valor de cadena.

---

## Suika.getHistoryVoice()

Recupera la ruta de voz en un índice de historial específico.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| index | Integer | Índice en la historia.      |

### Devolver

Valor de cadena.

---

## Suika.loadSeen()

Cargue los indicadores vistos (leídos) para el archivo de script actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa el éxito.

---

## Suika.saveSeen()

Guarde los indicadores vistos (leídos) para el archivo de script actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano que representa el éxito.

---

## Suika.getSeenFlags()

Obtenga el estado visto de la etiqueta actual.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Obtener devuelve un número entero.

Para una etiqueta `[text]`, `0` significa no leído y `1` significa leído.

Para una etiqueta `[choose]`, cada bit indica que la opción se seleccionó antes.

---

## Suika.setSeenFlags()

Establezca el estado visto para la etiqueta actual.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|----------------------------|
| flag | Integer | Bandera de estado vista.          |

### Devolver

Sin retorno.

---

## Suika.loadGUIFile()

Cargue un archivo de definición de GUI y prepárelo para su ejecución.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|---------|--------------------------------------------|
| file | String | Ruta al archivo .gui.                     |
| sys | Boolean | Ya sea una GUI del sistema (Guardar/Cargar/etc.). |

### ¿Qué es la GUI del sistema?

La GUI del sistema normalmente se llama cuando se está ejecutando `[text]` o `[choose]`,
y el control volverá a la etiqueta interrumpida.

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.startGUI()

Inicie la GUI cargada.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.stopGUI()

Detenga la GUI actualmente en ejecución.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isGUIRunning()

Compruebe si hay una GUI actualmente activa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.isGUIFinished()

Compruebe si una GUI ha completado su operación.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.getGUIResultLabel()

Obtenga la etiqueta del botón que se seleccionó para finalizar la GUI.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Cadena que representa la etiqueta del resultado.

---

## Suika.isGUIResultTitle()

Compruebe si la GUI se cerró con una acción de "volver al título".

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.checkIfSavedInGUI()

Compruebe si se realizó una operación de guardado mientras la GUI estaba activa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.checkIfLoadedInGUI()

Compruebe si se realizó una operación de carga mientras la GUI estaba activa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.checkRightAfterSysGUI()

Compruebe si el fotograma actual sigue inmediatamente a un retorno desde una GUI del sistema.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Valor booleano.

---

## Suika.getMillisec()

Obtenga un tiempo de vuelta desde el origen del tiempo en milisegundos.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Entero en milisegundos.

---

## Suika.checkFileExists()

Compruebe si existe un archivo.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| file | String | Ruta al archivo.                          |

### Devolver

Devuelve booleano.

---

## Suika.readFileContent()

Leer el contenido completo de un archivo.

### Parámetros (Diccionario) (para readFileContent)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| file | String | Ruta al archivo.                          |

### Devolver

Devuelve una cadena.

---

## Suika.writeSaveData()

Escriba directamente datos guardados sin procesar asociados con una clave.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Clave única para los datos.                   |
| data | String | Datos para escribir/leer.                        |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.readSaveData()

Lea directamente los datos guardados sin procesar asociados con una clave.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| key | String | Clave única para los datos.                   |

### Devolver

Booleano que representa el éxito o el fracaso.

---

## Suika.playVideo()

Controlar la reproducción de vídeo.

### Parámetros (Diccionario) (para reproducirVideo)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|--------------|---------|--------------------------------------|
| file | String | Ruta al archivo de vídeo.              |
| is_skippable | Boolean | Si el usuario puede omitir el video. |

### Devolver

La reproducción devuelve booleano; IsPlaying devuelve booleano.

---

## Suika.stopVideo()

Detenga la reproducción del vídeo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.isVideoPlaying()

Compruebe si se está reproduciendo un vídeo.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Devuelve booleano.

---

## Suika.isFullScreenSupported()

Verifique la capacidad del modo de pantalla completa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Booleano.

---

## Suika.enterFullScreenMode()

Ingrese al modo de pantalla completa.

### Parámetros (Diccionario)

Sin parámetros.

### Devolver

Sin retorno.

---

## Suika.speakText()

Ejecute Texto a voz (TTS) para el mensaje dado.

### Parámetros (Diccionario)

| Parﾃ｡metro | Tipo | Descripciﾃｳn |
|-----------|--------|--------------------------------------------|
| msg | String | Texto a pronunciar.                         |

### Devolver

Sin retorno.
