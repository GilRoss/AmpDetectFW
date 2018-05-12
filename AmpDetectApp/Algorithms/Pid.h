#ifndef __Pid__
#define __Pid__

#include <cstdint>


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class Pid
{
public:
    Pid(int32_t nKp = 0, int32_t nKi = 0, int32_t nKd = 0, int32_t nMinPwr_uA = 0, int32_t nMaxPwr_uA = 0, uint32_t nShift = 0)
        : _nKp(nKp)
        , _nKi(nKi)
        , _nKd(nKd)
        , _nShift(nShift)
        , _nMaxPwr(nMaxPwr_uA)
        , _nMinPwr(nMinPwr_uA)
        , _nIntegrationAcc(0)
        , _bIsStable(false)
        , _nStableBand(250)
        , _nStablePeriod_ms(500)
        , _nStableTimer_ms(0)
    {
    }
    
    bool Service(uint32_t nPeriod_ms, int32_t nSetpoint_mC, int32_t nProcessVar, int32_t nProcessErr, int32_t* pControlVar);
 
    void        SetKp(uint32_t nKp)         {_nKp = nKp;}
    uint32_t    GetKp() const               {return _nKp;}    
    void        SetKi(uint32_t nKi)         {_nKi = nKi;}
    uint32_t    GetKi() const               {return _nKi;}    
    void        SetKd(uint32_t nKd)         {_nKd = nKd;}
    uint32_t    GetKd() const               {return _nKd;}    
    void        SetStableFlg(bool b)        {_bIsStable = b;}
    bool        GetStableFlg() const        {return _bIsStable;}    
    void        SetStableBand(int32_t n)    {_nStableBand = n;}
    int32_t     GetStableBand() const       {return _nStableBand;}
    void        SetStablePeriod(int32_t n)  {_nStablePeriod_ms = n;}
    int32_t     GetStablePeriod() const     {return _nStablePeriod_ms;}    
    void        ResetStableFlg();

protected:

private:
    int32_t     _nKp;               //Proportional gain
    int32_t     _nKi;               //Integral gain
    int32_t     _nKd;               //Derivative gain
    uint32_t    _nShift;            //Right shift to divide
    int32_t     _nMaxPwr;           //Maximum value
    int32_t     _nMinPwr;           //Minimum value
    int32_t     _nIntegrationAcc;   //Integration accumulator
    
    bool        _bIsStable;
    int32_t     _nStableBand;       //Stability band (+/-).
    uint32_t    _nStablePeriod_ms;  //Period temp must be within stability band.
    uint32_t    _nStableTimer_ms;   //Timer.
};

#endif /* __Pid__ */
