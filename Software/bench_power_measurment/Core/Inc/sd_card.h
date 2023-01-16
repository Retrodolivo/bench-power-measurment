#ifndef _CD_CARD_H_
#define _CD_CARD_H_

#include "main.h"

#define SD_SPI_PORT      hspi1

extern SPI_HandleTypeDef SD_SPI_PORT;


typedef struct
{
	uint32_t blocks_num;
	uint32_t block_addr;
	uint32_t start_block_addr;
	uint32_t after_reboot_block_addr;
	uint8_t block[512];
} SD_t;
	

// all procedures return 0 on success, < 0 on failure

int sd_init(void);
int sd_get_blocks_num(uint32_t* num);
int sd_read_single_block(uint32_t blockNum, uint8_t* buff); // sizeof(buff) == 512!
int sd_write_single_block(uint32_t blockNum, const uint8_t* buff); // sizeof(buff) == 512!

// Read Multiple Blocks
int sd_read_begin(uint32_t blockNum);
int sd_read_data(uint8_t* buff); // sizeof(buff) == 512!
int sd_read_end(void);

// Write Multiple Blocks
int sd_write_begin(uint32_t blockNum);
int sd_write_data(const uint8_t* buff); // sizeof(buff) == 512!
int sd_write_end(void);

// TODO: read lock flag? CMD13, SEND_STATUS

#endif/*_CD_CARD_H_*/
