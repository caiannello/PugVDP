import struct
import pygame
import sys

import os
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'

# init graphics display
pygame.init()
WW = 640
HH = 480
SF = 1
screen=pygame.display.set_mode([WW*SF,HH*SF])#,flags=pygame.FULLSCREEN)
pygame.display.set_caption("GlyphArt")

gw = int(sys.argv[1])
gh = int(sys.argv[2])
fname = sys.argv[3]

permfile = f'temp/{gw}x{gh}_perm_bytes.bin'
#print(f'Reading glyph permutations file "{permfile}"...')
perms = []
with open(permfile,'rb') as f:
  while True:
    glyph_arr = f.read( gw*gh*3 )
    if glyph_arr == b'':
      break
    glyph_surf  = pygame.image.frombuffer(glyph_arr, (gw, gh), 'RGB')
    perms.append(glyph_surf)

idxs = []
#print(f'Reading permutation index file "{fname}"...')
with open(fname,'rb') as f:
  while True:
    ib = f.read(4)
    if ib == b'':
      break
    idxs.append(struct.unpack('<I', ib)[0])

# read cell differences from original image
nend = fname.find('_glyph_idxs')
basename = fname[0:nend]
difs = []
dname = f"{basename}_cell_difs.bin"
print(f'loading difs file : {dname}')
mindif = 999999999
maxdif = -999999999
with open(dname,'rb') as f:
  while True:
    ib = f.read(4)
    if ib == b'':
      break
    dif = struct.unpack('<I', ib)[0]
    if dif<mindif:
      mindif = dif
    if dif>maxdif:
      maxdif = dif
    difs.append(dif)

topleft = (0, 0)
glyphrect = (0,0,gw,gh)
glyphdims = (gw,gh)
j=0
binfile = f'{basename}_bgfgc.bin'
asmfile = f'{basename}_bgfgc.asm'

bgfile = f'{basename}_bg.bin'
fgfile = f'{basename}_fg.bin'
glfile = f'{basename}_gl.bin'

pngfile = f'{basename}.png'

with open(bgfile,'wb') as bgf:
  with open(fgfile,'wb') as fgf:
    with open(glfile,'wb') as glf:
      with open(binfile,'wb') as f:
        with open(asmfile,'wt') as o:
          for y in range(0,480,gh):
            o.write('    FCB  ')
            for x in range(0,640,gw):
              gi = idxs[j]
              if maxdif!=mindif:
                dif = (difs[j]-mindif)*255 // (maxdif-mindif)
              else:
                dif = 0
              dcolor = (dif,255-dif,0)
              j+=1    
              fg = gi%64
              bg = gi//64%64
              c = gi//4096
              o.write(f'${bg:02X},${fg:02X},${c:02X}')
              bgf.write(bytes([bg]))
              fgf.write(bytes([fg]))
              glf.write(bytes([c]))
              if x<(640-8):
                o.write(',')
              else:
                o.write('\n')
              f.write(bytes([bg,fg,c]))
              gsurf = perms[gi]
              # show approximated cell
              screen.blit(gsurf, (x,y), (0,0,gw,gh))
              # show cell difference from orig image on a scale from green to red
              #pygame.draw.rect(screen, dcolor, (x,y,gw,gh))
            pygame.display.update()   
pygame.image.save(screen, pngfile)   

#os.unlink(binfile)                  
#os.unlink(asmfile)                  
os.unlink(bgfile)                  
os.unlink(fgfile)                  
os.unlink(glfile)                  
os.unlink(dname)
os.unlink(fname)

print('All done, starting endloop.')
while True:
  for event in pygame.event.get():
    if event.type == pygame.QUIT:
      exit()
