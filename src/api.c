/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Suika3
 * VM API
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Copyright (c) 2026 The Suika3 Community
 * Copyright (c) 2025-2026 The Playfield Engine Project
 * Copyright (c) 2025-2026 The NoctVM Project
 * Copyright (c) 2025-2026 Awe Morris
 * Copyright (c) 2016-2024 The Suika2 Development Team
 * Copyright (c) 1996-2024 Keiichi Tabata
 *
 * This software is derived from the codebase of Playfield Engine, NoctLang,
 * Suika2, Suika Studio, Wind Game Lib, and 98/AT Game Lib.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#include <suika3/suika3.h>
#include <playfield/playfield.h>
#include <noct/noct.h>
#include "game.h"
#include "image.h"
#include "text.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define NEVER_COME_HERE 0

/* Skeleton */
static bool Suika_start(void *p);
static bool Suika_update(void *p);
static bool Suika_render(void *p);

/* Fundamental */
static bool Suika_loadPlugin(void *p);
static bool Suika_print(void *p);

/* Config */
static bool Suika_setConfig(void *p);
static bool Suika_getConfigCount(void *p);
static bool Suika_getConfigKey(void *p);
static bool Suika_isGlobalSaveConfig(void *p);
static bool Suika_isLocalSaveConfig(void *p);
static bool Suika_getConfigType(void *p);
static bool Suika_getStringConfig(void *p);
static bool Suika_getBoolConfig(void *p);
static bool Suika_getIntConfig(void *p);
static bool Suika_getFloatConfig(void *p);
static bool Suika_getConfigAsString(void *p);
static bool Suika_compareLocale(void *p);

/* Input */
static bool Suika_getMousePosX(void *p);
static bool Suika_getMousePosY(void *p);
static bool Suika_isMouseLeftPressed(void *p);
static bool Suika_isMouseRightPressed(void *p);
static bool Suika_isMouseLeftClicked(void *p);
static bool Suika_isMouseRightClicked(void *p);
static bool Suika_isMouseDragging(void *p);
static bool Suika_isReturnKeyPressed(void *p);
static bool Suika_isSpaceKeyPressed(void *p);
static bool Suika_isEscapeKeyPressed(void *p);
static bool Suika_isUpKeyPressed(void *p);
static bool Suika_isDownKeyPressed(void *p);
static bool Suika_isLeftKeyPressed(void *p);
static bool Suika_isRightKeyPressed(void *p);
static bool Suika_isPageUpKeyPressed(void *p);
static bool Suika_isPageDownKeyPressed(void *p);
static bool Suika_isControlKeyPressed(void *p);
static bool Suika_isSKeyPressed(void *p);
static bool Suika_isLKeyPressed(void *p);
static bool Suika_isHKeyPressed(void *p);
static bool Suika_isTouchCanceled(void *p);
static bool Suika_isSwiped(void *p);
static bool Suika_clearInputState(void *p);

/* Game */
static bool Suika_startCommandRepetition(void *p);
static bool Suika_stopCommandRepetition(void *p);
static bool Suika_isInCommandRepetition(void *p);
static bool Suika_setMessageActive(void *p);
static bool Suika_clearMessageActive(void *p);
static bool Suika_isMessageActive(void *p);
static bool Suika_startAutoMode(void *p);
static bool Suika_stopAutoMode(void *p);
static bool Suika_isAutoMode(void *p);
static bool Suika_startSkipMode(void *p);
static bool Suika_stopSkipMode(void *p);
static bool Suika_isSkipMode(void *p);
static bool Suika_setSaveLoad(void *p);
static bool Suika_isSaveLoadEnabled(void *p);
static bool Suika_setNonInterruptible(void *p);
static bool Suika_isNonInterruptible(void *p);
static bool Suika_setPenPosition(void *p);
static bool Suika_getPenPositionX(void *p);
static bool Suika_getPenPositionY(void *p);
static bool Suika_pushForCall(void *p);
static bool Suika_popForReturn(void *p);
static bool Suika_readCallStack(void *p);
static bool Suika_writeCallStack(void *p);
static bool Suika_setCallArgument(void *p);
static bool Suika_getCallArgument(void *p);
static bool Suika_isPageMode(void *p);
static bool Suika_appendBufferedMessage(void *p);
static bool Suika_getBufferedMessage(void *p);
static bool Suika_clearBufferedMessage(void *p);
static bool Suika_resetPageLine(void *p);
static bool Suika_incPageLine(void *p);
static bool Suika_isPageTop(void *p);
static bool Suika_registerBGVoice(void *p);
static bool Suika_getBGVoice(void *p);
static bool Suika_setBGVoicePlaying(void *p);
static bool Suika_isBGVoicePlaying(void *p);
static bool Suika_setChapterName(void *p);
static bool Suika_getChapterName(void *p);
static bool Suika_setLastMessage(void *p);
static bool Suika_setPrevLastMessage(void *p);
static bool Suika_getLastMessage(void *p);
static bool Suika_getPrevLastMessage(void *p);
static bool Suika_setTextSpeed(void *p);
static bool Suika_getTextSpeed(void *p);
static bool Suika_setAutoSpeed(void *p);
static bool Suika_getAutoSpeed(void *p);
static bool Suika_markLastEnglishTagIndex(void *p);
static bool Suika_getLastEnglishTagIndex(void *p);
static bool Suika_clearLastEnglishTagIndex(void *p);
static bool Suika_getLastTagName(void *p);

/* Image */
static bool Suika_createImageFromFile(void *p);
static bool Suika_createImage(void *p);
static bool Suika_getImageWidth(void *p);
static bool Suika_getImageHeight(void *p);
static bool Suika_loadGlyphImage(void *p);
static bool Suika_destroyImage(void *p);
static bool Suika_notifyImageUpdate(void *p);
static bool Suika_drawImage(void *p);
static bool Suika_drawImage3D(void *p);
static bool Suika_makeColor(void *p);
static bool Suika_fillImageRect(void *p);

/* Stage */
static bool Suika_reloadStageImages(void *p);
static bool Suika_reloadStagePositions(void *p);
static bool Suika_getLayerX(void *p);
static bool Suika_getLayerY(void *p);
static bool Suika_setLayerPosition(void *p);
static bool Suika_getLayerScaleX(void *p);
static bool Suika_getLayerScaleY(void *p);
static bool Suika_setLayerScale(void *p);
static bool Suika_getLayerCenterX(void *p);
static bool Suika_getLayerCenterY(void *p);
static bool Suika_setLayerCenter(void *p);
static bool Suika_getLayerRotate(void *p);
static bool Suika_setLayerRotate(void *p);
static bool Suika_getLayerDim(void *p);
static bool Suika_setLayerDim(void *p);
static bool Suika_getLayerWidth(void *p);
static bool Suika_getLayerHeight(void *p);
static bool Suika_getLayerAlpha(void *p);
static bool Suika_setLayerAlpha(void *p);
static bool Suika_getLayerBlend(void *p);
static bool Suika_setLayerBlend(void *p);
static bool Suika_getLayerFile(void *p);
static bool Suika_setLayerFile(void *p);
static bool Suika_getLayerImage(void *p);
static bool Suika_setLayerImage(void *p);
static bool Suika_getLayerFrame(void *p);
static bool Suika_setLayerFrame(void *p);
static bool Suika_getLayerText(void *p);
static bool Suika_setLayerText(void *p);
static bool Suika_getSysBtnIdleImage(void *p);
static bool Suika_getSysBtnHoverImage(void *p);
static bool Suika_clearStageBasic(void *p);
static bool Suika_clearStage(void *p);
static bool Suika_chposToLayer(void *p);
static bool Suika_chposToEyeLayer(void *p);
static bool Suika_chposToLipLayer(void *p);
static bool Suika_layerToChpos(void *p);
static bool Suika_renderStage(void *p);
static bool Suika_drawStageToThumb(void *p);
static bool Suika_getThumbImage(void *p);
static bool Suika_getFadeMethod(void *p);
static bool Suika_getAccelMethod(void *p);
static bool Suika_startFade(void *p);
static bool Suika_getShakeOffset(void *p);
static bool Suika_setShakeOffset(void *p);
static bool Suika_isFadeRunning(void *p);
static bool Suika_finishFade(void *p);
static bool Suika_setChNameMapping(void *p);
static bool Suika_getChTalking(void *p);
static bool Suika_setChTalking(void *p);
static bool Suika_getTalkingChpos(void *p);
static bool Suika_updateChDimByTalkingCh(void *p);
static bool Suika_forceChDim(void *p);
static bool Suika_getChDim(void *p);
static bool Suika_fillNameBox(void *p);
static bool Suika_getNameBoxRect(void *p);
static bool Suika_showNameBox(void *p);
static bool Suika_fillMessageBox(void *p);
static bool Suika_showMessageBox(void *p);
static bool Suika_getMessageBoxRect(void *p);
static bool Suika_setClickPosition(void *p);
static bool Suika_showClick(void *p);
static bool Suika_setClickIndex(void *p);
static bool Suika_getClickRect(void *p);
static bool Suika_fillChooseBoxIdleImage(void *p);
static bool Suika_fillChooseBoxHoverImage(void *p);
static bool Suika_showChooseBox(void *p);
static bool Suika_getChooseBoxRect(void *p);
static bool Suika_showAutoModeBanner(void *p);
static bool Suika_showSkipModeBanner(void *p);
static bool Suika_renderImage(void *p);
static bool Suika_renderImage3d(void *p);

/* Mixer */
static bool Suika_setMixerInputFile(void *p);
static bool Suika_setMixerVolume(void *p);
static bool Suika_getMixerVolume(void *p);
static bool Suika_setMasterVolume(void *p);
static bool Suika_getMasterVolume(void *p);
static bool Suika_setMixerGlobalVolume(void *p);
static bool Suika_getMixerGlobalVolume(void *p);
static bool Suika_setCharacterVolume(void *p);
static bool Suika_getCharacterVolume(void *p);
static bool Suika_isMixerSoundFinished(void *p);
static bool Suika_getTrackFile(void *p);
static bool Suika_applyCharacterVolume(void *p);

/* System */
static bool Suika_enableSysBtn(void *p);
static bool Suika_isSysBtnEnabled(void *p);
static bool Suika_updateSysBtnState(void *p);
static bool Suika_isSysBtnPointed(void *p);
static bool Suika_isSysBtnClicked(void *p);

/* Text */
static bool Suika_getStringWidth(void *p);
static bool Suika_getStringHeight(void *p);
static bool Suika_drawGlyph(void *p);
static bool Suika_createDrawMsg(void *p);
static bool Suika_destroyDrawMsg(void *p);
static bool Suika_countDrawMsgChars(void *p);
static bool Suika_drawMessage(void *p);
static bool Suika_getDrawMsgPenPosition(void *p);
static bool Suika_setDrawMsgIgnoreInlineWait(void *p);
static bool Suika_isQuotedSerif(void *p);
static bool Suika_isEscapeSequenceChar(void *p);

/* Tag */
static bool Suika_getTagCount(void *p);
static bool Suika_moveToTagFile(void *p);
static bool Suika_moveToTagIndex(void *p);
static bool Suika_moveToNextTag(void *p);
static bool Suika_moveToLabelTag(void *p);
static bool Suika_moveToMacroTag(void *p);
static bool Suika_moveToElseTag(void *p);
static bool Suika_moveToEndIfTag(void *p);
static bool Suika_moveToEndMacroTag(void *p);
static bool Suika_getTagFile(void *p);
static bool Suika_getTagIndex(void *p);
static bool Suika_getTagLine(void *p);
static bool Suika_getTagName(void *p);
static bool Suika_getTagPropertyCount(void *p);
static bool Suika_getTagPropertyName(void *p);
static bool Suika_getTagPropertyValue(void *p);
static bool Suika_checkTagArg(void *p);
static bool Suika_getTagArgBool(void *p);
static bool Suika_getTagArgInt(void *p);
static bool Suika_getTagArgFloat(void *p);
static bool Suika_getTagArgString(void *p);
static bool Suika_evaluateTag(void *p);
static bool Suika_pushTagStackIf(void *p);
static bool Suika_popTagStackIf(void *p);
static bool Suika_pushTagStackWhile(void *p);
static bool Suika_popTagStackWhile(void *p);
static bool Suika_pushTagStackFor(void *p);
static bool Suika_popTagStackFor(void *p);

/* Anime */
static bool Suika_loadAnimeFromFile(void *p);
static bool Suika_clearLayerAnimeSequence(void *p);
static bool Suika_clearAllAnimeSequence(void *p);
static bool Suika_newAnimeSequence(void *p);
static bool Suika_addAnimeSequencePropertyF(void *p);
static bool Suika_addAnimeSequencePropertyI(void *p);
static bool Suika_startLayerAnime(void *p);
static bool Suika_isAnimeRunning(void *p);
static bool Suika_isAnimeRunningWithLayerMask(void *p);
static bool Suika_isAnimeFinishedForLayer(void *p);
static bool Suika_updateAnimeFrame(void *p);
static bool Suika_unregisterAnime(void *p);
static bool Suika_getRegAnimeName(void *p);
static bool Suika_getRegAnimeFile(void *p);
static bool Suika_loadEyeImageIfExists(void *p);
static bool Suika_reloadEyeAnime(void *p);
static bool Suika_loadLipImageIfExists(void *p);
static bool Suika_runLipAnime(void *p);
static bool Suika_stopLipAnime(void *p);

/* Variable */
static bool Suika_setVariableInt(void *p);
static bool Suika_setVariableFloat(void *p);
static bool Suika_setVariableString(void *p);
static bool Suika_unsetVariable(void *p);
static bool Suika_makeVariableGlobal(void *p);
static bool Suika_getVariableInt(void *p);
static bool Suika_getVariableFloat(void *p);
static bool Suika_getVariableString(void *p);
static bool Suika_getVariableCount(void *p);
static bool Suika_getVariableName(void *p);
static bool Suika_checkVariableExists(void *p);
static bool Suika_isGlobalVariable(void *p);
static bool Suika_unsetLocalVariables(void *p);

/* Save */
static bool Suika_executeSaveGlobal(void *p);
static bool Suika_executeLoadGlobal(void *p);
static bool Suika_executeSaveLocal(void *p);
static bool Suika_executeLoadLocal(void *p);
static bool Suika_checkSaveExists(void *p);
static bool Suika_deleteLocalSave(void *p);
static bool Suika_deleteGlobalSave(void *p);
static bool Suika_checkRightAfterLoad(void *p);
static bool Suika_getSaveTimestamp(void *p);
static bool Suika_getLatestSaveIndex(void *p);
static bool Suika_getSaveChapterName(void *p);
static bool Suika_getSaveLastMessage(void *p);
static bool Suika_getSaveThumbnail(void *p);

/* History  */
static bool Suika_clearHistory(void *p);
static bool Suika_addHistory(void *p);
static bool Suika_getHistoryCount(void *p);
static bool Suika_getHistoryName(void *p);
static bool Suika_getHistoryMessage(void *p);
static bool Suika_getHistoryVoice(void *p);

