#pragma once
///////////////////////////////////////////////////////////////////////////////
// Command definitions and handler framework
///////////////////////////////////////////////////////////////////////////////
#include "defines.h"
#include "helpers.h"
// ----------------------------------------------------------------------------
// Command system functions
// ----------------------------------------------------------------------------

// this is the prototype for a VDP command handler function
typedef void (*t_vdphandler)(uint8_t *args, uint16_t sz, bool first);


// This allows modules to specify callback functions for commands.
// args_size is the minimum number of arg bytes needed before passing data
// to the relevant handler function: If zero, the command takes no args, and 
// the handler is called as soon as that command is selected.

void command_init(uint8_t cmd_index, uint8_t initial_args_size, t_vdphandler cmd_fcn );

// Called when host specifies a cmd. Sets selection and inits args counter. 
void command_select(uint8_t cmd_index);  

// Called when args/data received to pass them to the current handler.
void data_receive(uint16_t *dat, uint sz);
// ----------------------------------------------------------------------------
// PugVDP Video Modes
// ----------------------------------------------------------------------------
typedef enum {
  MODE_TEXT    = 0,     // (80x30, 80x60, 80x120, 80x240) Text rgb222 fg/bg per char cell
  MODE_BITMAP_HIRES,    // 640x480 8 bpp palettized
  MODE_BITMAP_LORES,    // 320x240 16 bpp rgba5551
  // Not yet implemented:
  MODE_TILEMAP_HIRES,    // 640x480 tilemaps & sprites
  MODE_TILEMAP_LORES,    // 320x240 tilemaps & sprites

  } t_vidmode;  
// ----------------------------------------------------------------------------
// Command register names and addresses
// ----------------------------------------------------------------------------

#define MODE                  0x00  // For changing video mode (80-cols default)
                                    // The avail modes are shown in the enum above
// --- Text Commands ---

#define TEXT_RESET            0x10  // Resets active text area to whole screen,
                                    // clears it to current color, unhides cursor,
                                    // sends it home, and arg0 (byte) selects font:
                                    // 0:   FONT_8X16_VGA 80x30 (See doc/ for fonts)
                                    // 1:   FONT_8X16_SEMIGRAPHICS
                                    // 16:  FONT_8X8_CGA
                                    // 17:  FONT_8X8_SEMIGRAPHICS 
                                    // 32:  FONT_8X4_SEMIGRAPHICS
                                    // 64:  FONT_8X1_SEMIGRAPHICS  
#define TEXT_COLOR_SET        0x11  // (for putc, printc, define_area, clear, etc)
                                    // arg0: BG (RGB222),
                                    // arg1: FG (RGB222)                              
#define TEXT_SET_TAB          0x12  // arg0: uint8 spaces per tab
#define TEXT_SET_CURSOR_SIZE  0x13  // arg0: high/low nybs.: start/end pix rows
#define TEXT_CURSOR_SET_VIS   0x14  // arg0: 1: visible, 0:invisible
#define TEXT_CURSOR_SET_BLINK 0x15  // arg0: num frames per cursor blink toggle
                                    //       0: no blink!
#define TEXT_CURSOR_RESET     0x16  // Reset cursor attributes to defaults
#define TEXT_AREA_DEFINE      0x17  // Sets active area for printing and scrolling
                                    // arg0: uint8 screen x (0..39/79),
                                    // arg1: uint16 screen y (0..14/29),
                                    // arg2: uint8 area width,
                                    // arg3: uint16 area height
#define TEXT_CLEAR            0x18  // clears active area to current color
#define TEXT_HOME             0x19  // same as TEXT_GOTOXY(0,0)
#define TEXT_GOTOXY           0x1a  // Position cursor within active area.
                                    // arg0: uint8 curs x,
                                    // arg1: uint16 curs y
#define TEXT_PUTC             0x1b  // Each arg byte is written to active area,
                                    // advancing cursor, with wraparound to top.
                                    // (No control codes are handled.)
