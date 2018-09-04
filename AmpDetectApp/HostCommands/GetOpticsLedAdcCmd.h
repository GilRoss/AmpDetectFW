/*
 * GetOpticsLedAdcCmd.h
 *
 *  Created on: Aug 29, 2018
 *      Author: z003xk2p
 */

#ifndef _GETOPTICSLEDADCCMD_H_
#define _GETOPTICSLEDADCCMD_H_

#include <cstdint>
#include "HostCommand.h"
#include "OpticsDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class GetOpticsLedAdcCmd : public HostCommand
{
public:
    GetOpticsLedAdcCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << _pMsgBuf;   //De-serialize request buffer into request object.
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;
        uint32_t ledAdcValue = 0;

        //If site and LED channel index are valid.
        if (_request.GetLedAdcIdx() < OpticsDriver::kNumOpticalAdcChans)
        {
            //Try to set the setpoint.
            Site* pSite = _pcrTask.GetSitePtr();
            ledAdcValue = pSite->GetOpticsLedAdc(_request.GetLedAdcIdx());
            nErrCode = ErrCode::kNoError;
        }

        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response.SetLedAdcValue(ledAdcValue);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    GetOpticsLedAdcReq    _request;
    GetOpticsLedAdcRes    _response;
};


#endif /* _GETOPTICSLEDADCCMD_H_ */
