// Basic VirtIO primitives

#include "virtio.h"
#include "uart_serial.h"

struct VirtioDeviceRegs* virtio_mmio_devices = (struct VirtioDeviceRegs*) VIRTIO_MMIO;

/**
 * @brief look for VirtIO device of given Id
 * 
 * @param virtio_device_id VirtIO device Id (eg. 18 for input device, 25 for sound, etc.)
 * @return int ordinal number (zero based) of detected device
 */
int detect_virtio_device(uint32_t virtio_device_id) {
    for (int n=0; n<VIRTIO_MMIO_SZ ; ++n) {
        // look for a matching device (magic, version, vendor and device id)
        if (virtio_mmio_devices[n].signature == VIRTIO_MMIO_MAGIC_VALUE && 
            virtio_mmio_devices[n].version == VIRTIO_MMIO_VERSION_VALUE && // Virtio version 1 (legacy) is not supported
            virtio_mmio_devices[n].vendorId == VIRTIO_VENDOR_ID &&
            virtio_mmio_devices[n].deviceId == virtio_device_id
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


int setup_queue(int dev_idx, uint32_t virtqueue, struct virtq_desc* descs, struct virtq_avail* p_avail, struct virtq_used* p_used) {
    virtio_mmio_devices[dev_idx].queueSel = virtqueue;
    uint32_t qmax = virtio_mmio_devices[dev_idx].queueNumMax;
    if (qmax < QUEUE_SIZE)
        return -3;
    virtio_mmio_devices[dev_idx].queueNum = QUEUE_SIZE;

    // configure Descriptor ring
    virtio_mmio_devices[dev_idx].queueDescLow = ((uintptr_t)descs) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueDescHi = ((uintptr_t)descs) >> 32;
    // configure Avail ring
    virtio_mmio_devices[dev_idx].queueAvailLow = ((uintptr_t)p_avail) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueAvailHi = ((uintptr_t)p_avail) >> 32;
    // configure Used ring
    virtio_mmio_devices[dev_idx].queueUsedLow = ((uintptr_t)p_used) & 0xffffffff;
    virtio_mmio_devices[dev_idx].queueUsedHi = ((uintptr_t)p_used) >> 32;

    // Mark queue as ready
    io_wmb();   // ensure queueReady is set as last instruction 
    virtio_mmio_devices[dev_idx].queueReady = 1;

    return 0; // ok
}