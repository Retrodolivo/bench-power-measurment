#include "tcp.h"

static uint16_t socket_status;
extern SD_t sd;
extern RTC_HandleTypeDef hrtc;
extern RTC_TimeTypeDef Time;
extern RTC_DateTypeDef Date;

void tcp_cmds_init(Tcp_device_t *tcp_device)
{
	/*commands to mcu via tcp*/
	
	/*
		get all power measurment values
	*/
	strcpy(tcp_device->cmd[0], "GET_ALL~");
	/*
		set time and data via timestamp
	*/
	/*cmd example "SET_DT": '0x96' '0xF0' '0xEF', '0x60' '~'*/
	strcpy(tcp_device->cmd[1], "SET_DT");
}

void tcp_rx_parse(Tcp_device_t *tcp_device)
{
	/*tx all power measurment values*/
	if(strcmp((char *)tcp_device->rx_data, tcp_device->cmd[0]) == 0)
	{
		sd_read_begin(sd.start_block_addr);
		for(uint32_t i = sd.start_block_addr; i < sd.block_addr; i++)
		{		
			sd_read_data(sd.block);
			tcp_send_msg(SOCKET_0, sd.block, sizeof(sd.block));
		}
		sd_read_end();
		sd_read_end();
	}
	if(strncmp((char *)tcp_device->rx_data, tcp_device->cmd[1], 6) == 0)
	{
		if(tcp_device->rx_data[10] != '~')
			return;
		
		uint8_t offset = 6; //lenght of "SET_DT"
		uint32_t temp = 0;
		uint32_t timestamp = 0;
		/*assemble 4 bytes of timestamp*/
		for(int8_t i = 3; i >= 0; i--)
    {
			temp = (uint32_t)tcp_device->rx_data[offset + i] << 8 * i;
			timestamp += temp;
		}
		RTC_FromEpoch(timestamp, &Time, &Date, 0);
	}
}

uint16_t tcp_server_init(uint8_t socket_num)
{	
	if((socket_status = socket(socket_num, Sn_MR_TCP, 5000, 0)) != 0)
		return socket_status;
	
	if((socket_status = listen(socket_num)) != SOCK_OK)
		return socket_status;
	
	return SOCK_OK;
}

uint16_t tcp_client_check(uint8_t socket_num)
{
	if((socket_status = getSn_SR(socket_num)) == SOCK_LISTEN)
		return socket_status;
	else if((socket_status = getSn_SR(socket_num)) == SOCK_ESTABLISHED)
		return socket_status;
	else
		return socket_status;
}
uint16_t tcp_send_msg(uint8_t socket_num, uint8_t *arr, uint16_t arr_size)
{
	if((socket_status = send(socket_num, arr, arr_size)) != (int16_t)sizeof(arr))
		return socket_status;
	else
		return SOCK_OK;
}

uint16_t tcp_disconnect(uint8_t socket_num)
{
	if((socket_status = disconnect(socket_num)) != SOCK_OK)
		return socket_status;
	if((socket_status = close(socket_num)) != SOCK_OK)
		return socket_status;
	else
		return SOCK_OK;
}

