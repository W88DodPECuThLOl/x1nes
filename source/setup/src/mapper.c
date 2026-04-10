#include "mapper.h"
#include "log.h"
#include <string.h>

static int
select_CNROM(Mapper *mapper)
{
    // PRG : BANK0
    // CHR : BANK1 ～32KiBまで
    //       EMM0  32KiBオーバー

    //
    // プログラム部分
    //
    if(mapper->PRG_banks > 2) {
        LOG(ERROR, "The program size is too large.");
        return -1;
    }
    u32 emmAddress = INES_HEADER_SIZE;
    u32 copySize = (u32)mapper->PRG_banks * 0x4000;
    x1_copyToBank0FromEmm0(0x0000, emmAddress, copySize);
    LOG(INFO, "BANK0  PRG  $0000-$%04X", (u16)(copySize - 1));
    emmAddress += copySize;
    //
    // キャラクタ部分
    //
    copySize = (u32)mapper->CHR_banks * 0x2000;
    if(mapper->CHR_banks <= 4) {
        x1_copyToBank1FromEmm0(0x0000, emmAddress, copySize);
        LOG(INFO, "BANK1  CHR  $0000-$%04X", (u16)(copySize - 1));
    } else {
        u32 endAddress = emmAddress + copySize - 1;
        LOG(INFO, "EMM0   CHR  $%02X:%04X-$%02X:%04X",
            (u16)(emmAddress >> 16), (u16)emmAddress,
            (u16)(endAddress >> 16), (u16)endAddress);
    }
    if(mapper->CHR_banks > 16) {
        LOG(ERROR, "The character size is too large.");
        return -1;
    }
    return 0;
}

static int
select_MMC1_MMC3_GNROM_VRC1(Mapper *mapper)
{
    // PRG : EMM0
    // CHR : EMM0

    //
    // プログラム部分
    //
    u32 emmAddress = INES_HEADER_SIZE;
    u32 copySize = (u32)mapper->PRG_banks * 0x4000;
    u32 endAddress = emmAddress + copySize - 1;
    LOG(INFO, "EMM0   PRG  $%02X:%04X-$%02X:%04X",
        (u16)(emmAddress >> 16), (u16)emmAddress,
        (u16)(endAddress >> 16), (u16)endAddress);
    emmAddress += copySize;
    //
    // キャラクタ部分
    //
    copySize = (u32)mapper->CHR_banks * 0x2000;
    endAddress = emmAddress + copySize - 1;
    LOG(INFO, "EMM0   CHR  $%02X:%04X-$%02X:%04X",
        (u16)(emmAddress >> 16), (u16)emmAddress,
        (u16)(endAddress >> 16), (u16)endAddress);

    if(endAddress >= (u32)320*1024) {
        LOG(ERROR, "Out of EMM.");
        return -1;
    }

    return 0;
}

static int
select_UXROM_UN1ROM_mapper180(Mapper *mapper)
{
    // PRG : EMM0  ～256KiB
    // CHR : BANK1

    //
    // プログラム部分
    //
    if(mapper->PRG_banks > 16) {
        LOG(ERROR, "The program size is too large.");
        return -1;
    }
    u32 emmAddress = INES_HEADER_SIZE;
    u32 copySize = (u32)mapper->PRG_banks * 0x4000;
    u32 endAddress = emmAddress + copySize - 1;
    LOG(INFO, "EMM0   PRG  $%02X:%04X-$%02X:%04X",
        (u16)(emmAddress >> 16), (u16)emmAddress,
        (u16)(endAddress >> 16), (u16)endAddress);
    emmAddress += copySize;
    //
    // キャラクタ部分
    //
    copySize = (u32)mapper->CHR_banks * 0x2000;
    if(mapper->CHR_banks <= 4) {
        x1_copyToBank1FromEmm0(0x0000, emmAddress, copySize);
        LOG(INFO, "BANK1  CHR  $0000-$%04X", (u16)(copySize - 1));
    } else {
        LOG(ERROR, "The character size is too large.");
        return -1;
    }
    return 0;
}

static int
select_AORPM(Mapper *mapper)
{
    // PRG : EMM0
    // CHR : BANK1

    //
    // プログラム部分
    //
    u32 emmAddress = INES_HEADER_SIZE;
    u32 copySize = (u32)mapper->PRG_banks * 0x4000;
    u32 endAddress = emmAddress + copySize - 1;
    LOG(INFO, "EMM0   CHR  $%02X:%04X-$%02X:%04X",
        (u16)(emmAddress >> 16), (u16)emmAddress,
        (u16)(endAddress >> 16), (u16)endAddress);
    emmAddress += copySize;
    //
    // キャラクタ部分
    //
    copySize = (u32)mapper->CHR_banks * 0x2000;
    if(mapper->CHR_banks <= 4) {
        x1_copyToBank1FromEmm0(0x0000, emmAddress, copySize);
        LOG(INFO, "BANK1  CHR  $0000-$%04X", (u16)(copySize - 1));
    } else {
        LOG(ERROR, "The character size is too large.");
        return -1;
    }
    return 0;
}

static int
select_mapper(Mapper *mapper)
{
    switch(mapper->mapper_num) {
        case 0:
        case 3: // CNROM
            return select_CNROM(mapper);
        case 1: // MMC1
        case 4: // MMC3
        case 66: // GNROM
        case 75: // VRC1
            return select_MMC1_MMC3_GNROM_VRC1(mapper);
        case 2: // UXROM
        case 94: // UN1ROM
        case 180: // mapper180
            return select_UXROM_UN1ROM_mapper180(mapper);
        case 7: // AOROM
            return select_AORPM(mapper);
    }
    return -1;
}

int
load_data(Mapper *mapper)
{
    u8 header[INES_HEADER_SIZE];
    x1_copyFromEmm0(header, 0, INES_HEADER_SIZE);
    u32 offset = 0;

    if (strncmp((char *) header, "NESM\x1A", 5) == 0) {
        LOG(INFO, "NSF format not support");
        return -1;
    }
    if (strncmp((char *) header, "NSFE", 4) == 0) {
        LOG(INFO, "NSFe format not support");
        return -1;
    }
    if (strncmp((char *) header, "NES\x1A", 4) != 0) {
        LOG(ERROR, "unknown file format");
        return -1;
    }

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
        if(mapper->RAM_size > 0x2000) {
            LOG(ERROR, "The PRG RAM size is too large.");
            return -1;
        }
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

    return 0;
}
