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
        Site* pSite = _pcrTask.GetSitePtr();

        //Determine max number of records that can fit into this response.
        uint32_t nMaxRecs = HostMsg::kMaxResponseSize - _response.GetStreamSize();
        nMaxRecs = nMaxRecs / sizeof(ThermalRec);

        //Copy the records
        pSite->GetAndDelThermalRecs(nMaxRecs, &_arThermalRecs);
        for (int i = 0; i < (int)_arThermalRecs.size(); i++)
            _response.AddThermalRec(_arThermalRecs[i]);

        _response.SetResponseHeader(_request, ErrCode::kNoError);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:
  
private:
    HostMsg                 _request;
    GetThermalRecsRes       _response;
    std::vector<ThermalRec> _arThermalRecs;
};

#endif // __GetThermalRecsCmd_H
