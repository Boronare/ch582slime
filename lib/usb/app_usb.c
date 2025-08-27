// CDC printf implementation
#include <stdarg.h>
#include <stdio.h>

uint8_t cdc_printf(const char *fmt, ...)
{
  char buf[128];
  va_list ap;
  va_start(ap, fmt);
  int len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  if (len > 0) {
    if (len > sizeof(buf)) len = sizeof(buf);
    USBSendData((uint8_t*)buf, len);
  }
  return 0;
}
/********************************** (C) COPYRIGHT *******************************
 * File Name          : app_usb.c
 * Author             : WCH
 * Version            : V1.1
 * Date               : 2022/01/19
 * Description        :
 * Copyright (c) 2021 Nanjing Qinheng Microelectronics Co., Ltd.
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*********************************************************************
 * INCLUDES
 */

#include "stdint.h"
#include "app_usb.h"
#include "CH58x_common.h"

/*********************************************************************
 * MACROS
 */

/*********************************************************************
 * CONSTANTS
 */
uint8_t DevConfig, Ready;
uint8_t SetupReqCode;
UINT16 SetupReqLen;
const uint8_t *pDescr;

#define DevEP0SIZE  0x40
// 设备描述符 (Device Descriptor)
// USB 장치의 기본 정보(버전, 클래스, VID/PID 등)를 정의합니다.
const uint8_t MyDevDescr[] = {  0x12, //bLength
                                0x01, //bDescriptorType
                                0x10,0x01, //bcdUSB
                                0xFF, //bDeviceClass
                                0x00, //bDeviceSubClass
                                0x00, //bDeviceProtocol
                                DevEP0SIZE, //bMaxPacketSize
                                0x86,0x1A, //idVendor
                                0xE0,0x55, //idProduct
                                0x63,0x02,
                                0x00,
                                0x02,
                                0x00,
                                0x01 };
// 配置描述符 (Configuration Descriptor)
// USB 장치의 전체 구조(인터페이스, 엔드포인트 등)를 정의합니다.
const uint8_t MyCfgDescr[] = {   0x09,0x02,0x27,0x00,0x01,0x01,0x00,0x80,0xf0,              //配置描述符，接口描述符,端点描述符
                                 0x09,0x04,0x00,0x00,0x03,0xff,0x01,0x02,0x00,
                                 0x07,0x05,0x82,0x02,0x20,0x00,0x00,                        //批量上传端点
                                 0x07,0x05,0x02,0x02,0x20,0x00,0x00,                        //批量下传端点
                                 0x07,0x05,0x81,0x03,0x08,0x00,0x01};                       //中断上传端点
// 语言描述符 (Language Descriptor)
// 지원하는 언어(예: 영어, 중국어 등)를 정의합니다.
// 0x0409 = English (United States)
const uint8_t MyLangDescr[] = { 0x04, 0x03, 0x09, 0x04 };
// 厂家信息 (Manufacturer String)
// 제조사 정보를 USB 문자열로 제공합니다.
const uint8_t MyManuInfo[] = { 0x0E, 0x03, 'w', 0, 'c', 0, 'h', 0, '.', 0, 'c', 0, 'n', 0 };
// 产品信息 (Product String)
// 제품 정보를 USB 문자열로 제공합니다.
const uint8_t MyProdInfo[] = { 0x0C, 0x03, 'C', 0, 'H', 0, '5', 0, '7', 0, 'x', 0 };
/*제품 설명자 (Product String Descriptor)*/
// 제품 이름을 유니코드로 표현한 문자열입니다.
const uint8_t StrDesc[28] =
{
  0x1C,0x03,0x55,0x00,0x53,0x00,0x42,0x00,
  0x32,0x00,0x2E,0x00,0x30,0x00,0x2D,0x00,
  0x53,0x00,0x65,0x00,0x72,0x00,0x69,0x00,
  0x61,0x00,0x6C,0x00
};

const uint8_t Return1[2] = {0x31,0x00};
const uint8_t Return2[2] = {0xC3,0x00};
const uint8_t Return3[2] = {0x9F,0xEE};

/*********************************************************************
 * LOCAL VARIABLES
 */

