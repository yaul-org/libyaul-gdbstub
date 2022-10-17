/*
 * Copyright (c) 2012-2022 Israel Jacquez
 * See LICENSE for details.
 *
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#ifndef _GDBSTUB_INTERNAL_H_
#define _GDBSTUB_INTERNAL_H_

#include <sys/cdefs.h>

#include <stdint.h>
#include <stdbool.h>

#include <cpu/instructions.h>
#include <cpu/registers.h>

char *__hex_buffer_to_mem_conv(const char *buffer, void *mem, size_t len);
char *__unsigned_long_parse(char *hargs, uint32_t *l, int delim);
char __low_nibble_to_hex_conv(char c);
int __hex_digit_to_integer_conv(char h);
int __mem_to_hex_buffer_conv(const void *mem, char *buffer, size_t len);

void __gdb_putc(int c);
int __gdb_getc(void);
void __gdb_step(cpu_registers_t *reg_file, uint32_t address);
int __gdb_break_remove(uint32_t type, uint32_t addr, uint32_t kind);
int __gdb_break(uint32_t type, uint32_t addr, uint32_t kind);
void __gdb_kill(void) __noreturn;

void __gdb_monitor_entry(cpu_registers_t *reg_file);

bool __gdb_register_file_read(cpu_registers_t *reg_file, uint32_t n, uint32_t *r);
bool __gdb_register_file_write(cpu_registers_t *reg_file, uint32_t n, uint32_t r);

#endif /* !_GDBSTUB_INTERNAL_H_ */