#define TEXT_PRINTC           0x1c  // Like PUTC except will handle some of the
                                    // ASCII control codes, and content will
                                    // scroll up past area bottom.
                                    // (Note an LF is treated as LF + CR.)
#define TEXT_SCROLL_H         0x1d  // arg0: int8 num area cols (positive: left)
#define TEXT_SCROLL_V         0x1e  // arg0: int16 num area lines (positive: up)
#define TEXT_FONT_RESET       0x1f  // resets current font to built-in 8x16 font
#define TEXT_FONT_SET         0x20  // allows font to be reprogrammed:
                                    // arg0: starting character code X (0..255),
                                    // args: glyph X,row 0 ... glyph X,row 15,
                                    // glyph X+1,row 0... (Wraps after glyph 255)
#define TEXT_TEST             0x21  // fills active area with colorful text
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//  !!! THE REST ARENT YET IMPLEMENTED !!!
//  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// --- Palette Commands ---

#define PALETTE_RESET         0x30  // Resets both 256-color palettes to default
                                    // palette 0 is for bitmaps and tilemaps, and
                                    // palette 1 is for sprites.
#define PALETTE_SET           0x31  // uint16 arg0:  starting color idx (0...511)
                                    //      0..255: bitmap/tilemap palette[256]
                                    //     256-511: sprite palette[256]
                                    //          Color N,hi byte, Color N,low byte,
                                    //         Color N+1,hi byte, Color N+1,low byte.
                                    //         ... (wraps around to 0 after 511)
// --- Bitmap graphics Commands ---

// For these, "color id" means different things depending on graphics mode:
// (either a palette index, a uint8 rgba2221, rgba2321, 
// rgba5551 [hi byte followed by lo byte])

#define BMP_CLEAR             0x40  // Set whole bmp to bg palette, color 0
#define BMP_SET               0x41  // bmp adrs hi, bmp adrs low, bmp byte0, bmp byte1 ...
#define BMP_MOVETO            0x42  // sets draw position. x hi, x lo, y hi, y lo
#define BMP_PLOT              0x43  // arg0: fg color id0, arg1: fg color id 1, ...
                                    // advances draw position to right with each pixel,
                                    // with wraparound at right and bottom
#define BMP_FLOODFILL         0x44  // arg0: fill tolerance (0:exact color, 255: vaguely same color)
#define BMP_COLOR_SET         0x45  // sets draw/fill color id
                                    // arg0: bg color id, arg1: fg color id
#define BMP_DRAWTO            0x46  // draw line from orig drawing pos to new pos.
                                    // arg0: x hi, arg1: x lo, arg2: y hi, arg3: y lo
                                    // ...
#define BMP_DRAW_RECT         0x47  // x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo
#define BMP_FILL_RECT         0x48  // x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo
#define BMP_DRAW_CIRC         0x49  // x hi, x lo, y hi, y lo, rad hi, rad lo
#define BMP_FILL_CIRC         0x4a  // x hi, x lo, y hi, y lo, rad hi, rad lo

// "Degrees" arguments to ARC-drawing functions below
// are int16 fixed-point integers int(degrees*50)

#define BMP_DRAW_ARC          0x4b  // x hi, x lo, y hi, y lo, rad hi, rad lo, deg0 hi, deg0 lo, deg1 hi, deg1 lo
#define BMP_FILL_ARC          0x4c  // x hi, x lo, y hi, y lo, rad hi, rad lo, deg0 hi, deg0 lo, deg1 hi, deg1 lo
#define BMP_DRAW_TEXT         0x4d  // arg0: obey_alpha (0: no, 1:yes),
                                    // arg1...: glyph index(s) to write
#define BMP_BLIT              0x4e  // copy screen rect to a specified dest pos.
                                    // x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo,
                                    // x1 hi, x1 lo, y1 hi, y1 lo,
                                    // obey_alpha (0: no, 1:yes)

// !!! TODO: Tilemap commands! !!!

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
