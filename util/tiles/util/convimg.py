# Given an image of a rendered SNES tilemap, decomposes it into
# unique tiles, eliminiating redundant tiles by accounting for
# optional per-tile h-flip/v-flip and also noting that each map 
# cell can be a stackup of up to 4 tiles with transparent areas. 

import struct,math,zlib
import subprocess
import time
import datetime
import random
import sys
import os
from copy import deepcopy

import contextlib
with contextlib.redirect_stdout(None):
    import pygame

os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'
utildir = os.path.dirname(os.path.realpath(__file__))
indir = os.path.join(utildir,'..','input')
tempdir = os.path.join(utildir,'..','temp')
outdir = os.path.join(utildir,'..','output')

pygame.init()
screen=pygame.display.set_mode( [ 640, 480 ] )

tile_w = 16
tile_h = 16
tiledims = (tile_w, tile_h)

def colordif(c0, c1):
    return abs(c0[0]-c1[0]) + abs(c0[1]-c1[1]) + abs(c0[2]-c1[2])

            # ttile     map            
def tilesim(t0_bytes, t1_bytes):
    dif = 0
    numopaque = 0
    ct = 0
    for i in range(0,len(t0_bytes),4):
        r0 = t0_bytes[ i + 0 ]
        g0 = t0_bytes[ i + 1 ]
        b0 = t0_bytes[ i + 2 ]
        a0 = t0_bytes[ i + 3 ]
        c0 = (r0, g0, b0)

        r1 = t1_bytes[ i + 0 ]
        g1 = t1_bytes[ i + 1 ]
        b1 = t1_bytes[ i + 2 ]
        a1 = t1_bytes[ i + 3 ]
        c1 = (r1, g1, b1)
        if (a0 > 127):
            dif += colordif(c0,c1)
            ct += 1
    if ct==0:
        return 0
    return (255*3) - dif/ct

def tiledif(t0_bytes, t1_bytes):
    dif=0
    for i in range(0,len(t0_bytes),4):
        r0 = t0_bytes[i+0]
        g0 = t0_bytes[i+1]
        b0 = t0_bytes[i+2]
        c0 = (r0,g0,b0)
        r1 = t1_bytes[i+0]
        g1 = t1_bytes[i+1]
        b1 = t1_bytes[i+2]
        c1 = (r1,g1,b1)
        dif+=colordif(c0,c1)
    return dif

# horizontal-flip tile image in bytes
def flip_horizontal(tile_bytes, width=16, height=16):
    """Flip a 16x16 RGBA tile horizontally (mirror left-right)."""
    bytes_per_pixel = 4
    row_size = width * bytes_per_pixel
    out = bytearray(len(tile_bytes))

    for y in range(height):
        row_start = y * row_size
        row_end = row_start + row_size
        row = tile_bytes[row_start:row_end]

        # reverse pixels within the row
        for x in range(width):
            src_start = x * bytes_per_pixel
            src_end = src_start + bytes_per_pixel
            dst_start = (width - 1 - x) * bytes_per_pixel
            out[row_start + dst_start:row_start + dst_start + bytes_per_pixel] = row[src_start:src_end]

    return out

def flip_vertical(tile_bytes, width=16, height=16):
    """Flip a 16x16 RGBA tile vertically (mirror top-bottom)."""
    bytes_per_pixel = 4
    row_size = width * bytes_per_pixel
    out = bytearray(len(tile_bytes))

    for y in range(height):
        src_start = y * row_size
        src_end = src_start + row_size
        dst_start = (height - 1 - y) * row_size
        out[dst_start:dst_start + row_size] = tile_bytes[src_start:src_end]

    return out

# checks if supplied tile is already a member of supplied tileset,
# either as-is, or if flipped horizontally and/or vertically.
# if tile is not new, the first element of returned tuple will
# be false, followed by the existing tile index and flips needed
# to make the existing tile match the given tile.
def tile_is_new(tile_bytes, tileset_bytes):
    min_dif = 256*3
    min_at = -1
    min_flips = -1
    for tidx,test_bytes in enumerate(tileset_bytes):
        for flips in range(0,4):
            fbytes = deepcopy(test_bytes)
            if flips&1:
                fbytes = flip_horizontal(fbytes)
            if flips&2:
                fbytes = flip_vertical(fbytes)
            dif = tiledif(fbytes, tile_bytes)
            if dif<min_dif:
                min_dif = dif
                min_at = tidx
                min_flips = flips
    is_new = min_dif>512 # there's some some weird slight color variations across my map
    return is_new, min_at, 0 if is_new else min_flips 

mapimg_name = os.path.join(indir,'simple_astoria.png')

