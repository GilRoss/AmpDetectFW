#ifndef __Site_H
#define __Site_H

#include <cstdint>
#include <vector>
#include "FreeRTOS.h"
#include "os_semphr.h"
#include "PcrProtocol.h"
#include "Pid.h"
#include "SysStatus.h"
#include "ThermalDriver.h"
#include "OpticsDriver.h"
#include "HostMessages.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Site
{
public:
    enum    {kPidTick_ms = 25};
    enum    {kThermalAcqPeriod_ms = 100};
    enum    {kMaxThermalRecs = 100};
    
    enum    ManCtrlState : uint32_t
            {kIdle = 0, kTemperatureControl, kOpticsLedControl, kCurrentControl};
    
    Site(uint32_t nSiteIdx = 0);
    
    void                Execute();
    void                ExecutePcr();
    void                ExecuteManualControl();
    bool                GetRunningFlg() const {return _siteStatus.GetRunningFlg();}
    uint32_t            GetNumThermalRecs() const;
    void                GetAndDelThermalRecs(uint32_t nMaxRecs, std::vector<ThermalRec>*);
    uint32_t            GetNumOpticsRecs() const;
    const OpticsRec*    GetOpticsRec(uint32_t nIdx) const;
    void                SetPcrProtocol(const PcrProtocol& p)   {_pcrProtocol = p;}
    void                GetSiteStatus(SiteStatus*);
    OpticsDriver&       GetOpticsDrvPtr()   {return _opticsDrv;}
    
    ErrCode             StartRun(bool bMeerstetterPid);
    ErrCode             StopRun();
    ErrCode             PauseRun(bool bPause, bool bCaptureCameraImageFlg);
    ErrCode             SetOpticsLed(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration);
    uint32_t            GetOpticsDiode(uint32_t nDiodeIdx);
    uint32_t            GetOpticsLedAdc(uint32_t nLedAdcIdx);
    uint32_t            GetActiveLedMonitorValue();
    uint32_t            GetActiveLedTemperature();
    uint32_t            GetActiveDiodeTemperature();
    uint32_t            ReadOptics(uint32_t nLedIdx, uint32_t nDiodeIdx, uint32_t nLedIntensity, uint32_t nIntegrationTime_us);
    ErrCode             SetIntegration(uint32_t nDuration_us);

    ErrCode             DisableManualControl();
    ErrCode             SetManControlTemperature(int32_t nSp_mC);
    ErrCode             SetManControlCurrent(int32_t nSp_mA);

protected:
  
private:
    uint32_t                    _nSiteIdx;
    SemaphoreHandle_t           _sysStatusSemId;
    ThermalDriver               _thermalDrv;
    OpticsDriver                _opticsDrv;
    PcrProtocol                 _pcrProtocol;
    bool                        _bMeerstetterPid;
    Pid                         _pid;
    int32_t                     _nTemperaturePidSlope;
    int32_t                     _nTemperaturePidYIntercept;
    SiteStatus                  _siteStatus;
    int32_t                     _nTempStableTolerance_mC;
    uint32_t                    _nTempStableTime_ms;
    std::vector<OpticsRec>      _arOpticsRecs;
    std::vector<ThermalRec>     _arThermalRecs;
    uint32_t                    _nThermalAcqTimer_ms;
    uint32_t                    _nThermalRecPutIdx;
    uint32_t                    _nThermalRecGetIdx;
    
    ManCtrlState                _nManControlState;
    uint32_t                    _nDuration_us;
    int32_t                     _nManControlTemperature_mC;
    int32_t                     _nManControlCurrent_mA;
    int32_t                     _nStartTemperature_mC;
    double                      _nFineTargetTemp_mC;
    bool                        _nTargetTempReachedFlag;
};

#endif // __Site_H
