#include "../emulator.h"
#include "mapper.h"
//#include "../utils.h"

extern Emulator gEmulator;

static u32 next_power_of_2(u32 num)
{
    u32 power = 1;
    while(power < num)
        power*=2;
    return power;
}

typedef struct {
    u8 PRG_reg;
    u8 CHR1_reg;
    u8 CHR2_reg;
//    u8 *PRG_bank1;
//    u8 *PRG_bank2;
//    u8 *CHR_bank1;
//    u8 *CHR_bank2;
    u32 PRG_bank1; // ptr32
    u32 PRG_bank2; // ptr32
    u32 CHR_bank1; // ptr32
    u32 CHR_bank2; // ptr32
    u8 CHR_mode;
    u8 PRG_mode;
    u8 reg;
    u32 PRG_clamp;
    u32 CHR_clamp;
    u8 cpu_cycle;
} MMC1_t;

enum {
    MIRROR_BITS = 0x3,
    REG_RESET = 1<<7,
    CHR_MODE = 1<<4,
    PRG_MODE = 0xC,
    REG_INIT = 0b100000
};

static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 value);
static u8 read_CHR(u16 address);
static void set_PRG_banks();
static void set_CHR_banks();

static MMC1_t mmc1;

int load_MMC1(Mapper *mapper) {
    // PRG : EMM0
    // CHR : EMM0
    mapper->PRG_ROM.ptr32 = (u32)INES_HEADER_SIZE;
    mapper->CHR_ROM.ptr32 = (u32)INES_HEADER_SIZE + (u32)mapper->PRG_banks * 0x4000;

    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mmc1.reg = REG_INIT;
    mmc1.PRG_mode = 3;
    mmc1.cpu_cycle = (u8)~0;
    // PRG banks in 8k chunks
    mmc1.PRG_clamp = next_power_of_2(mapper->PRG_banks);
    mmc1.PRG_clamp = mmc1.PRG_clamp > 0 ? mmc1.PRG_clamp - 1 : 0;
    // CHR banks in 1k chunks
    mmc1.CHR_clamp = next_power_of_2(mapper->CHR_banks * 2);
    mmc1.CHR_clamp = mmc1.CHR_clamp > 0 ? mmc1.CHR_clamp - 1 : 0;

    if (mapper->CHR_banks) {
        mmc1.CHR_bank1 = mapper->CHR_ROM.ptr32;
        mmc1.CHR_bank2 = mmc1.CHR_bank1 + 0x1000;
    }

    mmc1.PRG_bank1 = mapper->PRG_ROM.ptr32;
    mmc1.PRG_bank2 = mapper->PRG_ROM.ptr32 + (u32)(mapper->PRG_banks - 1) * 0x4000;
    return 0;
}


static u8 read_PRG(u16 address) {
    if (address < 0xC000)
        return x1_readByteFromEmm0(mmc1.PRG_bank1 + (address & 0x3fff));
    return x1_readByteFromEmm0(mmc1.PRG_bank2 + (address & 0x3fff));
}

