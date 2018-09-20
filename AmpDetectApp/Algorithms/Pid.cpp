#ifndef _PID_SOURCE_
#define _PID_SOURCE_

#include <iostream>
#include <cmath>
#include "pid.h"

using namespace std;

class PIDImpl
{
    public:
        PIDImpl( double dt, double max, double min, double Kp, double Ki, double Kd );
        ~PIDImpl();
        void Init();
        double calculate( double setpoint, double pv );
        void SetGains(double nKp, double nKi, double nKd)
        {
            _Kp = nKp;
            _Ki = nKi;
            _Kd = nKd;
        }

    private:
        double _dt;
        double _max;
        double _min;
        double _Kp;
        double _Kd;
        double _Ki;
        double _pre_error;
        double _integral;
};


Pid::Pid( double dt, double max, double min, double Kp, double Ki, double Kd )
{
    pimpl = new PIDImpl(dt,max,min,Kp,Ki,Kd);
}
Pid::~Pid()
{
    delete pimpl;
}
void Pid::Init( void )
{
    pimpl->Init();
}
double Pid::calculate( double setpoint, double pv )
{
    return pimpl->calculate(setpoint,pv);
}

void Pid::SetGains(double nKp, double nKi, double nKd)
{
    pimpl->SetGains(nKp, nKi, nKd);
}


/**
 * Implementation
*/
PIDImpl::PIDImpl( double dt, double max, double min, double Kp, double Ki, double Kd ) :
    _dt(dt),
    _max(max),
    _min(min),
    _Kp(Kp),
    _Kd(Kd),
    _Ki(Ki),
    _pre_error(0),
    _integral(0)
{
}

void PIDImpl::Init( void )
{
    _pre_error  = 0;
    _integral   = 0;
}

double PIDImpl::calculate( double setpoint, double pv )
{
    // Calculate error
    double error = setpoint - pv;

    // Proportional term
    double Pout = _Kp * error;

    // Integral term
    _integral += error; // * _dt;
    double Iout = _Ki * _integral;

    // Derivative term
    double derivative = (error - _pre_error) / _dt;
    double Dout = _Kd * derivative;

    // Calculate total output
    double output = Pout + Iout + Dout;

    // Restrict to max/min
    if( output > _max )
    {
        output = _max;
        _integral -= error;
    }
    else if( output < _min )
    {
        output = _min;
        _integral -= error;
    }

    // Save error to previous error
    _pre_error = error;

    return output;
}

PIDImpl::~PIDImpl()
{
}

#endif
