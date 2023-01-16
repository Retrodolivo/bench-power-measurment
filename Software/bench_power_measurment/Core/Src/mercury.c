#include "mercury.h"

static uint16_t crc_upd(uint8_t tx_byte, uint16_t crc_old);

extern UART_HandleTypeDef huart2;
extern Response_all_t resp_all;


static uint8_t buf_tx[50];
static uint8_t buf_rx[50];

HAL_StatusTypeDef Mercury_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return HAL_UART_Transmit(huart, pData, Size, Timeout);
}

HAL_StatusTypeDef Mercury_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
{
	return HAL_UART_Receive(huart, pData, Size, Timeout);
}

static uint32_t to_power(uint8_t b1, uint8_t b2, uint8_t b3)
{
	return ((uint32_t)b1 << 16) | (((uint16_t)b3 << 8) | b2);
}
/*---------------REQS FILLING--------------------------*/
Req_net_test_t net_test = 
{	
	.net_addr = MERC_NET_ADDR, 
	.code = NET_TEST_CODE,
	.tx_arr[0] = MERC_NET_ADDR,
	.tx_arr[1] = NET_TEST_CODE
};

Request_std_t ser_num_read = 
{
	.net_addr = MERC_NET_ADDR,
	.code = READ_PROP_CODE,
	.prop_num = 0x00,
	.tx_arr[0] = MERC_NET_ADDR, 
	.tx_arr[1] = READ_PROP_CODE,
	.tx_arr[2] = 0x00
};

Request_open_t open_1 = 
{
	.net_addr = MERC_NET_ADDR,
	.code = NET_OPEN_CODE,
	.access_lvl = 0x01,
	.password[0] = 0x01,
	.password[1] = 0x01,
	.password[2] = 0x01,
	.password[3] = 0x01,
	.password[4] = 0x01,
	.password[5] = 0x01,	
	.tx_arr[0] = MERC_NET_ADDR, 
	.tx_arr[1] = NET_OPEN_CODE,
	.tx_arr[2] = 0x01,
	.tx_arr[3] = 0x01,
	.tx_arr[4] = 0x01,
	.tx_arr[5] = 0x01,
	.tx_arr[6] = 0x01,
	.tx_arr[7] = 0x01,
	.tx_arr[8] = 0x01
};

Request_open_t open_2 = 
{
	.net_addr = MERC_NET_ADDR,
	.code = NET_OPEN_CODE,
	.access_lvl = 0x02,
	.password[0] = 0x02,
	.password[1] = 0x02,
	.password[2] = 0x02,
	.password[3] = 0x02,
	.password[4] = 0x02,
	.password[5] = 0x02,	
	.tx_arr[0] = MERC_NET_ADDR, 
	.tx_arr[1] = NET_OPEN_CODE,
	.tx_arr[2] = 0x02,
	.tx_arr[3] = 0x02,
	.tx_arr[4] = 0x02,
	.tx_arr[5] = 0x02,
	.tx_arr[6] = 0x02,
	.tx_arr[7] = 0x02,
	.tx_arr[8] = 0x02
};

Request_auxiliary_prop_t aux_prop = 
{
	.net_addr = MERC_NET_ADDR,
	.code = READ_PROP_CODE,
	.prop_num = 0x14,
	.BWRI = BWRI_S_POWER_3P,
	.tx_arr[0] = MERC_NET_ADDR, 
	.tx_arr[1] = READ_PROP_CODE,
	.tx_arr[2] = 0x14, 
	.tx_arr[3] = BWRI_S_POWER_3P
};

Request_std_t fix_data = 
{
	.net_addr = MERC_NET_ADDR,
	.code = WRITE_PROP_CODE,
	.prop_num = 0x08,
	.tx_arr[0] = MERC_NET_ADDR,
	.tx_arr[1] = WRITE_PROP_CODE,
	.tx_arr[2] = 0x08
};

Request_std_t set_speed = 
{
	.net_addr = MERC_NET_ADDR,
	.code = WRITE_PROP_CODE,
	.prop_num = 0x15,
	.prop_expan = SPEED_9600,
	.tx_arr[0] = MERC_NET_ADDR,
	.tx_arr[1] = WRITE_PROP_CODE,
	.tx_arr[2] = 0x15,
	.tx_arr[3] = SPEED_9600,
};
/*-----------------------------------------------------*/
static const uint8_t	srCRCHi[256] = {
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40};

