#include "emulator.h"
#include "ppu.h"
#include "mmu.h"

#include <string.h>
//#include <stdlib.h>
//#include "utils.h"
//#include "cpu6502.h"

extern Emulator gEmulator;

static u16 render_background();
static u16 render_sprites(u16 bg_addr, u8* back_priority);
static void update_NMI();

void x1_pset(u8 x, u8 y, u8 paletteIndex);

#define LINE_BUFFER_SIZE (sizeof(PixelFormat) * VISIBLE_DOTS)

#if 0
#define TO_PIXEL_FORMAT(ARGB8888) \
     (PixelFormat)((((u32)ARGB8888 & (u32)0x0000F0) >> 4) \
    | (((u32)ARGB8888 & (u32)0x00F000) >> 12) \
    | (((u32)ARGB8888 & (u32)0xF00000) >> 20))

//static PixelFormat lineBuffer[VISIBLE_DOTS];
static PixelFormat nes_palette[64] = {
    TO_PIXEL_FORMAT(0xff666666), TO_PIXEL_FORMAT(0xff002a88), TO_PIXEL_FORMAT(0xff1412a7), TO_PIXEL_FORMAT(0xff3b00a4), TO_PIXEL_FORMAT(0xff5c007e), TO_PIXEL_FORMAT(0xff6e0040), TO_PIXEL_FORMAT(0xff6c0600), TO_PIXEL_FORMAT(0xff561d00),
    TO_PIXEL_FORMAT(0xff333500), TO_PIXEL_FORMAT(0xff0b4800), TO_PIXEL_FORMAT(0xff005200), TO_PIXEL_FORMAT(0xff004f08), TO_PIXEL_FORMAT(0xff00404d), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000),
    TO_PIXEL_FORMAT(0xffadadad), TO_PIXEL_FORMAT(0xff155fd9), TO_PIXEL_FORMAT(0xff4240ff), TO_PIXEL_FORMAT(0xff7527fe), TO_PIXEL_FORMAT(0xffa01acc), TO_PIXEL_FORMAT(0xffb71e7b), TO_PIXEL_FORMAT(0xffb53120), TO_PIXEL_FORMAT(0xff994e00),
    TO_PIXEL_FORMAT(0xff6b6d00), TO_PIXEL_FORMAT(0xff388700), TO_PIXEL_FORMAT(0xff0c9300), TO_PIXEL_FORMAT(0xff008f32), TO_PIXEL_FORMAT(0xff007c8d), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000),
    TO_PIXEL_FORMAT(0xfffffeff), TO_PIXEL_FORMAT(0xff64b0ff), TO_PIXEL_FORMAT(0xff9290ff), TO_PIXEL_FORMAT(0xffc676ff), TO_PIXEL_FORMAT(0xfff36aff), TO_PIXEL_FORMAT(0xfffe6ecc), TO_PIXEL_FORMAT(0xfffe8170), TO_PIXEL_FORMAT(0xffea9e22),
    TO_PIXEL_FORMAT(0xffbcbe00), TO_PIXEL_FORMAT(0xff88d800), TO_PIXEL_FORMAT(0xff5ce430), TO_PIXEL_FORMAT(0xff45e082), TO_PIXEL_FORMAT(0xff48cdde), TO_PIXEL_FORMAT(0xff4f4f4f), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000),
    TO_PIXEL_FORMAT(0xfffffeff), TO_PIXEL_FORMAT(0xffc0dfff), TO_PIXEL_FORMAT(0xffd3d2ff), TO_PIXEL_FORMAT(0xffe8c8ff), TO_PIXEL_FORMAT(0xfffbc2ff), TO_PIXEL_FORMAT(0xfffec4ea), TO_PIXEL_FORMAT(0xfffeccc5), TO_PIXEL_FORMAT(0xfff7d8a5),
    TO_PIXEL_FORMAT(0xffe4e594), TO_PIXEL_FORMAT(0xffcfef96), TO_PIXEL_FORMAT(0xffbdf4ab), TO_PIXEL_FORMAT(0xffb3f3cc), TO_PIXEL_FORMAT(0xffb5ebf2), TO_PIXEL_FORMAT(0xffb8b8b8), TO_PIXEL_FORMAT(0xff000000), TO_PIXEL_FORMAT(0xff000000),
};
#endif

