
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <sys/types.h>

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/byteorder.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(mt29f_nand, LOG_LEVEL_INF);

#include "mt29f_defs.h"
#include "mt29f_nand.h"

#define DUMMY_BYTE      0x00
#define MAX_SUPPORTED_BUF  (4352)

#define DT_DRV_COMPAT nordic_nrf_spim

#define SPI_DEV DT_COMPAT_GET_ANY_STATUS_OKAY(micron_mt29f)
#define SPI_OP SPI_OP_MODE_MASTER | SPI_MODE_CPOL | SPI_MODE_CPHA | \
                            SPI_WORD_SET(8) | SPI_LINES_SINGLE

static struct spi_dt_spec spi_dev = SPI_DT_SPEC_GET(SPI_DEV, SPI_OP, 0);

typedef struct mt29f_row_addr {
  uint8_t   die_num;
  uint8_t   page_num;
  uint16_t  blk_num;
} mt29f_row_addr_t;

typedef uint16_t mt29f_col_addr_t;

static mt29f_cfg_t inst = {0};

static int spi_nand_get_feature(const uint8_t addr, uint8_t *val)
{
  uint8_t tx_data[] = {COMMAND_GET_FEATURE, addr};

  struct spi_buf spi_buf[2] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    },
    {
      .buf = val,
      .len = 1
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 2,
	};

	const struct spi_buf_set rx_set = {
    .buffers = spi_buf,
    .count = 2,
	};

  return spi_transceive_dt(&spi_dev, &tx_set, &rx_set);
}

static int spi_nand_set_feature(const uint8_t addr, const uint8_t val)
{
  uint8_t tx_data[] = {COMMAND_SET_FEATURE, addr, val};

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
	};

  return spi_write_dt(&spi_dev, &tx_set);
}

static int spi_nand_check_id(void)
{
  uint8_t const expected[2] = {MANUFACTURER_ID, DEVICE_ID};

  uint8_t tx_data[] = {COMMAND_READ_ID, DUMMY_BYTE};
  uint8_t read_id[2];

  struct spi_buf spi_buf[2] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    },
    {
      .buf = read_id,
      .len = ARRAY_SIZE(read_id),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 2,
	};

	const struct spi_buf_set rx_set = {
    .buffers = spi_buf,
    .count = 2,
	};

  int ret = spi_transceive_dt(&spi_dev, &tx_set, &rx_set);

  if (memcmp(expected, read_id, ARRAY_SIZE(read_id)) != 0) {
    LOG_ERR("Wrong ID: %02X %02X , expected: %02X %02X",
        read_id[0], read_id[1], expected[0], expected[1]);
    return -ENODEV;
  }
  return ret;
}

static int spi_nand_die_select(uint8_t die_num)
{
  int rc = 0;
  const uint8_t target_die = (die_num == 0) ? DIE_0 : DIE_1;
  uint8_t die;
  spi_nand_get_feature(REG_DIE_SELECT, &die);
  if (die != target_die) {
    rc = spi_nand_set_feature(REG_DIE_SELECT, target_die);
    if (rc != 0) {
      LOG_ERR("Fail to select Die: %d", rc);
    }
  }

  LOG_DBG("Die select: %d", die_num);

  return rc;
}

static int spi_nand_reset(void) {
  uint8_t tx_data[] = {COMMAND_RESET};

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
	};

  return spi_write_dt(&spi_dev, &tx_set);
}

static int spi_nand_write_enable(void)
{
  uint8_t tx_data[] = {COMMAND_WRITE_ENABLE};

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
	};

  return spi_write_dt(&spi_dev, &tx_set);
}

static void spi_nand_unlock(uint8_t die_number)
{
  // Select Die
  {
    int rc = spi_nand_die_select(die_number);
    if (rc != 0) {
      LOG_ERR("Fail to select Die: %d", rc);
    }
    
  }

  spi_nand_write_enable();
  
  // Unlock the flash memory
  {
    int rc = spi_nand_set_feature(REG_BLOCK_LOCK, 0);

    if (rc != 0) {
      LOG_ERR("Fail to unlock NAND: %d", rc);
      return;
    }
  }
}

static int spi_nand_wait_until_ready(void)
{
  int ret;
  uint8_t reg = 0;

  do {
    ret = spi_nand_get_feature(REG_STATUS, &reg);
  } while (!ret && (reg & STATUS_BIT_OIP_MASK));

  return ret;
}

