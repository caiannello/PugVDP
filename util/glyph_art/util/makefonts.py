import struct,math,zlib
import pygame
import subprocess
import time
import datetime
import random
import sys
import numpy as np 


#xtra = '../'
xtra = ''

pygame.init()
WW = 640
HH = 480
SF = 1
screen=pygame.display.set_mode([WW*SF,HH*SF])#,flags=pygame.FULLSCREEN)
pygame.display.set_caption("Making font semigraphics fonts etc...")

def checkDupes(filename,gw,gh):
    fontsurf = pygame.image.load(filename).convert_alpha()
    gnums = []
    for i in range(255,-1,-1):
        gx = i % 16
        gy = i // 16        
        px = gx * gw
        py = gy * gh
        gsurf = pygame.Surface((gw,gh),pygame.SRCALPHA)
        gsurf.blit(fontsurf, (0,0), (px,py,gw,gh))
        gbytes = pygame.image.tobytes(gsurf,"RGB")
        s = '0b'
        ones = '0b'
        for j in range(0,len(gbytes),3):
            ones+='1'
            if gbytes[j+2]>127:
                s+='1'
            else:
                s+='0'
        gnum = int(s,2)
        ones = int(ones,2)
        #print(f'gidx: {i}, gnum: {gnum:016x}')
        oppgnum = gnum^ones
        if (gnum in gnums):
            print(f'  Glyph {i} ({gx},{gy}) is a plain duplicate.')
        elif (oppgnum in gnums):
            print(f'  Glyph {i} ({gx},{gy}) is an inverse duplicate.')
        else:
            gnums.append(gnum)

#checkDupes(f'{xtra}/temp/new8x4.png',8,4)
#exit()

def conv2221(rgb2221):
    r2 = (rgb2221>>5)&3
    g2 = (rgb2221>>3)&3
    b2 = (rgb2221>>1)&3
    a1 = rgb2221&1
    r = int(r2*255/3)
    g = int(g2*255/3)
    b = int(b2*255/3)
    a = int(a1*255)
    return (r,g,b,a)
    
def make_permutations(filename, gw, gh):
    print(f'Cell size {gw}x{gh}: Making all 4096 permutations of font glyph[256], bg color[64], fg color[64]...')
    fontsurf = pygame.image.load(filename).convert_alpha()    
    # glyph width and height
    topleft = (0, 0)
    glyphrect = (0,0,gw,gh)
    glyphdims = (gw,gh)
    # make a surface for each glyph in 64 colors (RGB222)
    md = 16 # len of glyph rows in font
    glyphs = {}
    for c in range(0,256):
        fx = (c%md)*gw
        fy = c//md*gh
        source_rect = (fx,fy,gw,gh)
        for fg in range(0,64):
            cfg = (fg<<1)|1
            pfg = conv2221(cfg)
            clrglyph_surf = pygame.Surface(glyphdims,pygame.SRCALPHA)
            clrglyph_surf.blit(fontsurf, topleft, source_rect)
            clrglyph_surf.fill(pfg,glyphrect,pygame.BLEND_RGBA_MULT)
            glyphs[c<<8|cfg] = clrglyph_surf
    # Now, for each glyph, and in all 64x64 permutations
    # of background / foreground colors, store a surface
    # and an equivalent numpy array. 
    perms = {}
    with open(f'{xtra}temp/{gw}x{gh}_perm_bytes.bin','wb') as f:
        for c in range(0,256):
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    exit()            
            for bg in range(0,64):
                cbg = bg<<1|1
                qbg = conv2221(cbg)
                for fg in range(0,64):
                    cfg = fg<<1|1
                    # draw char background
                    test_surf = pygame.Surface(glyphdims,pygame.SRCALPHA)
                    pygame.draw.rect(test_surf, qbg, glyphrect)
                    # blit fg colored char
                    test_surf.blit(glyphs[c<<8|cfg], topleft, glyphrect)
                    perm_bytes = pygame.image.tobytes(test_surf,"RGB")
                    f.write(perm_bytes)