static u8  ppu_frames;
static u8  ppu_V_RAM[0x1000];
static u8  ppu_OAM[256];
static u8  ppu_OAM_cache[8];
static u8  ppu_palette[0x20];
static u8  ppu_OAM_cache_len;
static u8  ppu_ctrl;
static u8  ppu_status;
static u16 ppu_dots;
static u16 ppu_scanlines;
static u16 ppu_scanlines_per_frame;
static u16 ppu_v;
static u16 ppu_t;
static u8  ppu_x;
static u8  ppu_w;
static u8  ppu_oam_address;
static u8  ppu_buffer;
       u8  ppu_mask;
       u8  ppu_bus;
       u8  ppu_render;

void init_ppu(){
//    to_pixel_format(nes_palette_raw, nes_palette, 64, ABGR8888);
//    PPU* ppu = &gEmulator.ppu;
//    ppu->mapper = &gEmulator.mapper;
    ppu_scanlines_per_frame = gEmulator.type == NTSC ? NTSC_SCANLINES_PER_FRAME : PAL_SCANLINES_PER_FRAME;

    memset(ppu_palette, 0, sizeof(ppu_palette));
    memset(ppu_OAM_cache, 0, sizeof(ppu_OAM_cache));
    memset(ppu_V_RAM, 0, sizeof(ppu_V_RAM));
    memset(ppu_OAM, 0, sizeof(ppu_OAM));
    ppu_oam_address = 0;
    ppu_v = 0;
    reset_ppu();
}

void reset_ppu(){
    ppu_t = ppu_x = ppu_dots = 0;
    ppu_scanlines = 261;
    ppu_w = 1;
    ppu_ctrl &= ~0xFC;
    ppu_mask = 0;
    ppu_status = 0;
    ppu_frames = 0;
    ppu_OAM_cache_len = 0;
    memset(ppu_OAM_cache, 0, 8);
}

void exit_ppu() {
}

void set_address(u8 address){
    if(ppu_w){
        // first write
        ppu_t &= 0xff;
        ppu_t |= (address & 0x3f) << 8; // store only upto bit 14
        ppu_w = 0;
    }else{
        // second write
        ppu_t &= 0xff00;
        ppu_t |= address;
        ppu_v = ppu_t;
        ppu_w = 1;
    }
}


void set_oam_address(u8 address){
    ppu_oam_address = address;
}

u8 read_oam(){
    return ppu_OAM[ppu_oam_address];
}

void write_oam(u8 value){
    ppu_OAM[ppu_oam_address++] = value;
}

void set_scroll(u8 coord){
    if(ppu_w){
        // first write
        ppu_t &= ~X_SCROLL_BITS;
        ppu_t |= (coord >> 3) & X_SCROLL_BITS;
        ppu_x = coord & 0x7;
        ppu_w = 0;
    }else{
        // second write
        ppu_t &= ~Y_SCROLL_BITS;
        ppu_t |= ((coord & 0x7) << 12) | ((coord & 0xF8) << 2);
        ppu_w = 1;
    }
}

u8 read_ppu(){
    u8 prev_buff = ppu_buffer, data;
    ppu_buffer = read_vram(ppu_v);

    if(ppu_v >= 0x3F00) {
        data = ppu_buffer;
        // read underlying nametable mirrors into buffer
        // 0x3f00 - 0x3fff maps to 0x2f00 - 0x2fff
        ppu_buffer = read_vram(ppu_v & 0xefff);
    }else
        data = prev_buff;
    ppu_v += ((ppu_ctrl & (1<<2)) ? 32 : 1);
    return data;
}

void write_ppu(u8 value){
    write_vram(ppu_v, value);
    ppu_v += ((ppu_ctrl & (1<<2)) ? 32 : 1);
}

