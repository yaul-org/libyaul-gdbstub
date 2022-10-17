#include "gdbstub.h"

#include "gdbstub-internal.h"

#define IS_POW_2_ALIGNED(t, addr, len)                                         \
        (((len) >= sizeof(uint ## t ## _t)) &&                                 \
        (((uint32_t)(addr) & (sizeof(uint ## t ## _t) - 1)) == 0))

typedef union {
        uint32_t l_buffer;
        uint16_t s_buffer;
        char c_buffer[sizeof(uint32_t)];
} lc_t;

/* Convert '[0-9a-fA-F]' to its integer equivalent */
int
__hex_digit_to_integer_conv(char h)
{
        if ((h >= 'a') && (h <= 'f')) {
                return h - 'a' + 10;
        }

        if ((h >= '0') && (h <= '9')) {
                return h - '0';
        }

        if ((h >= 'A') && (h <= 'F')) {
                return h - 'A' + 10;
        }

        return 0;
}

/* Convert the low nibble of C to its hexadecimal character equivalent */
char
__low_nibble_to_hex_conv(char c)
{
        static const char lut[] = {
                "0123456789ABCDEF"
        };

        return lut[c & 0x0F];
}

/* Translates a delimited hexadecimal string to an unsigned long (32-bit) */
char *
__unsigned_long_parse(char *hargs, uint32_t *l, int delim)
{
        *l = 0x00000000;

        while (*hargs != delim) {
                *l = (*l << 4) + __hex_digit_to_integer_conv(*hargs++);
        }

        return hargs + 1;
}

/* Converts a memory region of length LEN bytes, starting at MEM, into a string
 * of hex bytes.
 *
 * Returns the number of bytes placed into hexadecimal buffer.
 *
 * This function carefully preserves the endianness of the data, because that's
 * what GDB expects */
int
__mem_to_hex_buffer_conv(const void *mem, char *buffer, size_t len)
{
        lc_t lc;

        uint32_t i;
        size_t cbuf_len;
        int ret_val;

        ret_val = 0;
        for (i = 0; len > 0; ) {
                if (IS_POW_2_ALIGNED(32, mem, len)) {
                        lc.l_buffer = *(uint32_t *)mem;
                        cbuf_len = sizeof(uint32_t);
                } else if (IS_POW_2_ALIGNED(16, mem, len)) {
                        lc.s_buffer = *(uint16_t *)mem;
                        cbuf_len = sizeof(uint16_t);
                } else {
                        lc.c_buffer[0] = *(char *)mem;
                        cbuf_len = sizeof(char);
                }

                mem = (void *)((uint32_t)mem + cbuf_len);
                len -= cbuf_len;

                for (i = 0; i < cbuf_len; i++) {
                        *buffer++ = __low_nibble_to_hex_conv(lc.c_buffer[i] >> 4);
                        *buffer++ = __low_nibble_to_hex_conv(lc.c_buffer[i]);
                        ret_val += 2;
                }
        }

        return ret_val;
}

/* Reads twice the LEN hexadecimal digits from BUF and converts them to binary.
 *
 * Points to the first empty byte after the region written */
char *
__hex_buffer_to_mem_conv(const char *buffer, void *mem, size_t len)
{
        lc_t lc;

        size_t cbuf_len;
        uint32_t i;

        for (i = 0; len > 0; ) {
                if (IS_POW_2_ALIGNED(32, mem, len)) {
                        cbuf_len = sizeof(uint32_t);
                        for (i = 0; i < cbuf_len; i++) {
                                lc.c_buffer[i] = (__hex_digit_to_integer_conv(*buffer++) << 4);
                                lc.c_buffer[i] += __hex_digit_to_integer_conv(*buffer++);
                        }
                        *(uint32_t *)mem = lc.l_buffer;
                } else if (IS_POW_2_ALIGNED(16, mem, len)) {
                        cbuf_len = sizeof(uint16_t);
                        for (i = 0; i < cbuf_len; i++ ) {
                                lc.c_buffer[i] = (__hex_digit_to_integer_conv(*buffer++) << 4);
                                lc.c_buffer[i] += __hex_digit_to_integer_conv(*buffer++);
                        }
                        *(uint16_t *)mem = lc.s_buffer;
                } else {
                        lc.c_buffer[0] = (__hex_digit_to_integer_conv(*buffer++) << 4);
                        lc.c_buffer[0] += __hex_digit_to_integer_conv(*buffer++);

                        cbuf_len = sizeof(char);
                        *(char *)mem = lc.c_buffer[0];
                }
                mem = (void *)((uint32_t)mem + cbuf_len);
                len -= cbuf_len;
        }

        return mem;
}
