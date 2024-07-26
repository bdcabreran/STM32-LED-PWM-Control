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
#include "led_animation.h"
#include "led_transition_manager.h"
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
TIM_HandleTypeDef htim2;

UART_HandleTypeDef huart2;

/* USER CODE BEGIN PV */

#define DEBUG_BUFFER_SIZE 100
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// Custom printf-like function
void UART_printf(const char* fmt, ...)
{
    char    buffer[DEBUG_BUFFER_SIZE];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, DEBUG_BUFFER_SIZE, fmt, args);
    va_end(args);
    HAL_UART_Transmit(&huart2, (uint8_t*)buffer, strlen(buffer), HAL_MAX_DELAY);
}

void PWM_SetDutyCycle_CH1(uint16_t dutyCycle)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dutyCycle);
}

void PWM_SetDutyCycle_CH2(uint16_t dutyCycle)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, dutyCycle);
}

void PWM_SetDutyCycle_CH3(uint16_t dutyCycle)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, dutyCycle);
}

void PWM_SetDutyCycle_CH4(uint16_t dutyCycle)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_4, dutyCycle);
}

void PWM_Start(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_4);
}

void PWM_Stop(void)
{
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_2);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_3);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_4);
}

#define MAX_DUTY_CYCLE 1000

// Initialize RGB configuration with function pointers
// PWM_Single_t ledPwmConfig = {
//     .led = {.setDutyCycle = PWM_SetDutyCycle_CH1}
// };

// PWM_Dual_t ledPwmConfig = {
//     .led1 = {.setDutyCycle = PWM_SetDutyCycle_CH1},
//     .led2 = {.setDutyCycle = PWM_SetDutyCycle_CH2}
// };

PWM_RGB_t ledPwmConfig = {
    .Red   = {.setDutyCycle = PWM_SetDutyCycle_CH1},
    .Green = {.setDutyCycle = PWM_SetDutyCycle_CH2},
    .Blue  = {.setDutyCycle = PWM_SetDutyCycle_CH3},
};

// Initialize LED Color Full Brightness
// const DualColor_t Color = {.Color1 = 255, .Color2 = 255};

const RGB_Color_t Color2 = {.R = 0, .G = 255, .B = 255};

const RGB_Color_t Color = {.R = 255, .G = 0, .B = 0};

// Initialize LED Controller with function pointers
LED_Controller_t LEDController = {.Start        = PWM_Start,
                                  .Stop         = PWM_Stop,
                                  .PwmConfig    = &ledPwmConfig,
                                  .LedType      = LED_TYPE_SINGLE_COLOR,
                                  .MaxDutyCycle = MAX_DUTY_CYCLE};

// Initialize Flash Animation
const LED_Animation_Flash_t globalFlashConfig = {
    .color       = &Color,
    .onTimeMs    = 50,
    .offTimeMs   = 200,
    .repeatTimes = 20 // Repeat ten times
};

const LED_Animation_Blink_t globalBlinkConfig = {
    .color       = &Color,
    .periodMs    = 500,
    .repeatTimes = 20 // Repeat ten times
};

const LED_Animation_Solid_t globalSolidConfig = {
    .color           = &Color,
    .executionTimeMs = 5000,
};

const LED_Animation_Breath_t globalBreathConfig = {
    .color = &Color, .riseTimeMs = 500, .fallTimeMs = 1000, .repeatTimes = -1, .invert = false};

const LED_Animation_Breath_t globalBreath2Config = {
    .color = &Color2, .riseTimeMs = 1000, .fallTimeMs = 1000, .repeatTimes = -1, .invert = false};

const LED_Animation_FadeIn_t globalFadeInConfig = {.color = &Color, .durationMs = 1000, .repeatTimes = 1};

const LED_Animation_FadeOut_t globalFadeOutConfig = {.color = &Color, .durationMs = 1000, .repeatTimes = 1};

const LED_Animation_Pulse_t globalPulseConfig = {
    .color         = &Color,
    .riseTimeMs    = 300,
    .holdOnTimeMs  = 200,
    .fallTimeMs    = 300,
    .holdOffTimeMs = 200,
    .repeatTimes   = 1,
};

const RGB_Color_t Red    = {.R = 255, .G = 0, .B = 0};
const RGB_Color_t Green  = {.R = 0, .G = 255, .B = 0};
const RGB_Color_t Blue   = {.R = 0, .G = 0, .B = 255};
const RGB_Color_t Purple = {.R = 255, .G = 0, .B = 255};
const RGB_Color_t Yellow = {.R = 255, .G = 255, .B = 0};
const RGB_Color_t Cyan   = {.R = 0, .G = 255, .B = 255};

