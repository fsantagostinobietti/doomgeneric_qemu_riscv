# Qemu build from sources

Qemu custom build is often needed to enable audio support on host machine
Qemu audio usually doesn't work out-of-the-box !

```shell
$ wget https://download.qemu.org/qemu-10.1.0.tar.xz   # replace with latest version
$ tar xvJf qemu-10.1.0.tar.xz
$ cd qemu-10.1.0

$ ./configure --target-list=riscv64-softmmu
# or 
# on my Linux Ubuntu 22.04
$ ./configure --target-list=riscv64-softmmu --audio-drv-list=pa --enable-gtk  # pulseaudio and gtk for video
# or
# on macos (see https://formulae.brew.sh/formula/qemu)
$ ./configure --target-list=riscv64-softmmu --disable-bsd-user --disable-guest-agent \
               --disable-sdl --disable-gtk --enable-cocoa
$ make
```
