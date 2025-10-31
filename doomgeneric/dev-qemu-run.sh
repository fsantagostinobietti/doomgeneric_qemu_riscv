
# Detect OS and pick default executable if QEMU is not set
: "${QEMU:=""}"
if [ -z "$QEMU" ]; then
    uname_s="$(uname -s 2>/dev/null || echo Unknown)"
    case "$uname_s" in
        Darwin*) QEMU="$HOME/StudioProjects/qemu-10.1.2/build/qemu-system-riscv64-unsigned"
                 AUDIO="coreaudio"
                 ;;
        Linux*)  QEMU="$HOME/programs/qemu-10.1.0/build/qemu-system-riscv64"
                 AUDIO="pa"  #  oss not working, alsa has noise
                 ;;
        *)       QEMU="qemu-system-riscv64"
                 AUDIO="default"
                 ;; # conservative default
    esac
fi

QEMU="$QEMU" AUDIO="$AUDIO" bash qemu-run.sh


