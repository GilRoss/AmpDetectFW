/*
 * PersistentMem.cpp
 *
 *  Created on: Jul 20, 2018
 *      Author: z003vxjv
 */

#include "PersistentMem.h"

extern "C"
{
#include "ti_fee.h"
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PersistentMem*   PersistentMem::_pPersistentMem = nullptr;
uint8_t          PersistentMem::_arBuf[kMaxPMemSize];



///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
PersistentMem* PersistentMem::GetInstance()      //Singleton
{
    if (_pPersistentMem == nullptr)
        _pPersistentMem = new PersistentMem;

    bool bSuccess = _pPersistentMem->ReadFromFlash();
    if (bSuccess == false)
        _pPersistentMem->WriteToFlash();

    for (int i = 0; i < kMaxPMemSize; i++)
        _arBuf[i] = i;
    Std_ReturnType nRetVal = TI_Fee_WriteSync(1, _arBuf);

    for (int i = 0; i < kMaxPMemSize; i++)
        _arBuf[i] = 0;
    nRetVal = TI_Fee_ReadSync(1, 0, _arBuf, kMaxPMemSize);

    return _pPersistentMem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::WriteToFlash()
{
    Std_ReturnType nRetVal = TI_Fee_WriteSync(1, _arBuf);
    return (nRetVal == E_OK);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::ReadFromFlash()
{
    Std_ReturnType nRetVal = TI_Fee_ReadSync(1, 0, _arBuf, kMaxPMemSize);
    return (nRetVal == E_OK);
}
