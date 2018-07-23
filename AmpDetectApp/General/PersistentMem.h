/*
 * PersistentMem.h
 *
 *  Created on: Jul 20, 2018
 *      Author: z003vxjv
 */

#ifndef PERSISTENTMEM_H_
#define PERSISTENTMEM_H_

#include "Common.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PersistentMem
{
public:
    static PersistentMem* GetInstance()      //Singleton
    {
        if (_pPersistentMem == nullptr)
            _pPersistentMem = new PersistentMem;
        return _pPersistentMem;
    }

    virtual ~PersistentMem() {}

    void                SetSerialNum(uint32_t n)    { _nSerialNum = n; }
    uint32_t            GetSerialNum() const        { return _nSerialNum; }
    void                SetCanId(uint32_t n)        { _nCanId = n; }
    uint32_t            GetCanId() const            { return _nCanId; }
    void                SetFluorDetectorType(FluorDetectorType n)    { _nFluorDetectorType = n; }
    FluorDetectorType   GetFluorDetectorType() const                 { return _nFluorDetectorType; }
    void                SetTemperaturePidParams(const PidParams& r)  { _temeraturePidParams = r;}
    PidParams           GetTemperaturePidParams() const              { return _temeraturePidParams; }
    void                SetCurrentPidParams(const PidParams& r)      { _currentPidParams = r; }
    PidParams           GetCurrentPidParams() const                  { return _currentPidParams; }

    bool                WriteToFlash()
    {
        return true;
    }

    bool                ReadFromFlash()
    {
        return true;
    }

protected:

private:
    PersistentMem()         //Singleton
        : _nSerialNum(0)
        , _nCanId(1)
        , _nFluorDetectorType(FluorDetectorType::kPhotoDiode)
    {
    }
    static PersistentMem* _pPersistentMem;

    uint32_t             _nSerialNum;
    uint32_t             _nCanId;
    FluorDetectorType    _nFluorDetectorType;
    PidParams            _temeraturePidParams;
    PidParams            _currentPidParams;
};

#endif /* PERSISTENTMEM_H_ */
