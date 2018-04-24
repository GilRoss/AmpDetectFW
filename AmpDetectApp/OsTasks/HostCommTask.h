#ifndef __HostCommTask_H
#define __HostCommTask_H



class PcrTask;

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class HostCommTask
{
public:
  
    static HostCommTask* GetInstance();
    static HostCommTask* GetInstancePtr()    {return _pHostCommTask;}   //No instantiation.

    void ExecuteThread(PcrTask* pPcrTask);
    
protected:
  
private:
    HostCommTask();            //Singleton.

    static HostCommTask*       _pHostCommTask;
};

#endif // __HostCommTask_H
