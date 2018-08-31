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
    GetOpticsDiodeCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;
        uint32_t diodeValue = 0;

        //If site and LED channel index are valid.
        if (_request.GetDiodeIdx() < OpticsDriver::kNumOpticalAdcChans)
        {
            //Try to set the setpoint.
            Site* pSite = _pcrTask.GetSitePtr();
            diodeValue = pSite->GetOpticsDiode(_request.GetDiodeIdx());
            nErrCode = ErrCode::kNoError;
        }

        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response.SetDiodeValue(diodeValue);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    GetOpticsDiodeReq     _request;
    GetOpticsDiodeRes     _response;
};


#endif /* _GETOPTICSDIODECMD_H_ */
