#!/usr/bin/env bash
#
# Build the kernel and test it in QEMU. Used both locally (`make test`) and by CI.
#
#   1. Build   - compile the kernel + test kernel in the Docker build image,
#                with warnings treated as errors.
#   2. Boot    - boot the real kernel and follow tests/boot_script.txt:
#                wait for output, type on the keyboard, check the response.
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
step "Boot test: dist/x86_64/kernel.iso (script: tests/boot_script.txt)"
boot_log=$LOG_DIR/boot.log
monitor_log=$LOG_DIR/monitor.log
script_status=$LOG_DIR/boot-script.status   # "ok" or why the script stopped
script_step=$LOG_DIR/boot-script.step       # the line being run
script_stop=$LOG_DIR/boot-script.stop       # created to tell the script to give up
rm -f "$script_status" "$script_step" "$script_stop"
: > "$boot_log"

# QEMU `sendkey` name for one character.
key_name() {
    case "$1" in
        [abcdefghijklmnopqrstuvwxyz0123456789]) printf '%s' "$1" ;;
        [ABCDEFGHIJKLMNOPQRSTUVWXYZ]) printf 'shift-%s' "$(printf '%s' "$1" | tr 'A-Z' 'a-z')" ;;
        ' ') printf spc ;;
        '-') printf minus ;;          '_') printf shift-minus ;;
        '=') printf equal ;;          '+') printf shift-equal ;;
        '.') printf dot ;;            '>') printf shift-dot ;;
        ',') printf comma ;;          '<') printf shift-comma ;;
        '/') printf slash ;;          '?') printf shift-slash ;;
        ';') printf semicolon ;;      ':') printf shift-semicolon ;;
        "'") printf apostrophe ;;     '"') printf shift-apostrophe ;;
        '[') printf bracket_left ;;   '{') printf shift-bracket_left ;;
        ']') printf bracket_right ;;  '}') printf shift-bracket_right ;;
        '\') printf backslash ;;     '|') printf shift-backslash ;;
        '`') printf grave_accent ;;   '~') printf shift-grave_accent ;;
        '!') printf shift-1 ;;  '@') printf shift-2 ;;  '#') printf shift-3 ;;
        '$') printf shift-4 ;;  '%') printf shift-5 ;;  '^') printf shift-6 ;;
        '&') printf shift-7 ;;  '*') printf shift-8 ;;  '(') printf shift-9 ;;
        ')') printf shift-0 ;;
        *) return 1 ;;
    esac
}

# Runs tests/boot_script.txt. Its output is piped into the QEMU monitor, so
# every `echo sendkey ...` presses a key in the virtual machine.
run_boot_script() {
    set +e +o pipefail  # this runs in its own subshell: handle failures by hand
    local line command argument deadline i name match
    local offset=0      # `expect` only searches output after the previous match
    while IFS= read -r line; do
        case "$line" in ''|'#'*) continue ;; esac
        command=${line%% *}
        argument=${line#* }
        printf '%s\n' "$line" > "$script_step"

        case "$command" in
            expect)
                deadline=$((SECONDS + TIMEOUT))
                while :; do
                    match=$(tail -c +$((offset + 1)) "$boot_log" | grep -boF -- "$argument" | head -n 1)
                    [ -n "$match" ] && break
                    [ -f "$script_stop" ] && return
                    if [ $SECONDS -ge $deadline ]; then
                        printf 'timed out after %ss waiting for: %s\n' "$TIMEOUT" "$argument" > "$script_status"
                        return
                    fi
                    sleep 0.2
                done
                offset=$((offset + ${match%%:*} + ${#argument}))
                ;;
            expect-poweroff)
                # The main loop checks that QEMU exits by itself. Keep the
                # monitor's input open meanwhile so QEMU can't stop for that reason.
                echo await-poweroff > "$script_status"
                while [ ! -f "$script_stop" ]; do
                    sleep 0.2
                done
                return
                ;;
            type)
                for ((i = 0; i < ${#argument}; i++)); do
                    if ! name=$(key_name "${argument:i:1}"); then
                        printf "can't type character '%s' in: %s\n" "${argument:i:1}" "$line" > "$script_status"
                        return
                    fi
                    echo "sendkey $name"
                    sleep 0.05
                done
                ;;
            key)
                echo "sendkey $argument"
                sleep 0.05
                ;;
            *)
                printf 'unknown command in tests/boot_script.txt: %s\n' "$line" > "$script_status"
                return
                ;;
        esac
    done < tests/boot_script.txt
    echo ok > "$script_status"
}

# No -no-reboot here: the `reboot` command must really restart the machine.
# (A crash then reboots too, and shows up as an `expect` that times out.)
run_boot_script | qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso -boot d -display none \
    -monitor stdio -serial "file:$boot_log" > "$monitor_log" 2>&1 &
QEMU_PID=$!

deadline=$((SECONDS + 5 * TIMEOUT))   # safety net; each expect has its own timeout
while [ ! -f "$script_status" ] && [ $SECONDS -lt $deadline ] && kill -0 "$QEMU_PID" 2>/dev/null; do
    sleep 0.5
done

script_result=$(cat "$script_status" 2>/dev/null || true)
if [ "$script_result" = await-poweroff ]; then
    deadline=$((SECONDS + TIMEOUT))
    while kill -0 "$QEMU_PID" 2>/dev/null && [ $SECONDS -lt $deadline ]; do
        sleep 0.5
    done
    if kill -0 "$QEMU_PID" 2>/dev/null; then
        script_result="QEMU was still running ${TIMEOUT}s after expect-poweroff"
    else
        script_result=ok
    fi
fi
touch "$script_stop"
stop_qemu

sed 's/^/    | /' "$boot_log"
echo
if [ "$script_result" = ok ]; then
    pass "kernel booted and passed every step of tests/boot_script.txt"
elif [ -n "$script_result" ]; then
    fail "boot script: $script_result"
else
    fail "QEMU stopped during step: $(cat "$script_step" 2>/dev/null) (crash? see $monitor_log)"
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
    fail "test kernel hung: no result after ${TIMEOUT}s (the test after the last PASS/FAIL line never finished)"
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
