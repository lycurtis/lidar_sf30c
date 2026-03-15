# SF30/C LiDAR Driver (`lidar.h` / `lidar.c`)

Low-level driver for parsing the LightWare SF30/C 2-byte distance stream and building ASCII configuration commands. This driver is **hardware-agnostic** -- it does not touch any USART registers. The caller is responsible for reading bytes from the peripheral and feeding them in.

## Files

| File | Purpose |
|:-----|:--------|
| `lidar.h` | Public types, defines, and function prototypes |
| `lidar.c` | Implementation of the parser state machine and command builder |

## RX: 2-byte distance parser

The SF30/C continuously streams distance readings as pairs of bytes over serial UART. Each byte carries a 1-bit flag in the MSB (bit 7) that identifies it:

| Byte | Bit 7 (MSB) | Bits 6-0 |
|:-----|:---:|:---|
| High byte | `1` | Upper 7 bits of distance |
| Low byte | `0` | Lower 7 bits of distance |

High byte is always transmitted first. The two 7-bit halves combine into a 14-bit unsigned distance in centimeters (0-16383 cm):

```
distance_cm = (byte_h << 7) | byte_l
```

### State machine

The parser uses a two-state machine driven one byte at a time via `sf_parser_feed()`:

```
                 MSB=0 (discard)
                 ┌───┐
                 │   v
            ┌────────────┐     MSB=1 (store high byte)     ┌────────────┐
            │  BYTE_HIGH │ ──────────────────────────────-> │  BYTE_LOW  │
            └────────────┘                                  └────────────┘
                 ^                                            │    │
                 │              MSB=0 (complete!)             │    │
                 └────────────────────────────────────────────┘    │
                                MSB=1 (resync: overwrite high byte)
                                ┌───┘
                                v
                          stays in BYTE_LOW
```

- **BYTE_HIGH**: Waiting for a byte with MSB set. If MSB is clear, the byte is discarded (self-synchronization on startup or after data loss).
- **BYTE_LOW**: Have a valid high byte, waiting for the low byte. If MSB is unexpectedly set, the previous high byte is discarded and this byte becomes the new high byte (resync without losing a state transition).

When `sf_parser_feed()` returns `true`, a complete reading is available in `ctx.distance_cm`.

### Types

```c
typedef enum {
    SF_PARSE_BYTE_HIGH,    // Waiting for high byte (MSB=1)
    SF_PARSE_BYTE_LOW,     // Waiting for low byte  (MSB=0)
} sf_parse_state_t;

typedef struct {
    sf_parse_state_t state;
    uint8_t byte_h;        // Masked high byte (MSB stripped)
    uint8_t byte_l;        // Masked low byte  (MSB stripped)
    uint16_t distance_cm;  // Combined 14-bit result, valid after feed() returns true
} sf_parse_ctx_t;
```

### API

#### `void sf_parser_init(sf_parse_ctx_t* p)`

Zero-initializes the context and sets the state to `SF_PARSE_BYTE_HIGH`. Must be called once before feeding any bytes.

#### `bool sf_parser_feed(sf_parse_ctx_t* p, uint8_t byte)`

Feeds one received byte into the state machine.

- Returns `false` -- need more data, no complete reading yet.
- Returns `true` -- a complete high+low pair was received. Read `p->distance_cm` for the result.

### Usage

```c
static sf_parse_ctx_t ctx;
sf_parser_init(&ctx);

// In main loop:
if (usart_rx_ready(USART1)) {
    uint8_t b = usart_read_byte(USART1);
    if (sf_parser_feed(&ctx, b)) {
        // ctx.distance_cm holds the new reading
    }
}
```

## TX: ASCII command builder

The SF30/C accepts configuration commands as ASCII strings over serial UART in the format `#<mnemonic><value>:`.

### API

#### `uint16_t sf_build_cmd(char mnemonic, int value, uint8_t* buf, uint16_t buf_size)`

Formats an SF30/C command string into `buf`.

**Parameters:**
- `mnemonic` -- command letter (e.g. `'R'`, `'U'`, `'N'`, `'Y'`)
- `value` -- numeric parameter, or `SF_CMD_NO_VALUE` (`-1`) for commands that take no argument
- `buf` -- caller-provided output buffer
- `buf_size` -- size of `buf` in bytes (minimum 4 required)

**Returns:** number of bytes written to `buf` (excluding any null terminator -- there is none). Returns 0 if `buf_size < 4`.

The output is **not** null-terminated. The return value gives the exact byte count to pass to `usart_write()`.

### Implementation detail

Digits are extracted via repeated `% 10` / `/ 10` into a temporary array in reverse order, then copied into `buf` in the correct order. This avoids any dependency on `sprintf` / `snprintf` and the associated code-size cost of pulling in the printf formatting library.

### Usage

```c
uint8_t cmd[16];
uint16_t len;

// Set update rate to 5002 Hz (value 2 in lookup table)
len = sf_build_cmd('R', 2, cmd, sizeof(cmd));    // buf: "#R2:"
usart_write(USART1, cmd, len);

// Set serial output rate to 5002 Hz
len = sf_build_cmd('U', 2, cmd, sizeof(cmd));    // buf: "#U2:"
usart_write(USART1, cmd, len);

// Stop the laser (no value)
len = sf_build_cmd('N', SF_CMD_NO_VALUE, cmd, sizeof(cmd));  // buf: "#N:"
usart_write(USART1, cmd, len);
```
