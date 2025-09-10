ENTRY(_start)
SECTIONS {

    /* Start of the firmware */
    . = 0x80000000;

    /* Code section */
    .text : { *(.text) }

    /* Initialized data section */
    .data : { *(.data) }

    /* Small initialized data section */
    .sdata : { *(.sdata) }

    /* Uninitialized data section (standard BSS) */
    .bss : { *(.bss COMMON) }

    /* Small BSS section for small uninitialized global/static variables */
    .sbss : { *(.sbss) *(.scommon) }

    /* 16 Mb stack */
    . = ALIGN(8);
    . += 0x1000000;
    _stack_top = .;
}
