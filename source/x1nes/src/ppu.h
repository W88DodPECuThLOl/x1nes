#ifndef INCL_ppu_h
#define INCL_ppu_h

#include "dev/BasicTypes.h"

typedef u16 PixelFormat;

#define VISIBLE_SCANLINES 240
#define VISIBLE_DOTS 256
#define NTSC_SCANLINES_PER_FRAME 261
#define PAL_SCANLINES_PER_FRAME 311
#define DOTS_PER_SCANLINE 341
#define END_DOT 340

enum {
    BG_TABLE        = 1 << 4,
    SPRITE_TABLE    = 1 << 3,
    SHOW_BG_8       = 1 << 1,
    SHOW_SPRITE_8   = 1 << 2,
    SHOW_BG         = 1 << 3,
    SHOW_SPRITE     = 1 << 4,
    LONG_SPRITE     = 1 << 5,
    SPRITE_0_HIT    = 1 << 6,
    FLIP_HORIZONTAL = 1 << 6,
    FLIP_VERTICAL   = 1 << 7,
    V_BLANK         = 1 << 7,
    GENERATE_NMI    = 1 << 7,
    RENDER_ENABLED  = 0x18,
    BASE_NAMETABLE  = 0x3,
    FINE_Y          = 0x7000,
    COARSE_Y        = 0x3E0,
    COARSE_X        = 0x1F,
    VERTICAL_BITS   = 0x7BE0,
    HORIZONTAL_BITS = 0x41F,
    X_SCROLL_BITS   = 0x1f,
    Y_SCROLL_BITS   = 0x73E0
};

typedef struct PPU {
//    u8 frames;
//    PixelFormat *lineBuffer;
//    u8 V_RAM[0x1000];
//    u8 OAM[256];
//    u8 OAM_cache[8];
//    u8 palette[0x20];
//    u8 OAM_cache_len;
//    u8 ctrl;
//    u8 mask;
//    u8 status;
//    u16 dots;
//    u16 scanlines;
//    u16 scanlines_per_frame;

//    u16 v;
//    u16 t;
//    u8 x;
//    u8 w;
//    u8 oam_address;
//    u8 buffer;

//    u8 render;
//    u8 bus;

//    struct Emulator* emulator;
//    struct Mapper* mapper;
u8 dummy;
} PPU;

extern u8 ppu_mask;
extern u8 ppu_bus;
extern u8 ppu_render; 

// ARGB8888 palette
/*
static const uint32_t nes_palette_raw[64] = {
    0xff666666, 0xff002a88, 0xff1412a7, 0xff3b00a4, 0xff5c007e, 0xff6e0040, 0xff6c0600, 0xff561d00,
    0xff333500, 0xff0b4800, 0xff005200, 0xff004f08, 0xff00404d, 0xff000000, 0xff000000, 0xff000000,
    0xffadadad, 0xff155fd9, 0xff4240ff, 0xff7527fe, 0xffa01acc, 0xffb71e7b, 0xffb53120, 0xff994e00,
    0xff6b6d00, 0xff388700, 0xff0c9300, 0xff008f32, 0xff007c8d, 0xff000000, 0xff000000, 0xff000000,
    0xfffffeff, 0xff64b0ff, 0xff9290ff, 0xffc676ff, 0xfff36aff, 0xfffe6ecc, 0xfffe8170, 0xffea9e22,
    0xffbcbe00, 0xff88d800, 0xff5ce430, 0xff45e082, 0xff48cdde, 0xff4f4f4f, 0xff000000, 0xff000000,
    0xfffffeff, 0xffc0dfff, 0xffd3d2ff, 0xffe8c8ff, 0xfffbc2ff, 0xfffec4ea, 0xfffeccc5, 0xfff7d8a5,
    0xffe4e594, 0xffcfef96, 0xffbdf4ab, 0xffb3f3cc, 0xffb5ebf2, 0xffb8b8b8, 0xff000000, 0xff000000,
};
*/

// extern uint32_t nes_palette[64];

void execute_ppu();
void reset_ppu();
void exit_ppu();
void init_ppu();
u8 read_status();
u8 read_ppu();
void set_ctrl(u8 ctrl);
void write_ppu(u8 value);
void dma(u8 value);
void set_scroll(u8 coord);
void set_address(u8 address);
void set_oam_address(u8 address);
u8 read_oam();
void write_oam(u8 value);
u8 read_vram(u16 address);
void write_vram(u16 address, u8 value);

#endif // INCL_ppu_h