/* Seen */
static bool Suika_loadSeen(void *p);
static bool Suika_saveSeen(void *p);
static bool Suika_getSeenFlags(void *p);
static bool Suika_setSeenFlags(void *p);

/* GUI */
static bool Suika_checkRightAfterSysGUI(void *p);
static bool Suika_loadGUIFile(void *p);
static bool Suika_startGUI(void *p);
static bool Suika_stopGUI(void *p);
static bool Suika_isGUIRunning(void *p);
static bool Suika_isGUIFinished(void *p);
static bool Suika_getGUIResultLabel(void *p);
static bool Suika_isGUIResultTitle(void *p);
static bool Suika_checkIfSavedInGUI(void *p);
static bool Suika_checkIfLoadedInGUI(void *p);

/* HAL */
static bool Suika_getMillisec(void *p);
static bool Suika_checkFileExists(void *p);
static bool Suika_readFileContent(void *p);
static bool Suika_writeSaveData(void *p);
static bool Suika_readSaveData(void *p);
static bool Suika_playVideo(void *p);
static bool Suika_stopVideo(void *p);
static bool Suika_isVideoPlaying(void *p);
static bool Suika_speakText(void *p);

/*
 * API function table.
 */

/*
 * Single `dict` parameter pattern.
 * Parameters are stored to a single parameter `dict`,
 * and accessed via pf_get_call_arg_*().
 */
static const char *dict_param[] = {"dict"};

/* Single `msg` parameter. */
static const char *msg_param[] = {"msg"};

/* Single `name` parameter. */
static const char *name_param[] = {"name"};

/*
 * API table.
 *  - All non-implemented APIs **should** be also listed here.
 */
struct api_func {
	const char *name;
	bool (*func)(void *);
	uint32_t param_count;
	const char **params;
};
static struct api_func api_func[] = {
	/* Skeleton API */
	{"start", Suika_start, 0, NULL},
	{"update", Suika_update, 0, NULL},
	{"render", Suika_render, 0, NULL},

	/* Fundamental API */
	{"print", Suika_print, 1, msg_param},
	{"loadPlugin", Suika_loadPlugin, 1, name_param},

	/* Config */
	{"setConfig",			Suika_setConfig,		1, dict_param},
	{"getConfigCount",		Suika_getConfigCount,		0, NULL},
	{"getConfigKey",		Suika_getConfigKey,		1, dict_param},
	{"isGlobalSaveConfig",		Suika_isGlobalSaveConfig,	1, dict_param},
	{"isLocalSaveConfig",		Suika_isLocalSaveConfig,	1, dict_param},
	{"getConfigType",		Suika_getConfigType,		1, dict_param},
	{"getBoolConfig",		Suika_getBoolConfig,		1, dict_param},
	{"getIntConfig",		Suika_getIntConfig,		1, dict_param},
	{"getFloatConfig",		Suika_getFloatConfig,		1, dict_param},
	{"getStringConfig",		Suika_getStringConfig,		1, dict_param},
	{"getConfigAsString",		Suika_getConfigAsString,	1, dict_param},
	{"compareLocale",		Suika_compareLocale,		1, dict_param},

	/* Input */
	{"getMousePosX",		Suika_getMousePosX,		0, NULL},
	{"getMousePosY",		Suika_getMousePosY,		0, NULL},
	{"isMouseLeftPressed",		Suika_isMouseLeftPressed,	0, NULL},
	{"isMouseRightPressed",		Suika_isMouseRightPressed,	0, NULL},
	{"isMouseLeftClicked",		Suika_isMouseLeftClicked,	0, NULL},
	{"isMouseRightClicked",		Suika_isMouseRightClicked,	0, NULL},
	{"isMouseDragging",		Suika_isMouseDragging,		0, NULL},
	{"isReturnKeyPressed",		Suika_isReturnKeyPressed,	0, NULL},
	{"isSpaceKeyPressed",		Suika_isSpaceKeyPressed,	0, NULL},
	{"isEscapeKeyPressed",		Suika_isEscapeKeyPressed,	0, NULL},
	{"isUpKeyPressed",		Suika_isUpKeyPressed,		0, NULL},
	{"isDownKeyPressed",		Suika_isDownKeyPressed,		0, NULL},
	{"isLeftKeyPressed",		Suika_isLeftKeyPressed,		0, NULL},
	{"isRightKeyPressed",		Suika_isRightKeyPressed,	0, NULL},
	{"isPageUpKeyPressed",		Suika_isPageUpKeyPressed,	0, NULL},
	{"isPageDownKeyPressed",	Suika_isPageDownKeyPressed,	0, NULL},
	{"isControlKeyPressed",		Suika_isControlKeyPressed,	0, NULL},
	{"isSKeyPressed",		Suika_isSKeyPressed,		0, NULL},
	{"isLKeyPressed",		Suika_isLKeyPressed,		0, NULL},
	{"isHKeyPressed",		Suika_isHKeyPressed,		0, NULL},
	{"isTouchCanceled",		Suika_isTouchCanceled,		0, NULL},
	{"isSwiped",			Suika_isSwiped,			0, NULL},
	{"clearInputState",		Suika_clearInputState,		0, NULL},

	/* Game */
	{"startCommandRepetition",	Suika_startCommandRepetition,	0, NULL},
	{"stopCommandRepetition",	Suika_stopCommandRepetition,	0, NULL},
	{"isInCommandRepetition",	Suika_isInCommandRepetition,	0, NULL},
	{"setMessageActive",		Suika_setMessageActive,		0, NULL},
	{"clearMessageActive",		Suika_clearMessageActive,	0, NULL},
	{"isMessageActive",		Suika_isMessageActive,		0, NULL},
	{"startAutoMode",		Suika_startAutoMode,		0, NULL},
	{"stopAutoMode",		Suika_stopAutoMode,		0, NULL},
	{"isAutoMode",			Suika_isAutoMode,		0, NULL},
	{"startSkipMode",		Suika_startSkipMode,		0, NULL},
	{"stopSkipMode",		Suika_stopSkipMode,		0, NULL},
	{"isSkipMode",			Suika_isSkipMode,		0, NULL},
	{"setSaveLoad",			Suika_setSaveLoad,		1, dict_param},
	{"isSaveLoadEnabled",		Suika_isSaveLoadEnabled,	0, NULL},
	{"setNonInterruptible",		Suika_setNonInterruptible,	1, dict_param},
	{"isNonInterruptible",		Suika_isNonInterruptible,	0, NULL},
	{"setPenPosition",		Suika_setPenPosition,		1, dict_param},
	{"getPenPositionX",		Suika_getPenPositionX,		0, NULL},
	{"getPenPositionY",		Suika_getPenPositionY,		0, NULL},
	{"pushForCall",			Suika_pushForCall,		1, dict_param},
	{"popForReturn",		Suika_popForReturn,		0, NULL},
	{"readCallStack",		Suika_readCallStack,		1, dict_param},
	{"writeCallStack",		Suika_writeCallStack,		1, dict_param},
	{"setCallArgument",		Suika_setCallArgument,		1, dict_param},
	{"getCallArgument",		Suika_getCallArgument,		1, dict_param},
	{"isPageMode",			Suika_isPageMode,		0, NULL},
	{"appendBufferedMessage",	Suika_appendBufferedMessage,	1, dict_param},
	{"getBufferedMessage",		Suika_getBufferedMessage,	0, NULL},
	{"clearBufferedMessage",	Suika_clearBufferedMessage,	0, NULL},
	{"resetPageLine",		Suika_resetPageLine,		0, NULL},
	{"incPageLine",			Suika_incPageLine,		0, NULL},
	{"isPageTop",			Suika_isPageTop,		0, NULL},
	{"registerBGVoice",		Suika_registerBGVoice,		1, dict_param},
	{"getBGVoice",			Suika_getBGVoice,		0, NULL},
	{"setBGVoicePlaying",		Suika_setBGVoicePlaying,	1, dict_param},
	{"isBGVoicePlaying",		Suika_isBGVoicePlaying,		0, NULL},
	{"setChapterName",		Suika_setChapterName,		1, dict_param},
	{"getChapterName",		Suika_getChapterName,		0, NULL},
	{"setLastMessage",		Suika_setLastMessage,		1, dict_param},
	{"setPrevLastMessage",		Suika_setPrevLastMessage,	1, dict_param},
	{"getLastMessage",		Suika_getLastMessage,		0, NULL},
	{"getPrevLastMessage",		Suika_getPrevLastMessage,	0, NULL},
	{"setTextSpeed",		Suika_setTextSpeed,		1, dict_param},
	{"getTextSpeed",		Suika_getTextSpeed,		0, NULL},
	{"setAutoSpeed",		Suika_setAutoSpeed,		1, dict_param},
	{"getAutoSpeed",		Suika_getAutoSpeed,		0, NULL},
	{"markLastEnglishTagIndex",	Suika_markLastEnglishTagIndex,	0, NULL},
	{"getLastEnglishTagIndex",	Suika_getLastEnglishTagIndex,	0, NULL},
	{"clearLastEnglishTagIndex",	Suika_clearLastEnglishTagIndex,	0, NULL},
	{"getLastTagName",		Suika_getLastTagName,		0, NULL},

	/* Image */
	{"createImageFromFile",		Suika_createImageFromFile,	1, dict_param},
	{"createImage",			Suika_createImage,		1, dict_param},
	{"getImageWidth",		Suika_getImageWidth,		1, dict_param},
	{"getImageHeight",		Suika_getImageHeight,		1, dict_param},
	{"loadGlyphImage",		Suika_loadGlyphImage,		1, dict_param},
	{"destroyImage",		Suika_destroyImage,		1, dict_param},
	{"notifyImageUpdate",		Suika_notifyImageUpdate,	1, dict_param},
	{"drawImage",			Suika_drawImage,		1, dict_param},
	{"drawImage3D",			Suika_drawImage3D,		1, dict_param},
	{"makeColor",			Suika_makeColor,		1, dict_param},
	{"fillImageRect",		Suika_fillImageRect,		1, dict_param},

	/* Stage */
	{"reloadStageImages",		Suika_reloadStageImages,	0, NULL},
	{"reloadStagePositions",	Suika_reloadStagePositions,	0, NULL},
	{"getLayerX",			Suika_getLayerX,		1, dict_param},
	{"getLayerY",			Suika_getLayerY,		1, dict_param},
	{"setLayerPosition",		Suika_setLayerPosition,		1, dict_param},
	{"getLayerScaleX",		Suika_getLayerScaleX,		1, dict_param},
	{"getLayerScaleY",		Suika_getLayerScaleY,		1, dict_param},
	{"setLayerScale",		Suika_setLayerScale,		1, dict_param},
	{"getLayerCenterX",		Suika_getLayerCenterX,		1, dict_param},
	{"getLayerCenterY",		Suika_getLayerCenterY,		1, dict_param},
	{"setLayerCenter",		Suika_setLayerCenter,		1, dict_param},
	{"getLayerRotate",		Suika_getLayerRotate,		1, dict_param},
	{"setLayerRotate",		Suika_setLayerRotate,		1, dict_param},
	{"getLayerDim",			Suika_getLayerDim,		1, dict_param},
	{"setLayerDim",			Suika_setLayerDim,		1, dict_param},
	{"getLayerWidth",		Suika_getLayerWidth,		1, dict_param},
	{"getLayerHeight",		Suika_getLayerHeight,		1, dict_param},
	{"getLayerAlpha",		Suika_getLayerAlpha,		1, dict_param},
	{"setLayerAlpha",		Suika_setLayerAlpha,		1, dict_param},
	{"getLayerBlend",		Suika_getLayerBlend,		1, dict_param},
	{"setLayerBlend",		Suika_setLayerBlend,		1, dict_param},
	{"getLayerFile",		Suika_getLayerFile,		1, dict_param},
	{"setLayerFile",		Suika_setLayerFile,		1, dict_param},
	{"getLayerImage",		Suika_getLayerImage,		1, dict_param},
	{"setLayerImage",		Suika_setLayerImage,		1, dict_param},
	{"getLayerFrame",		Suika_getLayerFrame,		1, dict_param},
	{"setLayerFrame",		Suika_setLayerFrame,		1, dict_param},
	{"getLayerText",		Suika_getLayerText,		1, dict_param},
	{"setLayerText",		Suika_setLayerText,		1, dict_param},
	{"getSysBtnIdleImage",		Suika_getSysBtnIdleImage,	0, NULL},
	{"getSysBtnHoverImage",		Suika_getSysBtnHoverImage,	0, NULL},
	{"clearStageBasic",		Suika_clearStageBasic,		0, NULL},
	{"clearStage",			Suika_clearStage,		0, NULL},
	{"chposToLayer",		Suika_chposToLayer,		1, dict_param},
	{"chposToEyeLayer",		Suika_chposToEyeLayer,		1, dict_param},
	{"chposToLipLayer",		Suika_chposToLipLayer,		1, dict_param},
	{"layerToChpos",		Suika_layerToChpos,		1, dict_param},
	{"renderStage",			Suika_renderStage,		0, NULL},
	{"drawStageToThumb",		Suika_drawStageToThumb,		0, NULL},
	{"getThumbImage",		Suika_getThumbImage,		0, NULL},
	{"getFadeMethod",		Suika_getFadeMethod,		1, dict_param},
	{"getAccelMethod",		Suika_getAccelMethod,		1, dict_param},
	{"startFade",			Suika_startFade,		1, dict_param},
	{"getShakeOffset",		Suika_getShakeOffset,		1, dict_param},
	{"setShakeOffset",		Suika_setShakeOffset,		1, dict_param},
	{"isFadeRunning",		Suika_isFadeRunning,		0, NULL},
	{"finishFade",			Suika_finishFade,		0, NULL},
	{"setChNameMapping",		Suika_setChNameMapping,		1, dict_param},
	{"getChTalking",		Suika_getChTalking,		0, NULL},
	{"setChTalking",		Suika_setChTalking,		1, dict_param},
	{"getTalkingChpos",		Suika_getTalkingChpos,		0, NULL},
	{"updateChDimByTalkingCh",	Suika_updateChDimByTalkingCh,	0, NULL},
	{"forceChDim",			Suika_forceChDim,		1, dict_param},
	{"getChDim",			Suika_getChDim,			1, dict_param},
	{"fillNameBox",			Suika_fillNameBox,		0, NULL},
	{"getNameBoxRect",		Suika_getNameBoxRect,		0, NULL},
	{"showNameBox",			Suika_showNameBox,		1, dict_param},
	{"fillMessageBox",		Suika_fillMessageBox,		0, NULL},
	{"showMessageBox",		Suika_showMessageBox,		1, dict_param},
	{"getMessageBoxRect",		Suika_getMessageBoxRect,	0, NULL},
	{"setClickPosition",		Suika_setClickPosition,		1, dict_param},
	{"showClick",			Suika_showClick,		1, dict_param},
	{"setClickIndex",		Suika_setClickIndex,		1, dict_param},
	{"getClickRect",		Suika_getClickRect,		0, NULL},
	{"fillChooseBoxIdleImage",	Suika_fillChooseBoxIdleImage,	1, dict_param},
	{"fillChooseBoxHoverImage",	Suika_fillChooseBoxHoverImage,	1, dict_param},
	{"showChooseBox",		Suika_showChooseBox,		1, dict_param},
	{"getChooseBoxRect",		Suika_getChooseBoxRect,		1, dict_param},
	{"showAutoModeBanner",		Suika_showAutoModeBanner,	1, dict_param},
	{"showSkipModeBanner",		Suika_showSkipModeBanner,	1, dict_param},
	{"renderImage",			Suika_renderImage,		1, dict_param},
	{"renderImage3d",		Suika_renderImage3d,		1, dict_param},