static const uint8_t srCRCLo[256] = {
0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06, 0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09, 0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4, 0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12,0x13, 0xD3,
0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3, 0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A, 0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED, 0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60, 0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F, 0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E, 0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71, 0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C, 0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B, 0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42, 0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

static uint16_t crc_upd(uint8_t tx_byte, uint16_t crc_old)
{
	uint8_t i = 0;
	
	union union_type
	{
		uint16_t old;
		uint8_t arr[2];
	} crc;
	
	crc.old = crc_old;
	
	i = crc.arr[1] ^ tx_byte;
	crc.arr[1] = crc.arr[0] ^ srCRCHi[i];
	crc.arr[0] = srCRCLo[i];
	
	return crc.old;
}

static uint16_t crc_get(uint8_t *tx_arr, uint8_t tx_len)
{
	const uint16_t crc_init = 0xFFFF;
	uint16_t crc = crc_upd(tx_arr[0], crc_init);
	
	for(uint8_t i = 1; i < tx_len; i++)
		crc = crc_upd(tx_arr[i], crc);
	
	return crc;
}

Status_merc_t Merc_open(uint8_t lvl, Response_all_t *resp_all)
{
	Status_merc_t status;
	
	switch(lvl)
	{
		case PORT_OPEN_LVL1:
			open_1.crc = crc_get(open_1.tx_arr, 9);
			buf_tx[0] = open_1.net_addr;
			buf_tx[1] = open_1.code;
			buf_tx[2] = open_1.access_lvl;
			memcpy((buf_tx + 3), &open_1.password, 6);
			buf_tx[9] = open_1.crc >> 8;
			buf_tx[10] = open_1.crc;
			Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 11, 1000);
			/*skip first rx byte because it always null*/
			while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 5, 1000) != HAL_OK)
			{
				Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 11, 1000);
			}
			memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
			resp_all->resp.net_addr = buf_rx[1];
			resp_all->resp.status = buf_rx[2];
			resp_all->resp.crc = buf_rx[3];
			resp_all->resp.crc = (resp_all->resp.crc << 8) | buf_rx[4];
			
			/*check if there was a rx data at all*/
			if(resp_all->resp.crc != 0)
				status = (Status_merc_t )resp_all->resp.status;
		break;	
			
		case PORT_OPEN_LVL2:
			open_2.crc = crc_get(open_2.tx_arr, 9);
			buf_tx[0] = open_2.net_addr;
			buf_tx[1] = open_2.code;
			buf_tx[2] = open_2.access_lvl;
			memcpy((buf_tx + 3), &open_2.password, 6);
			buf_tx[9] = open_2.crc >> 8;
			buf_tx[10] = open_2.crc;
			Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 11, 1000);
			/*skip first rx byte because it always null*/
			while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 5, 1000) != HAL_OK)
			{
				Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 11, 1000);
			}
			memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
			resp_all->resp.net_addr = buf_rx[1];
			resp_all->resp.status = buf_rx[2];
			resp_all->resp.crc = buf_rx[3];
			resp_all->resp.crc = (resp_all->resp.crc << 8) | buf_rx[4];
			
			/*check if there was a rx data at all*/
			if(resp_all->resp.crc != 0)
				status = (Status_merc_t )resp_all->resp.status;
		break;
	}
	return status;
}

