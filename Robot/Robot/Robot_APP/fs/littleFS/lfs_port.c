#include "lfs.h"
#ifdef USING_SFUD
#include "sfud.h"
#else
#include "hal_flash.h"
#endif

#define LFS_THREADSAFE

#ifdef LFS_NO_MALLOC
static char lfs_flash_read_buf[LFS_READ_SIZE];
static char lfs_flash_prog_buf[LFS_WRITE_SIZE];
static char lfs_flash_lookahead_buf[LFS_LOOKAHEAD_SIZE];
#endif

static int lfs_flash_read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t addr = block * c->block_size + off;
#ifdef USING_SFUD
    const sfud_flash *flash = sfud_get_device(0u);

    sfud_read(flash, addr, size, (uint8_t *)buffer);
#else
    FLASH_read(c->context, addr, buffer, size);
#endif
    return LFS_ERR_OK;
}

static int lfs_flash_erase(const struct lfs_config *c, lfs_block_t block)
{
#ifdef USING_SFUD
    const sfud_flash *flash = sfud_get_device(0u);

    sfud_erase(flash, block * c->block_size, c->block_size);
#else
    FLASH_erase(c->context, block * c->block_size, c->block_size);
#endif
    return LFS_ERR_OK;
}


static int
lfs_flash_prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
	for(uint32_t sector = block; sector < (block + ((off+ size) / LFS_BLOCK_SIZE) + (((off+ size) % LFS_BLOCK_SIZE) ?  1 : 0)); sector++)
		lfs_flash_erase(c, sector);
	
    uint32_t addr = block * c->block_size + off;
#ifdef USING_SFUD
    const sfud_flash *flash = sfud_get_device(0u);

    if(sfud_write(flash, addr, size, (uint8_t *)buffer) == SFUD_SUCCESS)
		return LFS_ERR_OK;
	else
		return LFS_ERR_INVAL;
#else
    FLASH_write(c->context, addr, buffer, size);
#endif
    //return LFS_ERR_OK;
}

static int lfs_flash_sync(const struct lfs_config *c)
{
    return LFS_ERR_OK;
}

int lfs_erase_flashchip(void)
{
#ifdef USING_SFUD
    const sfud_flash *flash = sfud_get_device(0u);

    sfud_chip_erase(flash);
#endif
    return LFS_ERR_OK;
}

int lfs_flash_init(struct lfs_config *c, void *pdev)
{
    if (0 != c
#ifdef USING_SFUD
        && SFUD_SUCCESS == sfud_init()
#endif
    ) {
        memset(c, 0, sizeof(*c));
        c->read = lfs_flash_read;
        c->prog = lfs_flash_prog;
        c->erase = lfs_flash_erase;
        c->sync = lfs_flash_sync;
        c->context = pdev;

        c->block_cycles = LFS_BLOCK_CYCLES;
        c->block_count = LFS_BLOCK_NUM;
        c->block_size = LFS_BLOCK_SIZE;
        //c->block_size = flash->chip.erase_gran;
        c->read_size = LFS_READ_SIZE;
        c->prog_size = LFS_WRITE_SIZE;
        c->cache_size = LFS_CACHE_SIZE;
        c->lookahead_size = LFS_LOOKAHEAD_SIZE;
#ifdef LFS_NO_MALLOC
        c->read_buffer = (void *)lfs_flash_read_buf;
        c->prog_buffer = (void *)lfs_flash_prog_buf;
        c->lookahead_buffer = (void *)lfs_flash_lookahead_buf;
#endif
        return LFS_ERR_OK;
    }
    return -1;
}
