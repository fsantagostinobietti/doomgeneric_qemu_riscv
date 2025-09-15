// virtio_keyboard.c
// Minimal freestanding virtio-MMIO (Virtio over MMIO) keyboard driver for QEMU “virt”

#include <stdint.h>
#include <stddef.h>
#include "uart_serial.h"
#include "virtio_keyboard.h"

// Qemu address for Virtio MMIO array of devices, as defined in Device Tree.
#define VIRTIO_MMIO      0x10001000UL
// Device Tree state Qemu ha 8 slots for devices
#define VIRTIO_MMIO_SZ     8

// MMIO base address: change to whatever your guest uses for the keyboard
// QEMU virt often maps devices at 0x1000_1000 (VIRTIO_MMIO) + N*0x1000

// Virtio memory-mapped device registers
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-1820002
struct VirtioDeviceRegs {
	uint32_t signature;
	uint32_t version;
	uint32_t deviceId;
	uint32_t vendorId;
	uint32_t deviceFeatures;
    uint32_t deviceFeaturesSel; // 0 -> deviceFeatures contains Low 32 bits, 1 -> deviceFeatures contains High 32 bits
	uint32_t unknown1[2];
	uint32_t driverFeatures;
	uint32_t driverFeaturesSel; // 0 -> driverFeatures contains Low 32 bits, 1 -> driverFeatures contains High 32 bits
	uint32_t guestPageSize; /* version 1 only */
	uint32_t unknown3[1];
	uint32_t queueSel;
	uint32_t queueNumMax;
	uint32_t queueNum;
	uint32_t queueAlign;    /* version 1 only */
	uint32_t queuePfn;      /* version 1 only */
	uint32_t queueReady;
	uint32_t unknown4[2];
	uint32_t queueNotify;
	uint32_t unknown5[3];
	uint32_t interruptStatus;
	uint32_t interruptAck;
	uint32_t unknown6[2];
	uint32_t status;
	uint32_t unknown7[3];
	uint32_t queueDescLow;
	uint32_t queueDescHi;
	uint32_t unknown8[2];
	uint32_t queueAvailLow;
	uint32_t queueAvailHi;
	uint32_t unknown9[2];
	uint32_t queueUsedLow;
	uint32_t queueUsedHi;
	uint32_t unknown10[21];
	uint32_t configGeneration;
	uint8_t config[3840];
};
// sizeof(VirtioDeviceRegs) == 4096 == 0x1000

struct VirtioDeviceRegs* virtio_mmio_devices = (struct VirtioDeviceRegs*) VIRTIO_MMIO;

// Expected magic/version/vendors
#define VIRTIO_MMIO_MAGIC_VALUE 0x74726976  // "virt"
#define VIRTIO_MMIO_VERSION_VALUE 2
#define VIRTIO_VENDOR_ID         0x554d4551  // "QEMU"

// Device type for virtio-input (keyboard/mouse/etc)
#define VIRTIO_DEVICE_ID_INPUT   18

// Driver status bits
enum {
    VIRTIO_STATUS_ACKNOWLEDGE = 1,  // Driver has detected the device
    VIRTIO_STATUS_DRIVER      = 2,  // Driver knows how to drive the device
    VIRTIO_STATUS_DRIVER_OK   = 4,  // Driver is set up and ready to drive the device
    VIRTIO_STATUS_FEATURES_OK = 8,  // Driver has accepted device features
    VIRTIO_STATUS_FAILED      = 0x80,   // Driver has failed to initialize the device
};


// Virtqueue constants
#define QUEUE_SIZE (1<<3)   // 8

// Descriptor flags
#define VIRTQ_DESC_F_NEXT  1
#define VIRTQ_DESC_F_WRITE 2

// Avail flags
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1

// Virtqueue description structure - describes guest buffer and its length
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};
// Virtqueue available structure - guest driver places the descriptor (indexe) the device is going to consume
struct virtq_avail {
    uint16_t flags;
    uint16_t idx;
    uint16_t ring[QUEUE_SIZE];
    // uint16_t used_event; // optional
};