void dma(u8 address){
    u8* ptr = get_ptr(address * 0x100);
    // halt CPU for DMA and skip extra cycle if on odd cycle
    do_DMA(513 + gEmulator.cpu.odd_cycle);
    if(ptr == nullptr) {
        // Probably in PRG ROM so it is not possible to resolve a pointer
        // due to bank switching, so we do it the slow hard way
        for(int i = 0; i < 256; i++) {
            ppu_OAM[(ppu_oam_address + i) & 0xff] = read_mem(address * 0x100 + i);
        }
    }else {
        // copy from OAM address to the end (256 bytes)
        memcpy(ppu_OAM + ppu_oam_address, ptr, 256 - ppu_oam_address);
        if(ppu_oam_address) {
            // wrap around and copy from start to OAM address if OAM is not 0x00
            memcpy(ppu_OAM, ptr + (256 - ppu_oam_address), ppu_oam_address);
        }
        // last value
        gEmulator.mem.bus = ptr[255];
    }
}



u8 read_vram(/* PPU* ppu, */ u16 address){
    address = address & 0x3fff;

    if(address < 0x2000) {
        ppu_bus = gEmulator.mapper.read_CHR(address);
        return ppu_bus;
    }

    if(address < 0x3F00){
        address = (address & 0xefff) - 0x2000;
        ppu_bus = ppu_V_RAM[gEmulator.mapper.name_table_map[address / 0x400] + (address & 0x3ff)];
        return ppu_bus;
    }

    // palette RAM provide first 6 bits and remaining 2 bits are open bus
    return ppu_palette[(address - 0x3F00) % 0x20] & 0x3f | (ppu_bus & 0xc0);
}

void write_vram(u16 address, u8 value){
    address = address & 0x3fff;
    ppu_bus = value;

    if(address < 0x2000)
        gEmulator.mapper.write_CHR(address, value);
    else if(address < 0x3F00){
        address = (address & 0xefff) - 0x2000;
        ppu_V_RAM[gEmulator.mapper.name_table_map[address / 0x400] + (address & 0x3ff)] = value;
    }

    else if(address < 0x4000) {
        address = (address - 0x3F00) % 0x20;
        if(address % 4 == 0) {
            ppu_palette[address] = value;
            ppu_palette[address ^ 0x10] = value;
        }
        else
            ppu_palette[address] = value;
    }

}

u8 read_status(){
    u8 status = ppu_status;
    ppu_w = 1;
    ppu_status &= ~(1<<7); // reset v_blank
    update_NMI();
    return status;
}

void set_ctrl(u8 ctrl){
    ppu_ctrl = ctrl;
    update_NMI();
    // set name table in temp address
    ppu_t &= ~0xc00;
    ppu_t |= (ctrl & BASE_NAMETABLE) << 10;
}

static void update_NMI() {
    if(ppu_ctrl & (1<<7) && ppu_status & (1<<7))
        interrupt(NMI);
    else
        interrupt_clear(NMI);
}

