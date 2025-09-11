; -----------------------------------------------------------------------------
; IO DEVICE BASE ADDRESSES
; -----------------------------------------------------------------------------

BANK_BASE   EQU  $FFEC      : FFEC - FFEF: MEMORY BANK REGS 0...3 (BUILT-IN)
ACIA_BASE   EQU  $FFE8      ; FFE8 - FFEB: SERIAL UART R65C51P2 (BUILT-IN)
VDP_BASE    EQU  $FFE4      ; FFE4 - FFE7: VIDEO CHIP V9958
OPL3_BASE   EQU  $FFE0      ; FFE0 - FFE3: MUSIC CHIP YMF262 (OPL3)
BREAK_BASE  EQU  $FFD0      ; FFD0 - FFDF: BUS BREAKOUT CARD 
VIA_BASE    EQU  $FFB0      ; FFB0 - FFBF: W65C22 VIA (SD CARD, SPI, KB, GPIO)

; V9958 VDP REGISTERS
VDAT        EQU  VDP_BASE+0 ; PORT 0: VRAM DATA (R/W)
VREG        EQU  VDP_BASE+1 ; PORT 1: STATUS REG (R), REGISTER/SETUP (W), VRAM ADDR (W)
VPAL        EQU  VDP_BASE+2 ; PORT 2: PALETTE REGISTERS (W)
VIND        EQU  VDP_BASE+3 ; PORT 3: REGISTER-INDIRECT ADDRESSING (W)


PICO_R0     EQU  BREAK_BASE+0
PICO_R1     EQU  BREAK_BASE+1

; MEMORY BANK REGISTERS 0..3 (BASE ADDRESS IN DEFINES.D)
; THESE ARE WRITE-ONLY, SO WE KEEP COPIES OF 1 THROUGH 3 IN 
; VARS IN MAIN.ASM: SBANK_1...SBANK_3

MBANK_0     EQU  BANK_BASE+0
MBANK_1     EQU  BANK_BASE+1
MBANK_2     EQU  BANK_BASE+2
MBANK_3     EQU  BANK_BASE+3


;------------------------------------------------------------------------------
; PugVDP constants
;------------------------------------------------------------------------------

FONT_8X16_VGA              EQU      0
FONT_8X16_SEMIGRAPHICS     EQU      1

FONT_8X8_CGA               EQU      16
FONT_8X8_SEMIGRAPHICS      EQU      17

FONT_8X4_SEMIGRAPHICS      EQU      32

;------------------------------------------------------------------------------
; PUGVDP COMMAND REGISTERS
;------------------------------------------------------------------------------

MODE                  EQU $00  ;    For changing video mode (80-cols default)
                                    ;    The avail modes are shown in the enum above
;    --- Text Commands ---

TEXT_RESET            EQU $10  ;    Resets active text area to whole screen,
                                    ;    clears it to current color, unhides cursor,
                                    ;    and sends it home.
TEXT_COLOR_SET        EQU $11  ;    (for putc, printc, define_area, clear, etc)
                                    ;    arg0: BG (RGB222),
                                    ;    arg1: FG (RGB222)                              
TEXT_SET_TAB          EQU $12  ;    arg0: uint8 spaces per tab
TEXT_SET_CURSOR_SIZE  EQU $13  ;    arg0: high/low nybs.: start/end pix rows
TEXT_CURSOR_SET_VIS   EQU $14  ;    arg0: 1: visible, 0:invisible
TEXT_CURSOR_SET_BLINK EQU $15  ;    arg0: num frames per cursor blink toggle
                               ;          0: no blink!
TEXT_CURSOR_RESET     EQU $16  ;    Reset cursor attributes to defaults
TEXT_AREA_DEFINE      EQU $17  ;    Sets active area for printing and scrolling
                                    ;    arg0: screen x (0..39/79),
                                    ;    arg1: screen y (0..14/29),
                                    ;    arg2: area width,
                                    ;    arg3: area height
TEXT_CLEAR            EQU $18  ;    clears active area to current color
TEXT_HOME             EQU $19  ;    same as TEXT_GOTOXY(0,0)
TEXT_GOTOXY           EQU $1a  ;    Position cursor within active area.
                                    ;    arg0: curs x,
                                    ;    arg1: curs y
TEXT_PUTC             EQU $1b  ;    Each arg byte is written to active area,
                                    ;    advancing cursor, with wraparound to top.
                                    ;    (No control codes are handled.)
TEXT_PRINTC           EQU $1c  ;    Like PUTC except will handle some of the
                                    ;    ASCII control codes, and content will
                                    ;    scroll up past area bottom.
                                    ;    (Note an LF is treated as LF + CR.)
