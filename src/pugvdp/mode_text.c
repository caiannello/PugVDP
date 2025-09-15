///////////////////////////////////////////////////////////////////////////////
//
// All functions and globals related to text-mode displays
//
///////////////////////////////////////////////////////////////////////////////
#include "mode_text.h"
#include "fonts.h"
// ----------------------------------------------------------------------------
uint16_t text_font_data_sz;            // glyph_width * 8 / glyph_height
uint8_t  font_buf[16*1*FONT_N_CHARS];  // buffers rom font so it can be modified by host
                                       // big enough for largest font size: 8x16 * 256
uint8_t   text_font_char_width, 
          text_font_char_height;
uint8_t   text_char_cols;
uint16_t  text_char_rows;
uint16_t  text_color_plane_size_words; // = (text_char_rows * text_char_cols * 4 / 32);

char      text_buf[480 * 80 ];  // worst-case sizes for buffers
uint32_t  colorbuf[3 * (480 * 80 * 4 / 32) ]; // .||.


uint8_t *font_set_pointer = font_buf;   // used when host reprogramming font
uint8_t  font_set_row_ct = 0;  // for counting glyph rows that were set (0..15) so we know when to move to next glyph
      // cursor configurables
uint8_t     text_tab_size = TEXT_DEFAULT_TAB_SZ; // tab width in spaces
                                        // cursor (row, col) on text screen,
uint8_t     text_curs_x = 0;            // always in relation to screen (0,0),
uint16_t     text_curs_y = 0;            // regardless of active area setting.
bool        text_curs_vis = true;       // true: show, false: hide

uint8_t     text_curs_row0;             // cursor starting row within cell 0..(cell_h-1)
uint8_t     text_curs_default_row0;
uint8_t     text_curs_row1;             // cursor ending row within cell 0..(cell_h-1)
uint8_t     text_curs_default_row1;
uint8_t     text_curs_blink_period = TEXT_CURS_DEFAULT_BLINKPD;
      // text cursor internal state
uint8_t     text_curs_glyph;
uint8_t     text_curs_count = 0;        // counts frames until blink toggle
uint8_t     text_curs_state = true;     // true: is blinked on, false: is blinked off 
// user can specify an active area for printing, wrapping text, 
// positioning cursor, and scrolling. (useful for multi-paned text UI's)
uint8_t     text_area_x = 0;
uint8_t     text_area_y = 0;
uint8_t     text_area_w = 80;
uint16_t     text_area_h = 30;
// Colors used when clearing an area or outputting new text (RGB222)
// todo: use these colors when showing cursor. Cursor currently uses whatever
// colors are assigned to the cell it is on.
uint8_t     text_color_bg = TEXT_COLOR_DEFAULT_BG;
uint8_t     text_color_fg = TEXT_COLOR_DEFAULT_FG;
const char * text_rom_font = NULL;  // points to most recently-used rom font

