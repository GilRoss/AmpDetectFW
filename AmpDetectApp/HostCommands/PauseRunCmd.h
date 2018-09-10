/*
 * PauseRunCmd.h
 *
 *  Created on: Jun 13, 2018
 *      Author: z003xk2p
 */

#ifndef __PauseRunCmd_H_
#define __PauseRunCmd_H_

#include <cstdint>
#include "HostCommand.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PauseRunCmd : public HostCommand
{
public:
    PauseRunCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
        :HostCommand(pMsgBuf, hostCommDrv, pcrTask)
    {
        _request << pMsgBuf;
    }

    virtual void Execute()
    {
        //Try to pause the run.
        Site* pSite = _pcrTask.GetSitePtr();
        ErrCode nErrCode = pSite->PauseRun(_request.GetPausedFlg(), _request.GetCaptureCameraImageFlg());

        //Send response.
        _response.SetResponseHeader(_request, nErrCode);
        _response >> _pMsgBuf;
        _hostCommDrv.TxMessage(_pMsgBuf, _response.GetStreamSize());
    }

protected:

private:
    SetPauseRunReq  _request;
    HostMsg         _response;
};



#endif /* __PauseRunCmd_H_ */
