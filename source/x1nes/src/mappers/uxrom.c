#include "../emulator.h"
#include "mapper.h"

extern Emulator gEmulator;

static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 value);
static void write_PRG_UN1ROM(u16 address, u8 value);
static u8 read_PRG_180(u16 address);

int load_UXROM(Mapper *mapper) {
    // Mapper #2
    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    // last bank offset
    mapper->clamp = (mapper->PRG_banks - 1) * 0x4000;
    mapper->PRG_ptrs[0] = mapper->PRG_ROM;
    return 0;
}

int load_UN1ROM(Mapper *mapper) {
    // Mapper #94
    load_UXROM(mapper);
    mapper->write_PRG = write_PRG_UN1ROM;
    return 0;
}

int load_mapper180(Mapper *mapper) {
    // Mapper #180
    load_UXROM(mapper);
    mapper->read_PRG = read_PRG_180;
    return 0;
}

static u8 read_PRG(u16 address) {
    if (address < 0xC000)
        return x1_readByteFromEmm0(gEmulator.mapper.PRG_ptrs[0].ptr32 + (address - 0x8000));
    return x1_readByteFromEmm0(gEmulator.mapper.PRG_ROM.ptr32 + gEmulator.mapper.clamp + (address - 0xC000));
}

static void write_PRG(u16 address, u8 value) {
    (void)address;
    gEmulator.mapper.PRG_ptrs[0].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(value & 0x7) * 0x4000;
}

static void write_PRG_UN1ROM(u16 address, u8 value) {
    (void)address;
    gEmulator.mapper.PRG_ptrs[0].ptr32 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)((value & 0b11100) >> 2) * 0x4000;
}

static u8 read_PRG_180(u16 address) {
    if (address < 0xC000)
        // Fixed to 1st bank
        return x1_readByteFromEmm0(gEmulator.mapper.PRG_ROM.ptr32 + (address - 0x8000));
    return x1_readByteFromEmm0(gEmulator.mapper.PRG_ptrs[0].ptr32 + (address - 0xC000));
}
