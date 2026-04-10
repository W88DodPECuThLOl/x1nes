#ifndef INCL_controller_h
#define INCL_controller_h

#include "dev/BasicTypes.h"

typedef enum {
    TURBO_B     = 1 << 9,
    TURBO_A     = 1 << 8,
    RIGHT       = 1 << 7,
    LEFT        = 1 << 6,
    DOWN        = 1 << 5,
    UP          = 1 << 4,
    START       = 1 << 3,
    SELECT      = 1 << 2,
    BUTTON_B    = 1 << 1,
    BUTTON_A    = 1
} KeyPad;

typedef struct JoyPad{
    u8 strobe;
    u8 index;
    u16 status;
    u8 player;
} JoyPad;


void init_joypad(struct JoyPad* joyPad, u8 player);
u8   read_joypad(struct JoyPad* joyPad);
void write_joypad(struct JoyPad* joyPad, u8 data);
//void update_joypad(struct JoyPad* joyPad, SDL_Event* event);
void update_joypad(struct JoyPad* joyPad);
void turbo_trigger(struct JoyPad* joyPad);
//void keyboard_mapper(struct JoyPad* joyPad, SDL_Event* event);
void keyboard_mapper(struct JoyPad* joyPad);

#endif // INCL_controller_h
