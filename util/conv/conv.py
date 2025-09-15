import pygame

# takes in a font image and a palette image and outputs them as 
# C source code for inclusion in PugVDP.

assets_path = 'assets/'


# infile name, out_suffix, glyph wid, glyph hei, draw x, draw y
fonts = [
	('IBM_VGA_8x16_Font_PS2_Model_30.png',	'vga', 	8,16,0,0),
	('Pug_Font_8x16.png',										'semi',	8,16,128+8,0),
	('ibm_cga_8x8.png',											'cga',	8,8,0,256+8),
	('Pug_Font_8x8.png',										'semi',	8,8,128+8,256+8),
	('Pug_Font_8x4.png',										'semi',	8,4,0,256+128+16),  
	('8x1.png',															'semi',	8,1,128+8,256+128+16)  

]

# 256-color Palette from the game "Sonic Robo Blast 2"
# (Duplicated from both the background and sprite default palettes)

palette_filename = "sonic_pug_palette_512.png"

pygame.init()
WW = 640
HH = 480
SF = 1
screen=pygame.display.set_mode([WW*SF,HH*SF])#,flags=pygame.FULLSCREEN)
pygame.display.set_caption("Images converted. Close window to quit.")

# load the palette image and do the same
# palette 0, color 0 is top left, and again things
# ascend right to left, top to bottom, spanning two palettes of 256
# colors each. The first one is for bitmap modes and tiled backgrounds,
# and the second ne if for sprites.

palsurf = pygame.image.load(assets_path+palette_filename)
palbytes = pygame.image.tobytes(palsurf,"RGB")



# -----------------------------------------------------------------------------
# Output font as an array of uint8_t.
# (does row 0 of all chars, then row 1 of all chars, thru all row 15's)
# -----------------------------------------------------------------------------
def do_font(font_filename,out_suffix, gw,gh,dx,dy):
	# load input image and convert to an array of bytes r0,g0,b0,r1,g1,b1...
	# for this vga font, the image is 16x16 characters, each being 8x16 
	# pixels in size. The ascii codes ascend from left->right, top->bottom.

	isurf = pygame.image.load(font_filename)
	imgbytes = pygame.image.tobytes(isurf,"RGB")

	# display font image
	screen.blit(isurf,(dx,dy))
	pygame.display.update()

	print(f'''
// Note, the data contains 256 monochrome, {gw}x{gh} {out_suffix} font characters.

const char __in_flash() font_{gw}x{gh}_{out_suffix}[] = '''+'{')
	j=0
	print('  ',end='')
	for row in range(0,gh):
		for char in range(0,256):
			px = (char%16)*gw
			py = (char//16)*gh
			cb = 0
			for x in range(gw):
				p = imgbytes[((px+7-x)+(py+row)*gw*16)*3]
				cb = (cb << 1) | (p & 1)
			print(f'0x{cb:02x},',end='')
			j+=1
			if j==32:
				j=0
				print('\n  ',end='')
	print('};')

# -----------------------------------------------------------------------------
# output the palette as an array of uint16_t
# -----------------------------------------------------------------------------
def do_palette():
	s = """

// Power-on default palette (see conf.py, sonic_pug_palette_512.png)

static const uint16_t default_palette[512] = {
""";
	for y in range(0,32):
		s+='  '
		for x in range(0,16):
			r,g,b = palbytes[x*3+(y*16*3):x*3+(y*16*3)+3]
			qr = r*31//255
			qg = g*31//255
			qb = b*31//255
			q = (qr<<10)|(qg<<5)|qb
			s+=f'0x{q:04x},'  # Color output format is RGB555
			pygame.draw.rect(screen, (r,g,b),(256+16+x*8,y*8,8,8))
		s+='\n'
	s+='};\n'
	pygame.display.update()
	print(s)


print('\n  ################################################# ')
print('  #### Paste this code into src/pugvdp/fonts.h #### ')
print('  ################################################# ')

for font in fonts:
	do_font(assets_path+font[0], font[1],font[2],font[3], font[4], font[5])

print('  ################################################### ')
print('  #### Paste this code into src/pugvdp/defines.h #### ')
print('  ################################################### \n')	

do_palette()

###############################################################################
# wait for user quit
###############################################################################
while True:
  for event in pygame.event.get():
    if event.type == pygame.QUIT:
      exit()

