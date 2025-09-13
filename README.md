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
// changed to an 8x16 IBM VGA font, and a PIO-based 8-bit bus was bolted-on 
// described below, and command handlers and video modes were added. 
//
// The intent is to use an RP2350, hooked directly to the bus of the 
// Pugputer 6309, to act as the display chip.
//
// Hopefully, other people will find this useful for their retrocomputers!
//
///////////////////////////////////////////////////////////////////////////////
```
#### Current Features:

648x480 display via DVI/HDMI 
Text display mode:
	Each character cell has configurable background/foreground color (64 colors - RGB222) 
	Font can be reprogrammed by the host at runtime.
	
	Five built-in fonts:
	-	8x16 IBM VGA (80 cols x 30 rows)
	-	8x16 ASCII + Semigraphics
	-	8x8	 IBM CGA (80 cols x 60 rows)
	-	8x8  ASCII + Semigraphics
	-	8x4  semigraphics only (80 cols x 120 rows)

### Semigraphics demos

In the doc folder are images of the built-in fonts. (No actual docs are yet in there, though.) There's some animated gifs in the media folder demonstrating semigraphice. The tool used to produce semigraphics images from PNG files is in util/glyph art, and some example outputs are in util/glyph_art/output .

### Importing new fonts

In util/conv/ is the utility used to convert font images and palettes to C source for inclusion into PugVDP.

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

My demo platform is an Adafruit Feather RP2350 with HSTX, using the available FFC HDMI accessory, and to get the ten contiquous GPIO's, *I had to remove the RGB LED and reuse its GPIO line (Pin 21) !*. 

Pin assignments are as follows:

-	Pin 20 - Chip-select, active low.
-	Pin 21 - A0 ( 0: Write command, 1: Write arguments / data )
-	Pin 22 - D0
-	Pin 23 - D1
-	Pin 24 - D2
-	Pin 25 - D3
-	Pin 26 - D4
-	Pin 27 - D5
-	Pin 28 - D6
-	Pin 29 - D7

### PugVDP Commands

See src/pugvdp/commands.h for command list and video modes. Currently, only the text/semigraphics mode is implemented, but I hope to add bitmap modes, tile layers, and sprites!

