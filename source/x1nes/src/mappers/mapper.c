#include <stdlib.h>
#include <string.h>
//#include <SDL.h>

#include "mapper.h"

#include "../emulator.h"
//#include "../genie.h"
//#include "../utils.h"
//#include "../nsf.h"

extern Emulator gEmulator;

__sfr __banked __at(0x0B00) IoPortBank;

static int select_mapper(Mapper *mapper);
static void set_mapping(u16 tl, u16 tr, u16 bl, u16 br);

// generic mapper implementations
static u8 read_PRG(u16 address);
static void write_PRG(u16 address, u8 value);
static u8 read_CHR(u16 address);
static void write_CHR(u16 address, u8 value);
static u8 read_ROM(u16 address);
static void write_ROM(u16 address, u8 value);
static void on_scanline();

static int select_mapper(Mapper *mapper) {
    // load generic implementations
    mapper->read_PRG = read_PRG;
    mapper->write_PRG = write_PRG;
    mapper->read_CHR = read_CHR;
    mapper->write_CHR = write_CHR;
    mapper->read_ROM = read_ROM;
    mapper->write_ROM = write_ROM;
    mapper->on_scanline = on_scanline;
    mapper->clamp = ((u32)mapper->PRG_banks * 0x4000) - 1;

    switch (mapper->mapper_num) {
        case 0: return 0;
        case 1: return load_MMC1(mapper);
        case 2: return load_UXROM(mapper);
        case 3: return load_CNROM(mapper);
        case 4: return load_MMC3(mapper);
        case 7: return load_AOROM(mapper);
#if 0
        case 11: return load_colordreams(mapper);
        case 13: return load_CPROM(mapper);
        case 46: return load_colordreams46(mapper);
#endif
        case 66: return load_GNROM(mapper);
        case 75: return load_VRC1(mapper);
        case 94: return load_UN1ROM(mapper);
        case 180: return load_mapper180(mapper);
#if 0
        case 185: return load_mapper185(mapper);
#endif
        default:
            LOG(ERROR, "Mapper no %u not implemented", mapper->mapper_num);
            return -1;
    }
}

static void set_mapping(u16 tl, u16 tr, u16 bl, u16 br) {
    gEmulator.mapper.name_table_map[0] = tl;
    gEmulator.mapper.name_table_map[1] = tr;
    gEmulator.mapper.name_table_map[2] = bl;
    gEmulator.mapper.name_table_map[3] = br;
}

void set_mirroring(Mirroring mirroring) {
    if (mirroring == gEmulator.mapper.mirroring)
        return;
    switch (mirroring) {
        case HORIZONTAL:
            set_mapping(0, 0, 0x400, 0x400);
            LOG(DEBUG, "Using mirroring: Horizontal");
            break;
        case VERTICAL:
            set_mapping(0, 0x400, 0, 0x400);
            LOG(DEBUG, "Using mirroring: Vertical");
            break;
        case ONE_SCREEN_LOWER:
        case ONE_SCREEN:
            set_mapping(0, 0, 0, 0);
            LOG(DEBUG, "Using mirroring: Single screen lower");
            break;
        case ONE_SCREEN_UPPER:
            set_mapping(0x400, 0x400, 0x400, 0x400);
            LOG(DEBUG, "Using mirroring: Single screen upper");
            break;
        case FOUR_SCREEN:
            set_mapping(0, 0x400, 0x800, 0xC00);
            LOG(DEBUG, "Using mirroring: Four screen");
            break;
        default:
            set_mapping(0, 0, 0, 0);
            LOG(ERROR, "Unknown mirroring %u", mirroring);
    }
    gEmulator.mapper.mirroring = mirroring;
}

static void on_scanline() {
}

static u8 read_ROM(u16 address) {
    if (address < 0x6000) {
        // expansion rom
        LOG(DEBUG, "Attempted to read from unavailable expansion ROM");
        return gEmulator.mem.bus;
    }
    if (address < 0x8000) {
        // PRG ram
        if (gEmulator.mapper.PRG_RAM != nullptr)
            return gEmulator.mapper.PRG_RAM[address - 0x6000];

        LOG(DEBUG, "Attempted to read from non existent PRG RAM");
        return gEmulator.mem.bus;
    }

    // PRG
    return gEmulator.mapper.read_PRG(address);
}

static void write_ROM(u16 address, u8 value) {
    if (address < 0x6000) {
        LOG(DEBUG, "Attempted to write to unavailable expansion ROM");
        return;
    }

    if (address < 0x8000) {
        // extended ram
        if (gEmulator.mapper.PRG_RAM != nullptr)
            gEmulator.mapper.PRG_RAM[address - 0x6000] = value;
        else {
            LOG(DEBUG, "Attempted to write to non existent PRG RAM");
        }
        return;
    }

    // PRG
    gEmulator.mapper.write_PRG(address, value);
}