	/* Mixer */
	{"setMixerInputFile",		Suika_setMixerInputFile,	1, dict_param},
	{"setMixerVolume",		Suika_setMixerVolume,		1, dict_param},
	{"getMixerVolume",		Suika_getMixerVolume,		1, dict_param},
	{"setMasterVolume",		Suika_setMasterVolume,		1, dict_param},
	{"getMasterVolume",		Suika_getMasterVolume,		0, NULL},
	{"setMixerGlobalVolume",	Suika_setMixerGlobalVolume,	1, dict_param},
	{"getMixerGlobalVolume",	Suika_getMixerGlobalVolume,	1, dict_param},
	{"setCharacterVolume",		Suika_setCharacterVolume,	1, dict_param},
	{"getCharacterVolume",		Suika_getCharacterVolume,	1, dict_param},
	{"isMixerSoundFinished",	Suika_isMixerSoundFinished,	1, dict_param},
	{"getTrackFile",		Suika_getTrackFile,		1, dict_param},
	{"applyCharacterVolume",	Suika_applyCharacterVolume,	1, dict_param},

	/* System */
	{"enableSysBtn",		Suika_enableSysBtn,		1, dict_param},
	{"isSysBtnEnabled",		Suika_isSysBtnEnabled,		0, NULL},
	{"updateSysBtnState",		Suika_updateSysBtnState,	0, NULL},
	{"isSysBtnPointed",		Suika_isSysBtnPointed,		0, NULL},
	{"isSysBtnClicked",		Suika_isSysBtnClicked,		0, NULL},

	/* Text */
	{"getStringWidth",		Suika_getStringWidth,		1, dict_param},
	{"getStringHeight",		Suika_getStringHeight,		1, dict_param},
	{"drawGlyph",			Suika_drawGlyph,		1, dict_param},
	{"createDrawMsg",		Suika_createDrawMsg,		1, dict_param}, 
	{"destroyDrawMsg",		Suika_destroyDrawMsg,		1, dict_param}, 
	{"countDrawMsgChars",		Suika_countDrawMsgChars,	1, dict_param},
	{"drawMessage",			Suika_drawMessage,		1, dict_param},
	{"getDrawMsgPenPosition",	Suika_getDrawMsgPenPosition,	1, dict_param},
	{"setDrawMsgIgnoreInlineWait",	Suika_setDrawMsgIgnoreInlineWait, 1, dict_param},
	{"isQuotedSerif",		Suika_isQuotedSerif,		1, dict_param},
	{"isEscapeSequenceChar",	Suika_isEscapeSequenceChar,	1, dict_param},

	/* Tag */
	{"getTagCount",			Suika_getTagCount,		0, NULL},
	{"moveToTagFile",		Suika_moveToTagFile,		1, dict_param},
	{"moveToTagIndex",		Suika_moveToTagIndex,		1, dict_param},
	{"moveToNextTag",		Suika_moveToNextTag,		0, NULL},
	{"moveToLabelTag",		Suika_moveToLabelTag,		1, dict_param},
	{"moveToMacroTag",		Suika_moveToMacroTag,		1, dict_param},
	{"moveToElseTag",		Suika_moveToElseTag,		0, NULL},
	{"moveToEndIfTag",		Suika_moveToEndIfTag,		0, NULL},
	{"moveToEndMacroTag",		Suika_moveToEndMacroTag,	0, NULL},
	{"getTagFile",			Suika_getTagFile,		0, NULL},
	{"getTagIndex",			Suika_getTagIndex,		0, NULL},
	{"getTagLine",			Suika_getTagLine,		0, NULL},
	{"getTagName",			Suika_getTagName,		0, NULL},
	{"getTagPropertyCount",		Suika_getTagPropertyCount,	0, NULL},
	{"getTagPropertyName",		Suika_getTagPropertyName,	1, dict_param},
	{"getTagPropertyValue",		Suika_getTagPropertyValue,	1, dict_param},
	{"checkTagArg",			Suika_checkTagArg,		1, dict_param},
	{"getTagArgBool",		Suika_getTagArgBool,		1, dict_param},
	{"getTagArgInt",		Suika_getTagArgInt,		1, dict_param},
	{"getTagArgFloat",		Suika_getTagArgFloat,		1, dict_param},
	{"getTagArgString",		Suika_getTagArgString,		1, dict_param},
	{"evaluateTag",			Suika_evaluateTag,		0, NULL},
	{"pushTagStackIf",		Suika_pushTagStackIf,		0, NULL},
	{"popTagStackIf",		Suika_popTagStackIf,		0, NULL},
	{"pushTagStackWhile",		Suika_pushTagStackWhile,	0, NULL},
	{"popTagStackWhile",		Suika_popTagStackWhile,		0, NULL},
	{"pushTagStackFor",		Suika_pushTagStackFor,		0, NULL},
	{"popTagStackFor",		Suika_popTagStackFor,		0, NULL},

	/* Anime */
	{"loadAnimeFromFile",		Suika_loadAnimeFromFile,		1, dict_param},
	{"clearLayerAnimeSequence",	Suika_clearLayerAnimeSequence,		1, dict_param},
	{"clearAllAnimeSequence",	Suika_clearAllAnimeSequence,		0, NULL},
	{"newAnimeSequence",		Suika_newAnimeSequence,			1, dict_param},
	{"addAnimeSequencePropertyF",	Suika_addAnimeSequencePropertyF,	1, dict_param},
	{"addAnimeSequencePropertyI",	Suika_addAnimeSequencePropertyI,	1, dict_param},
	{"startLayerAnime",		Suika_startLayerAnime,			1, dict_param},
	{"isAnimeRunning",		Suika_isAnimeRunning,			0, NULL},
	{"isAnimeRunningWithLayerMask",	Suika_isAnimeRunningWithLayerMask,	1, dict_param},
	{"isAnimeFinishedForLayer",	Suika_isAnimeFinishedForLayer,		1, dict_param},
	{"updateAnimeFrame",		Suika_updateAnimeFrame,			0, NULL},
	{"unregisterAnime",		Suika_unregisterAnime,			1, dict_param},
	{"getRegAnimeName",		Suika_getRegAnimeName,			1, dict_param},
	{"getRegAnimeFile",		Suika_getRegAnimeFile,			1, dict_param},
	{"loadEyeImageIfExists",	Suika_loadEyeImageIfExists,		1, dict_param},
	{"reloadEyeAnime",		Suika_reloadEyeAnime,			1, dict_param},
	{"loadLipImageIfExists",	Suika_loadLipImageIfExists,		1, dict_param},
	{"runLipAnime",			Suika_runLipAnime,			1, dict_param},
	{"stopLipAnime",		Suika_stopLipAnime,			1, dict_param},

	/* Variable */
	{"setVariableInt",		Suika_setVariableInt,		1, dict_param},
	{"setVariableFloat",		Suika_setVariableFloat,		1, dict_param},
	{"setVariableString",		Suika_setVariableString,	1, dict_param},
	{"unsetVariable",		Suika_unsetVariable,		1, dict_param},
	{"makeVariableGlobal",		Suika_makeVariableGlobal,	1, dict_param},
	{"getVariableInt",		Suika_getVariableInt,		1, dict_param},
	{"getVariableFloat",		Suika_getVariableFloat,		1, dict_param},
	{"getVariableString",		Suika_getVariableString,	1, dict_param},
	{"getVariableCount",		Suika_getVariableCount,		0, NULL},
	{"getVariableName",		Suika_getVariableName,		1, dict_param},
	{"checkVariableExists",		Suika_checkVariableExists,	1, dict_param},
	{"isGlobalVariable",		Suika_isGlobalVariable,		1, dict_param},
	{"unsetLocalVariables",		Suika_unsetLocalVariables,	0, NULL},

	/* Save */
	{"executeSaveGlobal",		Suika_executeSaveGlobal,	0, NULL},
	{"executeLoadGlobal",		Suika_executeLoadGlobal,	0, NULL},
	{"executeSaveLocal",		Suika_executeSaveLocal,		1, dict_param},
	{"executeLoadLocal",		Suika_executeLoadLocal,		1, dict_param},
	{"checkSaveExists",		Suika_checkSaveExists,		1, dict_param},
	{"deleteLocalSave",		Suika_deleteLocalSave,		1, dict_param},
	{"deleteGlobalSave",		Suika_deleteGlobalSave,		0, NULL},
	{"checkRightAfterLoad",		Suika_checkRightAfterLoad,	0, NULL},
	{"getSaveTimestamp",		Suika_getSaveTimestamp,		1, dict_param},
	{"getLatestSaveIndex",		Suika_getLatestSaveIndex,	0, NULL},
	{"getSaveChapterName",		Suika_getSaveChapterName,	1, dict_param},
	{"getSaveLastMessage",		Suika_getSaveLastMessage,	1, dict_param},
	{"getSaveThumbnail",		Suika_getSaveThumbnail,		1, dict_param},

	/* History */
	{"clearHistory",		Suika_clearHistory,		0, NULL},
	{"addHistory",			Suika_addHistory,		1, dict_param},
	{"getHistoryCount",		Suika_getHistoryCount,		0, NULL},
	{"getHistoryName",		Suika_getHistoryName,		1, dict_param},
	{"getHistoryMessage",		Suika_getHistoryMessage,	1, dict_param},
	{"getHistoryVoice",		Suika_getHistoryVoice,		1, dict_param},

	/* Seen */
	{"loadSeen",			Suika_loadSeen,			0, NULL},
	{"saveSeen",			Suika_saveSeen,			0, NULL},
	{"getSeenFlags",		Suika_getSeenFlags,		0, NULL},
	{"setSeenFlags",		Suika_setSeenFlags,		1, dict_param},

	/* GUI */
	{"checkRightAfterSysGUI",	Suika_checkRightAfterSysGUI,	0, NULL},
	{"loadGUIFile",			Suika_loadGUIFile,		1, dict_param},
	{"startGUI",			Suika_startGUI,			0, NULL},
	{"stopGUI",			Suika_stopGUI,			0, NULL},
	{"isGUIRunning",		Suika_isGUIRunning,		0, NULL},
	{"isGUIFinished",		Suika_isGUIFinished,		0, NULL},
	{"getGUIResultLabel",		Suika_getGUIResultLabel,	0, NULL},
	{"isGUIResultTitle",		Suika_isGUIResultTitle,		0, NULL},
	{"checkIfSavedInGUI",		Suika_checkIfSavedInGUI,	0, NULL},
	{"checkIfLoadedInGUI",		Suika_checkIfLoadedInGUI,	0, NULL},

	/* HAL */
	{"getMillisec",			Suika_getMillisec,		0, NULL},
	{"checkFileExists",		Suika_checkFileExists,		1, dict_param},
	{"readFileContent",		Suika_readFileContent,		1, dict_param},
	{"writeSaveData",		Suika_writeSaveData,		1, dict_param},
	{"readSaveData",		Suika_readSaveData,		1, dict_param},
	{"playVideo",			Suika_playVideo,		1, dict_param},
	{"stopVideo",			Suika_stopVideo,		0, NULL},
	{"isVideoPlaying",		Suika_isVideoPlaying,		0, NULL},
	{"speakText",			Suika_speakText,		1, dict_param},
};

/*
 * Constant Table
 */

