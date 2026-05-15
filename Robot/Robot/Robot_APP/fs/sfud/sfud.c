/*
 * This file is part of the Serial Flash Universal Driver Library.
 *
 * Copyright (c) 2016-2018, Armink, <armink.ztl@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * 'Software'), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Function: serial flash operate functions by SFUD lib.
 * Created on: 2016-04-23
 */

#include "sfud.h"
#include <string.h>
#include "lfs.h"
#include <time.h>

/* send dummy data for read data */
#define DUMMY_DATA                               0xFF

#ifndef SFUD_FLASH_DEVICE_TABLE
#error "Please configure the flash device information table in (in sfud_cfg.h)."
#endif

/* user configured flash device information table */
static sfud_flash flash_table[] = SFUD_FLASH_DEVICE_TABLE;
/* supported manufacturer information table */
static const sfud_mf mf_table[] = SFUD_MF_TABLE;

#ifdef SFUD_USING_FLASH_INFO_TABLE
/* supported flash chip information table */
static const sfud_flash_chip flash_chip_table[] = SFUD_FLASH_CHIP_TABLE;
#endif

#ifdef SFUD_USING_QSPI
/**
 * flash read data mode
 */
enum sfud_qspi_read_mode {
    NORMAL_SPI_READ = 1 << 0,               /**< mormal spi read mode */
    DUAL_OUTPUT = 1 << 1,                   /**< qspi fast read dual output */
    DUAL_IO = 1 << 2,                       /**< qspi fast read dual input/output */
    QUAD_OUTPUT = 1 << 3,                   /**< qspi fast read quad output */
    QUAD_IO = 1 << 4,                       /**< qspi fast read quad input/output */
};

/* QSPI flash chip's extended information table */
static const sfud_qspi_flash_ext_info qspi_flash_ext_info_table[] = SFUD_FLASH_EXT_INFO_TABLE;
#endif /* SFUD_USING_QSPI */

static sfud_err software_init(const sfud_flash *flash);
static sfud_err hardware_init(sfud_flash *flash);
static sfud_err page256_or_1_byte_write(const sfud_flash *flash, uint32_t addr, size_t size, uint16_t write_gran,
        const uint8_t *data);
static sfud_err aai_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data);
static sfud_err wait_busy(const sfud_flash *flash);
static sfud_err reset(const sfud_flash *flash);
static sfud_err read_jedec_id(sfud_flash *flash);
static sfud_err set_write_enabled(const sfud_flash *flash, bool enabled);
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled);
static void make_adress_byte_array(const sfud_flash *flash, uint32_t addr, uint8_t *array);

/* ../port/sfup_port.c */
extern void sfud_log_debug(const char *file, const long line, const char *format, ...);
extern void sfud_log_info(const char *format, ...);

uint8_t MonthOfDays[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
/**
 * SFUD initialize by flash device
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_device_init(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    /* hardware initialize */
    result = hardware_init(flash);
    if (result == SFUD_SUCCESS) {
        result = software_init(flash);
    }
    if (result == SFUD_SUCCESS) {
        flash->init_ok = true;
        SFUD_INFO("%s flash device is initialize success.", flash->name);
    } else {
        flash->init_ok = false;
        SFUD_INFO("Error: %s flash device is initialize fail.", flash->name);
    }

    return result;
}

/**
 * SFUD library initialize.
 *
 * @return result
 */
sfud_err sfud_init(void) {
    sfud_err cur_flash_result = SFUD_SUCCESS, all_flash_result = SFUD_SUCCESS;
    size_t i;

    SFUD_DEBUG("Start initialize Serial Flash Universal Driver(SFUD) V%s.", SFUD_SW_VERSION);
    SFUD_DEBUG("You can get the latest version on https://github.com/armink/SFUD .");
    /* initialize all flash device in flash device table */
    for (i = 0; i < sizeof(flash_table) / sizeof(sfud_flash); i++) {
        /* initialize flash device index of flash device information table */
        flash_table[i].index = i;
        cur_flash_result = sfud_device_init(&flash_table[i]);

        if (cur_flash_result != SFUD_SUCCESS) {
            all_flash_result = cur_flash_result;
        }
    }

    return all_flash_result;
}

/**
 * get flash device by its index which in the flash information table
 *
 * @param index the index which in the flash information table  @see flash_table
 *
 * @return flash device
 */
sfud_flash *sfud_get_device(size_t index) {
    if (index < sfud_get_device_num()) {
        return &flash_table[index];
    } else {
        return NULL;
    }
}

/**
 * get flash device total number on flash device information table  @see flash_table
 *
 * @return flash device total number
 */
size_t sfud_get_device_num(void) {
    return sizeof(flash_table) / sizeof(sfud_flash);
}

/**
 * get flash device information table  @see flash_table
 *
 * @return flash device table pointer
 */
const sfud_flash *sfud_get_device_table(void) {
    return flash_table;
}

#ifdef SFUD_USING_QSPI
static void qspi_set_read_cmd_format(sfud_flash *flash, uint8_t ins, uint8_t ins_lines, uint8_t addr_lines,
        uint8_t dummy_cycles, uint8_t data_lines) {
    /* if medium size greater than 16Mb, use 4-Byte address, instruction should be added one */
    if (flash->chip.capacity <= 0x1000000) {
        flash->read_cmd_format.instruction = ins;
        flash->read_cmd_format.address_size = 24;
    } else {
        flash->read_cmd_format.instruction = ins + 1;
        flash->read_cmd_format.address_size = 32;
    }

    flash->read_cmd_format.instruction_lines = ins_lines;
    flash->read_cmd_format.address_lines = addr_lines;
    flash->read_cmd_format.alternate_bytes_lines = 0;
    flash->read_cmd_format.dummy_cycles = dummy_cycles;
    flash->read_cmd_format.data_lines = data_lines;
}

/**
 * Enbale the fast read mode in QSPI flash mode. Default read mode is normal SPI mode.
 *
 * it will find the appropriate fast-read instruction to replace the read instruction(0x03)
 * fast-read instruction @see SFUD_FLASH_EXT_INFO_TABLE
 *
 * @note When Flash is in QSPI mode, the method must be called after sfud_device_init().
 *
 * @param flash flash device
 * @param data_line_width the data lines max width which QSPI bus supported, such as 1, 2, 4
 *
 * @return result
 */
