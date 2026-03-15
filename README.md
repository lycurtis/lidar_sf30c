# LiDAR SF30/C Driver Interface with Nucleo-STM32F103RB

## Default settings
- Serial port settings: Baud rate 115200, 8 data bits, 1 stop bit, no parity, no handshake
- Update rate: 39 readings per second

# My goal
- ~5 kHz Update Rate = 5002 readings/second
- 921600 baud rate

## Adjustable parameters
### Communication
- Serial UART baud rate: 9600 to 921600 (I am going with 921600)
### Data output
- Output type: Controls the data output type. Primarily used for legacy compatibility
   - Distance over USB,
   - Distance and strength over USB,
   - Distance over Serial, (I plan to use this since I am connected over UART)
   - Analog voltage over USB,
   - Full communication mode
- USB port output rate: 39 to 625 readings per second (Note this is limited by exposure time)
- Serial port output rate: 39 to 20010 readings per second (Note this is limited by exposure time)
### Measurement:
- Exposure time: 50us - 25587us (20010 to 30 readings per second) (Controls the duration of a single measurement and limits the total measurement speed)
- Return mode: First / Last (Controls whether the output measurements are first or last pulse)
- Lost signal confirmations: 1 to 250 (The number of failed readings required before a loss of signal is reported, (whole number))
- Zero offset: -100 to 100 (The distance in meters that the output measurement is adjusted by)
### Analog: (Don't plan to use)
- Analog port output rate: 39 to 20010 readings per second (Controls the ouptut rate of the analog port)
- Analog range: 1-256 (The value in meters that 0V to 2.048V represent)
### Alarms: (Don't plan to use)
- Alarm activation distance: 0.5 to 254 (Warn when an object is detected closer than this user-set alarm distance (in meters, upt to two decimal places))
- Alarm hysteresis: 0.06 to 50 meters
- Alarm latch: Select/Deselect (If latching is enabled and the alarm is triggered, the alarm output will remain active until manually released)
### Extra
- Synchornization output: Select/Deselect (Output on the Sync output (2, white) will be high while the ranging function is in operation)
- Sensitivty offset: -30000 to 0 (Adjust the receiver sensitivty to ensure consistent reliability in excessive levels of direct sunlight)

## SF30/C pinout table

| Connector Pin | Wire | Serial Function |
| 1 | Green | Alarm output (Not connected) |
| 2 | White | Sync ouput (Not connected) | 
| 3 | Yellow | TXD | 
| 4 | Orange | RXD |
| 5 | Blue | ANALOG (Not connected) | 
| 6 | Black | GND | 
| 7 | Red | VIN, +5 V |

Note: The serial UART, Alarm and Sync interfaces use 3.3V TTL logic, (5 V tolerant)

## Output interfaces
The distance measured by the SF30/C can be obtained via the following outputs:
- Serial UART (What I plan to use)
- Linear analogue voltage
- Alarm interfaces

## Serial UART interface
The serial port is used to transmit distance readings to an embedded host processor such as the flight controller in a UAV. The hardware interface uses 3.3 V logic levels and can be connected directly to any similar, compatible interface

Distances are output as a 2-byte binary number. You can identify which byte you are reading by looking at the most significant bit. If the MSB is not set then you have received the low byte, if the MSB is set then you have received the high byte. The high byte is always sent first for an individual reading. The remaining 7 bits in each byte combine to make a 14-bit distance reading in centimeters.

Converting the high and low byte data to a single reading can be accomblished with the following formula:
```
Reading = ((Byte_H & 0x7F) * 128)+(Byte_L & 0x7F)
```

The table below indicates the structure of a 2-byte bitstream
| Bit stream | Byte indicator | High byte data | Low byte data |
|:---:|:---:|:---:|:---:|
| 15 | **1** - High Byte | | |
| 14 | | High byte bit 6 | |
| 13 | | High byte bit 5 | |
| 12 | | High byte bit 4 | |
| 11 | | High byte bit 3 | |
| 10 | | High byte bit 2 | |
| 9 | | High byte bit 1 | |
| 8 | | High byte bit 0 | |
| 7 | **0** - Low Byte | | |
| 6 | | | Low byte bit 6 |
| 5 | | | Low byte bit 5 |
| 4 | | | Low byte bit 4 |
| 3 | | | Low byte bit 3 |
| 2 | | | Low byte bit 2 |
| 1 | | | Low byte bit 1 |
| 0 | | | Low byte bit 0 |

## Analog interface
The analog output is presented on connector pin 5 of the device. It produces a linear voltage of between 0.00 V and 2.048 V proportional to the measured distance.

A distance reading of zero meters produces a 0 V output

By setting the Analog range value in LightWare Studio, the upper limit distance equivalent to 2.048V can be set. This upper limit can range from 1m to 256m. A lower analog range value will result in a finer analog resolution.

## Alarm interface
The alarm output is a 0 V to 3.3 V signal that becomes active low when the distance reading falls below the preset distance.

In cases where the Analog range in LightWare Studio is set to a lower value than the Alarm activation distance, the alrm will remain activated.

## Commands
Commands can be sent to your LightWare SF30/C microLiDAR by means of ASCII-encoded characeters via the serial UART interface.

### Command structure
A command string is formatted as `#Annnnn:', where 
- '#' indicates the start of the command
- 'A' indicates the command mnemonic
- 'Nnnnn' indicates the values to be set (where applicable)
- ':' indicates the end of the command

### SF30/C commands table

| ID | Name | Description |
|:---|:-----|:------------|
| #Rn: | Update rate | Controls the SF30's sampling update rate. Writing this command will set the update rate. See [update rate table](#update-rate-values) below. |
| #Un: | Serial port output rate | Controls the update rate of the data output through the serial output. Reading this command will return the current update rate. Writing this command will set the update rate. See [update rate table](#update-rate-values) below. |
| #Vn: | Analog data update rate | Controls the update rate of the data output through the Analog output. Writing this command will set the update rate. See [update rate table](#update-rate-values) below. |
| #Annnnn: | Alarm distance | The distance in centimeters at which the alarm is activated. |
| #LN: | Alarm latching | Enables the alarm signal latch. This keeps the alarm in the active low state after an alarm event until it is cleared by sending "#X:" over the serial port. (0 = Disabled, 1 = Enabled) |
| #X: | Alarm latch clear | Clears the alarm latch if it is enabled. |
| #N: | Laser stop | Stops the laser from firing. |
| #Y: | Laser start | Starts the laser firing. |

#### Update rate values

The commands #Rn:, #Un:, and #Vn: use the following lookup table:

| Command value | Update rate (samples/second) |
|:---:|:---:|
| 0 | 20010 |
| 1 | 10005 |
| 2 | 5002 |
| 3 | 2501 |
| 4 | 1250 |
| 5 | 625 |
| 6 | 312 |
| 7 | 156 |
| 8 | 78 |
| 9 | 39 |
