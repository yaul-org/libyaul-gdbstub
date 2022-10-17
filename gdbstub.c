/*
 * Copyright (c) 2012-2019
 * See LICENSE for details.
 *
 * William A. Gatliff <bgat@billgatliff.com>
 * Israel Jacquez <mrkotfw@gmail.com>
 */

#include <sys/cdefs.h>

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "gdbstub.h"
#include "gdbstub-internal.h"

#define RX_BUFFER_LEN 2048

/* Helpers */
static uint8_t _buffer_put(const char *buffer, int len);
static void _packet_get(char *rx_buffer);
static void _packet_put(char c, const char *buffer, size_t len);

/* GDB commands */
static void _gdb_command_read_memory(uint32_t address, uint32_t len);
static void _gdb_command_read_registers(cpu_registers_t *reg_file);
static void _gdb_last_signal(int sigval);

static char _rx_buffer[RX_BUFFER_LEN];

/* At a minimum, a stub is required to support the `g',
 * `G', `m', `M', `c', and `s' commands
 *
 * All other commands are optional */
void
__gdb_monitor(cpu_registers_t *reg_file, int sigval)
{
        char *hargs;

        uint32_t n;
        uint32_t r;
        uint32_t addr;
        uint32_t length;
        uint32_t kind;
        uint32_t type;
        char *data;

        int error;

        _gdb_last_signal(sigval);

        __gdb_monitor_entry(reg_file);

        while (true) {
                hargs = &*_rx_buffer;

                _packet_get(hargs);

                switch (*hargs++) {
                case '?':
                        /* '?'
                         * Indicate the reason the target halted */
                        _gdb_last_signal(sigval);
                        break;
                case 'A':
                        /* 'A arg-len,arg-num,arg,...' *
                         * Initialized argv[] array passed into
                         * program */
                case 'c':
                        /* 'c [addr]'
                         * Continue */
                        return;
                case 'C':
                        /* 'C sig[;addr]'
                         * Continue with signal */
                        break;
                case 'D':
                        /* 'D'
                         * Detach gdb from the remote system */

                        _packet_put('\0', "OK", 2);
                        return;
                case 'g':
                        /* 'g'
                         * Read general registers */
                        _gdb_command_read_registers(reg_file);
                        break;
                case 'G':
                        /* 'G XX...'
                         * Write general registers */
                        break;
                case 'i':
                        /* 'i [addr[,nnn]]'
                         * Step the remote target by a single clock cycle */
                        break;
                case 'I':
                        /* 'I'
                         * Signal then cycle step */
                        break;
                case 'k':
                        /* 'k'
                         * Kill request
                         */

                        /* Send OK back to GDB */
                        _packet_put('\0', "OK", 2);

                        __gdb_kill();
                        return;
                case 'm':
                        /* 'm addr,length'
                         * Read LENGTH bytes memory from specified ADDRess */
                        hargs = __unsigned_long_parse(hargs, &addr, ',');
                        __unsigned_long_parse(hargs, &length, '\0');

                        _gdb_command_read_memory(addr, length);
                        break;
                case 'M':
                        /* 'M addr,length:XX...'
                         * Write LENGTH bytes memory from ADDRess */

                        hargs = __unsigned_long_parse(hargs, &addr, ',');
                        data = __unsigned_long_parse(hargs, &length, ':');

                        __hex_buffer_to_mem_conv(data, (void *)addr, length);

                        /* Send OK back to GDB */
                        _packet_put('\0', "OK", 2);
                        break;
                case 'P':
                        /* 'P n...=r...'
                         * Write to a specific register with a value */

                        hargs = __unsigned_long_parse(hargs, &n, '=');

                        __hex_buffer_to_mem_conv(hargs, &r, sizeof(uint32_t));
                        __gdb_register_file_write(reg_file, n, r);

                        /* Send OK back to GDB */
                        _packet_put('\0', "OK", 2);
                        break;
                case 'R':
                        /* 'R XX'
                         * Restart the program being debugged */
                        break;
                case 's':
                        /* 's [addr]'
                         * Single step */
                        addr = 0x00000000;
                        __unsigned_long_parse(hargs, &addr, '\0');
                        __gdb_step(reg_file, addr);
                        return;
                case 'S':
                        /* 'S sig[;addr]'
                         * Step with signal */
                        return;
                case 't':
                        /* 't addr:PP,MM'
                         * Search backwards */
                        break;
                case 'z':
                        /* 'z type,addr,kind'
                         * Remove breakpoint or watchpoint */

                        type = 0x00000000;
                        addr = 0x00000000;
                        hargs = __unsigned_long_parse(hargs, &type, ',');
                        hargs = __unsigned_long_parse(hargs, &addr, ',');
                        __unsigned_long_parse(hargs, &kind, '\0');

                        if ((error = __gdb_break_remove(type, addr, kind)) < 0) {
                                _packet_put('E', "01", 2);
                                break;
                        }

                        _packet_put('\0', "OK", 2);
                        break;
                case 'Z':
                        /* 'Z type,addr,kind'
                         * Insert breakpoint or watchpoint */

                        type = 0x00000000;
                        addr = 0x00000000;
                        hargs = __unsigned_long_parse(hargs, &type, ',');
                        hargs = __unsigned_long_parse(hargs, &addr, ',');
                        __unsigned_long_parse(hargs, &kind, '\0');

                        if ((error = __gdb_break(type, addr, kind)) < 0) {
                                _packet_put('E', "01", 2);
                                break;
                        }

                        /* Send OK back to GDB */
                        _packet_put('\0', "OK", 2);
                        break;
                default:
                        /* Invalid */
                        _packet_put('\0', NULL, 0);
                        break;
                }
        }
}