print(f'Loading rendered tilemap "{mapimg_name}"...')
mapimg_surf = pygame.image.load(mapimg_name).convert_alpha()
mapimg_w = mapimg_surf.get_width()
mapimg_h = mapimg_surf.get_height()
map_tiles_w = mapimg_w // tile_w
map_tiles_h = mapimg_h // tile_h

# set window to map size, for now. Later we want to be able to handle
# maps or sets of maps which aret are larger than the screen, and we
# may have to scroll around to show everything.
screen=pygame.display.set_mode( [ mapimg_w, mapimg_h ] )
pygame.display.set_caption("Tilemap Conversion Fun")

# draw rendered map to screen
screen.blit( mapimg_surf, ( 0, 0 ), (0, 0, mapimg_w, mapimg_h ) )
pygame.display.update()

# save rendered map as a series of binary tiles for use
# in the more performant C / OpemMP code.
tempfile = os.path.join(tempdir,f'map_as_tiles_{map_tiles_w}x{map_tiles_h}.bin')
with open(tempfile,'wb') as f:
    for map_y in range(0, map_tiles_h):
        for map_x in range(0, map_tiles_w):
            map_px, map_py = map_x * tile_w, map_y * tile_h
            xmap_surf = pygame.Surface(tiledims,pygame.SRCALPHA)
            xmap_surf.blit( mapimg_surf, ( 0, 0 ), ( map_px, map_py, tile_w, tile_h ) )
            xmap_bytes = pygame.image.tobytes(xmap_surf, "RGBA")
            f.write(xmap_bytes)

# Go through each tile of rendered map making initial tileset.
# This step will omit tiles that are plain duplicates of existing
# tiles as well as ones which would be duplicates if mirrored 
# horizontally and/or vertically.

# (We may still have reduntant tiles common elements placed
# onto different tile backgrounds. We'll try to eliminate those in a
# subsequent step.)

print('Making initial tileset, omitting duplicates and h/v mirror-duplicates...')
tileset_bytes = []
tileset_surfs = []
map_tile_idxs = []
for map_y in range(0, map_tiles_h):
    map_tile_row_idxs = []
    for map_x in range(0, map_tiles_w):
        map_px, map_py = map_x * tile_w, map_y * tile_h
        # get next tile as a surf and as bytes
        xmap_surf = pygame.Surface(tiledims,pygame.SRCALPHA)
        xmap_surf.blit( mapimg_surf, ( 0, 0 ), ( map_px, map_py, tile_w, tile_h ) )
        xmap_bytes = pygame.image.tobytes(xmap_surf, "RGBA")
        isnew, tidx, flips = tile_is_new(xmap_bytes, tileset_bytes)
        if isnew:
            tidx = len(tileset_bytes)
            tileset_bytes.append(xmap_bytes)
            tileset_surfs.append(xmap_surf)
        map_tile_row_idxs.append((tidx,flips))
        screen.blit( xmap_surf, ( map_px, map_py ), ( 0, 0, tile_w, tile_h ) )
    map_tile_idxs.append(map_tile_row_idxs)
pygame.display.update()
print('Tilemap idxs:')
for row in map_tile_idxs:
    print(f'    {row}')
print(f'Initial tileset size is {len(tileset_surfs)} tiles for {map_tiles_w*map_tiles_h} map cells.')

print(f'Eliminating tiles which are redundant due to leyer-stacking...')

print('Saving tileset/tilemap...')
# save initial tileset as png and as bytes
timg_w = timg_h = int(math.sqrt(len(tileset_surfs))) + 1
timg_pw = timg_w * tile_w
timg_ph = timg_h * tile_h
ts_surf = pygame.Surface((timg_pw, timg_ph),pygame.SRCALPHA)
tx = 0
ty = 0 
tempname = os.path.join(tempdir, f"tileset_{tile_w}x{tile_h}x{len(tileset_surfs)}.bin")
with open(tempname,"wb") as f:
    for tidx in range(0,len(tileset_surfs)):
        xmap_surf = tileset_surfs[ tidx ]
        f.write( tileset_bytes[ tidx ] )
        ts_surf.blit( xmap_surf, ( tx, ty ), ( 0, 0, tile_w, tile_h ) )
        tx+=tile_w
        if tx>=timg_pw:
            tx=0
            ty+=tile_h
tempname = os.path.join(tempdir, f"tileset_{tile_w}x{tile_h}x{len(tileset_surfs)}.png")
pygame.image.save(ts_surf,tempname)

# Because map tiles are really a stackup of 1 to 4 tiles, 
# where portions of some tiles are transparent, and tiles 
# can be flipped horizontally and/or vertically, we can 
# eliminate a lot redundant tiles if we can determine the
# stackup configuration for each map cell, eliminate 
# redundant tiles, and make portions of some remaining 
# tiles transparent.

while True:
  for event in pygame.event.get():
    if event.type == pygame.QUIT:
      exit()