static mt29f_row_addr_t spi_nand_offset_to_row_addr(const off_t offset)
{
  mt29f_row_addr_t row_addr = {0};

  const uint64_t bytes_per_block = inst.bytes_per_page * inst.pages_per_block;
  const uint64_t bytes_per_die = bytes_per_block * inst.blocks_per_die;

  row_addr.die_num = offset / (bytes_per_die);
  row_addr.blk_num = (offset % bytes_per_die) / bytes_per_block;

  const off_t page_offset = (offset % bytes_per_die) % bytes_per_block;
  row_addr.page_num = (page_offset % bytes_per_block) / inst.bytes_per_page;

  return row_addr;
}

static void spi_nand_block_erase(const mt29f_row_addr_t addr)
{
  spi_nand_die_select(addr.die_num);

  spi_nand_write_enable();

  // Row address
  uint32_t address = (addr.blk_num & 0x07FF) << BLOCK_POS;
  address |= (addr.page_num & 0x003F) << PAGE_POS;

  uint8_t tx_data[] = {
    COMMAND_BLOCK_ERASE,
    (address >> 16) & 0xFF,
    (address >> 8) & 0xFF,
    address & 0xFF
  };

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
  };

  LOG_DBG("Block %d erase: %d", addr.blk_num, address);

  int ret = spi_write_dt(&spi_dev, &tx_set);
  if (ret != 0) {
    LOG_ERR("Block erase failed: %d", ret);
    return;
  }

  spi_nand_wait_until_ready();
}

static int spi_nand_page_load(const uint32_t row_addr) {
    uint8_t tx_data[] = {
    COMMAND_PAGE_READ,
    (row_addr >> 16) & 0xFF,
    (row_addr >> 8) & 0xFF,
    row_addr & 0xFF
  };

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
  };

  LOG_DBG("Page load: %d", row_addr);

  return spi_write_dt(&spi_dev, &tx_set);
}

static int spi_nand_page_cache_read(const mt29f_col_addr_t col_addr, uint8_t *dest, const size_t len)
{
  uint8_t tx_data[] = {
    COMMAND_READ_FROM_CACHE_x1,
    (col_addr >> 8) & 0xFF,
    col_addr & 0xFF,
    DUMMY_BYTE
  };

  struct spi_buf spi_buf[2] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    },
    {
      .buf = dest,
      .len = len
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 2,
  };

  const struct spi_buf_set rx_set = {
    .buffers = spi_buf,
    .count = 2,
  };

  LOG_DBG("Cache read at %02X: %d bytes", col_addr, len);

  return spi_transceive_dt(&spi_dev, &tx_set, &rx_set);
}

static int spi_nand_page_read(const off_t offset, uint8_t *dest, const size_t len)
{
  int rc = 0;

  mt29f_row_addr_t row_addr = spi_nand_offset_to_row_addr(offset);

  spi_nand_die_select(row_addr.die_num);

  LOG_DBG("Page read: %ld", offset);
  LOG_DBG("Die: %d; Blk: %d; Page: %d", row_addr.die_num, row_addr.blk_num, row_addr.page_num);

  // Row address select
  uint32_t address = (row_addr.blk_num & 0x07FF) << BLOCK_POS;
  address |= (row_addr.page_num & 0x003F) << PAGE_POS;

  rc = spi_nand_page_load(address);
  if (rc != 0) {
    LOG_ERR("Page Load Failed: %d", rc);
    return rc;
  }

  spi_nand_wait_until_ready();

  // This only reads 1 whole page at a time
  rc = spi_nand_page_cache_read(0, dest, inst.bytes_per_page);
  if (rc != 0) {
    LOG_ERR("Page Cache Read Failed: %d", rc);
    return rc;
  }
  
  return rc;
}

static int spi_nand_program_load(const mt29f_col_addr_t col_addr, const uint8_t *data, const size_t len)
{
  uint8_t tx_data[] = {
    COMMAND_PROGRAM_LOAD_x1,
    (col_addr >> 8) & 0xFF,
    col_addr & 0xFF
  };

  struct spi_buf spi_buf[2] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    },
    {
      .buf = (void *)data,
      .len = len
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 2,
  };

  LOG_DBG("Program load at %02X: %d bytes", col_addr, len);

  return spi_write_dt(&spi_dev, &tx_set);
}