static void
_gdb_command_read_registers(cpu_registers_t *reg_file)
{
        char tx_buf[8];
        size_t tx_len;

        uint8_t csum;

        uint32_t n;
        uint32_t r;

        do {
                __gdb_putc('$');

                csum = 0;
                /* Loop through all registers */
                for (n = 0; __gdb_register_file_read(reg_file, n, &r); n++) {
                        tx_len = __mem_to_hex_buffer_conv(&r, tx_buf, sizeof(r));
                        /* Send to GDB and calculate checksum */
                        csum += _buffer_put(tx_buf, tx_len);
                }

                /* Send message footer */
                __gdb_putc('#');
                __gdb_putc(__low_nibble_to_hex_conv(csum >> 4));
                __gdb_putc(__low_nibble_to_hex_conv(csum));
        } while ((__gdb_getc() & 0x7F) != '+');
}

static void
_gdb_command_read_memory(uint32_t address, uint32_t len)
{
        char tx_buf[8];
        size_t tx_len;

        uint8_t csum;

        uint8_t *p;
        uint8_t r;
        uint32_t i;

        do {
                __gdb_putc('$');

                csum = 0x00;
                for (i = 0; i < len; i++) {
                        /* Read from memory */
                        p = (uint8_t *)(address + i);
                        r = *p;
                        tx_len = __mem_to_hex_buffer_conv(&r, tx_buf, 1);

                        /* Send to GDB and calculate checksum */
                        csum += _buffer_put(tx_buf, tx_len);
                }

                /* Send message footer */
                __gdb_putc('#');
                __gdb_putc(__low_nibble_to_hex_conv(csum >> 4));
                __gdb_putc(__low_nibble_to_hex_conv(csum));
        } while ((__gdb_getc() & 0x7F) != '+');
}

static void
_gdb_last_signal(int sigval)
{
        char tx_buf[2];

        tx_buf[0] = __low_nibble_to_hex_conv(sigval >> 4);
        tx_buf[1] = __low_nibble_to_hex_conv(sigval);

        /* Send packet */
        _packet_put('S', tx_buf, sizeof(tx_buf));
}

/* Packet format
 * $packet-data#checksum */
static void
_packet_put(char c, const char *buffer, size_t len)
{
        uint8_t csum;

        do {
                /* Send the header */
                __gdb_putc('$');

                /* Send the message type, if specified */
                if (c != '\0') {
                        __gdb_putc(c);
                }
                /* Send the data */
                csum = c + _buffer_put(buffer, len);

                /* Send the footer */
                __gdb_putc('#');

                /* Checksum */
                __gdb_putc(__low_nibble_to_hex_conv(csum >> 4));
                __gdb_putc(__low_nibble_to_hex_conv(csum));
        } while ((__gdb_getc() & 0x7F) != '+');
}

/* Packet format
 * $<data>#<checksum> */
static void
_packet_get(char *rx_buffer)
{
        uint8_t ch;
        uint8_t csum;
        uint8_t xmit_csum;

        uint16_t len;
        uint16_t i;

        do {
                /* Wait around for start character, ignore all others */
                while (((ch = __gdb_getc()) & 0x7F) != '$') {
                        if (ch == '') {
                                /* ETX (end of text) */
                                ;
                        }
                }

                csum = 0x00;
                xmit_csum = 0xFF;

                /* Now, read until a # or end of buffer is found */
                for (len = 0; ; len++) {
                        ch = __gdb_getc() & 0x7F;
                        if (ch == '#') {
                                break;
                        }

                        csum += ch;
                        rx_buffer[len] = ch;
                }
                /* NULL terminate */
                rx_buffer[len] = '\0';

                /* Check if packet has a correct checksum */
                xmit_csum = __hex_digit_to_integer_conv(__gdb_getc() & 0x7F);
                xmit_csum = __hex_digit_to_integer_conv(__gdb_getc() & 0x7F) + (xmit_csum << 4);

                if (csum != xmit_csum) {
                        /* Bad checksum */
                        __gdb_putc('-');
                        continue;
                }
                /* Good checksum */
                __gdb_putc('+');

                /* If a sequence char is present, reply with the
                 * sequence ID */
                /* Packet format
                 * $sequence-id:packet-data#checksum */
                if (rx_buffer[2] == ':') {
                        /* XXX: Why is this here? */
                        assert(false);

                        __gdb_putc(rx_buffer[0]);
                        __gdb_putc(rx_buffer[1]);

                        /* Skip the first 3 characters by removing the
                         * sequence characters */
                        for (i = 3; i < len; i++) {
                                rx_buffer[i - 3] = rx_buffer[i];
                        }
                }
        } while (false);
}

static uint8_t
_buffer_put(const char *buffer, int len)
{
        uint8_t sum;
        char c;

        sum = 0x00;
        while (len--) {
                c = *buffer++;
                sum += c;
                __gdb_putc(c);
        }

        return sum;
}
