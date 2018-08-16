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
        _pPersistentMem = new PersistentMem;

//    Std_ReturnType nRetVal = TI_Fee_ReadSync(1, 0, _arBuf, kMaxPMemSize);
//    if (nRetVal != E_OK)
//        nRetVal = TI_Fee_WriteSync(1, _arBuf);

    return _pPersistentMem;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::WriteToFlash()
{
    //Serialize persistent memory.
    this->operator>>(_arBuf);
    for (int i = 0; i < kMaxPMemSize; i++)
        _arBuf[i] = i;

    //Go into privileged mode.

    Std_ReturnType nRetVal = TI_Fee_WriteSync(1, _arBuf);

    //Return to user mode.

    return (nRetVal == E_OK);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool PersistentMem::ReadFromFlash()
{
    Std_ReturnType nRetVal = TI_Fee_ReadSync(1, 0, _arBuf, kMaxPMemSize);

    //Deserialize persistent memory.
    this->operator<<(_arBuf);

    return (nRetVal == E_OK);
}
