#include "wizchip.h"

static void w5500_select(void)
{
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_RESET);
}

static void w5500_unselect(void)
{
    HAL_GPIO_WritePin(W5500_CS_GPIO_Port, W5500_CS_Pin, GPIO_PIN_SET);
}

static uint8_t w5500_read_byte(void)
{
    uint8_t rb;
    HAL_SPI_Receive(W5500_SPI_PORT, &rb, 1, HAL_MAX_DELAY);
    return rb;
}

static void w5500_write_byte(uint8_t wb)
{
    HAL_SPI_Transmit(W5500_SPI_PORT, &wb, 1, HAL_MAX_DELAY);
}

void w5500_init(w5500chip_t *chip)
{
	/*link up architecture based spi cntrl funcs to lib funcs*/
	reg_wizchip_cs_cbfunc(w5500_select, w5500_unselect);
	reg_wizchip_spi_cbfunc(w5500_read_byte, w5500_write_byte);
	/*split up rxtx buffer among sockets*/
	chip->rxtx_buff[SOCK_0] = 16;
	chip->rxtx_buff[SOCK_1] = 0;
	chip->rxtx_buff[SOCK_2] = 0;
	chip->rxtx_buff[SOCK_3] = 0;
	chip->rxtx_buff[SOCK_4] = 0;
	chip->rxtx_buff[SOCK_5] = 0;
	chip->rxtx_buff[SOCK_6] = 0;
	chip->rxtx_buff[SOCK_7] = 0;

	uint8_t mac[6] = {0x00, 0x08, 0xdc, 0xab, 0xcd, 0xef};
	memcpy(chip->netinfo.mac, mac, 6);

	uint8_t ip[4] = {169, 254, 42, 16};
	memcpy(chip->netinfo.ip, ip, 4);

	/*setting subnet mask*/
	uint8_t sn[4] = {255, 255, 0, 0};
	memcpy(chip->netinfo.sn, sn, 4);

	/*setting gateway*/
	uint8_t gw[4] = {169, 254, 42, 123};
	memcpy(chip->netinfo.gw, gw, 4);

	wizchip_init(chip->rxtx_buff, chip->rxtx_buff);
	wizchip_setnetinfo(&chip->netinfo);
	/*after wizchip_getnetinfo() netinfo struct should stay the same*/
	wizchip_getnetinfo(&chip->netinfo);
}

