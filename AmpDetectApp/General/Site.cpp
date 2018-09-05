#include "Site.h"
#include "PersistentMem.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Site::Site(uint32_t nSiteIdx)
    :_nSiteIdx(nSiteIdx)
    ,_thermalDrv(nSiteIdx)
    ,_opticsDrv(nSiteIdx)
    ,_bMeerstetterPid(false)
    ,_pid((double)kPidTick_ms / 1000, 100000, -100000, 0.0, 0, 0.0)
    ,_nTemperaturePidSlope(1000)
    ,_nTemperaturePidYIntercept(0)
    ,_nTempStableTolerance_mC(500) // + or -
    ,_nTempStableTime_ms(1000)
    ,_arThermalRecs(kMaxThermalRecs)
    ,_nThermalAcqTimer_ms(0)
    ,_nThermalRecPutIdx(0)
    ,_nThermalRecGetIdx(0)
    ,_nManControlState(kIdle)
    ,_nManControlSetpoint_mC(0)
{
}

static float _nKp = 0.0003;
static float _nKi = 0.000026;
static float _nKd = 0;
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::Execute()
{
    //Make certain we have latest temperature PID params.
    PersistentMem* pPMem = PersistentMem::GetInstance();
    PidParams pidParams = pPMem->GetTemperaturePidParams();
    _pid.SetGains(_nKp, _nKi, _nKd);
    _nTemperaturePidSlope = pidParams.GetSlope();
    _nTemperaturePidYIntercept = pidParams.GetYIntercept();

    //Make certain we have latest current PID params.
//    _thermalDrv.SetPidParams(pPMem->GetCurrentPidParams());

    //Set setpoint according to the active segment and step.
    const Segment& seg = _pcrProtocol.GetSegment(_siteStatus.GetSegmentIdx());
    const Step& step = seg.GetStep(_siteStatus.GetStepIdx());
    static int cameraCaptureCount = 0;
    
    int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
    _siteStatus.SetTemperature(nBlockTemp);
    double nControlVar = 0;
    if (_bMeerstetterPid == true)
    {
        //_thermalDrv.SetTargetTemp(step.GetTargetTemp());
//        _bTempStable = _thermalDrv.TempIsStable();
    }
    else //Homegrown PID
    {
        nControlVar = _pid.calculate(step.GetTargetTemp(), nBlockTemp);
        _thermalDrv.SetControlVar((int32_t)(nControlVar * 1000));
        _thermalDrv.Enable();

        //If we have not yet stabilized on the setpoint?
        if (_siteStatus.GetTempStableFlg() == false)
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
        }
    }

    //Increment elapsed time. Includes ramp.
    _siteStatus.AddStepTimer(kPidTick_ms);
    _siteStatus.AddRunTimer(kPidTick_ms);
            
    //If target temp has been reached, increment hold time.
    if (_siteStatus.GetTempStableFlg())
        _siteStatus.AddHoldTimer(kPidTick_ms);

    //If done with this step.
    if ((_siteStatus.GetHoldTimer() >= step.GetHoldTimer()) && (!_siteStatus.GetPausedFlg()))
    {                            
        //If we want to read the fluorescence on this step
        if ((step.GetOpticalAcqFlg() == true))
        {
            OpticsRec opticsRec;
            OpticalRead optRead;
            //If Detector type is Camera
            if (_pcrProtocol.GetFluorDetectorType() == FluorDetectorType::kCamera)
            {
                if(!_siteStatus.GetCaptureCameraImageFlg())
                {
                    _opticsDrv.SetLedsOff();
                    if (cameraCaptureCount < _pcrProtocol.GetNumOpticalReads())
                    {
                        optRead = _pcrProtocol.GetOpticalRead(cameraCaptureCount);
                        // Turn On LED
                        _opticsDrv.SetLedIntensity(optRead.GetLedIdx(), optRead.GetLedIntensity());
                        _siteStatus.SetPausedFlg(true);
                        _siteStatus.SetCaptureCameraImageFlg(true);
                        _siteStatus.SetCameraIdx(optRead.GetDetectorIdx());
                        _siteStatus.SetOpticsDetectorExposureTime(optRead.GetDetectorIntegrationTime());
                        _siteStatus.SetLedIntensity(optRead.GetLedIntensity());
                        cameraCaptureCount++;
                    }
                    else
                        cameraCaptureCount = 0;

                }
            }
            else if (_pcrProtocol.GetFluorDetectorType() == FluorDetectorType::kPhotoDiode)
            {
               for (int i=0; i<_pcrProtocol.GetNumOpticalReads(); i++)
               {
                   //Save the optical record.
                   optRead = _pcrProtocol.GetOpticalRead(i);
                   opticsRec._nTimeTag_ms           = _siteStatus.GetRunTimer();
                   opticsRec._nCycleNum             = _siteStatus.GetCycleNum();
                   opticsRec._nLedIdx               = optRead.GetLedIdx();
                   opticsRec._nDetectorIdx          = optRead.GetDetectorIdx();
                   opticsRec._nDarkRead             = _opticsDrv.GetDarkReading(optRead);
                   opticsRec._nRefDarkRead          = _opticsDrv.GetPhotoDiodeAdc(optRead.GetReferenceIdx());
                   opticsRec._nIlluminatedRead      = _opticsDrv.GetIlluminatedReading(optRead);
                   opticsRec._nRefIlluminatedRead   = _opticsDrv.GetPhotoDiodeAdc(optRead.GetReferenceIdx());
                   opticsRec._nShuttleTemp_mC       = 0;
                   _arOpticsRecs.push_back( opticsRec );

                   _opticsDrv.SetLedsOff();
                }
            }
        }

        if (!_siteStatus.GetPausedFlg())
        {
            //If done with all steps in this segment.
            _siteStatus.NextStep();
            if (_siteStatus.GetStepIdx() >= seg.GetNumSteps())
            {
                //If done with all cycles in this segment.
                _siteStatus.NextCycle();
                if (_siteStatus.GetCycleNum() >= seg.GetNumCycles())
                    _siteStatus.NextSegment();

                //If done with entire protocol.
                if (_siteStatus.GetSegmentIdx() >= _pcrProtocol.GetNumSegs())
                    _siteStatus.SetRunningFlg(false);
            }
        }
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
        _arThermalRecs[_nThermalRecPutIdx]._nCurrent_mA = nControlVar * 1000;

        //Next record, and check for wrap.
        _nThermalRecPutIdx = (_nThermalRecPutIdx >= (kMaxThermalRecs - 1)) ? 0 : _nThermalRecPutIdx + 1;

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
        int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
        double nControlVar = _pid.calculate(_nManControlSetpoint_mC, nBlockTemp);
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
        _thermalDrv.Reset();
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
///////////////////////////////////////////////////////////////////////////////
ErrCode Site::PauseRun(bool bPause)
{
    ErrCode     nErrCode = ErrCode::kNoError;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == true)    {
        _siteStatus.SetPausedFlg(bPause);    }
    else
        nErrCode = ErrCode::kRunInProgressErr;

    return nErrCode;
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
/*ErrCode Site::SetPidParams(PidType nType, const PidParams& params)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        if (nType == PidType::kTemperature)
        {
            _nTemperaturePidSlope = params.GetSlope();
            _nTemperaturePidYIntercept = params.GetYIntercept();
            _pid.SetGains(params.GetKp(), params.GetKi(), params.GetKd());
        }
        else //if (nType == PidType::kCurrent)
            _thermalDrv.SetPidParams(nType, params);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}*/

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetOpticsLed(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _opticsDrv.SetLedIntensity(nChanIdx, nIntensity);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::GetOpticsDiode(uint32_t nDiodeIdx)
{
    uint32_t diodeValue = 0;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        diodeValue = (uint32_t) _opticsDrv.GetPhotoDiodeAdc(nDiodeIdx);
    }

    return diodeValue;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::GetOpticsLedAdc(uint32_t nLedAdcIdx)
{
    uint32_t ledAdcValue = 0;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        ledAdcValue = (uint32_t) _opticsDrv.GetLedAdc(nLedAdcIdx);
    }

    return ledAdcValue;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::ReadOptics(uint32_t nLedIdx, uint32_t nDiodeIdx, uint32_t nLedIntensity, uint32_t nIntegrationTime_us)
{
    uint32_t diodeValue = 0;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        diodeValue = _opticsDrv.GetPhotoDiodeValue(nLedIdx, nDiodeIdx, nIntegrationTime_us, nLedIntensity);
    }

    return diodeValue;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::GetActiveLedMonitorValue()
{
    uint32_t activeLedMonitorValue = 0;
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        activeLedMonitorValue = _opticsDrv.GetActiveLedMonitorPDValue();
    }

    return activeLedMonitorValue;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::GetActiveLedTemperature()
{
    uint32_t activeLedTemp = 0;
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        activeLedTemp = _opticsDrv.GetActiveLedTemp();
    }

    return activeLedTemp;
}

///////////////////////////////////////////////////////////////////////////////
uint32_t Site::GetActiveDiodeTemperature()
{
    uint32_t activeDiodeTemp = 0;
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        activeDiodeTemp = _opticsDrv.GetActivePhotoDiodeTemp();
    }

    return activeDiodeTemp;
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