static const void* AlternatingColors[] = {&Red, &Green, &Blue, &Purple, &Yellow, &Cyan};

// Initialize the alternating colors animation
const LED_Animation_AlternatingColors_t globalAlternatingColorsConfig = {
    .colors      = (void*)AlternatingColors,
    .colorCount  = sizeof(AlternatingColors) / sizeof(AlternatingColors[0]),
    .durationMs  = 1000, // 500 ms for each color
    .repeatTimes = 1,    // Repeat 3 times
};

const LED_Animation_ColorCycle_t globalColorCycleConfig = {.colors = (void*)AlternatingColors,
                                                           .colorCount =
                                                               sizeof(AlternatingColors) / sizeof(AlternatingColors[0]),
                                                           .transitionMs   = 300,
                                                           .holdTimeMs     = 700,
                                                           .repeatTimes    = 3,
                                                           .leaveLastColor = false};

// Global LED Handle for RGB LED
LED_Handle_t            MyLed;
LED_Transition_Handle_t TransitionsHandle;

#define MAX_TRANSITIONS (4)
const LED_Transition_Config_t transitionMapping[MAX_TRANSITIONS] = {
    {.StartAnim = &globalSolidConfig, .EndAnim = &globalFlashConfig, .TransitionType = LED_TRANSITION_IMMINENT},

    {.StartAnim      = &globalBreathConfig,
     .EndAnim        = &globalBreath2Config,
     .TransitionType = LED_TRANSITION_AT_CLEAN_ENTRY},

    {.StartAnim = &globalBreath2Config, .EndAnim = &globalBlinkConfig, .TransitionType = LED_TRANSITION_AT_CLEAN_ENTRY},
    {.StartAnim = &globalBlinkConfig, .EndAnim = &globalSolidConfig, .TransitionType = LED_TRANSITION_AT_CLEAN_ENTRY},
};

void LED_Complete_Callback(LED_Animation_Type_t animationType, LED_Status_t status)
{
    if (status == LED_SUCCESS)
    {
        UART_printf("Animation Completed, Type: %d\r\n", animationType);
    }
    else
    {
        UART_printf("Animation Failed, Type: %d, Error %d\r\n", animationType, status);
    }
}

void InitDWT(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; // Enable the trace and debug blocks
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;            // Enable the cycle count
    DWT->CYCCNT = 0;                                // Reset the cycle counter
}

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void        SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
#if 1
int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    /* USER CODE BEGIN 2 */
    InitDWT();

    // Initialize the LED Handle with the controller, LED type
    LED_Animation_Init(&MyLed, &LEDController, LED_Complete_Callback);

    // Intialize the LED Transition Handle
    //    LED_Transition_Init(&TransitionsHandle, &MyLed);

    // Set the blink animation for the single LED
    // LED_Animation_SetSolid(&MyLed, &globalSolidConfig);
    // LED_Animation_SetFlash(&MyLed, &globalFlashConfig);
    // LED_Animation_SetBlink(&MyLed, &globalBlinkConfig);
    // LED_Animation_SetFadeIn(&MyLed, &globalFadeInConfig);
    // LED_Animation_SetFadeOut(&MyLed, &globalFadeOutConfig);
    // LED_Animation_SetBreath(&MyLed, &globalBreathConfig);
    // LED_Animation_SetPulse(&MyLed, &globalPulseConfig);
    // LED_Animation_SetAlternatingColors(&MyLed, &globalAlternatingColorsConfig);
    // LED_Animation_SetColorCycle(&MyLed, &globalColorCycleConfig);

    LED_Transition_SetMapping(&TransitionsHandle, transitionMapping, MAX_TRANSITIONS);

    // Start the solid animation
    LED_Transition_SetBreath(&TransitionsHandle, &globalBreathConfig); // red breath

    LED_Transition_SetBreath(&TransitionsHandle, &globalBreath2Config); // green breath

    // Start the blink animation
    LED_Animation_Start(&MyLed);

    /* USER CODE END 2 */
    static uint32_t lastTick = 0;
    bool            send     = false;
    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
        LED_Animation_Update(&MyLed, HAL_GetTick());
        // LED_Transition_Update(&TransitionsHandle, HAL_GetTick());

        // if (HAL_GetTick() - lastTick > 10000)
        // {
        //     lastTick = HAL_GetTick();
        //     if (!send)
        //     {
        //         // LED_Transition_ExecAnimation(&TransitionsHandle, &globalFlashConfig, LED_ANIMATION_FLASH);
        //         LED_Transition_ExecAnimation(&TransitionsHandle, &globalBreath2Config, LED_ANIMATION_BREATH);
        //         send = true;
        //     }
        //     else
        //     {
        //         // LED_Transition_ExecAnimation(&TransitionsHandle, &globalSolidConfig, LED_ANIMATION_SOLID);
        //         LED_Transition_ExecAnimation(&TransitionsHandle, &globalBlinkConfig, LED_ANIMATION_BLINK);
        //         send = false;
        //     }
        // }
    }
    /* USER CODE END 3 */
}
#else
#include "pwm_led.h"