struct virtq_used_elem {
    uint32_t id;
    uint32_t len;
};
// Virtqueue used structure - device returns used (read or written) buffer to the driver
struct virtq_used {
    uint16_t flags;
    uint16_t idx;
    struct virtq_used_elem ring[QUEUE_SIZE];
    // uint32_t avail_event; // optional
};


// Static allocation of virtqueue (aligned and contiguous in physical memory)
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-350007
static struct virtq_desc    desc[QUEUE_SIZE]          __attribute__((aligned(16)));
static struct virtq_avail   avail                     __attribute__((aligned(2)));
static struct virtq_used    used                      __attribute__((aligned(4)));
static struct virtio_input_event buffers[QUEUE_SIZE];

// Track next used slot
static uint16_t used_idx  = 0;


int detect_virtio_keyboard(void) {
    for (int n=0; n<VIRTIO_MMIO_SZ ; ++n) {
        // look for keyboard device (magic, version, vendor and device id)
        if (virtio_mmio_devices[n].signature == VIRTIO_MMIO_MAGIC_VALUE && 
            virtio_mmio_devices[n].version == VIRTIO_MMIO_VERSION_VALUE && // Virtio version 1 (legacy) is not supported
            virtio_mmio_devices[n].vendorId == VIRTIO_VENDOR_ID &&
            virtio_mmio_devices[n].deviceId == VIRTIO_DEVICE_ID_INPUT
        ) {
            return n;
        }
    }
    return -1;
}

void print_device_stats(int dev_idx) {
    kprintf("deviceId [%x], ", virtio_mmio_devices[dev_idx].deviceId);
    virtio_mmio_devices[dev_idx].deviceFeaturesSel = 1; // select high 32 bits
    kprintf("deviceFeaturesHi [%x], ", virtio_mmio_devices[dev_idx].deviceFeatures);
    virtio_mmio_devices[dev_idx].deviceFeaturesSel = 0; // select low 32 bits
    kprintf("deviceFeaturesLo [%x]\n", virtio_mmio_devices[dev_idx].deviceFeatures);
    kprintf("queueNumMax [%d], queueNum [%d]\n", virtio_mmio_devices[dev_idx].queueNumMax, virtio_mmio_devices[dev_idx].queueNum);
    kprintf("queueDesc [%p][%p], ", virtio_mmio_devices[dev_idx].queueDescHi, virtio_mmio_devices[dev_idx].queueDescLow);
    kprintf("\n");
}

//-------------------------------------------------------------
// Initialize the virtio-keyboard device
// See https://docs.oasis-open.org/virtio/virtio/v1.3/virtio-v1.3.html#x1-1230001
int virtio_keyboard_init(void)
{
    // detect keyboard input device
    // i.e. index of 'virtio_mmio_devices[]'
    int dev_idx = detect_virtio_keyboard();
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
    virtio_mmio_devices[dev_idx].queueSel = 0; // queue id == 0
    uint32_t qmax = virtio_mmio_devices[dev_idx].queueNumMax;
    if (qmax < QUEUE_SIZE)
        return -3;
    virtio_mmio_devices[dev_idx].queueNum = QUEUE_SIZE;

    // configure Descriptor ring
    virtio_mmio_devices[dev_idx].queueDescLow = ((uintptr_t)desc) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueDescHi = ((uintptr_t)desc) >> 32;
    // configure Avail ring
    virtio_mmio_devices[dev_idx].queueAvailLow = ((uintptr_t)&avail) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueAvailHi = ((uintptr_t)&avail) >> 32;
    // configure Used ring
    virtio_mmio_devices[dev_idx].queueUsedLow = ((uintptr_t)&used) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueUsedHi = ((uintptr_t)&used) >> 32;

    // Mark queue as ready
    virtio_mmio_devices[dev_idx].queueReady = 1;

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
    // Publish initial avail.idx
    avail.idx   = QUEUE_SIZE;

    // Finally tell device we’re fully up
    kprintf("Notify device driver is ok\n");
    virtio_mmio_devices[dev_idx].status |= VIRTIO_STATUS_DRIVER_OK;

    return 0;
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
