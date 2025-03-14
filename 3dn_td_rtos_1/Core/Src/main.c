/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "cmsis_os.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define TASK1_PRIORITY 1
#define TASK2_PRIORITY 2
#define TASK1_STACK_DEPTH 250
#define TASK2_STACK_DEPTH 250

#define TASK_UART1_STACK_DEPTH 256
#define TASK_UART1_PRIORITY 1
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static TaskHandle_t task_uart1_handle = NULL;
static SemaphoreHandle_t uart1_rx_semaphore = NULL;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
int __io_putchar(int chr)
{
	HAL_UART_Transmit(&huart1, (uint8_t*)&chr, 1, HAL_MAX_DELAY);
	return chr;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t HigherPriorityTaskWoken;

	if (USART1 == huart->Instance)
	{
		// On veut débloquer la tâche
		xSemaphoreGiveFromISR(uart1_rx_semaphore, &HigherPriorityTaskWoken);

		// On est en interruption, le contexte est *différent* d'une tache
		// Donc le changement de contexte s'effectue différemment
		// Apres cette fonction, HigherPriorityTaskWoken vaut 1 si l'on a réveillé une tache plus prioritaire
	}

	// Appelle une version modifiée du scheduler si une tache plus prioritaire a été appelée
	portYIELD_FROM_ISR(HigherPriorityTaskWoken);
}

void task_uart1(void * unused)
{
	printf("Entering Task UART1\r\n");

	uint8_t uart1_chr;

	uart1_rx_semaphore = xSemaphoreCreateBinary();
	if (uart1_rx_semaphore == NULL)
	{
		printf("Could not create uart1_rx_semaphore\r\n");
		Error_Handler();
	}

	// arme une première fois la réception
	HAL_UART_Receive_IT(&huart1, &uart1_chr, 1);

	for(;;)
	{
		// On veut bloquer la tâche jusqu'à interruption
		if (xSemaphoreTake(uart1_rx_semaphore, 1000) == pdFALSE)
		{
			printf("plus vite!\r\n");
		}
		else {
			HAL_UART_Transmit(&huart1, &uart1_chr, 1, HAL_MAX_DELAY);	// echo
			HAL_UART_Receive_IT(&huart1, &uart1_chr, 1);	// réarme la réception

			/* Fonction qui prend du temps à exécuter */
		}
	}
}

#ifdef NOTIFICATION
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	BaseType_t HigherPriorityTaskWoken;

	if (USART1 == huart->Instance)
	{
		// On veut débloquer la tâche
		vTaskNotifyGiveFromISR(task_uart1_handle, &HigherPriorityTaskWoken);
		// On est en interruption, le contexte est *différent* d'une tache
		// Donc le changement de contexte s'effectue différemment
		// Apres cette fonction, HigherPriorityTaskWoken vaut 1 si l'on a réveillé une tache plus prioritaire
	}

	// Appelle une version modifiée du scheduler si une tache plus prioritaire a été appelée
	portYIELD_FROM_ISR(HigherPriorityTaskWoken);
}

void task_uart1(void * unused)
{
	printf("Entering Task UART1\r\n");

	uint8_t uart1_chr;

	// arme une première fois la réception
	HAL_UART_Receive_IT(&huart1, &uart1_chr, 1);

	for(;;)
	{
		// On veut bloquer la tâche jusqu'à interruption
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		HAL_UART_Transmit(&huart1, &uart1_chr, 1, HAL_MAX_DELAY);	// echo
		HAL_UART_Receive_IT(&huart1, &uart1_chr, 1);	// réarme la réception

		/* Fonction qui prend du temps à exécuter */

	}
}
#endif

#ifdef TD1
int variable_globale = 0;

void task_code_1(void * unused)
{
	printf("On rentre dans la Task 1\r\n");

	for(;;)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		vTaskDelay(100);
	}

	// Il ne faut pas retourner de cette fonction
	// Parce que c'est une tache
}

void task_code_2(void * unused)
{
	printf("On rentre dans la Task 2\r\n");

	int i = 0;

	for(;;)
	{
		printf("Task2 : i = %d\r\n", i);
		i++;
		vTaskDelay(1000);
	}
}
#endif
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
	MX_USART1_UART_Init();
	/* USER CODE BEGIN 2 */
	printf("\r\n===== TD RTOS 1 =====\r\n");

	if (xTaskCreate(task_uart1, "UART1", TASK_UART1_STACK_DEPTH, NULL, TASK_UART1_PRIORITY, &task_uart1_handle) != pdPASS)
	{
		printf("Could not allocate Task UART 1\r\n");
		Error_Handler();
	}

#ifdef TD1
	BaseType_t ret;

	ret = xTaskCreate(task_code_1, "Task 1", TASK1_STACK_DEPTH, NULL, TASK1_PRIORITY, NULL);
	if (ret != pdPASS)
	{
		printf("Could not allocate Task 1\r\n");
		Error_Handler();
	}

	ret = xTaskCreate(task_code_2, "Task 2", TASK2_STACK_DEPTH, NULL, TASK2_PRIORITY, NULL);
	if (ret != pdPASS)
	{
		printf("Could not allocate Task 2\r\n");
		Error_Handler();
	}
#endif

	vTaskStartScheduler();	// Appelle l'OS (avec une fonction de freertos
	/* USER CODE END 2 */

	/* Call init function for freertos objects (in cmsis_os2.c) */
	MX_FREERTOS_Init();

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		HAL_Delay(100);

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
	RCC_OscInitStruct.PLL.PLLM = 25;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 2;
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}

	/** Activate the Over-Drive mode
	 */
	if (HAL_PWREx_EnableOverDrive() != HAL_OK)
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

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		Error_Handler();
	}
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
