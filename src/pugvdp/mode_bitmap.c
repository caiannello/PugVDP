///////////////////////////////////////////////////////////////////////////////
//
// All functions and globals related to bitmap-mode displays
//
///////////////////////////////////////////////////////////////////////////////
#include "mode_bitmap.h"
// ----------------------------------------------------------------------------
uint8_t     bmp_buf[640*480];  // bmp 0 (640x480x256):  each u8 element is an index to a RGB555 color in palette 0
                               // bmp 1 (320x240x256):  each u8 element is an index to a RGB555 color in palette 0 
                               //                         (double buffered: first half, page 0, 2nd half page 1)
                               // bmp 2 (320x240):      each u16 element is an RGB555 color (no palette)
                               // bmp 3 (640x480x16):   each u4 element is an index to a RGB555 color in palette 0 
                               //                         (double buffered: first half, page 0, 2nd half page 1)

uint16_t    bmp_palette[512];  // 0..255: bg/bmp palette,  256..511: sprite palette

uint16_t    bmp_pal_idx = 0;  // next idx while host is setting palette colors (0...511)

uint16_t    bmp_curs_x = 0;   // pen position after a MOVETO, DRAWTO
uint16_t    bmp_curs_y = 0;         

uint16_t    bmp_dest_x = 0;   // ending pen position during a DRAWTO
uint16_t    bmp_dest_y = 0;         

// current palette color selections:
uint16_t     bmp_color_bg = 0;  // used when clearing
uint16_t     bmp_color_fg = 1;  // used when drawing

// ----------------------------------------------------------------------------
// refresh screen in text mode
inline void bmp_rasterize_screen(void)
{
  for (uint y = 0; y < 480; y++) 
  {
    /*
    uint32_t *tmdsbuf;
    queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
    for (int plane = 0; plane < 3; ++plane) 
    {
      tmds_encode_font_2bpp(
        //(const uint8_t*)&text_buf[char_cell_y * CHAR_COLS],
        (const uint8_t*)char_row_buf,
        &colorbuf[char_cell_y * (COLOR_PLANE_SIZE_WORDS / CHAR_ROWS) + plane * COLOR_PLANE_SIZE_WORDS],
        tmdsbuf + plane * (FRAME_WIDTH / DVI_SYMBOLS_PER_WORD),
        FRAME_WIDTH,
        //(const uint8_t*)&font_8x16_semi[y % FONT_CHAR_HEIGHT * FONT_N_CHARS] - FONT_FIRST_ASCII
        (const uint8_t*)&font_buf[y % FONT_CHAR_HEIGHT * FONT_N_CHARS] - FONT_FIRST_ASCII
      );
    }    
    queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
    */
  }
}
// ----------------------------------------------------------------------------
// Clear bitmap to current bg color
// 
void bmp_clear(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  memset(bmp_buf, bmp_color_bg, sizeof(bmp_buf));
}
// ----------------------------------------------------------------------------
// position pen to (x,y)
//
void bmp_moveto(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(sz<4)
    return;
  uint16_t x = *(uint16_t*)(arg_bytes+0);
  uint16_t y = *(uint16_t*)(arg_bytes+2);
  if ((x >= 640) || (y >= 480))
    return;
  bmp_curs_x =  x;
  bmp_curs_y =  y;
}
// ----------------------------------------------------------------------------
// Set current bg/fg colors (rgb222) for drawing, clearing.
// 
void bmp_color_set(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(sz<2)
    return;
  uint16_t bg = *(arg_bytes+0);
  uint16_t fg = *(arg_bytes+1);
  if ((bg >= 256) || (fg >= 256))
    return;
  bmp_color_bg = bg;
  bmp_color_fg = fg;
}
// ----------------------------------------------------------------------------
void palette_set(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  uint8_t *p = arg_bytes;
  if(sz<2)
    return;
  if(first_data)
  {
    bmp_pal_idx = *(uint16_t*)p;
    p+=2;
    sz-=2;
  }
  while(sz>=2)
  {
    bmp_palette[bmp_pal_idx] = *(uint16_t*)p;
    p+=2;
    sz-=2;
  }

}
// ----------------------------------------------------------------------------
// reset both 256-color palettes to default (see defines.h, conv.py)
//
void palette_reset(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
    memcpy(bmp_palette, default_palette, 512 * sizeof(uint16_t));
}
// ----------------------------------------------------------------------------
inline void plotPixel(int x, int y) 
{
  bmp_buf[y*640+x] = bmp_color_fg;
}
// ----------------------------------------------------------------------------
inline void bresenhamLine(int x1, int y1, int x2, int y2) 
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x1 < x2) ? 1 : -1; // Step direction for x
    int sy = (y1 < y2) ? 1 : -1; // Step direction for y
    int err = dx - dy; // Initial error term

    while (1) {
        plotPixel(x1, y1);

        if (x1 == x2 && y1 == y2) {
            break; // Reached the end point
        }

        int e2 = 2 * err; // Double the error for comparison

        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}
// ----------------------------------------------------------------------------
void bmp_drawto(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(sz<4)
    return;
  uint16_t x = *(uint16_t*)(arg_bytes+0);
  uint16_t y = *(uint16_t*)(arg_bytes+2);
  if ((x >= 640) || (y >= 480))
    return;
  bmp_dest_x =  x;
  bmp_dest_y =  y;  
  
  // bressenham line in selected color from curs x,y to dest x,y
  bresenhamLine(bmp_curs_x , bmp_curs_y, bmp_dest_x, bmp_dest_y );
  
  // leave cursor at dest.
  bmp_curs_x = bmp_dest_x;
  bmp_curs_y = bmp_dest_y;  

}
// ----------------------------------------------------------------------------
void bmp_init(void)
{
  //           cmd_number     min_arg_bytes   handler_function
  command_init(PALETTE_SET,           2,  palette_set);  // need to add another arg to these.. min arg sz after initial args
  command_init(PALETTE_RESET,         0,  palette_reset);
  command_init(BMP_COLOR_SET,         2,  bmp_color_set);
  command_init(BMP_CLEAR,             0,  bmp_clear);
  command_init(BMP_MOVETO,            4,  bmp_moveto);
  command_init(BMP_DRAWTO,            4,  bmp_drawto);
  palette_reset(NULL,0,true); // set default palette
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
