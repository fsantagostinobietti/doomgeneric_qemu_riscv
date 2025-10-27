//
// Copyright(C) 1993-1996 Id Software, Inc.
// Copyright(C) 2005-2014 Simon Howard
// Copyright(C) 2008 David Flater
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
//	Qemu Virt System interface for sound.
//

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>

#include "deh_str.h"
#include "i_sound.h"
#include "i_system.h"
#include "i_swap.h"
#include "m_argv.h"
#include "m_misc.h"
#include "w_wad.h"
#include "z_zone.h"

#include "doomtype.h"

// audio clip
struct audio_clip {
    uint8_t* pcm; // address
    uint32_t pcm_sz;  // size in bytes
};

#define NUM_CHANNELS 16

#define NUM_MIDI_CHANNELS 16


static boolean sound_initialized = false;

static sfxinfo_t *channels_playing[NUM_CHANNELS];

static int allegro_voices[NUM_CHANNELS];

static boolean use_sfx_prefix;


// We don't support libsamplerate with Virt but these have to be here since
// other code requires them
int use_libsamplerate = 0;

// Scale factor used when converting libsamplerate floating point numbers
// to integers. Too high means the sounds can clip; too low means they
// will be too quiet. This is an amount that should avoid clipping most
// of the time: with all the Doom IWAD sound effects, at least. If a PWAD
// is used, clipping might occur.

float libsamplerate_scale = 0.65f;


static void GetSfxLumpName(sfxinfo_t *sfx, char *buf, size_t buf_len)
{
	// Linked sfx lumps? Get the lump number for the sound linked to.

	if (sfx->link != NULL)
	{
		sfx = sfx->link;
	}

	// Doom adds a DS* prefix to sound lumps; Heretic and Hexen don't
	// do this.

	if (use_sfx_prefix)
	{
		M_snprintf(buf, buf_len, "ds%s", DEH_String(sfx->name));
	}
	else
	{
		M_StringCopy(buf, DEH_String(sfx->name), buf_len);
	}
}


// Load and convert a sound effect
// Returns true if successful

static boolean CacheSFX(sfxinfo_t *sfxinfo)
{
	int lumpnum;
	unsigned int lumplen;
	int samplerate;
	unsigned int length;
	byte *data;

	// need to load the sound

	lumpnum = sfxinfo->lumpnum;
	data = W_CacheLumpNum(lumpnum, PU_STATIC);
	lumplen = W_LumpLength(lumpnum);

	// Check the header, and ensure this is a valid sound

	if (lumplen < 8
	 || data[0] != 0x03 || data[1] != 0x00)
	{
		// Invalid sound

		return false;
	}

	// 16 bit sample rate field, 32 bit length field

	samplerate = (data[3] << 8) | data[2];
	length = (data[7] << 24) | (data[6] << 16) | (data[5] << 8) | data[4];

	// If the header specifies that the length of the sound is greater than
	// the length of the lump itself, this is an invalid sound lump

	// We also discard sound lumps that are less than 49 samples long,
	// as this is how DMX behaves - although the actual cut-off length
	// seems to vary slightly depending on the sample rate.  This needs
	// further investigation to better understand the correct
	// behavior.

	if (length > lumplen - 8 || length <= 48)
	{
		return false;
	}

	// The DMX sound library seems to skip the first 16 and last 16
	// bytes of the lump - reason unknown.

	data += 16;
	length -= 32;

	// Debug only
	char namebuf[9];
	GetSfxLumpName(sfxinfo, namebuf, sizeof(namebuf));
	printf("CacheSFX: sfx [%s], samplerate [%d], length [%d]\n", namebuf, samplerate, length);
	/* if (strcasecmp(namebuf, "dsshotgn")==0) {
		for (int i=0; i<length ;++i)
			printf("0x%x, ", data[i]);
		printf("\n");
	} */

	// TODO
	// memcpy(sample->data, data, length);
	//  sfxinfo->driver_data = sample;
	struct audio_clip* audio = malloc(sizeof(struct audio_clip));
	audio->pcm = data;
	audio->pcm_sz = length;
	sfxinfo->driver_data = audio;

	// don't need the original lump any more
	W_ReleaseLumpNum(lumpnum);

	return true;
}


static void I_Virt_PrecacheSounds(sfxinfo_t *sounds, int num_sounds)
{
	printf("I_Virt_PrecacheSounds: num_sounds [%d]\n", num_sounds);
	char namebuf[9];
	int i;

	printf("I_Virt_PrecacheSounds: Precaching all sound effects..");

	for (i=0; i<num_sounds; ++i)
	{
		if ((i % 6) == 0)
		{
			printf(".");
			fflush(stdout);
		}

		GetSfxLumpName(&sounds[i], namebuf, sizeof(namebuf));

		sounds[i].lumpnum = W_CheckNumForName(namebuf);

		if (sounds[i].lumpnum != -1)
		{
			if (!CacheSFX(&sounds[i]))
				printf("I_Virt_PrecacheSounds: error on CacheSFX() for sound [%s]\n", namebuf);
		}
	}

	printf("\n");
}


