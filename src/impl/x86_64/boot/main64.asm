global long_mode_start
extern kernel_main

section .text
bits 64
long_mode_start:
    ; load null into all data segment registers
    mov ax, 0
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

	call kernel_main

	; kernel_main returned: halt forever (hlt alone can be woken by an interrupt)
	cli
.hang:
	hlt
	jmp .hang
