; Interrupt entry stubs for vectors 0-47.
;
; When an interrupt arrives, the CPU pushes ss, rsp, rflags, cs, rip (and, for
; some exceptions, an error code) and jumps to the stub listed in the IDT.
; Each stub makes the stack look the same for every vector, then the common
; code saves all registers and calls interrupt_dispatch(frame) in idt.c.
; The resulting layout is `struct interrupt_frame` in src/intf/interrupts.h.

bits 64
extern interrupt_dispatch

section .text

; The CPU pushed no error code: push a 0 so every frame has the same layout.
%macro ISR_NO_ERROR_CODE 1
isr_stub_%1:
	push 0
	push %1
	jmp interrupt_common
%endmacro

; The CPU already pushed an error code.
%macro ISR_ERROR_CODE 1
isr_stub_%1:
	push %1
	jmp interrupt_common
%endmacro

interrupt_common:
	push rax
	push rbx
	push rcx
	push rdx
	push rsi
	push rdi
	push rbp
	push r8
	push r9
	push r10
	push r11
	push r12
	push r13
	push r14
	push r15

	; The stack is now 16-byte aligned, as the C calling convention requires:
	; the CPU aligns it before pushing its 5 values, and we pushed 17 more.
	cld                     ; C code expects the direction flag cleared
	mov rdi, rsp            ; argument: struct interrupt_frame*
	call interrupt_dispatch

	pop r15
	pop r14
	pop r13
	pop r12
	pop r11
	pop r10
	pop r9
	pop r8
	pop rbp
	pop rdi
	pop rsi
	pop rdx
	pop rcx
	pop rbx
	pop rax
	add rsp, 16             ; drop vector + error code
	iretq                   ; restore rip, cs, rflags, rsp, ss

; CPU exceptions
ISR_NO_ERROR_CODE 0   ; #DE divide error
ISR_NO_ERROR_CODE 1   ; #DB debug
ISR_NO_ERROR_CODE 2   ;     non-maskable interrupt
ISR_NO_ERROR_CODE 3   ; #BP breakpoint (int3)
ISR_NO_ERROR_CODE 4   ; #OF overflow
ISR_NO_ERROR_CODE 5   ; #BR bound range exceeded
ISR_NO_ERROR_CODE 6   ; #UD invalid opcode
ISR_NO_ERROR_CODE 7   ; #NM device not available
ISR_ERROR_CODE    8   ; #DF double fault
ISR_NO_ERROR_CODE 9   ;     coprocessor segment overrun (legacy)
ISR_ERROR_CODE    10  ; #TS invalid TSS
ISR_ERROR_CODE    11  ; #NP segment not present
ISR_ERROR_CODE    12  ; #SS stack-segment fault
ISR_ERROR_CODE    13  ; #GP general protection fault
ISR_ERROR_CODE    14  ; #PF page fault
ISR_NO_ERROR_CODE 15  ;     reserved
ISR_NO_ERROR_CODE 16  ; #MF x87 floating-point error
ISR_ERROR_CODE    17  ; #AC alignment check
ISR_NO_ERROR_CODE 18  ; #MC machine check
ISR_NO_ERROR_CODE 19  ; #XM SIMD floating-point error
ISR_NO_ERROR_CODE 20  ; #VE virtualization exception
ISR_ERROR_CODE    21  ; #CP control protection
ISR_NO_ERROR_CODE 22  ;     reserved
ISR_NO_ERROR_CODE 23  ;     reserved
ISR_NO_ERROR_CODE 24  ;     reserved
ISR_NO_ERROR_CODE 25  ;     reserved
ISR_NO_ERROR_CODE 26  ;     reserved
ISR_NO_ERROR_CODE 27  ;     reserved
ISR_NO_ERROR_CODE 28  ; #HV hypervisor injection
ISR_ERROR_CODE    29  ; #VC VMM communication
ISR_ERROR_CODE    30  ; #SX security exception
ISR_NO_ERROR_CODE 31  ;     reserved

; Hardware IRQs 0-15 (vectors 32-47 after the PIC is remapped)
ISR_NO_ERROR_CODE 32  ; IRQ 0  timer (PIT)
ISR_NO_ERROR_CODE 33  ; IRQ 1  keyboard
ISR_NO_ERROR_CODE 34  ; IRQ 2  cascade (slave PIC)
ISR_NO_ERROR_CODE 35  ; IRQ 3  COM2
ISR_NO_ERROR_CODE 36  ; IRQ 4  COM1
ISR_NO_ERROR_CODE 37  ; IRQ 5
ISR_NO_ERROR_CODE 38  ; IRQ 6  floppy
ISR_NO_ERROR_CODE 39  ; IRQ 7  (spurious on master)
ISR_NO_ERROR_CODE 40  ; IRQ 8  RTC
ISR_NO_ERROR_CODE 41  ; IRQ 9
ISR_NO_ERROR_CODE 42  ; IRQ 10
ISR_NO_ERROR_CODE 43  ; IRQ 11
ISR_NO_ERROR_CODE 44  ; IRQ 12 PS/2 mouse
ISR_NO_ERROR_CODE 45  ; IRQ 13
ISR_NO_ERROR_CODE 46  ; IRQ 14 primary ATA
ISR_NO_ERROR_CODE 47  ; IRQ 15 secondary ATA (spurious on slave)

; Address of every stub, used by idt.c to fill in the IDT.
section .rodata
global isr_stub_table
align 8
isr_stub_table:
%assign i 0
%rep 48
	dq isr_stub_%+i
%assign i i+1
%endrep
