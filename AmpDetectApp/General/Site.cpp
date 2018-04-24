#include "Site.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Site::Site(uint32_t nSiteIdx)
    :_nSiteIdx(nSiteIdx)
    ,_thermalDrv(nSiteIdx)
    ,_opticsDrv(nSiteIdx)
    ,_bMeerstetterPid(true)
    ,_pid(1400,12,0,-8000, 8000, 0)
    ,_nThermalAcqTimer_ms(0)
    ,_nThermalRecPutIdx(0)
    ,_nThermalRecGetIdx(0)
    ,_nManControlState(kIdle)
    ,_nManControlSetpoint_mC(0)
{
    _arThermalRecs.resize(kMaxThermalRecs);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::Execute()
{
    //Set setpoint according to the active segment and step.
    Segment seg = _pcrProtocol.GetSegment(_siteStatus.GetSegmentIdx());
    Step step = seg.GetStep(_siteStatus.GetStepIdx());
    
    bool bIsStable = false;
    _siteStatus.SetTemperature(_thermalDrv.GetBlockTemp());
    int32_t nControlVar = 0;
    if (_bMeerstetterPid == true)
    {
        _thermalDrv.SetTargetTemp(step.GetTargetTemp());
        bIsStable = _thermalDrv.TempIsStable();
    }
    else //Homegrown PID
    {
        int32_t nBlockTemp = _siteStatus.GetTemperature();
        int32_t nProcessErr = step.GetTargetTemp() - nBlockTemp;
        bIsStable = _pid.Service(kPidTick_ms, step.GetTargetTemp(), nBlockTemp, nProcessErr, &nControlVar);
        _thermalDrv.SetControlVar(nControlVar);
    }

    //Increment elapsed time. Includes ramp.
    _siteStatus.AddStepTime(kPidTick_ms);
    _siteStatus.AddRunTime(kPidTick_ms);
            
    //If target temp has been reached, increment hold time.
    if (bIsStable)
        _siteStatus.AddHoldTime(kPidTick_ms);
                
    //If done with this step.
    if (_siteStatus.GetHoldTime() >= step.GetHoldTime())
    {                            
        //If we want to read the fluorescence on this step.
        if (step.GetReadChanFlg(0) == true)
        {
            OpticsRec opticsRec;
            opticsRec._nTimeTag_ms     = _siteStatus.GetRunTime();
            opticsRec._nCycleIdx       = _siteStatus.GetCycle();
            opticsRec._nDarkRead       = _opticsDrv.GetDarkReading(_opticsDrv.kBlue1);
            opticsRec._nIlluminatedRead= _opticsDrv.GetIlluminatedReading(_opticsDrv.kBlue1);
            opticsRec._nShuttleTemp_mC = 0;
            _arOpticsRecs.push_back( opticsRec );
        }
        
        //If done with all steps in this segment.
        _siteStatus.NextStep();
        if (_siteStatus.GetStepIdx() >= seg.GetNumSteps())
        {                                                
            //If done with all cycles in this segment.
            _siteStatus.NextCycle();
            if (_siteStatus.GetCycle() >= seg.GetNumCycles())
                _siteStatus.NextSegment();
            
            //If done with entire protocol.
            if (_siteStatus.GetSegmentIdx() >= _pcrProtocol.GetNumSegs())
                _siteStatus.SetRunningFlg(false);
        }
        _pid.ResetStableFlg();
    }

    //If time to record the thermals.
    _nThermalAcqTimer_ms += kPidTick_ms;
    if (_nThermalAcqTimer_ms >= kThermalAcqPeriod_ms)
    {
        _arThermalRecs[_nThermalRecPutIdx]._nTimeTag_ms     = _siteStatus.GetRunTime();
        _arThermalRecs[_nThermalRecPutIdx]._nBlockTemp_mC   = _thermalDrv.GetBlockTemp();
        _arThermalRecs[_nThermalRecPutIdx]._nSampleTemp_mC  = _thermalDrv.GetSampleTemp();
        _arThermalRecs[_nThermalRecPutIdx]._nCurrent_mA     = nControlVar;
        _nThermalRecPutIdx++;
        if (_nThermalRecPutIdx >= kMaxThermalRecs)  //Check for wrap.
            _nThermalRecPutIdx = 0;
        _nThermalAcqTimer_ms = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::ManualControl()
{
    //If the user is setting target temperatures.
    if (_nManControlState == kSetpointControl)
    {
        int32_t nControlVar;
        int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
        int32_t nProcessErr = _nManControlSetpoint_mC - nBlockTemp;
        _pid.Service(kPidTick_ms, _nManControlSetpoint_mC, nBlockTemp, nProcessErr, &nControlVar);
        _thermalDrv.SetControlVar(nControlVar);
    }
    else    //Idle
    {
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ErrCode Site::StartRun(bool bMeerstetterPid)
{
    ErrCode     nErrCode = ErrCode::kNoError;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _nManControlState = kIdle;
        _nThermalRecPutIdx = 0;
        _nThermalRecGetIdx = 0;
        _nThermalAcqTimer_ms = 0;
        _arOpticsRecs.clear();
        _siteStatus.ResetForNewRun();
        _bMeerstetterPid = bMeerstetterPid;
        _siteStatus.SetRunningFlg(true);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ErrCode Site::StopRun()
{
    _nManControlState = kIdle;
    _siteStatus.SetRunningFlg(false);
    
    return ErrCode::kNoError;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetManControlSetpoint(int32_t nSp_mC)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _nManControlSetpoint_mC = nSp_mC;
        _nManControlState = kSetpointControl;
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetPidParams(uint32_t nKp, uint32_t nKi, uint32_t nKd)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _pid.SetKp(nKp);
        _pid.SetKi(nKi);
        _pid.SetKd(nKd);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetOpticsLed(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _opticsDrv.SetLedState2(nChanIdx, nIntensity, nDuration);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
const SiteStatus&   Site::GetStatus()
{
    _siteStatus.SetNumThermalRecs(GetNumThermalRecs());
    _siteStatus.SetNumOpticsRecs(GetNumOpticsRecs());
    return _siteStatus;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t    Site::GetNumThermalRecs() const
{
    uint32_t    nGetIdx = _nThermalRecGetIdx;
    uint32_t    nPutIdx = _nThermalRecPutIdx;
    return  nGetIdx <= nPutIdx ? nPutIdx - nGetIdx :
                                 (kMaxThermalRecs - nGetIdx) + nPutIdx;
}

///////////////////////////////////////////////////////////////////////////////
ThermalRec    Site::GetAndDelNextThermalRec()
{
    ThermalRec thermalRec = _arThermalRecs[_nThermalRecGetIdx++];
    if (_nThermalRecGetIdx >= kMaxThermalRecs)
        _nThermalRecGetIdx = 0;
    return thermalRec;
}
    
///////////////////////////////////////////////////////////////////////////////
uint32_t    Site::GetNumOpticsRecs() const
{
    return _arOpticsRecs.size();
}

///////////////////////////////////////////////////////////////////////////////
const OpticsRec* Site::GetOpticsRec(uint32_t nIdx) const
{
    if (nIdx < _arOpticsRecs.size())
        return &_arOpticsRecs[nIdx];
    else
        return NULL;
}
