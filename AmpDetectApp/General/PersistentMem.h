/*
 * PersistentMem.h
 *
 *  Created on: Jul 20, 2018
 *      Author: z003vxjv
 */

#ifndef PERSISTENTMEM_H_
#define PERSISTENTMEM_H_

#include "Common.h"
#include "StreamingObj.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class PersistentMem : StreamingObj
{
public:
    enum    {kMaxPMemSize = 1024};

    static PersistentMem* GetInstance();      //Singleton

    virtual ~PersistentMem()
    {
    }

    void                SetSerialNum(uint32_t n)    { _nSerialNum = n; }
    uint32_t            GetSerialNum() const        { return _nSerialNum; }
    void                SetCanId(uint32_t n)        { _nCanId = n; }
    uint32_t            GetCanId() const            { return _nCanId; }
    void                SetFluorDetectorType(FluorDetectorType n)    { _nFluorDetectorType = n; }
    FluorDetectorType   GetFluorDetectorType() const                 { return _nFluorDetectorType; }
    void                SetTemperaturePidParams(const PidParams& r)  { _temperaturePidParams = r;}
    PidParams           GetTemperaturePidParams() const              { return _temperaturePidParams; }
    void                SetCurrentPidParams(const PidParams& r)      { _currentPidParams = r; }
    PidParams           GetCurrentPidParams() const                  { return _currentPidParams; }

    bool                WriteToFlash();
    bool                ReadFromFlash();

    virtual uint32_t GetStreamSize() const
    {
        uint32_t nSize = StreamingObj::GetStreamSize();
        nSize += sizeof(_nSerialNum);
        nSize += sizeof(_nCanId);
        nSize += sizeof(_nFluorDetectorType);
        nSize += _temperaturePidParams.GetStreamSize();
        nSize += _currentPidParams.GetStreamSize();
        nSize += sizeof(_nCrc);
        return nSize;
    }

    virtual void     operator<<(const uint8_t* pData);
    virtual void     operator>>(uint8_t* pData);

protected:

private:
    PersistentMem()         //Singleton
        : StreamingObj((MakeObjId('P', 'M', 'e', 'm')))
        , _nSerialNum(0)
        , _nCanId(1)
        , _nFluorDetectorType(FluorDetectorType::kPhotoDiode)
        , _temperaturePidParams(0.0007, 0.00007, 0, 1000, 0)
        , _currentPidParams(0.4, 1475, 0, 1000, 0)
        , _nCrc(0)
    {
        bool bSuccess = ReadFromFlash();
        if (bSuccess == false)
            bSuccess = WriteToFlash();
    }
    static PersistentMem*   _pPersistentMem;
    static uint8_t          _arBuf[];

    uint32_t             _nSerialNum;
    uint32_t             _nCanId;
    FluorDetectorType    _nFluorDetectorType;
    PidParams            _temperaturePidParams;
    PidParams            _currentPidParams;
    uint32_t             _nCrc;
};

#endif /* PERSISTENTMEM_H_ */
