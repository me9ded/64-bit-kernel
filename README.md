# Overview

This is a 64-bit kernel built using C and x-86 assembly, this is my first project dealing with OS developement.
# Steps to emulate on your computer

```Dockerfile
docker build buildinv -t myos-buildinv # This command lets you build the Docker image for your environment
docker run --rm -it -v "${pwd}:/root/env" myos-buildinv # This command lets you enter the build environment in a Docker container
qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso # This command emulates your operating system using QEMU
```
# Final Result

![my_os](https://github.com/user-attachments/assets/79a291d7-5288-4123-84da-903017e7fca9)