static void write_PRG(u16 address, u8 value) {
    u8 same_cycle = gEmulator.cpu.t_cycles == mmc1.cpu_cycle;
    mmc1.cpu_cycle = gEmulator.cpu.t_cycles;
    if (value & REG_RESET) {
        // reset
        mmc1.reg = REG_INIT;
        mmc1.PRG_mode = 3;
        set_PRG_banks();
    } else {
        // ignore consequtive writes
        if (same_cycle)
            return;
        mmc1.reg = (mmc1.reg >> 1) | ((value & 1) << 5);

        if (!(mmc1.reg & 1))
            // register not yet full
            return;

        // remove register size check bit
        mmc1.reg >>= 1;
        switch (address & 0xE000) {
            case 0x8000:
                // mirroring
                switch (mmc1.reg & MIRROR_BITS) {
                    case 0: set_mirroring(ONE_SCREEN_LOWER);
                        break;
                    case 1: set_mirroring(ONE_SCREEN_UPPER);
                        break;
                    case 2: set_mirroring(VERTICAL);
                        break;
                    case 3: set_mirroring(HORIZONTAL);
                        break;
                    default: break;
                }

                mmc1.CHR_mode = (mmc1.reg & CHR_MODE) >> 4;
                mmc1.PRG_mode = (mmc1.reg & PRG_MODE) >> 2;

                set_PRG_banks();
                set_CHR_banks();
                break;
            case 0xa000:
                if (gEmulator.mapper.CHR_RAM_size) {
                    mmc1.PRG_reg &= ~(1<<4);
                    mmc1.PRG_reg |= mmc1.reg & (1<<4);
                    mmc1.PRG_reg &= mmc1.PRG_clamp;
                    set_PRG_banks();
                } else {
                    mmc1.CHR1_reg = mmc1.reg & 0x1f;
                    mmc1.CHR1_reg &= mmc1.CHR_clamp;
                    set_CHR_banks();
                }

                break;
            case 0xc000:
                if (!mmc1.CHR_mode)
                    break;
                if (gEmulator.mapper.CHR_RAM_size) {
                    mmc1.PRG_reg &= ~(1<<4);
                    mmc1.PRG_reg |= mmc1.reg & (1<<4);
                    mmc1.PRG_reg &= mmc1.PRG_clamp;
                    set_PRG_banks();
                } else {
                    mmc1.CHR2_reg = mmc1.reg & 0x1f;
                    mmc1.CHR2_reg &= mmc1.CHR_clamp;
                    set_CHR_banks();
                }
                break;
            case 0xe000:
                mmc1.PRG_reg &= ~0xf;
                mmc1.PRG_reg |= mmc1.reg & 0xF;
                mmc1.PRG_reg &= mmc1.PRG_clamp;
                set_PRG_banks();
                break;
            default:
                break;
        }
        mmc1.reg = REG_INIT;
    }
}

static void set_PRG_banks() {
    switch (mmc1.PRG_mode) {
        case 0:
        case 1:
            mmc1.PRG_bank1 = gEmulator.mapper.PRG_ROM.ptr32 + (0x4000 * (u32)(mmc1.PRG_reg & ~1));
            mmc1.PRG_bank2 = mmc1.PRG_bank1 + 0x4000;
            break;
        case 2:
            // fix first bank switch second bank
            mmc1.PRG_bank1 = gEmulator.mapper.PRG_ROM.ptr32 + (0x4000 * (u32)(mmc1.PRG_reg & (1<<4)));
            mmc1.PRG_bank2 = gEmulator.mapper.PRG_ROM.ptr32 + 0x4000 * (u32)mmc1.PRG_reg;
            break;
        case 3:
            // fix second bank switch first bank
            mmc1.PRG_bank1 = gEmulator.mapper.PRG_ROM.ptr32 + 0x4000 * (u32)mmc1.PRG_reg;
            if (gEmulator.mapper.PRG_banks > 16)
                mmc1.PRG_bank2 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(((mmc1.PRG_reg & (1<<4)) > 0) + 1) * 0x40000 - 0x4000;
            else
                mmc1.PRG_bank2 = gEmulator.mapper.PRG_ROM.ptr32 + (u32)(gEmulator.mapper.PRG_banks - 1) * 0x4000;
            break;
        default:
            break;
    }
}

static void set_CHR_banks() {
    if (mmc1.CHR_mode) {
        // 2 4KB banks
        mmc1.CHR_bank1 = gEmulator.mapper.CHR_ROM.ptr32 + (0x1000 * (u32)mmc1.CHR1_reg);
        mmc1.CHR_bank2 = gEmulator.mapper.CHR_ROM.ptr32 + (0x1000 * (u32)mmc1.CHR2_reg);
    } else {
        mmc1.CHR_bank1 = gEmulator.mapper.CHR_ROM.ptr32 + (0x1000 * (u32)(mmc1.CHR1_reg & ~1));
        mmc1.CHR_bank2 = mmc1.CHR_bank1 + 0x1000;
    }
}

static u8 read_CHR(u16 address) {
    if (gEmulator.mapper.CHR_RAM_size)
        return x1_readByteFromEmm0(gEmulator.mapper.CHR_ROM.ptr32 + address);
    if (address < 0x1000)
        return x1_readByteFromEmm0(mmc1.CHR_bank1 + address);
    return x1_readByteFromEmm0(mmc1.CHR_bank2 + (address & 0xfff));
}
