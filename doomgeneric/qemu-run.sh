
# Detect OS and pick default executable if QEMU is not set
: "${QEMU:=""}"
if [ -z "$QEMU" ]; then
    uname_s="$(uname -s 2>/dev/null || echo Unknown)"
    case "$uname_s" in
        Darwin*) QEMU="$HOME/StudioProjects/qemu/build/qemu-system-riscv64-unsigned"
                 AUDIO="sdl"  # coreaudio not working
                 ;;
        Linux*)  QEMU="$HOME/programs/qemu-10.1.0/build/qemu-system-riscv64"
                 AUDIO="pa"  #  oss not working, alsa has noise
                 ;;
        *)       QEMU="qemu-system-riscv64"
                 AUDIO="default"
                 ;; # conservative default
    esac
fi

# -global virtio-mmio.force-legacy=false : disable legacy virtio-mmio (version 1)
# -device virtio-keyboard-device,id=vkbd : virtualized keyboard
# -audiodev $AUDIO,id=snd0 -device virtio-sound-device,audiodev=snd0,id=vsnd \
"$QEMU" -global virtio-mmio.force-legacy=false -machine virt -m 128M \
 -device virtio-keyboard-device,id=vkbd \
 -device ramfb \
 -bios none -serial stdio \
 -kernel doomgeneric \


