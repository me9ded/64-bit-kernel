# Runs as x86-64 (emulated on Apple Silicon by Docker Desktop) so that the
# native gcc/binutils target x86-64 and grub-bios provides GRUB's BIOS modules.
# Alpine keeps the image small (~50 packages), which keeps scanner findings low.
FROM --platform=linux/amd64 alpine:3.24

RUN apk upgrade --no-cache && \
    apk add --no-cache gcc binutils make nasm xorriso grub grub-bios

# Inside this container the native x86-64 toolchain is the kernel toolchain.
ENV KCC=gcc KLD=ld

VOLUME /root/env
WORKDIR /root/env
