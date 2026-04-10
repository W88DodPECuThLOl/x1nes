#include "emulator.h"

Emulator gEmulator;
u8 main(){
    sos_msx("\x0c"); // screen clear
    init_emulator(&gEmulator, 0, nullptr);
    run_emulator(&gEmulator);
//    free_emulator(&gEmulator);
    return 0;
}
