///////////////////////////////////////////////////////////////////////////////
//
// Misc Helper functions
//
///////////////////////////////////////////////////////////////////////////////
#include "helpers.h"

// ----------------------------------------------------------------------------
// Converts an RGB222 color into the GBR222 format as expected by 
// the TMDS encoder when in RGB222 color modes.

inline uint8_t rgb_gbr(uint8_t rgb)
{
  uint8_t r=(rgb>>4)&3;
  uint8_t g=(rgb>>2)&3; 
  uint8_t b=(rgb>>0)&3;
  return (g<<4) | (b<<2) | r;
}


inline uint8_t gbr_rgb(uint8_t gbr)
{
  uint8_t g=(gbr>>4)&3;
  uint8_t b=(gbr>>2)&3; 
  uint8_t r=(gbr>>0)&3;
  return (r<<4) | (g<<2) | b;
}


///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////