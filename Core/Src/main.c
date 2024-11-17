/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ssd1306_conf_template.h"
#include "ssd1306.h"
#include "ssd1306_fonts.h"
#include "stdio.h"
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

RTC_HandleTypeDef hrtc;

TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_RTC_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
static void MX_USART2_UART_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t day, month, year, second, minute, hour, alarm_triggered=0,alarm_off=0;
char time[15], date[15];
char welcome[]="Welcome\nPlease select the operation you want to perform\n";
char options[]="1: Set Time\n2: Set Date\n3: Set a new alarm\n4: Return back to main menu\n x: Cancel";
char set_time[]="Please enter a new time as hh.mm.ss\n";
char set_date[]="Please enter a new date as dd.mm.yy\n";
char set_alarm[]="Please enter a new alarm as hh.mm.ss\n";
uint8_t rx_buffer[30], rx_temp[1], rx_index=0; //rx temp is used to get the data byte by byte from the user with usart call back function
RTC_AlarmTypeDef sAlarm={0}; //structures to change time, date and alarm values
RTC_TimeTypeDef sTime={0};
RTC_DateTypeDef sDate = {0};
uint32_t last_time=0; //for debouncing
uint8_t user_options=0,menu_counter=0, alarm_day=2; //menu variables to keep the track and alarm_day variable to match alarm week day and date


/**
 * @brief Retrieves the current time from the RTC and assigns it to global variables.
 */

void get_rtc_time()
{
    RTC_TimeTypeDef sTime = {0};

    // Get the current time from the RTC
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);

    // Assign the retrieved time to global variables
    second = sTime.Seconds;
    minute = sTime.Minutes;
    hour = sTime.Hours;
}

/**
 * @brief Retrieves the current date from the RTC and assigns it to global variables.
 */
void get_rtc_date()
{
    RTC_DateTypeDef sDate;

    // Get the current date from the RTC
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    // Assign the retrieved date to global variables
    day = sDate.Date;
    month = sDate.Month;
    year = sDate.Year;

}

void HAL_RTC_AlarmAEventCallback(RTC_HandleTypeDef *hrtc)
{

	  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_SET);
	  alarm_triggered=1;
}

void oled_display()
{
    // Format the time and store it in the time array
    sprintf(time, "%02u:%02u:%02u", hour, minute, second);

    // Display the time on the OLED screen
    ssd1306_SetCursor(30, 20);
    ssd1306_WriteString(time, Font_7x10, Black);
    ssd1306_UpdateScreen();

    // Format the date and store it in the date array
    sprintf(date, "%02u:%02u:20%02u", day, month, year);

    // Display the date on the OLED screen
    ssd1306_SetCursor(30, 35);
    ssd1306_WriteString(date, Font_7x10, Black);
    ssd1306_UpdateScreen();
}