// ----------------------------------------------------------------------------
// refreshes the text mode screen - called 60 times per second (hopefully).
inline void text_rasterize_screen(void)
{
  uint16_t   last_char_cell_y = 255;
  bool curs_row = false;  // true while rasterizing row containing cursor

  // check if time to toggle cursor
  if (++text_curs_count>=text_curs_blink_period)
  {
    text_curs_count=0;
    if(text_curs_blink_period)            // if blinkrate is nonzero,
      text_curs_state = !text_curs_state; // toggle blink, else
    else text_curs_state = true;          // stay lit always.
  } 

  char      char_row_buf[80];  
  uint32_t  color_row_buf[3][80];
  if (text_curs_state && text_curs_vis) // if cursor is visible...
  {
    // make a copy of the row at the cursor and stage the cursor on it.
    // (We'll rasterize from this buffered row, as needed to display the
    // cursor.)
    memcpy( char_row_buf, 
            text_buf + text_curs_y * text_char_cols, 
            text_char_cols);            // copy row text, as well as
    for(int plane=0;plane<3;plane++)    // fg/bg colors.
      memcpy( color_row_buf[plane],
              &colorbuf[
                text_curs_y * (text_color_plane_size_words / text_char_rows) + 
                plane * text_color_plane_size_words
                ],
              text_char_cols / 2);

    char_row_buf[text_curs_x] = text_curs_glyph; // poke cursor glyph (a blank one works, since fg/bg get inverted!)

    // set cursor cell color to the current fg/bg settings, except swapped.
    uint char_index = text_curs_x;
    uint bit_index = char_index % 8 * 4;
    uint word_index = char_index / 8;
    uint8_t fg = rgb_gbr(text_color_bg);
    uint8_t bg = rgb_gbr(text_color_fg);
    for (int plane = 0; plane < 3; ++plane) 
    {
      uint32_t fg_bg_combined = (fg & 0x3) | (bg << 2 & 0xc);
      color_row_buf[plane][word_index] = (color_row_buf[plane][word_index] & ~(0xfu << bit_index)) | (fg_bg_combined << bit_index);
      fg >>= 2;
      bg >>= 2;
    }
  }  

  // start rasterizing all the scanlines
  for (uint y = 0; y < FRAME_HEIGHT; ++y) 
  {
    uint16_t char_cell_y = y / text_font_char_height;
    uint16_t char_cell_row = y % text_font_char_height;
    if (char_cell_y!=last_char_cell_y)   // if starting a new text row,
    {
      last_char_cell_y = char_cell_y;
      // note whether this row has the cursor
      curs_row = (char_cell_y == text_curs_y);
    }

    // output a scanline in 6-bit color
    uint32_t *tmdsbuf;
    queue_remove_blocking(&dvi0.q_tmds_free, &tmdsbuf);
    if (  curs_row && 
          text_curs_state && 
          text_curs_vis && 
          (char_cell_row >= text_curs_row0) && 
          (char_cell_row <= text_curs_row1))  // if cursor is showing on this scanline
    {
      for (int plane = 0; plane < 3; ++plane) // rasterize from buffered row
      {
        tmds_encode_font_2bpp(
          (const uint8_t*)char_row_buf,
          (const uint32_t *)(color_row_buf[plane]),
          tmdsbuf + plane * (FRAME_WIDTH / DVI_SYMBOLS_PER_WORD),
          FRAME_WIDTH,
          (const uint8_t*)&font_buf[y % text_font_char_height * FONT_N_CHARS] - FONT_FIRST_ASCII
        );
      }
    } else                                    // no cursor showing, rasterize normally.
    {
      for (int plane = 0; plane < 3; ++plane) 
      {
        tmds_encode_font_2bpp(
          (const uint8_t*)&text_buf[char_cell_y * text_char_cols],
          &colorbuf[char_cell_y * (text_color_plane_size_words / text_char_rows) + plane * text_color_plane_size_words],
          tmdsbuf + plane * (FRAME_WIDTH / DVI_SYMBOLS_PER_WORD),
          FRAME_WIDTH,
          (const uint8_t*)&font_buf[y % text_font_char_height * FONT_N_CHARS] - FONT_FIRST_ASCII
        );
      }

    }
    queue_add_blocking(&dvi0.q_tmds_valid, &tmdsbuf);
  }

}

// ----------------------------------------------------------------------------
// pokes a char on screen at (x,y).
//
inline void text_cell_set_char(uint x, uint y, char c) 
{ // put a char into text screen at (x,y)
  if (x >= text_char_cols || y >= text_char_rows)
    return;
  text_buf[x + y * text_char_cols] = c;
}

