#ifndef __HostCmdFactory_H
#define __HostCmdFactory_H

#include        <cstdint>
#include        "HostCommDriver.h"
#include        "PcrTask.h"
#include        "GetSysStatusCmd.h"
#include        "StartRunCmd.h"
#include        "StopRunCmd.h"
#include        "PauseRunCmd.h"
#include        "ReadOpticalChanCmd.h"
#include        "LoadPcrProtocolCmd.h"
#include        "GetThermalRecsCmd.h"
#include        "GetOpticalRecsCmd.h"
#include        "SetPidParamsCmd.h"
#include        "GetPidParamsCmd.h"
#include        "SetOpticsLedCmd.h"
#include        "GetOpticsDiodeCmd.h"
#include        "GetOpticsLedAdcCmd.h"
#include        "ReadOpticsCmd.h"
#include        "DisableManualControlCmd.h"
#include        "SetManControlTemperatureCmd.h"
#include        "SetManControlCurrentCmd.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCmdFactory
{
public:
    //use GetHostCmd method to get object of type HostCmd. 
    static HostCommand* GetHostCmd(uint8_t* pMsgBuf, HostCommDriver& hostCommDrv, PcrTask& pcrTask)
    {
        uint32_t        nMsgId = *((uint32_t*)pMsgBuf);
        HostCommand*    pHostCmd = NULL;

        if (nMsgId == HostMsg::MakeObjId('G', 'S', 't', 't'))
            pHostCmd = new GetSysStatusCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('L', 'd', 'P', 'r'))
            pHostCmd = new LoadPcrProtocolCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'R', 'u', 'n'))
            pHostCmd = new StartRunCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 't', 'o', 'p'))
            pHostCmd = new StopRunCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('P', 'a', 'u', 's'))
            pHostCmd = new PauseRunCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('R', 'C', 'h', 'n'))
            pHostCmd = new ReadOpticalChanCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'T', 'h', 'm'))
            pHostCmd = new GetThermalRecsCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'O', 'p', 't'))
            pHostCmd = new GetOpticalRecsCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'P', 'i', 'd'))
            pHostCmd = new SetPidParamsCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'P', 'i', 'd'))
            pHostCmd = new GetPidParamsCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'O', 'L', 'd'))
            pHostCmd = new SetOpticsLedCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'O', 'D', 'i'))
            pHostCmd = new GetOpticsDiodeCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('G', 'O', 'L', 'e'))
            pHostCmd = new GetOpticsLedAdcCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('R', 'O', 'p', 't'))
            pHostCmd = new ReadOpticsCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('D', 'M', 'a', 'n'))
            pHostCmd = new DisableManualControlCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'T', 'm', 'p'))
            pHostCmd = new SetManControlTemperatureCmd(pMsgBuf, hostCommDrv, pcrTask);
        else if (nMsgId == HostMsg::MakeObjId('S', 'C', 'u', 'r'))
            pHostCmd = new SetManControlCurrentCmd(pMsgBuf, hostCommDrv, pcrTask);

        return pHostCmd;
    }
        
protected:
  
private:
};

#endif // __HostMsgFactory_H
