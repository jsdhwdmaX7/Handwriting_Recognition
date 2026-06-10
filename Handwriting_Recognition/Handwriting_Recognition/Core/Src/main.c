/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (semantic-equivalent rewritten version)
  ******************************************************************************
  */
/* USER CODE END Header */

#include "main.h"
#include <string.h>
#include <stdio.h>
#include "ai_platform.h"
#include "network.h"
#include "network_data.h"

CRC_HandleTypeDef hcrc;
UART_HandleTypeDef huart1;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_CRC_Init(void);
static void MX_USART1_UART_Init(void);

/* AI resources */
static ai_handle g_networkHandle;
static float g_inputData[AI_NETWORK_IN_1_SIZE];
static float g_outputData[AI_NETWORK_OUT_1_SIZE];
static ai_u8 g_activationMem[AI_NETWORK_DATA_ACTIVATIONS_SIZE];

static ai_buffer *g_inputBuffer;
static ai_buffer *g_outputBuffer;

/* UART resources */
#define UART_BUFF_LEN 1024
#define ONE_FRAME_LEN (1 + 784 + 2)

static uint16_t g_rxLength = 0;
static uint8_t g_rxByte = 0;
static uint8_t g_rxBuffer[UART_BUFF_LEN];
volatile uint8_t g_frameReady = 0;

/* function prototypes */
static void NeuralNet_Init(void);
static void NeuralNet_Run(float *input, float *output);
static void ConvertImageToFloat(uint8_t *src, float *dst, int len);
static void UART_SendString(char *text);

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_CRC_Init();
    MX_USART1_UART_Init();

    NeuralNet_Init();

    memset(g_rxBuffer, 0, 784);

    HAL_UART_Receive_IT(
        &huart1,
        &g_rxByte,
        1);

    UART_SendString("AI is running!\r\n");

    for (;;)
    {
        if (!g_frameReady)
        {
            continue;
        }

        if (g_rxLength == ONE_FRAME_LEN)
        {
            ConvertImageToFloat(
                &g_rxBuffer[1],
                g_inputData,
                784);

            NeuralNet_Run(
                g_inputData,
                g_outputData);
        }

        memset(g_rxBuffer, 0, 784);

        g_rxLength = 0;
        g_frameReady = 0;
    }
}

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 8;
    RCC_OscInitStruct.PLL.PLLN = 216;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 2;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    if (HAL_PWREx_EnableOverDrive() != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType =
        RCC_CLOCKTYPE_HCLK |
        RCC_CLOCKTYPE_SYSCLK |
        RCC_CLOCKTYPE_PCLK1 |
        RCC_CLOCKTYPE_PCLK2;

    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

    if (HAL_RCC_ClockConfig(
            &RCC_ClkInitStruct,
            FLASH_LATENCY_7) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_CRC_Init(void)
{
    hcrc.Instance = CRC;
    hcrc.Init.DefaultPolynomialUse = DEFAULT_POLYNOMIAL_ENABLE;
    hcrc.Init.DefaultInitValueUse = DEFAULT_INIT_VALUE_ENABLE;
    hcrc.Init.InputDataInversionMode = CRC_INPUTDATA_INVERSION_NONE;
    hcrc.Init.OutputDataInversionMode = CRC_OUTPUTDATA_INVERSION_DISABLE;
    hcrc.InputDataFormat = CRC_INPUTDATA_FORMAT_BYTES;

    if (HAL_CRC_Init(&hcrc) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_USART1_UART_Init(void)
{
    huart1.Instance = USART1;
    huart1.Init.BaudRate = 115200;
    huart1.Init.WordLength = UART_WORDLENGTH_8B;
    huart1.Init.StopBits = UART_STOPBITS_1;
    huart1.Init.Parity = UART_PARITY_NONE;
    huart1.Init.Mode = UART_MODE_TX_RX;
    huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart1.Init.OverSampling = UART_OVERSAMPLING_16;
    huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&huart1) != HAL_OK)
    {
        Error_Handler();
    }
}

static void MX_GPIO_Init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    (void)huart;

    if (!g_frameReady)
    {
        if (g_rxLength >= UART_BUFF_LEN)
        {
            g_rxLength = 0;
        }
        else
        {
            g_rxBuffer[g_rxLength++] = g_rxByte;

            if (g_rxByte == '\n')
            {
                g_frameReady = 1;
            }
        }
    }

    HAL_UART_Receive_IT(
        &huart1,
        &g_rxByte,
        1);
}

int __io_putchar(int c)
{
    uint8_t data = (uint8_t)c;

    HAL_UART_Transmit(
        &huart1,
        &data,
        1,
        HAL_MAX_DELAY);

    return c;
}

static void UART_SendString(char *text)
{
    HAL_UART_Transmit(
        &huart1,
        (uint8_t *)text,
        (uint16_t)strlen(text),
        HAL_MAX_DELAY);
}

static void NeuralNet_Init(void)
{
    ai_error err;

    const ai_handle activationAddr[] =
    {
        g_activationMem
    };

    err = ai_network_create_and_init(
        &g_networkHandle,
        activationAddr,
        NULL);

    if (err.type != AI_ERROR_NONE)
    {
        printf(
            "ai_network_create error - type=%d code=%d\r\n",
            err.type,
            err.code);

        Error_Handler();
    }

    g_inputBuffer =
        ai_network_inputs_get(
            g_networkHandle,
            NULL);

    g_outputBuffer =
        ai_network_outputs_get(
            g_networkHandle,
            NULL);
}

static void NeuralNet_Run(float *input, float *output)
{
    ai_i32 batch;
    ai_error err;

    char logMsg[100];
    float bestScore = 0.0f;
    uint32_t bestIndex = 0;

    g_inputBuffer[0].data = AI_HANDLE_PTR(input);
    g_outputBuffer[0].data = AI_HANDLE_PTR(output);

    batch = ai_network_run(
        g_networkHandle,
        g_inputBuffer,
        g_outputBuffer);

    if (batch != 1)
    {
        err = ai_network_get_error(g_networkHandle);

        printf(
            "AI ai_network_run error - type=%d code=%d\r\n",
            err.type,
            err.code);

        Error_Handler();
    }

    for (uint32_t i = 0; i < AI_NETWORK_OUT_1_SIZE; ++i)
    {
        sprintf(
            logMsg,
            "%lu  %8.6f\r\n",
            (unsigned long)i,
            g_outputData[i]);

        UART_SendString(logMsg);

        if (g_outputData[i] > bestScore)
        {
            bestScore = g_outputData[i];
            bestIndex = i;
        }
    }

    sprintf(
        logMsg,
        "current number is %lu\r\n",
        (unsigned long)bestIndex);

    UART_SendString(logMsg);
}

static void ConvertImageToFloat(
    uint8_t *src,
    float *dst,
    int len)
{
    int idx = 0;

    while (idx < len)
    {
        *(dst + idx) = (float)(*(src + idx));
        ++idx;
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif