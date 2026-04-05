#include "mapper.h"
// #include "../utils.h"

__sfr __banked __at(0x0B00) IoPortBank;

static void write_PRG(Mapper * mapper, u16 address, u8 value);
static u8 read_CHR(Mapper * mapper, u16 address);
static void write_CHR(Mapper * mapper, u16 address, u8 value);

int load_CNROM(Mapper *mapper) {
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->write_CHR = write_CHR;
    mapper->CHR_ptrs[0] = mapper->CHR_ROM;
    return 0;
}

static void write_PRG(Mapper *mapper, u16 address, u8 value) {
    (void)address;
    // 8k CHR bank selected determined by bit 0 - 1 or 0 - 2 if oversized
    u8 mask = mapper->CHR_banks > 4? 0xf : 0x3;
    mapper->CHR_ptrs[0] = mapper->CHR_ROM + 0x2000 * (value & mask);
}

static u8 read_CHR(Mapper *mapper, u16 address) {
    // return *(mapper->CHR_ptrs[0] + address);

    // BANK MEM 1
    __asm__ ("di"); // 割り込み禁止
    IoPortBank = 0x01; // バンク1
    u8 data = *(mapper->CHR_ptrs[0] + address);
    IoPortBank = 0x10; // メインメモリ
    __asm__ ("ei"); // 割り込み許可
    return data;
}

static void write_CHR(Mapper *mapper, u16 address, u8 value) {
    LOG(DEBUG, "Attempted to write to CHR-ROM");
    (void)mapper;
    (void)address;
    (void)value;
}
