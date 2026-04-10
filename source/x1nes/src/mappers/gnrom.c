#include "../emulator.h"
#include "mapper.h"

extern Emulator gEmulator;

static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 value);
static u8 read_CHR(u16 address);

int load_GNROM(Mapper *mapper) {
    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->PRG_ptrs[0] = mapper->PRG_ROM;
    mapper->CHR_ptrs[0] = mapper->CHR_ROM;
    return 0;
}

static u8 read_PRG(u16 address) {
    return x1_readByteFromEmm0(gEmulator.mapper.PRG_ptrs[0].ptr32 + (address - 0x8000));
}

static void write_PRG(u16 address, u8 value) {
    (void)address;

    // PRG bank determined by bit 5 - 4
    gEmulator.mapper.PRG_ptrs[0].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + ((value >> 4) & 0x3) * 0x8000;
    // 8k CHR bank selected determined by bit 0 - 1
    gEmulator.mapper.CHR_ptrs[0].ptr32 = gEmulator.mapper.CHR_ROM.ptr32 + 0x2000 * (value & 0x3);
}

static u8 read_CHR(u16 address) {
    return x1_readByteFromEmm0(gEmulator.mapper.CHR_ptrs[0].ptr32 + address);
}