sfud_err sfud_qspi_fast_read_enable(sfud_flash *flash, uint8_t data_line_width) {
    size_t i = 0;
    uint8_t read_mode = NORMAL_SPI_READ;
    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data_line_width == 1 || data_line_width == 2 || data_line_width == 4);

    /* get read_mode, If don't found, the default is SFUD_QSPI_NORMAL_SPI_READ */
    for (i = 0; i < sizeof(qspi_flash_ext_info_table) / sizeof(sfud_qspi_flash_ext_info); i++) {
        if ((qspi_flash_ext_info_table[i].mf_id == flash->chip.mf_id)
                && (qspi_flash_ext_info_table[i].type_id == flash->chip.type_id)
                && (qspi_flash_ext_info_table[i].capacity_id == flash->chip.capacity_id)) {
            read_mode = qspi_flash_ext_info_table[i].read_mode;
        }
    }

    /* determine qspi supports which read mode and set read_cmd_format struct */
    switch (data_line_width) {
    case 1:
        qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        break;
    case 2:
        if (read_mode & DUAL_IO) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_DUAL_IO_READ_DATA, 1, 2, 8, 2);
        } else if (read_mode & DUAL_OUTPUT) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_DUAL_OUTPUT_READ_DATA, 1, 1, 8, 2);
        } else {
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        }
        break;
    case 4:
        if (read_mode & QUAD_IO) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_QUAD_IO_READ_DATA, 1, 4, 6, 4);
        } else if (read_mode & QUAD_OUTPUT) {
            qspi_set_read_cmd_format(flash, SFUD_CMD_QUAD_OUTPUT_READ_DATA, 1, 1, 8, 4);
        } else {
            qspi_set_read_cmd_format(flash, SFUD_CMD_READ_DATA, 1, 1, 0, 1);
        }
        break;
    }

    return result;
}
#endif /* SFUD_USING_QSPI */

/**
 * hardware initialize
 */
static sfud_err hardware_init(sfud_flash *flash) {
    extern sfud_err sfud_spi_port_init(sfud_flash * flash);

    sfud_err result = SFUD_SUCCESS;
    size_t i;

    SFUD_ASSERT(flash);

    result = sfud_spi_port_init(flash);
    if (result != SFUD_SUCCESS) {
        return result;
    }

#ifdef SFUD_USING_QSPI
    /* set default read instruction */
    flash->read_cmd_format.instruction = SFUD_CMD_READ_DATA;
#endif /* SFUD_USING_QSPI */

    /* SPI write read function must be initialize */
    SFUD_ASSERT(flash->spi.wr);
    /* if the user don't configure flash chip information then using SFDP parameter or static flash parameter table */
    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 || flash->chip.erase_gran == 0
            || flash->chip.erase_gran_cmd == 0) {
        /* read JEDEC ID include manufacturer ID, memory type ID and flash capacity ID */
        result = read_jedec_id(flash);
        if (result != SFUD_SUCCESS) {
            return result;
        }

#ifdef SFUD_USING_SFDP
        extern bool sfud_read_sfdp(sfud_flash *flash);
        /* read SFDP parameters */
        if (sfud_read_sfdp(flash)) {
            flash->chip.name = NULL;
            flash->chip.capacity = flash->sfdp.capacity;
            /* only 1 byte or 256 bytes write mode for SFDP */
            if (flash->sfdp.write_gran == 1) {
                flash->chip.write_mode = SFUD_WM_BYTE;
            } else {
                flash->chip.write_mode = SFUD_WM_PAGE_256B;
            }
            /* find the the smallest erase sector size for eraser. then will use this size for erase granularity */
            flash->chip.erase_gran = flash->sfdp.eraser[0].size;
            flash->chip.erase_gran_cmd = flash->sfdp.eraser[0].cmd;
            for (i = 1; i < SFUD_SFDP_ERASE_TYPE_MAX_NUM; i++) {
                if (flash->sfdp.eraser[i].size != 0 && flash->chip.erase_gran > flash->sfdp.eraser[i].size) {
                    flash->chip.erase_gran = flash->sfdp.eraser[i].size;
                    flash->chip.erase_gran_cmd = flash->sfdp.eraser[i].cmd;
                }
            }
        } else {
#endif

#ifdef SFUD_USING_FLASH_INFO_TABLE
            /* read SFDP parameters failed then using SFUD library provided static parameter */
            for (i = 0; i < sizeof(flash_chip_table) / sizeof(sfud_flash_chip); i++) {
                if ((flash_chip_table[i].mf_id == flash->chip.mf_id)
                        && (flash_chip_table[i].type_id == flash->chip.type_id)
                        && (flash_chip_table[i].capacity_id == flash->chip.capacity_id)) {
                    flash->chip.name = flash_chip_table[i].name;
                    flash->chip.capacity = flash_chip_table[i].capacity;
                    flash->chip.write_mode = flash_chip_table[i].write_mode;
                    flash->chip.erase_gran = flash_chip_table[i].erase_gran;
                    flash->chip.erase_gran_cmd = flash_chip_table[i].erase_gran_cmd;
                    break;
                }
            }
#endif

#ifdef SFUD_USING_SFDP
        }
#endif

    }

    if (flash->chip.capacity == 0 || flash->chip.write_mode == 0 || flash->chip.erase_gran == 0
            || flash->chip.erase_gran_cmd == 0) {
        SFUD_INFO("Warning: This flash device is not found or not support.");
        return SFUD_ERR_NOT_FOUND;
    } else {
        const char *flash_mf_name = NULL;
        /* find the manufacturer information */
        for (i = 0; i < sizeof(mf_table) / sizeof(sfud_mf); i++) {
            if (mf_table[i].id == flash->chip.mf_id) {
                flash_mf_name = mf_table[i].name;
                break;
            }
        }
        /* print manufacturer and flash chip name */
        if (flash_mf_name && flash->chip.name) {
            SFUD_INFO("Find a %s %s flash chip. Size is 0x%lx bytes.", flash_mf_name, flash->chip.name,
                    flash->chip.capacity);
        } else if (flash_mf_name) {
            SFUD_INFO("Find a %s flash chip. Size is 0x%lx bytes.", flash_mf_name, flash->chip.capacity);
        } else {
            SFUD_INFO("Find a flash chip. Size is 0x%lx bytes.", flash->chip.capacity);
        }
    }

    /* reset flash device */
    result = reset(flash);
    if (result != SFUD_SUCCESS) {
        return result;
    }

    /* I found when the flash write mode is supported AAI mode. The flash all blocks is protected,
     * so need change the flash status to unprotected before write and erase operate. */
    if (flash->chip.write_mode & SFUD_WM_AAI) {
        result = sfud_write_status(flash, true, 0x00);
        if (result != SFUD_SUCCESS) {
            return result;
        }
    }

    /* if the flash is large than 16MB (256Mb) then enter in 4-Byte addressing mode */
    if (flash->chip.capacity > (1L << 24)) {
        result = set_4_byte_address_mode(flash, true);
    } else {
        flash->addr_in_4_byte = false;
    }

    return result;
}

/**
 * software initialize
 *
 * @param flash flash device
 *
 * @return result
 */
static sfud_err software_init(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;

    SFUD_ASSERT(flash);

    return result;
}

/**
 * read flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size read size
 * @param data read data pointer
 *
 * @return result
 */
