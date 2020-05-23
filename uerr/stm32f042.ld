MEMORY {
  flash(rx):   ORIGIN = 0x08000000, LENGTH = 32k
  sram(rwxi):  ORIGIN = 0x20000000, LENGTH = 6k
}

ENTRY(reset_handler)

estack         = ORIGIN(sram) + LENGTH(sram);
min_heap_size  = 0x200;
min_stack_size = 0x400;

SECTIONS {
  . = 0x00000000;

  .irq_vector : {
    . = ALIGN(4);

    KEEP(*(.vector))

    . = ALIGN(4);
  } > flash

  .text : {
    __text_start__ = .;

    *(.text)
    *(.text*)
    *(.rodata)
    *(.rodata*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP(*(.init))
    KEEP(*(.fini))

    __text_end__ = .;
  } > flash

  .rodata : {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } > flash

  .ARM.extab : {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } > flash

  .Arm.exidx : {
    __exidx_start = .;

   *(.ARM.exidx* .gnu.linkonce.armexidx.*)

    __exidx_end = .;
  } > flash

  .preinit_array : {
    PROVIDE_HIDDEN (__preinit_array_start = .);

    KEEP (*(.preinit_array*))

    PROVIDE_HIDDEN (__preinit_array_end = .);
  } > flash

  .init_array : {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } > flash

  .fini_array : {
    PROVIDE_HIDDEN (__fini_array_start = .);

    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))

    PROVIDE_HIDDEN (__fini_array_end = .);
  } > flash

  __end_code = .;
  _etext     = .;
  _sidata    = _etext;

  .data : AT(_sidata) {
    . = ALIGN(4);
    __data_start__ = .;

    *(.data)
    *(.data*)

    . = ALIGN(4);

    __data_end__ = .;
  } > sram
  
  . = ALIGN(4);
  .bss : {
    __bss_start__ = .;

    *(.bss)
    *(.bss*)
    *(COMMON)

    . = ALIGN(4);

    __bss_end__ = .;
  } > sram

  end = __bss_end__;

  ._user_heap_stack : {
    . = ALIGN(8);

    . = . + min_heap_size;
    . = . + min_stack_size;

    . = ALIGN(8);
  } > sram

  /DISCARD/ : {
    libc.a (*)
    libm.a (*)
    libgcc.a (*)
  }

  .ARM.attributes 0 : {
    *(.ARM.attributes)
  }
}
