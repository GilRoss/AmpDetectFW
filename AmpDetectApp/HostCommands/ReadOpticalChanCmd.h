#ifndef __ReadOpticalChanCmd_H
#define __ReadOpticalChanCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class ReadOpticalChanCmd : public HostCommand
{
public:
    enum    {kMaxAdcReadings = 200};
    
    ReadOpticalChanCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {        
//        OpticsDriver opticsDrv;
//        if ((_nNumVals < kMaxAdcReadings) && (_nChanIdx < opticsDrv.kNumOptChans))
        {  
//            opticsDrv.SetLedsOff();
            for (int i = 0; i < (int)_nNumVals; i++)
            {
//                _arAdcValsLedOff[i] = (uint16_t)opticsDrv.GetAdc(_nChanIdx);
            }
            
//            opticsDrv.SetLedState(_nChanIdx, true);
            for (int i = 0; i < (int)_nNumVals; i++)
            {
//                _arAdcValsLedOn[i] = (uint16_t)opticsDrv.GetAdc(_nChanIdx);
            }
//            opticsDrv.SetLedsOff();
            
            char pBuf[20];
            for (int i = 0; i < (int)_nNumVals; i++)
            {
//                sprintf(pBuf, "%d\r", _arAdcValsLedOff[i]);
                _hostCommDrv.TxMessage((uint8_t*)pBuf, strlen(pBuf));
            }
            
            for (int i = 0; i < (int)_nNumVals; i++)
            {
//                sprintf(pBuf, "%d\r", _arAdcValsLedOn[i]);
                _hostCommDrv.TxMessage((uint8_t*)pBuf, strlen(pBuf));
            }
        }
    }

protected:
  
private:
    HostMsg     _request;
    HostMsg     _response;
    uint32_t    _nChanIdx;
    uint32_t    _nNumVals;
    uint16_t    _arAdcValsLedOn[kMaxAdcReadings];
    uint16_t    _arAdcValsLedOff[kMaxAdcReadings];
};

#endif // __ReadOpticalChanCmd_H
