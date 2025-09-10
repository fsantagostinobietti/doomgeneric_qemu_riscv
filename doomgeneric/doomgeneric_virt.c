#include "doomkeys.h"
#include "doomgeneric.h"
#include "syscon.h"
#include "rtc.h"
#include "fb.h"
#include "qemu_dma.h"

#include <stdio.h>

fb_info fb; // global video framebuffer

void DG_Init()
{
	printf("DG_Init\n");

	printf("DG_Init: DG_ScreenBuffer [%p]\n", DG_ScreenBuffer);

	if (check_fw_cfg_dma()) {
		printf("DG_Init: guest fw_cfg dma-interface enabled \n");
	} else {
		printf("DG_Init: guest fw_cfg dma-interface NOT enabled - abort \n");
		poweroff();
		return;
	}
  
  uint32_t fb_width = 640;
  uint32_t fb_height = 400;
  uint32_t fb_bpp = 4;
  uint32_t fb_stride = fb_bpp * fb_width;

  fb.fb_addr = (uint64_t) malloc(fb_stride * fb_height);
  fb.fb_width = fb_width;
  fb.fb_height = fb_height;
  fb.fb_bpp = fb_bpp;
  fb.fb_stride = fb_stride;
  fb.fb_size = fb_stride * fb_height;
  
  if (ramfb_setup(&fb) != 0)
    printf("DG_Init: error setting up ramfb \n");
  printf("DG_Init: setup ramfb successfull\n");

  printf("DG_Init: DG_ScreenBuffer2 [%p]\n", DG_ScreenBuffer);
}

void DG_DrawFrame()
{
	printf("DG_DrawFrame\n");
	draw_frame((uint32_t*)DG_ScreenBuffer, &fb);
}

void DG_SleepMs(uint32_t ms)
{
	//printf("DG_SleepMs: ms [%d]\n", ms);
	kusleep(ms * 1000);
}

uint32_t DG_GetTicksMs()
{
	//printf("DG_GetTicksMs\n");
	uint64_t ticks = kmtime();
	return ticks / 10000;
}

// convert console extended char into doomkey
unsigned char convert_to_doomkey(int ch) {
	unsigned char key;
	switch (ch)
	{
	case 32: // SPACE (use objects)
		key = KEY_USE;
		break;
	case 119: // 'w' (up)
		key = KEY_UPARROW;
		break;
	case 115: // 's' (down)
		key = KEY_DOWNARROW;
		break;
	case 97: // 'a' (left)
		key = KEY_LEFTARROW;
		break;
	case 100: // 'd' (right)
		key = KEY_RIGHTARROW;
		break;
	case 45: // '-' (fire)
		key = KEY_FIRE;
		break;
	
	default:
		key = ch;
		break;
	}
	return key;
}

// global - current pressed doomkey value
int pressed_key = 0;
int next_pressed_key = 0;
uint64_t received_char_ticks = 0;

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	//printf("DG_GetKey\n");
	if ( next_pressed_key != 0 ) { // unprocessed event found
		*pressed = 1;
		*doomKey = next_pressed_key;
		printf("DG_GetKey: pressed doomkey value [%d]\n", *doomKey);
		// store doomkey pressed value
		pressed_key = *doomKey;
		// start ticker
		received_char_ticks = kmtime();

		next_pressed_key = 0;
		return 1;
	}

	if ( received_char_ticks != 0 && (kmtime()-received_char_ticks) < 80*10000 ) { // delta < 80 ms
		// do nothing
		return 0; // no key event detected
	}

	int ch; // = kreadchar();
	while ( (ch = kreadchar()) != -1 ) {} // consider last char in uart buffer 

	if (ch == -1) { // no key
		if (pressed_key!=0) {
			printf("DG_GetKey: release doomkey value [%d]\n", pressed_key);
			*pressed = 0;
			*doomKey = pressed_key;
			// reset pressed key
			pressed_key = 0;
			// reset ticks time of pressed event
			received_char_ticks = 0;
			return 1;  // key released event detected
		}
		return 0; // no key event detected
	}	
    printf("DG_GetKey: key value [%d]\n", ch);
	
	unsigned char key = convert_to_doomkey(ch);
	
	if (key == pressed_key) { // key already pressed
		printf("DG_GetKey: already pressed doomkey [%d]\n", key);
		received_char_ticks = kmtime();  // restart ticker
		return 0; // no event
	}

	// TODO handle key != pressed_key when pressed_key != 0
	if (pressed_key != 0) {
		printf("DG_GetKey: release doomkey value [%d]\n", pressed_key);
		*pressed = 0;
		*doomKey = pressed_key;
		// reset pressed key
		pressed_key = 0;
		// reset ticks time of pressed event
		received_char_ticks = 0;

		// store next pressed doomkey
		next_pressed_key = key;
		return 1;  // key released event detected
	}
	//printf("DG_GetKey: ERROR unhandled 'pressed_key != 0' condition\n");

	*pressed = 1;
	*doomKey = key;
	printf("DG_GetKey: pressed doomkey value [%d]\n", *doomKey);
	// store doomkey pressed value
	pressed_key = *doomKey;
	// start ticker
	received_char_ticks = kmtime();

	return 1; // key pressed event detected
}

void DG_SetWindowTitle(const char * title)
{
	printf("DG_SetWindowTitle\n");
}

int main(int argc, char **argv)
{
	printf("main\n");
    doomgeneric_Create(argc, argv);

    while (1)
    {
        doomgeneric_Tick();
    }
    

	/* printf("poweroff\n");
	poweroff(); */
    return 0;
}