#if 0
static u8 read_PRG(Mapper *mapper, u16 address) {
//    return mapper->PRG_ROM[(address - 0x8000) & mapper->clamp];
    __asm__ ("di"); // 割り込み禁止
    IoPortBank = 0x00; // バンク0
    //u8 data = mapper->PRG_ROM[(address - 0x8000) & mapper->clamp];
    u8 data = *(u8*)((address - 0x8000) & mapper->clamp);
    IoPortBank = 0x10; // メインメモリ
    __asm__ ("ei"); // 割り込み許可
    return data;
}
#else

static u8 read_BANK0(u16 address) __naked
{
    (void)address;

    __asm
    ld  bc,#0x0B00
    di
    out (c),c

    ld  a,(hl)

    ld  l,#0x10
    out (c),l
    ei
    ret
    __endasm;
}

static u8 read_PRG(u16 address)
{
    //    return mapper->PRG_ROM[(address - 0x8000) & gEmulator.mapper.clamp];
    return read_BANK0((address - 0x8000) & gEmulator.mapper.clamp);
}
#endif

static void write_PRG(u16 address, u8 value) {
    LOG(DEBUG, "Attempted to write to PRG-ROM");
    (void)address;
    (void)value;
}

#if 0
static u8 read_CHR(Mapper *mapper, u16 address) {
//    return mapper->CHR_ROM[address];
    __asm__ ("di"); // 割り込み禁止
    IoPortBank = 0x01; // バンク1
    u8 data = mapper->CHR_ROM[address];
    IoPortBank = 0x10; // メインメモリ
    __asm__ ("ei"); // 割り込み許可
    return data;
}
#else
static u8 read_CHR(u16 address) __naked
{
    (void)address;

    // hl : address
    // return : a

    __asm
    ld	bc, #_IoPortBank
    ld	de, #0x1001
	di
	out	(c), e ; バンク1
    ld	a, (hl)
	out	(c), d ; メインメモリ
    ei

	ret
    __endasm;
}
#endif

static void write_CHR(u16 address, u8 value) {
    if (!gEmulator.mapper.CHR_RAM_size) {
        LOG(DEBUG, "Attempted to write to CHR-ROM");
        return;
    }
    __asm__ ("di"); // 割り込み禁止
    IoPortBank = 0x01; // バンク1
    //mapper->CHR_ROM[address] = value;
    *(u8*)address = value;
    IoPortBank = 0x10; // メインメモリ
    __asm__ ("ei"); // 割り込み許可
}

#if 0
static long long read_file_to_buffer(char *file_name, u8 **buffer) {
    SDL_IOStream *file = SDL_IOFromFile(file_name, "rb");

    if (file == nullptr) {
        LOG(ERROR, "file '%s' not found", file_name);
        return -1;
    }

    long long f_size = SDL_SeekIO(file, 0, SDL_IO_SEEK_END);
    if (f_size < 0) {
        LOG(ERROR, "Error reading length for file %s", file_name);
        return -1;
    }
    SDL_SeekIO(file, 0, SDL_IO_SEEK_SET);

    *buffer = malloc(f_size);
    if (*buffer == nullptr) {
        LOG(ERROR, "Error allocating memory for %s", file_name);
        return -1;
    }

    size_t input_bytes = SDL_ReadIO(file, *buffer, f_size);
    if (input_bytes != f_size) {
        free(*buffer);
        LOG(ERROR, "Error reading file %s", file_name);
        return -1;
    }

    return f_size;
}
#endif

void load_file(char *file_name, char *game_genie, Mapper *mapper) {
#if 0
    ROMData data = {0};
    long long rom_size = read_file_to_buffer(file_name, &data.rom);
    if (rom_size < 0) {
        quit(EXIT_FAILURE);
    }

    data.rom_size = rom_size;
    data.rom_name = file_name;

    if (game_genie != nullptr) {
        rom_size = read_file_to_buffer(game_genie, &data.genie_rom);
        if (rom_size < 0) {
            quit(EXIT_FAILURE);
        }
        data.genie_rom_size = rom_size;
    }

    const int result = load_data(&data, mapper);
    if (data.rom != nullptr)
        free(data.rom);
    if (data.genie_rom != nullptr)
        free(data.genie_rom);

    if (result < 0) {
        quit(EXIT_FAILURE);
    }
#else
    (void)file_name;
    (void)game_genie;
    (void)mapper;

    // ヘッダをEMMから読み込む
    u8 header[INES_HEADER_SIZE];
    x1_copyFromEmm0(header, 0, INES_HEADER_SIZE);

    ROMData data = {0};
    data.rom = header;       // BANK MEM 0
    data.rom_size = INES_HEADER_SIZE;
    data.rom_name = "DATA0.NES";
    load_data(&data, mapper);
#endif
}

