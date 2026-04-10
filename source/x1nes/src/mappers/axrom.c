#include "../emulator.h"
#include "mapper.h"

extern Emulator gEmulator;

static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 value);

int load_AOROM(Mapper *mapper) {
    // PRG : EMM0
    // CHR : BANK1
    mapper->PRG_ROM.ptr32 = (u32)INES_HEADER_SIZE;
    mapper->CHR_ROM.ptr32 = 0;

    mapper->write_PRG = write_PRG;
    mapper->read_PRG = read_PRG;
    mapper->PRG_ptrs[0] = mapper->PRG_ROM;
    return 0;
}

static void write_PRG(u16 address, u8 value) {
    (void)address;

    gEmulator.mapper.PRG_ptrs[0].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(value & 0x7) * 0x8000;
    if ((value >> 4) & 0x1)
        set_mirroring(ONE_SCREEN_UPPER);
    else
        set_mirroring(ONE_SCREEN_LOWER);
}

static u8 read_PRG(u16 address) {
    // PRG bank determined by bit 0 - 2
    return x1_readByteFromEmm0(gEmulator.mapper.PRG_ptrs[0].ptr32 + (address - 0x8000));
}