//
// Retrieve the raw data lump index
//  for a given SFX name.
//

static int I_Virt_GetSfxLumpNum(sfxinfo_t *sfx)
{
	printf("I_Virt_GetSfxLumpNum\n");
	char namebuf[9];

	GetSfxLumpName(sfx, namebuf, sizeof(namebuf));

	return W_GetNumForName(namebuf);
}

// vol - volume [0, 127]
//       0: silence, 127: max volume
// sep - stereo separation (aka panning) [0, 254]
//       0: all audio on the left, 128: equal distribution, 254: all audio on the right


static void I_Virt_UpdateSoundParams(int handle, int vol, int sep)
{
	printf("I_Virt_UpdateSoundParams: handle [%d], vol [%d], sep [%d]\n", handle, vol, sep);
	int left, right;

	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return;
	}

	if (channels_playing[handle] == NULL) {
		return;
	}

	// TODO sett volume and pan (i.e. sep)
}

//
// Starting a sound means adding it
//  to the current list of active sounds
//  in the internal channels.
// As the SFX info struct contains
//  e.g. a pointer to the raw data,
//  it is ignored.
// As our sound handling does not handle
//  priority, it is ignored.
// Pitching (that is, increased speed of playback)
//  is set, but currently not used by mixing.
//

static int I_Virt_StartSound(sfxinfo_t *sfxinfo, int channel, int vol, int sep)
{
	printf("I_Virt_StartSound: name [%s], driver_data [%p], channel [%d], vol [%d], sep [%d]\n", sfxinfo->name, sfxinfo->driver_data, channel, vol, sep);
	if (!sound_initialized || channel < 0 || channel >= NUM_CHANNELS)
	{
		return -1;
	}

	// Release a sound effect if there is already one playing
	// on this channel
	if (channels_playing[channel]) {
		// TODO
		channels_playing[channel] = NULL;
	}

	// Get the sound data
	if (sfxinfo->driver_data == NULL)
	{
		if (!CacheSFX(sfxinfo))
		{
			return -1;
		}
	}
	//assert(sfxinfo->driver_data);
	printf("I_Virt_StartSound: name [%s], pcm addr [%p]\n", sfxinfo->name, ((struct audio_clip*)sfxinfo->driver_data)->pcm);
	
	// play sound
	// TODO

	channels_playing[channel] = sfxinfo;

	return channel;
}


static void I_Virt_StopSound(int handle)
{
	printf("I_Virt_StopSound: handle [%d]\n", handle);
	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return;
	}

	if (channels_playing[handle] == NULL) {
		return;
	}

	// TODO

	channels_playing[handle] = NULL;
}


static boolean I_Virt_SoundIsPlaying(int handle)
{
	printf("I_Virt_SoundIsPlaying: handle [%d]\n", handle);
	if (!sound_initialized || handle < 0 || handle >= NUM_CHANNELS)
	{
		return false;
	}

	if (channels_playing[handle] == NULL) {
		return false;
	}

	// TODO

	// still playing
	return true;
}

// 
// Periodically called to update the sound system
//

static void I_Virt_UpdateSound(void)
{
	//printf("I_Virt_UpdateSound\n");
	int i;

	// loop through all channels which have sample, check if they're finished
	for (i = 0; i < NUM_CHANNELS; i++) {
		if (channels_playing[i] && !I_Virt_SoundIsPlaying(i)) {
			// TODO 
			// finished
			channels_playing[i] = NULL;
		}
	}
}


static void I_Virt_ShutdownSound(void)
{
	printf("I_Virt_ShutdownSound\n");
	if (!sound_initialized)
	{
		return;
	}

	// TODO

	sound_initialized = false;
}


static boolean I_Virt_InitSound(boolean _use_sfx_prefix)
{
	printf("I_Virt_InitSound: _use_sfx_prefix [%d]\n", _use_sfx_prefix);
	int i;

    use_sfx_prefix = _use_sfx_prefix;

    // No sounds yet

    for (i=0; i<NUM_CHANNELS; ++i)
    {
        channels_playing[i] = NULL;
    }

	// TODO 

	sound_initialized = true;

	return true;
}


static snddevice_t sound_virt_devices[] =
{
	SNDDEVICE_SB,  // only to be recognised as a valid sound device
	SNDDEVICE_VIRTIO,
};


sound_module_t DG_sound_module = 
{
	sound_virt_devices,
	arrlen(sound_virt_devices),
	I_Virt_InitSound,
	I_Virt_ShutdownSound,
	I_Virt_GetSfxLumpNum,
	I_Virt_UpdateSound,
	I_Virt_UpdateSoundParams,
	I_Virt_StartSound,
	I_Virt_StopSound,
	I_Virt_SoundIsPlaying,
	I_Virt_PrecacheSounds,
};

