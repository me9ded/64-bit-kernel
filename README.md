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

# Back on your host machine: emulate the operating system with QEMU.
# -serial stdio shows the kernel's serial output (kprintf) in your terminal.
qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -serial stdio
```

# Testing

```sh
make test       # run on your Mac (not inside the container); needs Docker and QEMU
```

This runs `tests/run.sh`, which:
1. **Builds** the kernel and a separate test kernel in the Docker image, with warnings treated as errors.
2. **Boot-tests** the real kernel: every line in `tests/boot_expect.txt` must appear on the serial port.
3. **Runs the kernel tests**: the test kernel boots in QEMU, runs every `TEST(...)` in `tests/kernel/`, and reports the result through QEMU's exit code. Crashes (triple faults) and hangs are reported as failures too.

Serial logs are saved in `build/test-logs/`. GitHub Actions (`.github/workflows/ci.yml`) runs the same script on every push to `main` and on every pull request.

To add tests for a new feature, write them in any `.c` file in `tests/kernel/`; they're picked up automatically:

```c
#include "test.h"

TEST(feature_does_the_thing) {
    CHECK(condition);
    CHECK_EQ_INT(actual, expected);
    CHECK_EQ_STR(actual_string, "expected");
}
```

If you change what the kernel prints at boot, update `tests/boot_expect.txt`.

# Final Result

![my_os](https://github.com/user-attachments/assets/79a291d7-5288-4123-84da-903017e7fca9)
