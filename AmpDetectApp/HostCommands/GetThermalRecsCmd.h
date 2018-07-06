#ifndef __GetThermalRecsCmd_H
#define __GetThermalRecsCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetThermalRecsCmd : public HostCommand
{
public:
    GetThermalRecsCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual ~GetThermalRecsCmd()
    {
    }

    virtual void Execute()
    {
        _response.ClearAllThermalRecs();
        Site* pSite = _pcrTask.GetSitePtr(0);
        uint32_t nNumRecs = pSite->GetNumThermalRecs() > 10 ? 10 : pSite->GetNumThermalRecs();
        for (int i = 0; i < (int)nNumRecs; i++)
            _response.AddThermalRec(pSite->GetAndDelNextThermalRec());

        _response.SetResponseHeader(_request, ErrCode::kNoError);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg             _request;
    GetThermalRecsRes   _response;
};

#endif // __GetThermalRecsCmd_H