sfud_err sfud_read(const sfud_flash *flash, uint32_t addr, size_t size, uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(data);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    result = wait_busy(flash);

    if (result == SFUD_SUCCESS) {
#ifdef SFUD_USING_QSPI
        if (flash->read_cmd_format.instruction != SFUD_CMD_READ_DATA) {
            result = spi->qspi_read(spi, addr, (sfud_qspi_read_cmd_format *)&flash->read_cmd_format, data, size);
        } else
#endif
        {
            cmd_data[0] = SFUD_CMD_READ_DATA;
            make_adress_byte_array(flash, addr, &cmd_data[1]);
            cmd_size = flash->addr_in_4_byte ? 5 : 4;
            result = spi->wr(spi, cmd_data, cmd_size, data, size);
        }
    }
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * erase all flash data
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_chip_erase(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[4];

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }

    cmd_data[0] = SFUD_CMD_ERASE_CHIP;
    /* dual-buffer write, like AT45DB series flash chip erase operate is different for other flash */
    if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER) {
        cmd_data[1] = 0x94;
        cmd_data[2] = 0x80;
        cmd_data[3] = 0x9A;
        result = spi->wr(spi, cmd_data, 4, NULL, 0);
    } else {
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    }
    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Flash chip erase SPI communicate error.");
        goto __exit;
    }
    result = wait_busy(flash);

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * erase all flash data
 *
 * @param flash flash device
 *
 * @return result
 */
sfud_err sfud_sector_erase(const sfud_flash *flash, uint32_t sector) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[4];

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }

    cmd_data[0] = SFUD_SECTOR_EREASE;
    cmd_data[1] = sector >> 16;
    cmd_data[2] = sector >> 8;
    cmd_data[3] = sector;
    result = spi->wr(spi, cmd_data, 4, NULL, 0);

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Flash chip erase SPI communicate error.");
        goto __exit;
    }
    result = wait_busy(flash);

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}


/**
 * erase flash data
 *
 * @note It will erase align by erase granularity.
 *
 * @param flash flash device
 * @param addr start address
 * @param size erase size
 *
 * @return result
 */
sfud_err sfud_erase(const sfud_flash *flash, uint32_t addr, size_t size) {
    extern size_t sfud_sfdp_get_suitable_eraser(const sfud_flash *flash, uint32_t addr, size_t erase_size);

    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[5], cmd_size, cur_erase_cmd;
    size_t cur_erase_size;

    SFUD_ASSERT(flash);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }

    if (addr == 0 && size == flash->chip.capacity) {
        return sfud_chip_erase(flash);
    }

    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* loop erase operate. erase unit is erase granularity */
    while (size) {
        /* if this flash is support SFDP parameter, then used SFDP parameter supplies eraser */
#ifdef SFUD_USING_SFDP
        size_t eraser_index;
        if (flash->sfdp.available) {
            /* get the suitable eraser for erase process from SFDP parameter */
            eraser_index = sfud_sfdp_get_suitable_eraser(flash, addr, size);
            cur_erase_cmd = flash->sfdp.eraser[eraser_index].cmd;
            cur_erase_size = flash->sfdp.eraser[eraser_index].size;
        } else {
#else
        {
#endif
            cur_erase_cmd = flash->chip.erase_gran_cmd;
            cur_erase_size = flash->chip.erase_gran;
        }
        /* set the flash write enable */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }

        cmd_data[0] = cur_erase_cmd;
        make_adress_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;
        result = spi->wr(spi, cmd_data, cmd_size, NULL, 0);
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash erase SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        /* make erase align and calculate next erase address */
        if (addr % cur_erase_size != 0) {
            if (size > cur_erase_size - (addr % cur_erase_size)) {
                size -= cur_erase_size - (addr % cur_erase_size);
                addr += cur_erase_size - (addr % cur_erase_size);
            } else {
                goto __exit;
            }
        } else {
            if (size > cur_erase_size) {
                size -= cur_erase_size;
                addr += cur_erase_size;
            } else {
                goto __exit;
            }
        }
    }

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate) for write 1 to 256 bytes per page mode or byte write mode
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param write_gran write granularity bytes, only support 1 or 256
 * @param data write data
 *
 * @return result
 */
static sfud_err page256_or_1_byte_write(const sfud_flash *flash, uint32_t addr, size_t size, uint16_t write_gran,
        const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    static uint8_t cmd_data[5 + SFUD_WRITE_MAX_PAGE_SIZE];
    uint8_t cmd_size;
    size_t data_size;

    SFUD_ASSERT(flash);
    /* only support 1 or 256 */
    SFUD_ASSERT(write_gran == 1 || write_gran == 256);
    /* must be call this function after initialize OK */
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }

    /* loop write operate. write unit is write granularity */
    while (size) {
        /* set the flash write enable */
        result = set_write_enabled(flash, true);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        cmd_data[0] = SFUD_CMD_PAGE_PROGRAM;
        make_adress_byte_array(flash, addr, &cmd_data[1]);
        cmd_size = flash->addr_in_4_byte ? 5 : 4;

        /* make write align and calculate next write address */
        if (addr % write_gran != 0) {
            if (size > write_gran - (addr % write_gran)) {
                data_size = write_gran - (addr % write_gran);
            } else {
                data_size = size;
            }
        } else {
            if (size > write_gran) {
                data_size = write_gran;
            } else {
                data_size = size;
            }
        }
        size -= data_size;
        addr += data_size;

        memcpy(&cmd_data[cmd_size], data, data_size);

        result = spi->wr(spi, cmd_data, cmd_size + data_size, NULL, 0);
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }
        result = wait_busy(flash);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        data += data_size;
    }

__exit:
    /* set the flash write disable */
    set_write_enabled(flash, false);
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate) for auto address increment mode
 *
 * If the address is odd number, it will place one 0xFF before the start of data for protect the old data.
 * If the latest remain size is 1, it will append one 0xFF at the end of data for protect the old data.
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
static sfud_err aai_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[8], cmd_size;
    bool first_write = true;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(flash->init_ok);
    /* check the flash address bound */
    if (addr + size > flash->chip.capacity) {
        SFUD_INFO("Error: Flash address is out of bound.");
        return SFUD_ERR_ADDR_OUT_OF_BOUND;
    }
    /* lock SPI */
    if (spi->lock) {
        spi->lock(spi);
    }
    /* The address must be even for AAI write mode. So it must write one byte first when address is odd. */
    if (addr % 2 != 0) {
        result = page256_or_1_byte_write(flash, addr++, 1, 1, data++);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }
        size--;
    }
    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        goto __exit;
    }
    /* loop write operate. */
    cmd_data[0] = SFUD_CMD_AAI_WORD_PROGRAM;
    while (size >= 2) {
        if (first_write) {
            make_adress_byte_array(flash, addr, &cmd_data[1]);
            cmd_size = flash->addr_in_4_byte ? 5 : 4;
            cmd_data[cmd_size] = *data;
            cmd_data[cmd_size + 1] = *(data + 1);
            first_write = false;
        } else {
            cmd_size = 1;
            cmd_data[1] = *data;
            cmd_data[2] = *(data + 1);
        }

        result = spi->wr(spi, cmd_data, cmd_size + 2, NULL, 0);
        if (result != SFUD_SUCCESS) {
            SFUD_INFO("Error: Flash write SPI communicate error.");
            goto __exit;
        }

        result = wait_busy(flash);
        if (result != SFUD_SUCCESS) {
            goto __exit;
        }

        size -= 2;
        addr += 2;
        data += 2;
    }
    /* set the flash write disable for exit AAI mode */
    result = set_write_enabled(flash, false);
    /* write last one byte data when origin write size is odd */
    if (result == SFUD_SUCCESS && size == 1) {
        result = page256_or_1_byte_write(flash, addr, 1, 1, data);
    }

