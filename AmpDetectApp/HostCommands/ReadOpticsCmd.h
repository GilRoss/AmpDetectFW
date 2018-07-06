/*
 * ReadOpticsCmd.h
 *
 *  Created on: May 30, 2018
 *      Author: z003xk2p
 */

#ifndef READOPTICSCMD_H_
#define READOPTICSCMD_H_

#include <cstdint>
#include "HostCommand.h"
#include "OpticsDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class ReadOpticsCmd : public HostCommand
{
public:
    ReadOpticsCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParams;
        uint32_t diodeValue = 0;

        //If site and LED channel index are valid.
        if ((_request.GetLedIdx() < OpticsDriver::kNumOptChans) && (_request.GetDiodeIdx() < OpticsDriver::kNumOptChans))
        {
            //Try to set the setpoint.
            Site* pSite = _pcrTask.GetSitePtr();
            diodeValue = pSite->ReadOptics(_request.GetLedIdx(),
                                           _request.GetDiodeIdx(),
                                           _request.GetLedIntensity(),
                                           _request.GetIntegrationTime());
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
    ReadOpticsReq     _request;
    ReadOpticsRes     _response;
};

#endif /* READOPTICSCMD_H_ */
