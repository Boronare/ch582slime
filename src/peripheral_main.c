/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2020/08/06
 * Description        : ����ӻ�Ӧ��������������ϵͳ��ʼ��
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/******************************************************************************/
/* ͷ�ļ����� */
#include <CH58x_common.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
// #include "stdarg.h"
#include "config.h"
#include "HAL.h"
#include "gattprofile.h"
#include "peripheral.h"
#include "app_usb.h"

/*********************************************************************
 * GLOBAL TYPEDEFS
 */
__attribute__((aligned(4))) uint32_t MEM_BUF[BLE_MEMHEAP_SIZE / 4];

#if(defined(BLE_MAC)) && (BLE_MAC == TRUE)
const uint8_t MacAddr[6] = {0x84, 0xC2, 0xE4, 0x03, 0x02, 0x02};
#endif
/* Global Variable */
TaskHandle_t Task1Task_Handler;
TaskHandle_t MainTask_Handler;
// SemaphoreHandle_t xBinarySem;


// /*********************************************************************
//  * @fn      App_Printf
//  *
//  * @brief   printf can be used within freertos.
//  *
//  * @param  *fmt - printf params.
//  *
//  * @return  none
//  */
// __HIGH_CODE
// void App_Printf(const char *fmt, ...)
// {
//     char  buf_str[128]; /* 需要注意在这里的内存空间是否足够打印 */
//     va_list   v_args;

//     va_start(v_args, fmt);
//    (void)vsnprintf((char       *)&buf_str[0],
//                    (size_t      ) sizeof(buf_str),
//                    (char const *) fmt,
//                                   v_args);
//     va_end(v_args);

//     /* 互斥量操作，不可在中断中使用 */
//     xSemaphoreTake(printMutex, portMAX_DELAY);
//     printf("%s", buf_str);
//     xSemaphoreGive(printMutex);
// }

/*********************************************************************
 * @fn      task1_task
 *
 * @brief   task1 program.
 *
 * @param  *pvParameters - Parameters point of task1
 *
 * @return  none
 */
__HIGH_CODE
void task1_task(void *pvParameters)
{
    while (1)
    {
        //vTaskDelay for 0.5s
        vTaskDelay(pdMS_TO_TICKS(500));
        GPIOB_InverseBits(GPIO_Pin_4);
    }
}

void main_task(void *pvParameters)
{
    while(1)
    {
        TMOS_SystemProcess();
    }
}

/*********************************************************************
 * @fn      main
 *
 * @brief   ������
 *
 * @return  none
 */
int main(void)
{
#if(defined(DCDC_ENABLE)) && (DCDC_ENABLE == TRUE)
    PWR_DCDCCfg(ENABLE);
#endif
    SetSysClock(CLK_SOURCE_PLL_60MHz);
#if(defined(HAL_SLEEP)) && (HAL_SLEEP == TRUE)
    GPIOA_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
    GPIOB_ModeCfg(GPIO_Pin_All, GPIO_ModeIN_PU);
#endif
    app_usb_init();
    GPIOB_ModeCfg(GPIO_Pin_4, GPIO_ModeOut_PP_5mA);
    GPIOB_SetBits(GPIO_Pin_4);

    CH58X_BLEInit();
    HAL_Init();
    GAPRole_PeripheralInit();
    Peripheral_Init();
    
    GPIOB_ResetBits(GPIO_Pin_4);
    
    xTaskCreate((TaskFunction_t)task1_task,
                (const char *)"task1",
                (uint16_t)256,
                (void *)NULL,
                (UBaseType_t)5,
                (TaskHandle_t *)&Task1Task_Handler);
    xTaskCreate((TaskFunction_t)main_task,
                (const char *)"main",
                (uint16_t)256,
                (void *)NULL,
                (UBaseType_t)5,
                (TaskHandle_t *)&MainTask_Handler);
    vTaskStartScheduler();
    return 0;
}


/******************************** endfile @ main ******************************/
