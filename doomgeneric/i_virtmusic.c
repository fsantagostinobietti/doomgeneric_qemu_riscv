//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//	Qemu Virt System interface for music.
//


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#include "doomtype.h"
#include "i_sound.h"



// Shutdown music
static void I_Virt_ShutdownMusic(void)
{
	// nothing here
	// TODO: stop song?
}


// Initialize music subsystem
static boolean I_Virt_InitMusic(void)
{
	// not implemented

	return true;
}


// Set music volume (0 - 127)
static void I_Virt_SetMusicVolume(int volume)
{
	// not implemented
}


// Start playing a mid
static void I_Virt_PlaySong(void *handle, boolean looping)
{
	// not implemented
}


static void I_Virt_PauseSong(void)
{
	// not implemented
}


static void I_Virt_ResumeSong(void)
{
	// not implemented
}


static void I_Virt_StopSong(void)
{
	// not implemented
}


static void I_Virt_UnRegisterSong(void *handle)
{
	// not implemented
}

static void *I_Virt_RegisterSong(void *data, int len)
{
	// not implemented
	return NULL;
}


// Is the song playing?
static boolean I_Virt_MusicIsPlaying(void)
{
	// not implemented
}


// Poll music position; if we have passed the loop point end position
// then we need to go back.
static void I_Virt_PollMusic(void)
{
	// not implemented
}


static snddevice_t music_virt_devices[] =
{
	SNDDEVICE_PAS,
	SNDDEVICE_GUS,
	SNDDEVICE_WAVEBLASTER,
	SNDDEVICE_SOUNDCANVAS,
	SNDDEVICE_GENMIDI,
	SNDDEVICE_AWE32,
};


music_module_t DG_music_module =
{
	music_virt_devices,
	arrlen(music_virt_devices),
	I_Virt_InitMusic,
	I_Virt_ShutdownMusic,
	I_Virt_SetMusicVolume,
	I_Virt_PauseSong,
	I_Virt_ResumeSong,
	I_Virt_RegisterSong,
	I_Virt_UnRegisterSong,
	I_Virt_PlaySong,
	I_Virt_StopSong,
	I_Virt_MusicIsPlaying,
	I_Virt_PollMusic,
};

