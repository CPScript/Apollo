OUTPUT_FORMAT("elf64-x86-64")
OUTPUT_ARCH("i386:x86-64")
ENTRY(boot_entry_point)

SECTIONS
{
    . = 0x100000;

    .multiboot :
    {
        KEEP(*(.multiboot))
    }

    . = ALIGN(0x1000);

    .text :
    {
        *(.text)
        *(.text.*)
    }

    . = ALIGN(0x1000);
    .rodata :
    {
        *(.rodata)
        *(.rodata.*)
    }

    . = ALIGN(0x1000);
    .data :
    {
        *(.data)
        *(.data.*)
    }

    . = ALIGN(0x1000);
    .bss :
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.note.*)
        *(.eh_frame)
        *(.eh_frame_hdr)
        *(.gnu.hash)
        *(.gnu.version*)
        *(.rel.*)
        *(.rela.*)
    }
}