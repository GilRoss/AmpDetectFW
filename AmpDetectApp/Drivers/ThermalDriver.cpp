#include "ThermalDriver.h"

//UART_HandleTypeDef      _h485Uart;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ThermalDriver::ThermalDriver(uint32_t nSiteIdx)
{
    strcpy((char*)ucHex, "0123456789ABCDEF");
    /* Set the RS485 parameters */
    MX_USART1_UART_Init();            
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetTargetTemp(uint32_t targetTemp_mC)
{
    float   nTargetTemp = targetTemp_mC / 1000;
    uint8_t txBuffer[] = "#0115B0VS0BB801vvvvvvvvxxxx\r";
    InsertFloat(&txBuffer[15], nTargetTemp);
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetControlVar(int32_t nControlVar)
{
    uint8_t txBuffer[] = "#0115B0VS07E401vvvvvvvvxxxx\r";
    InsertFloat(&txBuffer[15], ((float)nControlVar) / 1000);
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ThermalDriver::TempIsStable()
{
    //Query temperature stable state.
    uint8_t txBuffer[] = "#0115AB?VR04B001xxxx\r";
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
    
    return (_rxBuffer[14] == '2');
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t ThermalDriver::GetSampleTemp()
{
    uint8_t txBuffer[] = "#0215AB?VR03E801xxxx\r";
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
    
    //!0115AB41CD2F2890A1  (Example response).
    _rxBuffer[15] = 0;
    _nVal = 0;
    _nVal += My_atoi(_rxBuffer[7]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[8]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[9]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[10]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[11]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[12]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[13]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[14]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t ThermalDriver::GetSinkTemp()
{
    uint8_t txBuffer[] = "#0115AB?VR03FC01xxxx\r";
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
    
    //!0115AB41CD2F2890A1  (Example response).
    _rxBuffer[15] = 0;
    _nVal = 0;
    _nVal += My_atoi(_rxBuffer[7]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[8]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[9]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[10]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[11]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[12]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[13]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[14]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t ThermalDriver::GetBlockTemp()
{
    uint8_t txBuffer[] = "#0115AB?VR03E801xxxx\r";
    InsertCRC(txBuffer, sizeof(txBuffer) - 1);
    MessageTransaction(txBuffer, sizeof(txBuffer) - 1, _rxBuffer, sizeof(_rxBuffer));
    
    //!0215AB41CD2F2890A1  (Example response).
    _rxBuffer[15] = 0;
    _nVal = 0;
    _nVal += My_atoi(_rxBuffer[7]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[8]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[9]);  _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[10]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[11]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[12]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[13]); _nVal =  _nVal << 4;
    _nVal += My_atoi(_rxBuffer[14]);
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::My_atoi(uint8_t ch)
{
    if (ch >= 0x61)
        return (ch - 0x61) + 0x0A;
    if (ch >= 0x41)
        return (ch - 0x41) + 0x0A;
    if (ch >= 0x30)
        return ch - 0x30;
    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::MessageTransaction(uint8_t* pSrc, size_t nSrcSize, uint8_t* pDst, uint32_t nDstSize)
{
    uint32_t    nRxCount = 0;
/*    bool        bSuccess = false;
    uint32_t    nRetries = 0;
    uint32_t    nTimeout_ms = 0;
    memcpy(pDst, 0, nDstSize);
    
    while ((bSuccess == false) && (nRetries < 3))
    {
        osDelay(5);

        //Transmit the request.
        HAL_StatusTypeDef nErrCode = HAL_UART_Transmit(&_h485Uart, pSrc, nSrcSize, 500);
        if (nErrCode == HAL_OK)
        {
            //Wait for the response.
            nErrCode = HAL_UART_Receive_IT(&_h485Uart, pDst, nDstSize);
            if (nErrCode == HAL_OK)
            {
                //Wait for start of response.
                nTimeout_ms = 0;
                while ((nDstSize == _h485Uart.RxXferCount) && (nTimeout_ms < 500))
                {
                    osDelay(5);
                    nTimeout_ms += 5;
                }
                
                //Wait for entire response.
                while ((pDst[nDstSize - _h485Uart.RxXferCount - 1] != '\r') && (nTimeout_ms < 1000))
                {
                    osDelay(5);
                    nTimeout_ms += 5;
                }
                
                //If the response was received successfully.
                if (pDst[nDstSize - _h485Uart.RxXferCount - 1] == '\r')
                {
                    bSuccess = true;
                    nRxCount = nDstSize - _h485Uart.RxXferCount;
                }
            }
        }
                
//        HAL_UART_Abort(&_h485Uart);
        nRetries++;
    }
*/
    return nRxCount;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::MX_USART1_UART_Init(void)
{
/*    uint32_t nAdvFeatureInitAll = 0xFF;
    
    _h485Uart.Instance                          = USART1;
    _h485Uart.gState                            = HAL_UART_STATE_RESET;
    _h485Uart.Init.BaudRate                     = 38400;
    _h485Uart.Init.WordLength                   = UART_WORDLENGTH_8B;
    _h485Uart.Init.StopBits                     = UART_STOPBITS_1;
    _h485Uart.Init.Parity                       = UART_PARITY_NONE;
    _h485Uart.Init.Mode                         = UART_MODE_TX_RX;
    _h485Uart.Init.HwFlowCtl                    = UART_HWCONTROL_NONE;
    _h485Uart.Init.OverSampling                 = UART_OVERSAMPLING_16;
    _h485Uart.Init.OneBitSampling               = UART_ONE_BIT_SAMPLE_DISABLE;
    _h485Uart.AdvancedInit.AdvFeatureInit       = nAdvFeatureInitAll;
    _h485Uart.AdvancedInit.TxPinLevelInvert     = UART_ADVFEATURE_TXINV_DISABLE;
    _h485Uart.AdvancedInit.RxPinLevelInvert     = UART_ADVFEATURE_RXINV_DISABLE;
    _h485Uart.AdvancedInit.DataInvert           = UART_ADVFEATURE_DATAINV_DISABLE;
    _h485Uart.AdvancedInit.Swap                 = UART_ADVFEATURE_SWAP_DISABLE;
    _h485Uart.AdvancedInit.OverrunDisable       = UART_ADVFEATURE_OVERRUN_DISABLE;
    _h485Uart.AdvancedInit.DMADisableonRxError  = UART_ADVFEATURE_DMA_DISABLEONRXERROR;
    _h485Uart.AdvancedInit.AutoBaudRateEnable   = UART_ADVFEATURE_AUTOBAUDRATE_DISABLE;
    _h485Uart.AdvancedInit.AutoBaudRateMode     = UART_ADVFEATURE_AUTOBAUDRATE_ONSTARTBIT;
    _h485Uart.AdvancedInit.MSBFirst             = UART_ADVFEATURE_MSBFIRST_DISABLE;

    if(HAL_RS485Ex_Init(&_h485Uart, UART_DE_POLARITY_HIGH, 0, 0) != HAL_OK)
        _h485Uart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::InsertFloat(uint8_t* pDst, float nValue)
{
    uint8_t*    pValue = (uint8_t*)&nValue;
    pDst[7] = ucHex[pValue[0] & 0x0F];
    pDst[6] = ucHex[(pValue[0] >> 4) & 0x0F];
    pDst[5] = ucHex[pValue[1] & 0x0F];
    pDst[4] = ucHex[(pValue[1] >> 4) & 0x0F];
    pDst[3] = ucHex[pValue[2] & 0x0F];
    pDst[2] = ucHex[(pValue[2] >> 4) & 0x0F];
    pDst[1] = ucHex[pValue[3] & 0x0F];
    pDst[0] = ucHex[(pValue[3] >> 4) & 0x0F];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::InsertCRC(uint8_t* pMsg, uint16_t nNumChars)
{
    uint16_t nCrc = 0;
    for (int i = 0; i < nNumChars - 5; i++)
        CRC16Algorithm(&nCrc, pMsg[i]);
    
    pMsg[nNumChars - 2] = ucHex[nCrc & 0x000F];
    pMsg[nNumChars - 3] = ucHex[(nCrc >> 4) & 0x000F];
    pMsg[nNumChars - 4] = ucHex[(nCrc >> 8) & 0x000F];
    pMsg[nNumChars - 5] = ucHex[(nCrc >> 12) & 0x000F];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::CRC16Algorithm(uint16_t* pCRC, unsigned char Ch)
{
    uint16_t genPoly = 0x1021; //CCITT CRC-16 Polynominal
    uint16_t uiCharShifted = ((unsigned int)Ch & 0x00FF) << 8;
    *pCRC = *pCRC ^ uiCharShifted;
    for (int i = 0; i < 8; i++)
    if ( *pCRC & 0x8000 )
        *pCRC = (*pCRC << 1) ^ genPoly;
    else
        *pCRC = *pCRC << 1;
    *pCRC &= 0xFFFF;
}