void execute_ppu(){
    if(ppu_scanlines < VISIBLE_SCANLINES){
        // render scanlines 0 - 239
        if(ppu_dots > 0 && ppu_dots <= VISIBLE_DOTS){
            int x = (int)ppu_dots - 1;
            u8 fine_x = ((u16)ppu_x + x) % 8, palette_addr = 0, palette_addr_sp = 0, back_priority = 0;

            if(ppu_mask & SHOW_BG){
                palette_addr = render_background();
                if(fine_x == 7) {
                    if ((ppu_v & COARSE_X) == 31) {
                        ppu_v &= ~COARSE_X;
                        // switch horizontal nametable
                        ppu_v ^= 0x400;
                    }
                    else
                        ppu_v++;
                }
            }
            if(ppu_mask & SHOW_SPRITE && ((ppu_mask & SHOW_SPRITE_8) || x >=8)){
                palette_addr_sp = render_sprites(palette_addr, &back_priority);
            }
            if((!palette_addr && palette_addr_sp) || (palette_addr && palette_addr_sp && !back_priority))
                palette_addr = palette_addr_sp;

            palette_addr = ppu_palette[palette_addr];
            //ppu->screen[ppu->scanlines * VISIBLE_DOTS + ppu->dots - 1] = nes_palette[palette_addr];
#if 1
            x1_pset(ppu_dots - 1, ppu_scanlines, palette_addr);
#endif
        }
        if(ppu_dots == VISIBLE_DOTS + 1 && ppu_mask & SHOW_BG){
            if((ppu_v & FINE_Y) != FINE_Y) {
                // increment coarse x
                ppu_v += 0x1000;
            }
            else{
                ppu_v &= ~FINE_Y;
                u16 coarse_y = (ppu_v & COARSE_Y) >> 5;
                if(coarse_y == 29){
                    coarse_y = 0;
                    // toggle bit 11 to switch vertical nametable
                    ppu_v ^= 0x800;
                }
                else if(coarse_y == 31){
                    // nametable not switched
                    coarse_y = 0;
                }
                else{
                    coarse_y++;
                }

                ppu_v = (ppu_v & ~COARSE_Y) | (coarse_y << 5);
            }
        }
        else if(ppu_dots == VISIBLE_DOTS + 2 && (ppu_mask & RENDER_ENABLED)){
            ppu_v &= ~HORIZONTAL_BITS;
            ppu_v |= ppu_t & HORIZONTAL_BITS;
        }
        else if(ppu_dots == VISIBLE_DOTS + 4 && ppu_mask & SHOW_SPRITE && ppu_mask & SHOW_BG) {
            gEmulator.mapper.on_scanline();
        }
        else if(ppu_dots == 320 && ppu_mask & RENDER_ENABLED){
            memset(ppu_OAM_cache, 0, 8);
            ppu_OAM_cache_len = 0;
            u8 range = ppu_ctrl & LONG_SPRITE ? 16: 8;
            for(u8 i = ppu_oam_address / 4; i < 64; i++){
                int diff = (int)ppu_scanlines - ppu_OAM[i * 4];
                if(diff >= 0 && diff < range){
                    ppu_OAM_cache[ppu_OAM_cache_len++] = i * 4;
                    if(ppu_OAM_cache_len >= 8)
                        break;
                }
            }
        }
    }
    else if(ppu_scanlines == VISIBLE_SCANLINES){
        // post render scanline 240/239
    }
    else if(ppu_scanlines < ppu_scanlines_per_frame){
        // v blanking scanlines 241 - 261/311
        if(ppu_dots == 1 && ppu_scanlines == VISIBLE_SCANLINES + 1){
            // set v-blank
            ppu_status |= V_BLANK;
            update_NMI();
        }
    }
    else{
        // pre-render scanline 262/312
        if(ppu_dots == 1){
            // reset v-blank and sprite zero hit
            ppu_status &= ~(V_BLANK | SPRITE_0_HIT);
            update_NMI();
        }
        else if(ppu_dots == VISIBLE_DOTS + 2 && (ppu_mask & RENDER_ENABLED)){
            ppu_v &= ~HORIZONTAL_BITS;
            ppu_v |= ppu_t & HORIZONTAL_BITS;
        }
        else if(ppu_dots == VISIBLE_DOTS + 4 && ppu_mask & SHOW_SPRITE && ppu_mask & SHOW_BG) {
            gEmulator.mapper.on_scanline();
        }
        else if(ppu_dots > 280 && ppu_dots <= 304 && (ppu_mask & RENDER_ENABLED)){
            ppu_v &= ~VERTICAL_BITS;
            ppu_v |= ppu_t & VERTICAL_BITS;
        }
        else if((ppu_dots == END_DOT - 1) && (ppu_frames & 1) && (ppu_mask & RENDER_ENABLED)) {
            // skip one cycle on odd frames if rendering is enabled for NTSC
            ppu_dots++;
        }

        if(ppu_dots >= END_DOT) {
            // inform emulator to render contents of ppu on first dot
            ppu_render = 1;
            ppu_frames++;
        }
    }

    // increment dots and scanlines

    if(++ppu_dots >= DOTS_PER_SCANLINE) {
        if (ppu_scanlines++ >= ppu_scanlines_per_frame)
            ppu_scanlines = 0;
        ppu_dots = 0;
    }
}


