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

See the gif files in the media folder for semigraphics examples. In the doc folder are images of the built-in fonts. (No actual docs are yet in there, though.)

### Building PugVDP and Luke Wren's LibDVI:

Note, the Raspberry pi pico SDK must be installed, per their instructions. In my case, 
and as shown in the example below, mine is in `/home/user/pico-sdk`.

	cd PugVDP/src/build
	cmake --fresh  -DPICO_SDK_PATH=/home/user/pico-sdk -DPICO_PLATFORM=rp2350 -DPICO_BOARD=adafruit_feather_rp2350 -DPICO_COPY_TO_RAM=1 -DDVI_DEFAULT_SERIAL_CONFIG=pico_sock_cfg ..
	make

If all goes well, the uf2 file will appear here:

	PugVDP/src/build/pugvdp/PugVDP.uf2

### Using PugVDP

#### PugVDP Bus Interface (Connecting the Raspberry Pi Pico 2 to a vintage CPU)

#### PugVDP Commands

	See src/pugvdp/commands.h for command list and video modes

