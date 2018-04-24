#ifndef __GetOpticalRecsCmd_H
#define __GetOpticalRecsCmd_H

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetOpticalRecsCmd : public HostCommand
{
public:
    GetOpticalRecsCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual ~GetOpticalRecsCmd()
    {
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParams;
        _response.ClearAllOpticsRecs();

        if (_request.GetSiteIdx() < _pPcrTask->GetNumSites())
        {
            Site* pSite = _pPcrTask->GetSitePtr(_request.GetSiteIdx());
            if ((_request.GetNumRecsToRead() <= 10) &&
                ((_request.GetFirstRecToReadIdx() + _request.GetNumRecsToRead()) <= pSite->GetNumOpticsRecs()))
            {
                for (int i = 0; i < (int)_request.GetNumRecsToRead(); i++)
                    _response.AddOpticsRec(*(pSite->GetOpticsRec(_request.GetFirstRecToReadIdx() + i)));
                nErrCode = ErrCode::kNoError;
            }
        }

        _response.SetMsgSize(_response.GetStreamSize());
        _response.SetError(nErrCode);
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:
  
private:
    GetOpticsRecsReq   _request;
    GetOpticsRecsRes   _response;
};

#endif // __GetOpticalRecsCmd_H