void rtc_alarm_config(void)
{
	uint8_t hour_n=(rx_buffer[0] - '0') * 10 + (rx_buffer[1] - '0'); //hour minute and second data is taken from the user
	uint8_t minute_n=(rx_buffer[3] - '0') * 10 + (rx_buffer[4] - '0');
    uint8_t second_n=(rx_buffer[6] - '0') * 10 + (rx_buffer[7] - '0');
    if(hour_n<24 && minute_n<59 && second_n<59)
    {
		sAlarm.AlarmTime.Hours = hour_n;  //alarm configurations
		sAlarm.AlarmTime.Minutes =minute_n;
		sAlarm.AlarmTime.Seconds = second_n;
		sAlarm.AlarmTime.SubSeconds = 0;
		sAlarm.AlarmTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
		sAlarm.AlarmTime.StoreOperation = RTC_STOREOPERATION_RESET;
		sAlarm.AlarmMask = RTC_ALARMMASK_NONE;
		sAlarm.AlarmSubSecondMask = RTC_ALARMSUBSECONDMASK_ALL;
		sAlarm.AlarmDateWeekDaySel = RTC_ALARMDATEWEEKDAYSEL_DATE;
		sAlarm.AlarmDateWeekDay = alarm_day;
		sAlarm.Alarm = RTC_ALARM_A;
		HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN); //alarm activation
		char message[25];
		sprintf(message,"Alarm is set to %u.%u.%u\n",hour_n,minute_n,second_n);
		HAL_UART_Transmit(&huart2, (uint8_t*)message, sizeof(message), 200);
    }
    else{
    	HAL_UART_Transmit(&huart2, (uint8_t*)("Please enter a valid value\n"), sizeof("Please enter a valid value"), 200);
    }
}
void rtc_time_config(void)
{
	uint8_t hour_n=(rx_buffer[0] - '0') * 10 + (rx_buffer[1] - '0'); //hour minute and second data is taken from the user
	uint8_t minute_n=(rx_buffer[3] - '0') * 10 + (rx_buffer[4] - '0');
    uint8_t second_n=(rx_buffer[6] - '0') * 10 + (rx_buffer[7] - '0');
    if(hour_n<24 && minute_n<59 && second_n<59)
    {
    	sTime.Hours = hour_n;
    	sTime.Minutes = minute_n;
    	sTime.Seconds = second_n;
    	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
    	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
    	HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN); //time is set
		char message[25];
		sprintf(message,"Time is set to %u.%u.%u\n",hour_n,minute_n,second_n);
		HAL_UART_Transmit(&huart2, (uint8_t*)message, sizeof(message), 200);
    }
    else{
    	HAL_UART_Transmit(&huart2, (uint8_t*)("Please enter a valid value\n"), sizeof("Please enter a valid value"), 200);
    }
}

void rtc_date_config(void)
{
	uint8_t day_n=(rx_buffer[0] - '0') * 10 + (rx_buffer[1] - '0');
	uint8_t month_n=(rx_buffer[3] - '0') * 10 + (rx_buffer[4] - '0');
    uint8_t year_n=(rx_buffer[6] - '0') * 10 + (rx_buffer[7] - '0');
    if(day_n<32 && month_n<13 && year_n<100)
    {
    	  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
    	  sDate.Month = month_n;
    	  sDate.Date = day_n;
    	  sDate.Year = year_n;
    	  alarm_day=day_n;
    	  HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  		  char message[25];
  		  sprintf(message,"Date is set to %u.%u.%u\n",day_n,month_n,year_n);
  		  HAL_UART_Transmit(&huart2, (uint8_t*)message, sizeof(message), 200);
    }
    else{
    	HAL_UART_Transmit(&huart2, (uint8_t*)("Please enter a valid value\n"), sizeof("Please enter a valid value"), 200);
    }
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	if(huart->Instance == USART2) //it checks if the call back is made by usart2
		{
		rx_buffer[rx_index++]=rx_temp[0]; //taken bytes are written in rx_buffer
		if(rx_temp[0]=='\n') //end of the data
		{
			menu_counter++;
			switch (menu_counter){
			case 1 :
				user_options=rx_buffer[0]-'0';
				switch (user_options){
				case 1:
					HAL_UART_Transmit(&huart2, (uint8_t*)set_time, sizeof(set_time), 200);
					break;
				case 2:
					HAL_UART_Transmit(&huart2, (uint8_t*)set_date, sizeof(set_date), 200);
					break;
				case 3:
					HAL_UART_Transmit(&huart2, (uint8_t*)set_alarm, sizeof(set_alarm), 200);
					break;
				case 4:
					HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
					HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
					menu_counter=0;
				default:
					HAL_UART_Transmit(&huart2, (uint8_t*)"Please enter a valid value\n", sizeof("Please enter a valid value\n"), 200);
					  HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
					  HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
					  menu_counter=0;
					break;
				}
				break;

			case 2 :
					switch(user_options){

					case 1:
						rtc_time_config();
						break;
					case 2:
						rtc_date_config();
						break;
					case 3:
						rtc_alarm_config();
						break;
					case 4:
						HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
						HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
						menu_counter=0;
					default:
						HAL_UART_Transmit(&huart2, (uint8_t*)"Please enter a valid value\n", sizeof("Please enter a valid value\n"), 200);
						  HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
						  HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
						break;
					}
				break;
			}
			if(menu_counter==2)
			{
				menu_counter=0;
				HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
				HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
			}
		rx_index=0;
		memset(rx_buffer,'\0',sizeof(rx_buffer));
		}
		HAL_UART_Receive_IT(&huart2, rx_temp, 1);
		}
}

