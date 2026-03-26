/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Pseudo gettext support.
 */

#if defined(NOCT_USE_TRANSLATION)

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#ifdef _WIN32
#include <windows.h>
#endif

/*
 * Language code for translation.
 */
const char *noct_lang_code = "en";

/*
 * Called from noct_gettext().
 */
const char *noct_get_system_language(void)
{
	return noct_lang_code;
}

/*
 * Initialized the locale.
 */
void noct_init_locale(void)
{
#if !defined(_WIN32)
	const char *locale = setlocale(LC_MESSAGES, "");
        if (locale == NULL || locale[0] == '\0') {
		locale = getenv("LC_ALL");
		if (locale == NULL || locale[0] == '\0') {
			locale = getenv("LC_MESSAGES");
			if (locale == NULL || locale[0] == '\0')
				locale = getenv("LANG");
		}
	}
	if (locale == NULL || locale[0] == '\0')
		noct_lang_code ="en";
	/* English */
	else if (strncmp(locale, "en_AU", 5) == 0)
		noct_lang_code = "en-au";
	else if (strncmp(locale, "en_GB", 5) == 0)
		noct_lang_code = "en-gb";
	else if (strncmp(locale, "en_NZ", 5) == 0)
		noct_lang_code = "en-nz";
	else if (strncmp(locale, "en_US", 5) == 0)
		noct_lang_code = "en-us";
	else if (strncmp(locale, "en", 2) == 0)
		noct_lang_code = "en";
	/* French */
	else if (strncmp(locale, "fr_CA", 5) == 0)
		noct_lang_code = "fr-ca";
	else if (strncmp(locale, "fr", 2) == 0)
		noct_lang_code = "fr-fr";
	/* Spanish */
	else if (strncmp(locale, "es_ES", 5) == 0)
		noct_lang_code = "es-es";
	else if (strncmp(locale, "es", 2) == 0)
		noct_lang_code = "es-la";
	/* Chinese */
	else if (strncmp(locale, "zh_TW", 5) == 0 ||
		 strncmp(locale, "zh_HK", 5) == 0)
		noct_lang_code = "zh-tw";
	else if (strncmp(locale, "zh", 2) == 0)
		noct_lang_code = "zh-cn";
	/* Others */
	else if (strncmp(locale, "ja", 2) == 0)
		noct_lang_code = "ja";
	else if (strncmp(locale, "de", 2) == 0)
		noct_lang_code = "de";
	else if (strncmp(locale, "it", 2) == 0)
		noct_lang_code = "it";
	else if (strncmp(locale, "el", 2) == 0)
		noct_lang_code = "el";
	else if (strncmp(locale, "ru", 2) == 0)
		noct_lang_code = "ru";
	else if (strncmp(locale, "ko", 2) == 0)
		noct_lang_code = "ko";
	/* Fallback */
	else
		noct_lang_code = "en";
#else
	LCID lcid = GetUserDefaultLCID();
	WORD langId = PRIMARYLANGID(lcid);
	WORD sublangId = SUBLANGID(lcid);

	switch (langId) {
	case LANG_ENGLISH:
		switch (sublangId) {
		case SUBLANG_ENGLISH_US:  noct_lang_code = "en-us"; break;
		case SUBLANG_ENGLISH_UK:  noct_lang_code = "en-gb"; break;
		case SUBLANG_ENGLISH_AUS: noct_lang_code = "en-au"; break;
		case SUBLANG_ENGLISH_NZ:  noct_lang_code = "en-nz"; break;
		default:                  noct_lang_code = "en"; break;
		}
	case LANG_FRENCH:
		switch (sublangId) {
		case SUBLANG_FRENCH:          noct_lang_code = "fr-fr"; break;
		case SUBLANG_FRENCH_CANADIAN: noct_lang_code = "fr-ca"; break;
		default:                      noct_lang_code = "fr"; break;
		}
	case LANG_SPANISH:
		switch (sublangId) {
		case SUBLANG_SPANISH:         noct_lang_code = "es-es"; break;
		case SUBLANG_SPANISH_MODERN:  noct_lang_code = "es-es"; break;
		default:                      noct_lang_code = "es-la"; break;
		}
	case LANG_GERMAN:
		noct_lang_code = "de";
		break;
	case LANG_ITALIAN:
		noct_lang_code = "it";
		break;
	case LANG_GREEK:
		noct_lang_code = "el";
		break;
	case LANG_RUSSIAN:
		noct_lang_code = "ru";
		break;
	case LANG_JAPANESE:
		noct_lang_code = "ja";
		break;
	case LANG_CHINESE:
		switch (sublangId) {
		case SUBLANG_CHINESE_SIMPLIFIED:  noct_lang_code = "zh-cn"; break;
		case SUBLANG_CHINESE_HONGKONG:    noct_lang_code = "zh-tw"; break;
		case SUBLANG_CHINESE_TRADITIONAL: noct_lang_code = "zh-tw"; break;
		default:                          noct_lang_code = "zh-cn"; break;
		}
	default:
		break;
	}
	noct_lang_code = "en";
#endif	/* !defined(_WIN32) */
}

#endif	/* defined(NOCT_USE_TRANSLATION) */
