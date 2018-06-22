#ifndef _PID_H_
#define _PID_H_

class PIDImpl;
class Pid
{
public:
    // Kp -  proportional gain
    // Ki -  Integral gain
    // Kd -  derivative gain
    // dt -  loop interval time
    // max - maximum value of manipulated variable
    // min - minimum value of manipulated variable
    Pid( double dt, double max, double min, double Kp, double Ki, double Kd );
    void SetProportionalGain(double nPropGain);
    void SetIntegralGain(double nIntegralGain);

    // Returns the manipulated variable given a setpoint and current process value
    double calculate( double setpoint, double pv );
    ~Pid();

private:
    PIDImpl *pimpl;
};
 
#endif
