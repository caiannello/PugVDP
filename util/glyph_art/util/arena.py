import struct,math,zlib
import pygame
import subprocess
import time
import datetime
import random
import sys
import numpy as np 

pygame.init()
WW = 640
HH = 480
SF = 1
screen=pygame.display.set_mode([WW*SF,HH*SF])#,flags=pygame.FULLSCREEN)
pygame.display.set_caption("Font arena...")

def bitwise_inverse(x):
    return ~x & 0xFFFFFFFF

def hamming_distance(a, b):
    return bin(a ^ b).count('1')

def generate_candidates(existing_codes, forbidden_codes, sample_size=10000):
    candidates = set()
    while len(candidates) < sample_size:
        candidate = random.getrandbits(32)
        if candidate not in existing_codes and candidate not in forbidden_codes:
            candidates.add(candidate)
    return candidates

# given a set of 8x4 glyphs where each is unique and
# no bitwise inverse of any glyph may be included in the set,
# fill out the set to 256 glyphs, maximizing the hamming distance 
# of the new glyphs vs existing ones,

def expand_codes(codes, target_size=256, sample_size=10000):
    existing_codes = set(codes)
    forbidden_codes = {bitwise_inverse(code) for code in existing_codes}

    while len(existing_codes) < target_size:
        candidates = generate_candidates(existing_codes, forbidden_codes, sample_size)
        best_candidate = None
        max_min_distance = -1

        for candidate in candidates:
            distances = [hamming_distance(candidate, code) for code in existing_codes]
            min_distance = min(distances)
            if min_distance > max_min_distance:
                max_min_distance = min_distance
                best_candidate = candidate

        if best_candidate is not None:
            existing_codes.add(best_candidate)
            forbidden_codes.add(bitwise_inverse(best_candidate))
        else:
            # If no suitable candidate is found, increase the sample size
            sample_size *= 2

    return list(existing_codes)

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
    md = 32 if gh == 16 else 16
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
    with open(f'../temp/{gw}x{gh}_perm_bytes.bin','wb') as f:
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
                    #par_test_arr = np.frombuffer(perm_bytes, dtype=np.uint8).astype(np.uint16) 
                    #perm_arr.append((c,bg,fg,test_surf))
                    #perms[(c,bg,fg)]=test_surf
                    #par_perm_arr.append((c,bg,fg,par_test_arr))  # amenable to parallelization        
print('Making initial glyph set...')
# 32 glyphs come from a png file, and 
# 128 more are made with an incrementing bit pattern.
gw = 8
gh = 4
pattern_idxs = []
fontsurf = pygame.image.load('../fonts/half_8x4.png')
octels = [(0,0),(2,0),(4,0),(6,0),(0,2),(2,2),(4,2),(6,2)] 
for gidx in range(0,256):
    if (gidx not in pattern_idxs) and (gidx^0xff not in pattern_idxs):
        pattern_idxs.append(gidx)
        pattern = b'\x00\x00\x00\x00' * gw * gh
        for i in range(0,8):
            if gidx&(1<<i):
                px,py = octels[i]
                for y in range(0,2):
                    for x in range(0,2):
                        pidx = ((py+y) * gw + px + x ) * 4
                        pattern = pattern[0:pidx] + b'\xff\xff\xff\xff' + pattern[pidx+4:]
        glyphsurf = pygame.image.frombuffer(pattern, (gw, gh), 'RGBA')
        pidx = len(pattern_idxs) - 1
        fx = pidx%16 * gw
        fy = pidx//16 * gh
        fontsurf.blit(glyphsurf, (fx,fy+32), (0,0,gw,gh)) 
pygame.draw.rect(fontsurf,(0,0,0,0),(0,8,8*16,32-8))
imgbytes = pygame.image.tobytes(fontsurf,"RGBA")

# convert the glyphs to uint32_t's 
gcodes = []
origidxs=[]
for g in range(0,256):
    gx = g%16*8
    gy = g//16*4
    s='0b'
    for y in range(0,4):
        for x in range(0,8):
            gpx = gx+x
            gpy = gy+y
            b = imgbytes[(gpy*128+gpx)*4:(gpy*128+gpx)*4+4][-1]
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

