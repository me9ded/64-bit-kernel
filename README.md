# 64-bit-kernel
In order to be able to host the kernel on your machine you need to follow these steps:
1-docker build buildinv -t myos-buildinv: This command lets you build the Docker image for your environment
2-docker run --rm -it -v "${pwd}:/root/env" myos-buildinv: This command lets you enter the build environment in a Docker container
3-qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso : This command emulates your operating system using QEMU

This is the output after finishing the steps
![my_os](https://github.com/user-attachments/assets/79a291d7-5288-4123-84da-903017e7fca9)
