/* USER CODE BEGIN Header */
/*
	"Контроллер счетчика электроэнергии".
	Об устройстве:
	Осуществляет сбор и передачу информации о потребляемой электроэнергии.
	Сбор происходит путем опроса счетчика электроэнергии Меркурий 236 по интерфейсу RS-485 и записью данных на SD карту. Передача осуществляется по Etherenet по протоколу TCP.
	Также имеются три гальванически развязанных входа для определения хода работы станка (Red, Yellow, Green).
	------------------------------
	STM32->UART->MAX3485->RS-485
	STM32->SPI->W5500->Ethernet
	STM32->SPI->Micro SD
	------------------------------
	Частота записи данных - 120 р/мин
	Структура данных - 56 точек [Значение мощности - 4 байта, Время(timestamp) - 4 байта, Статус станка(Red, Yellow, Green) - 1 байт] = 56 * 9 = 512 байт - 1 блок памяти SD карты

	Контроллер работает как TCP сервер. При установлении связи с клиентом передаются инфо о контроллере, а затем все, накопленные на SD, данные.
	В случае перезагрузки контроллера запись на SD продолжается в первый найденный свободный сектор(см. функцию init_routine()).
	(Находит ооочень долго, поэтому в будущем планирую использовать EEPROM для хранения указателя адреса сектора SD карты).

	Либа для W5500(Ethernet контроллер) - здесь https://github.com/Wiznet/ioLibrary_Driver
	При подключении счетчика Меркурий 236 необходимо изменить дефайн MERC_NET_ADDR в хэдере mercury.h
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "time.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
	RED,
	YELLOW,
	GREEN
} Bench_state_t;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
RTC_HandleTypeDef hrtc;

SPI_HandleTypeDef hspi1;
SPI_HandleTypeDef hspi3;

TIM_HandleTypeDef htim16;
TIM_HandleTypeDef htim17;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */
extern RTC_TimeTypeDef Time;
extern RTC_DateTypeDef Date;

time_t timestamp;
uint32_t curr_block_num;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_RTC_Init(void);
static void MX_SPI3_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM16_Init(void);
static void MX_TIM17_Init(void);
/* USER CODE BEGIN PFP */
//static void uint32_to_uint8_arr(uint32_t *u32, uint8_t *arr)
//{
//	arr[0] = *u32 & 0x000000FF;
//	arr[1] = (*u32 & 0x0000FF00) >> 8;
//	arr[2] = (*u32 & 0x00FF0000) >> 16;
//	arr[3] = (*u32 & 0xFF000000) >> 24;
//}
void init_routine(void);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
Response_all_t resp_all;
Status_merc_t status_merc;
Power_t power;
Bench_state_t bench_state;
uint8_t merc_addr = MERC_NET_ADDR;

w5500chip_t w5500;

SD_t sd =
{
	.blocks_num = 0,
	.start_block_addr = 0x00ABCD,
	.block_addr = 0x00ABCD, //equal .start_block_addr
	.after_reboot_block_addr = 0
};
int8_t status_sd = 0;

uint32_t timestamp_arr[64];
Tcp_device_t tcp_device;
/*counters and flags*/
uint32_t i = 0;
uint8_t sys_msg = NOT_SENT;
uint8_t tcp_status = 0;
uint8_t tcp_init = 0;

//write cycles to data arrays before moving data to sd card block
uint8_t max_cycles = (sizeof(sd.block) / (sizeof(power.S) + 4 + 1)); //56 cycles (power + timestamp + bench state)

