#ifndef __CurrentPidTask_H
#define __CurrentPidTask_H

#include <cstdint>

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class CurrentPidTask
{
public:
    ~CurrentPidTask();
  
    static CurrentPidTask*  GetInstance();
    static CurrentPidTask*  GetInstancePtr()    {return _pCurrentPidTask;}   //No instantiation.
    void                    ExecuteThread();

protected:
  
private:
    CurrentPidTask();              //Singleton.

    float                   GetProcessVar();
    void                    SetControlVar(float nControlVar);

    static CurrentPidTask*  _pCurrentPidTask;

    bool                    _bEnabled;
    float                   _nCurrentSetpoint;
    float                   _nProportionalGain;
    float                   _nIntegralGain;
    float                   _nDerivativeGain;

    float                   _nPrevPidError;
    float                   _nAccError;
};

#endif // __CurrentPidTask_H
