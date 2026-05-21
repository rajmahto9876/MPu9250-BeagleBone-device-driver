---
title: "Building an SPI Interface for the MPU9250: Design, Wiring, and Implementation"
author: ""
date: 2026-05-21
---

# Building an SPI Interface for the MPU9250: Design, Wiring, and Implementation

## Goal

This guide describes end-to-end SPI integration for the MPU9250: wiring and pin mappings, SPI settings, device tree, kernel and user-space access patterns, minimal code snippets, and testing tips.

## Audience

Embedded engineers writing Linux drivers or user-space interfaces, and firmware engineers doing bare-metal or RTOS SPI.

## Why choose SPI for MPU9250

- Speed & robustness: SPI provides higher throughput and simpler timing than I²C for burst reads (useful for high-rate IMU data).
- Full-duplex: Useful for pipelined or continuous reads.
- Tradeoff: SPI requires a dedicated CS line per device, but avoids I²C address conflicts.

---

## Essential pins (MPU9250)

- SCLK (SPI clock)
- MOSI (master out / slave in)
- MISO (master in / slave out)
- CS (chip select)
- INT (data-ready / interrupt)
- VCC (3.3 V)
- GND

> Note: MPU9250 is a 3.3 V device - do not use 5 V signals without proper level shifting.

---

## Hardware wiring examples

Raspberry Pi (SPI0) example:

- SCLK: GPIO11 (physical pin 23)
- MOSI: GPIO10 (physical pin 19)
- MISO: GPIO9  (physical pin 21)
- CE0 (CS): GPIO8  (physical pin 24)
- INT: any free GPIO (e.g., GPIO25) wired to MPU9250 `INT` pin

Arduino Uno example:

- SCK: D13
- MISO: D12
- MOSI: D11
- SS (use as CS): D10
- INT: any digital pin supporting interrupts

STM32 and other MCUs: use the MCU's hardware SPI pins; use an accessible GPIO for CS and another GPIO for INT/DRDY.

---

## SPI Mode, Clock & Transaction Rules

- Mode: Start with SPI mode 0 (CPOL=0, CPHA=0). Confirm against the MPU9250 datasheet and test.
- Clock: Start conservatively (e.g., 1 MHz) and verify data integrity before increasing to the sensor's maximum supported clock.
- Read/Write protocol: For MPU9250/MPU9xxx devices the register address MSB indicates read vs write: set MSB=1 for read, MSB=0 for write.

Practical notes:

- Read N bytes: send `[reg | 0x80]` then read N bytes.
- Write: send `[reg & 0x7F, value(s)...]`.
- Burst reads: use a single SPI transaction (address byte followed by continuous clocking) to read consecutive registers.

---

## Register access examples

- To read register `0x3B` (accel X MSB): send `0x3B | 0x80` then clock out dummy bytes to receive data.
- To write register `0x6B` (PWR_MGMT_1): send `0x6B & 0x7F` followed by the value byte.

Always verify the exact bit conventions from the MPU9250 datasheet used for your part revision.

---

## Device Tree (Linux)

Add a node for the SPI device in your board DTS. Example snippet (adjust GPIO/interrupt numbers for your platform):

```dts
&spi0 {
    status = "okay";
    mpu9250@0 {
        compatible = "invensense,mpu9250";
        reg = <0>; /* chip-select 0 */
        spi-max-frequency = <1000000>;
        interrupts = <25 IRQ_TYPE_EDGE_RISING>;
        interrupt-parent = <&gpio0>;
    };
};
```

If you have an existing `mpu9250.dts` file, add or adapt a node there.

---

## Kernel driver integration (high-level)

- Use Linux SPI APIs: `spi_register_driver()`, `spi_sync()`, helpers such as `spi_write_then_read()` or `spi_sync_transfer()`.
- Typical probe flow:
  - Configure `spi_device` settings (`spi->mode`, `spi->max_speed_hz`) and call `spi_setup(spi)`.
  - Parse DTS for `interrupts` and request the IRQ with `devm_request_threaded_irq()` or `devm_request_irq()`.
  - Register device interfaces (input, misc/char, or industrial sensor interfaces) and start sampling via a workqueue, hrtimer or IRQ-driven handler.

