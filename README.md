# Pico-PAL-Test-Card-Generator

A hardware PAL (625-line, 50 Hz, interlaced) test signal generator built around a **YD-RP2040** board. It outputs a composite video signal on an RCA jack, including a digitally synthesised PAL colour subcarrier, plus a 1 kHz test tone on a second RCA jack. Eight test patterns are selectable with one push button, including the Philips **PM5544**, the Telefunken **FuBK**, the Grundig **VG1001** (Bulgarian BNT/Nova TV variant) and a **UEIT** card.

There is no video chip involved. The RP2040 clocks out 8-bit samples straight from RAM through PIO and DMA, at four times the PAL subcarrier frequency, into a resistor-ladder DAC. Everything, including sync, burst and chroma, is computed in software.

## Features

- Full PAL composite video: sync, equalising pulses, colour burst, V-switch (PAL phase alternation), interlace
- 8 test cards, switchable at runtime with a push button
- Pixel clock locked to a 17.734475 MHz crystal (4 × fsc), so chroma is generated with exact phase relationships
- Zero CPU cost during scan-out: PIO, three DMA channels and an ISR do all the work
- Circular masking of the central pattern area (as on the original PM5544), with an anti-gap outline
- 1 kHz sine test tone via PWM and DMA, with RC filtering and a volume potentiometer
- Heartbeat LED, which stays lit while the button is held (useful as a wiring check)

## Test cards

| # | Card | Notes |
|---|------|-------|
| 0 | EBU colour bars | 100% bars with PAL burst |
| 1 | Crosshatch | White grid on grey |
| 2 | Checkerboard | Black/white 47 px cells |
| 3 | Multiburst | 1.0, 2.0, 3.0, 4.0, 4.43, 5.0 MHz sine bursts |
| 4 | **Philips PM5544** | Circular layout, LF/reflection checks, colour bars, gratings, grayscale, Y/C delay check, coloured side signals |
| 5 | **Telefunken FuBK** | Colour bars, grayscale, gratings, PAL test sectors, tapering triangle |
| 6 | **Grundig VG1001** (BNT / Nova TV style) | Castellated border, centre box with colour bars, gratings, grayscale |
| 7 | **UEIT** | 26×20 cell grid, colour bars, grayscale, stripes, slanted/gradient fields, gratings |

Press the button to step to the next card. After card 7 it wraps to card 0.

## Hardware

### Bill of materials

| Part | Value / type | Purpose |
|------|--------------|---------|
| U1 | YD-RP2040 board | Microcontroller (overclock-free, clocked externally) |
| U2 | 74HCU04D hex inverter | Crystal oscillator and clock buffer |
| Y1 | 17.7344 MHz crystal | Pixel clock (4 × PAL subcarrier, 17.734475 MHz nominal) |
| R | 1 MΩ | Oscillator feedback |
| C | 2 × 22 pF | Crystal load capacitors |
| R-2R ladder | 8 × 1 kΩ, 8 × 470 Ω, 1 × 1 kΩ termination | 8-bit video DAC |
| Q1 | BC547 (NPN) | Emitter-follower output buffer |
| R | 1 kΩ | Emitter pull-down |
| R | 75 Ω | Output series (source) impedance |
| R | 470 Ω | Audio low-pass |
| C | 100 nF | Audio low-pass |
| C | 4.7 µF | Audio DC blocking |
| RV1 | Potentiometer (volume) | Audio level |
| SW1 | Momentary push button | Card selection |
| J1, J2 | RCA jacks | Video and audio outputs |

### Pin map

| RP2040 pin | Function |
|-----------|----------|
| GP0–GP7 | 8-bit video DAC (GP0 = LSB, GP7 = MSB) |
| GP14 | Card-select button (to GND, internal pull-up) |
| GP15 | Audio PWM output |
| GP20 | External clock input (from the 74HCU04 oscillator) |
| GPIO25 | On-board LED (heartbeat) |
| 3V3 / GND | Power for the 74HCU04, BC547 collector and DAC reference |

### Clock

Two gates of the 74HCU04D are used. The first (1A→1Y), with the 1 MΩ feedback resistor, the crystal and the two 22 pF capacitors, forms a Pierce oscillator. An unbuffered (U-type) inverter is used on purpose, because it works as a linear amplifier in this role. The second gate (2A→2Y) buffers the oscillator output and drives GP20. Unused gate inputs are tied off.

The RP2040 then switches **clk_sys** directly to this external clock (GPIN0 as the `clk_sys` aux source). The system clock is therefore exactly 17.734475 MHz, one PIO instruction per pixel, which is 56.38 ns per sample.

### Video DAC and output stage

GP0–GP7 feed an R-2R style ladder (1 kΩ for the "2R" legs, 470 Ω for the "R" series elements, a 1 kΩ termination to ground). The ladder output drives the base of a BC547 emitter follower. The collector is on 3V3, and the emitter feeds a 1 kΩ pull-down and a 75 Ω series resistor to the RCA centre pin, giving a 75 Ω source impedance.

Signal levels (8-bit DAC codes):

| Level | Code |
|-------|------|
| Sync tip | 70 |
| Black / blanking | 116 |
| Grey | 170 |
| White | 224 |

### Audio

