#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>      // OpenMP multiprocessing library
#include <unistd.h>

#define FNAME "appleiigs_wiki_dither"
#define IW 640
#define IH 480
#define GW 8
#define GH 1

const char * const fnames[] = {
  "appleiigs_wiki_dither",
  "advwars",
  "solid_advwars",
  "advwars2",
  "cyberpunk",
  "doomtitle",
  "ironman",
  "kanagawa",
  "night_forest3",
  "persia",
  "pug",
  "shodan",
  "spock",
  "stark",
  "starry",
  "torii",
  "solid_ssfight2",
  "mklogo",
  "mki_orig",
  "AppleIILogo",
  "woodstock",
  ""
};

const char * fname = fnames[0];

#if GW == 8 && GH == 1
#define PERMFILE "8x1_perm_bytes.bin"
#elif GW == 8 && GH == 4
#define PERMFILE "8x4_perm_bytes.bin"
#elif GW == 8 && GH == 2
#define PERMFILE "8x2_perm_bytes.bin"
#elif GW == 2 && GH == 2
#define PERMFILE "2x2_perm_bytes.bin"
#elif GW == 4 && GH == 4
#define PERMFILE "4x4_perm_bytes.bin"
#elif GW == 4 && GH == 2
#define PERMFILE "4x2_perm_bytes.bin"
#elif GW == 4 && GH == 1
#define PERMFILE "4x1_perm_bytes.bin"
#elif GW == 8 && GH == 8
#define PERMFILE "8x8_perm_bytes.bin"
#elif GW == 8 && GH == 16
#define PERMFILE "8x16_perm_bytes.bin"
#else
#define PERMFILE "16x16_perm_bytes.bin"
#endif

#define PYPATH      "C:\\Users\\caian\\AppData\\Local\\Programs\\Python\\Python312\\python.exe"
#define UTILS_DIR   "util\\"
#define INPUT_DIR   "input\\"
#define TEMP_DIR    "temp\\"
#define OUTPUT_DIR  "output\\"

#define NUM_ROWS (IH/GH)
#define NUM_COLS (IW/GW)
#define CELL_SZ_BYTES (GW*GH*3)  // glyph width * height * 3
#define TRUTH_SZ (IW*IH*3)          // ground truth image
#define PERMS_SZ (256*GW*GH*64*64*3)   // glyph permutations (256 glyphs * 8x16 pixels * 64 bg colors * 64 fg colors * 3 bytes per pixel)

uint8_t truth_bytes[TRUTH_SZ];
uint8_t perms_bytes[PERMS_SZ];

int load_file(const char *fname, size_t fsz, uint8_t *dst)
{
  FILE* infile;
  //printf("Loading '%s'...",fname);
  infile = fopen(fname,"rb");
  if (infile)
  {
    size_t res = fread(dst, 1, fsz, infile);
    if(res!=fsz)
    {
      //printf("  File load error. %lu != %lu\n", (long unsigned int)res, (long unsigned int)fsz);
      fclose(infile);
      return -1;
    }
    //printf("  File loaded.\n");
    fclose(infile);
  } else
  {
    printf("  File not found!  %s\n",fname);
    return -1;
  }
  return 0;
}

uint64_t find_match(uint8_t *truth_bytes)
{
    uint32_t best_idx=0;
    uint32_t best_dif = 5000000;
    uint32_t dif = 0;
    uint8_t *t = truth_bytes;
    uint8_t *p = perms_bytes;
    for(uint32_t pidx = 0 ; pidx<(256*64*64);pidx++)
    {
      //int gidx = pidx/4096;
      //int cidx = pidx & 4095;
      //int fg = cidx&0b111111;
      //int bg = (cidx>>6)&0b111111;
      //if ((gidx<32)||(gidx>127))
      //  continue;
      //if ((bg>0)||(fg<63))
      //  continue;
      //p = perms_bytes+pidx*CELL_SZ_BYTES;
      dif = 0;
      t = truth_bytes;
      for(uint16_t i=0;i<CELL_SZ_BYTES;i++)
      {
        dif+=abs((int32_t)(*t) - (int32_t)(*p));
        t++;p++;
      }
      if(dif<best_dif)
      {
        best_dif = dif;
        best_idx = pidx;
      }
    }
    return ((uint64_t)best_dif<<32)|best_idx;
}

uint32_t sol[NUM_COLS*NUM_ROWS];
uint32_t cell_difs[NUM_COLS*NUM_ROWS];

