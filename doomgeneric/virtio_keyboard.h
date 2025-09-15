#ifndef __VIRTIO_KEYBOAD__
#define __VIRTIO_KEYBOAD__

#include  <stdint.h>

// Input event
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-4240006
struct virtio_input_event {
    uint16_t type;
    uint16_t code;
    uint32_t value;
} __attribute__((packed));  // no extra padding between the struct members

// event types
enum {
    VirtioInputEvSyn = 0,
	VirtioInputEvKey = 1,   // key event
	VirtioInputEvRel = 2,
	VirtioInputEvAbs = 3,
	VirtioInputEvRep = 4,
    VirtioInputEvNone = 999,    // no event (custom value)
};


int virtio_keyboard_init();

struct virtio_input_event virtio_keyboard_read_event(void);

#endif