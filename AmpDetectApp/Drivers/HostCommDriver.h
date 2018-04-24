#ifndef __HostCommDriver_H
#define __HostCommDriver_H

#include <string.h>
#include <cstdint>
#include "HostMessages.h"

//extern UART_HandleTypeDef  _huart2;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCommDriver
{
public:
    HostCommDriver();
    
    bool        TxMessage(uint8_t* pMsgBuf, uint32_t nMsgSize);
    uint32_t    RxMessage(uint8_t* pDst, uint32_t nDstSize);
    
protected:
  
private:
};

#endif // __HostCommDriver_H
