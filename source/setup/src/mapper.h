#ifndef INCL_mapper__h
#define INCL_mapper__h

#include "BasicTypes.h"

typedef enum {
    NTSC = 0,
    DUAL,
    PAL,
    DENDY,
} TVSystem;

typedef enum {
    NO_MIRRORING,
    VERTICAL,
    HORIZONTAL,
    ONE_SCREEN,
    ONE_SCREEN_LOWER,
    ONE_SCREEN_UPPER,
    FOUR_SCREEN,
} Mirroring;

typedef enum {
    ARCHAIC_INES,
    INES,
    NES2,
} MapperFormat;

typedef struct Mapper {
    u8 *CHR_ROM;
    u8 *PRG_ROM;
    u8 *PRG_RAM;
    u8 *PRG_ptrs[8];
    u8 *CHR_ptrs[8];
    u8 PRG_regs[8];
    u8 CHR_regs[8];
    u16 PRG_banks;
    u16 CHR_banks;
    u32 CHR_RAM_size;
    u8 RAM_banks;
    u32 RAM_size;
    Mirroring mirroring;
    TVSystem type;
    MapperFormat format;
    u16 name_table_map[4];
    u32 clamp;
    u16 mapper_num;
    u8 submapper;
    u8 is_nsf;
    void (*on_scanline)(struct Mapper *);
    u8 (*read_ROM)(struct Mapper *, u16);
    void (*write_ROM)(struct Mapper *, u16, u8);
    u8 (*read_PRG)(struct Mapper *, u16);
    void (*write_PRG)(struct Mapper *, u16, u8);
    u8 (*read_CHR)(struct Mapper *, u16);
    void (*write_CHR)(struct Mapper *, u16, u8);
    void (*reset)(struct Mapper *);
} Mapper;

#define INES_HEADER_SIZE (16)

int load_data(Mapper *mapper);

#endif // INCL_mapper__h
