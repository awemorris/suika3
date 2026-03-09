/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Copyright (c) 2025, Awe Morris. All rights reserved.
 */

/*
 * Pseudo gettext support.
 */

#if defined(NOCT_USE_TRANSLATION)

#include <stdio.h>
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
	const char *locale;
	locale = setlocale(LC_ALL, "");
	if (locale == NULL || locale[0] == '\0' || locale[1] == '\0')
		noct_lang_code = "en";
	else if (strncmp(locale, "en", 2) == 0)
		noct_lang_code = "en";
	else if (strncmp(locale, "fr", 2) == 0)
		noct_lang_code = "fr";
	else if (strncmp(locale, "de", 2) == 0)
		noct_lang_code = "de";
	else if (strncmp(locale, "it", 2) == 0)
		noct_lang_code = "it";
	else if (strncmp(locale, "es", 2) == 0)
		noct_lang_code = "es";
	else if (strncmp(locale, "el", 2) == 0)
		noct_lang_code = "el";
	else if (strncmp(locale, "ru", 2) == 0)
		noct_lang_code = "ru";
	else if (strncmp(locale, "zh_CN", 5) == 0)
		noct_lang_code = "zh";
	else if (strncmp(locale, "zh_TW", 5) == 0)
		noct_lang_code = "tw";
	else if (strncmp(locale, "ja", 2) == 0)
		noct_lang_code = "ja";
	else
		noct_lang_code = "en";
	setlocale(LC_ALL, "C");
#else
	DWORD dwLang = GetUserDefaultLCID() & 0x3ff;
	switch (dwLang) {
	case LANG_ENGLISH:
		noct_lang_code = "en";
		break;
	case LANG_FRENCH:
		noct_lang_code = "fr";
		break;
	case LANG_GERMAN:
		noct_lang_code = "de";
		break;
	case LANG_SPANISH:
		noct_lang_code = "es";
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
	case LANG_CHINESE_SIMPLIFIED:
		noct_lang_code = "zh";
		break;
	case LANG_CHINESE_TRADITIONAL:
		noct_lang_code = "tw";
		break;
	case LANG_JAPANESE:
		noct_lang_code = "ja";
		break;
	default:
		noct_lang_code = "en";
		break;
	}
#endif	/* !defined(_WIN32) */
}

#endif	/* defined(NOCT_USE_TRANSLATION) */