GP15 outputs a PWM signal (wrap value 276, about 64 kHz carrier). A 64-entry sine table is played through it by DMA, which gives a **~1 kHz tone**. A 470 Ω + 100 nF RC network filters the carrier. A 4.7 µF capacitor blocks DC, and a potentiometer sets the level at the RCA centre pin.

## How it works

### Video timing

- 625 lines per frame, 2:1 interlace, 576 visible lines
- 1136 samples per line at 17.734475 MHz, which is about 64.05 µs
- The line sequence repeats every 4 frames (2500 lines), which covers the 8-field PAL colour sequence
- 4 variants of each line buffer (`s_mod = line % 4`) carry the subcarrier phase and PAL V-switch state

### Signal path

```
 RAM line buffers ──▶ DMA compositor ──▶ ping/pong buffers ──▶ DMA ──▶ PIO ──▶ GP0–GP7 ──▶ R-2R DAC ──▶ BC547 ──▶ RCA
```

1. **PIO** (`video.pio`): a one-instruction program, `out pins, 8`, with a 32-bit autopull threshold. Each 32-bit word from the FIFO becomes four 8-bit pixels, one per clock.
2. **Ping-pong DMA**: two chained DMA channels stream 284 words (1136 bytes) per line into the PIO TX FIFO. Each channel raises an interrupt when it finishes.
3. **Line composition ISR** (`dma_isr` / `compose_line`): while one buffer is on screen, the ISR builds the next line in the other one. A third DMA channel copies the line from `base_table[]` (full-width background) and `special_table[]` (inner pattern), using the per-row circle masks `center_copy_dx[]` and `center_mask_dx[]`. The outline of the circle is drawn on top by the CPU.
4. **Line tables**: `base_table[2500]` and `special_table[2500]` hold one pointer per line of the 4-frame sequence. Each test card builder fills them in.
5. **Audio**: a second pair of DMA channels (data and control) loops the sine table into the PWM compare register with no CPU involvement.

### Colour generation

PAL chroma is synthesised at 4 × fsc, so every subcarrier cycle spans exactly 4 samples. U and V are added to the luma value according to the sample phase (`x % 4`), and V is inverted on alternate lines. The colour burst in `generate_hblank()` follows the same scheme. A 3-tap FIR low-pass (`apply_luma_lpf`) is applied to the luma-only parts of the card templates. Colour bars and gratings are added afterwards, so they stay sharp.

### Memory

All line buffers are 32-bit aligned, so the DMA can move four pixels at a time. They need about 230 KB of the RP2040's 264 KB SRAM. The cards share buffers through `#define` aliases (see `ueit.c` and `vg1001_bnt.c`), so each card can reuse buffers originally named for PM5544.

## Source layout

| File | Description |
|------|-------------|
| `main.c` | Hardware setup, clocking, PIO and DMA configuration, line compositor, ISR, button handling |
| `video.pio` | PIO program that outputs 8-bit pixels |
| `pm5544.c` | Sync/blanking templates, burst generation, shared helpers (`fill_color_bar`, `apply_luma_lpf`, overlays) and the PM5544 builder |
| `fubk.c` | Telefunken FuBK card |
| `vg1001_bnt.c` | Grundig VG1001 (BNT / Nova TV) card |
| `ueit.c` | UEIT card |
| `simple_patterns.c` | EBU bars, crosshatch, checkerboard, multiburst |
| `CMakeLists.txt` | Pico SDK build configuration |

The card sources are `#include`d into `main.c`, so the whole project is a single translation unit.

## Building

Requirements: Raspberry Pi Pico SDK (with `PICO_SDK_PATH` set), CMake ≥ 3.13 and an ARM GCC toolchain.

```bash
git clone <this-repo>
cd <this-repo>
mkdir build && cd build
cmake ..
make -j
```

The build produces `pico_pal_generator.uf2`.

## Flashing

1. Hold **BOOT** on the YD-RP2040 and plug in USB (or press RESET while holding BOOT).
2. Copy `pico_pal_generator.uf2` to the `RPI-RP2` drive that appears.

> **The board needs the external clock to run.** The firmware switches `clk_sys` to GP20 shortly after start. If the 17.7344 MHz oscillator is not running, the board will stall. Flash it with the oscillator wired up, or hold BOOT while powering up.

## Usage

1. Connect the video RCA jack to a PAL monitor, TV or capture device (75 Ω input).
2. Power the board over USB.
3. Press the button on GP14 to cycle through the cards.
4. Optional: connect the audio RCA jack for the 1 kHz tone.

## Known issues and notes

- Colour decoding can differ between capture and display paths, because of the digital subcarrier synthesis and the simple output stage.
- Colour-rendering behaviour should be checked on your own decoder and capture card.

## Customisation

- **Levels**: `SYNC_TIP`, `BLACK_LEVEL`, `GRAY_LEVEL`, `WHITE_LEVEL` at the top of `main.c`
- **Tone frequency**: change the PWM wrap value or the length of `audio_lut`
- **New cards**: add a `build_xxx()` function that fills the buffers and the two line tables, then add it to the button handler in `main()`

## Acknowledgements

- Philips PM5544, Telefunken FuBK, Grundig VG1001 and UEIT test cards are the work of their original designers. This project is an independent, hobbyist reproduction for equipment testing.

## License

Add your license here (for example MIT or GPL-3.0).