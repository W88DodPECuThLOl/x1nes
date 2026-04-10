#include "../emulator.h"
#include "mapper.h"
// #include "../utils.h"

extern Emulator gEmulator;

__sfr __banked __at(0x0B00) IoPortBank;

static void write_PRG(u16 address, u8 value);
static u8 read_CHR_Bank1(u16 address);
static u8 read_CHR_Emm0(u16 address);
static void write_CHR(u16 address, u8 value);

int load_CNROM(Mapper *mapper) {
    mapper->write_PRG = write_PRG;
    mapper->PRG_ROM.ptr32 = 0;
    if(mapper->CHR_banks <= 4) {
        // ～32KiB
        // PRG : BANK0
        // CHR : BANK1
        mapper->CHR_ROM.ptr32 = 0;
        mapper->read_CHR = read_CHR_Bank1;
    } else {
        // 32KiB～
        // PRG : BANK0
        // CHR : EMM0
        mapper->CHR_ROM.ptr32 = (u32)INES_HEADER_SIZE + (u32)mapper->PRG_banks * 0x4000;
        mapper->read_CHR = read_CHR_Emm0;
    }
    mapper->write_CHR = write_CHR;
    mapper->CHR_ptrs[0] = mapper->CHR_ROM;
    return 0;
}

static void write_PRG(u16 address, u8 value) {
    (void)address;
    // 8k CHR bank selected determined by bit 0 - 1 or 0 - 2 if oversized
    u8 mask = gEmulator.mapper.CHR_banks > 4? 0xf : 0x3;
    gEmulator.mapper.CHR_ptrs[0].ptr32 = gEmulator.mapper.CHR_ROM.ptr32 + 0x2000 * (value & mask);
}

static u8 read_CHR_Bank1(u16 address) {
    // return *(mapper->CHR_ptrs[0] + address);

    // BANK MEM 1
    __asm__ ("di"); // 割り込み禁止
    IoPortBank = 0x01; // バンク1
    u8 data = *(gEmulator.mapper.CHR_ptrs[0].ptr + address);
    IoPortBank = 0x10; // メインメモリ
    __asm__ ("ei"); // 割り込み許可
    return data;
}

static u8 read_CHR_Emm0(u16 address) {
    // return *(mapper->CHR_ptrs[0] + address);
    return x1_readByteFromEmm0(gEmulator.mapper.CHR_ptrs[0].ptr32 + address);
}

static void write_CHR(u16 address, u8 value) {
    LOG(DEBUG, "Attempted to write to CHR-ROM");
    (void)address;
    (void)value;
}
