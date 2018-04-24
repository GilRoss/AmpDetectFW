#ifndef __GetThermalRecsCmd_H
#define __GetThermalRecsCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetThermalRecsCmd : public HostCommand
{
public:
    GetThermalRecsCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual ~GetThermalRecsCmd()
    {
    }

    virtual void Execute()
    {
        _response.ClearAllThermalRecs();
        Site* pSite = _pPcrTask->GetSitePtr(0);
        uint32_t nNumRecs = pSite->GetNumThermalRecs() > 10 ? 10 : pSite->GetNumThermalRecs();
        for (int i = 0; i < (int)nNumRecs; i++)
            _response.AddThermalRec(pSite->GetAndDelNextThermalRec());

        _response.SetMsgSize(_response.GetStreamSize());
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg             _request;
    GetThermalRecsRes   _response;
};

#endif // __GetThermalRecsCmd_H
