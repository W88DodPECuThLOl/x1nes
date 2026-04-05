#include <string.h> // memset
#include "mmu.h"
#include "mappers/mapper.h"
#include "emulator.h"
//#include "utils.h"

#if !DYNAMIC_MAIN_RAM
static u8 RAM[RAM_SIZE];
extern Emulator emulator;
#endif

void init_mem(Emulator* emulator){
    Memory* mem = &emulator->mem;
    mem->emulator = emulator;
    mem->mapper = &emulator->mapper;

#if DYNAMIC_MAIN_RAM
    memset(mem->RAM, 0, RAM_SIZE);
#else
    memset(RAM, 0, RAM_SIZE);
#endif
#if defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
    init_joypad(&mem->joy1, 0);
    init_joypad(&mem->joy2, 1);
#endif // defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
}

u8* get_ptr(Memory* mem, u16 address){
    if(address < 0x2000)
#if DYNAMIC_MAIN_RAM
        return mem->RAM + (address % 0x800);
#else
        return RAM + (address % 0x800);
#endif
    if((address > 0x6000) && (address < 0x8000) && (mem->mapper->PRG_RAM != nullptr))
        return mem->mapper->PRG_RAM + (address - 0x6000);
    return nullptr;
}

void write_mem(Memory* mem, u16 address, u8 value){
    u8 old = mem->bus;
    mem->bus = value;

    if(address < RAM_END) {
#if DYNAMIC_MAIN_RAM
        mem->RAM[address % RAM_SIZE] = value;
#else
        RAM[address % RAM_SIZE] = value;
#endif
        return;
    }

    // resolve mirrored registers
    if(address < IO_REG_MIRRORED_END)
        address = 0x2000 + (address - 0x2000) % 0x8;

    // handle all IO registers
    if(address < IO_REG_END){
#if DYNAMIC_MAIN_RAM
        PPU* ppu = &mem->emulator->ppu;
#else
        PPU* ppu = &emulator.ppu;
#endif
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
        APU* apu = &mem->emulator->apu;
#endif // defined(ENABLE_AUDIO) && ENABLE_AUDIO
        switch (address) {
            case PPU_CTRL:
                ppu->bus = value;
                set_ctrl(ppu, value);
                break;
            case PPU_MASK:
                ppu->bus = value;
                ppu->mask = value;
                break;
            case PPU_SCROLL:
                ppu->bus = value;
                set_scroll(ppu, value);
                break;
            case PPU_ADDR:
                ppu->bus = value;
                set_address(ppu, value);
                break;
            case PPU_DATA:
                ppu->bus = value;
                write_ppu(ppu, value);
                break;
            case OAM_ADDR:
                ppu->bus = value;
                set_oam_address(ppu, value);
                break;
            case OAM_DMA:
                dma(ppu, value);
                break;
            case OAM_DATA:
                ppu->bus = value;
                write_oam(ppu, value);
                break;
            case PPU_STATUS:
                ppu->bus = value;
                break;
            case JOY1:
#if defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                write_joypad(&mem->joy1, value);
                write_joypad(&mem->joy2, value);
#endif // defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                mem->bus = (old & 0xf0) | value & 0xf;
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

    mem->mapper->write_ROM(mem->mapper, address, value);
}

u8 read_mem(Memory* mem, u16 address){
    if(address < RAM_END) {
#if DYNAMIC_MAIN_RAM
        mem->bus = mem->RAM[address % RAM_SIZE];
#else
        mem->bus = RAM[address % RAM_SIZE];
#endif
        return mem->bus;
    }
    
    // resolve mirrored registers
    if(address < IO_REG_MIRRORED_END)
        address = 0x2000 + (address - 0x2000) % 0x8;

    // handle all IO registers
    if(address < IO_REG_END){
#if DYNAMIC_MAIN_RAM
        PPU* ppu = &mem->emulator->ppu;
#else
        PPU* ppu = &emulator.ppu;
#endif
        switch (address) {
            case PPU_STATUS:
                ppu->bus &= 0x1f;
                ppu->bus |= read_status(ppu) & 0xe0;
                mem->bus = ppu->bus;
                return mem->bus;
            case OAM_DATA:
                mem->bus = ppu->bus = read_oam(ppu);
                return mem->bus;
            case PPU_DATA:
                mem->bus = ppu->bus = read_ppu(ppu);
                return mem->bus;
            case PPU_CTRL:
            case PPU_MASK:
            case PPU_SCROLL:
            case PPU_ADDR:
            case OAM_ADDR:
                // ppu open bus
                mem->bus = ppu->bus;
                return mem->bus;
            case JOY1:
                mem->bus &= 0xe0;
#if defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                mem->bus |= read_joypad(&mem->joy1) & 0x1f;
#endif // defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                return mem->bus;
            case JOY2:
                mem->bus &= 0xe0;
#if defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                mem->bus |= read_joypad(&mem->joy2) & 0x1f;
#endif // defined(ENABLE_JOYSTICK) && ENABLE_JOYSTICK
                return mem->bus;
            case APU_STATUS:
                // BIT 5 is open bus
                // reading from $4015 does not update the mem bus
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
                return (read_apu_status(&mem->emulator->apu) & ~BIT_5) | (mem->bus & BIT_5);
#endif
            default:
                // open bus
                return mem->bus;
        }
    }

    mem->bus = mem->mapper->read_ROM(mem->mapper, address);
    return mem->bus;
}