// ----------------------------------------------------------------------------
// Set bg/fg color of a text cell at (x,y). (both colors are RGB222)
//
inline void text_cell_set_color(uint x, uint y, uint8_t bg, uint8_t fg) 
{
  if (x >= text_char_cols || y >= text_char_rows)
    return;
  uint char_index = x + y * text_char_cols;
  uint bit_index = char_index % 8 * 4;
  uint word_index = char_index / 8;
  fg = rgb_gbr(fg);
  bg = rgb_gbr(bg); 
  for (int plane = 0; plane < 3; ++plane) 
  {
    uint32_t fg_bg_combined = (fg & 0x3) | (bg << 2 & 0xc);
    colorbuf[word_index] = (colorbuf[word_index] & ~(0xfu << bit_index)) | (fg_bg_combined << bit_index);
    fg >>= 2;
    bg >>= 2;
    word_index += text_color_plane_size_words;
  }
}
// ----------------------------------------------------------------------------
inline void text_copy_row(int dest_char_idx,int src_char_idx,int length)
{
  memcpy(text_buf+dest_char_idx, text_buf+src_char_idx,length);
  // move color row
  uint dst_wi;
  uint dst_bi;
  uint dst_st_bit_index = dest_char_idx % 8 * 4;
  uint dst_st_word_index = dest_char_idx / 8;

  uint src_wi;
  uint src_bi;
  uint src_st_bit_index = src_char_idx % 8 * 4;
  uint src_st_word_index = src_char_idx / 8;

  for (uint plane=0;plane<3;plane++)
  {
    src_wi = src_st_word_index;
    src_bi = src_st_bit_index;
    dst_wi = dst_st_word_index;
    dst_bi = dst_st_bit_index;

    for (int l=length;l>0;l--)
    {
      if ((l>=8)&&(src_bi==0)&&(dst_bi==0)) // if have a full word of colors to move
      {
        colorbuf[dst_wi] = colorbuf[src_wi];
        src_wi+=1;
        dst_wi+=1;
        l-=7;
      } else  // slow way - need to optimize this away from the single-cell code
      {
        colorbuf[dst_wi] = (colorbuf[dst_wi] & ~(0xfu << dst_bi)) | (colorbuf[src_wi] & (0xfu << src_bi));
        src_bi+=4;
        if(src_bi>28)
        {
          src_bi=0;
          src_wi+=1;
        }
        dst_bi+=4;
        if(dst_bi>28)
        {
          dst_bi=0;
          dst_wi+=1;
        }
      }
    }
    src_st_word_index+=text_color_plane_size_words;
    dst_st_word_index+=text_color_plane_size_words;
  }  
}
// ----------------------------------------------------------------------------
inline void text_clear_row(int dest_char_idx, int length, uint8_t bg, uint8_t fg)
{    
  uint32_t fg_bg_combined;
  uint wi;
  uint bi;
  memset(text_buf+dest_char_idx,0,length);    // set row chars to nulls
  uint st_bit_index = dest_char_idx % 8 * 4;
  uint st_word_index = dest_char_idx / 8;
  fg = rgb_gbr(fg);
  bg = rgb_gbr(bg); 
  uint32_t w;

  for (int plane = 0; plane < 3; ++plane) 
  {
    fg_bg_combined = (fg & 0x3) | (bg << 2 & 0xc);
    wi = st_word_index;
    bi = st_bit_index;
    w=0;
    for (int l=length;l>0;l--)  
    {
      if((l>=8)&&(bi==0))  // Can write a uint32 at a time
      {
        if(!w)
        {
          for(int i=0;i<32;i+=4)          
            w |= (fg_bg_combined << i);
        }
        colorbuf[wi]=w;
        wi+=1;
        l-=7; 
      } else   // cant do whole uint32, so instead do a cell at a time.
      {
        colorbuf[wi] = (colorbuf[wi] & ~(0xfu << bi)) | (fg_bg_combined << bi);
        bi+=4;
        if(bi>28)
        {
          bi=0;
          wi+=1;
        }     

      }
    }
    fg >>= 2;
    bg >>= 2;
    st_word_index += text_color_plane_size_words;    
  }
}
// ----------------------------------------------------------------------------
// Clear active area to null chars in current colors
// 
void text_clear(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  for(int y=text_area_y;y<text_area_y+text_area_h;y++)
    text_clear_row(y*text_char_cols+text_area_x,text_area_w, text_color_bg, text_color_fg);
}
// ----------------------------------------------------------------------------
// position cursor to (x,y) within current active area
//
void text_gotoxy(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(sz<3)
    return;
  uint8_t x = *(arg_bytes+0);
  uint16_t y = *(uint16_t*)(arg_bytes+1);
  if ((x >= text_area_w) || (y >= text_area_h))
    return;
  text_curs_x =  text_area_x + x;
  text_curs_y =  text_area_y + y;
}
// ----------------------------------------------------------------------------
void text_home(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_gotoxy((uint8_t*)"\x00\x00",2,true);
}
// ----------------------------------------------------------------------------
// Resets font to defaults
void text_font_reset(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  memcpy(font_buf, text_rom_font, text_font_data_sz);
}
// ----------------------------------------------------------------------------
// Resets active text area to whole screen, clears it to current color, 
// sends cursor home, selects specified font, and font to rom default.
// 0: 8x16 font, 1: 8x8 font, 2: 8x4 semigraphics font
//
void text_reset(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(!sz)
    return;
  uint8_t arg = arg_bytes[0];
  
  // look for specified font idx and load it up if found.
  for(int i = 0 ; i < NUM_ROM_FONTS; i++)
  {
    if (ROM_FONTS[i].font_idx == arg)
    {
      text_font_char_width = ROM_FONTS[i].char_width;
      text_font_char_height = ROM_FONTS[i].char_height;
      text_curs_glyph = ROM_FONTS[i].curs_char;
      text_curs_default_row0 = text_curs_row0 = ROM_FONTS[i].curs_row0;
      text_curs_default_row1 = text_curs_row1 = ROM_FONTS[i].curs_row1;    
      text_font_data_sz = text_font_char_height*(text_font_char_width/8)*FONT_N_CHARS;
      text_char_cols = FRAME_WIDTH / text_font_char_width;
      text_char_rows = FRAME_HEIGHT / text_font_char_height;
      text_color_plane_size_words = (text_char_rows * text_char_cols * 4 / 32);
      text_area_x = 0;
      text_area_y = 0;
      text_area_w = text_char_cols;
      text_area_h = text_char_rows;
      text_rom_font = ROM_FONTS[i].bitmap;
      text_font_reset(NULL,0,true);
      text_clear(NULL,0,true);
      text_home(NULL,0,true);
      return;
    }
  }
}
// ----------------------------------------------------------------------------
// sets spaces per tab (default 4)  todo: handling tabs in PRINTC
//
void text_set_tab(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  if(!sz)
    return;
  text_tab_size = arg_bytes[0];
}
// ----------------------------------------------------------------------------
// uint8_t arg sets cursor appearance. High nybble is starting row, 0..15, and
// low nybble is ending row, 0..15.
//
void text_set_cursor_size(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  uint8_t a = arg_bytes[0];
  uint8_t r0 = (a>>4)&0x0f;
  uint8_t r1 = a&0x0f;
  if(r0<r1)
  {
    text_curs_row0=r0;
    text_curs_row1=r1;
  } else
  {
    text_curs_row0=r1;
    text_curs_row1=r0;
  }
}
// ----------------------------------------------------------------------------
// sets cursor visibility (default: true- visible)
//
void text_cursor_set_vis(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_curs_vis = arg_bytes[0];
}
// ----------------------------------------------------------------------------
// Set text screen active area for printing, wrapping, scrolling. 
// arg0: screen x, arg1: screen y, arg2: area width, arg3: area height
//
void text_area_define(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_curs_x = text_area_x = *(arg_bytes+0);
  text_curs_y = text_area_y = *(uint16_t*)(arg_bytes+1);
  text_area_w = *(arg_bytes+3);
  text_area_h = *(uint16_t*)(arg_bytes+4);
}
// ----------------------------------------------------------------------------
// Set current bg/fg colors (rgb222) for text print, screen clear.
// 
void text_color_set(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_color_bg = *(arg_bytes+0);
  text_color_fg = *(arg_bytes+1);
}
// ----------------------------------------------------------------------------
// arg0: num frames per blink toggle. 0: no blink!
//
void text_cursor_set_blink(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_curs_blink_period = arg_bytes[0];
}
// ----------------------------------------------------------------------------
// int8 cols_displacement - positive num: scrolls left, neg: scr. right
//
void text_scroll_h(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  int8_t cols_displacement = *(int8_t*)arg_bytes;
  if(!cols_displacement)
    return;
  int bw = abs(cols_displacement);
  if (bw>=text_area_w)              // if |displacement| >= area_height,
  {
    text_clear(NULL,0,true); // clear whole area
    text_curs_y=text_area_y;     // place cursor at start of new area
    if (cols_displacement<0) 
      text_curs_x = text_area_x;
    else 
      text_curs_x = text_area_x + text_area_w-1;
    return;       // done  
  }

  ////////////////////////////
  // TODO: Horizontal scroll 
  ////////////////////////////

}
// ----------------------------------------------------------------------------
// int8 lines_displacement - positive num: scrolls up (like LF), neg: scr. down
//
// !!! If any function takes too long, the host fifo will overflow! Need to
// try to optimize this, or increase size of fifo.
//
void text_scroll_v(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  int lines_displacement = *(int16_t*)arg_bytes;
  int bh,sy,dy,by,sx0,dx0;
  if(!lines_displacement)
    return;
  bh = abs(lines_displacement);
  if (bh>=text_area_h)              // if |displacement| >= area_height,
  {
    text_clear(NULL,0,true); // clear whole area
    text_curs_x=text_area_x;     // place cursor at start of new area
    if (lines_displacement<0) 
      text_curs_y = text_area_y;
    else 
      text_curs_y = text_area_y + text_area_h-1;
    return;       // done  
  }
  // height of preserved content
  int ph = text_area_h - bh;
  if (lines_displacement>0)
  {
    // source of preserved content
    sy = text_area_y + lines_displacement;
    // dest of preserved content
    dy = text_area_y;
    // dest of blanks
    by = text_area_y+ph;
    // linecopy order (top to bottom)
    sx0 = sy*text_char_cols+text_area_x;
    dx0 = dy*text_char_cols+text_area_x;
    for (int y=0;y<ph;y++)
    {
      // copy line from sy+y to dy+y
      text_copy_row(dx0,sx0,text_area_w);
      sx0+=text_char_cols;
      dx0+=text_char_cols;
    }
    text_curs_x=text_area_x;
    text_curs_y=text_area_y+text_area_h-bh;       
  } else 
  {
    sy = text_area_y+text_area_h-1+lines_displacement;
    dy = text_area_y+text_area_h-1;
    by = text_area_y ;   
    sx0 = sy*text_char_cols+text_area_x;
    dx0 = dy*text_char_cols+text_area_x;
    for (int y=0;y<ph;y++)
    {
      // copy line from sy+y to dy+y
      text_copy_row(dx0,sx0,text_area_w);
      sx0-=text_char_cols;
      dx0-=text_char_cols;
    }
    text_curs_x=text_area_x;
    text_curs_y=text_area_y;    
  }

  // introduce blanklines
  dy = (by*text_char_cols)+text_area_x;
  for (int y=0;y<bh;y++)
  {
    text_clear_row(dy,text_area_w,text_color_bg, text_color_fg);
    dy += text_char_cols;
  }

}
// ----------------------------------------------------------------------------
// does a linefeed
//
inline void do_lf()
{
  /*
  for(int x=text_curs_x;x<text_area_x+text_area_w;x++)
  {
    text_cell_set_char(x, text_curs_y,0);
    text_cell_set_color(x,text_curs_y,text_color_bg,text_color_fg);
  }
  */
  text_clear_row((text_curs_y*text_char_cols)+text_curs_x,(text_area_x+text_area_w)-text_curs_x, text_color_bg, text_color_fg);
  
  text_curs_x = text_area_x;  // wrap x
  if(++text_curs_y >= text_area_y+text_area_h)
  {
    // note, the scroll function should place the cursor 
    // at introduced blank line(s) automatically.
    text_curs_y = text_area_y + text_area_h - 1;
    text_scroll_v((uint8_t *)"\x01",1,true);
  }  
}
// ----------------------------------------------------------------------------
// Put ANY glyph within active area at cursor position, and advance cursor.
// Cursor wraps right to left, bottom to top. 
// (Does no scrolling or handling of control codes, unlike PRINTC.)
//
void text_putc(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  uint8_t *p = arg_bytes;
  while(sz)
  {
    text_cell_set_char(text_curs_x,text_curs_y,*(p++));
    text_cell_set_color(text_curs_x,text_curs_y,text_color_bg,text_color_fg);

    if(++text_curs_x >= text_area_x+text_area_w)
    {
      text_curs_x = text_area_x;  // wrap x
      if(++text_curs_y >= text_area_y+text_area_h)
      {
        text_curs_y = text_area_y;  // wrap y
      }
    }
    sz--;
  }
}
// ----------------------------------------------------------------------------
// like PUTC, but honors some ascii control codes, and autoscrolls the text 
// area upwards as needed.
//
void text_printc(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  uint8_t *p = arg_bytes;
  while(sz)
  {
    uint8_t c = *(p++);
    if(c == '\r')           // carriage return
    {
      text_curs_x = text_area_x;
    } else if (c == '\n')   // linefeed
    {
      do_lf();
    } else if (c == '\f')   // formfeed
    {
      text_clear(NULL,0,true);
      text_curs_x = text_area_x;
      text_curs_y = text_area_y + text_area_h - 1;
    } else
    {
      text_cell_set_char(text_curs_x, text_curs_y, c);
      text_cell_set_color(text_curs_x, text_curs_y, text_color_bg, text_color_fg);
      if(++text_curs_x >= text_area_x+text_area_w)
      {
        text_curs_x = text_area_x;
        if(++text_curs_y >= text_area_y+text_area_h)
        {
          // note, the scroll function should place the cursor 
          // at introduced blank line(s) automatically.
          text_curs_y = text_area_y + text_area_h - 1;
          text_scroll_v((uint8_t *)"\x01",1,true);
        }  
      }
    }

    sz--;
  }  
}
// ----------------------------------------------------------------------------
// arg0: starting glyph index, arg1: row 0 of glyph 0, arg2: row 1 of glyph 0..
// ,.. row 15 of glyph 255. Wraps back to to glyph 0 after that.
void text_font_set(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  uint8_t *p = arg_bytes;
  // if first data, the first byte is the glyph index
  if(first_data)
  {
    uint8_t font_starting_glyph_idx = *(p++);
    font_set_pointer = font_buf + font_starting_glyph_idx;
    font_set_row_ct = 0;
    sz--;
  } 

  while(sz)
  {
    *font_set_pointer = *(p++); // copy bytes to font data
    font_set_row_ct ++;
    font_set_pointer += 256; // move to next glyph row

    if(font_set_row_ct >=text_font_char_height ) // if finished a glyph
    {
      font_set_row_ct=0;
      font_set_pointer += 1;      // move to next glyph
    }

    // wraparound at end of font data
    if(font_set_pointer>=(font_buf+text_font_data_sz))
      font_set_pointer -= text_font_data_sz;
    sz--;
  }
}
// ----------------------------------------------------------------------------
// reset cursor attributes to defaults
void text_cursor_reset(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{
  text_curs_row0 = text_curs_default_row0;
  text_curs_row1 = text_curs_default_row1;
  text_curs_vis = true;
  text_curs_blink_period = TEXT_CURS_DEFAULT_BLINKPD;
}
// ----------------------------------------------------------------------------
// Test pattern - fills active area with multicolored text
//
void text_test(uint8_t* arg_bytes, uint16_t sz, bool first_data)
{  
  uint8_t fg = 8;
  uint16_t i = 0; 
  for (uint y = 0; y < text_area_h; ++y) 
  {
    for (uint x = 0; x < text_area_w; ++x) 
    {
      uint8_t bg = (i*64/(text_char_rows*text_char_cols));
      text_cell_set_char(x + text_area_x, y + text_area_y, i);
      text_cell_set_color(x + text_area_x, y + text_area_y, bg, fg);
      fg++;
      i++;
    }
  } 
}
// ----------------------------------------------------------------------------
void text_init(void)
{

  //           cmd_number    initial_arg_sz   handler_function
  command_init(TEXT_RESET,            1,      text_reset);
  command_init(TEXT_SET_TAB,          1,      text_set_tab);
  command_init(TEXT_SET_CURSOR_SIZE,  1,      text_set_cursor_size);
  command_init(TEXT_CURSOR_SET_VIS,   1,      text_cursor_set_vis);
  command_init(TEXT_CURSOR_SET_BLINK, 1,      text_cursor_set_blink);
  command_init(TEXT_CURSOR_RESET,     0,      text_cursor_reset);
  command_init(TEXT_AREA_DEFINE,      6,      text_area_define);
  command_init(TEXT_COLOR_SET,        2,      text_color_set);
  command_init(TEXT_CLEAR,            0,      text_clear);
  command_init(TEXT_HOME,             0,      text_home);
  command_init(TEXT_GOTOXY,           3,      text_gotoxy);
  command_init(TEXT_PUTC,             1,      text_putc);
  command_init(TEXT_PRINTC,           1,      text_printc);
  command_init(TEXT_SCROLL_H,         1,      text_scroll_h);
  command_init(TEXT_SCROLL_V,         2,      text_scroll_v);
  command_init(TEXT_FONT_RESET,       0,      text_font_reset);
  command_init(TEXT_FONT_SET,         1,      text_font_set);
  command_init(TEXT_TEST,             0,      text_test);

  // init textmode 80x30 ( 8x16 VGA font )
  text_reset((uint8_t *)"\x00", 1, true);

  // draw test pattern
  text_test(NULL, 0, true);

  // select printc as the poweron default command
  command_select(TEXT_PRINTC);

}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////
