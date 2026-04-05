#ifndef INCL_mapper_h
#define INCL_mapper_h

#include "../dev/BasicTypes.h"

#define INES_HEADER_SIZE (16)

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

struct Genie;
struct Emulator;
struct NSF;

typedef struct ROMData {
    const char *rom_name;
    u8 *rom;
    u8 *genie_rom;
    u64 rom_size;
    u64 genie_rom_size;
} ROMData;

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
//    u16 clamp;
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

    // mapper extension structs would be attached here
    // memory should be allocated dynamically and should
    // not be freed since this is done by the generic mapper functions
    void *extension;
    // pointer to game genie if any
    struct Genie *genie;
    struct NSF *NSF;
    struct Emulator *emulator;
} Mapper;

void load_file(char *file_name, char *game_genie, Mapper *mapper);
int load_data(ROMData *data, Mapper *mapper);
void free_mapper(Mapper *mapper);
void set_mirroring(Mapper *mapper, Mirroring mirroring);

// mapper specifics

int load_UXROM(Mapper *mapper);
int load_MMC1(Mapper *mapper);
int load_CNROM(Mapper *mapper);
int load_GNROM(Mapper *mapper);
int load_AOROM(Mapper *mapper);
int load_MMC3(Mapper *mapper);
int load_colordreams(Mapper *mapper);
int load_colordreams46(Mapper *mapper);
int load_VRC1(Mapper *mapper);
int load_UN1ROM(Mapper *mapper);
int load_mapper180(Mapper *mapper);
int load_mapper185(Mapper *mapper);
int load_CPROM(Mapper *mapper);

#endif // INCL_mapper_h
