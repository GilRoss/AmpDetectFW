/*
 * GetOpticsDiodeCmd.h
 *
 *  Created on: May 30, 2018
 *      Author: z003xk2p
 */

#ifndef _GETOPTICSDIODECMD_H_
#define _GETOPTICSDIODECMD_H_

#include <cstdint>
#include "HostCommand.h"
#include "OpticsDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetOpticsDiodeCmd : public HostCommand
{
public:
    GetOpticsDiodeCmd(const uint8_t* pMsgBuf = NULL, HostCommDriver* pCommDrv = NULL, PcrTask* pPcrTask = NULL)
        :HostCommand(&_request, pMsgBuf, pCommDrv, pPcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kNoError;
        uint32_t diodeValue = 0;

        //If site and LED channel index are valid.
        if (_request.GetDiodeIdx() < OpticsDriver::kNumOptChans)
        {
            //Try to set the setpoint.
            Site* pSite = _pPcrTask->GetSitePtr();
            diodeValue = pSite->GetOpticsDiode(_request.GetDiodeIdx());
        }

        //Send response.
        //_response = (HostMsg)_request;
        _response.SetError(nErrCode);
        _response.SetMsgSize(_response.GetStreamSize());
        _response.SetDiodeValue(diodeValue);
        _response >> _arResponseBuf;
        _pHostCommDrv->TxMessage(_arResponseBuf, _response.GetStreamSize());
    }

protected:

private:
    GetOpticsDiodeReq     _request;
    GetOpticsDiodeRes     _response;
};


#endif /* _GETOPTICSDIODECMD_H_ */
