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
#include "app_usb.h"
#include "twi.h"


/* Global Variable */
TaskHandle_t Task1Task_Handler;
TaskHandle_t MainTask_Handler;


void task1_task(void *pvParameters)
{
    vTaskDelay(pdMS_TO_TICKS(4000));
    twi_init();
    cdc_printf("checkDevice 0x68 : %d\n",i2c_checkDevice(0x68));
    cdc_printf("checkDevice 0x68 sec : %d\n",i2c_checkDevice(0x68));
    cdc_printf("checkDevice 0x15 : %d\n",i2c_checkDevice(0x15));

    cdc_printf("i2c write!");
    i2c_writeByte(0x68, 0x6B, 0x00); // Wake up MPU6050
    cdc_printf("i2c write Done!");

    uint8_t buffer[14];
    int16_t ax,ay,az,gx,gy,gz;
    uint16_t count = 0;
    uint32_t lasttimer = 0;
    while (1)
    {
        i2c_readBytes(0x68, 0x3B, buffer, 14);
        ax = buffer[0]<<8|buffer[1];
        ay = buffer[2]<<8|buffer[3];
        az = buffer[4]<<8|buffer[5];
        // // Process the data
        gx = buffer[8]<<8|buffer[9];
        gy = buffer[10]<<8|buffer[11];
        gz = buffer[12]<<8|buffer[13];
        count++;
        //check Timer elapsed more than 1s
        if((xTaskGetTickCount()-lasttimer) >= 1000)
        {
            lasttimer = xTaskGetTickCount();
            cdc_printf("1s Elapsed Count: %d\n", count);
            cdc_printf("Accel: %d %d %d\n", ax, ay, az);
            cdc_printf("Gyro: %d %d %d\n", gx, gy, gz);
            count = 0;
        }
        // vTaskDelay for 0.5s
    }
}

void main_task(void *pvParameters)
{
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(3000));
        LOG_INFO("Main_Task_Running!");
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
