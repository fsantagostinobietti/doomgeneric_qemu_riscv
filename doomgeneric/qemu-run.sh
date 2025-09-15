
# -global virtio-mmio.force-legacy=false : disable legacy virtio-mmio (version 1)
# -device virtio-keyboard-device,id=vkbd : virtualized keyboard
qemu-system-riscv64 -global virtio-mmio.force-legacy=false -machine virt -m 128M \
 -device virtio-keyboard-device,id=vkbd \
 -device ramfb \
 -bios none -serial stdio \
 -kernel doomgeneric \


