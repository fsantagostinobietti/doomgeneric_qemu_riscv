// virtio_keyboard.c
// Minimal freestanding virtio-MMIO (Virtio over MMIO) keyboard driver for QEMU “virt”

#include <stdint.h>
#include <stddef.h>

#include "uart_serial.h"
#include "virtio.h"
#include "virtio_keyboard.h"


// Static allocation of virtqueue (aligned and contiguous in physical memory)
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-350007
static struct virtq_desc    desc[QUEUE_SIZE]          __attribute__((aligned(16)));
static struct virtq_avail   avail                     __attribute__((aligned(2)));
static struct virtq_used    used                      __attribute__((aligned(4)));
static struct virtio_input_event buffers[QUEUE_SIZE];

// Track next used slot
static uint16_t used_idx  = 0;

//-------------------------------------------------------------
// Initialize the virtio-keyboard device
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-1230001
int virtio_keyboard_init(void)
{
    // detect keyboard input device
    // i.e. index of 'virtio_mmio_devices[]'
    int dev_idx = detect_virtio_device(VIRTIO_DEVICE_ID_INPUT);
    if ( dev_idx < 0 ) {
        kprintf("virtio_keyboard_init: keyboard device not found\n");
        return -1;
    }
    //print_device_stats(dev_idx);

    // Reset status & set it as acknowledge driver
    virtio_mmio_devices[dev_idx].status = 0; // reset status
    virtio_mmio_devices[dev_idx].status = VIRTIO_STATUS_ACKNOWLEDGE;
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_DRIVER;
    //kprintf("1. Virtio device status [%x]\n", virtio_mmio_devices[dev_idx].status);

    // Driver feature negotiation (we request no optional features)
    virtio_mmio_devices[dev_idx].driverFeaturesSel = 1;  // select high 32 bits
    virtio_mmio_devices[dev_idx].driverFeatures = 0;
    virtio_mmio_devices[dev_idx].driverFeaturesSel = 0; // select low 32 bits
    virtio_mmio_devices[dev_idx].driverFeatures = 0; 
    
    // inform feature setting is done
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_FEATURES_OK;
    //kprintf("2. Virtio device status [%x]\n", virtio_mmio_devices[dev_idx].status);
    // Confirm FEATURES_OK
    if (virtio_mmio_devices[dev_idx].status & VIRTIO_STATUS_FEATURES_OK == 0)
        return -2;

    // Setup single virtqueue (id==0)
    if (setup_queue(dev_idx, 0, desc, &avail, &used) < 0)
        return -3;
    
    // Populate the “available” ring with our buffers
    for (uint16_t i = 0; i < QUEUE_SIZE; i++) {
        desc[i].addr  = (uint64_t)(uintptr_t)&buffers[i];
        desc[i].len   = sizeof(buffers[i]);
        desc[i].flags = VIRTQ_DESC_F_WRITE;   // device writes events into us
        desc[i].next  = 0;

        avail.ring[i] = i;
    }
    // ask device not to use interrupt on new events. we'll pool for new events
    avail.flags = VIRTQ_AVAIL_F_NO_INTERRUPT;

    wmb();  // Make desc/ring visible first

    // Publish initial avail.idx
    avail.idx   = QUEUE_SIZE;

    io_wmb(); // Ensure idx update is visible before MMIO notify

    // Finally tell device we’re fully up
    kprintf("Notify device driver is ok\n");
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_DRIVER_OK;

    return dev_idx;
}


//-------------------------------------------------------------
// Read the next key scancode (blocks/polls)
//-------------------------------------------------------------
struct virtio_input_event virtio_keyboard_read_event(void)
{
    while (used_idx < used.idx) { // Check for a used buffer
        //kprintf("virtio_keyboard_read_scancode(): used.idx [%d], used_idx [%d]\n", used.idx, used_idx);

        // get event
        uint32_t buf_id = used.ring[used_idx % QUEUE_SIZE].id;
        struct virtio_input_event e = buffers[buf_id];

        // Re-queue this buffer for more events
        desc[buf_id].flags  = VIRTQ_DESC_F_WRITE;
        avail.ring[avail.idx % QUEUE_SIZE] = buf_id;
        avail.idx++;

        // keep track of used events
        used_idx++;

        // return on key-press events (type=1, value=1) or key-released (type=1, value=0)
        //kprintf("e->type [%d], e->code [%d], e->value [%d]\n", e.type, e.code, e.value);
        if (e.type == VirtioInputEvKey)
            return e;
        // skip non-key event (if any)
    }
    // no key event found
    struct virtio_input_event nokey = {.type = VirtioInputEvNone};
    return nokey;
}
