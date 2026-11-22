# Runs as x86-64 (emulated on Apple Silicon by Docker Desktop) so that the
# native gcc/binutils target x86-64 and grub-pc-bin's BIOS modules are available.
FROM --platform=linux/amd64 debian:bookworm

RUN apt-get update && \
    apt-get install -y --no-install-recommends \
        gcc binutils make nasm xorriso grub-pc-bin grub-common && \
    rm -rf /var/lib/apt/lists/*

# Inside this container the native x86-64 toolchain is the kernel toolchain.
ENV KCC=gcc KLD=ld

VOLUME /root/env
WORKDIR /root/env
