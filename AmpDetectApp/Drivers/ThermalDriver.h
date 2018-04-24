#ifndef __ThermalDriver_H
#define __ThermalDriver_H

#include    <stdlib.h>
#include    <cstdint>
#include    <string.h>

//extern UART_HandleTypeDef  _h485Uart;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class ThermalDriver
{
public:

    unsigned char ucHex[16+1];
//            {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};

    ThermalDriver(uint32_t nSiteIdx = 0);
    
    void        SetTargetTemp(uint32_t targetTemp_mC);
    void        SetControlVar(int32_t nControlVar);
    bool        TempIsStable();
    int32_t     GetSampleTemp();
    int32_t     GetSinkTemp();
    int32_t     GetBlockTemp();
    uint32_t    My_atoi(uint8_t ch);
    uint32_t    MessageTransaction(uint8_t* pSrc, size_t nSrcSize, uint8_t* pDst, uint32_t nDstSize);
    void        MX_USART1_UART_Init(void);
    void        InsertFloat(uint8_t* pDst, float nValue);
    void        InsertCRC(uint8_t* pMsg, uint16_t nNumChars);
    void        CRC16Algorithm(uint16_t* pCRC, unsigned char Ch);
    
protected:
  
private:
    uint8_t _rxBuffer[35];
    uint32_t _nVal;
};

#endif // __ThermalDriver_H