#print(expanded_codes)
#expanded_codes = [0, 2147483648, 3221225472, 3766484992, 4039114752, 4175462400, 4243636224, 50331648, 16777216, 4278190080, 3233808384, 4042260480, 808452096, 3084, 3233811468, 808455180, 4042263564, 202116108, 3435924492, 1010568204, 4244376588, 50531340, 2888030735, 44297763, 252706044, 3486514428, 1061158140, 4294966524, 4137064491, 368547375, 4294914096, 12336, 3233820720, 808464432, 4042272816, 202125360, 3435933744, 1010577456, 4244385840, 50540592, 3284398320, 859042032, 4092850416, 252702960, 3486511344, 1061155056, 4294963440, 15420, 3233823804, 808467516, 4042275900, 202128444, 117506048, 1150272062, 2947628099, 3505092169, 2656454215, 1344183891, 2857740885, 2073057887, 50580684, 3284389068, 859032780, 4092841164, 252693708, 3486502092, 1061145804, 4294954188, 447320170, 3649221223, 3284348976, 858992688, 4092801072, 252653616, 3486462000, 1061105712, 2778369140, 3113550450, 432507095, 2102174843, 2388795517, 4277723264, 2155905152, 565623429, 1452505222, 3142098059, 3579449998, 1779065487, 2971496079, 3983357591, 1683720345, 2611275929, 3284339724, 858983436, 4092791820, 252644364, 3486452748, 1061096460, 4294904844, 459564086, 2089393829, 2571233956, 2263713959, 4244422848, 4294951104, 3233857728, 49344, 808501440, 4042309824, 202162368, 3435970752, 251854848, 50528256, 3284336640, 858980352, 4092788736, 52428, 3233860812, 808504524, 4042312908, 202165452, 3435973836, 1010617548, 1061142720, 4244425932, 4042325244, 202177788, 3435986172, 1010629884, 50593020, 3284401404, 859045116, 4092853500, 1904944854, 535299800, 3772834016, 4161262810, 1715148514, 24748767, 1553345761, 4237034207, 47518953, 2844469481, 4042322160, 61680, 3233870064, 808513776, 202174704, 3435983088, 1010626800, 4244435184, 4177066232, 50589936, 1445307125, 3274574587, 4244438268, 64764, 4278124286, 3233873148, 1057948416, 2132739841, 520552704, 4294967040, 4278255360, 808516860, 1010565120, 4244373504, 202113024, 3435921408, 415956231, 2272055562, 1587900173, 3435936828, 1010580540, 4244388924, 50543676, 3284352060, 858995772, 4092804156, 252656700, 3486465084, 1061108796, 4294917180, 2662114070, 4114122520, 3709649197, 3946066734, 2120796974, 2562083637, 1648223545, 2489822914, 1500592456, 990282303, 3404372817, 2194492754, 232056658, 1431655765, 4100653399, 3525212508, 1764365660, 3347112797, 2942283615, 2696712543, 743609188, 3016631653, 1441007974, 345582443, 2536097651, 2057013623, 3545268602, 1759971195, 4065437063, 1382913932, 3165808015, 1235442578, 3789390740, 652556698, 2316441498, 885303197, 1658212262, 3423315880, 3195080617, 4294923690, 845962161, 1568113586, 2772591539, 1336968631, 165415354, 1500209597, 2987196862, 3780032962, 4294901760, 252641280, 3486449664, 1061093376, 1010614464, 50577600, 3284385984, 859029696, 4092838080, 252690624, 3486499008, 648184779, 3442993621, 2289802290, 2494804956, 2570360801, 150562788, 560537063, 3850706920, 3456653799, 1188607499, 1779969008, 1447742960, 4126864890, 2857762815]

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

# save font bitmap to disk.

fname = f'../temp/new{gw}x{gh}.png'
print(f'Saving set as "{fname}"...')
pygame.image.save(fontsurf, fname)  
print('Done!')

# make all 4096 fg/bg color permutations
make_permutations("../temp/new8x4.png",8,4)