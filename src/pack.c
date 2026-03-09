/* -*- coding: utf-8; tab-width: 8; indent-tabs-mode: t; -*- */

/*
 * Playfield Engine
 * Archiver main
 */

/*-
 * SPDX-License-Identifier: Zlib
 *
 * Playfield Engine
 * Copyright (c) 2025-2026 Awe Morris
 *
 * This software is derived from the codebase of Suika2.
 * Copyright (c) 1996-2024 Keiichi Tabata
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

int command_archive(int argc, char *argv[]);

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char *argv[])
{
#ifdef _WIN32
	if (argc == 1)
	{
		MessageBoxW(NULL,
			    L"Please specify files and folders in relative paths to make an archive file.",
			    L"Playfield Archiver",
			    MB_OK | MB_ICONINFORMATION);
		return 1;
	}
	else
	{
		MessageBoxW(NULL,
			    L"Playfield Archiver will make an archive file. This will take a while.",
			    L"Playfield Archiver",
			    MB_OK | MB_ICONINFORMATION);
	}
#endif

	int ret = command_archive(argc, argv);

#ifdef _WIN32
	if (ret == 0)
	{
		MessageBoxW(NULL,
			    L"Done making the assets.arc file.",
			    L"Playfield Archiver",
			    MB_OK | MB_ICONINFORMATION);
	}
	else
	{
		MessageBoxW(NULL,
			    L"Error(s) on making the assets.arc file. See the console.",
			    L"Playfield Archiver",
			    MB_OK | MB_ICONINFORMATION);
	}
#endif

	return ret;
}
