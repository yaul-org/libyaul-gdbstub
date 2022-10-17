/*
 * Copyright (c) 2012-2019
 * See LICENSE for details.
 *
 * Joe Fenton <jlfenton65@gmail.com>
 * Israel Jacquez <mrkotfw@gmail.com>
 */

OUTPUT_FORMAT ("elf32-sh")
OUTPUT_ARCH (sh)
ENTRY (_start)
SEARCH_DIR ("$YAUL_INSTALL_ROOT/$YAUL_ARCH_SH_PREFIX/lib");

MEMORY {
  ram (Wx) : ORIGIN = 0x202FE000, LENGTH = 0x00002000
}

SECTIONS
{
  .header ORIGIN(ram) :
  {
     LONG(0x00010000); /* Version: BCD: Major, minor, patch */
     LONG(_gdb_init);
     LONG(_gdb_device);

     . = ALIGN(16);
  }

  .text . :
  {
     PROVIDE_HIDDEN (__text_start = .);

     KEEP (*(.text.start)) /* Keep _start at the start */
     KEEP (*(.text.*))
     KEEP (*(.text))
     KEEP (*(.gnu.linkonce.t.*))

     PROVIDE_HIDDEN (__text_end = .);
  }

  .rodata . :
  {
     . = ALIGN (0x10);

     *(.rdata)
     *(.rodata)
     *(.rodata.*)
     *(.gnu.linkonce.r.*)
  }

  .data . :
  {
     . = ALIGN (0x10);

     PROVIDE_HIDDEN (__data_start = .);

     *(.data)
     *(.data.*)
     *(.gnu.linkonce.d.*)
     *(.sdata)
     *(.sdata.*)
     *(.gnu.linkonce.s.*)

     PROVIDE_HIDDEN (__data_end = .);
  }

  .bss . :
  {
     . = ALIGN (0x10);

     PROVIDE (__bss_start = .);

     *(.bss)
     *(.bss.*)
     *(.gnu.linkonce.b.*)
     *(.sbss)
     *(.sbss.*)
     *(.gnu.linkonce.sb.*)
     *(.scommon)
     *(COMMON)

     PROVIDE (__bss_end = .);
  }
}
