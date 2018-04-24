#include "HostCommDriver.h"
#include "FreeRTOS.h"
#include "os_task.h"

//UART_HandleTypeDef      _huart2;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HostCommDriver::HostCommDriver()
{
/*    _huart2.Instance = USART2;
    _huart2.Init.BaudRate = 38400;
    _huart2.Init.WordLength = UART_WORDLENGTH_8B;
    _huart2.Init.StopBits = UART_STOPBITS_1;
    _huart2.Init.Parity = UART_PARITY_NONE;
    _huart2.Init.Mode = UART_MODE_TX_RX;
    _huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_DeInit(&_huart2);
    HAL_UART_Init(&_huart2);*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool HostCommDriver::TxMessage(uint8_t* pMsgBuf, uint32_t nMsgSize)
{
//    HAL_UART_Transmit(&_huart2, pMsgBuf, nMsgSize, 3000);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t HostCommDriver::RxMessage(uint8_t* pDst, uint32_t nDstSize)
{
/*    HAL_UART_Receive_IT(&_huart2, pDst, nDstSize);
    
    //First, get the header of the incoming message.
    HostMsg msgHdr;
    while ((nDstSize - _huart2.RxXferCount) < msgHdr.GetStreamSize())
        osDelay(5);  
    msgHdr << pDst;
    
    //Get the remainder of the message.
    while ((nDstSize - _huart2.RxXferCount) < msgHdr.GetMsgSize())
        osDelay(5);  
    
    HAL_UART_Abort(&_huart2);
    return nDstSize - _huart2.RxXferCount;*/
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    return 0;
}