static u16 render_background(){
    int x = (int)ppu_dots - 1;

    if(!(ppu_mask & SHOW_BG_8) && x < 8)
        return 0;

    u8 fine_x = ((u16)ppu_x + x) % 8;

    u16 tile_addr = 0x2000 | (ppu_v & 0xFFF);

    u16 pattern_addr = (read_vram(/* ppu, */ tile_addr) * 16 + ((ppu_v >> 12) & 0x7)) | ((ppu_ctrl & BG_TABLE) << 8);

    u16 palette_addr = (read_vram(/* ppu, */ pattern_addr) >> (7 ^ fine_x)) & 1;
    palette_addr |= ((read_vram(/* ppu, */ pattern_addr + 8) >> (7 ^ fine_x)) & 1) << 1;

    if(!palette_addr)
        return 0;

    u16 attr_addr = 0x23C0 | (ppu_v & 0x0C00) | ((ppu_v >> 4) & 0x38) | ((ppu_v >> 2) & 0x07);
    u8 attr = read_vram(/* ppu, */ attr_addr);
    return palette_addr | (((attr >> ((ppu_v >> 4) & 4 | ppu_v & 2)) & 0x3) << 2);
}

static u16 render_sprites_hit_check(u8* restrict back_priority){
    // 4 bytes per sprite
    // byte 0 -> y index
    // byte 1 -> tile index
    // byte 2 -> render info
    // byte 3 -> x index
    int x = (int)ppu_dots - 1, y = (int)ppu_scanlines;
    u16 palette_addr = 0;
    u8 length = ppu_ctrl & LONG_SPRITE ? 16: 8;
    for(u8 j = 0; j < ppu_OAM_cache_len; j++) {
        u8 i = ppu_OAM_cache[j];
        u8 tile_x = ppu_OAM[i + 3];

        if (x - tile_x < 0 || x - tile_x >= 8)
            continue;

        u16 tile = ppu_OAM[i + 1];
        u8 tile_y = ppu_OAM[i] + 1;
        u8 attr = ppu_OAM[i + 2];
        int x_off = (x - tile_x) % 8, y_off = (y - tile_y) % length;

        if (!(attr & FLIP_HORIZONTAL))
            x_off ^= 7;
        if (attr & FLIP_VERTICAL)
            y_off ^= (length - 1);

        u16 tile_addr;

        if (ppu_ctrl & LONG_SPRITE) {
            y_off = y_off & 7 | ((y_off & 8) << 1);
            tile_addr = (tile >> 1) * 32 + y_off;
            tile_addr |= (tile & 1) << 12;
        } else {
            tile_addr = tile * 16 + y_off + (ppu_ctrl & SPRITE_TABLE ? 0x1000 : 0);
        }

        palette_addr = (read_vram(/* ppu, */ tile_addr) >> x_off) & 1;
        palette_addr |= ((read_vram(/* ppu, */ tile_addr + 8) >> x_off) & 1) << 1;

        if (!palette_addr)
            continue;

        palette_addr |= 0x10 | ((attr & 0x3) << 2);
        *back_priority = attr & (1<<5);

        // sprite hit evaluation
#if 0
        if (!(ppu->status & SPRITE_0_HIT)
            && (ppu_mask & SHOW_BG)
            && i == 0
            && palette_addr
            && bg_addr
            && x < 255)
            ppu->status |= SPRITE_0_HIT;
#else
        if (i == 0
            && palette_addr
            && x < 255)
            ppu_status |= SPRITE_0_HIT;
#endif
        break;
    }
    return palette_addr;
}

