#include "doomkeys.h"
#include "doomgeneric.h"
#include "syscon.h"
#include "rtc.h"
#include "fb.h"
#include "qemu_dma.h"
#include "virtio_keyboard.h"
#include "virt_clint.h"

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
  
  if (ramfb_setup(&fb) != 0){
    printf("DG_Init: error setting up ramfb \n");
	poweroff();
	return;
  }
  printf("DG_Init: setup ramfb successfull\n");

  int res =virtio_keyboard_init();
  if (res < 0 ) {
	printf("DG_Init: error during virtio keyboad init [%d] - abort\n", res);
	poweroff();
	return;
  }
  printf("DG_Init: virtio keyboad init successfully\n");

  if (init_interrupts() < 0) {
	poweroff();
	return;
  }
  printf("DG_Init: interrupts setup completed successfully\n");
}

void DG_DrawFrame()
{
	//printf("DG_DrawFrame\n");
	draw_frame((uint32_t*)DG_ScreenBuffer, &fb);
}

void DG_SleepMs(uint32_t ms)
{
	//printf("DG_SleepMs: ms [%d]\n", ms);
	//kusleep(ms * 1000);
	sleep_us(ms * 1000);
}

uint32_t DG_GetTicksMs()
{
	//printf("DG_GetTicksMs\n");
	uint64_t ticks = kmtime();
	return ticks / 10000;
}


// convert console extended char into doomkey
unsigned char convert_to_doomkey(int code) {
	unsigned char key;
	switch (code)
	{
		
	// override keys
	/* case 1: // ESC
		key = KEY_ESCAPE;
		break; */

	default:
		key = at_to_doom[code];
		break;
	}
	return key;
}

int DG_GetKey(int* pressed, unsigned char* doomKey)
{
	struct virtio_input_event key_event = virtio_keyboard_read_event();
	if (key_event.type == VirtioInputEvNone)
		return 0; // no key event detected
	unsigned char key = convert_to_doomkey(key_event.code);
	//printf("DG_GetKey: key_event.code [%d] -> doomkey [%d], key_event.value [%d]\n", key_event.code, key, key_event.value);
	*doomKey = key;
	*pressed = key_event.value;
	return 1; // key event detected
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