__exit:
    if (result != SFUD_SUCCESS) {
        set_write_enabled(flash, false);
    }
    /* unlock SPI */
    if (spi->unlock) {
        spi->unlock(spi);
    }

    return result;
}

/**
 * write flash data (no erase operate)
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;

    if (flash->chip.write_mode & SFUD_WM_PAGE_256B) {
        result = page256_or_1_byte_write(flash, addr, size, 256, data);
    } else if (flash->chip.write_mode & SFUD_WM_AAI) {
        result = aai_write(flash, addr, size, data);
    } else if (flash->chip.write_mode & SFUD_WM_DUAL_BUFFER) {
        //TODO dual-buffer write mode
    }

    return result;
}

/**
 * erase and write flash data
 *
 * @param flash flash device
 * @param addr start address
 * @param size write size
 * @param data write data
 *
 * @return result
 */
sfud_err sfud_erase_write(const sfud_flash *flash, uint32_t addr, size_t size, const uint8_t *data) {
    sfud_err result = SFUD_SUCCESS;

    result = sfud_erase(flash, addr, size);

    if (result == SFUD_SUCCESS) {
        result = sfud_write(flash, addr, size, data);
    }

    return result;
}

static sfud_err reset(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_ENABLE_RESET;
    result = spi->wr(spi, cmd_data, 1, NULL, 0);
    if (result == SFUD_SUCCESS) {
        result = wait_busy(flash);
    } else {
        SFUD_INFO("Error: Flash device reset failed.");
        return result;
    }

    cmd_data[1] = SFUD_CMD_RESET;
    result = spi->wr(spi, &cmd_data[1], 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        result = wait_busy(flash);
    }

    if (result == SFUD_SUCCESS) {
        SFUD_DEBUG("Flash device reset success.");
    } else {
        SFUD_INFO("Error: Flash device reset failed.");
    }

    return result;
}

static sfud_err read_jedec_id(sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[1], recv_data[3];

    SFUD_ASSERT(flash);

    cmd_data[0] = SFUD_CMD_JEDEC_ID;
    result = spi->wr(spi, cmd_data, sizeof(cmd_data), recv_data, sizeof(recv_data));
    if (result == SFUD_SUCCESS) {
        flash->chip.mf_id = recv_data[0];
        flash->chip.type_id = recv_data[1];
        flash->chip.capacity_id = recv_data[2];
        SFUD_DEBUG("The flash device manufacturer ID is 0x%02X, memory type ID is 0x%02X, capacity ID is 0x%02X.",
                flash->chip.mf_id, flash->chip.type_id, flash->chip.capacity_id);
    } else {
        SFUD_INFO("Error: Read flash device JEDEC ID error.");
    }

    return result;
}

/**
 * set the flash write enable or write disable
 *
 * @param flash flash device
 * @param enabled true: enable  false: disable
 *
 * @return result
 */
static sfud_err set_write_enabled(const sfud_flash *flash, bool enabled) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd, register_status;

    SFUD_ASSERT(flash);

    if (enabled) {
        cmd = SFUD_CMD_WRITE_ENABLE;
    } else {
        cmd = SFUD_CMD_WRITE_DISABLE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        result = sfud_read_status(flash, &register_status);
    }

    if (result == SFUD_SUCCESS) {
        if (enabled && (register_status & SFUD_STATUS_REGISTER_WEL) == 0) {
            SFUD_INFO("Error: Can't enable write status.");
            return SFUD_ERR_WRITE;
        } else if (!enabled && (register_status & SFUD_STATUS_REGISTER_WEL) != 0) {
            SFUD_INFO("Error: Can't disable write status.");
            return SFUD_ERR_WRITE;
        }
    }

    return result;
}

/**
 * enable or disable 4-Byte addressing for flash
 *
 * @note The 4-Byte addressing just supported for the flash capacity which is large then 16MB (256Mb).
 *
 * @param flash flash device
 * @param enabled true: enable   false: disable
 *
 * @return result
 */
static sfud_err set_4_byte_address_mode(sfud_flash *flash, bool enabled) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t cmd;

    SFUD_ASSERT(flash);

    /* set the flash write enable */
    result = set_write_enabled(flash, true);
    if (result != SFUD_SUCCESS) {
        return result;
    }

    if (enabled) {
        cmd = SFUD_CMD_ENTER_4B_ADDRESS_MODE;
    } else {
        cmd = SFUD_CMD_EXIT_4B_ADDRESS_MODE;
    }

    result = flash->spi.wr(&flash->spi, &cmd, 1, NULL, 0);

    if (result == SFUD_SUCCESS) {
        flash->addr_in_4_byte = enabled ? true : false;
        SFUD_DEBUG("%s 4-Byte addressing mode success.", enabled ? "Enter" : "Exit");
    } else {
        SFUD_INFO("Error: %s 4-Byte addressing mode failed.", enabled ? "Enter" : "Exit");
    }

    return result;
}

/**
 * read flash register status
 *
 * @param flash flash device
 * @param status register status
 *
 * @return result
 */
sfud_err sfud_read_status(const sfud_flash *flash, uint8_t *status) {
    uint8_t cmd = SFUD_CMD_READ_STATUS_REGISTER;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(status);

    return flash->spi.wr(&flash->spi, &cmd, 1, status, 1);
}

static sfud_err wait_busy(const sfud_flash *flash) {
    sfud_err result = SFUD_SUCCESS;
    uint8_t status;
    size_t retry_times = flash->retry.times;

    SFUD_ASSERT(flash);

    while (true) {
        result = sfud_read_status(flash, &status);
        if (result == SFUD_SUCCESS && ((status & SFUD_STATUS_REGISTER_BUSY)) == 0) {
            break;
        }
        /* retry counts */
        SFUD_RETRY_PROCESS(flash->retry.delay, retry_times, result);
    }

    if (result != SFUD_SUCCESS || ((status & SFUD_STATUS_REGISTER_BUSY)) != 0) {
        SFUD_INFO("Error: Flash wait busy has an error.");
    }

    return result;
}