int load_data(ROMData *rom_data, Mapper *mapper) {
    const u8 *header = rom_data->rom;
    u32 offset = 0;

    // clear mapper
    memset(mapper, 0, sizeof(Mapper));

#if 0
    if (strncmp((char *) header, "NESM\x1A", 5) == 0) {
#if defined(ENABLE_NSF) && ENABLE_NSF
        LOG(INFO, "Using NSF format");
        return load_nsf(rom_data, mapper);
#else
        LOG(INFO, "NSF format not support");
        return -1;
#endif
    }

    if (strncmp((char *) header, "NSFE", 4) == 0) {
#if defined(ENABLE_NSF) && ENABLE_NSF
        LOG(INFO, "Using NSFe format");
        return load_nsfe(rom_data, mapper);
#else
        LOG(INFO, "NSFe format not support");
        return -1;
#endif
    }

    if (strncmp((char *) header, "NES\x1A", 4) != 0) {
        LOG(ERROR, "unknown file format");
        return -1;
    }
#endif

    u8 mapnum = header[7] & 0x0C;
    if (mapnum == 0x08) {
        mapper->format = NES2;
        LOG(INFO, "Using NES2.0 format");
    }

    u8 last_4_zeros = 1;
    for (u8 i = 12; i < INES_HEADER_SIZE; i++) {
        if (header[i] != 0) {
            last_4_zeros = 0;
            break;
        }
    }

    if (mapnum == 0x00 && last_4_zeros) {
        mapper->format = INES;
        LOG(INFO, "Using iNES format");
    } else if (mapnum == 0x04) {
        mapper->format = ARCHAIC_INES;
        LOG(INFO, "Using iNES (archaic) format");
    } else if (mapnum != 0x08) {
        mapper->format = ARCHAIC_INES;
        LOG(INFO, "Possibly using iNES (archaic) format");
    }

    mapper->PRG_banks = header[4];
    mapper->CHR_banks = header[5];

    if (header[6] & (1<<1)) {
        LOG(INFO, "Uses Battery backed save RAM 8KB");
    }

    if (header[6] & (1<<2)) {
        LOG(ERROR, "Trainer not supported");
        return -1;
    }

    Mirroring mirroring;

    if (header[6] & (1<<3)) {
        mirroring = FOUR_SCREEN;
    } else if (header[6] & (1<<0)) {
        mirroring = VERTICAL;
    } else {
        mirroring = HORIZONTAL;
    }
    mapper->mapper_num = (header[6] & 0xF0) >> 4;
    mapper->type = NTSC;

    if (mapper->format == INES) {
        mapper->mapper_num |= header[7] & 0xF0;
        mapper->RAM_banks = header[8];

        if (mapper->RAM_banks) {
            mapper->RAM_size = (u32)0x2000 * (u32)mapper->RAM_banks;
            LOG(INFO, "PRG RAM Banks (8kb): %u", mapper->RAM_banks);
        }

        if (header[9] & 1) {
            mapper->type = PAL;
        } else {
            mapper->type = NTSC;
        }
    } else if (mapper->format == NES2) {
        mapper->mapper_num |= header[7] & 0xF0;
        mapper->mapper_num |= ((header[8] & 0xf) << 8);
        mapper->submapper = header[8] >> 4;

        mapper->PRG_banks |= ((header[9] & 0x0f) << 8);
        mapper->CHR_banks |= ((header[9] & 0xf0) << 4);

        if (header[10] & 0xf) {
            mapper->RAM_size = ((u32)64 << (header[10] & 0xf));
        }
        if (header[10] & 0xf0) {
            mapper->RAM_size += ((u32)64 << ((header[10] & 0xf0) >> 4));
        }
        if (mapper->RAM_size) {
            LOG(INFO, "PRG-RAM size: %u", mapper->RAM_size);
        }

        if (header[11] & 0xf) {
            mapper->CHR_RAM_size = ((u32)64 << (header[11] & 0xf));
        }
        if (header[11] & 0xf0) {
            mapper->CHR_RAM_size += ((u32)64 << ((header[11] & 0xf0) >> 4));
        }
        if (mapper->CHR_RAM_size) {
            LOG(INFO, "CHR-RAM size: %u", mapper->CHR_RAM_size);
        }

        switch (header[12] & 0x3) {
            case 0:
                mapper->type = NTSC;
                break;
            case 1:
                mapper->type = PAL;
                break;
            case 2:
                // multi-region
                mapper->type = DUAL;
                break;
            case 3:
                mapper->type = DENDY;
                LOG(ERROR, "Dendy ROM not supported");
                return -1;
            default:
                break;
        }
    }

    if (!mapper->RAM_banks && mapper->format != NES2) {
        LOG(INFO, "PRG RAM Banks (8kb): Not specified, Assuming 8kb");
        mapper->RAM_size = 0x2000;
    }

    if (mapper->RAM_size) {
#if 0
        mapper->PRG_RAM = malloc(mapper->RAM_size);
        memset(mapper->PRG_RAM, 0, mapper->RAM_size);
#else
        // @todo
        mapper->PRG_RAM = (u8*)0x6000;
#endif
    }

    if (mapper->format != NES2) {
        if (!mapper->CHR_banks) {
            mapper->CHR_RAM_size = 0x2000;
            LOG(INFO, "CHR-ROM Not specified, Assuming 8kb CHR-RAM");
        }
#if 0
        if (rom_data->rom_name != nullptr) {
            if (strstr(rom_data->rom_name, "(E)") != nullptr && mapper->type == NTSC) {
                // probably PAL ROM
                mapper->type = PAL;
            }
        }
#endif
    }

    LOG(INFO, "PRG banks (16KB): %u", mapper->PRG_banks);
    LOG(INFO, "CHR banks (8KB): %u", mapper->CHR_banks);

#if 0
    mapper->PRG_ROM = malloc(0x4000 * mapper->PRG_banks);
    memcpy(mapper->PRG_ROM, rom_data->rom + offset, 0x4000 * mapper->PRG_banks);
    offset += 0x4000 * mapper->PRG_banks;

    if (mapper->CHR_banks) {
        mapper->CHR_ROM = malloc(0x2000 * mapper->CHR_banks);
        memcpy(mapper->CHR_ROM, rom_data->rom + offset, 0x2000 * mapper->CHR_banks);
        // offset += 0x2000 * mapper->CHR_banks;
    } else {
        if (!mapper->CHR_RAM_size) {
            LOG(INFO, "No CHR-RAM or CHR-ROM specified, Using 8kb CHR-RAM");
            mapper->CHR_RAM_size = 0x2000;
        }
        mapper->CHR_ROM = malloc(mapper->CHR_RAM_size);
        memset(mapper->CHR_ROM, 0, mapper->CHR_RAM_size);
    }
#else
    mapper->PRG_ROM.ptr32 = 0;
    mapper->CHR_ROM.ptr32 = 0;
    if (mapper->CHR_banks) {
    } else {
        if (!mapper->CHR_RAM_size) {
            LOG(INFO, "No CHR-RAM or CHR-ROM specified, Using 8kb CHR-RAM");
            mapper->CHR_RAM_size = 0x2000;
        }
    }
#endif

    switch (mapper->type) {
        case NTSC:
            LOG(INFO, "ROM type: NTSC");
            break;
        case DUAL:
            LOG(INFO, "ROM type: DUAL (Using NTSC)");
            mapper->type = NTSC;
            break;
        case PAL:
            LOG(INFO, "ROM type: PAL");
            break;
        default:
            LOG(INFO, "ROM type: Unknown");
    }

    LOG(INFO, "Using mapper #%d: sub-mapper #%d", mapper->mapper_num, mapper->submapper);

    if (select_mapper(mapper) < 0)
        return -1;
    set_mirroring(mirroring);

    if (rom_data->genie_rom != nullptr) {
        LOG(INFO, "-------- Game Genie Cartridge info ---------");
#if 0
        return load_genie(rom_data, mapper);
#endif
    }
    return 0;
}

void free_mapper(Mapper *mapper) {
#if 0
    if (mapper->PRG_ROM != nullptr)
        free(mapper->PRG_ROM);
    if (mapper->CHR_ROM != nullptr)
        free(mapper->CHR_ROM);
    if (mapper->PRG_RAM != nullptr)
        free(mapper->PRG_RAM);
    if (mapper->extension != nullptr)
        free(mapper->extension);
    if (mapper->genie != nullptr)
        free(mapper->genie);
#endif
#if defined(ENABLE_NSF) && ENABLE_NSF
    if (mapper->NSF != nullptr) {
        free_NSF(mapper->NSF);
    }
#endif
    (void)mapper;
    LOG(DEBUG, "Mapper cleanup complete");
}