TEXT_SCROLL_H         EQU $1d  ;    arg0: int8 num area cols (positive: left)
TEXT_SCROLL_V         EQU $1e  ;    arg0: int8 num area lines (positive: up)
TEXT_FONT_RESET       EQU $1f  ;    resets current font to built-in 8x16 font
TEXT_FONT_SET         EQU $20  ;    allows font to be reprogrammed:
                                    ;    arg0: starting character code X (0..255),
                                    ;    args: glyph X,row 0 ... glyph X,row 15,
                                    ;    glyph X+1,row 0... (Wraps after glyph 255)
TEXT_TEST             EQU $21  ;    fills active area with colorful text
;     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
;     !!! THE REST ARENT YET IMPLEMENTED !!!
;     !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

;    --- Palette Commands ---

PALETTE_RESET         EQU $30  ;    Resets both 256-color palettes to default
                                    ;    palette 0 is for bitmaps and tilemaps, and
                                    ;    palette 1 is for sprites.
PALETTE_SET           EQU $31  ;    arg0:   Palette number [0,1]:
                                    ;              0: bitmap/tilemap palette[256]
                                    ;              1: sprite palette[256]
                                    ;    arg1:   Starting color index N (All colors are RGBA5551)
                                    ;    arg2+:  Color N,hi byte, Color N,low byte,
                                    ;            Color N+1,hi byte, Color N+1,low byte.
                                    ;            ... (wraps around to 0 after 255)
;    --- Bitmap graphics Commands ---

;    For these, "color id" means different things depending on graphics mode:
;    (either a palette index, a uint8 rgba2221, rgba2321, 
;    rgba5551 [hi byte followed by lo byte])

BMP_CLEAR             EQU $40
BMP_SET               EQU $41  ;    bmp adrs hi, bmp adrs low, bmp byte0, bmp byte1 ...
BMP_MOVETO            EQU $42  ;    sets draw position. x hi, x lo, y hi, y lo
BMP_PLOT              EQU $43  ;    arg0: fg color id0, arg1: fg color id 1, ...
                                    ;    advances draw position to right with each pixel,
                                    ;    with wraparound at right and bottom
BMP_FLOODFILL         EQU $44  ;    arg0: fill tolerance (0:exact color, 255: vaguely same color)
BMP_COLOR_SET         EQU $45  ;    sets draw/fill color id
                                    ;    arg0: bg color id, arg1: fg color id
BMP_DRAWTO            EQU $46  ;    draw line from orig drawing pos to new pos.
                                    ;    arg0: x hi, arg1: x lo, arg2: y hi, arg3: y lo
                                    ;    ...
BMP_DRAW_RECT         EQU $47  ;    x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo
BMP_FILL_RECT         EQU $48  ;    x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo
BMP_DRAW_CIRC         EQU $49  ;    x hi, x lo, y hi, y lo, rad hi, rad lo
BMP_FILL_CIRC         EQU $4a  ;    x hi, x lo, y hi, y lo, rad hi, rad lo

;    "Degrees" arguments to ARC-drawing functions below
;    are int16 fixed-point integers int(degrees*50)

BMP_DRAW_ARC          EQU $4b  ;    x hi, x lo, y hi, y lo, rad hi, rad lo, deg0 hi, deg0 lo, deg1 hi, deg1 lo
BMP_FILL_ARC          EQU $4c  ;    x hi, x lo, y hi, y lo, rad hi, rad lo, deg0 hi, deg0 lo, deg1 hi, deg1 lo
BMP_DRAW_TEXT         EQU $4d  ;    arg0: obey_alpha (0: no, 1:yes),
                                    ;    arg1...: glyph index(s) to write
BMP_BLIT              EQU $4e  ;    copy screen rect to a specified dest pos.
                                    ;    x hi, x lo, y hi, y lo, w hi, w lo, h hi, h lo,
                                    ;    x1 hi, x1 lo, y1 hi, y1 lo,
                                    ;    obey_alpha (0: no, 1:yes)

;    !!! TODO: Tilemap commands! !!!
;------------------------------------------------------------------------------
; COMMON DEFINITIONS
;------------------------------------------------------------------------------
; ASCII CODES
ESC         EQU  $1B    ; ESCAPE
BS          EQU  $08
LF          EQU  $0A    ; LINE FEED
CR          EQU  $0D    ; CARRIAGE RETURN
NUL         EQU  $00    ; NULL
SPACE       EQU  $20
TILDE       EQU  $7E
;------------------------------------------------------------------------------
_DATE       STRUCT
YEAR        RMB  2      ; DATE AS INTEGERS
DAYYR       RMB  2
MON         RMB  1
DAYMON      RMB  1
HOUR        RMB  1
MIN         RMB  1
SEC         RMB  1
HUND        RMB  1
YEARBCD     RMB  2      ; DATE AS BCD
DAYYRBCD    RMB  2
MONBCD      RMB  1
DAYMONBCD   RMB  1
HOURBCD     RMB  1
MINBCD      RMB  1
SECBCD      RMB  1
HUNDBCD     RMB  1
            ENDS
;------------------------------------------------------------------------------
; EOF
;------------------------------------------------------------------------------
