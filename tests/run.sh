#!/usr/bin/env bash
#
# Build the kernel and test it in QEMU. Used both locally (`make test`) and by CI.
#
#   1. Build   - compile the kernel + test kernel in the Docker build image,
#                with warnings treated as errors.
#   2. Boot    - boot the real kernel; every line in tests/boot_expect.txt
#                must appear on its serial port.
#   3. Tests   - boot the test kernel (tests/kernel/*.c); it runs every TEST(...)
#                and reports the result through QEMU's exit code.
#
# Serial logs are saved in build/test-logs/.
#
# Options (environment variables):
#   SKIP_BUILD=1     reuse the existing dist/x86_64/*.iso instead of rebuilding
#   TEST_TIMEOUT=60  seconds before a hung kernel counts as a failure

set -euo pipefail
cd "$(dirname "$0")/.."

IMAGE=myos-buildinv
LOG_DIR=build/test-logs
TIMEOUT=${TEST_TIMEOUT:-60}

QEMU_EXIT_SUCCESS=33 # test kernel wrote 0x10 to the isa-debug-exit port
QEMU_EXIT_FAILURE=35 # test kernel wrote 0x11

if [ -t 1 ]; then
    RED=$'\033[31m' GREEN=$'\033[32m' BOLD=$'\033[1m' RESET=$'\033[0m'
else
    RED='' GREEN='' BOLD='' RESET=''
fi

step()  { printf '\n%s==> %s%s\n' "$BOLD" "$*" "$RESET"; }
pass()  { printf '%sPASS%s %s\n' "$GREEN" "$RESET" "$*"; }
fail()  { printf '%sFAIL%s %s\n' "$RED" "$RESET" "$*"; FAILED=1; }
FAILED=0

command -v qemu-system-x86_64 >/dev/null ||
    { echo "qemu-system-x86_64 not found (macOS: brew install qemu)"; exit 1; }

# --- 1. Build ---------------------------------------------------------------
if [ "${SKIP_BUILD:-0}" != 1 ]; then
    step "Build (Docker image: $IMAGE, warnings are errors)"
    docker build --quiet --platform linux/amd64 buildinv -t "$IMAGE" >/dev/null
    # --user: files in build/ and dist/ belong to you, not root (matters on Linux/CI).
    # Mounted at /work because a non-root user can't enter /root inside the container.
    docker run --rm --platform linux/amd64 --user "$(id -u):$(id -g)" \
        -v "$PWD:/work" -w /work "$IMAGE" \
        make clean build-x86_64 test-iso EXTRA_CFLAGS=-Werror
fi
mkdir -p "$LOG_DIR"

# Start QEMU headless in the background with serial output going to a log file.
# Usage: start_qemu <iso> <log> [extra qemu args...]; sets QEMU_PID.
start_qemu() {
    local iso=$1 log=$2
    shift 2
    : > "$log"
    qemu-system-x86_64 -cdrom "$iso" -boot d -display none -monitor none \
        -serial "file:$log" -no-reboot "$@" &
    QEMU_PID=$!
}

stop_qemu() {
    kill "$QEMU_PID" 2>/dev/null || true
    wait "$QEMU_PID" 2>/dev/null || true
}

# --- 2. Boot test -----------------------------------------------------------
step "Boot test: dist/x86_64/kernel.iso"
boot_log=$LOG_DIR/boot.log
start_qemu dist/x86_64/kernel.iso "$boot_log"

missing_lines() {
    local line
    while IFS= read -r line; do
        case "$line" in ''|'#'*) continue ;; esac
        grep -qF -- "$line" "$boot_log" || printf '%s\n' "$line"
    done < tests/boot_expect.txt
}

deadline=$((SECONDS + TIMEOUT))
while [ -n "$(missing_lines)" ] && [ $SECONDS -lt $deadline ] && kill -0 "$QEMU_PID" 2>/dev/null; do
    sleep 0.5
done
stop_qemu

sed 's/^/    | /' "$boot_log"
missing=$(missing_lines)
if [ -z "$missing" ]; then
    pass "kernel booted and printed every line in tests/boot_expect.txt"
else
    fail "kernel did not print (within ${TIMEOUT}s):"
    printf '%s\n' "$missing" | sed 's/^/    - /'
fi

# --- 3. Kernel tests --------------------------------------------------------
step "Kernel tests: dist/x86_64/kernel-test.iso"
test_log=$LOG_DIR/kernel-tests.log
start_qemu dist/x86_64/kernel-test.iso "$test_log" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x04

deadline=$((SECONDS + TIMEOUT))
while kill -0 "$QEMU_PID" 2>/dev/null && [ $SECONDS -lt $deadline ]; do
    sleep 0.5
done
timed_out=0
if kill -0 "$QEMU_PID" 2>/dev/null; then
    timed_out=1
    kill "$QEMU_PID" 2>/dev/null || true
fi
status=0
wait "$QEMU_PID" 2>/dev/null || status=$?

sed 's/^/    | /' "$test_log"
if [ $timed_out = 1 ]; then
    fail "test kernel hung: no result after ${TIMEOUT}s (infinite loop or halted CPU?)"
elif [ $status = $QEMU_EXIT_SUCCESS ]; then
    pass "all kernel tests passed"
elif [ $status = $QEMU_EXIT_FAILURE ]; then
    fail "kernel tests failed (see FAIL lines above)"
elif [ $status = 0 ]; then
    fail "test kernel crashed: CPU reset, most likely a triple fault (QEMU exit 0)"
else
    fail "QEMU exited with unexpected status $status"
fi

# --- Summary ----------------------------------------------------------------
echo
if [ $FAILED = 0 ]; then
    printf '%sAll checks passed.%s Logs: %s/\n' "$GREEN$BOLD" "$RESET" "$LOG_DIR"
else
    printf '%sSome checks failed.%s Logs: %s/\n' "$RED$BOLD" "$RESET" "$LOG_DIR"
fi
exit $FAILED
