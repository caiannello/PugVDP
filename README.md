### Introduction
```
///////////////////////////////////////////////////////////////////////////////
// 
// PugVDP v0.1 - Retrocomputer video display processor based on a Pi Pico 2!!
//
// Adapted from Luke Wren's PicoDVI project on GitHub. (He did all of the
// heavy lifting regarding video generation! Thanks, Luke!!)
//
// Started July 2025 by Craig Iannello
//
// This project started with Luke's colour_terminal demo. I first changed 
// the font to an 8x16 IBM VGA font, bolted-on a PIO-based 8-bit bus 
// interface, described below, and added some command handlers and video 
// modes. The intent is to use an RP2350, hooked directly to the bus of my 
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
	Font can be reprogrammed by the host at runtime
	Five built-in fonts ()


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

