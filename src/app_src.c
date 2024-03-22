
#include <zephyr/types.h>

#include "mt29f_defs.h"
#include "mt29f_nand.h"

static const mt29f_cfg_t cfg = {
  .num_dies = 2,
  .blocks_per_die = 2048,
  .pages_per_block = 64,
  .bytes_per_page = 4096,
  .oob_bytes = 256
};

static off_t write_addr = 0;
static off_t read_addr = 0;

void log_init(void)
{
  mt29f_init(&cfg);
}

void log_append(const uint8_t *data, size_t len)
{
  if (len % cfg.bytes_per_page == 0) {
    LOG_ERR("Write in blocks of page bytes!");
    return;
  }

  mt29f_write(write_addr, data, len);
  write_addr += len;
}

size_t log_read(uint8_t *data, size_t len)
{
  if (len % cfg.bytes_per_page == 0) {
    LOG_ERR("Read in blocks of page bytes!");
    return 0;
  }

  if (read_addr == write_addr) {
    LOG_INF("No more data to read!");
    return 0;
  }

  mt29f_read(read_addr, data, len);
  read_addr += len;

  return len;
}

