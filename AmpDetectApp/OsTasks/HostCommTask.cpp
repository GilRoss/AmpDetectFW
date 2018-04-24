#include <memory>
#include <cstdint>
#include "HostCommTask.h"
#include "HostCmdFactory.h"
#include "HostCommDriver.h"

///////////////////////////////////////////////////////////////////////////////
HostCommTask*        HostCommTask::_pHostCommTask = nullptr;         //Singleton.

///////////////////////////////////////////////////////////////////////////////
HostCommTask* HostCommTask::GetInstance()
{
    if (_pHostCommTask == nullptr)
        _pHostCommTask = new HostCommTask;

    return _pHostCommTask;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern "C" void StartHostCommTask(void * pvParameters);
void StartHostCommTask(void * pvParameters)
{
    //Wait for PCR task object to be created.
    if (PcrTask::GetInstancePtr() == nullptr)
        vTaskDelay (100 / portTICK_PERIOD_MS);

    HostCommTask* pHostCommTask = HostCommTask::GetInstance();
    pHostCommTask->ExecuteThread(PcrTask::GetInstancePtr());
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
HostCommTask::HostCommTask()
{
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void HostCommTask::ExecuteThread(PcrTask* pPcrTask)
{
    HostCommDriver*     pHostCommDrv = new HostCommDriver;
    std::unique_ptr<uint8_t[]> pRequestBuf = std::make_unique<uint8_t[]>(HostMsg::kMaxRequestSize);
    
    for (;;)
    {
        //Wait for message from host, then de-serialize it.
        pHostCommDrv->RxMessage(pRequestBuf.get(), HostMsg::kMaxRequestSize);
        HostCommand* pHostCmd = HostCmdFactory::GetHostCmd(pRequestBuf.get(), pHostCommDrv, pPcrTask);

        //If this is a valid command, process the command.
        if (pHostCmd != NULL)
        {
            pHostCmd->Execute();
            delete pHostCmd;
        }
    }
}
