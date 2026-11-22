# Overview

This is a 64-bit kernel built using C and x86 assembly, this is my first project dealing with OS development.

# Steps to emulate on your computer

You need [Docker](https://www.docker.com/) for the build environment and [QEMU](https://www.qemu.org/) to run the kernel (`brew install qemu` on macOS). The build image runs as `linux/amd64`, so it also works on Apple Silicon Macs through Docker's emulation.

```sh
# Build the Docker image for the build environment (only needed once)
docker build --platform linux/amd64 buildinv -t myos-buildinv

# Enter the build environment, with this repository mounted at /root/env
docker run --rm -it --platform linux/amd64 -v "$(pwd):/root/env" myos-buildinv

# Inside the container: build the kernel and the bootable ISO
make            # outputs dist/x86_64/kernel.iso
make clean      # removes build outputs
exit

# Back on your host machine: emulate the operating system with QEMU
qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso
```

# Final Result

![my_os](https://github.com/user-attachments/assets/79a291d7-5288-4123-84da-903017e7fca9)