// -------- 사용자 정의 엔드포인트 버퍼 영역 --------
// 각 엔드포인트별로 USB 데이터 버퍼를 할당합니다.
__attribute__((aligned(4)))  uint8_t EP0_Databuf[64 + 64 + 64];    //ep0(64)+ep4_out(64)+ep4_in(64)
__attribute__((aligned(4)))  uint8_t EP1_Databuf[64 + 64];    //ep1_out(64)+ep1_in(64)
__attribute__((aligned(4)))  uint8_t EP2_Databuf[64 + 64];    //ep2_out(64)+ep2_in(64)
__attribute__((aligned(4)))  uint8_t EP3_Databuf[64 + 64];    //ep3_out(64)+ep3_in(64)

/*********************************************************************
 * PUBLIC FUNCTIONS
 */

/*********************************************************************
 * @fn      app_usb_init
 *
 * @brief   USB 초기화 함수
 *          USB 엔드포인트 버퍼를 할당하고, USB 장치를 초기화합니다.
 *
 * @return  없음
 */
void app_usb_init()
{
    pEP0_RAM_Addr = EP0_Databuf;
    pEP1_RAM_Addr = EP1_Databuf;
    pEP2_RAM_Addr = EP2_Databuf;
    pEP3_RAM_Addr = EP3_Databuf;

    USB_DeviceInit();
    PFIC_EnableIRQ( USB_IRQn );
}

/*********************************************************************
 * @fn      USBSendData
 *
 * @brief   USB CDC IN 엔드포인트로 데이터 전송
 *          USB를 통해 PC(호스트)로 데이터를 보냅니다.
 *
 * @return  없음
 */
void USBSendData( uint8_t *SendBuf, uint8_t l)
{
   memcpy(pEP2_IN_DataBuf,SendBuf,l);
   DevEP2_IN_Deal( l );
}

/*********************************************************************
 * @fn      DevEP1_OUT_Deal
 *
 * @brief   엔드포인트 1(OUT) 데이터 처리
 *          필요시 사용자 정의 처리 코드를 작성합니다.
 *
 * @return  없음
 */
void DevEP1_OUT_Deal( uint8_t l )
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      DevEP2_OUT_Deal
 *
 * @brief   엔드포인트 2(OUT) 데이터 처리
 *          USB CDC로 수신된 데이터를 BLE 등으로 전달하거나, 에코 처리할 수 있습니다.
 *
 * @return  없음
 */
void DevEP2_OUT_Deal( uint8_t l )
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      DevEP3_OUT_Deal
 *
 * @brief   엔드포인트 3(OUT) 데이터 처리
 *          필요시 사용자 정의 처리 코드를 작성합니다.
 *
 * @return  없음
 */
void DevEP3_OUT_Deal( uint8_t l )
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      DevEP4_OUT_Deal
 *
 * @brief   엔드포인트 4(OUT) 데이터 처리
 *          필요시 사용자 정의 처리 코드를 작성합니다.
 *
 * @return  없음
 */
void DevEP4_OUT_Deal( uint8_t l )
{ /* 用户可自定义 */
}

/*********************************************************************
 * @fn      USB_DevTransProcess
 *
 * @brief   USB 데이터 전송 및 제어 처리 함수
 *          USB 인터럽트에서 호출되며, 각 엔드포인트의 데이터 처리 및 표준/클래스 요청을 처리합니다.
 *
 * @return  없음
 */