/**********/
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	/*Power counter polling. 0.5s period, So f = 120Hz*/
	if(htim->Instance == TIM16)
	{
		if(i < max_cycles)
		{
			power.S = Merc_get_S_power();
			power.S_arr[i] = power.S;
			power.bench_states[i] = bench_state;
			RTC_GetDateTime(&Time, &Date);

			timestamp = RTC_ToEpoch(&Time, &Date);
			timestamp_arr[i] = timestamp;
			i++;
		}
		if(i == max_cycles)		//if data for block is ready
		{
			/*fill sd.block with power data first, timestamp and bench state after*/
			for(uint8_t i = 0; i < max_cycles; i++)
			{
				memcpy(&sd.block[i * sizeof(power.S_arr[0])], &power.S_arr[i], sizeof(power.S_arr[0]));
				/*starting from the end of power data, so from 56 * 4 position. */
				memcpy(&sd.block[i * sizeof(timestamp_arr[0]) + max_cycles * sizeof(power.S_arr[0])],
							 &timestamp_arr[i], sizeof(timestamp_arr[0]));
				memcpy(&sd.block[i * sizeof(power.bench_states[0]) + max_cycles * (sizeof(power.S_arr[0]) + sizeof(timestamp_arr[0]))],
							 &power.bench_states[i], sizeof(power.bench_states[0]));
			}
			memset(power.S_arr, 0, sizeof(power.S_arr) / sizeof(power.S_arr[0]));
			memset(timestamp_arr, 0, sizeof(timestamp_arr) / sizeof(timestamp_arr[0]));
			memset(power.bench_states, 0, sizeof(power.bench_states) / sizeof(power.bench_states[0]));

			sd_write_single_block(sd.block_addr, sd.block);
			if(sd.block_addr != sd.blocks_num)
				sd.block_addr += 1;
			else
				sd.block_addr = sd.start_block_addr;

			i = 0;
		}
	}
	/*ETH controller polling. 1s period*/
	if(htim->Instance == TIM17)
	{
		/*Firstly send info about controller/bench*/
		if(tcp_client_check(SOCKET_0) == SOCK_ESTABLISHED && sys_msg == NOT_SENT)
		{
			/*send sys messages*/
//			tcp_send_msg(SOCKET_0, w5500_netinfo.ip, sizeof(w5500_netinfo.ip));
//			tcp_send_msg(SOCKET_0, &merc_addr, sizeof(merc_addr));
			sys_msg = SENT;
			/*an amount of sd blocks would be transmit*/
			uint32_t blocks_cnt = sd.block_addr - sd.start_block_addr;
			/*slice uint32 to uint8[4] and send arr*/
//			uint8_t blocks_num_arr[4];
//			uint32_to_uint8_arr(&blocks_cnt, blocks_num_arr);
//			tcp_send_msg(SOCKET_0, blocks_num_arr, sizeof(blocks_num_arr));
		}
		/*Then send data according to cmd*/
		if(tcp_client_check(SOCKET_0) == SOCK_ESTABLISHED && sys_msg == SENT)
		{
			tcp_status = recv(SOCKET_0, tcp_device.rx_data, sizeof(tcp_device.rx_data));
			tcp_rx_parse(&tcp_device);
			memset(tcp_device.rx_data, 0, sizeof(tcp_device.rx_data));
		}
		if(tcp_client_check(SOCKET_0) != SOCK_ESTABLISHED)
		{
			tcp_server_init(SOCKET_0);
			sys_msg = NOT_SENT;
		}
	}
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_SPI1_Init();
  MX_RTC_Init();
  MX_SPI3_Init();
  MX_USART2_UART_Init();
  MX_TIM16_Init();
  MX_TIM17_Init();
  /* USER CODE BEGIN 2 */
  HAL_Delay(10);
  init_routine();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_LOW);
  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE|RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2|RCC_PERIPHCLK_RTC;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief RTC Initialization Function
  * @param None
  * @retval None
  */
static void MX_RTC_Init(void)
{

  /* USER CODE BEGIN RTC_Init 0 */

  /* USER CODE END RTC_Init 0 */

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */
  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 127;
  hrtc.Init.SynchPrediv = 255;
  hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
  hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
  hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;
  if (HAL_RTC_Init(&hrtc) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN Check_RTC_BKUP */

  /* USER CODE END Check_RTC_BKUP */

  /** Initialize RTC and set the Time and Date
  */
  sTime.Hours = 0x0;
  sTime.Minutes = 0x0;
  sTime.Seconds = 0x0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 0x1;
  sDate.Year = 0x0;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BCD) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */

}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief SPI3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI3_Init(void)
{

  /* USER CODE BEGIN SPI3_Init 0 */

  /* USER CODE END SPI3_Init 0 */

  /* USER CODE BEGIN SPI3_Init 1 */

  /* USER CODE END SPI3_Init 1 */
  /* SPI3 parameter configuration*/
  hspi3.Instance = SPI3;
  hspi3.Init.Mode = SPI_MODE_MASTER;
  hspi3.Init.Direction = SPI_DIRECTION_2LINES;
  hspi3.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi3.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi3.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi3.Init.NSS = SPI_NSS_SOFT;
  hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
  hspi3.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi3.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi3.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi3.Init.CRCPolynomial = 7;
  hspi3.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi3.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI3_Init 2 */

  /* USER CODE END SPI3_Init 2 */

}