def expand_set(insurf, gw, gh):
    print(f'expandset {gw}x{gh}  {insurf.get_width()} {insurf.get_height()}')
    imgbytes = pygame.image.tobytes(insurf,"RGB")
    # convert the glyphs to uint32_t's 
    gcodes = []
    origidxs=[]
    for g in range(0,256):
        gx = g%16*gw
        gy = g//16*gh
        s='0b'
        for y in range(0,gh):
            for x in range(0,gw):
                gpx = gx+x
                gpy = gy+y
                try:
                    b = imgbytes[(gpy*(gw*16)+gpx)*4:(gpy*(gw*16)+gpx)*4+4][-1]
                except:
                    print(f'{g=},{gx=},{x=},{gy=},{y=},{gpy=},{gw=},{gpx=}')
                    exit()
                b = 1 if b >0 else 0
                s+=str(b)
                #print(b,end='')
            #print() 
        gcode = int(s,2)
        if (not gcode in gcodes) and (not (gcode^0xffffffff) in gcodes):
            gcodes.append(gcode)
            origidxs.append(g) # to preserve the placement of the premade glyphs in the charset
        #print(f'> {s} 0x{gcode:08x}')
    print(f'\n{len(gcodes)} unique codes in initial set.')

    # expand set to 256 glyphs
    print('Iteratively expanding set to 256, please wait... ')
    expanded_codes = expand_codes(gcodes)


    # reorder font to put handmade glyphs in their original locations
    # ithin the character set
    reordered_codes = [ 0 ] * 256
    for i in range(0,256):
        if i in origidxs:
            origcode = gcodes[origidxs.index(i)]
            reordered_codes[i] = origcode
            to_move = expanded_codes.index(origcode)
            expanded_codes = expanded_codes[0:to_move] + expanded_codes[to_move+1:]
    for i in range(0,256):
        if not i in origidxs:
            reordered_codes[i] = expanded_codes[0]
            expanded_codes = expanded_codes[1:]

    # turn the 256 uint32_t's back into 8x4 bitmaps, 
    # combined into a 128x64 bitmap.

    fontsurf = pygame.Surface((gw*16, gh*16), pygame.SRCALPHA)
    for gidx in range(0,256):
        code = reordered_codes[gidx]
        pattern = b''
        for i in range(0,32):
            if code & 0x80000000:
                pattern += b'\xff\xff\xff\xff'
            else:
                pattern += b'\x00\x00\x00\x00'
            code<<=1
        glyphsurf = pygame.image.frombuffer(pattern, (gw, gh), 'RGBA')
        fx = gidx%16 * gw
        fy = gidx//16 * gh
        fontsurf.blit(glyphsurf, (fx,fy), (0,0,gw,gh)) 
    return fontsurf

def add_pseudographics(src_file, do_expand_set, gw, gh, octels, bit_pix_wid, bit_bix_hei):
    if src_file:
        fontsurf = pygame.image.load(f'{xtra}fonts/half_{gw}x{gh}.png').convert_alpha()
        yofs = 8 * gh
    else:
        fontsurf = pygame.Surface((gw*16, gh*16), pygame.SRCALPHA)
        yofs = 0
    pattern_idxs = []    
    for gidx in range(0,256):
        if (gidx not in pattern_idxs) and (gidx^0xff not in pattern_idxs):
            pattern_idxs.append(gidx)
            pattern = b'\x00\x00\x00\x00' * gw * gh
            for i in range(0,len(octels)):
                if gidx&(1<<i):
                    px,py = octels[i]
                    for y in range(0,bit_bix_hei):
                        for x in range(0,bit_pix_wid):
                            pidx = ((py+y)*gw + px + x) * 4
                            pattern = pattern[0:pidx] + b'\xff\xff\xff\xff' + pattern[pidx+4:]
            glyphsurf = pygame.image.frombuffer(pattern, (gw, gh), 'RGBA')
            pidx = len(pattern_idxs) - 1
            fx = pidx%16 * gw
            fy = (pidx//16 * gh)  + yofs
            fontsurf.blit(glyphsurf, (fx,fy), (0,0,gw,gh)) 
    # generate more glyphs to round out the set to 256
    #if (gw,gh) in [ (8,4), (8,2) ]:
    #if do_expand_set:
    #    fontsurf = expand_set(fontsurf, gw,gh)
    outfile = f'{xtra}temp/new{gw}x{gh}.png'
    pygame.image.save(fontsurf, outfile) 
    make_permutations(outfile, gw, gh)   

# given glyph dimensions, this dict provides 
# how to subdivide that space into eight or
# less 'pixels' for generating pseudographics
# characters.
glyph_divs = {
    # (gw, gh) : (Have_Src_img, do_Expand_Set, (pix w, pix h), [(top-left coord 0), (top-left coord 1), ... ]
    (16,16):    (True,  False, (4,8), [(0,0),(4,0),(8,0),(12,0),(0,8),(4,8),(8,8),(12,8)]),
    (8,16):     (True,  False, (4,4), [(0,0),(0,4),(0,8),(0,12),(4,0),(4,4),(4,8),(4,12)]),
    (8,8):      (True,  False, (2,4), [(0,0),(2,0),(4,0),(6,0),(0,4),(2,4),(4,4),(6,4)]),
    (8,4):      (True,  True,  (2,2), [(0,0),(2,0),(4,0),(6,0),(0,2),(2,2),(4,2),(6,2)]),
    (8,2):      (True,  False, (1,2), [(0,0),(1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0)]),
    (8,1):      (False, False, (1,1), [(0,0),(1,0),(2,0),(3,0),(4,0),(5,0),(6,0),(7,0)]),
    (4,4):      (True,  False, (1,2), [(0,0),(1,0),(2,0),(3,0),(0,2),(1,2),(2,2),(3,2)]),
    (2,2):      (False, False, (1,1), [(0,0),(1,0),(0,1),(1,1)]),
    (4,2):      (False, False, (1,1), [(0,0),(1,0),(2,0),(3,0),(0,1),(1,1),(2,1),(3,1)]),
    (4,1):      (False, False, (1,1), [(0,0),(1,0),(2,0),(3,0)]),    
}

def make_pseudogfx_fonts():
    for gw, gh in glyph_divs:
        have_src, do_expand_set, pix_dims, divs = glyph_divs[(gw,gh)]
        print(have_src, pix_dims, divs)
        add_pseudographics( have_src, do_expand_set, gw, gh, divs, pix_dims[0],pix_dims[1] )

def main():
    make_pseudogfx_fonts()

if __name__ == "__main__":
    main()