int main()
{

  char cmdline[512];
  int gw = GW;
  int gh = GH;

  // for given cell size (gw x gh),
  // load as bytes all 4096 permutations of
  // glyph (256), bg color (64), and fg color (64)
  sprintf(cmdline, TEMP_DIR "%s", PERMFILE);
  if(load_file(cmdline, PERMS_SZ, perms_bytes))
  {
    printf("Generating permutation data...\n");
    sprintf(cmdline,PYPATH " " UTILS_DIR "makefonts.py");
    system(cmdline);
    sprintf(cmdline, TEMP_DIR "%s", PERMFILE);
    if(load_file(cmdline, PERMS_SZ, perms_bytes))
      return -1;
  }

  int fidx=0;
  while(1)
  {
    //printf("%s...\n",fname);
    #ifdef FNAME
    fname = FNAME;
    #else
    fname = fnames[fidx];
    if(!strlen(fname))
    {
      //printf("All images were done.\n");
      break;
    }
    fidx++;
    #endif // FNAME

    sprintf(cmdline,TEMP_DIR "%s_%dx%d_truth_bytes.bin",fname,gw,gh);
    //printf("truth load name: '%s'\n",cmdline);
    if(load_file(cmdline, TRUTH_SZ, truth_bytes))
    {
      // convert 640x480 image to bytes using python util, and load them in.
      sprintf(cmdline,PYPATH " " UTILS_DIR "convimg.py %d %d " INPUT_DIR "%s.png",gw,gh,fname);
      printf("%s\n",cmdline);
      system( cmdline );
      sprintf(cmdline,TEMP_DIR "%s_%dx%d_truth_bytes.bin",fname,gw,gh);
      if(load_file(cmdline, TRUTH_SZ, truth_bytes))
      {
        return -1;
      }
    }

    // for each cell of original image, find the glyph permutation
    // that matches the image most closely, and note it.

    //printf("\n\nStarting exhaustive search...\n");
    omp_set_num_threads(24);
    uint64_t tot_dif = 0;
    uint32_t glyph_cts[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
    uint64_t glyph_sums[256] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
    for(int y=0;y<NUM_ROWS;y++)
    {
      #pragma omp parallel for       // Using OpenMP to parallelize this loop
      for(int x=0;x<NUM_COLS;x++)
      {
        uint64_t midx = find_match(truth_bytes+(x*CELL_SZ_BYTES)+(y*NUM_COLS*CELL_SZ_BYTES));
        uint32_t perm_idx = midx&0xffffffff;
        uint8_t glyph_idx = perm_idx / 4096;
        uint32_t cell_dif = (midx>>32)&0xffffffff;

        glyph_cts[glyph_idx]++;
        glyph_sums[glyph_idx]+=cell_dif;
        uint16_t cell_idx = x+y*NUM_COLS;
        sol[cell_idx] = perm_idx;
        cell_difs[cell_idx] = cell_dif;
        tot_dif += cell_dif;
      }
    }
    // write the NUM_COLSxNUM_ROWS uint32_t permutation indexes
    sprintf(cmdline, OUTPUT_DIR "%dx%d_%s_glyph_idxs.bin",gw,gh, fname);
    //printf("Writing glyphidxs file '%s'...\n",cmdline);
    FILE *outfile = fopen(cmdline,"wb");
    fwrite(sol,1,(NUM_COLS*NUM_ROWS*4),outfile);
    fclose(outfile);

    sprintf(cmdline, OUTPUT_DIR "%dx%d_%s_cell_difs.bin",gw,gh, fname);
    //printf("Writing cell_difs file '%s'...\n",cmdline);
    outfile = fopen(cmdline,"wb");
    fwrite(cell_difs,1,(NUM_COLS*NUM_ROWS*4),outfile);
    fclose(outfile);

    // show glyph histogram
    uint8_t gi = 0;
    printf("     ");
    for(int x=0;x<16;x++)
      printf("%5u ", x);
    printf("\n");
    for(int y=0;y<16;y++)
    {
      printf("%3d: ",y*16);
      for(int x=0;x<16;x++)
      {
        uint32_t gc = glyph_cts[gi++];
        printf("%5u ", gc);
      }
      printf("\n");
    }

    uint64_t sz_orig = (IW*IH);
    uint64_t sz_new = (NUM_ROWS*NUM_COLS*3);
    float sz_savings = (float)(sz_orig-sz_new) * 100.0 / (float)sz_orig;
    uint64_t worst_dif = (IW*IH)*255*3;
    float img_dif = (float)tot_dif*100.0/worst_dif;
    printf("%s (%d x %d) sz_savings: %.1f%%, tot_pixel_dif: %.4f%%\n",fname, gw, gh, sz_savings ,img_dif);

    // Use python util to show final image result and output
    // in assembly language form for the Pugputer 6309.
    sprintf(cmdline,PYPATH " " UTILS_DIR "show_glyphart.py %d %d " OUTPUT_DIR "%dx%d_%s_glyph_idxs.bin",gw,gh,gw,gh, fname);
    system( cmdline );

    #ifdef FNAME
    break;
    #endif
  }


  return 0;
}