/**
  * @brief TIM16 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM16_Init(void)
{

  /* USER CODE BEGIN TIM16_Init 0 */

  /* USER CODE END TIM16_Init 0 */

  /* USER CODE BEGIN TIM16_Init 1 */

  /* USER CODE END TIM16_Init 1 */
  htim16.Instance = TIM16;
  htim16.Init.Prescaler = 35999;
  htim16.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim16.Init.Period = 1000;
  htim16.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim16.Init.RepetitionCounter = 0;
  htim16.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim16) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM16_Init 2 */

  /* USER CODE END TIM16_Init 2 */

}

/**
  * @brief TIM17 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM17_Init(void)
{

  /* USER CODE BEGIN TIM17_Init 0 */

  /* USER CODE END TIM17_Init 0 */

  /* USER CODE BEGIN TIM17_Init 1 */

  /* USER CODE END TIM17_Init 1 */
  htim17.Instance = TIM17;
  htim17.Init.Prescaler = 35999;
  htim17.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim17.Init.Period = 2000;
  htim17.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim17.Init.RepetitionCounter = 0;
  htim17.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim17) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM17_Init 2 */

  /* USER CODE END TIM17_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 9600;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_RS485Ex_Init(&huart2, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOF_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, SD_CS_Pin|W5500_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(W5500_RST_GPIO_Port, W5500_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : SD_CS_Pin W5500_CS_Pin */
  GPIO_InitStruct.Pin = SD_CS_Pin|W5500_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : STATE_RED_Pin STATE_YELLOW_Pin STATE_GREEN_Pin */
  GPIO_InitStruct.Pin = STATE_RED_Pin|STATE_YELLOW_Pin|STATE_GREEN_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOE, &GPIO_InitStruct);

  /*Configure GPIO pin : W5500_RST_Pin */
  GPIO_InitStruct.Pin = W5500_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_MEDIUM;
  HAL_GPIO_Init(W5500_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : W5500_INT_Pin */
  GPIO_InitStruct.Pin = W5500_INT_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(W5500_INT_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

}

/* USER CODE BEGIN 4 */
/*Falling edge interrupt*/
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
	if(GPIO_Pin == STATE_RED_Pin)
		bench_state = RED;
	if(GPIO_Pin == STATE_YELLOW_Pin)
		bench_state = YELLOW;
	if(GPIO_Pin == STATE_GREEN_Pin)
		bench_state = GREEN;
}

void init_routine(void)
{
	/*Make 0 bits of SubPriority, so only PreemptPriority bits are making impact on Priority. F1 - F7 feature*/
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);

	HAL_NVIC_SetPriority(TIM16_IRQn, 0, 0);		//power counter polling timer
	HAL_NVIC_SetPriority(TIM17_IRQn, 1, 0);		//ETH controller polling timer
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 2, 0); //bench states
	HAL_NVIC_SetPriority(EXTI9_5_IRQn, 2, 0);

	/*---------SD CARD-------------*/
	status_sd = sd_init();
	//get total num of sd blocks of 512 byte
	status_sd = sd_get_blocks_num(&sd.blocks_num);
	uint8_t blank_block[512];
	memset(blank_block, 0, sizeof(blank_block));

//	//do erase of sd card blocks
//	sd_write_begin(sd.block_addr);
//	for(uint32_t i = 0; i < 500; i++)
//		sd_write_data(blank_block);
//	sd_write_end();

	//check last write sector addr. Should find first empty blank;
	sd.after_reboot_block_addr = sd.start_block_addr;
	uint8_t empty = false;
	sd_read_begin(sd.start_block_addr);
	while(!empty)
	{
		empty = true;
		sd_read_data(sd.block);
		for(uint32_t i = 0; i < 512; i++)
		{
			if(sd.block[i] != 0)
			{
				empty = false;
				//break;
			}
		}
		sd.after_reboot_block_addr++;
	}
	sd_read_end();
	sd.after_reboot_block_addr--;
	sd.block_addr = sd.after_reboot_block_addr; //from here we start write next time

	/*---------W5500 ETHERNET-------------*/
	w5500_init(&w5500);
	while((tcp_init = tcp_server_init(SOCKET_0) != 1))
	{
	}
	tcp_cmds_init(&tcp_device);

	/*---------MERCURY-------------*/
	status_merc = Mercury_init();
	/*-----------------------------*/
	/*enable timer interrupt for mercury requests*/
	HAL_TIM_Base_Start_IT(&htim16);
	/*enable timer interrupt for ethernet controller*/
	HAL_TIM_Base_Start_IT(&htim17);

}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