static void make_adress_byte_array(const sfud_flash *flash, uint32_t addr, uint8_t *array) {
    uint8_t len, i;

    SFUD_ASSERT(flash);
    SFUD_ASSERT(array);

    len = flash->addr_in_4_byte ? 4 : 3;

    for (i = 0; i < len; i++) {
        array[i] = (addr >> ((len - (i + 1)) * 8)) & 0xFF;
    }
}

/**
 * write status register
 *
 * @param flash flash device
 * @param is_volatile true: volatile mode, false: non-volatile mode
 * @param status register status
 *
 * @return result
 */
sfud_err sfud_write_status(const sfud_flash *flash, bool is_volatile, uint8_t status) {
    sfud_err result = SFUD_SUCCESS;
    const sfud_spi *spi = &flash->spi;
    uint8_t cmd_data[2];

    SFUD_ASSERT(flash);

    if (is_volatile) {
        cmd_data[0] = SFUD_VOLATILE_SR_WRITE_ENABLE;
        result = spi->wr(spi, cmd_data, 1, NULL, 0);
    } else {
        result = set_write_enabled(flash, true);
    }

    if (result == SFUD_SUCCESS) {
        cmd_data[0] = SFUD_CMD_WRITE_STATUS_REGISTER;
        cmd_data[1] = status;
        result = spi->wr(spi, cmd_data, 2, NULL, 0);
    }

    if (result != SFUD_SUCCESS) {
        SFUD_INFO("Error: Write_status register failed.");
    }

    return result;
}

int Flash_write_data(lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	return gLfsDevice->prog(gLfsDevice, block, off, buffer, size);
}

int Flash_read_data(lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
	return gLfsDevice->read(gLfsDevice, block, off, buffer, size);
}

char Plan_ID_Compare(unsigned char * buffer1, unsigned char * buffer2)
{
	if(memcmp(buffer1, buffer2, 6) == 0)
		return 1;
	return 0;
}

static unsigned long _get_plan_num(unsigned char * ucPlan)
{
	unsigned char PlanNum[5] = {0};
	memcpy(&PlanNum[0], &ucPlan[6], 4);
	return sys_atoi((const char *)PlanNum);
}

int clear_all_work_plan(void)
{
	unsigned char pucBuff[64] = {0};
	Flash_write_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	gWorkWorker.GetRealWorkTime = 0;
	gWorkWorker.ReflashWorkTime = 0;
	sys_delay_ms(10);
	return 0;
}

void read_plan_by_index(unsigned char * pucData, unsigned int uwIndex)
{
	Flash_read_data(RUN_PLAN_START_SECTOR + (uwIndex / PLAN_NUM_EACH_SECTOR), (uwIndex % PLAN_NUM_EACH_SECTOR) * PLAN_DATA_SIZE, pucData, PLAN_DATA_SIZE);
}

void write_plan_by_index(unsigned char * pucData, unsigned int uwIndex)
{
	Flash_write_data(RUN_PLAN_START_SECTOR + (uwIndex / PLAN_NUM_EACH_SECTOR), 0, pucData, LFS_BLOCK_SIZE);
}

static int _save_plan_num(unsigned char * pucData, lfs_size_t write_size, unsigned long ulPlanNum, unsigned int uwIndex)
{
	unsigned char pucBuff[64] = {0};
	sprintf((char *)pucBuff, "CREAT:%04u", (unsigned int)ulPlanNum);
	Flash_write_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	if(pucData != NULL)
		Flash_write_data(RUN_PLAN_START_SECTOR + (uwIndex / PLAN_NUM_EACH_SECTOR), 0, pucData, write_size);
	sys_delay_ms(20);
	return 0;
}


int get_work_plan(unsigned char * pucBuf, unsigned int ulIndex)
{
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	if(memcmp(pucBuff, "CREAT:", 6) == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		if(ulIndex <= ulPlanNum)
		{
			read_plan_by_index(pucBuf, ulIndex);
			return 0;
		}
	}
	else
		debug_printf("didn't has work plan\r\n");
	return -1;
}

void diplay_all_plan(void)
{
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	if(memcmp(pucBuff, "CREAT:", 6) == 0)
	{
		ulPlanNum = _get_plan_num(pucBuff);
		if(ulPlanNum >= PLAN_MAX_NUM) return;
		for(int i = 0; i < ulPlanNum; i++)
		{
			read_plan_by_index(pucBuff, i);
			debug_printf("Plan %04u:%s\r\n", i, pucBuff);
		}
	}
	else
	{
		debug_printf("didn't exist any plan\r\n");
	}
}