Kernel read example (short):

```c
static int mpu_read(struct spi_device *spi, u8 reg, u8 *buf, size_t len)
{
    u8 tx = reg | 0x80;
    return spi_write_then_read(spi, &tx, 1, buf, len);
}
```

Kernel write example (short):

```c
static int mpu_write(struct spi_device *spi, u8 reg, const u8 *buf, size_t len)
{
    int ret;
    u8 *tx = kmalloc(len + 1, GFP_KERNEL);
    if (!tx)
        return -ENOMEM;
    tx[0] = reg & 0x7F;
    memcpy(&tx[1], buf, len);
    ret = spi_write(spi, tx, len + 1);
    kfree(tx);
    return ret;
}
```

For more advanced control (CS hold, DMA, multiple transfers) build `struct spi_transfer` and `struct spi_message`.

---

## User-space example (Python with spidev)

Quick check using `python3` and the `spidev` module:

```python
import spidev

spi = spidev.SpiDev()
spi.open(0, 0)            # bus 0, CS0
spi.max_speed_hz = 1000000
spi.mode = 0

# Read WHO_AM_I (0x75) expecting 0x71 for MPU9250
resp = spi.xfer2([0x75 | 0x80, 0x00])
whoami = resp[1]
print(hex(whoami))
spi.close()
```

Notes:

- `xfer2()` performs a single transaction: transmit the address byte then clock dummy bytes to receive replies.

---

## Interrupts & data-ready handling

- Use the MPU9250 `INT` pin for DRDY (data-ready) or other interrupt sources. Wire `INT` to a GPIO on the host.
- In kernel drivers use `devm_gpiod_get()` then `devm_request_threaded_irq()` and handle sensor reads in the IRQ handler or a threaded handler.
- Alternative: periodic polling (workqueue or hrtimer), but interrupts are preferred for lower latency and less CPU use.

---

## Power management

- Connect VCC to a 3.3 V supply or controlled regulator.
- Control power state via the `PWR_MGMT_1` register in software to enter/exit low power modes.
- Implement suspend/resume in the driver to stop transfers and disable IRQs.

---

## DMA vs PIO

- For high sample rates or continuous high-bandwidth transfers, consider DMA-backed SPI transfers in the kernel to reduce CPU overhead.
- Check `spi_master` capabilities before enabling DMA.

---

## Testing & debugging checklist

1. Wiring: SCLK, MOSI, MISO, CS, INT, VCC (3.3V), GND wired correctly.
2. SPI settings: start with `mode=0` and `max_speed=1 MHz`.
3. Device tree: add the device node and `compatible` string.
4. Driver: call `spi_setup()`, use `spi_write_then_read()` for reads.
5. First test: read `WHO_AM_I` (reg `0x75`) — expected `0x71` for MPU9250.
6. Use a logic analyzer or scope to verify waveforms if data looks wrong.

Common issues:

- Wrong SPI mode.
- Insufficient CS hold or setup/teardown timing.
- Pull-ups/pull-downs or level shifting errors.
- Using 5 V signals without level shifting.

---

## Example quick checklist (summary)

- Pins wired and level-shifted where needed.
- SPI configured (mode, clock) and `spi_setup()` called in kernel.
- DTS node present and IRQ configured.
- WHO_AM_I test passes.

---

## Next steps

- Convert an existing I²C driver in this workspace to an SPI variant (kernel driver + `mpu9250.dts` update).
- Generate a kernel driver skeleton or a user-space test harness in C or Python.

If you want, I can implement a kernel SPI probe skeleton or a user-space tester for Raspberry Pi. Tell me which target you prefer.

---

## References

- MPU9250 datasheet (refer to your device's datasheet for exact details and register maps).
