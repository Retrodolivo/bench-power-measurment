#ifndef  _TCP_H_
#define  _TCP_H_

#include "main.h"

#define SOCKET_0	0
#define SOCKET_1	1
#define SOCKET_2	2
#define SOCKET_3	3
#define SOCKET_4	4
#define SOCKET_5	5
#define SOCKET_6	6
#define SOCKET_7	7

/*msg states*/
#define SENT	1
#define NOT_SENT 0

#define TCP_RX_DATA_SIZE		50
#define TCP_TX_DATA_SIZE		50
#define NUMBER_OF_CMDS			5
#define CMD_LENGHT					20

typedef struct
{
	char cmd[NUMBER_OF_CMDS][CMD_LENGHT];
	uint8_t rx_data[TCP_RX_DATA_SIZE];
	uint8_t tx_data[TCP_TX_DATA_SIZE];
} Tcp_device_t;

uint16_t tcp_server_init(uint8_t socket_num);
void tcp_cmds_init(Tcp_device_t *tcp_device);
void tcp_rx_parse(Tcp_device_t *tcp_device);
uint16_t tcp_client_check(uint8_t socket_num);
uint16_t tcp_send_msg(uint8_t socket_num, uint8_t *arr, uint16_t arr_size);
uint16_t tcp_disconnect(uint8_t socket_num);

#endif /*_TCP_H_*/
