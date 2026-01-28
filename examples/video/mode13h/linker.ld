/* The bootloader will start execution at the symbol designated as the entry */
ENTRY(_start)

/* Define the memory layout */
SECTIONS
{
    /* Begin at 1 MiB, a conventional place for kernels to be loaded */
    . = 1M;

    /*
     * Place multiboot header and text together to ensure the header
     * appears early in the file. GRUB requires the multiboot header
     * within the first 8KB of the file.
     */
    .text ALIGN(4K) :
    {
        *(.multiboot)
        *(.text)
    }

    /* Read-only data */
    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }

    /* Initialized read-write data */
    .data ALIGN(4K) :
    {
        *(.data)
    }

    /* Uninitialized read-write data and stack */
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss)
    }
}
