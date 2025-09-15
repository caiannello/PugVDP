### Introduction
```
///////////////////////////////////////////////////////////////////////////////
// 
// PugVDP v0.1 - Retrocomputer Video Display Processor for a Pi Pico 2 !!
//
// Adapted from Luke Wren's PicoDVI project on GitHub. (He did all of the
// heavy lifting regarding video generation! Thanks, Luke!!)
//
// Started July 2025 by Craig Iannello
//
// This project started with Luke's colour_terminal demo. The font was 
// changed to an 8x16 IBM VGA font, and a PIO-based 8-bit bus interface 
// was bolted-on, described below, and command handlers and video modes 
// were added. 
//
// The intent is to use an RP2350, hooked directly to the bus of the 
// Pugputer 6309, to act as the display chip.
//
// Hopefully, other people will find this useful for their retrocomputers!
//
///////////////////////////////////////////////////////////////////////////////
```
### Current Features:

648x480 display via DVI/HDMI 
Text display mode:
	Each character cell has configurable background/foreground color (64 colors - RGB222). IBM-style cursor with adjustable height, visibility, and blink rate.

	Font can be reprogrammed by the host at runtime.
	
	Six built-in fonts:
	-	8x16 IBM VGA (80 cols x 30 rows)
	-	8x16 ASCII + Semigraphics
	-	8x8	 IBM CGA (80 cols x 60 rows)
	-	8x8  ASCII + Semigraphics
	-	8x4  semigraphics only (80 cols x 120 rows)
	-	8x1  semigraphics only (80 cols x 480 rows)	


<img src="https://github.com/caiannello/PugVDP/blob/main/media/lorem.png?raw=true" width="640" height="480" />
<img src="https://github.com/caiannello/PugVDP/blob/main/media/multicolor.png?raw=true" width="640" height="480" />

### Importing new fonts

`util/conv/` is the utility used to convert font images and palettes to C source for inclusion into the PugVDP sourcecode. Fonts can also be reprogrammed by the host CPU, as shown in the included assembly-language demo for Motorola MC6809-compatible CPU's.

### Semigraphics demos

In the doc folder are images of the built-in fonts. (No actual docs are yet in there, though.) There's some animated gifs in the media folder demonstrating semigraphics, and the tool used to produce them from PNG files is in `util/glyph art`. Some example outputs are in `util/glyph_art/output` .

**This is an animated gif showing how an image can by represented by various fonts in text mode:**

<img src="https://github.com/caiannello/PugVDP/blob/main/media/torii_semigraphics_8x8_8x4_8x2_8x1_4x2.gif?raw=true" width="640" height="480" />

### Planned Features

I've been having too much fun with semigraphics, but I intend to add some proper bitmapped graphics modes with commands to draw lines and shapes at various color depths. I also want to add a tiles & sprites mode to support game development. ( Hoping to have SNES-level graphics, with independently scrollable tile layers and multiple sprites.)

Also, the Pi Pico 2 adds some very helpful hardware support (called HSTX) for generating video signals with less CPU impact. This feature is not yet used, but should free up the CPU for more fun video modes and features.

### Building PugVDP and Luke Wren's LibDVI:

Note, the Raspberry pi pico SDK must be installed, per their instructions. In my case, 
and as shown in the example below, mine is in `/home/user/pico-sdk`.

	cd PugVDP/src/build
	cmake --fresh  -DPICO_SDK_PATH=/home/user/pico-sdk -DPICO_PLATFORM=rp2350 -DPICO_BOARD=adafruit_feather_rp2350 -DPICO_COPY_TO_RAM=1 -DDVI_DEFAULT_SERIAL_CONFIG=pico_sock_cfg ..
	make

If all goes well, the uf2 file will appear here:

	PugVDP/src/build/pugvdp/PugVDP.uf2

### PugVDP Bus Interface

Ten contiguous GPIO pins are needed for the bus interface. The starting GPIO number is specified in `src/pugvdp/defines.h`:

`#define GPIO_BASE   		    20`

My demo platform is an Adafruit Feather RP2350 with HSTX, using the available FFC HDMI accessory. **I had to remove the RGB LED and reuse its GPIO line (GPIO21) to get ten contiguous GPIO's !**. 

GPIO assignments are as follows:

-	GPIO20 - /CS ( Chip-select, active low. )
-	GPIO21 - A0 ( 0: Write command, 1: Write data/args )
-	GPIO22 - D0
-	GPIO23 - D1
-	GPIO24 - D2
-	GPIO25 - D3
-	GPIO26 - D4
-	GPIO27 - D5
-	GPIO28 - D6
-	GPIO29 - D7

### PugVDP Commands

See src/pugvdp/commands.h for command list and video modes. Currently, only the text/semigraphics mode is implemented, but I hope to add bitmap modes, tile layers, and sprites!

### Demo code for MC6309 CPU

`/src/MC6809_demo/PUGVDP_DEMO.ASM`