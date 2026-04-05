#include "emulator.h"

Emulator emulator;
int main(){
    sos_msx("\x0c"); // screen clear
    init_emulator(&emulator, 0, nullptr);
    run_emulator(&emulator);
    free_emulator(&emulator);
    return 0;
}
