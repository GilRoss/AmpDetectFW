#include "HostCommDriver.h"
#include "FreeRTOS.h"
#include "os_task.h"
#include "sci.h"

static bool bRequestComplete = false;


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HostCommDriver::HostCommDriver()
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool HostCommDriver::TxMessage(uint8_t* pMsgBuf, uint32_t nMsgSize)
{
    sciSend(scilinREG, nMsgSize, pMsgBuf);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t HostCommDriver::RxMessage(uint8_t* pDst, uint32_t nDstSize)
{
    //First, get the header of the incoming message.
    HostMsg msgHdr;
    sciReceive(scilinREG, nDstSize, pDst);
    while (sciGetRxCount() < msgHdr.GetStreamSize())
        vTaskDelay (1 / portTICK_PERIOD_MS);
    msgHdr << pDst;
    
    //If there is more than just the header, get the remainder of the message.
    if (msgHdr.GetStreamSize() < msgHdr.GetMsgSize())
    {
        while (sciGetRxCount() < msgHdr.GetMsgSize())
            vTaskDelay (1 / portTICK_PERIOD_MS);
    }
    
    return msgHdr.GetMsgSize();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern "C" void HostCommRxISR();
void HostCommRxISR()
{
    bRequestComplete = true;
}

