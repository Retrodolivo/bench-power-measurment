#ifndef INC_WIZCHIP_H_
#define INC_WIZCHIP_H_

#include "main.h"

extern SPI_HandleTypeDef hspi3;

#define W5500_SPI_PORT	&hspi3

typedef enum
{
	SOCK_0,
	SOCK_1,
	SOCK_2,
	SOCK_3,
	SOCK_4,
	SOCK_5,
	SOCK_6,
	SOCK_7
}SOCK_t;

typedef struct
{
	wiz_NetInfo netinfo;
	uint8_t rxtx_buff[8];

} w5500chip_t;


void w5500_init(w5500chip_t *chip);


#endif /* INC_WIZCHIP_H_ */
