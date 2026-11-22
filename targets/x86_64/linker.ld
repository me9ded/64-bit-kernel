ENTRY(start)

SECTIONS
{
	. = 1M;

	.boot :
	{
		KEEP(*(.multiboot_header))
	}

	.text :
	{
		*(.text .text.*)
	}

	.rodata : ALIGN(4K)
	{
		*(.rodata .rodata.*)

		/* Test registry: every TEST(...) in tests/kernel/ adds one entry here.
		   Empty in the normal kernel. */
		. = ALIGN(8);
		__tests_start = .;
		KEEP(*(.kernel_tests))
		__tests_end = .;
	}

	.data : ALIGN(4K)
	{
		*(.data .data.*)
	}

	.bss : ALIGN(4K)
	{
		*(COMMON)
		*(.bss .bss.*)
	}
}