void USB_DevTransProcess( void )
{
  uint8_t len, chtype;
  uint8_t intflag, errflag = 0;

  intflag = R8_USB_INT_FG;
  if ( intflag & RB_UIF_TRANSFER )
  {
  if ( ( R8_USB_INT_ST & MASK_UIS_TOKEN ) != MASK_UIS_TOKEN )    // USB가 실제 데이터 전송 중일 때(비유휴 상태)
    {
      switch ( R8_USB_INT_ST & ( MASK_UIS_TOKEN | MASK_UIS_ENDP ) )
  // USB 토큰 및 엔드포인트 번호 분석
      {
        case UIS_TOKEN_IN :
        {
          switch ( SetupReqCode )
          {
            case USB_GET_DESCRIPTOR :
              len = SetupReqLen >= DevEP0SIZE ?
                  DevEP0SIZE : SetupReqLen;    // 本次传输长度
              memcpy( pEP0_DataBuf, pDescr, len ); /* 加载上传数据 */
              SetupReqLen -= len;
              pDescr += len;
              R8_UEP0_T_LEN = len;
              R8_UEP0_CTRL ^= RB_UEP_T_TOG;                             // 翻转
              break;
            case USB_SET_ADDRESS :
              R8_USB_DEV_AD = ( R8_USB_DEV_AD & RB_UDA_GP_BIT ) | SetupReqLen;
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
            default :
              R8_UEP0_T_LEN = 0;                                      // 状态阶段完成中断或者是强制上传0长度数据包结束控制传输
              R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
              break;
          }
        }
          break;

        case UIS_TOKEN_OUT :
        {
          len = R8_USB_RX_LEN;
        }
          break;

        case UIS_TOKEN_OUT | 1 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 동기화되지 않은 데이터 패킷은 무시됨
            len = R8_USB_RX_LEN;
            DevEP1_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 1 :
          R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 2 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 不同步的数据包将丢弃
            len = R8_USB_RX_LEN;
            DevEP2_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 2 :
          R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 3 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {                       // 不同步的数据包将丢弃
            len = R8_USB_RX_LEN;
            DevEP3_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 3 :
          R8_UEP3_CTRL = ( R8_UEP3_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        case UIS_TOKEN_OUT | 4 :
        {
          if ( R8_USB_INT_ST & RB_UIS_TOG_OK )
          {
            R8_UEP4_CTRL ^= RB_UEP_R_TOG;
            len = R8_USB_RX_LEN;
            DevEP4_OUT_Deal( len );
          }
        }
          break;

        case UIS_TOKEN_IN | 4 :
          R8_UEP4_CTRL ^= RB_UEP_T_TOG;
          R8_UEP4_CTRL = ( R8_UEP4_CTRL & ~MASK_UEP_T_RES ) | UEP_T_RES_NAK;
          break;

        default :
          break;
      }
      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
  if ( R8_USB_INT_ST & RB_UIS_SETUP_ACT )                  // Setup 패킷 처리(제어 전송)
    {
      R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_NAK;
      SetupReqLen = pSetupReqPak->wLength;
      SetupReqCode = pSetupReqPak->bRequest;
      chtype = pSetupReqPak->bRequestType;

      len = 0;
      errflag = 0;
      if ( ( pSetupReqPak->bRequestType & USB_REQ_TYP_MASK ) != USB_REQ_TYP_STANDARD )
      {
        if( pSetupReqPak->bRequestType == 0xC0 )
        {
          if(SetupReqCode==0x5F)
          {
            pDescr = Return1;
            len = sizeof(Return1);
          }
          else if(SetupReqCode==0x95)
          {
            if((pSetupReqPak->wValue)==0x18)
            {
              pDescr = Return2;
              len = sizeof(Return2);
            }
            else if((pSetupReqPak->wValue)==0x06)
            {
              pDescr = Return3;
              len = sizeof(Return3);
            }
          }
          else
          {
            errflag = 0xFF;
          }
          memcpy(pEP0_DataBuf,pDescr,len);
        }
        else
        {
          len = 0;
        }
      }
  else /* 표준 USB 요청 처리 */
      {
        switch ( SetupReqCode )
        {
          case USB_GET_DESCRIPTOR :
          {
            switch ( ( ( pSetupReqPak->wValue ) >> 8 ) )
            {
              case USB_DESCR_TYP_DEVICE :
              {
                pDescr = MyDevDescr;
                len = sizeof(MyDevDescr);
              }
                break;

              case USB_DESCR_TYP_CONFIG :
              {
                pDescr = MyCfgDescr;
                len = sizeof(MyCfgDescr);
              }
                break;

              case USB_DESCR_TYP_REPORT :
//              {
//                if ( ( ( pSetupReqPak->wIndex ) & 0xff ) == 0 )                             //接口0报表描述符
//                {
//                  pDescr = KeyRepDesc;                                  //数据准备上传
//                  len = sizeof( KeyRepDesc );
//                }
//                else if ( ( ( pSetupReqPak->wIndex ) & 0xff ) == 1 )                        //接口1报表描述符
//                {
//                  pDescr = MouseRepDesc;                                //数据准备上传
//                  len = sizeof( MouseRepDesc );
//                  Ready = 1;                                            //如果有更多接口，该标准位应该在最后一个接口配置完成后有效
//                }
//                else
//                  len = 0xff;                                           //本程序只有2个接口，这句话正常不可能执行
//              }
                break;

              case USB_DESCR_TYP_STRING :
              {
                switch ( ( pSetupReqPak->wValue ) & 0xff )
                {
                  case 1 :
                    pDescr = MyManuInfo;
                    len = MyManuInfo[0];
                    break;
                  case 2 :
                    pDescr = StrDesc;
                    len = StrDesc[0];
                    break;
                  case 0 :
                    pDescr = MyLangDescr;
                    len = MyLangDescr[0];
                    break;
                  default :
                    errflag = 0xFF;                               // 不支持的字符串描述符
                    break;
                }
              }
                break;

              default :
                errflag = 0xff;
                break;
            }
            if ( SetupReqLen > len )
              SetupReqLen = len;      // 실제로 전송할 총 길이
            len = ( SetupReqLen >= DevEP0SIZE ) ?
                DevEP0SIZE : SetupReqLen;
            memcpy( pEP0_DataBuf, pDescr, len );
            pDescr += len;
          }
            break;

          case USB_SET_ADDRESS :
            SetupReqLen = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_GET_CONFIGURATION :
            pEP0_DataBuf[0] = DevConfig;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_SET_CONFIGURATION :
            DevConfig = ( pSetupReqPak->wValue ) & 0xff;
            break;

          case USB_CLEAR_FEATURE :
          {
            if ( ( pSetupReqPak->bRequestType & USB_REQ_RECIP_MASK ) == USB_REQ_RECIP_ENDP )    // 端点
            {
              switch ( ( pSetupReqPak->wIndex ) & 0xff )
              {
                case 0x82 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x02 :
                  R8_UEP2_CTRL = ( R8_UEP2_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                case 0x81 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_T_TOG | MASK_UEP_T_RES ) ) | UEP_T_RES_NAK;
                  break;
                case 0x01 :
                  R8_UEP1_CTRL = ( R8_UEP1_CTRL & ~( RB_UEP_R_TOG | MASK_UEP_R_RES ) ) | UEP_R_RES_ACK;
                  break;
                default :
                  errflag = 0xFF;                                 // 不支持的端点
                  break;
              }
            }
            else
              errflag = 0xFF;
          }
            break;

          case USB_GET_INTERFACE :
            pEP0_DataBuf[0] = 0x00;
            if ( SetupReqLen > 1 )
              SetupReqLen = 1;
            break;

          case USB_GET_STATUS :
            pEP0_DataBuf[0] = 0x00;
            pEP0_DataBuf[1] = 0x00;
            if ( SetupReqLen > 2 )
              SetupReqLen = 2;
            break;

          default :
            errflag = 0xff;
            break;
        }
      }
      if ( errflag == 0xff )        // 错误或不支持
      {
//                  SetupReqCode = 0xFF;
        R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_STALL | UEP_T_RES_STALL;    // STALL
      }
      else
      {
  if ( chtype & 0x80 )     // 업로드(호스트로 데이터 전송)
        {
          len = ( SetupReqLen > DevEP0SIZE ) ?
              DevEP0SIZE : SetupReqLen;
          SetupReqLen -= len;
        }
        else
          len = 0;        // 다운로드(호스트에서 데이터 수신)
  R8_UEP0_T_LEN = len;
  R8_UEP0_CTRL = RB_UEP_R_TOG | RB_UEP_T_TOG | UEP_R_RES_ACK | UEP_T_RES_ACK;    // 기본 데이터 패킷은 DATA1
      }

      R8_USB_INT_FG = RB_UIF_TRANSFER;
    }
  }
  else if ( intflag & RB_UIF_BUS_RST ) // USB 버스 리셋 처리
  {
    R8_USB_DEV_AD = 0;
    R8_UEP0_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK;
    R8_UEP1_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP2_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
    R8_UEP3_CTRL = UEP_R_RES_ACK | UEP_T_RES_NAK | RB_UEP_AUTO_TOG;
  R8_USB_INT_FG = RB_UIF_BUS_RST; // 리셋 후 엔드포인트 초기화
  }
  else if ( intflag & RB_UIF_SUSPEND ) // USB 서스펜드/웨이크업 처리
  {
    if ( R8_USB_MIS_ST & RB_UMS_SUSPEND )
    {
      ;
    }    // 挂起
    else
    {
      ;
    }               // 唤醒
  R8_USB_INT_FG = RB_UIF_SUSPEND; // 서스펜드/웨이크업 상태 갱신
  }
  else
  {
    R8_USB_INT_FG = intflag;
  }
}

/*********************************************************************
 * @fn      USB_IRQHandler
 *
 * @brief   USB 인터럽트 핸들러
 *          USB 인터럽트 발생 시 데이터 및 제어 처리를 담당합니다.
 *
 * @return  없음
 */
__attribute__((interrupt("WCH-Interrupt-fast")))
__attribute__((section(".highcode")))
void USB_IRQHandler( void ) /* USB中断服务程序,使用寄存器组1 */
{
  USB_DevTransProcess();
}


/*********************************************************************
*********************************************************************/
