#pragma once

/* Restart the computer. */
__attribute__((noreturn)) void reboot(void);

/*
 * Power off. Works in QEMU, Bochs and VirtualBox; on other machines (which
 * need full ACPI support) it prints that it's safe to turn off, then halts.
 */
__attribute__((noreturn)) void shutdown(void);
