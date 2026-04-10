#include <string.h> // memset
#include "mmu.h"
#include "mappers/mapper.h"
#include "emulator.h"
//#include "utils.h"

#if !DYNAMIC_MAIN_RAM
static u8 RAM[RAM_SIZE];
extern Emulator gEmulator;
#endif

void init_mem(Emulator* emulator){
    Memory* mem = &emulator->mem;
    mem->emulator = emulator;
    mem->mapper = &emulator->mapper;

    memset(RAM, 0, RAM_SIZE);

    init_joypad(&mem->joy1, 0);
    init_joypad(&mem->joy2, 1);
}

u8* get_ptr(u16 address){
    if(address < 0x2000)
        return RAM + (address % 0x800);
    if((address > 0x6000) && (address < 0x8000) && (gEmulator.mapper.PRG_RAM != nullptr))
        return gEmulator.mapper.PRG_RAM + (address - 0x6000);
    return nullptr;
}

void write_mem(u16 address, u8 value){
    u8 old = gEmulator.mem.bus;
    gEmulator.mem.bus = value;

    if(address < RAM_END) {
        RAM[address % RAM_SIZE] = value;
        return;
    }

    // resolve mirrored registers
    if(address < IO_REG_MIRRORED_END)
        address = 0x2000 + (address - 0x2000) % 0x8;

    // handle all IO registers
    if(address < IO_REG_END){
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
        APU* apu = &mem->emulator->apu;
#endif // defined(ENABLE_AUDIO) && ENABLE_AUDIO
        switch (address) {
            case PPU_CTRL:
                ppu_bus = value;
                set_ctrl(value);
                break;
            case PPU_MASK:
                ppu_bus = value;
                ppu_mask = value;
                break;
            case PPU_SCROLL:
                ppu_bus = value;
                set_scroll(value);
                break;
            case PPU_ADDR:
                ppu_bus = value;
                set_address(value);
                break;
            case PPU_DATA:
                ppu_bus = value;
                write_ppu(value);
                break;
            case OAM_ADDR:
                ppu_bus = value;
                set_oam_address(value);
                break;
            case OAM_DMA:
                dma(value);
                break;
            case OAM_DATA:
                ppu_bus = value;
                write_oam(value);
                break;
            case PPU_STATUS:
                ppu_bus = value;
                break;
            case JOY1:
                write_joypad(&gEmulator.mem.joy1, value);
                write_joypad(&gEmulator.mem.joy2, value);
                gEmulator.mem.bus = (old & 0xf0) | value & 0xf;
                break;
            case APU_P1_CTRL:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_ctrl(&apu->pulse1, value);
#endif
                break;
            case APU_P2_CTRL:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_ctrl(&apu->pulse2, value);
#endif
                break;
            case APU_P1_RAMP:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_sweep(&apu->pulse1, value);
#endif
                break;
            case APU_P2_RAMP:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_sweep(&apu->pulse2, value);
#endif
                break;
            case APU_P1_FT:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_timer(&apu->pulse1, value);
#endif
                break;
            case APU_P2_FT:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_timer(&apu->pulse2, value);
#endif
                break;
            case APU_P1_CT:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_length_counter(&apu->pulse1, value);
#endif
                break;
            case APU_P2_CT:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_pulse_length_counter(&apu->pulse2, value);
#endif
                break;
            case APU_TRI_LINEAR_COUNTER:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_tri_counter(&apu->triangle, value);
#endif
                break;
            case APU_TRI_FREQ1:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_tri_timer_low(&apu->triangle, value);
#endif
                break;
            case APU_TRI_FREQ2:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_tri_length(&apu->triangle, value);
#endif
                break;
            case APU_NOISE_CTRL:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_noise_ctrl(&apu->noise, value);
#endif
                break;
            case APU_NOISE_FREQ1:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_noise_period(apu, value);
#endif
                break;
            case APU_NOISE_FREQ2:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_noise_length(&apu->noise, value);
#endif
                break;
            case APU_DMC_ADDR:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_dmc_addr(&apu->dmc, value);
#endif
                break;
            case APU_DMC_CTRL:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_dmc_ctrl(apu, value);
#endif
                break;
            case APU_DMC_DA:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_dmc_da(&apu->dmc, value);
#endif
                break;
            case APU_DMC_LEN:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_dmc_length(&apu->dmc, value);
#endif
                break;
            case FRAME_COUNTER:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_frame_counter_ctrl(apu, value);
#endif
                break;
            case APU_STATUS:
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                set_status(apu, value);
#endif
                break;
            default:
                break;
        }
        return;
    }

    gEmulator.mapper.write_ROM(address, value);
}

u8 read_mem(u16 address){
    if(address < RAM_END) {
        gEmulator.mem.bus = RAM[address % RAM_SIZE];
        return gEmulator.mem.bus;
    }
    
    // resolve mirrored registers
    if(address < IO_REG_MIRRORED_END)
        address = 0x2000 + (address - 0x2000) % 0x8;

    // handle all IO registers
    if(address < IO_REG_END){
        switch (address) {
            case PPU_STATUS:
                ppu_bus &= 0x1f;
                ppu_bus |= read_status() & 0xe0;
                gEmulator.mem.bus = ppu_bus;
                return gEmulator.mem.bus;
            case OAM_DATA:
                gEmulator.mem.bus = ppu_bus = read_oam();
                return gEmulator.mem.bus;
            case PPU_DATA:
                gEmulator.mem.bus = ppu_bus = read_ppu();
                return gEmulator.mem.bus;
            case PPU_CTRL:
            case PPU_MASK:
            case PPU_SCROLL:
            case PPU_ADDR:
            case OAM_ADDR:
                // ppu open bus
                gEmulator.mem.bus = ppu_bus;
                return gEmulator.mem.bus;
            case JOY1:
                gEmulator.mem.bus &= 0xe0;
                gEmulator.mem.bus |= read_joypad(&gEmulator.mem.joy1) & 0x1f;
                return gEmulator.mem.bus;
            case JOY2:
                gEmulator.mem.bus &= 0xe0;
                gEmulator.mem.bus |= read_joypad(&gEmulator.mem.joy2) & 0x1f;
                return gEmulator.mem.bus;
            case APU_STATUS:
                // BIT 5 is open bus
                // reading from $4015 does not update the mem bus
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                return (read_apu_status(&gEmulator.mem.emulator->apu) & ~BIT_5) | (gEmulator.mem.bus & BIT_5);
#endif
            default:
                // open bus
                return gEmulator.mem.bus;
        }
    }

    gEmulator.mem.bus = gEmulator.mapper.read_ROM(address);
    return gEmulator.mem.bus;
}