int add_work_plan(unsigned char * buffer)
{	//增加巡检计划
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	unsigned int index;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	if(memcmp(pucBuff, "CREAT:", 6) == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		if(ulPlanNum >= PLAN_MAX_NUM) return -1;
		
		for(index = 0; index < ulPlanNum; index++)
		{	//判断巡检计划ID是否已存在，存在则覆盖,否则则新增
			if(get_work_plan(pucBuff, index) == 0)
			{
				if(Plan_ID_Compare(&buffer[1], &pucBuff[1]))
				{
					push_plan_status(&buffer[1], PLAN_CHANGE, 0, 0);
					break;
				}
			}
		}
		Flash_read_data(RUN_PLAN_START_SECTOR + index / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
		if(index >= ulPlanNum)
		{
			push_plan_status(&buffer[1], PLAN_ADD, 0, 0);
			index = ulPlanNum;
			ulPlanNum++;
		}
		gWorkWorker.ReflashWorkTime = 0;
		gWorkWorker.GetRealWorkTime = 0;
		memcpy(&gucBlockTemp[(index % PLAN_NUM_EACH_SECTOR)   * PLAN_DATA_SIZE], buffer, PLAN_DATA_SIZE);
		_save_plan_num(gucBlockTemp, LFS_BLOCK_SIZE, ulPlanNum, index);
		diplay_all_plan();
	}
	else
	{	//创建第一个任务
		push_plan_status(&pucBuff[1], PLAN_ADD, 0, 0);
		gWorkWorker.ReflashWorkTime = 0;
		gWorkWorker.GetRealWorkTime = 0;
		_save_plan_num(buffer, PLAN_DATA_SIZE, 1, 0);
		diplay_all_plan();
	}
	
	return 0;
}

int change_work_plan(unsigned char * buffer)
{	//修改巡检计划
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	unsigned int index;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	index = memcmp(pucBuff, "CREAT:", 6);
	if(index == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		for(index = 0; index < ulPlanNum; index++)
		{	//判断巡检计划ID是否已存在，存在则覆盖,否则则新增
			if(get_work_plan(pucBuff, index) == 0)
			{
				if(Plan_ID_Compare(&buffer[1], &pucBuff[1])) break;
			}
		}
		if(index < ulPlanNum)
		{
			push_plan_status(&pucBuff[1], PLAN_CHANGE, 0, 0);
			gWorkWorker.ReflashWorkTime = 0;
			gWorkWorker.GetRealWorkTime = 0;
			Flash_read_data(RUN_PLAN_START_SECTOR + index / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
			memcpy(&gucBlockTemp[(index % PLAN_NUM_EACH_SECTOR) * PLAN_DATA_SIZE], buffer, PLAN_DATA_SIZE);
			write_plan_by_index(gucBlockTemp, index);
			sys_delay_ms(20);
			diplay_all_plan();
		}
	}
	
	return 0;
}

int pause_resume_work_plan(unsigned char * buffer, unsigned char ucType)
{	//暂停/启用计划
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	unsigned int index;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	index = memcmp(pucBuff, "CREAT:", 6);
	if(index == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		for(index = 0; index < ulPlanNum; index++)
		{	//判断巡检计划ID是否已存在，存在则覆盖,否则则新增
			if(get_work_plan(pucBuff, index) == 0)
			{
				if(Plan_ID_Compare(&buffer[1], &pucBuff[1])) break;
			}
		}
		if(index < ulPlanNum)
		{
			if(ucType == 1)
				push_plan_status(&pucBuff[1], PLAN_PAUSE, 0, 0);
			else
				push_plan_status(&pucBuff[1], PLAN_RESUME, 0, 0);
			gWorkWorker.ReflashWorkTime = 0;
			gWorkWorker.GetRealWorkTime = 0;
			Flash_read_data(RUN_PLAN_START_SECTOR + index / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
			if(gucBlockTemp[index * PLAN_DATA_SIZE] == ((ucType == 1) ? '1' : '0'))
			{
				gucBlockTemp[(index % PLAN_NUM_EACH_SECTOR) * PLAN_DATA_SIZE] = ((ucType == 1) ? '0' : '1');
				write_plan_by_index(gucBlockTemp, index);
			}
			diplay_all_plan();
		}
	}
	
	return 0;
}

int del_work_plan(unsigned char * buffer, unsigned char ucType)
{	//删除巡检计划,ucType 0:巡检计划匹配 1:巡检计划ID匹配
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;
	unsigned int index;
	
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	index = memcmp(pucBuff, "CREAT:", 6);
	if(index == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		for(index = 0; index < ulPlanNum; index++)
		{	//判断巡检计划ID是否已存在，存在则删除
			if(get_work_plan(pucBuff, index) == 0)
			{
				if(ucType == 0)
				{
					if(Plan_ID_Compare(&buffer[1], &pucBuff[1]))
					{
						push_plan_status(&pucBuff[1], PLAN_DEL, 0, 0);
						break;
					}
				}
				else
				{
					if(Plan_ID_Compare(buffer, &pucBuff[1]))
					{
						push_plan_status(&pucBuff[1], PLAN_DEL, 0, 0);
						break;
					}
				}
			}
		}
		if(index < ulPlanNum)
		{
			gWorkWorker.ReflashWorkTime = 0;
			gWorkWorker.GetRealWorkTime = 0;
			if(ulPlanNum == 1)	//只有一个巡检计划时直接清除巡检状态
				clear_all_work_plan();
			else
			{
				Flash_read_data(RUN_PLAN_START_SECTOR + index / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
				ulPlanNum--;
				if(index < ulPlanNum)
				{
					Flash_read_data(RUN_PLAN_START_SECTOR + index / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
					read_plan_by_index(pucBuff, ulPlanNum);
					memcpy(&gucBlockTemp[(index % PLAN_NUM_EACH_SECTOR) * PLAN_DATA_SIZE], pucBuff, PLAN_DATA_SIZE);
					_save_plan_num(gucBlockTemp, LFS_BLOCK_SIZE, ulPlanNum, index);
				}
				else
					_save_plan_num(NULL, LFS_BLOCK_SIZE, ulPlanNum, index);
				diplay_all_plan();
			}
		}
	}
	
	return 0;
}

int del_work_plan_index(unsigned long ulIndex)
{	//通过索引号删除巡检计划
	unsigned char pucBuff[100] = {0};
	unsigned long ulPlanNum;

	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	if(memcmp(pucBuff, "CREAT:", 6) == 0)
	{
		ulPlanNum = _get_plan_num(pucBuff);
		if(ulIndex >= ulPlanNum) return -1;
		if(ulIndex < ulPlanNum)
		{
			if(ulPlanNum == 1)	//只有一个巡检计划时直接清除巡检状态
				clear_all_work_plan();
			else
			{
				Flash_read_data(RUN_PLAN_START_SECTOR + ulIndex / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
				ulPlanNum--;
				if(ulIndex < ulPlanNum)
				{
					Flash_read_data(RUN_PLAN_START_SECTOR + ulIndex / PLAN_NUM_EACH_SECTOR, 0, gucBlockTemp, LFS_BLOCK_SIZE);
					read_plan_by_index(pucBuff, ulPlanNum);
					memcpy(&gucBlockTemp[(ulIndex % PLAN_NUM_EACH_SECTOR) * PLAN_DATA_SIZE], pucBuff, PLAN_DATA_SIZE);
					_save_plan_num(gucBlockTemp, LFS_BLOCK_SIZE, ulPlanNum, ulIndex);
				}
				else
					_save_plan_num(NULL, LFS_BLOCK_SIZE, ulPlanNum, ulIndex);
				diplay_all_plan();
			}
		}
	}
	
	return 0;
}

unsigned char check_time_out(unsigned char * pucBuff)
{	//已过了巡检计划
	unsigned char pucTurn[3] = {0}, * dataindex;
	Time_info time;
	
	dataindex = pucBuff;
	memcpy(pucTurn, dataindex, 2);
	time.Year = sys_atoi((const char *)pucTurn);
	if(time.Year < gCurrentTime.Year)	//计划年份已经过时，删除
		return 1;
	else if(time.Year > gCurrentTime.Year)
		return 0;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Month = sys_atoi((const char *)pucTurn);
	if(time.Month < gCurrentTime.Month)	//计划月份已经过时，删除
		return 1;
	else if(time.Month > gCurrentTime.Month)
		return 0;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Day = sys_atoi((const char *)pucTurn);
	if(time.Day < gCurrentTime.Day)
		return 1;
	else if(time.Day > gCurrentTime.Day)
		return 0;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Hour = sys_atoi((const char *)pucTurn);
	if(time.Hour < gCurrentTime.Hour)
		return 1;
	else if(time.Hour > gCurrentTime.Hour)
		return 0;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Minute = sys_atoi((const char *)pucTurn);
	if(time.Minute < gCurrentTime.Minute)
		return 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Second = sys_atoi((const char *)pucTurn);
	if((time.Minute == gCurrentTime.Minute) && (time.Second < gCurrentTime.Second))
		return 1;

	return 0;
}

unsigned char check_time_unreach(unsigned char * pucBuff, Time_info * Turn)
{	//巡检计划时间未到
	unsigned char ucSta = 0, ucFin = 0;

	unsigned char pucTurn[3] = {0}, * dataindex;
	Time_info time;
	
	dataindex = pucBuff;
	memcpy(pucTurn, dataindex, 2);
	time.Year = sys_atoi((const char *)pucTurn);
	if(time.Year > gCurrentTime.Year)	
		ucSta = 1;
	else if(time.Year < gCurrentTime.Year)
		ucFin = 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Month = sys_atoi((const char *)pucTurn);
	if(time.Month > gCurrentTime.Month)	
	{
		if(ucFin == 0) ucSta |= 0x01;
	}
	else if(time.Month < gCurrentTime.Month)
	{
		if(ucSta == 0) ucFin = 1;
	}
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Day = sys_atoi((const char *)pucTurn);
	if(time.Day > gCurrentTime.Day)
	{
		if(ucFin == 0) ucSta |= 0x01;
	}
	else if(time.Day < gCurrentTime.Day)
	{
		if(ucSta == 0) ucFin = 1;
	}
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Hour = sys_atoi((const char *)pucTurn);
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Minute = sys_atoi((const char *)pucTurn);
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time.Second = sys_atoi((const char *)pucTurn);
	memcpy(Turn, &time, sizeof(Time_info));

	if(ucFin ||!ucSta) return 0;
	else return 1;
}

unsigned char get_single_time(unsigned char * pucBuff, Time_info * time)
{
	unsigned char pucTurn[3] = {0}, * dataindex;

	dataindex = pucBuff;
	memcpy(pucTurn, dataindex, 2);
	time->Year = sys_atoi((const char *)pucTurn);
	if(time->Year != gCurrentTime.Year)	//不等于当前年份
		return 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time->Month = sys_atoi((const char *)pucTurn);
	if(time->Month != gCurrentTime.Month)	//不等于当前月份
		return 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time->Day = sys_atoi((const char *)pucTurn);
	if(time->Day != gCurrentTime.Day)		//不等于当天
		return 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time->Hour = sys_atoi((const char *)pucTurn);
	if(time->Hour > gCurrentTime.Hour)
	{
		dataindex += 2;
		memcpy(pucTurn, dataindex, 2);
		time->Minute = sys_atoi((const char *)pucTurn);
		dataindex += 2;
		memcpy(pucTurn, dataindex, 2);
		time->Second = sys_atoi((const char *)pucTurn);
		return 0;
	}
	else if(time->Hour < gCurrentTime.Hour)
		return 1;
	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time->Minute = sys_atoi((const char *)pucTurn);
	if(time->Minute < gCurrentTime.Minute)
		return 1;

	dataindex += 2;
	memcpy(pucTurn, dataindex, 2);
	time->Second = sys_atoi((const char *)pucTurn);

	return 0;
}

static void _set_work_time(uint32_t hour, uint32_t minute, uint32_t second, unsigned char * pucBuff, uint16_t usTaskID)
{
	gWorkWorker.Worktime.Hour = hour;
	gWorkWorker.Worktime.Minute = minute;
	gWorkWorker.Worktime.Second = second;
	memcpy(gWorkWorker.CurrentPlanID,&pucBuff[1], 6);
	gWorkWorker.repeatsta = pucBuff[7] - '0';
	gWorkWorker.usTaskID = usTaskID;
	debug_printf("Get time:%02u:%02u:%02u, TaskID:%u\r\n", hour, minute, second, gWorkWorker.usTaskID);
}

uint16_t get_current_task_id(Time_info StartTime, Time_info EndTime, cron_expr cycle)
{
	uint16_t usTaskID = 0;
	uint8_t ucYear, ucMonth, ucDay, ucHour, ucMinute, ucSecond, ucFirst = 0;
	uint8_t ucMonMax, ucDayMax, ucHourMax, ucMinuteMax, ucSecondMax;
	
	for(ucYear = StartTime.Year; ucYear <= EndTime.Year; ucYear++)
	{
		if(ucFirst == 0) ucMonth = StartTime.Month;
		else ucMonth = 1;
		if(ucYear == EndTime.Year) ucMonMax = EndTime.Month;
		else ucMonth = 12;
		for(;ucMonth <= ucMonMax; ucMonth++)
		{
			if(((cycle.months[(ucMonth - 1)/8] >> ((ucMonth - 1) % 8)) & BIT(0)) == 1)
			{	//当月存在巡检计划
				if(ucFirst == 0) ucDay = StartTime.Day;
				else ucDay = 1;
				if((ucYear == EndTime.Year) && (ucMonth == EndTime.Month)) ucDayMax = EndTime.Day;
				else {
					if((ucMonth == 2) && ((ucYear % 4) == 0) && ((ucYear % 100) != 0) && ((ucYear % 400) == 0))
						ucDayMax = MonthOfDays[ucMonth - 1] + 1;
					else
						ucDayMax = MonthOfDays[ucMonth - 1] + 1;
				}
				for(;ucDay <= ucDayMax; ucDay++)
				{
					if(((cycle.days_of_month[(ucDay - 1)/8] >> ((ucDay - 1) % 8)) & BIT(0)) == 1)
					{	//当日存在巡检计划
						if(ucFirst == 0) ucHour = StartTime.Hour;
						else ucHour = 0;
						if((ucYear == EndTime.Year) && (ucMonth == EndTime.Month) && (ucDay == EndTime.Day)) ucHourMax = EndTime.Hour;
						else ucHourMax = 23;
						for(;ucHour <= ucHourMax; ucHour++)
						{
							if(((cycle.hours[ucHour/8] >> (ucHour % 8)) & BIT(0)) == 1)
							{	//当前小时存在巡检计划
								if(ucFirst == 0) ucMinute = StartTime.Minute;
								else ucMinute = 0;
								if((ucYear == EndTime.Year) && (ucMonth == EndTime.Month) && (ucDay == EndTime.Day) && (ucHour == EndTime.Hour)) ucMinuteMax = EndTime.Minute;
								else ucMinuteMax = 59;
								for(;ucMinute <= ucMinuteMax; ucMinute++)
								{
									if(((cycle.minutes[ucMinute/8] >> (ucMinute % 8)) & BIT(0)) == 1)
									{
										if(ucFirst == 0) ucSecond = StartTime.Second;
										else ucSecond = 0;
										if((ucYear == EndTime.Year) && (ucMonth == EndTime.Month) && (ucDay == EndTime.Day) && (ucHour == EndTime.Hour) && (ucMinute == EndTime.Minute)) ucSecondMax = EndTime.Second;
										else ucSecondMax = 59;
										ucFirst = 1;
										for(; ucSecond <= ucSecondMax; ucSecond++)
										{
											if(((cycle.seconds[ucSecond/8] >> (ucSecond % 8)) & BIT(0)) == 1)
												usTaskID++;
											else continue;
										}
									}
									else {
										ucFirst = 1;
										continue;
									}
								}
							}
							else {
								ucFirst = 1;
								continue;
							}
						}
					}
					else {
						ucFirst = 1;
						continue;
					}
				}
			}
			else { 
				ucFirst = 1;
				continue;
			}
		}
	}
	return usTaskID;
}

int lookup_work_plan(Time_info * last_time)
{	//获取最近的巡检时间
	unsigned char pucBuff[64] = {0}, crondata[32], ucSta, ucStartTimeFlag, ucStartMin;
	unsigned long ulPlanNum;
	unsigned int index, indexHour, indexMin, indexSecond;
	st_inspect_plan plan;
	const char * perror = NULL;
	Time_info time, starttime, endtime;
	uint16_t usTaskID = 0;
//	struct tm tm = {0};

	gWorkWorker.GetRealWorkTime = 0;
	Flash_read_data(PLAN_NUM_SECTOR, 0, pucBuff, PLAN_DATA_SIZE);
	index = memcmp(pucBuff, "CREAT:", 6);
	if(index == 0)
	{	//原已存储计划
		ulPlanNum = _get_plan_num(pucBuff);
		for(index = 0; index < ulPlanNum; index++)
		{	//重复的巡检计划
			get_work_plan(pucBuff, index);
			if(pucBuff[0] == '1') continue;		//巡检计划暂停中
			ucSta = 0;
			if(pucBuff[7] == '1')
			{
				if(check_time_unreach(&pucBuff[8], &starttime)) continue;	//未到巡检的最小时间
				if(check_time_out(&pucBuff[20]) == 1)
				{
					push_plan_status(&pucBuff[1], PLAN_DEL, 0, 0);
					del_work_plan_index(index);
					index--;
					ulPlanNum--;
					continue;
				}
				ucStartTimeFlag = 0;
				if((starttime.Year == gCurrentTime.Year) && (starttime.Month == gCurrentTime.Month) && (starttime.Day == gCurrentTime.Day))
					ucStartTimeFlag = 1;
				memcpy(crondata, &pucBuff[32], 32);
				cron_parse_expr((const char *)crondata, &plan.cycle, &perror);
				if(NULL != perror) continue;
				if(((plan.cycle.months[(gCurrentTime.Month - 1)/8] >> ((gCurrentTime.Month - 1) % 8)) & BIT(0)) == 0) continue;	//当月无巡检计划
				if(((plan.cycle.days_of_week[0] >> gCurrentTime.WeekDay) & BIT(0)) == 0) continue;	//周几无巡检计划
				if(((plan.cycle.days_of_month[gCurrentTime.Day/8] >> (gCurrentTime.Day % 8)) & BIT(0)) == 0) continue;	//今天无巡检计划
				for(indexHour = 0; indexHour < 24; indexHour++)
				{
					if((plan.cycle.hours[indexHour / 8] >> (indexHour % 8)) & BIT(0))
					{
						if((ucStartTimeFlag == 1) && (indexHour < starttime.Hour))
							continue;
						if(gCurrentTime.Hour < indexHour)		//未到巡检时间，直接退出
						{
							if(ucStartTimeFlag && starttime.Hour == indexHour)
								ucStartMin = 1;
							else
								ucStartMin = 0;
							for(indexMin = 0; indexMin < 60; indexMin++)
							{
								if((plan.cycle.minutes[indexMin / 8] >> (indexMin % 8)) & BIT(0))
								{
									if(ucStartMin == 1 && indexMin < starttime.Minute)
										continue;
									break;
								}
							}
							ucSta = 1;						//获取到巡检时间
							break;
						}
						else if(gCurrentTime.Hour == indexHour)
						{
							if(ucStartTimeFlag && starttime.Hour == indexHour)
								ucStartMin = 1;
							else
								ucStartMin = 0;
							for(indexMin = 0; indexMin < 60; indexMin++)
							{
								if(ucStartMin == 1 && indexMin < starttime.Minute)
									continue;
								if((plan.cycle.minutes[indexMin / 8] >> (indexMin % 8)) & BIT(0))
								{
									if(gCurrentTime.Minute <= indexMin)
									{
										ucSta = 1;
										break;
									}
									else if((gCurrentTime.Minute - indexMin) < 2)
									{
										ucSta = 1;
										break;
									}
									else continue;
								}
							}
							if(ucSta == 1) break;
							else continue;
						}
						else continue;			
					}
				}
			}
			else
			{
				if(check_time_out(&pucBuff[8]) == 1)	
				{	//超过巡检时间
					push_plan_status(&pucBuff[1], PLAN_DEL, 0, 0);
					del_work_plan_index(index);
					index--;
					ulPlanNum--;
					continue;
				}
				if(get_single_time(&pucBuff[8], &time)) continue;
				usTaskID = 0;
				ucSta = 1;
			}
			if(ucSta == 1)
			{
				if(pucBuff[7] == '1')
				{
					usTaskID = 0;
					for(indexSecond = 0; indexSecond < 60; indexSecond++)
					{
						if((plan.cycle.seconds[indexSecond / 8] >> (indexSecond % 8)) & BIT(0))
							break;
					}
					endtime.Year = gCurrentTime.Year;
					endtime.Month = gCurrentTime.Month;
					endtime.Day = gCurrentTime.Day;
					endtime.Hour = indexHour;
					endtime.Minute = indexMin;
					endtime.Second = indexSecond;
					usTaskID = get_current_task_id(starttime, endtime, plan.cycle);
				}
				else
				{
					indexSecond = time.Second;
					indexMin = time.Minute;
					indexHour = time.Hour;
				}
				if(gWorkWorker.GetRealWorkTime == 0)
				{	//未获得过时间
					_set_work_time(indexHour, indexMin, indexSecond, pucBuff, usTaskID);
					gWorkWorker.GetRealWorkTime = 1;
				}
				else
				{
					if(gWorkWorker.Worktime.Hour > indexHour)
						_set_work_time(indexHour, indexMin, indexSecond, pucBuff, usTaskID);
					else if(gWorkWorker.Worktime.Hour == indexHour){
						if(gWorkWorker.Worktime.Minute > indexMin)
							_set_work_time(indexHour, indexMin, indexSecond, pucBuff, usTaskID);
						else if(gWorkWorker.Worktime.Minute == indexMin)
						{
							if(gWorkWorker.Worktime.Second > indexSecond)
								_set_work_time(indexHour, indexMin, indexSecond, pucBuff, usTaskID);
						}
					}
				}
			}
		}
		
		if(gWorkWorker.GetRealWorkTime == 0)
			gWorkWorker.GetRealWorkTime = 2;
	}
	else
	{
		gWorkWorker.GetRealWorkTime = 2;
		debug_printf("didn't exist any plan!\r\n");
	}
	
	return 0;
}