Status_merc_t Merc_req(uint16_t req_name, Response_all_t *resp_all)
{
	Status_merc_t status;
	
	switch(req_name)
	{
		case NET_TEST_CMD:
			net_test.crc = crc_get(net_test.tx_arr, sizeof(net_test.tx_arr));
			buf_tx[0] = net_test.net_addr;
			buf_tx[1] = net_test.code;
			buf_tx[2] = net_test.crc >> 8;
			buf_tx[3] = net_test.crc;
			Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 4, 1000);
			/*skip first rx byte because it always null*/
			/*transmitt till answer*/
			while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 5, 1000) != HAL_OK)
			{
				Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 4, 1000);
			}
			memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
			resp_all->resp.net_addr = buf_rx[1];
			resp_all->resp.status = buf_rx[2];
			resp_all->resp.crc = buf_rx[3];
			resp_all->resp.crc = (resp_all->resp.crc << 8) | buf_rx[4];
				
			/*check if there was a rx data at all*/
			if(resp_all->resp.crc != 0)
				status = (Status_merc_t )resp_all->resp.status;
		break;
			
		case SER_NUM_CMD:
			ser_num_read.crc = crc_get(ser_num_read.tx_arr, 3);
			buf_tx[0] = ser_num_read.net_addr;
			buf_tx[1] = ser_num_read.code;
			buf_tx[2] = ser_num_read.prop_num;
			buf_tx[3] = ser_num_read.crc >> 8;
			buf_tx[4] = ser_num_read.crc;
			Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 5, 1000);
			/*skip first rx byte because it always null*/
			/*transmitt till answer*/
			while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 11, 1000) != HAL_OK)
			{
				Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 5, 1000);
			}
			memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
		    resp_all->resp.net_addr = buf_rx[1];
			/*add additional byte to rx*/
			memcpy(resp_all->resp.data, (uint8_t *)buf_rx + 2, 7); /*serial num and production date*/
			resp_all->resp.crc = buf_rx[9];
			resp_all->resp.crc = (resp_all->resp.crc << 8) | buf_rx[10];
				
			/*check if there was a rx data at all*/
			if(resp_all->resp.crc != 0)
				status = OK;
		break;
		
		case SET_SPEED_CMD:
			set_speed.crc = crc_get(set_speed.tx_arr, 4);
			buf_tx[0] = set_speed.net_addr;
			buf_tx[1] = set_speed.code;
			buf_tx[2] = set_speed.prop_num;
			buf_tx[3] = set_speed.prop_expan;
			buf_tx[4] = set_speed.crc >> 8;
			buf_tx[5] = set_speed.crc;
			Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 6, 1000);
			while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 5, 1000) != HAL_OK)
			{
				Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 6, 1000);
			}
			memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
	}
	return status;
	
}

uint32_t Merc_get_S_power(void)
{
	uint32_t full_power = 0;
	/*first of all send cmd to fix the data*/
	fix_data.crc = crc_get(fix_data.tx_arr, 3);
	buf_tx[0] = fix_data.net_addr;
	buf_tx[1] = fix_data.code;
	buf_tx[2] = fix_data.prop_num;
	buf_tx[3] = fix_data.crc >> 8;
	buf_tx[4] = fix_data.crc;
	Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 5, 1000);
	while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 5, 1000) != HAL_OK)
	{
		Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 5, 1000);
	}
	memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
	for(uint32_t i = 0; i < 10000; i++);
	/*-------------------------------------*/
	aux_prop.crc = crc_get(aux_prop.tx_arr, 4);
	buf_tx[0] = aux_prop.net_addr;
	buf_tx[1] = aux_prop.code;
	buf_tx[2] = aux_prop.prop_num;
	buf_tx[3] = aux_prop.BWRI;
	buf_tx[4] = aux_prop.crc >> 8;
	buf_tx[5] = aux_prop.crc;
	Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 6, 1000);
	/*skip first rx byte because it always null*/
	while(Mercury_Receive(&huart2, (uint8_t *)buf_rx, 20, 1000) != HAL_OK)
	{
		Mercury_Transmit(&huart2, (uint8_t *)buf_tx, 6, 1000);
	}
	memset(buf_tx, 0, sizeof(buf_tx) / sizeof(buf_tx[0]));
	full_power = to_power(buf_rx[2], buf_rx[4], buf_rx[5]);
	
	memset(buf_rx, 0, sizeof(buf_rx) / sizeof(buf_rx[0]));	
	
	return full_power;
}

Status_merc_t Mercury_init(void)
{
	Status_merc_t status;

	status = Merc_req(NET_TEST_CMD, &resp_all);
	if(status != OK)
		return status;
	HAL_Delay(5);

	status = Merc_req(SER_NUM_CMD, &resp_all);
	if(status != OK)
		return status;
	HAL_Delay(5);

	status = Merc_open(PORT_OPEN_LVL2, &resp_all);
	if(status != OK)
		return status;
	HAL_Delay(5);
/*Uncomment and set param in struct Request_std_t set_speed*/
//	status = Merc_req(SET_SPEED_CMD, &resp_all);
//	if(status != OK)
//		return status;
//	HAL_Delay(20);
	
	
	return OK;
}


