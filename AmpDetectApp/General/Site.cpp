#include "Site.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Site::Site(uint32_t nSiteIdx)
    :_nSiteIdx(nSiteIdx)
    ,_thermalDrv(nSiteIdx)
    ,_opticsDrv(nSiteIdx)
    ,_bMeerstetterPid(false)
    ,_pid(0.050, 3000, -3000, 0.00005, 0.000005, 0)
    ,_nTempStableTolerance_mC(1000) // + or -
    ,_nTempStableTime_ms(1000)
    ,_nThermalAcqTimer_ms(0)
    ,_nThermalRecPutIdx(0)
    ,_nThermalRecGetIdx(0)
    ,_nManControlState(kIdle)
    ,_nManControlSetpoint_mC(0)
{
    _arThermalRecs.resize(kMaxThermalRecs);

    Segment seg; //debug
    Step step; //debug
    step.SetTargetTemp(80000, 10000); //debug
    seg.PushStep(step); //debug
//    step.SetTargetTemp(90000, 10000); //debug
//    seg.PushStep(step); //debug
    seg.SetNumCycles(5); //debug
    _pcrProtocol.AddSegment(seg); //debug
    _siteStatus.SetRunningFlg(true);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::Execute()
{
    //Set setpoint according to the active segment and step.
    Segment seg = _pcrProtocol.GetSegment(_siteStatus.GetSegmentIdx());
    Step step = seg.GetStep(_siteStatus.GetStepIdx());
    
    int32_t nBlockTemp = _siteStatus.GetTemperature();
    _siteStatus.SetTemperature(nBlockTemp);
    int32_t nControlVar = 0;
    if (_bMeerstetterPid == true)
    {
        //_thermalDrv.SetTargetTemp(step.GetTargetTemp());
//        _bTempStable = _thermalDrv.TempIsStable();
    }
    else //Homegrown PID
    {
        double nControlVar = _pid.calculate(step.GetTargetTemp(), nBlockTemp);
//        _thermalDrv.SetControlVar((int32_t)(nControlVar * 1000));
        _thermalDrv.SetControlVar(1000);
        _thermalDrv.Enable();

        //If we have not yet stabilized on the setpoint?
/*        if (_siteStatus.GetTempStableFlg() == false)
        {
            //Is temperature within tolerance?
            if ((nBlockTemp >= (step.GetTargetTemp() - _nTempStableTolerance_mC)) &&
                (nBlockTemp <= (step.GetTargetTemp() + _nTempStableTolerance_mC)))
            {
                _siteStatus.SetStableTimer(_siteStatus.GetStableTimer() + kPidTick_ms);
                if (_siteStatus.GetStableTimer() >= _nTempStableTime_ms)
                    _siteStatus.SetTempStableFlg(true);
            }
            else
                _siteStatus.SetStableTimer(0);
        }*/
    }

    //Increment elapsed time. Includes ramp.
    _siteStatus.AddStepTimer(kPidTick_ms);
    _siteStatus.AddRunTimer(kPidTick_ms);
            
    //If target temp has been reached, increment hold time.
    if (_siteStatus.GetTempStableFlg())
        _siteStatus.AddHoldTimer(kPidTick_ms);

    //If done with this step.
    if (_siteStatus.GetHoldTimer() >= step.GetHoldTimer())
    {                            
        //If we want to read the fluorescence on this step.
        if (step.GetReadChanFlg(0) == true)
        {
            OpticsRec opticsRec;
            opticsRec._nTimeTag_ms     = _siteStatus.GetRunTimer();
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
//        _pid.ResetStableFlg();
    }

    //If time to record the thermals.
    _nThermalAcqTimer_ms += kPidTick_ms;
    if (_nThermalAcqTimer_ms >= kThermalAcqPeriod_ms)
    {
        _arThermalRecs[_nThermalRecPutIdx]._nTimeTag_ms = _siteStatus.GetRunTimer();
        _arThermalRecs[_nThermalRecPutIdx]._nChan1_mC   = _siteStatus.GetTemperature();
        _arThermalRecs[_nThermalRecPutIdx]._nChan2_mC   = _thermalDrv.GetSampleTemp();
        _arThermalRecs[_nThermalRecPutIdx]._nChan3_mC   = 0;
        _arThermalRecs[_nThermalRecPutIdx]._nChan4_mC   = 0;
        _arThermalRecs[_nThermalRecPutIdx]._nCurrent_mA = nControlVar;
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
        double nControlVar;
        int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
        nControlVar = _pid.calculate( _nManControlSetpoint_mC / 1000, nBlockTemp / 1000);
        _thermalDrv.SetControlVar((int32_t)(nControlVar * 1000));
        _thermalDrv.Enable();
    }
    else    //Idle
    {
        _thermalDrv.Disable();
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
