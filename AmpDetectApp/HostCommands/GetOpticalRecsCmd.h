#ifndef __GetOpticalRecsCmd_H
#define __GetOpticalRecsCmd_H

#include <cstdint>
#include "HostCommand.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetOpticalRecsCmd: public HostCommand
{
public:
    GetOpticalRecsCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv,
                      PcrTask& pcrTask) :
            HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf; //De-serialize request buffer into request object.
    }

    virtual ~GetOpticalRecsCmd()
    {
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kNoError;
        Site*   pSite = _pcrTask.GetSitePtr();
        _response.ClearAllOpticsRecs();

/*        Site* pSite = _pcrTask.GetSitePtr(0);
        if ((_request.GetNumRecsToRead() <= 10)
                && ((_request.GetFirstRecToReadIdx()
                        + _request.GetNumRecsToRead())
                        <= pSite->GetNumOpticsRecs()))
        {
            for (int i = 0; i < (int) _request.GetNumRecsToRead(); i++)
                _response.AddOpticsRec(
                        *(pSite->GetOpticsRec(
                                _request.GetFirstRecToReadIdx() + i)));
            nErrCode = ErrCode::kNoError;
        }
*/
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    GetOpticsRecsReq _request;
    GetOpticsRecsRes _response;
};

#endif // __GetOpticalRecsCmd_H
