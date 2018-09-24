/*
 * PersistentMem.cpp
 *
 *  Created on: Jul 20, 2018
 *      Author: z003vxjv
 */

#include "PersistentMem.h"

extern "C"
{
#include "crc.h"
#include "ti_fee.h"
}

//#pragma SWI_ALIAS(swiSwitchToMode, 1)

extern void swiSwitchToMode ( uint32 mode );

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PersistentMem*   PersistentMem::_pPersistentMem = nullptr;
uint8_t          PersistentMem::_arBuf[kMaxPMemSize];



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PersistentMem* PersistentMem::GetInstance()      //Singleton
{
    if (_pPersistentMem == nullptr)
    {
        //Initialize flash and wait for complete.
        TI_Fee_Init();
        uint16_t nStatus = UNINIT;
        while(nStatus != IDLE)
        {
            TI_Fee_MainFunction();
            uint16_t dummycnt = 0xFFU;
            while(dummycnt > 0)
                dummycnt--;

            nStatus = TI_Fee_GetStatus(0);
        }

        _pPersistentMem = new PersistentMem;
    }

    return _pPersistentMem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::WriteToFlash()
{
    //Serialize persistent memory.
    this->operator>>(_arBuf);

    //Get current mode, then switch to privileged mode.

    Std_ReturnType nRetVal = TI_Fee_WriteSync(1, _arBuf);

    //Switch back to original mode.portSWITCH_TO_USER_MODE

    return (nRetVal == E_OK);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::ReadFromFlash()
{
    bool bSuccess = false;

    Std_ReturnType nRetVal = TI_Fee_ReadSync(1, 0, _arBuf, kMaxPMemSize);
    if (nRetVal == E_OK)
    {
        //The size of the data last written to persistent memory should be
        //stored in the uint32_t directly following the StreamingObj class.
        uint32_t nSize = *((uint32_t*)&_arBuf[StreamingObj::GetStreamSize()]);

        //If persistent memory has never been written, the _nSize may be bogus.
        //Before calculating the CRC, make sure _nSize is in range.
        if ((nSize < kMaxPMemSize) && (nSize != 0) && ((nSize % sizeof(uint32_t)) == 0))
        {
            //If persistent memory is valid, de-serialize into this object.
            uint32_t nCrc = crcSlow(_arBuf, nSize - sizeof(_nCrc));
            if (nCrc == *((uint32_t*)&_arBuf[nSize - sizeof(_nCrc)]))
            {
                this->operator<<(_arBuf);
                bSuccess = true;
            }
        }
    }

    return bSuccess;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PersistentMem::operator<<(const uint8_t* pData)
{
    StreamingObj::operator<<(pData);
    uint32_t*   pSrc = (uint32_t*)(pData + StreamingObj::GetStreamSize());
    _nSize                  = swap_uint32(*pSrc++);
    _nSerialNum             = swap_uint32(*pSrc++);
    _nCanId                 = swap_uint32(*pSrc++);
    _nFluorDetectorType     = (FluorDetectorType)swap_uint32(*pSrc++);
    _temperaturePidParams    << (uint8_t*)pSrc;
    pSrc += _temperaturePidParams.GetStreamSize() / sizeof(uint32_t);
    _currentPidParams       << (uint8_t*)pSrc;
    pSrc += _currentPidParams.GetStreamSize() / sizeof(uint32_t);
    _nCrc                   = swap_uint32(*pSrc++);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void PersistentMem::operator>>(uint8_t* pData)
{
    StreamingObj::operator>>(pData);
    uint32_t*   pDst = (uint32_t*)(pData + StreamingObj::GetStreamSize());
    *pDst++ = swap_uint32(GetStreamSize());
    *pDst++ = swap_uint32(_nSerialNum);
    *pDst++ = swap_uint32(_nCanId);
    *pDst++ = swap_uint32((uint32_t)_nFluorDetectorType);
    _temperaturePidParams >> (uint8_t*)pDst;
    pDst += _temperaturePidParams.GetStreamSize() / sizeof(uint32_t);
    _currentPidParams >> (uint8_t*)pDst;
    pDst += _currentPidParams.GetStreamSize() / sizeof(uint32_t);

    //Calc and store CRC.
    _nCrc = crcSlow(pData, GetStreamSize() - sizeof(_nCrc));
    *pDst++ = swap_uint32(_nCrc);
}

