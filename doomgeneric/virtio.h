#ifndef __VIRTIO__
#define __VIRTIO__

#include <stdint.h>

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

extern struct VirtioDeviceRegs* virtio_mmio_devices;

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
#define QUEUE_SIZE 8

// Descriptor flags
#define VIRTQ_DESC_F_NEXT  1    // buffer continues via the 'next' field
#define VIRTQ_DESC_F_WRITE 2    // buffer as device write-only (otherwise device read-only)

// Virtqueue description structure - describes guest buffer and its length
struct virtq_desc {
    uint64_t addr;
    uint32_t len;
    uint16_t flags;
    uint16_t next;
};

// Avail flags
#define VIRTQ_AVAIL_F_NO_INTERRUPT 1
// Virtqueue available structure - guest driver places the descriptor (indexe) the device is going to consume
struct virtq_avail {
    uint16_t flags;
    uint16_t idx;   // next descriptor entry in the ring (modulo the queue size). This starts at 0, and increases.
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


// RISC-V hardware memory barriers:
//   ensure correct ordering for ring updates and MMIO notifies

// Prior writes before later writes
static inline void wmb(void)    { __asm__ __volatile__("fence w, w"   ::: "memory"); }
// Prior reads before later reads
static inline void rmb(void)    { __asm__ __volatile__("fence r, r"   ::: "memory"); }
// Prior writes before MMIO writes
static inline void io_wmb(void) { __asm__ __volatile__("fence w, o"   ::: "memory"); }


int detect_virtio_device(uint32_t virtio_device_id);

void print_device_stats(int dev_idx);

int setup_queue(int dev_idx, uint32_t virtqueue, struct virtq_desc* descs, struct virtq_avail* p_avail, struct virtq_used* p_used);

#endif