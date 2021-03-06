#include "Site.h"
#include "PersistentMem.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Site::Site(uint32_t nSiteIdx)
    :_nSiteIdx(nSiteIdx)
    ,_thermalDrv(nSiteIdx)
    ,_opticsDrv(nSiteIdx)
    ,_bMeerstetterPid(false)
    ,_pid(1, 550 * 20, -550 * 20, 0.0, 0.0, 0.0)
    ,_nTemperaturePidSlope(1)
    ,_nTemperaturePidYIntercept(0)
    ,_nTempStableTolerance_mC(500) // + or -
    ,_nTempStableTime_ms(0)
    ,_arThermalRecs(kMaxThermalRecs)
    ,_nThermalAcqTimer_ms(0)
    ,_nThermalRecPutIdx(0)
    ,_nThermalRecGetIdx(0)
    ,_nManControlState(kIdle)
    ,_nManControlTemperature_mC(0)
    ,_nManControlCurrent_mA(0)
    ,_nStartTemperature_mC(0)
    ,_nFineTargetTemp_mC(0)
    ,_nTargetTempReachedFlag(false)
{
    _sysStatusSemId = xSemaphoreCreateMutex();
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::Execute()
{
    //Make sure while updating the site, we do not try to read the site status.
    xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);

    //If the site is active.
    if (GetRunningFlg() == true)
        ExecutePcr();
    else
        ExecuteManualControl();

    xSemaphoreGive(_sysStatusSemId);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::ExecutePcr()
{
    //Make certain we have latest temperature PID params.
    PersistentMem* pPMem = PersistentMem::GetInstance();
    PidParams pidParams = pPMem->GetTemperaturePidParams();
    _pid.SetGains(pidParams.GetKp(), pidParams.GetKi(), pidParams.GetKd());
    _nTemperaturePidSlope = pidParams.GetSlope();
    _nTemperaturePidYIntercept = pidParams.GetYIntercept();
    _nTempStableTolerance_mC = pidParams.GetStabilizationTolerance() *1000;
    _nTempStableTime_ms = pidParams.GetStabilizationTime() * 1000;

    //Set setpoint according to the active segment and step.
    const Segment& seg = _pcrProtocol.GetSegment(_siteStatus.GetSegmentIdx());
    const Step& step = seg.GetStep(_siteStatus.GetStepIdx());
    static int cameraCaptureCount = 0;
    bool pidControlFlag = true;
    
    int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
    /* Check if Step hasn't started and set Start Temperature to block temperature */
    _siteStatus.SetTemperature(nBlockTemp);
    double nControlVar = 0;
    if (_bMeerstetterPid == true)
    {
        //_thermalDrv.SetTargetTemp(step.GetTargetTemp());
//        _bTempStable = _thermalDrv.TempIsStable();
    }
    else //Homegrown PID
    {
#if 1
        if (_siteStatus.GetStepTimer() == 0)
        {
            _nStartTemperature_mC = nBlockTemp;
            _nFineTargetTemp_mC = (double)_nStartTemperature_mC;
            _nTargetTempReachedFlag = false;
        }

        if(step.GetRampRate() > 0)
        {

            if ((step.GetTargetTemp() - _nStartTemperature_mC) >= 0)
            {
                if (_nFineTargetTemp_mC < step.GetTargetTemp())
                    _nFineTargetTemp_mC += (step.GetRampRate()/40);
                else
                {
                    _nFineTargetTemp_mC = step.GetTargetTemp();
                    _nTargetTempReachedFlag = true;
                }
            }
            else
            {
                if (_nFineTargetTemp_mC > step.GetTargetTemp())
                    _nFineTargetTemp_mC -= (step.GetRampRate()/40);
                else
                {
                    _nFineTargetTemp_mC = step.GetTargetTemp();
                    _nTargetTempReachedFlag = true;
                }
            }
        }
        else
        {
            /* Set maximimum current until block temperature is 5 degC within target temperate*/
            if ((step.GetTargetTemp() - _nStartTemperature_mC) >= 0)
            {
                if ((step.GetTargetTemp() - nBlockTemp) >= 5000)
                {
                    pidControlFlag = false;
                    _thermalDrv.SetPidState(_thermalDrv.kPidMaxPosPower);
                }
                else
                {
                    pidControlFlag = true;
                    //printf("Positive Temp within 5 deg C\n");
                }

            }
            else
            {
                if ((nBlockTemp - step.GetTargetTemp()) >= 5000)
                {
                    pidControlFlag = false;
                    _thermalDrv.SetPidState(_thermalDrv.kPidMaxNegPower);
                }
                else
                {
                    pidControlFlag = true;
                    //printf("Negative Temp within 5 deg C\n");
                }
            }
            _nFineTargetTemp_mC = step.GetTargetTemp();
        }
#endif
#if 1
        /* Calculate pid only when pidControlFlag is true */
        if (pidControlFlag == true)
        {
            nControlVar = _pid.calculate(_nFineTargetTemp_mC, nBlockTemp);
            //_thermalDrv.SetPidState(_thermalDrv.kPidOn);
            _thermalDrv.SetControlVar((int32_t)nControlVar);
        }
        else
        {
            if ((step.GetTargetTemp() - _nStartTemperature_mC) > 0)
            {
                /* max positive current */
                nControlVar = 11200;
            }
            else if ((step.GetTargetTemp() - _nStartTemperature_mC) < 0)
            {
                /* max negative current */
                nControlVar = -11200;
            }
        }
#endif
        //nControlVar = _pid.calculate(step.GetTargetTemp(), nBlockTemp);
        //nControlVar = _pid.calculate(_nFineTargetTemp_mC, nBlockTemp);
        //_thermalDrv.SetControlVar((int32_t)nControlVar);

        //If we have not yet stabilized on the setpoint?
        if (_siteStatus.GetTempStableFlg() == false)
        {
            //Is temperature within tolerance?
            if ((nBlockTemp >= (step.GetTargetTemp() - _nTempStableTolerance_mC)) &&
                (nBlockTemp <= (step.GetTargetTemp() + _nTempStableTolerance_mC)))
            //if (_nTargetTempReachedFlag == true)
            {
                _siteStatus.SetStableTimer(_siteStatus.GetStableTimer() + kPidTick_ms);
                if (_siteStatus.GetStableTimer() >= _nTempStableTime_ms)
                {
                    _siteStatus.SetTempStableFlg(true);
                    _siteStatus.SetHoldTimer(_nTempStableTime_ms);
                }
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
                   opticsRec._nRefDarkRead          = _opticsDrv.GetActiveLedMonitorPDValue();
                   opticsRec._nIlluminatedRead      = _opticsDrv.GetIlluminatedReading(optRead);
                   opticsRec._nRefIlluminatedRead   = _opticsDrv.GetActiveLedMonitorPDValue();
                   opticsRec._nActiveLedTemp_mC     = _opticsDrv.GetActiveLedTemp();
                   opticsRec._nActiveDiodeTemp_mC   = _opticsDrv.GetActivePhotoDiodeTemp();
                   opticsRec._nShuttleTemp_mC       = 0;
                   _arOpticsRecs.push_back( opticsRec );

                   _opticsDrv.SetLedsOff();
                }
            }
        }

        if ((!_siteStatus.GetPausedFlg()) && (cameraCaptureCount == 0))
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
        _arThermalRecs[_nThermalRecPutIdx]._nCurrent_mA = ((float)_thermalDrv.GetISenseCounts() * 0.56);

        //Next record, and check for wrap.
        _nThermalRecPutIdx = (_nThermalRecPutIdx >= (kMaxThermalRecs - 1)) ? 0 : _nThermalRecPutIdx + 1;

        _nThermalAcqTimer_ms = 0;
    }
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void Site::ExecuteManualControl()
{
    //If the user is setting target temperatures.
    int32_t nBlockTemp = _thermalDrv.GetBlockTemp();
    if (_nManControlState == kTemperatureControl)
    {
        double nControlVar = _pid.calculate(_nManControlTemperature_mC, nBlockTemp);
        _thermalDrv.SetControlVar((int32_t)(nControlVar * 1000));
    }
    else if (_nManControlState == kCurrentControl)
    {
        _thermalDrv.SetControlVar(_nManControlCurrent_mA);
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
ErrCode Site::PauseRun(bool bPause, bool bCaptureCameraImageFlg)
{
    ErrCode     nErrCode = ErrCode::kNoError;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == true)    {
        _siteStatus.SetPausedFlg(bPause);
        _siteStatus.SetCaptureCameraImageFlg(bCaptureCameraImageFlg);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;

    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::DisableManualControl()
{
    _thermalDrv.Disable();
    _nManControlState = kIdle;
    return ErrCode::kNoError;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetManControlTemperature(int32_t nSp_mC)
{
    ErrCode     nErrCode = ErrCode::kNoError;
    
    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _nManControlTemperature_mC = nSp_mC;
        _nManControlState = kTemperatureControl;
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;
    
    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
ErrCode Site::SetManControlCurrent(int32_t nSp_mA)
{
    ErrCode     nErrCode = ErrCode::kNoError;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _nManControlCurrent_mA = nSp_mA;
        _nManControlState = kCurrentControl;
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
ErrCode Site::SetIntegration(uint32_t nDuration_us)
{
    ErrCode     nErrCode = ErrCode::kNoError;

    //If there is not an active run on this site.
    if (_siteStatus.GetRunningFlg() == false)
    {
        _opticsDrv.IntegrateCommand(nDuration_us);
    }
    else
        nErrCode = ErrCode::kRunInProgressErr;

    return nErrCode;
}

///////////////////////////////////////////////////////////////////////////////
void Site::GetSiteStatus(SiteStatus* pSiteStatus)
{
    xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
    _siteStatus.SetNumThermalRecs(GetNumThermalRecs());
    _siteStatus.SetNumOpticsRecs(GetNumOpticsRecs());
    _siteStatus.SetTemperature(_thermalDrv.GetBlockTemp());

    *pSiteStatus = _siteStatus;
    xSemaphoreGive(_sysStatusSemId);
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
void Site::GetAndDelThermalRecs(uint32_t nMaxRecs, std::vector<ThermalRec>* pThermalRecs)
{
    xSemaphoreTake(_sysStatusSemId, portMAX_DELAY);
    for (int i = 0; i < nMaxRecs; i++)
    {
        //If there are more thermal records.
        if (_nThermalRecGetIdx != _nThermalRecPutIdx)
        {
            pThermalRecs->push_back(_arThermalRecs[_nThermalRecGetIdx]);

            //Inc Get index and take care of wrap.
            _nThermalRecGetIdx = (_nThermalRecGetIdx == kMaxThermalRecs-1) ? 0 : _nThermalRecGetIdx + 1;
        }
    }
    xSemaphoreGive(_sysStatusSemId);
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