static int spi_nand_program_execute(const uint32_t mt29f_row_addr)
{
  uint8_t tx_data[] = {
    COMMAND_PROGRAM_EXECUTE,
    (mt29f_row_addr >> 16) & 0xFF,
    (mt29f_row_addr >> 8) & 0xFF,
    mt29f_row_addr & 0xFF
  };

  struct spi_buf spi_buf[] = {
    {
      .buf = tx_data,
      .len = ARRAY_SIZE(tx_data),
    }
  };

  const struct spi_buf_set tx_set = {
    .buffers = spi_buf,
    .count = 1,
  };

  LOG_DBG("Program execute page %d", mt29f_row_addr);

  return spi_write_dt(&spi_dev, &tx_set);
}

static int spi_nand_page_write(const off_t offset, const uint8_t *data, const size_t len)
{
  int rc = 0;

  mt29f_row_addr_t row_addr = spi_nand_offset_to_row_addr(offset);

  spi_nand_die_select(row_addr.die_num);
  spi_nand_write_enable();

  // TODO: This is destructive, but required before writing to an already-written block
  if (row_addr.page_num == 0) {
    spi_nand_block_erase(row_addr);
  }

  // This only writes 1 whole page at a time
  rc = spi_nand_program_load(0, data, inst.bytes_per_page);
  if (rc != 0) {
    LOG_ERR("Page Program Load Failed: %d", rc);
    return rc;
  }

  spi_nand_write_enable();

  // Row address select
  uint32_t address = (row_addr.blk_num & 0x07FF) << BLOCK_POS;
  address |= (row_addr.page_num & 0x003F) << PAGE_POS;

  rc = spi_nand_program_execute(address);
  if (rc != 0) {
    LOG_ERR("Page Program Execute Failed: %d", rc);
    return rc;
  }

  spi_nand_wait_until_ready();

  return rc;
}

/**
 * --------------------------------------------------------
 * Public API
 * --------------------------------------------------------
*/ 
/**
* @brief this function execute flash memory init on selected die.
*/
void mt29f_init(mt29f_cfg_t const *cfg)
{
  if (!cfg) {
    LOG_ERR("Invalid MT29F Config!");
    return;
  }
  inst = *cfg;

  if (!spi_is_ready_dt(&spi_dev)) {
    LOG_ERR("SPI device not initialized!");
    return;
  }

  // Reset the flash memory
  {
    int rc = spi_nand_reset();
    if (rc) {
      LOG_ERR("Fail to reset NAND! err: %d", rc);
    }
  }
 
  // Wait for power on reset (Datasheet value is equal to 1.25mSec)
  k_msleep(2);
  spi_nand_wait_until_ready();

  // Check ID response 
  {
    int rc = spi_nand_check_id();
    if (rc) {
      LOG_ERR("Check ID Failed");
      return;
    }
  }

  spi_nand_write_enable();

  // Enable ECC
  {
    int rc = spi_nand_set_feature(REG_CONFIGURATION, SEC_STATUS_BIT_ECC_EN);
    if (rc != 0) {
      LOG_ERR("Set Feature Failed: %d", rc);
    }
  }

  spi_nand_unlock(DIE_1);
  spi_nand_unlock(DIE_0);

  LOG_INF("MT29F Init Complete");
}

int mt29f_read(const off_t offset, uint8_t *data, const size_t len)
{
  if (!data) {
    LOG_ERR("Invalid data buffer!");
    return -EINVAL;
  }

  if (len % inst.bytes_per_page != 0) {
    LOG_ERR("Read in blocks of page bytes!");
    return -EINVAL;
  }

  return spi_nand_page_read(offset, data, len);
}

int mt29f_write(const off_t offset, const uint8_t *data, const size_t len)
{
  if (!data) {
    LOG_ERR("Invalid data buffer!");
    return -EINVAL;
  }

  if (len % inst.bytes_per_page != 0) {
    LOG_ERR("Write in blocks of page bytes!");
    return -EINVAL;
  }

  return spi_nand_page_write(offset, data, len);
}

void mt29f_chip_erase(void)
{
  LOG_INF("Erasing NAND chip...");
  
  for (int i = 0; i < inst.num_dies; i++) {
    for (int j = 0; j < inst.blocks_per_die; j++) {
      mt29f_row_addr_t addr = {.die_num = i, .blk_num = j, .page_num = 0};
      spi_nand_block_erase(addr);
    }
  }
  LOG_INF("Erase complete");
}