PWMLED_Handle_t pwmLedHandle;

// Command the LED to breath
PWMLED_Breath_Handle_t breathCmd = {.Duty = 255, .PeriodTicks = 1000, .HoldInTicks = 200, .HoldOutTicks = 200};

void updateHardwarePWMDuty(uint16_t dutyValue)
{
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, dutyValue);
}

int main(void)
{

    /* USER CODE BEGIN 1 */

    /* USER CODE END 1 */

    /* MCU
     * Configuration--------------------------------------------------------*/

    /* Reset of all peripherals, Initializes the Flash interface and the
     * Systick. */
    HAL_Init();

    /* USER CODE BEGIN Init */

    /* USER CODE END Init */

    /* Configure the system clock */
    SystemClock_Config();

    /* USER CODE BEGIN SysInit */

    /* USER CODE END SysInit */

    /* Initialize all configured peripherals */
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    /* USER CODE BEGIN 2 */
    InitDWT();

    pwmLedHandle.MaxPulse     = 1000;            // Set according to your PWM resolution
    pwmLedHandle.ActiveConfig = LED_Active_HIGH; // Set this based on your LED configuration

    PWMLED_Init(&pwmLedHandle);

    PWMLED_Command_Breath(&pwmLedHandle, &breathCmd, true);

    PWM_Start();

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1)
    {
        updateHardwarePWMDuty(pwmLedHandle.PWMDuty);

        /* USER CODE END 3 */
    }
}
#endif

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
    if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState            = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState        = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM            = 1;
    RCC_OscInitStruct.PLL.PLLN            = 10;
    RCC_OscInitStruct.PLL.PLLP            = RCC_PLLP_DIV7;
    RCC_OscInitStruct.PLL.PLLQ            = RCC_PLLQ_DIV2;
    RCC_OscInitStruct.PLL.PLLR            = RCC_PLLR_DIV2;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }
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

    TIM_ClockConfigTypeDef  sClockSourceConfig = {0};
    TIM_MasterConfigTypeDef sMasterConfig      = {0};
    TIM_OC_InitTypeDef      sConfigOC          = {0};

    /* USER CODE BEGIN TIM2_Init 1 */

    /* USER CODE END TIM2_Init 1 */
    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 79;
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 999;
    htim2.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
    htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_OC_Init(&htim2) != HAL_OK)
    {
        Error_Handler();
    }
    sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
    sMasterConfig.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode     = TIM_OCMODE_PWM1;
    sConfigOC.Pulse      = 0;
    sConfigOC.OCPolarity = TIM_OCPOLARITY_LOW;
    sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_2) != HAL_OK)
    {
        Error_Handler();
    }
    if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_3) != HAL_OK)
    {
        Error_Handler();
    }
    sConfigOC.OCMode = TIM_OCMODE_TIMING;
    if (HAL_TIM_OC_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_4) != HAL_OK)
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
    huart2.Instance                    = USART2;
    huart2.Init.BaudRate               = 115200;
    huart2.Init.WordLength             = UART_WORDLENGTH_8B;
    huart2.Init.StopBits               = UART_STOPBITS_1;
    huart2.Init.Parity                 = UART_PARITY_NONE;
    huart2.Init.Mode                   = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl              = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling           = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling         = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
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
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pin : B1_Pin */
    GPIO_InitStruct.Pin  = B1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

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
    /* User can add his own implementation to report the HAL error return state
     */
    __disable_irq();
    while (1)
    {
    }
    /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t* file, uint32_t line)
{
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line
       number, ex: printf("Wrong parameters value: file %s on line %d\r\n",
       file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */