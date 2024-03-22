
#pragma once

#include <stddef.h>
#include <stdint.h>

#include <sys/types.h>

typedef struct mt29f_cfg {
  uint8_t   num_dies;         // AKA "plane"
  uint16_t  blocks_per_die;
  uint16_t  pages_per_block;
  uint16_t  bytes_per_page;
  uint16_t  oob_bytes;
} mt29f_cfg_t;

/**
 * @brief This function initializes the flash device
*/
void mt29f_init(const mt29f_cfg_t *cfg);

/**
 * @brief Read data from flash device
*/
int mt29f_read(const off_t offset, uint8_t *data, const size_t len);

/**
 * @brief Write data to flash device
*/
int mt29f_write(const off_t offset, const uint8_t *data, const size_t len);

/**
 * @brief Erases entire flash device
*/
void mt29f_chip_erase(void);