struct const_str_item {
	const char *name;
	const char *val;
};
struct const_int_item {
	const char *name;
	int val;
};
static struct const_str_item const_str_item[] = {
	/* String */
	{"PATH_START_TAG",	S3_PATH_START_TAG},
	{"PATH_CONFIG",		S3_PATH_CONFIG},
	{"PATH_SYSMENU_GUI",	S3_PATH_SYSMENU_GUI},
	{"PATH_SAVE_GUI",	S3_PATH_SAVE_GUI},
	{"PATH_LOAD_GUI",	S3_PATH_LOAD_GUI},
	{"PATH_HISTORY_GUI",	S3_PATH_HISTORY_GUI},
	{"PATH_CONFIG_GUI",	S3_PATH_CONFIG_GUI},
};
static struct const_int_item const_int_item[] = {
	/* Layer */
	{"LAYER_BG",			S3_LAYER_BG},
	{"LAYER_BG_FO",			S3_LAYER_BG_FO},
	{"LAYER_BG2",			S3_LAYER_BG2},
	{"LAYER_EFB1",			S3_LAYER_EFB1},
	{"LAYER_EFB2",			S3_LAYER_EFB2},
	{"LAYER_EFB3",			S3_LAYER_EFB3},
	{"LAYER_EFB4",			S3_LAYER_EFB4},
	{"LAYER_CHB",			S3_LAYER_CHB},
	{"LAYER_CHB_EYE",		S3_LAYER_CHB_EYE},
	{"LAYER_CHB_LIP",		S3_LAYER_CHB_LIP},
	{"LAYER_CHB_FO",		S3_LAYER_CHB_FO},
	{"LAYER_CHL",			S3_LAYER_CHL},
	{"LAYER_CHL_EYE",		S3_LAYER_CHL_EYE},
	{"LAYER_CHL_LIP",		S3_LAYER_CHL_LIP},
	{"LAYER_CHL_FO",		S3_LAYER_CHL_FO},
	{"LAYER_CHLC",			S3_LAYER_CHLC},
	{"LAYER_CHLC_EYE",		S3_LAYER_CHLC_EYE},
	{"LAYER_CHLC_LIP",		S3_LAYER_CHLC_LIP},
	{"LAYER_CHLC_FO",		S3_LAYER_CHLC_FO},
	{"LAYER_CHR",			S3_LAYER_CHR},
	{"LAYER_CHR_EYE",		S3_LAYER_CHR_EYE},
	{"LAYER_CHR_LIP",		S3_LAYER_CHR_LIP},
	{"LAYER_CHR_FO",		S3_LAYER_CHR_FO},
	{"LAYER_CHRC",			S3_LAYER_CHRC},
	{"LAYER_CHRC_EYE",		S3_LAYER_CHRC_EYE},
	{"LAYER_CHRC_LIP",		S3_LAYER_CHRC_LIP},
	{"LAYER_CHRC_FO",		S3_LAYER_CHRC_FO},
	{"LAYER_CHC",			S3_LAYER_CHC},
	{"LAYER_CHC_EYE",		S3_LAYER_CHC_EYE},
	{"LAYER_CHC_LIP",		S3_LAYER_CHC_LIP},
	{"LAYER_CHC_FO",		S3_LAYER_CHC_FO},
	{"LAYER_EFF1",			S3_LAYER_EFF1},
	{"LAYER_EFF2",			S3_LAYER_EFF2},
	{"LAYER_EFF3",			S3_LAYER_EFF3},
	{"LAYER_EFF4",			S3_LAYER_EFF4},
	{"LAYER_MSGBOX",		S3_LAYER_MSGBOX},
	{"LAYER_NAMEBOX",		S3_LAYER_NAMEBOX},
	{"LAYER_CHOOSE1_IDLE",		S3_LAYER_CHOOSE1_IDLE},
	{"LAYER_CHOOSE1_HOVER",		S3_LAYER_CHOOSE1_HOVER},
	{"LAYER_CHOOSE2_IDLE",		S3_LAYER_CHOOSE2_IDLE},
	{"LAYER_CHOOSE2_HOVER",		S3_LAYER_CHOOSE2_HOVER},
	{"LAYER_CHOOSE3_IDLE",		S3_LAYER_CHOOSE3_IDLE},
	{"LAYER_CHOOSE3_HOVER",		S3_LAYER_CHOOSE3_HOVER},
	{"LAYER_CHOOSE4_IDLE",		S3_LAYER_CHOOSE4_IDLE},
	{"LAYER_CHOOSE4_HOVER",		S3_LAYER_CHOOSE4_HOVER},
	{"LAYER_CHOOSE5_IDLE",		S3_LAYER_CHOOSE5_IDLE},
	{"LAYER_CHOOSE5_HOVER",		S3_LAYER_CHOOSE5_HOVER},
	{"LAYER_CHOOSE6_IDLE",		S3_LAYER_CHOOSE6_IDLE},
	{"LAYER_CHOOSE6_HOVER",		S3_LAYER_CHOOSE6_HOVER},
	{"LAYER_CHOOSE7_IDLE",		S3_LAYER_CHOOSE7_IDLE},
	{"LAYER_CHOOSE7_HOVER",		S3_LAYER_CHOOSE7_HOVER},
	{"LAYER_CHOOSE8_IDLE",		S3_LAYER_CHOOSE8_IDLE},
	{"LAYER_CHOOSE8_HOVER",		S3_LAYER_CHOOSE8_HOVER},
	{"LAYER_CHF",			S3_LAYER_CHF},
	{"LAYER_CHF_EYE",		S3_LAYER_CHF_EYE},
	{"LAYER_CHF_LIP",		S3_LAYER_CHF_LIP},
	{"LAYER_CHF_FO",		S3_LAYER_CHF_FO},
	{"LAYER_CLICK",			S3_LAYER_CLICK},
	{"LAYER_AUTO",			S3_LAYER_CLICK},
	{"LAYER_SKIP",			S3_LAYER_CLICK},
	{"LAYER_TEXT1",			S3_LAYER_TEXT1},
	{"LAYER_TEXT2",			S3_LAYER_TEXT2},
	{"LAYER_TEXT3",			S3_LAYER_TEXT3},
	{"LAYER_TEXT4",			S3_LAYER_TEXT4},
	{"LAYER_TEXT5",			S3_LAYER_TEXT5},
	{"LAYER_TEXT6",			S3_LAYER_TEXT6},
	{"LAYER_TEXT7",			S3_LAYER_TEXT7},
	{"LAYER_TEXT8",			S3_LAYER_TEXT8},
	{"LAYER_GUI_BTN1",		S3_LAYER_GUI_BTN1},
	{"LAYER_GUI_BTN2",		S3_LAYER_GUI_BTN2},
	{"LAYER_GUI_BTN3",		S3_LAYER_GUI_BTN3},
	{"LAYER_GUI_BTN4",		S3_LAYER_GUI_BTN4},
	{"LAYER_GUI_BTN5",		S3_LAYER_GUI_BTN5},
	{"LAYER_GUI_BTN6",		S3_LAYER_GUI_BTN6},
	{"LAYER_GUI_BTN7",		S3_LAYER_GUI_BTN7},
	{"LAYER_GUI_BTN8",		S3_LAYER_GUI_BTN8},
	{"LAYER_GUI_BTN9",		S3_LAYER_GUI_BTN9},
	{"LAYER_GUI_BTN10",		S3_LAYER_GUI_BTN10},
	{"LAYER_GUI_BTN11",		S3_LAYER_GUI_BTN11},
	{"LAYER_GUI_BTN12",		S3_LAYER_GUI_BTN12},
	{"LAYER_GUI_BTN13",		S3_LAYER_GUI_BTN13},
	{"LAYER_GUI_BTN14",		S3_LAYER_GUI_BTN14},
	{"LAYER_GUI_BTN15",		S3_LAYER_GUI_BTN15},
	{"LAYER_GUI_BTN16",		S3_LAYER_GUI_BTN16},
	{"LAYER_GUI_BTN17",		S3_LAYER_GUI_BTN17},
	{"LAYER_GUI_BTN18",		S3_LAYER_GUI_BTN18},
	{"LAYER_GUI_BTN19",		S3_LAYER_GUI_BTN19},
	{"LAYER_GUI_BTN20",		S3_LAYER_GUI_BTN20},
	{"LAYER_GUI_BTN21",		S3_LAYER_GUI_BTN21},
	{"LAYER_GUI_BTN22",		S3_LAYER_GUI_BTN22},
	{"LAYER_GUI_BTN23",		S3_LAYER_GUI_BTN23},
	{"LAYER_GUI_BTN24",		S3_LAYER_GUI_BTN24},
	{"LAYER_GUI_BTN25",		S3_LAYER_GUI_BTN25},
	{"LAYER_GUI_BTN26",		S3_LAYER_GUI_BTN26},
	{"LAYER_GUI_BTN27",		S3_LAYER_GUI_BTN27},
	{"LAYER_GUI_BTN28",		S3_LAYER_GUI_BTN28},
	{"LAYER_GUI_BTN29",		S3_LAYER_GUI_BTN29},
	{"LAYER_GUI_BTN30",		S3_LAYER_GUI_BTN30},
	{"LAYER_GUI_BTN31",		S3_LAYER_GUI_BTN31},
	{"LAYER_GUI_BTN32",		S3_LAYER_GUI_BTN32},
	{"STAGE_LAYERS",		S3_STAGE_LAYERS},

	/* Layer Internal & Etc.*/
	{"TEXT_LAYERS",			S3_TEXT_LAYERS},
	{"EFFECT_LAYERS",		S3_EFFECT_LAYERS},
	{"BUTTON_LAYERS",		S3_BUTTON_LAYERS},
	{"CHOOSEBOX_COUNT",		S3_CHOOSEBOX_COUNT},
	{"CLICK_FRAMES",		S3_CLICK_FRAMES},
	{"BASIC_LAYERS",		S3_CH_BASIC_LAYERS},
	{"ALL_LAYERS",			S3_CH_ALL_LAYERS},
	{"KIRAKIRA_FRAMES",		S3_KIRAKIRA_FRAMES},

	/* Character Position (chpos) */
	{"CH_BACK",			S3_CH_BACK},
	{"CH_LEFT",			S3_CH_LEFT},
	{"CH_LEFT_CENTER",		S3_CH_LEFT_CENTER},
	{"CH_RIGHT",			S3_CH_RIGHT},
	{"CH_RIGHT_CENTER",		S3_CH_RIGHT_CENTER},
	{"CH_CENTER",			S3_CH_CENTER},
	{"CH_FACE",			S3_CH_FACE},

	/* Fading Method */
	{"FADE_INVALID",		S3_FADE_INVALID},
	{"FADE_NORMAL",			S3_FADE_NORMAL},
	{"FADE_RULE", 			S3_FADE_RULE},
	{"FADE_MELT", 			S3_FADE_MELT},

	/* Blend Type */
	{"BLEND_ALPHA",			S3_BLEND_ALPHA},
	{"BLEND_ADD", 			S3_BLEND_ADD},
	{"BLEND_SUB", 			S3_BLEND_SUB},

	/* Fade Descriptor */
	{"FADE_DESC_BG",		S3_FADE_DESC_BG},
	{"FADE_DESC_CHB",		S3_FADE_DESC_CHB},
	{"FADE_DESC_CHL",		S3_FADE_DESC_CHL},
	{"FADE_DESC_CHLC",		S3_FADE_DESC_CHLC},
	{"FADE_DESC_CHR",		S3_FADE_DESC_CHR},
	{"FADE_DESC_CHRC",		S3_FADE_DESC_CHRC},
	{"FADE_DESC_CHC",		S3_FADE_DESC_CHC},
	{"FADE_DESC_CHF",		S3_FADE_DESC_CHF},
	{"FADE_DESC_COUNT",		S3_FADE_DESC_COUNT},

	/* Mixer */
	{"MIXER_TRACKS",		S3_MIXER_TRACKS},
	{"TRACK_BGM",			S3_TRACK_BGM},
	{"TRACK_VOICE",			S3_TRACK_VOICE},
	{"TRACK_SE",			S3_TRACK_SE},
	{"TRACK_SYS",			S3_TRACK_SYS},
	{"CH_VOL_SLOTS",		S3_CH_VOL_SLOTS},
	{"CH_VOL_SLOT_DEFAULT",		S3_CH_VOL_SLOT_DEFAULT},

	/* Character Map */
	{"CHARACTER_MAP_COUNT",		S3_CHARACTER_MAP_COUNT},

	/* Anime */
	{"REG_ANIME_COUNT",		S3_REG_ANIME_COUNT},
	{"ANIME_ACCEL_INVALID",		S3_ANIME_ACCEL_INVALID},
	{"ANIME_ACCEL_UNIFORM",		S3_ANIME_ACCEL_UNIFORM},
	{"ANIME_ACCEL_ACCEL",		S3_ANIME_ACCEL_ACCEL},
	{"ANIME_ACCEL_DEACCEL",		S3_ANIME_ACCEL_DEACCEL},
	{"ANIME_ACCEL_SMOOTHSTEP",	S3_ANIME_ACCEL_SMOOTHSTEP},
	{"ANIME_ACCEL_INVSMOOTHSTEP",	S3_ANIME_ACCEL_INVSMOOTHSTEP},

	/* Font */
	{"FONT_SELECT1",		S3_FONT_SELECT1},
	{"FONT_SELECT2",		S3_FONT_SELECT2},
	{"FONT_SELECT3",		S3_FONT_SELECT3},
	{"FONT_SELECT4",		S3_FONT_SELECT4},
	{"FONT_COUNT",			S3_FONT_COUNT},
	{"EMOJI_COUNT",			S3_EMOJI_COUNT},

	/* Blend */
	{"BLEND_ALPHA",			S3_BLEND_ALPHA},
	{"BLEND_ADD",			S3_BLEND_ADD},	{"BLEND_SUB",			S3_BLEND_SUB},

	/* Call Stack */
	{"CALL_STACK_MAX",		S3_CALL_STACK_MAX},

	/* Call Argument */
	{"CALL_ARGS",			S3_CALL_ARGS},

	/* Save */
	{"ALL_SAVE_SLOTS",		S3_ALL_SAVE_SLOTS},
	{"NORMAL_SAVE_SLOTS",		S3_NORMAL_SAVE_SLOTS},
	{"QUICK_SAVE_INDEX",		S3_QUICK_SAVE_INDEX},
};

/* Time origin. */
static uint64_t time_origin;

/* Forward declaration. */
static bool serialize_printer(NoctEnv *env, char *buf, size_t size, NoctValue *value, bool is_inside_obj);

/*
 * Install the API.
 */
bool
s3i_install_default_api(void)
{
	uint32_t i;

	/* Register functions. */
	for (i = 0; i < sizeof(api_func) / sizeof(struct api_func); i++) {
		if (!s3_install_api(api_func[i].name,
				    api_func[i].func,
				    api_func[i].param_count,
				    api_func[i].params))
			return false;
	}

	/* Register string constants. */
	for (i = 0; i < sizeof(const_str_item) / sizeof(struct const_str_item); i++) {
		if (!s3_define_const_string(const_str_item[i].name,
					    const_str_item[i].val))
			return false;
	}
	
	/* Register integer constants. */
	for (i = 0; i < sizeof(const_int_item) / sizeof(struct const_int_item); i++) {
		if (!s3_define_const_int(const_int_item[i].name,
					 const_int_item[i].val))
			return false;
	}

	/* Get the time origin. */
	s3_reset_lap_timer(&time_origin);

	return true;
}

/*
 * Skeleton
 */

static bool
Suika_start(
	    void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3i_on_game_start())
		return false;

	return true;
}

static bool
Suika_update(
	     void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3i_on_game_update())
		return false;

	return true;
}

static bool
Suika_render(
	     void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3i_on_game_render())
		return false;

	return true;
}

/*
 * Fundamental
 */

static bool
Suika_print(
	    void *p)
{
	NoctEnv *env;
	char buf[8192];
	NoctValue value;

	UNUSED_PARAMETER(p);

	env = pf_get_vm_env();
	if (!noct_get_arg(env, 0, &value))
		return false;

	memset(buf, 0, sizeof(buf));
	if (!serialize_printer(env, buf, sizeof(buf), &value, false))
		return false;

	s3_log_info("%s", buf);

	return true;
}

