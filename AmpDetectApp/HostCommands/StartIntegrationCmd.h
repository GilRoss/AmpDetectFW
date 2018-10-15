/*
 * StartIntegrationCmd.h
 *
 *  Created on: Oct 4, 2018
 *      Author: z003xk2p
 */

#ifndef __StartIntegrationCmd_H
#define __StartIntegrationCmd_H

#include <cstdint>
#include "HostCommand.h"
#include "OpticsDriver.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class StartIntegrationCmd : public HostCommand
{
public:
    StartIntegrationCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        ErrCode nErrCode = ErrCode::kInvalidCmdParamsErr;

        //Try to set the setpoint.
        Site* pSite = _pcrTask.GetSitePtr();
        nErrCode = pSite->SetIntegration(_request.GetDuration());

        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    StartIntegrationReq _request;
    HostMsg             _response;
};

#endif /* __StartIntegrationCmd_H */
