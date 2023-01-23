#ifndef _MERCURY_H_
#define _MERCURY_H_

#include "main.h"

#define MERC_NET_ADDR					0x59 		/*last 2 digits of serial number*/
#define TRANS_TIMEOUT					5			/*5ms for 9600 baud*/
/*------------REQUESTS_NAMES----------------*/
#define NET_TEST_CMD					0
#define SER_NUM_CMD						1
#define S_POWER_READ_CMD				2
#define SET_SPEED_CMD					3

#define PORT_OPEN_LVL1					0
#define PORT_OPEN_LVL2					1
/*------------REQUESTS_CODES----------------*/
#define NET_TEST_CODE					0x00
#define NET_OPEN_CODE					0x01
#define	NET_CLOSE_CODE					0x02
/*Writing prop*/
#define WRITE_PROP_CODE					0x03
#define WRITE_CAL_COEF_CODE				0x07
/*Reading prop*/
#define READ_TIME_ARR_CODE				0x04
#define READ_ENRG_YEAR_CODE				0x05
#define READ_PROP_CODE					0x08
#define	READ_PROP_POW_ARR_CODE		 	0x15
#define READ_RATER_STATUS_CODE		 	0x17
#define READ_LOAD_CTRL_STATUS_CODE 		0x18
/*------------------------------------------*/
#define BWRI_P_POWER_3P					0x00
#define BWRI_Q_POWER_3P					0x04
#define BWRI_S_POWER_3P					0x08
/*-------------------------------------------------*/
#define SPEED_9600						0x00
#define SPEED_56700						0x08
#define SPEED_115200					0x09

/*--------------REQUESTS----------------*/
typedef struct
{
	uint8_t net_addr;
	uint8_t code;
	uint16_t crc;
	uint8_t tx_arr[2]; //packet that will trans exclude crc 2 bytes
} Req_net_test_t;

typedef struct
{
	uint8_t net_addr;
	uint8_t code;
	uint8_t access_lvl;
	uint8_t password[6];
	uint8_t tx_arr[9]; //packet that will trans exclude crc 2 bytes
	uint16_t crc;
} Request_open_t;

typedef struct
{
	uint8_t net_addr;
	uint8_t code;
	uint8_t prop_num;
	uint8_t prop_expan;
	uint8_t prop[3];
	uint8_t tx_arr[7]; //packet that will trans exclude crc 2 bytes
	uint16_t crc;
} Request_std_t;

typedef struct
{
	uint8_t net_addr;
	uint8_t code;
	uint8_t prop_num;
	uint8_t BWRI;
	uint8_t tx_arr[4];
	uint16_t crc;
} Request_auxiliary_prop_t;

typedef struct
{
	uint8_t net_addr;
	uint8_t status;
	uint8_t data[16];
	uint16_t crc;
} Response_t;

typedef struct
{
	Response_t resp;
	Response_t resp_pow_S;
	
} Response_all_t;

typedef struct
{
	uint32_t S;
	uint32_t P;
	uint32_t Q;
	uint32_t S_arr[64];
	uint8_t bench_states[64];
} Power_t;

typedef enum
{
	OK = 				0x00,
	INVALID_CMD = 		0x01,
	INTERNAL_ERR =		0x02,
	LVL_LACK = 			0x03,
	TIMER_ERR =			0x04,
	CH_CLOSED = 		0x05,
	
	STM_ERR = 			0xFF
} Status_merc_t;



HAL_StatusTypeDef Mercury_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);

Status_merc_t Merc_req(uint16_t req_name, Response_all_t *resp_all);
Status_merc_t Merc_open(uint8_t lvl, Response_all_t *resp_all);
uint32_t Merc_get_S_power(void);
Status_merc_t Mercury_init(void);
#endif /*_MERCURY_H_*/
