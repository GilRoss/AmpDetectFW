#ifndef __HostCmdFactory_H
#define __HostCmdFactory_H

#include        <cstdint>
#include        "HostCommDriver.h"
#include        "PcrTask.h"
#include        "GetSysStatusCmd.h"
#include        "StartRunCmd.h"
#include        "StopRunCmd.h"
#include        "ReadOpticalChanCmd.h"
#include        "LoadPcrProtocolCmd.h"
#include        "GetThermalRecsCmd.h"
#include        "GetOpticalRecsCmd.h"
#include        "SetManControlSetpointCmd.h"
#include        "SetPidParamsCmd.h"
#include        "SetOpticsLedCmd.h"
#include        "GetOpticsDiodeCmd.h"
#include        "ReadOpticsCmd.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCmdFactory
{
public:
    //use GetHostCmd method to get object of type HostCmd. 
    static HostCommand* GetHostCmd(uint8_t* pMsgBuf, HostCommDriver* pCommDrv, PcrTask* pPcrTask)
    {
        uint32_t        nMsgId = *((uint32_t*)pMsgBuf);
        HostCommand*    pHostCmd = NULL;

        if (nMsgId == HostMsg::MakeObjId('G', 'S', 't', 't'))
            pHostCmd = new GetSysStatusCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('L', 'd', 'P', 'r'))
            pHostCmd = new LoadPcrProtocolCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'R', 'u', 'n'))
            pHostCmd = new StartRunCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 't', 'o', 'p'))
            pHostCmd = new StopRunCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('R', 'C', 'h', 'n'))
            pHostCmd = new ReadOpticalChanCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'T', 'h', 'm'))
            pHostCmd = new GetThermalRecsCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'O', 'p', 't'))
            pHostCmd = new GetOpticalRecsCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('M', 'c', 'S', 'p'))
            pHostCmd = new SetManControlSetpointCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'P', 'i', 'd'))
            pHostCmd = new SetPidParamsCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'O', 'L', 'd'))
            pHostCmd = new SetOpticsLedCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'O', 'D', 'i'))
            pHostCmd = new GetOpticsDiodeCmd(pMsgBuf, pCommDrv, pPcrTask);
        else if (nMsgId == HostMsg::MakeObjId('R', 'O', 'p', 't'))
            pHostCmd = new ReadOpticsCmd(pMsgBuf, pCommDrv, pPcrTask);

        return pHostCmd;
    }
        
protected:
  
private:
};

#endif // __HostMsgFactory_H
