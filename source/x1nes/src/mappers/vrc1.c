#include "../emulator.h"
#include "mapper.h"
//#include "../utils.h"

extern Emulator gEmulator;

static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 val);
static u8 read_CHR(u16 address);
static void update_CHR_ptrs();

typedef struct VCR1 {
    u8 CHR_0;
    u8 CHR_1;
} VRC1_t;

static VRC1_t vrc1;

int load_VRC1(Mapper *mapper) {
    // PRG : EMM0
    // CHR : EMM0
    mapper->PRG_ROM.ptr32 = (u32)INES_HEADER_SIZE;
    mapper->CHR_ROM.ptr32 = (u32)INES_HEADER_SIZE + (u32)mapper->PRG_banks * 0x4000;

    //    mapper->extension = vrc1;
    mapper->write_PRG = write_PRG;
    mapper->read_PRG = read_PRG;
    mapper->read_CHR = read_CHR;
    // Fix last 8k bank
    mapper->PRG_ptrs[3].ptr32 = mapper->PRG_ROM.ptr32 + (u32)(mapper->PRG_banks * 2 - 1) * 0x2000;
    // Init all PRG banks to first bank
    mapper->PRG_ptrs[0].ptr32 = mapper->PRG_ptrs[1].ptr32 = mapper->PRG_ptrs[2].ptr32 = mapper->PRG_ROM.ptr32;
    // Initialize both banks to the first bank
    vrc1.CHR_0 = vrc1.CHR_1 = 0;
    update_CHR_ptrs();
    return 0;
}

static void write_PRG(u16 address, u8 val) {
    switch (address & 0xf000) {
        case 0x8000:
            gEmulator.mapper.PRG_ptrs[0].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(val & 0xf) * 0x2000;
            break;
        case 0x9000:
            if (gEmulator.mapper.mirroring != FOUR_SCREEN) {
                if (val & (1 << 1))
                    set_mirroring(HORIZONTAL);
                else
                    set_mirroring(VERTICAL);
            }
            vrc1.CHR_0 = (vrc1.CHR_0 & 0xf) | ((val & (1 << 1)) << 3);
            vrc1.CHR_1 = (vrc1.CHR_1 & 0xf) | ((val & (1 << 2)) << 2);
            update_CHR_ptrs();
            break;
        case 0xa000:
            gEmulator.mapper.PRG_ptrs[1].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(val & 0xf) * 0x2000;
            break;
        case 0xc000:
            gEmulator.mapper.PRG_ptrs[2].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(val & 0xf) * 0x2000;
            break;
        case 0xe000:
            vrc1.CHR_0 = (vrc1.CHR_0 & ~0xf) | (val & 0xf);
            update_CHR_ptrs();
            break;
        case 0xf000:
            vrc1.CHR_1 = (vrc1.CHR_1 & ~0xf) | (val & 0xf);
            update_CHR_ptrs();
            break;
        default:
            break;
    }
}

static u8 read_PRG(u16 address) {
    return x1_readByteFromEmm0(gEmulator.mapper.PRG_ptrs[(address & 0x7fff) >> 13].ptr32 + (address & 0x1fff));
}

static u8 read_CHR(u16 address) {
    return x1_readByteFromEmm0(gEmulator.mapper.CHR_ptrs[address >> 12].ptr32 + (address & 0xfff));
}

static void update_CHR_ptrs() {
    gEmulator.mapper.CHR_ptrs[0].ptr32 = gEmulator.mapper.CHR_ROM.ptr32 + (u32)vrc1.CHR_0 * 0x1000;
    gEmulator.mapper.CHR_ptrs[1].ptr32 = gEmulator.mapper.CHR_ROM.ptr32 + (u32)vrc1.CHR_1 * 0x1000;
}