void EXTI9_5_IRQHandler(void) //button interrupt to turn off the alarm
{
  /* USER CODE BEGIN EXTI9_5_IRQn 0 */

  /* USER CODE END EXTI9_5_IRQn 0 */
  HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_6);
  /* USER CODE BEGIN EXTI9_5_IRQn 1 */
  uint32_t current_time=HAL_GetTick();
  if(current_time-last_time>200) //debouncing
  {
	  last_time=current_time;
	  alarm_off=1;
  }

  /* USER CODE END EXTI9_5_IRQn 1 */
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
  MX_RTC_Init();
  MX_I2C1_Init();
  MX_TIM2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  ssd1306_Init();
  ssd1306_Fill(White);
  ssd1306_UpdateScreen();
  HAL_UART_Transmit(&huart2, (uint8_t*)welcome, sizeof(welcome), 200);
  HAL_UART_Transmit(&huart2, (uint8_t*)options, sizeof(options), 200);
  HAL_NVIC_EnableIRQ(USART2_IRQn); //usart interrupt activation
  HAL_NVIC_SetPriority(USART2_IRQn, 2, 0);
  HAL_UART_Receive_IT(&huart2, rx_temp, 1); //interrupt start
  sAlarm.AlarmDateWeekDay=2; //default value for alarm date week day
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	  if(alarm_triggered)
	  {
		  alarm_triggered=0;
		  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1); //pwm generation for buzzer
		  ssd1306_Fill(White);
		  ssd1306_UpdateScreen();
		  ssd1306_SetCursor(30, 20);
		  ssd1306_WriteString("alarm", Font_7x10, Black);
		  ssd1306_UpdateScreen();
		  while(!alarm_off); //alarm rings until the button is pressed
		  HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1); //buzzer stops
		  ssd1306_Fill(White);
		  ssd1306_UpdateScreen();
		  alarm_off=0;
	  }
	  get_rtc_time();
	  get_rtc_date();
	  oled_display();

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

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

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
 // RTC_AlarmTypeDef sAlarm = {0};

  /* USER CODE BEGIN RTC_Init 1 */

  /* USER CODE END RTC_Init 1 */

  /** Initialize RTC Only
  */
  hrtc.Instance = RTC;
  hrtc.Init.HourFormat = RTC_HOURFORMAT_24;
  hrtc.Init.AsynchPrediv = 99;
  hrtc.Init.SynchPrediv = 3999;
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
  sTime.Hours = 13;
  sTime.Minutes = 47;
  sTime.Seconds = 0;
  sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
  sTime.StoreOperation = RTC_STOREOPERATION_RESET;
  if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }
  sDate.WeekDay = RTC_WEEKDAY_MONDAY;
  sDate.Month = RTC_MONTH_JANUARY;
  sDate.Date = 2;
  sDate.Year = 24;

  if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
  {
    Error_Handler();
  }

  /* USER CODE BEGIN RTC_Init 2 */

  /* USER CODE END RTC_Init 2 */
}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 1;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 8399;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 4199;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  if (HAL_UART_Init(&huart2) != HAL_OK)
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
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /*Configure GPIO pin : PD5 */
  GPIO_InitStruct.Pin = GPIO_PIN_6;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
