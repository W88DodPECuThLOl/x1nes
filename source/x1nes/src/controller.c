#include "controller.h"
//#include "gamepad.h"
//#include "touchpad.h"

#define JOY_DEBUG (1)

void init_joypad(struct JoyPad* joyPad, u8 player){
    joyPad->strobe = 0;
    joyPad->index = 0;
    joyPad->status = 0;
    joyPad->player = player;

#if JOY_DEBUG // for debug
    sos_loc(0x0021); // Y:0 X:33
    sos_msx("Joy#0");
#endif
}


u8 read_joypad(struct JoyPad* joyPad){
    if(joyPad->index > 7)
        return 1;
    u8 val = (joyPad->status & (1 << joyPad->index)) != 0;
    if(!joyPad->strobe)
        joyPad->index++;
    return val;
}


void write_joypad(struct JoyPad* joyPad, u8 data){
    joyPad->strobe = data & 1;
    if(joyPad->strobe) {
        joyPad->index = 0;
        update_joypad(joyPad);
    }
}


#if 0
void keyboard_mapper(struct JoyPad* joyPad, SDL_Event* event){
    uint16_t key = 0;
    switch (event->key.key) {
        case SDLK_RIGHT:
            key = RIGHT;
            break;
        case SDLK_LEFT:
            key = LEFT;
            break;
        case SDLK_DOWN:
            key = DOWN;
            break;
        case SDLK_UP:
            key = UP;
            break;
        case SDLK_RETURN:
            key = START;
            break;
        case SDLK_RSHIFT:
            key = SELECT;
            break;
        case SDLK_J:
            key = BUTTON_A;
            break;
        case SDLK_K:
            key = BUTTON_B;
            break;
        case SDLK_L:
            key = TURBO_B;
            break;
        case SDLK_H:
            key = TURBO_A;
            break;

    }
    if(event->type == SDL_EVENT_KEY_UP) {
        joyPad->status &= ~key;
        if(key == TURBO_A) {
            // clear button A
            joyPad->status &= ~BUTTON_A;
        }
        if(key == TURBO_B) {
            // clear button B
            joyPad->status &= ~BUTTON_B;
        }
    } else if(event->type == SDL_EVENT_KEY_DOWN) {
        joyPad->status |= key;
        if(key == TURBO_A) {
            // set button A
            joyPad->status |= BUTTON_A;
        }
        if(key == TURBO_B) {
            // set button B
            joyPad->status |= BUTTON_B;
        }
    }
}
#endif

void update_joypad(struct JoyPad* joyPad){
    if(joyPad->player != 0) {
        return;
    }

    u8 data0 = ~x1_readJoyStick(0);
    u8 data1 = ~x1_readJoyStick(1);

    joyPad->status = 0;
    if(data0 & (1 << 0)) { joyPad->status |= UP; }
    if(data0 & (1 << 1)) { joyPad->status |= DOWN; }
    if(data0 & (1 << 2)) { joyPad->status |= LEFT; }
    if(data0 & (1 << 3)) { joyPad->status |= RIGHT; }

    // @todo どっちがBUTTON_Aなのか、あってるのか？
    if(data0 & (1 << 5)) { joyPad->status |= BUTTON_A; } // TRIGGER1
    if(data0 & (1 << 6)) { joyPad->status |= BUTTON_B; } // TRIGGER2

    // @todo START, SELECTボタンないよ？
    if(data1 & (1 << 5)) { joyPad->status |= START; } // TRIGGER1
    if(data1 & (1 << 6)) { joyPad->status |= SELECT; } // TRIGGER2

#if JOY_DEBUG // for debug
    sos_loc(0x0120); // Y:1 X:33
    char text[9];
    char* p = text;
    for(u8 i = 0x01; i != 0; i<<=1) {
        *p++ = (joyPad->status & i) ? '1' : '0';
    }
    *p = 0;
    sos_msx(text);
#endif
}

void turbo_trigger(struct JoyPad* joyPad){
    // toggle BUTTON_A AND BUTTON_B if TURBO_A and TURBO_B are set respectively
    joyPad->status ^= joyPad->status >> 8;
}
