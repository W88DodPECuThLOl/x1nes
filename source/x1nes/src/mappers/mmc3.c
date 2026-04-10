#include "../emulator.h"
#include "mapper.h"
#include "../cpu6502.h"

//#include "../utils.h"

extern Emulator gEmulator;

typedef struct {
    u32 PRG_bank_ptrs[4]; // ptr
    u32 CHR_bank_ptrs[8]; // ptr
    u8 PRG_mode;
    u8 CHR_inversion;
    u8 next_bank_data;
    u8 IRQ_latch;
    u8 IRQ_counter;
    u8 IRQ_cleared;
    u8 IRQ_enabled;
    u32 PRG_clamp;
    u32 CHR_clamp;
} MMC3_t;


static u8 read_PRG(u16 addr);

static void write_PRG(u16 addr, u8 val);

static u8 read_CHR(u16 addr);

static void write_bank_data(u8 val);

static void on_scanline();

static MMC3_t mmc3;

static u32 next_power_of_2(u32 num)
{
    u32 power = 1;
    while(power < num)
        power*=2;
    return power;
}

int load_MMC3(Mapper *mapper) {
    // PRG : EMM0
    // CHR : EMM0
    mapper->PRG_ROM.ptr32 = (u32)INES_HEADER_SIZE;
    mapper->CHR_ROM.ptr32 = (u32)INES_HEADER_SIZE + (u32)mapper->PRG_banks * 0x4000;

    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->on_scanline = on_scanline;
    // PRG banks in 8k chunks
    mmc3.PRG_clamp = next_power_of_2(mapper->PRG_banks * 2);
    mmc3.PRG_clamp = mmc3.PRG_clamp > 0 ? mmc3.PRG_clamp - 1: 0;
    // CHR banks in 1k chunks
    mmc3.CHR_clamp = next_power_of_2(mapper->CHR_banks * 8);
    mmc3.CHR_clamp = mmc3.CHR_clamp > 0 ? mmc3.CHR_clamp - 1: 0;

    // last bank
    mmc3.PRG_bank_ptrs[3] = mapper->PRG_ROM.ptr32 + (u32)mapper->PRG_banks * 0x4000 - 0x2000;
    // 2nd last bank
    mmc3.PRG_bank_ptrs[2] = mmc3.PRG_bank_ptrs[3] - 0x2000;

    // point to first banks to prevent seg faults just in case
    mmc3.PRG_bank_ptrs[0] = mmc3.PRG_bank_ptrs[1] = mapper->PRG_ROM.ptr32;

    // point CHR to first bank
    for(int i = 0; i < 8; i++)
        mmc3.CHR_bank_ptrs[i] = mapper->CHR_ROM.ptr32;
    mmc3.CHR_bank_ptrs[1] = mmc3.CHR_bank_ptrs[0] + 0x400;
    mmc3.CHR_bank_ptrs[3] = mmc3.CHR_bank_ptrs[0] + 0x400;
    return 0;
}

static void on_scanline() {
    // TODO cycle-accurate A12 based IRQ
    if(mmc3.IRQ_cleared || !mmc3.IRQ_counter) {
        mmc3.IRQ_counter = mmc3.IRQ_latch;
        mmc3.IRQ_cleared = 0;
    }else {
        mmc3.IRQ_counter--;
    }

    if(!mmc3.IRQ_counter && mmc3.IRQ_enabled)
        interrupt(MAPPER_IRQ);
}

u8 read_PRG(u16 addr) {
    switch (addr & 0xE000) {
        case 0x8000:
            if (mmc3.PRG_mode) {
                // 2nd last
                return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[2] + (addr - 0x8000));
            }
            // R6
            return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[0] + (addr - 0x8000));
        case 0xA000:
            // R7
            return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[1] + (addr - 0xA000));
        case 0xC000:
            if (mmc3.PRG_mode) {
                // R6
                return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[0] + (addr - 0xC000));
            }
            // 2nd-last
            return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[2] + (addr - 0xC000));
        case 0xE000:
            // last
            return x1_readByteFromEmm0(mmc3.PRG_bank_ptrs[3] + (addr - 0xE000));
        default:
            // out of bounds
            LOG(ERROR, "PRG Read (0x%04x) out of bounds", addr);
            return 0;
    }
}

void write_PRG(u16 addr, u8 val) {
    switch (addr & 0xE001) {
        case 0x8000:
            mmc3.next_bank_data = val & 0x7;
            mmc3.PRG_mode = (val >> 6) & 1;
            mmc3.CHR_inversion = (val >> 7) & 1;
            break;
        case 0x8001:
            write_bank_data(val);
            break;
        case 0xA000:
            if (gEmulator.mapper.mirroring != FOUR_SCREEN)
                set_mirroring(val & 1 ? HORIZONTAL : VERTICAL);
            break;
        case 0xA001:
            // PRG RAM write protect stuff
            break;
        case 0xC000:
            mmc3.IRQ_latch = val;
            break;
        case 0xC001:
            mmc3.IRQ_counter = 0;
            mmc3.IRQ_cleared = 1;
            break;
        case 0xE000:
            mmc3.IRQ_enabled = 0;
            // acknowledge pending interrupts
            interrupt_clear(MAPPER_IRQ);
            break;
        case 0xE001:
            mmc3.IRQ_enabled = 1;
            break;
        default:
            // out of bounds
            LOG(ERROR, "PRG Write (0x%04x) out of bounds", addr);
            break;
    }
}

void write_bank_data(u8 val) {
    switch (mmc3.next_bank_data) {
        case 0:
            // R0
            val = val & 0xFE;
            val &= mmc3.CHR_clamp;
            mmc3.CHR_bank_ptrs[0] = gEmulator.mapper.CHR_ROM.ptr32 + (u32)val * 0x400;
            mmc3.CHR_bank_ptrs[1] = mmc3.CHR_bank_ptrs[0] + 0x400;
            break;
        case 1:
            // R1
            val = val & 0xFE;
            val &= mmc3.CHR_clamp;
            mmc3.CHR_bank_ptrs[2] = gEmulator.mapper.CHR_ROM.ptr32 + (u32)val * 0x400;
            mmc3.CHR_bank_ptrs[3] = mmc3.CHR_bank_ptrs[2] + 0x400;
            break;
        case 2:case 3:case 4:case 5:
            // R2/R3/R4/R5
            val &= mmc3.CHR_clamp;
            mmc3.CHR_bank_ptrs[mmc3.next_bank_data + 2] = gEmulator.mapper.CHR_ROM.ptr32 + (u32)val * 0x400;
            break;
        case 6:case 7:default:
            // R6/R7
            val &= mmc3.PRG_clamp;
            mmc3.PRG_bank_ptrs[mmc3.next_bank_data - 6] = gEmulator.mapper.PRG_ROM.ptr32 + (u32)val * 0x2000;
            break;
    }
}

u8 read_CHR(u16 addr) {
    if(!gEmulator.mapper.CHR_banks) {
        return x1_readByteFromEmm0(gEmulator.mapper.CHR_ROM.ptr32 + addr);
    }
    u8 ptr_index = (addr & 0x1C00) / 0x400;
    addr = addr - (addr & 0x1C00);
    if(mmc3.CHR_inversion) {
        ptr_index = (ptr_index + 4) % 8;
    }
    return x1_readByteFromEmm0(mmc3.CHR_bank_ptrs[ptr_index] + addr);
}