static bool
serialize_printer(
		  NoctEnv *env,
		  char *buf,
		  size_t size,
		  NoctValue *value,
		  bool is_inside_obj)
{
	int type;
	int ival;
	float fval;
	const char *sval;
	uint32_t items, i;
	char digits[1024];

	if (!noct_get_value_type(env, value, &type))
		return false;

	switch (type) {
	case NOCT_VALUE_INT:
		if (!noct_get_int(env, value, &ival))
			return false;
		snprintf(digits, sizeof(digits), "%d", ival);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_FLOAT:
		if (!noct_get_float(env, value, &fval))
			return false;
		snprintf(digits, sizeof(digits), "%f", fval);
		strncat(buf, digits, size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_STRING:
		if (!noct_get_string(env, value, &sval))
			return false;
		if (is_inside_obj)
			strncat(buf, "\"", size - strlen(buf) - 1);
		strncat(buf, sval, size - strlen(buf) - 1);
		if (is_inside_obj)
			strncat(buf, "\"", size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_ARRAY:
		if (!noct_get_array_size(env, value, &items))
			return false;
		strncat(buf, "[", size - strlen(buf) - 1);
		for (i = 0; i < items; i++) {
			NoctValue elem;
			if (!noct_get_array_elem(env, value, i, &elem))
				return false;
			if (!serialize_printer(env, buf, size, &elem, true))
				return false;
			if (i != items - 1)
				strncat(buf, ", ", size - strlen(buf) - 1);
		}
		strncat(buf, "]", size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_DICT:
		if (!noct_get_dict_size(env, value, &items))
			return false;
		strncat(buf, "{", size - strlen(buf) - 1);
		for (i = 0; i < items; i++) {
			NoctValue k, v;
			if (!noct_get_dict_key_by_index(env, value, i, &k))
				return false;
			if (!noct_get_string(env, &k, &sval))
				return false;
			strncat(buf, "\"", size - strlen(buf) - 1);
			strncat(buf, sval, size - strlen(buf) - 1);
			strncat(buf, "\"", size - strlen(buf) - 1);
			strncat(buf, ": ", size - strlen(buf) - 1);
			if (!noct_get_dict_value_by_index(env, value, i, &v))
				return false;
			if (!serialize_printer(env, buf, size, &v, true))
				return false;
			if (i != items - 1)
				strncat(buf, ", ", size - strlen(buf) - 1);
		}
		strncat(buf, "}", size - strlen(buf) - 1);
		break;
	case NOCT_VALUE_FUNC:
		strncat(buf, "<func>", size - strlen(buf) - 1);
		break;
	default:
		assert(0);
		return false;
	}

	return true;
}

static bool
Suika_loadPlugin(
		 void *p)
{
	NoctEnv *env;
	NoctValue name_val;
	const char *name;
	char path[256];
	char func_name[256];
	char *data;
	size_t len;

	UNUSED_PARAMETER(p);

	/* Get the argument. */
	env = pf_get_vm_env();
	if (!noct_get_arg_check_string(env, 0, &name_val, &name))
		return false;

	/* TODO: Add a path traversal check. */

	/* Load a source file content. */
	snprintf(path, sizeof(path), "plugins/%s/%s.pf", name, name);
	if (!s3_read_file_content(path, &data, &len))
		return false;

	/* Check for the bytecode header. */
	if (strncmp(data, NOCT_BYTECODE_HEADER, strlen(NOCT_BYTECODE_HEADER)) != 0) {
		/* It's a source file. */
		if (!noct_register_source(env, path, data)) {
			free(data);
			return false;
		}
	} else {
		/* It's a bytecode file. */
		if (!noct_register_bytecode(env, (void *)data, (uint32_t)len)) {
			free(data);
			return false;
		}
	}
	free(data);

	/* Call the initializer. */
	snprintf(func_name, sizeof(func_name), "plugin_init_%s", name);
	if (!pf_call_vm_function(func_name))
		return false;
		
	return true;
}

/*
 * Config
 */

static bool
Suika_setConfig(
		void *p)
{
	char *key;
	char *value;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	value = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;
		if (!pf_get_call_arg_string("value", &value))
			break;

		/* Set the config value. */
		if (!s3_set_config(key, value))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);
	if (value != NULL)
		free(value);

	return ret;
}

static bool
Suika_getConfigCount(
		     void *p)
{
	bool ret;
	int count;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the config count. */
		count = s3_get_config_count();

		/* Set the return value. */
		if (!pf_set_return_int(count))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getConfigKey(
		   void *p)
{
	int index;
	const char *key;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		/* Get the config key. */
		key = s3_get_config_key(index);
		if (key == NULL)
			break;

		/* Set the return value. */
		if (!pf_set_return_string(key))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isGlobalSaveConfig(
	void *p)
{
	char *key;
	bool is_global_save;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Check if the config is saved-global. */
		is_global_save = s3_is_global_save_config(key);

		/* Set the return value. */
		if (!pf_set_return_int(is_global_save ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_isLocalSaveConfig(
	void *p)
{
	char *key;
	bool is_local_save;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Check if the config is saved-local. */
		is_local_save = s3_is_local_save_config(key);

		/* Set the return value. */
		if (!pf_set_return_int(is_local_save ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getConfigType(
		    void *p)
{
	char *key;
	char type;
	const char *type_s;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config type. */
		type = s3_get_config_type(key);
		switch (type) {
		case 's':
			type_s = "s";
			break;
		case 'b':
			type_s = "b";
			break;
		case 'i':
			type_s = "i";
			break;
		case 'f':
			type_s = "f";
			break;
		default:
			assert(NEVER_COME_HERE);
			return false;
		}

		/* Set the return value. */
		if (!pf_set_return_string(type_s))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getStringConfig(
		      void *p)
{
	char *key;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config value. */
		val = s3_get_string_config(key);

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getBoolConfig(
		    void *p)
{
	char *key;
	bool val;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config value. */
		val = s3_get_bool_config(key);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getIntConfig(
		   void *p)
{
	char *key;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config value. */
		val = s3_get_int_config(key);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getFloatConfig(
		     void *p)
{
	char *key;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config value. */
		val = s3_get_float_config(key);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_getConfigAsString(
			void *p)
{
	char *key;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	key = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("key", &key))
			break;

		/* Get the config value. */
		val = s3_get_config_as_string(key);

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	if (key != NULL)
		free(key);

	return ret;
}

static bool
Suika_compareLocale(
		    void *p)
{
	char *locale;
	bool val;
	bool ret;

	UNUSED_PARAMETER(p);

	locale = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("locale", &locale))
			break;

		/* Get the config value. */
		val = s3_compare_locale(locale);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (locale != NULL)
		free(locale);

	return ret;
}

/*
 * Input
 */

static bool
Suika_getMousePosX(void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_get_mouse_pos_x();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;
	
	return true;
}

static bool
Suika_getMousePosY(void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_get_mouse_pos_y();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;
	
	return true;
}

static bool
Suika_isMouseLeftPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_mouse_left_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isMouseRightPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_mouse_right_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isMouseLeftClicked(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_mouse_left_clicked();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isMouseRightClicked(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_mouse_right_clicked();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isMouseDragging(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_mouse_dragging();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isReturnKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_return_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isSpaceKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_space_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isEscapeKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_escape_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isUpKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_up_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isDownKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_down_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isLeftKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_left_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isRightKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_right_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isPageUpKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_pageup_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isPageDownKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_pagedown_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isControlKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_control_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isSKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_s_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isLKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_l_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isHKeyPressed(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_h_key_pressed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isTouchCanceled(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_touch_canceled();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_isSwiped(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	/* Get the value. */
	val = s3_is_swiped();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_clearInputState(void *p)
{
	UNUSED_PARAMETER(p);

	/* Clear the input state. */
	s3_clear_input_state();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

/*
 * Game
 */

static bool
Suika_startCommandRepetition(void *p)
{
	UNUSED_PARAMETER(p);

	s3_start_command_repetition();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_stopCommandRepetition(void *p)
{
	UNUSED_PARAMETER(p);

	s3_stop_command_repetition();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_isInCommandRepetition(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_in_command_repetition();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_setMessageActive(void *p)
{
	UNUSED_PARAMETER(p);

	s3_set_message_active();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_clearMessageActive(void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_message_active();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_isMessageActive(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_message_active();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_startAutoMode(void *p)
{
	UNUSED_PARAMETER(p);

	s3_start_auto_mode();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_stopAutoMode(void *p)
{
	UNUSED_PARAMETER(p);

	s3_stop_auto_mode();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_isAutoMode(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_auto_mode();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_startSkipMode(void *p)
{
	UNUSED_PARAMETER(p);

	s3_start_skip_mode();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_stopSkipMode(void *p)
{
	UNUSED_PARAMETER(p);

	s3_stop_skip_mode();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;
	
	return true;
}

static bool
Suika_isSkipMode(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_skip_mode();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_setSaveLoad(void *p)
{
	int enable;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("enable", &enable))
			break;

		s3_set_save_load(enable ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isSaveLoadEnabled(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_save_load_enabled();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_setNonInterruptible(void *p)
{
	int enable;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("enable", &enable))
			break;

		s3_set_non_interruptible(enable ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isNonInterruptible(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_non_interruptible();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;
	
	return true;
}

static bool
Suika_setPenPosition(void *p)
{
	int x;
	int y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("x", &x))
			break;
		if (!pf_get_call_arg_int("y", &y))
			break;

		s3_set_pen_position(x, y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getPenPositionX(void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_pen_position_x();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getPenPositionY(void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_pen_position_y();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_pushForCall(void *p)
{
	char *file;
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_int("index", &index))
			break;

		if (!s3_push_for_call(file, index))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_popForReturn(void *p)
{
	NoctEnv *env;
	char *file;
	int index;
	NoctValue dic, tmp;
	bool ret;

	UNUSED_PARAMETER(p);

	env = p;

	file = NULL;
	ret = false;
	do {
		if (!s3_pop_for_return(&file, &index))
			break;

		/* Set the return value. */
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_string(env, &dic, "file", &tmp, file))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "index", &tmp, index))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_readCallStack(void *p)
{
	NoctEnv *env;
	int sp;
	char *file;
	int index;
	NoctValue dic, tmp;
	bool ret;

	UNUSED_PARAMETER(p);

	env = p;

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("sp", &sp))
			break;

		if (!s3_read_call_stack(sp, &file, &index))
			break;

		/* Set the return value. */
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_string(env, &dic, "file", &tmp, file))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "index", &tmp, index))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_writeCallStack(void *p)
{
	int sp;
	char *file;
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("sp", &sp))
			break;
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_int("index", &index))
			break;

		if (!s3_write_call_stack(sp, file, index))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_setCallArgument(void *p)
{
	int index;
	char *value;
	bool ret;

	UNUSED_PARAMETER(p);

	value = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("index", &index))
			break;
		if (!pf_get_call_arg_string("value", &value))
			break;

		if (!s3_set_call_argument(index, value))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (value != NULL)
		free(value);

	return ret;
}

static bool
Suika_getCallArgument(void *p)
{
	int index;
	const char *value;
	bool ret;

	UNUSED_PARAMETER(p);

	value = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		value = s3_get_call_argument(index);
		if (value == NULL)
			value = "";

		/* Set the return value. */
		if (!pf_set_return_string(value))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isPageMode(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_page_mode();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_appendBufferedMessage(void *p)
{
	char *message;
	bool ret;

	UNUSED_PARAMETER(p);

	message = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("message", &message))
			break;

		if (!s3_append_buffered_message(message))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (message != NULL)
		free(message);

	return ret;
}

static bool
Suika_getBufferedMessage(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_buffered_message();
	if (val == NULL)
		val = "";

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_clearBufferedMessage(void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_buffered_message();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_resetPageLine(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_reset_page_line();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_incPageLine(void *p)
{
	UNUSED_PARAMETER(p);

	s3_inc_page_line();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_isPageTop(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_page_top();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_registerBGVoice(void *p)
{
	char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("file", &file))
			break;

		if (!s3_register_bgvoice(file))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_getBGVoice(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_bgvoice();

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_setBGVoicePlaying(void *p)
{
	int is_playing;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("isPlaying", &is_playing))
			break;

		s3_set_bgvoice_playing(is_playing ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isBGVoicePlaying(void *p)
{
	bool val;

	UNUSED_PARAMETER(p);

	val = s3_is_bgvoice_playing();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_setChapterName(void *p)
{
	char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		if (!s3_set_chapter_name(name))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getChapterName(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_chapter_name();

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_setLastMessage(void *p)
{
	char *message;
	int is_append;
	bool ret;

	UNUSED_PARAMETER(p);

	message = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("message", &message))
			break;
		if (!pf_get_call_arg_int("isAppend", &is_append))
			break;

		if (!s3_set_last_message(message, is_append ? true : false))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (message != NULL)
		free(message);

	return ret;
}

static bool
Suika_setPrevLastMessage(void *p)
{
	char *message;
	bool ret;

	UNUSED_PARAMETER(p);

	message = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("message", &message))
			break;

		if (!s3_set_prev_last_message(message))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (message != NULL)
		free(message);

	return ret;
}

static bool
Suika_getLastMessage(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_last_message();

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_getPrevLastMessage(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_prev_last_message();

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_setTextSpeed(void *p)
{
	float speed;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_float("speed", &speed))
			break;

		s3_set_text_speed(speed);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getTextSpeed(void *p)
{
	float val;

	UNUSED_PARAMETER(p);

	val = s3_get_text_speed();

	/* Set the return value. */
	if (!pf_set_return_float(val))
		return false;

	return true;
}

static bool
Suika_setAutoSpeed(void *p)
{
	float speed;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_float("speed", &speed))
			break;

		s3_set_auto_speed(speed);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getAutoSpeed(void *p)
{
	float val;

	UNUSED_PARAMETER(p);

	val = s3_get_auto_speed();

	/* Set the return value. */
	if (!pf_set_return_float(val))
		return false;

	return true;
}

static bool
Suika_markLastEnglishTagIndex(void *p)
{
	UNUSED_PARAMETER(p);

	s3_mark_last_english_tag_index();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getLastEnglishTagIndex(void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_last_english_tag_index();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_clearLastEnglishTagIndex(void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_last_english_tag_index();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getLastTagName(void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_last_tag_name();

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

/*
 * Image
 */

static bool
Suika_createImageFromFile(void *p)
{
	char *file;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;

		img = s3_create_image_from_file(file);
		if (img == NULL)
			break;

		val = s3i_image_to_int(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_createImage(void *p)
{
	int width;
	int height;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("width", &width))
			break;
		if (!pf_get_call_arg_int("height", &height))
			break;

		img = s3_create_image(width, height);
		if (img == NULL)
			break;

		val = s3i_image_to_int(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getImageWidth(void *p)
{
	int image;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;

		img = s3i_int_to_image(image);
		if (img == NULL)
			break;

		val = s3_get_image_width(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getImageHeight(void *p)
{
	int image;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;

		img = s3i_int_to_image(image);
		if (img == NULL)
			break;

		val = s3_get_image_height(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_loadGlyphImage(void *p)
{
	int font_type;
	char *glyph;
	int size;
	int color;
	int outline_width;
	int outline_color;
	uint32_t wc;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	glyph = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("fontType", &font_type))
			break;
		if (!pf_get_call_arg_string("glyph", &glyph))
			break;
		if (!pf_get_call_arg_int("size", &size))
			break;
		if (!pf_get_call_arg_int("color", &color))
			break;
		if (!pf_get_call_arg_int("outlineWidth", &outline_width))
			break;
		if (!pf_get_call_arg_int("outlineColor", &outline_color))
			break;

		if (!s3_utf8_to_utf32(glyph, &wc))
			break;

		img = s3_load_glyph_image(font_type,
					  wc,
					  size,
					  (s3_pixel_t)color,
					  outline_width,
					  (s3_pixel_t)outline_color);
		if (img == NULL)
			break;

		val = s3i_image_to_int(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (glyph != NULL)
		free(glyph);

	return ret;
}

static bool
Suika_destroyImage(void *p)
{
	int image;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;

		img = s3i_int_to_image(image);
		if (img == NULL)
			break;

		s3_destroy_image(img);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_notifyImageUpdate(void *p)
{
	int image;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;

		img = s3i_int_to_image(image);
		if (img == NULL)
			break;

		s3_notify_image_update(img);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_drawImage(
	void *p)
{
	int dst_image;
	int dst_left;
	int dst_top;
	int src_image;
	int src_left;
	int src_top;
	int src_width;
	int src_height;
	int alpha;
	int blend;
	struct s3_image *src_img;
	struct s3_image *dst_img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("dstImage", &dst_image))
			break;
		if (!pf_get_call_arg_int("dstLeft", &dst_left))
			break;
		if (!pf_get_call_arg_int("dstTop", &dst_top))
			break;
		if (!pf_get_call_arg_int("srcImage", &src_image))
			break;
		if (!pf_get_call_arg_int("srcLeft", &src_left))
			break;
		if (!pf_get_call_arg_int("srcTop", &src_top))
			break;
		if (!pf_get_call_arg_int("srcWidth", &src_width))
			break;
		if (!pf_get_call_arg_int("srcHeight", &src_height))
			break;
		if (!pf_get_call_arg_int("alpha", &alpha))
			break;
		if (!pf_get_call_arg_int("blend", &blend))
			break;

		src_img = s3i_int_to_image(src_image);
		if (src_img == NULL)
			break;
		dst_img = s3i_int_to_image(dst_image);
		if (dst_img == NULL)
			break;

		s3_draw_image(dst_img,
			      dst_left,
			      dst_top,
			      src_img,
			      src_left,
			      src_top,
			      src_width,
			      src_height,
			      255,
			      blend);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_drawImage3D(
	void *p)
{
	int x1, y1, x2, y2, x3, y3, x4, y4;
	int src_left, src_top, src_width, src_height;
	int alpha, blend;
	int src_image, dst_image;
	struct s3_image *src_img;
	struct s3_image *dst_img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("dstImage", &dst_image))
			break;
		if (!pf_get_call_arg_int("x1", &x1))
			break;
		if (!pf_get_call_arg_int("y1", &y1))
			break;
		if (!pf_get_call_arg_int("x2", &x2))
			break;
		if (!pf_get_call_arg_int("y2", &y2))
			break;
		if (!pf_get_call_arg_int("x3", &x3))
			break;
		if (!pf_get_call_arg_int("y3", &y3))
			break;
		if (!pf_get_call_arg_int("x4", &x4))
			break;
		if (!pf_get_call_arg_int("y4", &y4))
			break;
		if (!pf_get_call_arg_int("srcImage", &src_image))
			break;
		if (!pf_get_call_arg_int("srcLeft", &src_left))
			break;
		if (!pf_get_call_arg_int("srcTop", &src_top))
			break;
		if (!pf_get_call_arg_int("srcWidth", &src_width))
			break;
		if (!pf_get_call_arg_int("srcHeight", &src_height))
			break;
		if (!pf_get_call_arg_int("alpha", &alpha))
			break;
		if (!pf_get_call_arg_int("blend", &blend))
			break;

		src_img = s3i_int_to_image(src_image);
		if (src_img == NULL)
			break;
		dst_img = s3i_int_to_image(dst_image);
		if (dst_img == NULL)
			break;

		s3_draw_image_3d(dst_img,
				 x1,
				 y1,
				 x2,
				 y2,
				 x3,
				 y3,
				 x4,
				 y4,
				 src_img,
				 src_left,
				 src_top,
				 src_width,
				 src_height,
				 alpha,
				 blend);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_makeColor(void *p)
{
	int r;
	int g;
	int b;
	int a;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("r", &r))
			break;
		if (!pf_get_call_arg_int("g", &g))
			break;
		if (!pf_get_call_arg_int("b", &b))
			break;
		if (!pf_get_call_arg_int("a", &a))
			break;

		val = (int)s3_make_pixel((uint32_t)a,
					 (uint32_t)r,
					 (uint32_t)g,
					 (uint32_t)b);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_fillImageRect(void *p)
{
	int image;
	int left;
	int top;
	int width;
	int height;
	int color;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;
		if (!pf_get_call_arg_int("left", &left))
			break;
		if (!pf_get_call_arg_int("top", &top))
			break;
		if (!pf_get_call_arg_int("width", &width))
			break;
		if (!pf_get_call_arg_int("height", &height))
			break;
		if (!pf_get_call_arg_int("color", &color))
			break;

		img = s3i_int_to_image(image);
		if (img == NULL)
			break;

		s3_fill_image_rect(img,
				   left,
				   top,
				   width,
				   height,
				   (s3_pixel_t)color);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * Stage
 */

static bool
Suika_reloadStageImages(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_reload_stage_images();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_reloadStagePositions(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_reload_stage_positions();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getLayerX(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_x(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerY(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_y(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerPosition(
	void *p)
{
	int layer;
	int x;
	int y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("x", &x))
			break;
		if (!pf_get_call_arg_int("y", &y))
			break;

		s3_set_layer_position(layer, x, y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerScaleX(
	void *p)
{
	int layer;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_scale_x(layer);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerScaleY(
	void *p)
{
	int layer;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_scale_y(layer);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerScale(
	void *p)
{
	int layer;
	float scale_x;
	float scale_y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_float("scaleX", &scale_x))
			break;
		if (!pf_get_call_arg_float("scaleY", &scale_y))
			break;

		s3_set_layer_scale(layer, scale_x, scale_y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerCenterX(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_center_x(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerCenterY(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_center_y(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerCenter(
	void *p)
{
	int layer;
	int center_x;
	int center_y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("centerX", &center_x))
			break;
		if (!pf_get_call_arg_int("centerY", &center_y))
			break;

		s3_set_layer_center(layer, center_x, center_y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerRotate(
	void *p)
{
	int layer;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_rotate(layer);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerRotate(
	void *p)
{
	int layer;
	float rotate;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_float("rotate", &rotate))
			break;

		s3_set_layer_rotate(layer, rotate);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerDim(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_dim(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerDim(
	void *p)
{
	int layer;
	int dim;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("dim", &dim))
			break;

		s3_set_layer_dim(layer, dim);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerWidth(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_width(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerHeight(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_height(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerAlpha(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_alpha(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerAlpha(
	void *p)
{
	int layer;
	int alpha;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("alpha", &alpha))
			break;

		s3_set_layer_alpha(layer, alpha);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerBlend(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
	
		val = s3_get_layer_blend(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setLayerBlend(
	void *p)
{
	int layer;
	int blend;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("blend", &blend))
			break;
	
		s3_set_layer_blend(layer, blend);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_getLayerFile(
	void *p)
{
	int layer;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
	
		val = s3_get_layer_file_name(layer);
		if (val == NULL)
			val = "";

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setLayerFile(
	void *p)
{
	int layer;
	char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_string("file", &file))
			break;

		if (!s3_set_layer_file_name(layer, file))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;	
}

static bool
Suika_getLayerImage(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
	
		val = s3i_image_to_int(s3_get_layer_image(layer));

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setLayerImage(
	void *p)
{
	int layer;
	int image;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("image", &image))
			break;

		img = s3i_int_to_image(image);
		s3_set_layer_image(layer, img);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_getLayerFrame(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_frame(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setLayerFrame(
	void *p)
{
	int layer;
	int frame;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_int("frame", &frame))
			break;

		s3_set_layer_frame(layer, frame);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getLayerText(
	void *p)
{
	int layer;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_get_layer_text(layer);
		if (val == NULL)
			val = "";

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setLayerText(
	void *p)
{
	int layer;
	char *text;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;
		if (!pf_get_call_arg_string("text", &text))
			break;

		s3_set_layer_text(layer, text);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_getSysBtnIdleImage(
	void *p)
{
	int val;
	struct s3_image *img;

	UNUSED_PARAMETER(p);

	img = s3_get_sysbtn_idle_image();
	if (img == NULL)
		return false;

	val = s3i_image_to_int(img);

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getSysBtnHoverImage(
	void *p)
{
	int val;
	struct s3_image *img;

	UNUSED_PARAMETER(p);

	img = s3_get_sysbtn_hover_image();
	if (img == NULL)
		return false;

	val = s3i_image_to_int(img);

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_clearStageBasic(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_stage_basic();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_clearStage(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_stage();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_chposToLayer(
	void *p)
{
	int chpos;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		val = s3_chpos_to_layer(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_chposToEyeLayer(
	void *p)
{
	int chpos;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		val = s3_chpos_to_eye_layer(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_chposToLipLayer(
	void *p)
{
	int chpos;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		val = s3_chpos_to_lip_layer(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_layerToChpos(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_layer_to_chpos(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_renderStage(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_render_stage();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}	

static bool
Suika_drawStageToThumb(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_draw_stage_to_thumb();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getThumbImage(
	void *p)
{
	int val;
	struct s3_image *img;

	UNUSED_PARAMETER(p);

	img = s3_get_thumb_image();
	val = s3i_image_to_int(img);

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getFadeMethod(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_get_fade_method(name);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getAccelMethod(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_get_accel_method(name);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_startFade(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * TODO:
	 *
	 * This is intentional.
	 * Write this function before the 1.0.0 release.
	 */
	s3_log_error(S3_TR("This API is not implemented yet."));
	return false;
}

static bool
Suika_getShakeOffset(
	void *p)
{
	int x;
	int y;
	NoctEnv *env;
	NoctValue dic, tmp;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		s3_get_shake_offset(&x, &y);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, y))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setShakeOffset(
	void *p)
{
	int x;
	int y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("x", &x))
			break;
		if (!pf_get_call_arg_int("y", &y))
			break;

		s3_set_shake_offset(x, y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);


	return ret;
}

static bool
Suika_isFadeRunning(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_fade_running();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_finishFade(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_finish_fade();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_setChNameMapping(
	void *p)
{
	int chpos;
	int chNameIndex;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;
		if (!pf_get_call_arg_int("chNameIndex", &chNameIndex))
			break;

		s3_set_ch_name_mapping(chpos, chNameIndex);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getChTalking(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_ch_talking();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_setChTalking(
	void *p)
{
	int chpos;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		s3_set_ch_talking(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getTalkingChpos(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_talking_chpos();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_updateChDimByTalkingCh(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_update_ch_dim_by_talking_ch();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_forceChDim(
	void *p)
{
	int chpos;
	int dim;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;
		if (!pf_get_call_arg_int("dim", &dim))
			break;

		s3_force_ch_dim(chpos, dim ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getChDim(
	void *p)
{
	int chpos;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		val = s3_get_ch_dim(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_fillNameBox(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_fill_namebox();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getNameBoxRect(
	void *p)
{
	NoctEnv *env;
	NoctValue dic, tmp;
	int x;
	int y;
	int w;
	int h;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		s3_get_namebox_rect(&x, &y, &w, &h);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, y))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "w", &tmp, w))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "h", &tmp, h))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_showNameBox(
	void *p)
{
	int show;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("show", &show))
			break;

		s3_show_namebox(show ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_fillMessageBox(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_fill_msgbox();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_showMessageBox(
	void *p)
{
	int show;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("show", &show))
			break;

		s3_show_msgbox(show ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getMessageBoxRect(
	void *p)
{
	NoctEnv *env;
	NoctValue dic, tmp;
	int x;
	int y;
	int w;
	int h;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		s3_get_msgbox_rect(&x, &y, &w, &h);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, y))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "w", &tmp, w))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "h", &tmp, h))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setClickPosition(
	void *p)
{
	int x;
	int y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("x", &x))
			break;
		if (!pf_get_call_arg_int("y", &y))
			break;

		s3_set_click_position(x, y);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_showClick(
	void *p)
{
	int show;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("show", &show))
			break;

		s3_show_click(show ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setClickIndex(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_set_click_index(index);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getClickRect(
	void *p)
{
	NoctEnv *env;
	NoctValue dic, tmp;
	int x;
	int y;
	int w;
	int h;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		s3_get_click_rect(&x, &y, &w, &h);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, y))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "w", &tmp, w))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "h", &tmp, h))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_fillChooseBoxIdleImage(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_fill_choosebox_idle_image(index);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_fillChooseBoxHoverImage(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_fill_choosebox_hover_image(index);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_showChooseBox(
	void *p)
{
	int index;
	int show_idle;
	int show_hover;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;
		if (!pf_get_call_arg_int("showIdle", &show_idle))
			break;
		if (!pf_get_call_arg_int("showHover", &show_hover))
			break;

		s3_show_choosebox(index,
				  show_idle ? true : false,
				  show_hover ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getChooseBoxRect(
	void *p)
{
	NoctEnv *env;
	NoctValue dic, tmp;
	int index;
	int x;
	int y;
	int w;
	int h;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_get_choosebox_rect(index, &x, &y, &w, &h);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, y))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "w", &tmp, w))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "h", &tmp, h))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_showAutoModeBanner(
	void *p)
{
	int show;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("show", &show))
			break;

		s3_show_automode_banner(show ? true : false);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_showSkipModeBanner(
	void *p)
{
	int show;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("show", &show))
			break;

		s3_show_skipmode_banner(show ? true : false);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			return false;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_renderImage(
	void *p)
{
	int dst_left;
	int dst_top;
	int dst_width;
	int dst_height;
	int image;
	int src_left;
	int src_top;
	int src_width;
	int src_height;
	int alpha;
	int blend;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("dstLeft", &dst_left))
			break;
		if (!pf_get_call_arg_int("dstTop", &dst_top))
			break;
		if (!pf_get_call_arg_int("dstWidth", &dst_width))
			break;
		if (!pf_get_call_arg_int("dstHeight", &dst_height))
			break;
		if (!pf_get_call_arg_int("image", &image))
			break;
		if (!pf_get_call_arg_int("srcLeft", &src_left))
			break;
		if (!pf_get_call_arg_int("srcTop", &src_top))
			break;
		if (!pf_get_call_arg_int("srcWidth", &src_width))
			break;
		if (!pf_get_call_arg_int("srcHeight", &src_height))
			break;
		if (!pf_get_call_arg_int("alpha", &alpha))
			break;
		if (!pf_get_call_arg_int("blend", &blend))
			break;

		img = s3i_int_to_image(image);
		s3_render_image(
				dst_left,
				dst_top,
				dst_width,
				dst_height,
				img,
				src_left,
				src_top,
				src_width,
				src_height,
				alpha,
				blend);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_renderImage3d(
	void *p)
{
	int x1, y1, x2, y2, x3, y3, x4, y4;
	int src_left, src_top, src_width, src_height;
	int image;
	int alpha;
	int blend;
	struct s3_image *img;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("x1", &x1))
			break;
		if (!pf_get_call_arg_int("y1", &y1))
			break;
		if (!pf_get_call_arg_int("x2", &x2))
			break;
		if (!pf_get_call_arg_int("y2", &y2))
			break;
		if (!pf_get_call_arg_int("x3", &x3))
			break;
		if (!pf_get_call_arg_int("y3", &y3))
			break;
		if (!pf_get_call_arg_int("x4", &x4))
			break;
		if (!pf_get_call_arg_int("y4", &y4))
			break;
		if (!pf_get_call_arg_int("image", &image))
			break;
		if (!pf_get_call_arg_int("srcLeft", &src_left))
			break;
		if (!pf_get_call_arg_int("srcTop", &src_top))
			break;
		if (!pf_get_call_arg_int("srcWidth", &src_width))
			break;
		if (!pf_get_call_arg_int("srcHeight", &src_height))
			break;
		if (!pf_get_call_arg_int("alpha", &alpha))
			break;
		if (!pf_get_call_arg_int("beldn", &blend))
			break;

		img = s3i_int_to_image(image);
		s3_render_image_3d((float)x1,
				   (float)y1,
				   (float)x2,
				   (float)y2,
				   (float)x3,
				   (float)y3,
				   (float)x4,
				   (float)y4,
				   img,
				   src_left,
				   src_top,
				   src_width,
				   src_height,
				   alpha,
				   blend);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * Mixer
 */

static bool
Suika_setMixerInputFile(
	void *p)
{
	int track;
	char *file;
	int is_looped;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("track", &track))
			break;
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_int("isLooped", &is_looped))
			break;

		if (!s3_set_mixer_input_file(track, file, is_looped ? true : false))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_setMixerVolume(
	void *p)
{
	int track;
	float volume;
	float span;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("track", &track))
			break;
		if (!pf_get_call_arg_float("volume", &volume))
			break;
		if (!pf_get_call_arg_float("span", &span))
			break;

		s3_set_mixer_volume(track, volume, span);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getMixerVolume(
	void *p)
{
	int track;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("track", &track))
			break;

		val = s3_get_mixer_volume(track);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setMasterVolume(
	void *p)
{
	float volume;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_float("volume", &volume))
			break;

		s3_set_master_volume(volume);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getMasterVolume(
	void *p)
{
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		val = s3_get_master_volume();

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setMixerGlobalVolume(void *p)
{
	int track;
	float volume;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("track", &track))
			break;
		if (!pf_get_call_arg_float("volume", &volume))
			break;

		s3_set_mixer_global_volume(track, volume);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getMixerGlobalVolume(
	void *p)
{
	int track;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("track", &track))
			break;

		val = s3_get_mixer_global_volume(track);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_setCharacterVolume(
	void *p)
{
	int index;
	float vol;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("index", &index))
			break;
		if (!pf_get_call_arg_float("volume", &vol))
			break;

		s3_set_character_volume(index, vol);
	
		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getCharacterVolume(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		float val = s3_get_character_volume(index);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isMixerSoundFinished(
	void *p)
{
	int track;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("track", &track))
			break;

		val = s3_is_mixer_sound_finished(track);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getTrackFile(
	void *p)
{
	int track;
	const char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("track", &track))
			break;

		file = s3_get_track_file_name(track);
		if (file == NULL)
			file = "";

		/* Set the return value. */
		if (!pf_set_return_string(file))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_applyCharacterVolume(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_apply_character_volume(index);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * System Button Subsystem
 */

static bool
Suika_enableSysBtn(
	void *p)
{
	int is_enabled;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("isEnabled", &is_enabled))
			break;

		s3_enable_sysbtn(is_enabled ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isSysBtnEnabled(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_sysbtn_enabled();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_updateSysBtnState(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_update_sysbtn_state();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_isSysBtnPointed(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_sysbtn_pointed();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_isSysBtnClicked(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_sysbtn_clicked();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

/*
 * Text Subsystem
 */

static bool
Suika_getStringWidth(
	void *p)
{
	int fontType;
	int fontSize;
	char *text;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("fontType", &fontType))
			break;
		if (!pf_get_call_arg_int("fontSize", &fontSize))
			break;
		if (!pf_get_call_arg_string("text", &text))
			break;

		val = s3_get_string_width(fontType, fontSize, text);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_getStringHeight(
	void *p)
{
	int fontType;
	int fontSize;
	char *text;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("fontType", &fontType))
			break;
		if (!pf_get_call_arg_int("fontSize", &fontSize))
			break;
		if (!pf_get_call_arg_string("text", &text))
			break;

		val = s3_get_string_height(fontType, fontSize, text);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_drawGlyph(
	void *p)
{
	int image;
	int fontType;
	int fontSize;
	int baseFontSize;
	int outlineSize;
	int x;
	int y;
	int color;
	int outlineColor;
	char *glyph;
	int dim;
	int ret_w, ret_h;
	struct s3_image *img;
	uint32_t wc;
	bool ret;

	UNUSED_PARAMETER(p);

	glyph = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("image", &image))
			break;
		if (!pf_get_call_arg_int("fontType", &fontType))
			break;
		if (!pf_get_call_arg_int("fontSize", &fontSize))
			break;
		if (!pf_get_call_arg_int("baseFontSize", &baseFontSize))
			break;
		if (!pf_get_call_arg_int("outlineSize", &outlineSize))
			break;
		if (!pf_get_call_arg_int("x", &x))
			break;
		if (!pf_get_call_arg_int("y", &y))
			break;
		if (!pf_get_call_arg_int("color", &color))
			break;
		if (!pf_get_call_arg_int("outlineColor", &outlineColor))
			break;
		if (!pf_get_call_arg_string("glyph", &glyph))
			break;
		if (!pf_get_call_arg_int("dim", &dim))
			break;

		img = s3i_int_to_image(image);
		assert(img != NULL);

		if (!s3_utf8_to_utf32(glyph, &wc))
			break;

		s3_draw_glyph(
			      img,
			      fontType,
			      fontSize,
			      baseFontSize,
			      outlineSize,
			      x,
			      y,
			      (s3_pixel_t)color,
			      (s3_pixel_t)outlineColor,
			      wc,
			      &ret_w,
			      &ret_h,
			      dim ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (glyph != NULL)
		free(glyph);

	return ret;
}

static bool
Suika_createDrawMsg(
	void *p)
{
	int image;
	char *text;
	int fontType;
	int fontSize;
	int baseFontSize;
	int rubySize;
	int outlineSize;
	int penX;
	int penY;
	int areaWidth;
	int areaHeight;
	int leftMargin;
	int rightMargin;
	int topMargin;
	int bottomMargin;
	int lineMargin;
	int charMargin;
	int color;
	int outlineColor;
	int bgColor;
	int fillBg;
	int dim;
	int ignoreLF;
	int ignoreFont;
	int ignoreOutline;
	int ignoreColor;
	int ignoreSize;
	int ignorePosition;
	int ignoreRuby;
	int ignoreWait;
	int inlineWaitHook;
	int tategaki;
	struct s3_drawmsg *context;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_int("image", &image))
			break;
		if (!pf_get_call_arg_string("text", &text))
			break;
		if (!pf_get_call_arg_int("fontType", &fontType))
			break;
		if (!pf_get_call_arg_int("fontSize", &fontSize))
			break;
		if (!pf_get_call_arg_int("baseFontSize", &baseFontSize))
			break;
		if (!pf_get_call_arg_int("rubySize", &rubySize))
			break;
		if (!pf_get_call_arg_int("outlineSize", &outlineSize))
			break;
		if (!pf_get_call_arg_int("penX", &penX))
			break;
		if (!pf_get_call_arg_int("penY", &penY))
			break;
		if (!pf_get_call_arg_int("areaWidth", &areaWidth))
			break;
		if (!pf_get_call_arg_int("areaHeight", &areaHeight))
			break;
		if (!pf_get_call_arg_int("leftMargin", &leftMargin))
			break;
		if (!pf_get_call_arg_int("rightMargin", &rightMargin))
			break;
		if (!pf_get_call_arg_int("topMargin", &topMargin))
			break;
		if (!pf_get_call_arg_int("bottomMargin", &bottomMargin))
			break;
		if (!pf_get_call_arg_int("lineMargin", &lineMargin))
			break;
		if (!pf_get_call_arg_int("charMargin", &charMargin))
			break;
		if (!pf_get_call_arg_int("color", &color))
			break;
		if (!pf_get_call_arg_int("outlineColor", &outlineColor))
			break;
		if (!pf_get_call_arg_int("bgColor", &bgColor))
			break;
		if (!pf_get_call_arg_int("fillBg", &fillBg))
			break;
		if (!pf_get_call_arg_int("dim", &dim))
			break;
		if (!pf_get_call_arg_int("ignoreLF", &ignoreLF))
			break;
		if (!pf_get_call_arg_int("ignoreFont", &ignoreFont))
			break;
		if (!pf_get_call_arg_int("ignoreOutline", &ignoreOutline))
			break;
		if (!pf_get_call_arg_int("ignoreColor", &ignoreColor))
			break;
		if (!pf_get_call_arg_int("ignoreSize", &ignoreSize))
			break;
		if (!pf_get_call_arg_int("ignorePosition", &ignorePosition))
			break;
		if (!pf_get_call_arg_int("ignoreRuby", &ignoreRuby))
			break;
		if (!pf_get_call_arg_int("ignoreWait", &ignoreWait))
			break;
		if (!pf_get_call_arg_int("inlineWaitHook", &inlineWaitHook))
			break;
		if (!pf_get_call_arg_int("tategaki", &tategaki))
			break;

		/* TODO: inline hook. */

		context = s3_create_drawmsg(
					    s3i_int_to_image(image),
					    text,
					    fontType,
					    fontSize,
					    baseFontSize,
					    rubySize,
					    outlineSize,
					    penX,
					    penY,
					    areaWidth,
					    areaHeight,
					    leftMargin,
					    rightMargin,
					    topMargin,
					    bottomMargin,
					    lineMargin,
					    charMargin,
					    (s3_pixel_t)color,
					    (s3_pixel_t)outlineColor,
					    (s3_pixel_t)bgColor,
					    fillBg ? true : false,
					    dim ? true : false,
					    ignoreLF ? true : false,
					    ignoreFont ? true : false,
					    ignoreOutline ? true : false,
					    ignoreColor ? true : false,
					    ignoreSize ? true : false,
					    ignorePosition ? true : false,
					    ignoreRuby ? true : false,
					    ignoreWait ? true : false,
					    NULL,
					    tategaki ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(s3i_drawmsg_to_int(context)))
			break;
	
		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_destroyDrawMsg(
	void *p)
{
	int context;
	struct s3_drawmsg *ctx;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("context", &context))
			break;

		ctx = s3i_int_to_drawmsg(context);
		if (ctx == NULL)
			break;

		s3_destroy_drawmsg(ctx);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;
	
		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_countDrawMsgChars(
	void *p)
{
	int context;
	struct s3_drawmsg *ctx;
	int val;
	bool ret;
	int width;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("context", &context))
			break;

		ctx = s3i_int_to_drawmsg(context);
		if (ctx == NULL)
			break;

		val = s3_count_drawmsg_chars(ctx, &width);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;
	
		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_drawMessage(
	void *p)
{
	int context;
	int max_chars;
	struct s3_drawmsg *ctx;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("context", &context))
			break;
		if (!pf_get_call_arg_int("maxChars", &max_chars))
			break;

		ctx = s3i_int_to_drawmsg(context);
		if (ctx == NULL)
			break;

		val = s3_draw_message(ctx, max_chars);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;
	
		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getDrawMsgPenPosition(
	void *p)
{
	NoctEnv *env;
	NoctValue dic, tmp;
	int context;
	struct s3_drawmsg *ctx;
	int pen_x, pen_y;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("context", &context))
			break;

		ctx = s3i_int_to_drawmsg(context);
		if (ctx == NULL)
			break;

		s3_get_drawmsg_pen_position(ctx, &pen_x, &pen_y);

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_dict(env, &dic))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "x", &tmp, pen_x))
			break;
		if (!noct_set_dict_elem_make_int(env, &dic, "y", &tmp, pen_y))
			break;
		if (!noct_set_return(env, &dic))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_setDrawMsgIgnoreInlineWait(
	void *p)
{
	int context;
	struct s3_drawmsg *ctx;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("context", &context))
			break;

		ctx = s3i_int_to_drawmsg(context);
		if (ctx == NULL)
			break;

		s3_set_drawmsg_ignore_inline_wait(ctx);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;	
}

static bool
Suika_isQuotedSerif(
	void *p)
{
	char *text;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("text", &text))
			break;

		val = s3_is_quoted_serif(text);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_isEscapeSequenceChar(
	void *p)
{
	char *text;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("text", &text))
			break;

		val = s3_is_escape_sequence_char(*text);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

/*
 * Tag
 */

static bool
Suika_getTagCount(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_tag_count();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_moveToTagFile(
	void *p)
{
	char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;

		if (!s3_move_to_tag_file(file))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_moveToTagIndex(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		if (!s3_move_to_tag_index(index))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_moveToNextTag(
	void *p)
{
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		if (!s3_move_to_next_tag())
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_moveToLabelTag(
	void *p)
{
	char *label;
	bool ret;

	UNUSED_PARAMETER(p);

	label = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("label", &label))
			break;

		if (!s3_move_to_label_tag(label))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (label != NULL)
		free(label);

	return ret;
}

static bool
Suika_moveToMacroTag(
	void *p)
{
	char *macro;
	bool ret;

	UNUSED_PARAMETER(p);

	macro = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("macro", &macro))
			break;

		if (!s3_move_to_macro_tag(macro))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (macro != NULL)
		free(macro);

	return ret;
}

static bool
Suika_moveToElseTag(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_move_to_else_tag())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_moveToEndIfTag(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_move_to_endif_tag())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_moveToEndMacroTag(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_move_to_endmacro_tag())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getTagFile(
	void *p)
{
	const char *file;

	UNUSED_PARAMETER(p);

	file = s3_get_tag_file();
	if (file == NULL)
		file = "";

	/* Set the return value. */
	if (!pf_set_return_string(file))
		return false;

	return true;
}

static bool
Suika_getTagIndex(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_tag_index();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getTagLine(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_tag_line();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getTagName(
	void *p)
{
	const char *name;

	UNUSED_PARAMETER(p);

	name = s3_get_tag_name();
	if (name == NULL)
		name = "";

	/* Set the return value. */
	if (!pf_set_return_string(name))
		return false;

	return true;
}

static bool
Suika_getTagPropertyCount(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_tag_property_count();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getTagPropertyName(
	void *p)
{
	int index;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;
	
		val = s3_get_tag_property_name(index);
		if (val == NULL)
			val = "";

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getTagPropertyValue(
	void *p)
{
	int index;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;
	
		val = s3_get_tag_property_value(index);
		if (val == NULL)
			val = "";

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_checkTagArg(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_check_tag_arg(name);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;	
}

static bool
Suika_getTagArgBool(
	void *p)
{
	char *name;
	int omissible;
	int def_value;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("omissible", &omissible))
			break;
		if (!pf_get_call_arg_int("defValue", &def_value))
			break;

		val = s3_get_tag_arg_bool(name,
					  omissible ? true : false,
					  def_value ? true : false);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getTagArgInt(
	void *p)
{
	char *name;
	int omissible;
	int def_value;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("omissible", &omissible))
			break;
		if (!pf_get_call_arg_int("defValue", &def_value))
			break;

		val = s3_get_tag_arg_int(name,
					 omissible ? true : false,
					 def_value);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getTagArgFloat(
	void *p)
{
	char *name;
	int omissible;
	float def_value;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("omissible", &omissible))
			break;
		if (!pf_get_call_arg_float("defValue", &def_value))
			break;

		val = s3_get_tag_arg_float(name,
					   omissible ? true : false,
					   def_value);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getTagArgString(
	void *p)
{
	char *name;
	int omissible;
	char *def_value;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	def_value = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("omissible", &omissible))
			break;
		if (!pf_get_call_arg_string("defValue", &def_value))
			break;

		val = s3_get_tag_arg_string(name,
					    omissible ? true : false,
					    def_value);

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);
	if (def_value != NULL)
		free(def_value);

	return ret;
}

static bool
Suika_evaluateTag(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_evaluate_tag();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_pushTagStackIf(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_push_tag_stack_if())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_popTagStackIf(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_pop_tag_stack_if())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_pushTagStackWhile(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_push_tag_stack_while())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_popTagStackWhile(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_pop_tag_stack_while())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_pushTagStackFor(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_push_tag_stack_for())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_popTagStackFor(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_pop_tag_stack_for())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

/*
 * Anime
 */

static bool
Suika_loadAnimeFromFile(
	void *p)
{
	NoctEnv *env;
	NoctValue arr, tmp;
	char *file;
	char *reg_name;
	uint32_t i;
	bool used_layer[S3_STAGE_LAYERS];
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	reg_name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_string("regName", &reg_name))
			break;

		if (strcmp(reg_name, "") == 0) {
			free(reg_name);
			reg_name = NULL;
		}

		if (!s3_load_anime_from_file(file, reg_name, used_layer))
			break;

		/* Set the return value. */
		env = p;
		if (!noct_make_empty_array(env, &arr))
			break;
		for (i = 0; i < S3_STAGE_LAYERS; i++) {
			if (!noct_set_array_elem_make_int(env, &arr, i, &tmp, used_layer[i] ? 1 : 0))
				break;
		}
		if (i != S3_STAGE_LAYERS)
			break;
		if (!noct_set_return(env, &arr))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);
	if (reg_name != NULL)
		free(reg_name);

	return ret;
}

static bool
Suika_clearLayerAnimeSequence(
	void *p)
{
	int layer;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		s3_clear_layer_anime_sequence(layer);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_clearAllAnimeSequence(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_all_anime_sequence();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_newAnimeSequence(
	void *p)
{
	int layer;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		s3_new_anime_sequence(layer);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_addAnimeSequencePropertyF(
	void *p)
{
	char *name;
	float value;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_float("value", &value))
			break;

		s3_add_anime_sequence_property_f(name, value);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_addAnimeSequencePropertyI(
	void *p)
{
	char *name;
	int value;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("value", &value))
			break;

		s3_add_anime_sequence_property_i(name, value);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_startLayerAnime(
	void *p)
{
	int layer;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		s3_start_layer_anime(layer);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_isAnimeRunning(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_anime_running();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_isAnimeRunningWithLayerMask(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * TODO: Not Implemented
	 *
	 * This is intentional.
	 * Should be implemented before 1.0.0.
	 */
	s3_log_error(S3_TR("This API is not implemented yet."));
	return false;
}

static bool
Suika_isAnimeFinishedForLayer(
	void *p)
{
	int layer;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("layer", &layer))
			break;

		val = s3_is_anime_finished_for_layer(layer);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_updateAnimeFrame(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_update_anime_frame();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_unregisterAnime(
	void *p)
{
	char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		if (!s3_unregister_anime(name))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;
		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getRegAnimeName(
	void *p)
{
	int index;
	const char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		name = s3_get_reg_anime_name(index);
		if (name == NULL)
			name = "";

		/* Set the return value. */
		if (!pf_set_return_string(name))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getRegAnimeFile(
	void *p)
{
	int index;
	const char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		file = s3_get_reg_anime_file_name(index);
		if (file == NULL)
			file = "";

		/* Set the return value. */
		if (!pf_set_return_string(file))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_loadEyeImageIfExists(
	void *p)
{
	int chpos;
	char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;
		if (!pf_get_call_arg_string("file", &file))
			break;

		if (!s3_load_eye_image_if_exists(chpos, file))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_reloadEyeAnime(
	void *p)
{
	int chpos;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		if (!s3_reload_eye_anime(chpos))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_loadLipImageIfExists(
	void *p)
{
	int chpos;
	char *file;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;
		if (!pf_get_call_arg_string("file", &file))
			break;

		if (!s3_load_lip_image_if_exists(chpos, file))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_runLipAnime(
	void *p)
{
	int chpos;
	char *text;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;
		if (!pf_get_call_arg_string("text", &text))
			break;

		s3_run_lip_anime(chpos, text);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}

static bool
Suika_stopLipAnime(
	void *p)
{
	int chpos;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("chpos", &chpos))
			break;

		s3_stop_lip_anime(chpos);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * Variable
 */

static bool
Suika_setVariableInt(
	void *p)
{
	char *name;
	int value;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("value", &value))
			break;

		s3_set_variable_int(name, value);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_setVariableFloat(
	void *p)
{
	char *name;
	float value;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_float("value", &value))
			break;

		s3_set_variable_float(name, value);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_setVariableString(
	void *p)
{
	char *name;
	char *value;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	value = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_string("value", &value))
			break;

		s3_set_variable_string(name, value);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);
	if (value != NULL)
		free(value);

	return ret;
}

static bool
Suika_unsetVariable(
	void *p)
{
	char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		s3_unset_variable(name);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_makeVariableGlobal(
	void *p)
{
	char *name;
	int is_global;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_int("isGlobal", &is_global))
			break;

		s3_make_variable_global(name, is_global);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getVariableInt(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_get_variable_int(name);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getVariableFloat(
	void *p)
{
	char *name;
	float val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_get_variable_float(name);

		/* Set the return value. */
		if (!pf_set_return_float(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getVariableString(
	void *p)
{
	char *name;
	const char *val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_get_variable_string(name);
		if (val == NULL)
			val = "";

		/* Set the return value. */
		if (!pf_set_return_string(val))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_getVariableCount(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_variable_count();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getVariableName(
	void *p)
{
	int index;
	const char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		name = s3_get_variable_name(index);
		if (name == NULL)
			name = "";

		/* Set the return value. */
		if (!pf_set_return_string(name))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_checkVariableExists(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_check_variable_exists(name);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_isGlobalVariable(
	void *p)
{
	char *name;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("name", &name))
			break;

		val = s3_is_global_variable(name);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);

	return ret;
}

static bool
Suika_unsetLocalVariables(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_unset_local_variables();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

/*
 * Save
 */

static bool
Suika_executeSaveGlobal(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_execute_save_global())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_executeLoadGlobal(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_execute_load_global())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_executeSaveLocal(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		if (!s3_execute_save_local(index))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_executeLoadLocal(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		if (!s3_execute_load_local(index))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_checkSaveExists(
	void *p)
{
	int index;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		val = s3_check_save_exists(index);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;
		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_deleteLocalSave(
	void *p)
{
	int index;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		s3_delete_local_save(index);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_deleteGlobalSave(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_delete_global_save();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_checkRightAfterLoad(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_check_right_after_load();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_getSaveTimestamp(
	void *p)
{
	int index;
	uint64_t timestamp;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		timestamp = s3_get_save_timestamp(index);

		/* Set the return value. */
		if (!pf_set_return_int((int)timestamp))
			break;

		ret = true;	
	} while (0);

	return ret;
}

static bool
Suika_getLatestSaveIndex(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_latest_save_index();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getSaveChapterName(
	void *p)
{
	int index;
	const char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		name = s3_get_save_chapter_name(index);
		if (name == NULL)
			name = "";

		/* Set the return value. */
		if (!pf_set_return_string(name))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getSaveLastMessage(
	void *p)
{
	int index;
	const char *message;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		message = s3_get_save_last_message(index);
		if (message == NULL)
			message = "";

		/* Set the return value. */
		if (!pf_set_return_string(message))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getSaveThumbnail(
	void *p)
{
	int index;
	struct s3_image *img;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		img = s3_get_save_thumbnail(index);
		if (img == NULL)
			break;

		val = s3i_image_to_int(img);

		/* Set the return value. */
		if (!pf_set_return_int(val))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * History
 */

static bool
Suika_clearHistory(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_clear_history();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_addHistory(
	void *p)
{
	char *name;
	char *message;
	char *voice;
	int body_color;
	int body_outline_color;
	int name_color;
	int name_outline_color;
	bool ret;

	UNUSED_PARAMETER(p);

	name = NULL;
	message = NULL;
	voice = NULL;
	ret = false;
	do {
		/* Get the arguments. */
		if (!pf_get_call_arg_string("name", &name))
			break;
		if (!pf_get_call_arg_string("message", &message))
			break;
		if (!pf_get_call_arg_string("voice", &voice))
			break;
		if (!pf_get_call_arg_int("bodyColor", &body_color))
			break;
		if (!pf_get_call_arg_int("bodyOutlineColor", &body_outline_color))
			break;
		if (!pf_get_call_arg_int("nameColor", &name_color))
			break;
		if (!pf_get_call_arg_int("nameOutlineColor", &name_outline_color))
			break;

		if (!s3_add_history(name,
				    message,
				    voice,
				    (s3_pixel_t)body_color,
				    (s3_pixel_t)body_outline_color,
				    (s3_pixel_t)name_color,
				    (s3_pixel_t)name_outline_color))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (name != NULL)
		free(name);
	if (message != NULL)
		free(message);
	if (voice != NULL)
		free(voice);

	return ret;
}

static bool
Suika_getHistoryCount(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_get_history_count();

	/* Set the return value. */
	if (!pf_set_return_int(val))
		return false;

	return true;
}

static bool
Suika_getHistoryName(
	void *p)
{
	int index;
	const char *name;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		name = s3_get_history_name(index);
		if (name == NULL)
			name = "";

		/* Set the return value. */
		if (!pf_set_return_string(name))
			break;

		ret = true;
	} while (0);
	return ret;
}

static bool
Suika_getHistoryMessage(
	void *p)
{
	int index;
	const char *message;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		message = s3_get_history_message(index);
		if (message == NULL)
			message = "";

		/* Set the return value. */
		if (!pf_set_return_string(message))
			break;

		ret = true;
	} while (0);

	return ret;
}

static bool
Suika_getHistoryVoice(
	void *p)
{
	int index;
	const char *voice;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("index", &index))
			break;

		voice = s3_get_history_voice(index);
		if (voice == NULL)
			voice = "";

		/* Set the return value. */
		if (!pf_set_return_string(voice))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * Seen
 */

static bool
Suika_loadSeen(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_load_seen())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_saveSeen(
	void *p)
{
	UNUSED_PARAMETER(p);

	if (!s3_save_seen())
		return false;

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_getSeenFlags(
	void *p)
{
	int flags;

	UNUSED_PARAMETER(p);

	flags = s3_get_seen_flags();

	/* Set the return value. */
	if (!pf_set_return_int(flags))
		return false;

	return true;
}

static bool
Suika_setSeenFlags(
	void *p)
{
	int flags;
	bool ret;

	UNUSED_PARAMETER(p);

	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_int("flags", &flags))
			break;

		s3_set_seen_flags(flags);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	return ret;
}

/*
 * GUI
 */

static bool
Suika_checkRightAfterSysGUI(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_check_right_after_sys_gui();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_loadGUIFile(
	void *p)
{
	char *file;
	int is_sys;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_int("isSys", &is_sys))
			break;

		if (!s3_load_gui_file(file, is_sys ? true : false))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_startGUI(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_start_gui();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_stopGUI(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_stop_gui();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_isGUIRunning(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_gui_running();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_isGUIFinished(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_gui_finished();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_getGUIResultLabel(
	void *p)
{
	const char *val;

	UNUSED_PARAMETER(p);

	val = s3_get_gui_result_label();
	if (val == NULL)
		val = "";

	/* Set the return value. */
	if (!pf_set_return_string(val))
		return false;

	return true;
}

static bool
Suika_isGUIResultTitle(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_gui_result_title();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_checkIfSavedInGUI(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_check_if_saved_in_gui();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_checkIfLoadedInGUI(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_check_if_loaded_in_gui();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

/*
 * HAL
 */

static bool
Suika_getMillisec(
	void *p)
{
	uint64_t lap;

	UNUSED_PARAMETER(p);

	lap = pf_get_lap_timer_millisec(&time_origin);

	/* Note: Conversionto to int is intentional. */
	if (!pf_set_return_int((int)(uint32_t)lap))
		return false;

	return true;
}

static bool
Suika_checkFileExists(
	void *p)
{
	char *file;
	int val;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;

		val = s3_check_file_exists(file);

		/* Set the return value. */
		if (!pf_set_return_int(val ? 1 : 0))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_readFileContent(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * TODO: Not Implemented
	 *
	 * This is intentional.
	 * NoctLang does not support binary data yet.
	 */
	s3_log_error(S3_TR("This API is not implemented yet."));
	return false;
}

static bool
Suika_writeSaveData(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * TODO: Not Implemented
	 *
	 * This is intentional.
	 * NoctLang does not support binary data yet.
	 */
	s3_log_error(S3_TR("This API is not implemented yet."));
	return false;
}

static bool
Suika_readSaveData(
	void *p)
{
	UNUSED_PARAMETER(p);

	/*
	 * TODO: Not Implemented
	 *
	 * This is intentional.
	 * NoctLang does not support binary data yet.
	 */
	s3_log_error(S3_TR("This API is not implemented yet."));
	return false;
}

static bool
Suika_playVideo(
	void *p)
{
	char *file;
	int is_skippable;
	bool ret;

	UNUSED_PARAMETER(p);

	file = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("file", &file))
			break;
		if (!pf_get_call_arg_int("isSkippable", &is_skippable))
			break;
		
		if (!s3_play_video(file, is_skippable ? true : false))
			break;

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (file != NULL)
		free(file);

	return ret;
}

static bool
Suika_stopVideo(
	void *p)
{
	UNUSED_PARAMETER(p);

	s3_stop_video();

	/* Set the return value. */
	if (!pf_set_return_int(1))
		return false;

	return true;
}

static bool
Suika_isVideoPlaying(
	void *p)
{
	int val;

	UNUSED_PARAMETER(p);

	val = s3_is_video_playing();

	/* Set the return value. */
	if (!pf_set_return_int(val ? 1 : 0))
		return false;

	return true;
}

static bool
Suika_speakText(
	void *p)
{
	char *text;
	bool ret;

	UNUSED_PARAMETER(p);

	text = NULL;
	ret = false;
	do {
		/* Get the argument. */
		if (!pf_get_call_arg_string("text", &text))
			break;

		s3_speak_text(text);

		/* Set the return value. */
		if (!pf_set_return_int(1))
			break;

		ret = true;
	} while (0);

	if (text != NULL)
		free(text);

	return ret;
}
