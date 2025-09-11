DEL *.S19
DEL *.HEX
DEL *.LST
DEL *.MAP
DEL *.O
DEL *.BIN
DEL *.ZX0

..\..\..\BIN\LWASM.EXE VDP_TEXT.ASM --format=srec --output=VDP_TEXT.S19 --list=VDP_TEXT.LST
..\..\..\BIN\LWASM.EXE PICO_TEST.ASM --format=srec --output=PICO_TEST.S19 --list=PICO_TEST.LST