static u16 render_sprites_no_hit_check(u8* restrict back_priority){
    // 4 bytes per sprite
    // byte 0 -> y index
    // byte 1 -> tile index
    // byte 2 -> render info
    // byte 3 -> x index
    int x = (int)ppu_dots - 1, y = (int)ppu_scanlines;
    u16 palette_addr = 0;
    u8 length = ppu_ctrl & LONG_SPRITE ? 16: 8;
    for(u8 j = 0; j < ppu_OAM_cache_len; j++) {
        u8 i = ppu_OAM_cache[j];
        u8 tile_x = ppu_OAM[i + 3];

        if (x - tile_x < 0 || x - tile_x >= 8)
            continue;

        u16 tile = ppu_OAM[i + 1];
        u8 tile_y = ppu_OAM[i] + 1;
        u8 attr = ppu_OAM[i + 2];
        int x_off = (x - tile_x) % 8, y_off = (y - tile_y) % length;

        if (!(attr & FLIP_HORIZONTAL))
            x_off ^= 7;
        if (attr & FLIP_VERTICAL)
            y_off ^= (length - 1);

        u16 tile_addr;

        if (ppu_ctrl & LONG_SPRITE) {
            y_off = y_off & 7 | ((y_off & 8) << 1);
            tile_addr = (tile >> 1) * 32 + y_off;
            tile_addr |= (tile & 1) << 12;
        } else {
            tile_addr = tile * 16 + y_off + (ppu_ctrl & SPRITE_TABLE ? 0x1000 : 0);
        }

        palette_addr = (read_vram(/* ppu, */ tile_addr) >> x_off) & 1;
        palette_addr |= ((read_vram(/* ppu, */ tile_addr + 8) >> x_off) & 1) << 1;

        if (!palette_addr)
            continue;

        palette_addr |= 0x10 | ((attr & 0x3) << 2);
        *back_priority = attr & (1<<5);

#if 0
        // sprite hit evaluation
        if (!(ppu->status & SPRITE_0_HIT)
            && (ppu_mask & SHOW_BG)
            && i == 0
            && palette_addr
            && bg_addr
            && x < 255)
            ppu->status |= SPRITE_0_HIT;
#endif
        break;
    }
    return palette_addr;
}

static u16
render_sprites(u16 bg_addr, u8* restrict back_priority)
{
    if (!(ppu_status & SPRITE_0_HIT)
        && (ppu_mask & SHOW_BG)
        && bg_addr) {
        // sprite hit処理する
        return render_sprites_hit_check(back_priority);
    } else {
        // sprite hit処理しない
        return render_sprites_no_hit_check(back_priority);
    }
}

#if 1

#include "dev/BasicTypes.h"

