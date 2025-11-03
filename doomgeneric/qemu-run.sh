

: "${QEMU:="qemu-system-riscv64"}"
: "${AUDIO_DEVICE:="-device virtio-sound-device,id=vsnd"}"

# -global virtio-mmio.force-legacy=false : disable legacy virtio-mmio (version 1)
# -device virtio-keyboard-device,id=vkbd : virtualized keyboard
# -audiodev $AUDIO,id=snd0 -device virtio-sound-device,audiodev=snd0,id=vsnd : virtualized audio
"$QEMU" -global virtio-mmio.force-legacy=false -machine virt -m 128M \
 -device virtio-keyboard-device,id=vkbd $AUDIO_DEVICE \
 -device ramfb \
 -bios none -serial stdio \
 -kernel doomgeneric \


