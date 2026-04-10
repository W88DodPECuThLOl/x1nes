#ifndef INCL_emulator_h
#define INCL_emulator_h

#include "dev/BasicTypes.h"

#include "cpu6502.h"
#include "ppu.h"
#include "mmu.h"
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
#include "apu.h"
#endif // defined(ENABLE_AUDIO) && ENABLE_AUDIO
#include "mappers/mapper.h"
//#include "gfx.h"
//#include "timers.h"


// frame rate in Hz
#define NTSC_FRAME_RATE 60
#define PAL_FRAME_RATE 50

// turbo keys toggle rate (Hz)
// value should be a factor of FRAME_RATE
// and should never exceed FRAME_RATE for best result
#define NTSC_TURBO_RATE 30
#define PAL_TURBO_RATE 25

// sleep time when emulator is paused in milliseconds
#define IDLE_SLEEP 50


typedef struct Emulator {
    c6502 cpu;
    PPU ppu;
#if defined(ENABLE_AUDIO) && ENABLE_AUDIO
    APU apu;
#endif // defined(ENABLE_AUDIO) && ENABLE_AUDIO
    Memory mem;
    Mapper mapper;
//    GraphicsContext g_ctx;
//    Timer timer;

    TVSystem type;

//    double time_diff;

    u8 exit;
    u8 pause;
} Emulator;

void init_emulator(Emulator* emulator, int argc, char *argv[]);
void reset_emulator(Emulator* emulator);
void run_emulator(Emulator* emulator);
void run_NSF_player(Emulator* emulator);
void free_emulator(Emulator* emulator);

#endif // INCL_emulator_h