static const u16 lineAddr[200] = {
0x0000, // 0
0x0800, // 1
0x1000, // 2
0x1800, // 3
0x2000, // 4
0x2800, // 5
0x3000, // 6
0x3800, // 7

0x0028, // 8
0x0828, // 9
0x1028, // 10
0x1828, // 11
0x2028, // 12
0x2828, // 13
0x3028, // 14
0x3828, // 15

0x0050, // 16
0x0850, // 17
0x1050, // 18
0x1850, // 19
0x2050, // 20
0x2850, // 21
0x3050, // 22
0x3850, // 23

0x0078, // 24
0x0878, // 25
0x1078, // 26
0x1878, // 27
0x2078, // 28
0x2878, // 29
0x3078, // 30
0x3878, // 31
0x00a0, // 32
0x08a0, // 33
0x10a0, // 34
0x18a0, // 35
0x20a0, // 36
0x28a0, // 37
0x30a0, // 38
0x38a0, // 39
0x00c8, // 40
0x08c8, // 41
0x10c8, // 42
0x18c8, // 43
0x20c8, // 44
0x28c8, // 45
0x30c8, // 46
0x38c8, // 47
0x00f0, // 48
0x08f0, // 49
0x10f0, // 50
0x18f0, // 51
0x20f0, // 52
0x28f0, // 53
0x30f0, // 54
0x38f0, // 55
0x0118, // 56
0x0918, // 57
0x1118, // 58
0x1918, // 59
0x2118, // 60
0x2918, // 61
0x3118, // 62
0x3918, // 63
0x0140, // 64
0x0940, // 65
0x1140, // 66
0x1940, // 67
0x2140, // 68
0x2940, // 69
0x3140, // 70
0x3940, // 71
0x0168, // 72
0x0968, // 73
0x1168, // 74
0x1968, // 75
0x2168, // 76
0x2968, // 77
0x3168, // 78
0x3968, // 79
0x0190, // 80
0x0990, // 81
0x1190, // 82
0x1990, // 83
0x2190, // 84
0x2990, // 85
0x3190, // 86
0x3990, // 87
0x01b8, // 88
0x09b8, // 89
0x11b8, // 90
0x19b8, // 91
0x21b8, // 92
0x29b8, // 93
0x31b8, // 94
0x39b8, // 95
0x01e0, // 96
0x09e0, // 97
0x11e0, // 98
0x19e0, // 99
0x21e0, // 100
0x29e0, // 101
0x31e0, // 102
0x39e0, // 103
0x0208, // 104
0x0a08, // 105
0x1208, // 106
0x1a08, // 107
0x2208, // 108
0x2a08, // 109
0x3208, // 110
0x3a08, // 111
0x0230, // 112
0x0a30, // 113
0x1230, // 114
0x1a30, // 115
0x2230, // 116
0x2a30, // 117
0x3230, // 118
0x3a30, // 119
0x0258, // 120
0x0a58, // 121
0x1258, // 122
0x1a58, // 123
0x2258, // 124
0x2a58, // 125
0x3258, // 126
0x3a58, // 127
0x0280, // 128
0x0a80, // 129
0x1280, // 130
0x1a80, // 131
0x2280, // 132
0x2a80, // 133
0x3280, // 134
0x3a80, // 135
0x02a8, // 136
0x0aa8, // 137
0x12a8, // 138
0x1aa8, // 139
0x22a8, // 140
0x2aa8, // 141
0x32a8, // 142
0x3aa8, // 143
0x02d0, // 144
0x0ad0, // 145
0x12d0, // 146
0x1ad0, // 147
0x22d0, // 148
0x2ad0, // 149
0x32d0, // 150
0x3ad0, // 151
0x02f8, // 152
0x0af8, // 153
0x12f8, // 154
0x1af8, // 155
0x22f8, // 156
0x2af8, // 157
0x32f8, // 158
0x3af8, // 159
0x0320, // 160
0x0b20, // 161
0x1320, // 162
0x1b20, // 163
0x2320, // 164
0x2b20, // 165
0x3320, // 166
0x3b20, // 167
0x0348, // 168
0x0b48, // 169
0x1348, // 170
0x1b48, // 171
0x2348, // 172
0x2b48, // 173
0x3348, // 174
0x3b48, // 175
0x0370, // 176
0x0b70, // 177
0x1370, // 178
0x1b70, // 179
0x2370, // 180
0x2b70, // 181
0x3370, // 182
0x3b70, // 183
0x0398, // 184
0x0b98, // 185
0x1398, // 186
0x1b98, // 187
0x2398, // 188
0x2b98, // 189
0x3398, // 190
0x3b98, // 191
0x03c0, // 192
0x0bc0, // 193
0x13c0, // 194
0x1bc0, // 195
0x23c0, // 196
0x2bc0, // 197
0x33c0, // 198
0x3bc0, // 199
};

/**
 * @brief 点を描画する
 * @param x     x座標(0 - 255)
 * @param y     y座標(0 - 239)
 * @param color RGB444形式
 */
void x1_pset(u8 x, u8 y, u8 paletteIndex)
{
    // 24～223を描画してみる
    if(y < 24) {
        return;
    }
    if(y >= 224) {
        return;
    }
    y -= 24;

    u8 mask = 0x80 >> (x & 7);
    u16 vramAddress = lineAddr[y] + (x >> 3);

    u8 a = inp(vramAddress | 0x4400);
    if(paletteIndex & 0x01) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0x4400, a);

    a = inp(vramAddress | 0x4000);
    if(paletteIndex & 0x02) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0x4000, a);

    a = inp(vramAddress | 0x8400);
    if(paletteIndex & 0x04) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0x8400, a);

    a = inp(vramAddress | 0x8000);
    if(paletteIndex & 0x08) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0x8000, a);

    a = inp(vramAddress | 0xC400);
    if(paletteIndex & 0x10) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0xC400, a);

    a = inp(vramAddress | 0xC000);
    if(paletteIndex & 0x20) { a |= mask; } else { a &= ~mask; }
    outp(vramAddress | 0xC000, a);
}
#endif
