import struct,math,zlib
import pygame
import subprocess
import time
import datetime
import random
import sys
import os
os.environ['PYGAME_HIDE_SUPPORT_PROMPT'] = '1'

pygame.init()
WW = 640
HH = 480
SF = 1
screen=pygame.display.set_mode([WW*SF,HH*SF])#,flags=pygame.FULLSCREEN)
pygame.display.set_caption("Img conv...")

def main():
    test = False
    if len(sys.argv)!=4:
        print("Invalid args...")
        #exit()
        gw = 8
        gh = 8
        fname = '../input/SD3Inner-Inn-05.png'
        test=True
    else:
        gw = int(sys.argv[1])
        gh = int(sys.argv[2])
        fname = sys.argv[3]
    #print(f'\nConverting "{fname}" (Cell size: {gw}x{gh})...')
    isurf = pygame.image.load(fname)
    imgbytes = pygame.image.tobytes(isurf,"RGB")

    hist = {}
    if test:
        for i in range(0,len(imgbytes),3):
            r = imgbytes[i + 0]
            g = imgbytes[i + 1]
            b = imgbytes[i + 2]            
            if (r,g,b) in hist:
                hist[(r,g,b)]+=1
            else:
                hist[(r,g,b)]=1
        for c in hist:
            print(f'{str(c):>16s}: {hist[c]}')
        print(f'{len(hist.keys())} colors.')
        exit()

    topleft = (0, 0)
    glyphrect = (0,0,gw,gh)
    glyphdims = (gw,gh)

    truth_surf = pygame.Surface(glyphdims,pygame.SRCALPHA) 
    truthname = f'temp/{os.path.basename(fname)[0:-4]}_{gw}x{gh}_truth_bytes.bin'
    #print(f'Writing {truthname}...')
    with open(truthname,'wb') as f:
        for py in range(0,480,gh): # cell pixel y  ---------------
            for px in range(0,640,gw):   # cell pixel x   ----------------
                truth_surf.blit(isurf,topleft,(px,py,gw,gh))
                truth_bytes = pygame.image.tobytes(truth_surf,"RGB")
                f.write(truth_bytes)

if __name__ == "__main__":
    main()



