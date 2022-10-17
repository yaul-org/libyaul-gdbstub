Standalone GDB stub
===

## Build

1. Run `make`

2. After successfully building, there are two files to copy into your project:
   `gdbstub.bin` and `gdbstub.h`.

## Constraints

The GDB stub has the following constraints:

1. The binary must reside in read/writable memory.

2. The `sp` (`r15`) register must be properly set.

## Loading

### Including `gdbstub.bin` into your project

Depending on how your project is structured, there's a few ways to include `gdbstub.bin`:

1. Use the builtin assets linking feature.

```makefile
# Each asset follows the format: <path>;<symbol>. Duplicates are removed
BUILTIN_ASSETS+= \
	assets/gdbstub.bin;gdbstub_bin
```

2. Convert `gdbstub.bin` into a C array and include that as a source file.

```sh
bin2c gdbstub.bin gdbstub_bin gdbstub.bin.h
```

3. Load `gdbstub.bin` from CD.

### Initializing `gdbstub.bin`

Assuming that the following variables:

  - `gdbstub_bin` is the pointer to the start of the binary, and
  - `gdbstub_bin_size` is the size (in bytes) of the binary.

1. Copy the binary over to the load address.

```c
(void)memcpy(GDBSTUB_LOAD_ADDRESS, gdbstub_bin, gdbstub_bin_size);
```

2. The driver is the device used to send and receive bytes from/to the GDB stub.

```c
static void
_gdb_device_init(void)
{
        usb_cart_init();
}

static uint8_t
_gdb_device_byte_read(void)
{
        return usb_cart_byte_read();
}

void
_gdb_device_byte_write(uint8_t value)
{
        usb_cart_byte_send(value);
}
```

3. Update the driver interface in the GDB stub header.

```c
gdbstub_t * const gdbstub = (gdbstub_t *)gdbstub_bin;

gdbstub->device->init       = _gdb_device_init;
gdbstub->device->byte_read  = _gdb_device_byte_read;
gdbstub->device->byte_write = _gdb_device_byte_write;
```

4. Initialize the GDB stub.

```c
gdbstub->init();
```

## Usage

See the [GDB example][1] on usage.

[1]: https://github.com/ijacquez/libyaul-examples/tree/develop/gdb
