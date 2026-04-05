#include <catZ80Lib.h>
#include "mapper.h"

typedef  u16 PixelFormat;

#define TO_PIXEL_FORMAT(ARGB8888) ARGB8888

static PixelFormat nes_palette[64] = {
    TO_PIXEL_FORMAT(0x666), TO_PIXEL_FORMAT(0x028), TO_PIXEL_FORMAT(0x11a), TO_PIXEL_FORMAT(0x30a), TO_PIXEL_FORMAT(0x507), TO_PIXEL_FORMAT(0x604), TO_PIXEL_FORMAT(0x600), TO_PIXEL_FORMAT(0x510),
    TO_PIXEL_FORMAT(0x330), TO_PIXEL_FORMAT(0x040), TO_PIXEL_FORMAT(0x050), TO_PIXEL_FORMAT(0x040), TO_PIXEL_FORMAT(0x044), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000),
    TO_PIXEL_FORMAT(0xaaa), TO_PIXEL_FORMAT(0x15d), TO_PIXEL_FORMAT(0x44f), TO_PIXEL_FORMAT(0x72f), TO_PIXEL_FORMAT(0xa1c), TO_PIXEL_FORMAT(0xb17), TO_PIXEL_FORMAT(0xb32), TO_PIXEL_FORMAT(0x940),
    TO_PIXEL_FORMAT(0x660), TO_PIXEL_FORMAT(0x380), TO_PIXEL_FORMAT(0x090), TO_PIXEL_FORMAT(0x083), TO_PIXEL_FORMAT(0x078), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000),
    TO_PIXEL_FORMAT(0xfff), TO_PIXEL_FORMAT(0x6bf), TO_PIXEL_FORMAT(0x99f), TO_PIXEL_FORMAT(0xc7f), TO_PIXEL_FORMAT(0xf6f), TO_PIXEL_FORMAT(0xf6c), TO_PIXEL_FORMAT(0xf87), TO_PIXEL_FORMAT(0xe92),
    TO_PIXEL_FORMAT(0xbb0), TO_PIXEL_FORMAT(0x8d0), TO_PIXEL_FORMAT(0x5e3), TO_PIXEL_FORMAT(0x4e8), TO_PIXEL_FORMAT(0x4cd), TO_PIXEL_FORMAT(0x444), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000),
    TO_PIXEL_FORMAT(0xfff), TO_PIXEL_FORMAT(0xcdf), TO_PIXEL_FORMAT(0xddf), TO_PIXEL_FORMAT(0xecf), TO_PIXEL_FORMAT(0xfcf), TO_PIXEL_FORMAT(0xfce), TO_PIXEL_FORMAT(0xfcc), TO_PIXEL_FORMAT(0xfda),
    TO_PIXEL_FORMAT(0xee9), TO_PIXEL_FORMAT(0xce9), TO_PIXEL_FORMAT(0xbfa), TO_PIXEL_FORMAT(0xbfc), TO_PIXEL_FORMAT(0xbef), TO_PIXEL_FORMAT(0xbbb), TO_PIXEL_FORMAT(0x000), TO_PIXEL_FORMAT(0x000),
};

void
setupPalette()
{
    // 0bit   bank1 0x4400
    // 1bit   bank1 0x4000
    for(u8 i = 0; i < 64; ++i) {
		u8 b = i & 3;
		u8 r = (i >> 2) & 3;
		u8 g = (i >> 4) & 3;
		u16 color = nes_palette[i];

        u16 cr = (color & 0xF00) >> 8;
        u16 cg = (color & 0x0F0) >> 4;
        u16 cb = (color & 0x00F);
        color = (cr << 4) | (cg << 8) | (cb);

		u16 index = (u16)b | ((u16)r << 4) | ((u16)g << 8);

        //GRB
        x1_setPaletteZ(index, color);
		index <<= 2;
		x1_setPaletteZ(index, color);
	}
}

Mapper mapper;

u8 main()
{
    sos_msx("Neko Setup\r");

    if(load_data(&mapper) < 0) {
        return 14; // Bad Data
    }

    // 画面設定
    outp(0x1FD0, 0x02); // 200ラインモード、2本ラスタ/ドット、バンク0表示、バンク0アクセス
	outp(0x1FB0, 0x80); // 多色モード、4096色1画面モード
	outp(0x1FC0, 0x00); // 
//	outp(0x1FB0, 0x90); // 多色モード、64色2画面モード
//	outp(0x1FC0, 0x10); // 

	// パレット設定
    setupPalette();

    // バンク1
	outp(0x1FD0, 0x12); // 200ラインモード、2本ラスタ/ドット、バンク1アクセス

	return 0;
}
