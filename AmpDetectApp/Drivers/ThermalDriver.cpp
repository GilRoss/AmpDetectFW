#include "ThermalDriver.h"
#include "gio.h"
#include "mibspi.h"
#include "het.h"
#include "stdio.h"


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Pid          ThermalDriver::_pid(0.000050, 14000, -14000, 1.0, 0.0, 0);    //Fixed 2ohm load.
bool         ThermalDriver::_bCurrentPidEnabled = false;
uint32_t     ThermalDriver::_nIsrCount = 0;
uint32_t     ThermalDriver::_nBlockTemp_cnts = 0;
uint32_t     ThermalDriver::_nSampleTemp_cnts = 0;
int32_t      ThermalDriver::_nSetpoint_mA;
uint32_t     ThermalDriver::_nProportionalGain;
uint32_t     ThermalDriver::_nIntegralGain;
uint32_t     ThermalDriver::_nDerivativeGain;
int32_t      ThermalDriver::_nPrevPidError_A2DCounts;
int32_t      ThermalDriver::_nAccError_A2DCounts;
double       ThermalDriver::_nControlVar;
int32_t      ThermalDriver::_nA2DCounts = 0;
int32_t      ThermalDriver::_nErrCounts = 0;



//static int32_t  _arA2DVals[8];
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
ThermalDriver::ThermalDriver(uint32_t nSiteIdx)
{
    //Set direction (output) and initial state of D/A control pins.
    gioSetBit(gioPORTA, kClearBit, 1);
    gioSetBit(gioPORTA, kSyncBit, 1);
    gioSetBit(gioPORTA, kLdacBit, 0);
    uint32_t nDirMsk = (0x01 << kClearBit) + (0x01 << kSyncBit) + (0x01 << kLdacBit);
    gioSetDirection(gioPORTA, nDirMsk);

    //To generate the 4.096V ref for the ADS8330, initialize the temperature A/D.
    AD7699Read(0);

    ADS8330Init();
    AD5683Write(0x04, 0x0800);  //Config A/D
//    for (int i = 0; i < (sizeof(_arA2DVals) / sizeof(uint32_t)); i++)
//        _arA2DVals[i] = ADS8330ReadWrite(0x0D, 0x0000);

//    gioSetBit(mibspiPORT5, PIN_SIMO, 0);    //Disable TEC.

    //Initialize DAC8563
/*    SendDacMsg(CMD_RESET, 0, POWER_ON_RESET);
    SendDacMsg(CMD_ENABLE_INT_REF, 0, DISABLE_INT_REF_AND_RESET_DAC_GAINS_TO_1);
    SendDacMsg(CMD_SET_LDAC_PIN, 0, SET_LDAC_PIN_INACTIVE_DAC_B_INACTIVE_DAC_A);
    SendDacMsg(CMD_POWER_DAC, 0, POWER_DOWN_DAC_B_HI_Z);
    SetCurrentControlVar((0xFFFF / 2) + 180);*/
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
extern "C" void CurrentPidISR();
void CurrentPidISR()
{
    ThermalDriver::CurrentPidISR();
}

void ThermalDriver::CurrentPidISR()
{
    _pid.SetGains(0.4, 1475, 0);
//    _arA2DVals[_nIsrCount & ((sizeof(_arA2DVals) / sizeof(_arA2DVals[0])) - 1)] = ADS8330ReadWrite(0x0D, 0x0000);
    int32_t nA2DCounts = ADS8330ReadWrite(0x0D, 0x0000);
    if (_bCurrentPidEnabled)
    {
        //Get average of last 'n' current readings.
//        for (int i = 0; i < (sizeof(_arA2DVals) / sizeof(_arA2DVals[0])); i++)
//            nA2DCounts += _arA2DVals[i];
//        nA2DCounts = (~(((nA2DCounts >> 3) + 230) - 0x7FFF)) + 1;

        nA2DCounts = (~(((nA2DCounts) + 230) - 0x7FFF)) + 1;
        float nControlVar = _pid.calculate((float)_nSetpoint_mA, (float)nA2DCounts * 0.59);

        AD5683Write(0x03, ((uint16_t)0x8000 - 410) - nControlVar);
        gioSetBit(hetPORT1, PIN_HET_16, 1); //TEC_EN = true
    }
    else
    {
        gioSetBit(hetPORT1, PIN_HET_16, 0); //TEC_EN = false
    }
    _nIsrCount++;
}

/*        static uint32_t arVals[1000];
        arVals[_nIsrCount++] = nA2DCounts;
        if (_nIsrCount >= 1000)
        {
            gioSetBit(hetPORT1, PIN_HET_16, 0); //TEC_EN = false
            for (int i = 0; i < 1000; i++)
            {
                printf("%d\n", arVals[i]);
                fflush (stdout);
            }
            _nControlVar = 0;
        }
*/

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetPidParams(const PidParams& params)
{

}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::Enable()
{
    _pid.Init();
//    uint32_t nA2DVal = ADS8330ReadWrite(0x0D, 0x0000);
//    for (int i = 0; i < (sizeof(_arA2DVals) / sizeof(_arA2DVals[0])); i++)
//        _arA2DVals[i] = nA2DVal;
    _bCurrentPidEnabled = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::Reset()
{
    //Initialize DAC8563
    SendDacMsg(CMD_RESET, 0, POWER_ON_RESET);
    SendDacMsg(CMD_ENABLE_INT_REF, 0, DISABLE_INT_REF_AND_RESET_DAC_GAINS_TO_1);
    SendDacMsg(CMD_SET_LDAC_PIN, 0, SET_LDAC_PIN_INACTIVE_DAC_B_INACTIVE_DAC_A);
    SendDacMsg(CMD_POWER_DAC, 0, POWER_DOWN_DAC_B_HI_Z);
    SetCurrentControlVar((0xFFFF / 2) + 180);
    _bCurrentPidEnabled = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetControlVar(int32_t nControlVar)
{
    SetCurrentSetpoint(nControlVar);
    _bCurrentPidEnabled = true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
bool ThermalDriver::TempIsStable()
{
    return true;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t ThermalDriver::GetSampleTemp()
{
    float nVoltage_V = _nSampleTemp_cnts * (5.0 / 65535);
    int32_t nSampleTemp_mC =  convertVoltageToTemp(nVoltage_V);

    return nSampleTemp_mC;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t ThermalDriver::GetBlockTemp()
{
    uint16_t nA2DCounts = AD7699Read(0);
    float nResistance_omhs = ((float)nA2DCounts * 0.34) - 700;
    int32_t nBlockTemp_mC =  convertVoltageToTemp(nResistance_omhs);

    return nBlockTemp_mC;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetCurrentSetpoint(int32_t nCurrent_mA)
{
    _nSetpoint_mA = nCurrent_mA;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    ThermalDriver::SetCurrentControlVar(uint32_t nControlVar_D2ACounts)
{
    SendDacMsg(CMD_WR_ONE_REG_AND_UPDATE_ONE_DAC, ADDR_DAC_A,
               (uint8_t)(nControlVar_D2ACounts >> 8), (uint8_t)nControlVar_D2ACounts);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void    ThermalDriver::SendDacMsg(uint8_t nCmd, uint8_t nAdr, uint8_t nHiByte, uint8_t nLoByte)
{
    uint16_t    arTxBuf[3] = {0x0000};

    //Using MibSpi1, prepare and write the message.
    arTxBuf[0] = (uint16_t)((nCmd << 3) + nAdr);
    arTxBuf[1] = (uint16_t)nHiByte;
    arTxBuf[2] = (uint16_t)nLoByte;

    gioSetBit(gioPORTA, kSyncBit, 0);   //CS
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
    gioSetBit(gioPORTA, kSyncBit, 1);   //CS
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
int32_t    ThermalDriver::GetProcessVar()
{
    uint16_t    arTxBuf[3] = {0x0000, 0x0000, 0x0000};
    uint16_t    arRxBuf[3] = {0x0000};

    //Use MibSpi1 to read the current value.
    gioSetBit(gioPORTA, kSyncBit, 0);
    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));
    gioSetBit(gioPORTA, kSyncBit, 1);

    //Read the A/D.
    mibspiGetData(mibspiREG1, 0, arRxBuf);

    return 0; //return nProcessVar_mA;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::ReadDacMsg(uint16 cfg, uint16_t* pData)
{
    uint16_t    arTxBuf[3];
    uint16_t    arRxBuf[3];

    arTxBuf[0] = cfg >> 8;
    arTxBuf[1] = cfg & 0x00FF;

    gioSetBit(mibspiPORT5, PIN_ENA, 0);          //Conversion pin
    mibspiSetData(mibspiREG5, 0, arTxBuf);
    mibspiTransfer(mibspiREG5, 0);
    while (! mibspiIsTransferComplete(mibspiREG5, 0));
    gioSetBit(mibspiPORT5, PIN_ENA, 1);          //Conversion pin
    mibspiGetData(mibspiREG5, 0, arRxBuf);
    gioSetBit(mibspiPORT5, PIN_ENA, 1);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_ENA, 1);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_ENA, 1);          //Conversion pin

    *pData = (arRxBuf[0] << 8) + arRxBuf[1];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::AD7699Read(int nChanIdx)
{
    uint16 data;
    uint16 cfg = 0;
    cfg |= OVERWRITE_CFG << CFG_SHIFT;
    cfg |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    cfg |= nChanIdx << IN_CH_SEL_SHIFT;
    cfg |= FULL_BW << FULL_BW_SEL_SHIFT;
    cfg |= INT_REF4_096_AND_TEMP_SENS << REF_SEL_SHIFT;
    cfg |= DISABLE_SEQ << SEQ_EN_SHIFT;
    cfg |= READ_BACK_DISABLE << READ_BACK_SHIFT;
    cfg <<= 2;

    ReadDacMsg(cfg, &data); // conv DATA(n-1), clock out CFG(n), clock in DATA(n-2)
    ReadDacMsg(0x00, &data); // conv DATA(n), clock out CFG(n+1), clock in DATA(n-1)
    ReadDacMsg(0x00, &data); // conv DATA(n+1), clock out CFG(n+2), clock in DATA(n)

    return data;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::AD5683Write(uint16_t nCmd, uint16_t nA2DCounts)
{
    uint16_t    arTxBuf[3] = {0x0000, 0x0000, 0x0000};

    arTxBuf[0] += (nCmd << 4) + (nA2DCounts >> 12);
    arTxBuf[1] += (nA2DCounts >> 4) & 0xFF;
    arTxBuf[2] += (nA2DCounts & 0x0F) << 4;

    mibspiSetData(mibspiREG1, 0, arTxBuf);
    mibspiTransfer(mibspiREG1, 0);
    while (! mibspiIsTransferComplete(mibspiREG1, 0));

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::ADS8330ReadWrite(uint16_t nCmd, uint16_t nCfr)
{
    uint16_t    arTxBuf[2] = {0x0000, 0x0000};
    uint16_t    arRxBuf[2] = {0x0000, 0x0000};

    arTxBuf[0] += (nCmd << 4) + (nCfr >> 8);
    arTxBuf[1] += nCfr & 0xFF;

    mibspiSetData(mibspiREG1, 1, arTxBuf);
    mibspiTransfer(mibspiREG1, 1);
    while (! mibspiIsTransferComplete(mibspiREG1, 1));
    mibspiGetData(mibspiREG1, 1, arRxBuf);

    return (arRxBuf[0] << 8) + (arRxBuf[1] & 0xFF);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::ADS8330Init()
{
    //Set configuration register.
    ADS8330ReadWrite(0x0E, 0x04FD);
    uint32_t nTemp = ADS8330ReadWrite(0x0C, 0x0000);

    //Select channel 0.
    ADS8330ReadWrite(0x00, 0x0000);
}


// TEMP_AINx = 2.048 * Rt/(107000 + Rt)
// 1/TEMP_AINx = (107000 / Rt + 1) / 2.048
// 1 / Rt = (2.048 /TEMP_AINx - 1) / 107000
int32_t ThermalDriver::convertVoltageToTemp(float nResistance_omhs, int standard)
{
    float rt = nResistance_omhs;
    int i;
    float temp;
    if (rt > s_convTable[0].rt) {
        return s_convTable[0].temp_C;
    }
    for (i = 0; s_convTable[i].rt; i++) {
        if (s_convTable[i].rt >= rt && rt >= s_convTable[i + 1].rt) {
            if (s_convTable[i + 1].rt == 0) {
                temp = s_convTable[i].temp_C;
                break;
            }
            else {
                temp = s_convTable[i + 1].temp_C
                  + (rt - s_convTable[i + 1].rt)
                  * (s_convTable[i].temp_C - s_convTable[i + 1].temp_C)
                  / (s_convTable[i].rt - s_convTable[i + 1].rt);
                 break;
            }
        }
    }
    if (!s_convTable[i].rt) {
        return s_convTable[i].temp_C;
    }
    return (int32_t)(temp * 1000);
}

const conversion ThermalDriver::s_convTable[] = {
    { .rt = 24600, .temp_C = 4.574 },
    { .rt = 24500, .temp_C = 4.659 },
    { .rt = 24400, .temp_C = 4.744 },
    { .rt = 24300, .temp_C = 4.829 },
    { .rt = 24200, .temp_C = 4.914 },
    { .rt = 24100, .temp_C = 5 },
    { .rt = 24000, .temp_C = 5.087 },
    { .rt = 23900, .temp_C = 5.174 },
    { .rt = 23800, .temp_C = 5.261 },
    { .rt = 23700, .temp_C = 5.348 },
    { .rt = 23600, .temp_C = 5.436 },
    { .rt = 23500, .temp_C = 5.525 },
    { .rt = 23400, .temp_C = 5.614 },
    { .rt = 23300, .temp_C = 5.703 },
    { .rt = 23200, .temp_C = 5.793 },
    { .rt = 23100, .temp_C = 5.883 },
    { .rt = 23000, .temp_C = 5.974 },
    { .rt = 22900, .temp_C = 6.065 },
    { .rt = 22800, .temp_C = 6.156 },
    { .rt = 22700, .temp_C = 6.248 },
    { .rt = 22600, .temp_C = 6.34 },
    { .rt = 22500, .temp_C = 6.433 },
    { .rt = 22400, .temp_C = 6.527 },
    { .rt = 22300, .temp_C = 6.62 },
    { .rt = 22200, .temp_C = 6.715 },
    { .rt = 22100, .temp_C = 6.809 },
    { .rt = 22000, .temp_C = 6.905 },
    { .rt = 21900, .temp_C = 7 },
    { .rt = 21800, .temp_C = 7.097 },
    { .rt = 21700, .temp_C = 7.193 },
    { .rt = 21600, .temp_C = 7.29 },
    { .rt = 21500, .temp_C = 7.388 },
    { .rt = 21400, .temp_C = 7.486 },
    { .rt = 21300, .temp_C = 7.585 },
    { .rt = 21200, .temp_C = 7.684 },
    { .rt = 21100, .temp_C = 7.784 },
    { .rt = 21000, .temp_C = 7.884 },
    { .rt = 20900, .temp_C = 7.985 },
    { .rt = 20800, .temp_C = 8.086 },
    { .rt = 20700, .temp_C = 8.188 },
    { .rt = 20600, .temp_C = 8.291 },
    { .rt = 20500, .temp_C = 8.394 },
    { .rt = 20400, .temp_C = 8.497 },
    { .rt = 20300, .temp_C = 8.601 },
    { .rt = 20200, .temp_C = 8.706 },
    { .rt = 20100, .temp_C = 8.811 },
    { .rt = 20000, .temp_C = 8.917 },
    { .rt = 19900, .temp_C = 9.024 },
    { .rt = 19800, .temp_C = 9.131 },
    { .rt = 19700, .temp_C = 9.239 },
    { .rt = 19600, .temp_C = 9.347 },
    { .rt = 19500, .temp_C = 9.456 },
    { .rt = 19400, .temp_C = 9.565 },
    { .rt = 19300, .temp_C = 9.676 },
    { .rt = 19200, .temp_C = 9.786 },
    { .rt = 19100, .temp_C = 9.898 },
    { .rt = 19000, .temp_C = 10.01 },
    { .rt = 18900, .temp_C = 10.123 },
    { .rt = 18800, .temp_C = 10.236 },
    { .rt = 18700, .temp_C = 10.35 },
    { .rt = 18600, .temp_C = 10.465 },
    { .rt = 18500, .temp_C = 10.581 },
    { .rt = 18400, .temp_C = 10.697 },
    { .rt = 18300, .temp_C = 10.814 },
    { .rt = 18200, .temp_C = 10.931 },
    { .rt = 18100, .temp_C = 11.05 },
    { .rt = 18000, .temp_C = 11.169 },
    { .rt = 17900, .temp_C = 11.289 },
    { .rt = 17800, .temp_C = 11.409 },
    { .rt = 17700, .temp_C = 11.531 },
    { .rt = 17600, .temp_C = 11.653 },
    { .rt = 17500, .temp_C = 11.776 },
    { .rt = 17400, .temp_C = 11.899 },
    { .rt = 17300, .temp_C = 12.024 },
    { .rt = 17200, .temp_C = 12.149 },
    { .rt = 17100, .temp_C = 12.275 },
    { .rt = 17000, .temp_C = 12.402 },
    { .rt = 16900, .temp_C = 12.53 },
    { .rt = 16800, .temp_C = 12.659 },
    { .rt = 16700, .temp_C = 12.788 },
    { .rt = 16600, .temp_C = 12.919 },
    { .rt = 16500, .temp_C = 13.05 },
    { .rt = 16400, .temp_C = 13.182 },
    { .rt = 16300, .temp_C = 13.315 },
    { .rt = 16200, .temp_C = 13.449 },
    { .rt = 16100, .temp_C = 13.584 },
    { .rt = 16000, .temp_C = 13.72 },
    { .rt = 15900, .temp_C = 13.857 },
    { .rt = 15800, .temp_C = 13.994 },
    { .rt = 15700, .temp_C = 14.133 },
    { .rt = 15600, .temp_C = 14.273 },
    { .rt = 15500, .temp_C = 14.414 },
    { .rt = 15400, .temp_C = 14.555 },
    { .rt = 15300, .temp_C = 14.698 },
    { .rt = 15200, .temp_C = 14.842 },
    { .rt = 15100, .temp_C = 14.987 },
    { .rt = 15000, .temp_C = 15.133 },
    { .rt = 14900, .temp_C = 15.28 },
    { .rt = 14800, .temp_C = 15.428 },
    { .rt = 14700, .temp_C = 15.578 },
    { .rt = 14600, .temp_C = 15.728 },
    { .rt = 14500, .temp_C = 15.88 },
    { .rt = 14400, .temp_C = 16.032 },
    { .rt = 14300, .temp_C = 16.187 },
    { .rt = 14200, .temp_C = 16.342 },
    { .rt = 14100, .temp_C = 16.498 },
    { .rt = 14000, .temp_C = 16.656 },
    { .rt = 13900, .temp_C = 16.815 },
    { .rt = 13800, .temp_C = 16.975 },
    { .rt = 13700, .temp_C = 17.137 },
    { .rt = 13600, .temp_C = 17.3 },
    { .rt = 13500, .temp_C = 17.464 },
    { .rt = 13400, .temp_C = 17.629 },
    { .rt = 13300, .temp_C = 17.796 },
    { .rt = 13200, .temp_C = 17.965 },
    { .rt = 13100, .temp_C = 18.135 },
    { .rt = 13000, .temp_C = 18.306 },
    { .rt = 12900, .temp_C = 18.479 },
    { .rt = 12800, .temp_C = 18.653 },
    { .rt = 12700, .temp_C = 18.829 },
    { .rt = 12600, .temp_C = 19.006 },
    { .rt = 12500, .temp_C = 19.185 },
    { .rt = 12400, .temp_C = 19.366 },
    { .rt = 12300, .temp_C = 19.548 },
    { .rt = 12200, .temp_C = 19.732 },
    { .rt = 12100, .temp_C = 19.918 },
    { .rt = 12000, .temp_C = 20.105 },
    { .rt = 11900, .temp_C = 20.294 },
    { .rt = 11800, .temp_C = 20.485 },
    { .rt = 11700, .temp_C = 20.678 },
    { .rt = 11600, .temp_C = 20.873 },
    { .rt = 11500, .temp_C = 21.069 },
    { .rt = 11400, .temp_C = 21.268 },
    { .rt = 11300, .temp_C = 21.468 },
    { .rt = 11200, .temp_C = 21.67 },
    { .rt = 11100, .temp_C = 21.875 },
    { .rt = 11000, .temp_C = 22.081 },
    { .rt = 10900, .temp_C = 22.29 },
    { .rt = 10800, .temp_C = 22.501 },
    { .rt = 10700, .temp_C = 22.714 },
    { .rt = 10600, .temp_C = 22.929 },
    { .rt = 10500, .temp_C = 23.147 },
    { .rt = 10400, .temp_C = 23.367 },
    { .rt = 10300, .temp_C = 23.589 },
    { .rt = 10200, .temp_C = 23.814 },
    { .rt = 10100, .temp_C = 24.041 },
    { .rt = 10000, .temp_C = 24.271 },
    { .rt = 9900, .temp_C = 24.503 },
    { .rt = 9800, .temp_C = 24.738 },
    { .rt = 9700, .temp_C = 24.976 },
    { .rt = 9600, .temp_C = 25.216 },
    { .rt = 9500, .temp_C = 25.46 },
    { .rt = 9400, .temp_C = 25.706 },
    { .rt = 9300, .temp_C = 25.955 },
    { .rt = 9200, .temp_C = 26.207 },
    { .rt = 9100, .temp_C = 26.463 },
    { .rt = 9000, .temp_C = 26.721 },
    { .rt = 8900, .temp_C = 26.983 },
    { .rt = 8800, .temp_C = 27.248 },
    { .rt = 8700, .temp_C = 27.517 },
    { .rt = 8600, .temp_C = 27.789 },
    { .rt = 8500, .temp_C = 28.064 },
    { .rt = 8400, .temp_C = 28.343 },
    { .rt = 8300, .temp_C = 28.627 },
    { .rt = 8200, .temp_C = 28.914 },
    { .rt = 8100, .temp_C = 29.204 },
    { .rt = 8000, .temp_C = 29.499 },
    { .rt = 7900, .temp_C = 29.799 },
    { .rt = 7800, .temp_C = 30.102 },
    { .rt = 7700, .temp_C = 30.41 },
    { .rt = 7600, .temp_C = 30.722 },
    { .rt = 7500, .temp_C = 31.039 },
    { .rt = 7400, .temp_C = 31.361 },
    { .rt = 7300, .temp_C = 31.688 },
    { .rt = 7200, .temp_C = 32.02 },
    { .rt = 7100, .temp_C = 32.357 },
    { .rt = 7000, .temp_C = 32.7 },
    { .rt = 6900, .temp_C = 33.048 },
    { .rt = 6800, .temp_C = 33.402 },
    { .rt = 6700, .temp_C = 33.762 },
    { .rt = 6600, .temp_C = 34.128 },
    { .rt = 6500, .temp_C = 34.5 },
    { .rt = 6400, .temp_C = 34.879 },
    { .rt = 6300, .temp_C = 35.264 },
    { .rt = 6200, .temp_C = 35.657 },
    { .rt = 6100, .temp_C = 36.056 },
    { .rt = 6000, .temp_C = 36.464 },
    { .rt = 5900, .temp_C = 36.879 },
    { .rt = 5800, .temp_C = 37.302 },
    { .rt = 5700, .temp_C = 37.733 },
    { .rt = 5600, .temp_C = 38.173 },
    { .rt = 5500, .temp_C = 38.622 },
    { .rt = 5400, .temp_C = 39.08 },
    { .rt = 5300, .temp_C = 39.548 },
    { .rt = 5200, .temp_C = 40.026 },
    { .rt = 5175, .temp_C = 40.147 },
    { .rt = 5150, .temp_C = 40.269 },
    { .rt = 5125, .temp_C = 40.391 },
    { .rt = 5100, .temp_C = 40.514 },
    { .rt = 5075, .temp_C = 40.638 },
    { .rt = 5050, .temp_C = 40.763 },
    { .rt = 5025, .temp_C = 40.888 },
    { .rt = 5000, .temp_C = 41.014 },
    { .rt = 4975, .temp_C = 41.14 },
    { .rt = 4950, .temp_C = 41.268 },
    { .rt = 4925, .temp_C = 41.396 },
    { .rt = 4900, .temp_C = 41.525 },
    { .rt = 4875, .temp_C = 41.654 },
    { .rt = 4850, .temp_C = 41.785 },
    { .rt = 4825, .temp_C = 41.916 },
    { .rt = 4800, .temp_C = 42.048 },
    { .rt = 4775, .temp_C = 42.18 },
    { .rt = 4760, .temp_C = 42.26 },
    { .rt = 4745, .temp_C = 42.34 },
    { .rt = 4730, .temp_C = 42.421 },
    { .rt = 4715, .temp_C = 42.502 },
    { .rt = 4700, .temp_C = 42.583 },
    { .rt = 4685, .temp_C = 42.664 },
    { .rt = 4670, .temp_C = 42.746 },
    { .rt = 4655, .temp_C = 42.828 },
    { .rt = 4640, .temp_C = 42.91 },
    { .rt = 4625, .temp_C = 42.993 },
    { .rt = 4610, .temp_C = 43.076 },
    { .rt = 4595, .temp_C = 43.159 },
    { .rt = 4580, .temp_C = 43.243 },
    { .rt = 4565, .temp_C = 43.327 },
    { .rt = 4550, .temp_C = 43.411 },
    { .rt = 4535, .temp_C = 43.495 },
    { .rt = 4520, .temp_C = 43.58 },
    { .rt = 4505, .temp_C = 43.665 },
    { .rt = 4490, .temp_C = 43.75 },
    { .rt = 4475, .temp_C = 43.836 },
    { .rt = 4460, .temp_C = 43.922 },
    { .rt = 4445, .temp_C = 44.009 },
    { .rt = 4430, .temp_C = 44.095 },
    { .rt = 4415, .temp_C = 44.183 },
    { .rt = 4400, .temp_C = 44.27 },
    { .rt = 4385, .temp_C = 44.358 },
    { .rt = 4370, .temp_C = 44.446 },
    { .rt = 4355, .temp_C = 44.534 },
    { .rt = 4340, .temp_C = 44.623 },
    { .rt = 4325, .temp_C = 44.712 },
    { .rt = 4310, .temp_C = 44.802 },
    { .rt = 4295, .temp_C = 44.891 },
    { .rt = 4280, .temp_C = 44.982 },
    { .rt = 4265, .temp_C = 45.072 },
    { .rt = 4250, .temp_C = 45.163 },
    { .rt = 4235, .temp_C = 45.254 },
    { .rt = 4220, .temp_C = 45.346 },
    { .rt = 4205, .temp_C = 45.438 },
    { .rt = 4190, .temp_C = 45.53 },
    { .rt = 4175, .temp_C = 45.623 },
    { .rt = 4160, .temp_C = 45.716 },
    { .rt = 4145, .temp_C = 45.81 },
    { .rt = 4130, .temp_C = 45.904 },
    { .rt = 4115, .temp_C = 45.998 },
    { .rt = 4100, .temp_C = 46.093 },
    { .rt = 4085, .temp_C = 46.188 },
    { .rt = 4070, .temp_C = 46.283 },
    { .rt = 4055, .temp_C = 46.379 },
    { .rt = 4040, .temp_C = 46.475 },
    { .rt = 4025, .temp_C = 46.572 },
    { .rt = 4010, .temp_C = 46.669 },
    { .rt = 3995, .temp_C = 46.767 },
    { .rt = 3980, .temp_C = 46.865 },
    { .rt = 3965, .temp_C = 46.963 },
    { .rt = 3950, .temp_C = 47.062 },
    { .rt = 3935, .temp_C = 47.161 },
    { .rt = 3920, .temp_C = 47.26 },
    { .rt = 3905, .temp_C = 47.36 },
    { .rt = 3890, .temp_C = 47.461 },
    { .rt = 3875, .temp_C = 47.562 },
    { .rt = 3860, .temp_C = 47.663 },
    { .rt = 3845, .temp_C = 47.765 },
    { .rt = 3830, .temp_C = 47.867 },
    { .rt = 3815, .temp_C = 47.97 },
    { .rt = 3800, .temp_C = 48.073 },
    { .rt = 3785, .temp_C = 48.177 },
    { .rt = 3770, .temp_C = 48.281 },
    { .rt = 3755, .temp_C = 48.386 },
    { .rt = 3740, .temp_C = 48.491 },
    { .rt = 3725, .temp_C = 48.596 },
    { .rt = 3710, .temp_C = 48.702 },
    { .rt = 3695, .temp_C = 48.809 },
    { .rt = 3680, .temp_C = 48.916 },
    { .rt = 3665, .temp_C = 49.023 },
    { .rt = 3650, .temp_C = 49.131 },
    { .rt = 3635, .temp_C = 49.24 },
    { .rt = 3620, .temp_C = 49.349 },
    { .rt = 3605, .temp_C = 49.458 },
    { .rt = 3590, .temp_C = 49.568 },
    { .rt = 3575, .temp_C = 49.679 },
    { .rt = 3560, .temp_C = 49.79 },
    { .rt = 3545, .temp_C = 49.902 },
    { .rt = 3530, .temp_C = 50.014 },
    { .rt = 3515, .temp_C = 50.127 },
    { .rt = 3500, .temp_C = 50.24 },
    { .rt = 3485, .temp_C = 50.354 },
    { .rt = 3470, .temp_C = 50.468 },
    { .rt = 3455, .temp_C = 50.583 },
    { .rt = 3440, .temp_C = 50.699 },
    { .rt = 3425, .temp_C = 50.815 },
    { .rt = 3410, .temp_C = 50.931 },
    { .rt = 3395, .temp_C = 51.049 },
    { .rt = 3380, .temp_C = 51.166 },
    { .rt = 3365, .temp_C = 51.285 },
    { .rt = 3350, .temp_C = 51.404 },
    { .rt = 3340, .temp_C = 51.484 },
    { .rt = 3330, .temp_C = 51.564 },
    { .rt = 3320, .temp_C = 51.644 },
    { .rt = 3310, .temp_C = 51.724 },
    { .rt = 3300, .temp_C = 51.805 },
    { .rt = 3290, .temp_C = 51.886 },
    { .rt = 3280, .temp_C = 51.968 },
    { .rt = 3270, .temp_C = 52.049 },
    { .rt = 3260, .temp_C = 52.131 },
    { .rt = 3250, .temp_C = 52.213 },
    { .rt = 3240, .temp_C = 52.296 },
    { .rt = 3230, .temp_C = 52.379 },
    { .rt = 3220, .temp_C = 52.462 },
    { .rt = 3210, .temp_C = 52.545 },
    { .rt = 3200, .temp_C = 52.629 },
    { .rt = 3190, .temp_C = 52.713 },
    { .rt = 3180, .temp_C = 52.797 },
    { .rt = 3170, .temp_C = 52.881 },
    { .rt = 3160, .temp_C = 52.966 },
    { .rt = 3150, .temp_C = 53.052 },
    { .rt = 3140, .temp_C = 53.137 },
    { .rt = 3130, .temp_C = 53.223 },
    { .rt = 3120, .temp_C = 53.309 },
    { .rt = 3110, .temp_C = 53.395 },
    { .rt = 3100, .temp_C = 53.482 },
    { .rt = 3090, .temp_C = 53.569 },
    { .rt = 3080, .temp_C = 53.656 },
    { .rt = 3070, .temp_C = 53.744 },
    { .rt = 3060, .temp_C = 53.832 },
    { .rt = 3050, .temp_C = 53.921 },
    { .rt = 3040, .temp_C = 54.009 },
    { .rt = 3030, .temp_C = 54.098 },
    { .rt = 3020, .temp_C = 54.188 },
    { .rt = 3010, .temp_C = 54.277 },
    { .rt = 3000, .temp_C = 54.367 },
    { .rt = 2990, .temp_C = 54.458 },
    { .rt = 2980, .temp_C = 54.548 },
    { .rt = 2970, .temp_C = 54.639 },
    { .rt = 2960, .temp_C = 54.731 },
    { .rt = 2950, .temp_C = 54.823 },
    { .rt = 2940, .temp_C = 54.915 },
    { .rt = 2930, .temp_C = 55.007 },
    { .rt = 2920, .temp_C = 55.1 },
    { .rt = 2910, .temp_C = 55.193 },
    { .rt = 2900, .temp_C = 55.287 },
    { .rt = 2890, .temp_C = 55.381 },
    { .rt = 2880, .temp_C = 55.475 },
    { .rt = 2870, .temp_C = 55.57 },
    { .rt = 2860, .temp_C = 55.665 },
    { .rt = 2850, .temp_C = 55.76 },
    { .rt = 2840, .temp_C = 55.856 },
    { .rt = 2830, .temp_C = 55.952 },
    { .rt = 2820, .temp_C = 56.049 },
    { .rt = 2810, .temp_C = 56.146 },
    { .rt = 2800, .temp_C = 56.243 },
    { .rt = 2790, .temp_C = 56.341 },
    { .rt = 2780, .temp_C = 56.439 },
    { .rt = 2770, .temp_C = 56.538 },
    { .rt = 2760, .temp_C = 56.637 },
    { .rt = 2750, .temp_C = 56.736 },
    { .rt = 2740, .temp_C = 56.836 },
    { .rt = 2730, .temp_C = 56.936 },
    { .rt = 2720, .temp_C = 57.037 },
    { .rt = 2710, .temp_C = 57.138 },
    { .rt = 2700, .temp_C = 57.239 },
    { .rt = 2690, .temp_C = 57.341 },
    { .rt = 2680, .temp_C = 57.443 },
    { .rt = 2670, .temp_C = 57.546 },
    { .rt = 2660, .temp_C = 57.649 },
    { .rt = 2650, .temp_C = 57.753 },
    { .rt = 2640, .temp_C = 57.857 },
    { .rt = 2630, .temp_C = 57.962 },
    { .rt = 2620, .temp_C = 58.067 },
    { .rt = 2610, .temp_C = 58.172 },
    { .rt = 2601, .temp_C = 58.267 },
    { .rt = 2593.5, .temp_C = 58.347 },
    { .rt = 2586, .temp_C = 58.427 },
    { .rt = 2578.5, .temp_C = 58.507 },
    { .rt = 2571, .temp_C = 58.588 },
    { .rt = 2563.5, .temp_C = 58.669 },
    { .rt = 2556, .temp_C = 58.75 },
    { .rt = 2548.5, .temp_C = 58.831 },
    { .rt = 2541, .temp_C = 58.913 },
    { .rt = 2533.5, .temp_C = 58.994 },
    { .rt = 2526, .temp_C = 59.077 },
    { .rt = 2518.5, .temp_C = 59.159 },
    { .rt = 2511, .temp_C = 59.242 },
    { .rt = 2503.5, .temp_C = 59.325 },
    { .rt = 2496, .temp_C = 59.408 },
    { .rt = 2488.5, .temp_C = 59.492 },
    { .rt = 2481, .temp_C = 59.575 },
    { .rt = 2473.5, .temp_C = 59.659 },
    { .rt = 2466, .temp_C = 59.744 },
    { .rt = 2458.5, .temp_C = 59.829 },
    { .rt = 2451, .temp_C = 59.914 },
    { .rt = 2443.5, .temp_C = 59.999 },
    { .rt = 2436, .temp_C = 60.085 },
    { .rt = 2428.5, .temp_C = 60.17 },
    { .rt = 2421, .temp_C = 60.257 },
    { .rt = 2413.5, .temp_C = 60.343 },
    { .rt = 2406, .temp_C = 60.43 },
    { .rt = 2398.5, .temp_C = 60.517 },
    { .rt = 2391, .temp_C = 60.605 },
    { .rt = 2383.5, .temp_C = 60.692 },
    { .rt = 2376, .temp_C = 60.78 },
    { .rt = 2368.5, .temp_C = 60.869 },
    { .rt = 2361, .temp_C = 60.958 },
    { .rt = 2353.5, .temp_C = 61.047 },
    { .rt = 2346, .temp_C = 61.136 },
    { .rt = 2338.5, .temp_C = 61.226 },
    { .rt = 2331, .temp_C = 61.316 },
    { .rt = 2323.5, .temp_C = 61.406 },
    { .rt = 2316, .temp_C = 61.497 },
    { .rt = 2308.5, .temp_C = 61.588 },
    { .rt = 2301, .temp_C = 61.679 },
    { .rt = 2293.5, .temp_C = 61.771 },
    { .rt = 2286, .temp_C = 61.863 },
    { .rt = 2278.5, .temp_C = 61.955 },
    { .rt = 2271, .temp_C = 62.048 },
    { .rt = 2263.5, .temp_C = 62.141 },
    { .rt = 2256, .temp_C = 62.234 },
    { .rt = 2248.5, .temp_C = 62.328 },
    { .rt = 2241, .temp_C = 62.422 },
    { .rt = 2233.5, .temp_C = 62.517 },
    { .rt = 2226, .temp_C = 62.612 },
    { .rt = 2218.5, .temp_C = 62.707 },
    { .rt = 2211, .temp_C = 62.802 },
    { .rt = 2203.5, .temp_C = 62.898 },
    { .rt = 2196, .temp_C = 62.995 },
    { .rt = 2188.5, .temp_C = 63.091 },
    { .rt = 2181, .temp_C = 63.189 },
    { .rt = 2173.5, .temp_C = 63.286 },
    { .rt = 2166, .temp_C = 63.384 },
    { .rt = 2158.5, .temp_C = 63.482 },
    { .rt = 2151, .temp_C = 63.581 },
    { .rt = 2143.5, .temp_C = 63.68 },
    { .rt = 2136, .temp_C = 63.779 },
    { .rt = 2128.5, .temp_C = 63.879 },
    { .rt = 2121, .temp_C = 63.979 },
    { .rt = 2113.5, .temp_C = 64.08 },
    { .rt = 2106, .temp_C = 64.181 },
    { .rt = 2098.5, .temp_C = 64.282 },
    { .rt = 2091, .temp_C = 64.384 },
    { .rt = 2083.5, .temp_C = 64.487 },
    { .rt = 2076, .temp_C = 64.589 },
    { .rt = 2068.5, .temp_C = 64.693 },
    { .rt = 2061, .temp_C = 64.796 },
    { .rt = 2053.5, .temp_C = 64.9 },
    { .rt = 2046, .temp_C = 65.005 },
    { .rt = 2038.5, .temp_C = 65.109 },
    { .rt = 2031, .temp_C = 65.215 },
    { .rt = 2023.5, .temp_C = 65.32 },
    { .rt = 2016, .temp_C = 65.427 },
    { .rt = 2008.5, .temp_C = 65.533 },
    { .rt = 2001, .temp_C = 65.64 },
    { .rt = 1993.5, .temp_C = 65.748 },
    { .rt = 1986, .temp_C = 65.856 },
    { .rt = 1978.5, .temp_C = 65.965 },
    { .rt = 1971, .temp_C = 66.074 },
    { .rt = 1963.5, .temp_C = 66.183 },
    { .rt = 1956, .temp_C = 66.293 },
    { .rt = 1948.5, .temp_C = 66.403 },
    { .rt = 1941, .temp_C = 66.514 },
    { .rt = 1933.5, .temp_C = 66.626 },
    { .rt = 1926, .temp_C = 66.738 },
    { .rt = 1918.5, .temp_C = 66.85 },
    { .rt = 1911, .temp_C = 66.963 },
    { .rt = 1903.5, .temp_C = 67.076 },
    { .rt = 1896, .temp_C = 67.19 },
    { .rt = 1888.5, .temp_C = 67.305 },
    { .rt = 1881, .temp_C = 67.42 },
    { .rt = 1873.5, .temp_C = 67.535 },
    { .rt = 1866, .temp_C = 67.651 },
    { .rt = 1858.5, .temp_C = 67.768 },
    { .rt = 1851, .temp_C = 67.885 },
    { .rt = 1843.5, .temp_C = 68.002 },
    { .rt = 1836, .temp_C = 68.12 },
    { .rt = 1828.5, .temp_C = 68.239 },
    { .rt = 1821, .temp_C = 68.358 },
    { .rt = 1816, .temp_C = 68.438 },
    { .rt = 1811, .temp_C = 68.518 },
    { .rt = 1806, .temp_C = 68.599 },
    { .rt = 1801, .temp_C = 68.679 },
    { .rt = 1796, .temp_C = 68.76 },
    { .rt = 1791, .temp_C = 68.841 },
    { .rt = 1786, .temp_C = 68.923 },
    { .rt = 1781, .temp_C = 69.004 },
    { .rt = 1776, .temp_C = 69.086 },
    { .rt = 1771, .temp_C = 69.168 },
    { .rt = 1766, .temp_C = 69.251 },
    { .rt = 1761, .temp_C = 69.333 },
    { .rt = 1756, .temp_C = 69.416 },
    { .rt = 1751, .temp_C = 69.5 },
    { .rt = 1746, .temp_C = 69.583 },
    { .rt = 1741, .temp_C = 69.667 },
    { .rt = 1736, .temp_C = 69.751 },
    { .rt = 1731, .temp_C = 69.835 },
    { .rt = 1726, .temp_C = 69.92 },
    { .rt = 1721, .temp_C = 70.005 },
    { .rt = 1716, .temp_C = 70.09 },
    { .rt = 1711, .temp_C = 70.175 },
    { .rt = 1706, .temp_C = 70.261 },
    { .rt = 1701, .temp_C = 70.347 },
    { .rt = 1696, .temp_C = 70.433 },
    { .rt = 1691, .temp_C = 70.52 },
    { .rt = 1686, .temp_C = 70.607 },
    { .rt = 1681, .temp_C = 70.694 },
    { .rt = 1676, .temp_C = 70.782 },
    { .rt = 1671, .temp_C = 70.87 },
    { .rt = 1666, .temp_C = 70.958 },
    { .rt = 1661, .temp_C = 71.046 },
    { .rt = 1656, .temp_C = 71.135 },
    { .rt = 1651, .temp_C = 71.224 },
    { .rt = 1646, .temp_C = 71.313 },
    { .rt = 1641, .temp_C = 71.403 },
    { .rt = 1636, .temp_C = 71.493 },
    { .rt = 1631, .temp_C = 71.583 },
    { .rt = 1626, .temp_C = 71.674 },
    { .rt = 1621, .temp_C = 71.764 },
    { .rt = 1616, .temp_C = 71.856 },
    { .rt = 1611, .temp_C = 71.947 },
    { .rt = 1606, .temp_C = 72.039 },
    { .rt = 1601, .temp_C = 72.131 },
    { .rt = 1596, .temp_C = 72.224 },
    { .rt = 1591, .temp_C = 72.317 },
    { .rt = 1586, .temp_C = 72.41 },
    { .rt = 1581, .temp_C = 72.503 },
    { .rt = 1576, .temp_C = 72.597 },
    { .rt = 1571, .temp_C = 72.692 },
    { .rt = 1566, .temp_C = 72.786 },
    { .rt = 1561, .temp_C = 72.881 },
    { .rt = 1556, .temp_C = 72.976 },
    { .rt = 1551, .temp_C = 73.072 },
    { .rt = 1546, .temp_C = 73.168 },
    { .rt = 1541, .temp_C = 73.264 },
    { .rt = 1536, .temp_C = 73.361 },
    { .rt = 1531, .temp_C = 73.458 },
    { .rt = 1526, .temp_C = 73.555 },
    { .rt = 1521, .temp_C = 73.653 },
    { .rt = 1516, .temp_C = 73.751 },
    { .rt = 1511, .temp_C = 73.85 },
    { .rt = 1506, .temp_C = 73.949 },
    { .rt = 1501, .temp_C = 74.048 },
    { .rt = 1496, .temp_C = 74.147 },
    { .rt = 1491, .temp_C = 74.248 },
    { .rt = 1486, .temp_C = 74.348 },
    { .rt = 1481, .temp_C = 74.449 },
    { .rt = 1476, .temp_C = 74.55 },
    { .rt = 1471, .temp_C = 74.652 },
    { .rt = 1466, .temp_C = 74.753 },
    { .rt = 1461, .temp_C = 74.856 },
    { .rt = 1456, .temp_C = 74.959 },
    { .rt = 1451, .temp_C = 75.062 },
    { .rt = 1446, .temp_C = 75.165 },
    { .rt = 1441, .temp_C = 75.269 },
    { .rt = 1436, .temp_C = 75.374 },
    { .rt = 1431, .temp_C = 75.479 },
    { .rt = 1426, .temp_C = 75.584 },
    { .rt = 1421, .temp_C = 75.69 },
    { .rt = 1416, .temp_C = 75.796 },
    { .rt = 1411, .temp_C = 75.902 },
    { .rt = 1406, .temp_C = 76.009 },
    { .rt = 1401, .temp_C = 76.117 },
    { .rt = 1396, .temp_C = 76.225 },
    { .rt = 1391, .temp_C = 76.333 },
    { .rt = 1386, .temp_C = 76.442 },
    { .rt = 1381, .temp_C = 76.551 },
    { .rt = 1376, .temp_C = 76.661 },
    { .rt = 1371, .temp_C = 76.771 },
    { .rt = 1366, .temp_C = 76.881 },
    { .rt = 1361, .temp_C = 76.992 },
    { .rt = 1356, .temp_C = 77.104 },
    { .rt = 1351, .temp_C = 77.216 },
    { .rt = 1346, .temp_C = 77.328 },
    { .rt = 1341, .temp_C = 77.441 },
    { .rt = 1336, .temp_C = 77.555 },
    { .rt = 1332.5, .temp_C = 77.634 },
    { .rt = 1329, .temp_C = 77.714 },
    { .rt = 1325.5, .temp_C = 77.794 },
    { .rt = 1322, .temp_C = 77.875 },
    { .rt = 1318.5, .temp_C = 77.955 },
    { .rt = 1315, .temp_C = 78.036 },
    { .rt = 1311.5, .temp_C = 78.118 },
    { .rt = 1308, .temp_C = 78.199 },
    { .rt = 1304.5, .temp_C = 78.281 },
    { .rt = 1301, .temp_C = 78.362 },
    { .rt = 1297.5, .temp_C = 78.445 },
    { .rt = 1294, .temp_C = 78.527 },
    { .rt = 1290.5, .temp_C = 78.61 },
    { .rt = 1287, .temp_C = 78.693 },
    { .rt = 1283.5, .temp_C = 78.776 },
    { .rt = 1280, .temp_C = 78.859 },
    { .rt = 1276.5, .temp_C = 78.943 },
    { .rt = 1273, .temp_C = 79.027 },
    { .rt = 1269.5, .temp_C = 79.111 },
    { .rt = 1266, .temp_C = 79.196 },
    { .rt = 1262.5, .temp_C = 79.28 },
    { .rt = 1259, .temp_C = 79.366 },
    { .rt = 1255.5, .temp_C = 79.451 },
    { .rt = 1252, .temp_C = 79.536 },
    { .rt = 1248.5, .temp_C = 79.622 },
    { .rt = 1245, .temp_C = 79.708 },
    { .rt = 1241.5, .temp_C = 79.795 },
    { .rt = 1238, .temp_C = 79.882 },
    { .rt = 1234.5, .temp_C = 79.969 },
    { .rt = 1231, .temp_C = 80.056 },
    { .rt = 1227.5, .temp_C = 80.143 },
    { .rt = 1224, .temp_C = 80.231 },
    { .rt = 1220.5, .temp_C = 80.319 },
    { .rt = 1217, .temp_C = 80.408 },
    { .rt = 1213.5, .temp_C = 80.497 },
    { .rt = 1210, .temp_C = 80.586 },
    { .rt = 1206.5, .temp_C = 80.675 },
    { .rt = 1203, .temp_C = 80.764 },
    { .rt = 1199.5, .temp_C = 80.854 },
    { .rt = 1196, .temp_C = 80.945 },
    { .rt = 1192.5, .temp_C = 81.035 },
    { .rt = 1189, .temp_C = 81.126 },
    { .rt = 1185.5, .temp_C = 81.217 },
    { .rt = 1182, .temp_C = 81.308 },
    { .rt = 1178.5, .temp_C = 81.4 },
    { .rt = 1175, .temp_C = 81.492 },
    { .rt = 1171.5, .temp_C = 81.585 },
    { .rt = 1168, .temp_C = 81.677 },
    { .rt = 1164.5, .temp_C = 81.77 },
    { .rt = 1161, .temp_C = 81.864 },
    { .rt = 1157.5, .temp_C = 81.957 },
    { .rt = 1154, .temp_C = 82.051 },
    { .rt = 1150.5, .temp_C = 82.146 },
    { .rt = 1147, .temp_C = 82.24 },
    { .rt = 1143.5, .temp_C = 82.335 },
    { .rt = 1140, .temp_C = 82.431 },
    { .rt = 1136.5, .temp_C = 82.526 },
    { .rt = 1133, .temp_C = 82.622 },
    { .rt = 1129.5, .temp_C = 82.719 },
    { .rt = 1126, .temp_C = 82.815 },
    { .rt = 1122.5, .temp_C = 82.912 },
    { .rt = 1119, .temp_C = 83.01 },
    { .rt = 1115.5, .temp_C = 83.107 },
    { .rt = 1112, .temp_C = 83.205 },
    { .rt = 1108.5, .temp_C = 83.304 },
    { .rt = 1105, .temp_C = 83.403 },
    { .rt = 1101.5, .temp_C = 83.502 },
    { .rt = 1098, .temp_C = 83.601 },
    { .rt = 1094.5, .temp_C = 83.701 },
    { .rt = 1091, .temp_C = 83.801 },
    { .rt = 1087.5, .temp_C = 83.902 },
    { .rt = 1084, .temp_C = 84.003 },
    { .rt = 1080.5, .temp_C = 84.104 },
    { .rt = 1077, .temp_C = 84.206 },
    { .rt = 1073.5, .temp_C = 84.308 },
    { .rt = 1070, .temp_C = 84.411 },
    { .rt = 1066.5, .temp_C = 84.514 },
    { .rt = 1063, .temp_C = 84.617 },
    { .rt = 1059.5, .temp_C = 84.721 },
    { .rt = 1056, .temp_C = 84.825 },
    { .rt = 1052.5, .temp_C = 84.929 },
    { .rt = 1049, .temp_C = 85.034 },
    { .rt = 1045.5, .temp_C = 85.139 },
    { .rt = 1042, .temp_C = 85.245 },
    { .rt = 1038.5, .temp_C = 85.351 },
    { .rt = 1035, .temp_C = 85.458 },
    { .rt = 1031.5, .temp_C = 85.565 },
    { .rt = 1028, .temp_C = 85.672 },
    { .rt = 1024.5, .temp_C = 85.78 },
    { .rt = 1021, .temp_C = 85.888 },
    { .rt = 1017.5, .temp_C = 85.996 },
    { .rt = 1014, .temp_C = 86.106 },
    { .rt = 1010.5, .temp_C = 86.215 },
    { .rt = 1007, .temp_C = 86.325 },
    { .rt = 1003.5, .temp_C = 86.435 },
    { .rt = 1000, .temp_C = 86.546 },
    { .rt = 997.5, .temp_C = 86.626 },
    { .rt = 995, .temp_C = 86.705 },
    { .rt = 992.5, .temp_C = 86.785 },
    { .rt = 990, .temp_C = 86.865 },
    { .rt = 987.5, .temp_C = 86.945 },
    { .rt = 985, .temp_C = 87.026 },
    { .rt = 982.5, .temp_C = 87.107 },
    { .rt = 980, .temp_C = 87.188 },
    { .rt = 977.5, .temp_C = 87.269 },
    { .rt = 975, .temp_C = 87.351 },
    { .rt = 972.5, .temp_C = 87.433 },
    { .rt = 970, .temp_C = 87.515 },
    { .rt = 967.5, .temp_C = 87.597 },
    { .rt = 965, .temp_C = 87.679 },
    { .rt = 962.5, .temp_C = 87.762 },
    { .rt = 960, .temp_C = 87.845 },
    { .rt = 957.5, .temp_C = 87.928 },
    { .rt = 955, .temp_C = 88.012 },
    { .rt = 952.5, .temp_C = 88.096 },
    { .rt = 950, .temp_C = 88.18 },
    { .rt = 947.5, .temp_C = 88.264 },
    { .rt = 945, .temp_C = 88.348 },
    { .rt = 942.5, .temp_C = 88.433 },
    { .rt = 940, .temp_C = 88.518 },
    { .rt = 937.5, .temp_C = 88.603 },
    { .rt = 935, .temp_C = 88.689 },
    { .rt = 932.5, .temp_C = 88.775 },
    { .rt = 930, .temp_C = 88.861 },
    { .rt = 927.5, .temp_C = 88.947 },
    { .rt = 925, .temp_C = 89.034 },
    { .rt = 922.5, .temp_C = 89.121 },
    { .rt = 920, .temp_C = 89.208 },
    { .rt = 917.5, .temp_C = 89.295 },
    { .rt = 915, .temp_C = 89.383 },
    { .rt = 912.5, .temp_C = 89.471 },
    { .rt = 910, .temp_C = 89.559 },
    { .rt = 907.5, .temp_C = 89.648 },
    { .rt = 905, .temp_C = 89.737 },
    { .rt = 902.5, .temp_C = 89.826 },
    { .rt = 900, .temp_C = 89.915 },
    { .rt = 897.5, .temp_C = 90.005 },
    { .rt = 895, .temp_C = 90.095 },
    { .rt = 892.5, .temp_C = 90.185 },
    { .rt = 890, .temp_C = 90.276 },
    { .rt = 887.5, .temp_C = 90.367 },
    { .rt = 885, .temp_C = 90.458 },
    { .rt = 882.5, .temp_C = 90.549 },
    { .rt = 880, .temp_C = 90.641 },
    { .rt = 877.5, .temp_C = 90.733 },
    { .rt = 875, .temp_C = 90.825 },
    { .rt = 872.5, .temp_C = 90.918 },
    { .rt = 870, .temp_C = 91.011 },
    { .rt = 867.5, .temp_C = 91.104 },
    { .rt = 865, .temp_C = 91.198 },
    { .rt = 862.5, .temp_C = 91.292 },
    { .rt = 860, .temp_C = 91.386 },
    { .rt = 857.5, .temp_C = 91.481 },
    { .rt = 855, .temp_C = 91.575 },
    { .rt = 852.5, .temp_C = 91.671 },
    { .rt = 850, .temp_C = 91.766 },
    { .rt = 847.5, .temp_C = 91.862 },
    { .rt = 845, .temp_C = 91.958 },
    { .rt = 842.5, .temp_C = 92.054 },
    { .rt = 840, .temp_C = 92.151 },
    { .rt = 837.5, .temp_C = 92.248 },
    { .rt = 835, .temp_C = 92.346 },
    { .rt = 832.5, .temp_C = 92.444 },
    { .rt = 830, .temp_C = 92.542 },
    { .rt = 827.5, .temp_C = 92.64 },
    { .rt = 825, .temp_C = 92.739 },
    { .rt = 822.5, .temp_C = 92.838 },
    { .rt = 820, .temp_C = 92.938 },
    { .rt = 817.5, .temp_C = 93.038 },
    { .rt = 815, .temp_C = 93.138 },
    { .rt = 812.5, .temp_C = 93.239 },
    { .rt = 810, .temp_C = 93.34 },
    { .rt = 807.5, .temp_C = 93.441 },
    { .rt = 805, .temp_C = 93.543 },
    { .rt = 802.5, .temp_C = 93.645 },
    { .rt = 800, .temp_C = 93.747 },
    { .rt = 797.5, .temp_C = 93.85 },
    { .rt = 795, .temp_C = 93.953 },
    { .rt = 792.5, .temp_C = 94.056 },
    { .rt = 790, .temp_C = 94.16 },
    { .rt = 787.5, .temp_C = 94.265 },
    { .rt = 785, .temp_C = 94.369 },
    { .rt = 782.5, .temp_C = 94.474 },
    { .rt = 780, .temp_C = 94.58 },
    { .rt = 777.5, .temp_C = 94.686 },
    { .rt = 775, .temp_C = 94.792 },
    { .rt = 772.5, .temp_C = 94.898 },
    { .rt = 770, .temp_C = 95.005 },
    { .rt = 767.5, .temp_C = 95.113 },
    { .rt = 765, .temp_C = 95.221 },
    { .rt = 762.5, .temp_C = 95.329 },
    { .rt = 760, .temp_C = 95.438 },
    { .rt = 757.5, .temp_C = 95.547 },
    { .rt = 755, .temp_C = 95.656 },
    { .rt = 752.5, .temp_C = 95.766 },
    { .rt = 750, .temp_C = 95.876 },
    { .rt = 747.5, .temp_C = 95.987 },
    { .rt = 745, .temp_C = 96.098 },
    { .rt = 730, .temp_C = 96.774 },
    { .rt = 715, .temp_C = 97.467 },
    { .rt = 700, .temp_C = 98.177 },
    { .rt = 690, .temp_C = 98.659 },
    { .rt = 680, .temp_C = 99.15 },
    { .rt = 670, .temp_C = 99.65 },
    { .rt = 660, .temp_C = 100.158 },
    { .rt = 650, .temp_C = 100.675 },
    { .rt = 640, .temp_C = 101.202 },
    { .rt = 630, .temp_C = 101.738 },
    { .rt = 620, .temp_C = 102.284 },
    { .rt = 610, .temp_C = 102.84 },
    { .rt = 600, .temp_C = 103.407 },
    { .rt = 590, .temp_C = 103.985 },
    { .rt = 580, .temp_C = 104.575 },
    { .rt = 570, .temp_C = 105.176 },
    { .rt = 560, .temp_C = 105.79 },
    { .rt = 550, .temp_C = 106.417 },
    { .rt =    0,                             },    //last element.
};

/*
200 145.6835835
220 141.7288761
240 138.1784483
260 134.961637
280 132.024471
300 129.3248461
320 126.8293168
340 124.5108969
360 122.3475139
380 120.3208957
400 118.415755
420 116.6191791
440 114.9201675
460 113.3092763
480 111.7783406
500 110.3202561
520 108.928805
540 107.5985162
560 106.3245513
580 105.102612
600 103.9288632
620 102.7998698
640 101.7125435
660 100.664098
680 99.65201127
700 98.67399388
720 97.72796119
740 96.81201009
760 95.92439874
780 95.06352902
800 94.22793132
820 93.41625134
840 92.62723837
860 91.8597352
880 91.11266909
900 90.38504383
920 89.67593273
940 88.98447236
960 88.30985702
980 87.6513337
1000    87.00819769
1020    86.37978857
1040    85.76548662
1060    85.16470955
1080    84.57690962
1100    84.00157097
1120    83.43820723
1140    82.88635934
1160    82.34559354
1180    81.81549963
1200    81.29568922
1220    80.78579433
1240    80.28546591
1260    79.79437265
1280    79.31219976
1300    78.83864793
1320    78.37343235
1340    77.91628176
1360    77.46693766
1380    77.02515351
1400    76.59069399
1420    76.16333438
1440    75.74285991
1460    75.32906521
1480    74.92175375
1500    74.52073737
1520    74.12583578
1540    73.73687618
1560    73.35369283
1580    72.97612669
1600    72.60402502
1620    72.23724112
1640    71.87563398
1660    71.51906801
1680    71.16741276
1700    70.82054267
1720    70.47833681
1740    70.14067872
1760    69.8074561
1780    69.47856072
1800    69.15388814
1820    68.83333759
1840    68.51681179
1860    68.20421677
1880    67.89546177
1900    67.59045904
1920    67.28912376
1940    66.99137389
1960    66.69713003
1980    66.40631536
2000    66.11885549
2020    65.83467838
2040    65.55371421
2060    65.27589536
2080    65.00115623
2100    64.72943324
2120    64.46066469
2140    64.19479075
2160    63.93175331
2180    63.67149599
2200    63.413964
2220    63.15910415
2240    62.90686474
2260    62.65719555
2280    62.41004772
2300    62.16537377
2320    61.92312751
2340    61.68326401
2360    61.44573954
2380    61.21051153
2400    60.97753856
2420    60.74678028
2440    60.5181974
2460    60.29175162
2480    60.06740566
2500    59.84512315
2520    59.62486866
2540    59.40660763
2560    59.19030637
2580    58.97593201
2600    58.76345248
2620    58.55283649
2640    58.34405352
2660    58.13707374
2680    57.93186807
2700    57.72840807
2720    57.526666
2740    57.32661473
2760    57.12822779
2780    56.93147928
2800    56.73634391
2820    56.54279694
2840    56.3508142
2860    56.16037204
2880    55.97144735
2900    55.7840175
2920    55.59806038
2940    55.41355433
2960    55.23047818
2980    55.04881118
3000    54.86853305
3020    54.68962391
3040    54.51206431
3060    54.33583519
3080    54.1609179
3100    53.98729416
3120    53.81494607
3140    53.64385606
3160    53.47400696
3180    53.30538192
3200    53.1379644
3220    52.97173822
3240    52.80668751
3260    52.6427967
3280    52.48005051
3300    52.31843399
3320    52.15793243
3340    51.99853143
3360    51.84021685
3380    51.68297482
3400    51.52679172
3420    51.3716542
3440    51.21754913
3460    51.06446364
3480    50.91238509
3500    50.76130106
3520    50.61119937
3540    50.46206803
3560    50.3138953
3580    50.16666963
3600    50.02037965
3620    49.87501423
3640    49.73056241
3660    49.58701342
3680    49.44435669
3700    49.3025818
3720    49.16167855
3740    49.02163687
3760    48.88244689
3780    48.74409889
3800    48.60658333
3820    48.4698908
3840    48.33401206
3860    48.19893802
3880    48.06465975
3900    47.93116845
3920    47.79845547
3940    47.66651228
3960    47.53533051
3980    47.40490192
4000    47.27521839
4020    47.14627193
4040    47.01805469
4060    46.89055891
4080    46.76377699
4100    46.63770141
4120    46.5123248
4140    46.38763987
4160    46.26363946
4180    46.14031653
4200    46.01766411
4220    45.89567537
4240    45.77434356
4260    45.65366204
4280    45.53362427
4300    45.41422379
4320    45.29545425
4340    45.17730939
4360    45.05978305
4380    44.94286913
4400    44.82656165
4420    44.71085469
4440    44.59574244
4460    44.48121914
4480    44.36727915
4500    44.25391688
4520    44.14112682
4540    44.02890355
4560    43.91724171
4580    43.80613604
4600    43.69558132
4620    43.58557243
4640    43.47610429
4660    43.36717192
4680    43.25877038
4700    43.15089482
4720    43.04354044
4740    42.9367025
4760    42.83037633
4780    42.72455734
4800    42.61924096
4820    42.51442272
4840    42.41009819
4860    42.30626299
4880    42.20291281
4900    42.10004338
4920    41.99765052
4940    41.89573006
4960    41.7942779
4980    41.69329001
5000    41.59276238
5020    41.49269108
5040    41.3930722
5060    41.2939019
5080    41.19517639
5100    41.09689191
5120    40.99904475
5140    40.90163126
5160    40.80464782
5180    40.70809086
5200    40.61195686
5220    40.51624232
5240    40.42094381
5260    40.32605793
5280    40.23158131
5300    40.13751063
5320    40.04384261
5340    39.95057402
5360    39.85770163
5380    39.7652223
5400    39.67313289
5420    39.5814303
5440    39.49011148
5460    39.39917341
5480    39.3086131
5500    39.2184276
5520    39.12861398
5540    39.03916937
5560    38.9500909
5580    38.86137577
5600    38.77302118
5620    38.68502436
5640    38.59738261
5660    38.51009321
5680    38.42315351
5700    38.33656086
5720    38.25031266
5740    38.16440633
5760    38.07883932
5780    37.99360911
5800    37.9087132
5820    37.82414912
5840    37.73991443
5860    37.65600671
5880    37.57242358
5900    37.48916268
5920    37.40622166
5940    37.3235982
5960    37.24129003
5980    37.15929487
6000    37.07761049
6020    36.99623466
6040    36.91516519
6060    36.83439991
6080    36.75393667
6100    36.67377334
6120    36.59390783
6140    36.51433803
6160    36.4350619
6180    36.35607739
6200    36.27738248
6220    36.19897517
6240    36.12085348
6260    36.04301545
6280    35.96545915
6300    35.88818264
6320    35.81118404
6340    35.73446145
6360    35.65801301
6380    35.58183689
6400    35.50593124
6420    35.43029427
6440    35.35492417
6460    35.27981918
6480    35.20497754
6500    35.13039752
6520    35.05607737
6540    34.98201541
6560    34.90820994
6580    34.83465929
6600    34.76136179
6620    34.68831582
6640    34.61551973
6660    34.54297193
6680    34.47067081
6700    34.39861479
6720    34.32680231
6740    34.25523183
6760    34.18390179
6780    34.11281068
6800    34.04195699
6820    33.97133922
6840    33.9009559
6860    33.83080556
6880    33.76088675
6900    33.69119802
6920    33.62173795
6940    33.55250512
6960    33.48349813
6980    33.4147156
7000    33.34615614
7020    33.2778184
7040    33.20970102
7060    33.14180266
7080    33.074122
7100    33.00665772
7120    32.93940851
7140    32.87237309
7160    32.80555017
7180    32.73893848
7200    32.67253676
7220    32.60634377
7240    32.54035826
7260    32.47457902
7280    32.40900483
7300    32.34363447
7320    32.27846676
7340    32.21350051
7360    32.14873455
7380    32.08416772
7400    32.01979885
7420    31.9556268
7440    31.89165045
7460    31.82786866
7480    31.76428032
7500    31.70088432
7520    31.63767956
7540    31.57466496
7560    31.51183944
7580    31.44920193
7600    31.38675136
7620    31.32448669
7640    31.26240686
7660    31.20051086
7680    31.13879764
7700    31.07726619
7720    31.01591551
7740    30.95474458
7760    30.89375242
7780    30.83293805
7800    30.77230047
7820    30.71183873
7840    30.65155186
7860    30.59143891
7880    30.53149893
7900    30.47173098
7920    30.41213413
7940    30.35270746
7960    30.29345005
7980    30.23436099
8000    30.17543938
8020    30.11668433
8040    30.05809493
8060    29.99967032
8080    29.94140963
8100    29.88331197
8120    29.82537649
8140    29.76760234
8160    29.70998867
8180    29.65253464
8200    29.59523941
8220    29.53810216
8240    29.48112205
8260    29.42429829
8280    29.36763006
8300    29.31111655
8320    29.25475696
8340    29.19855052
8360    29.14249642
8380    29.0865939
8400    29.03084218
8420    28.97524049
8440    28.91978806
8460    28.86448415
8480    28.809328
8500    28.75431887
8520    28.69945602
8540    28.64473871
8560    28.59016621
8580    28.5357378
8600    28.48145277
8620    28.4273104
8640    28.37330998
8660    28.31945081
8680    28.2657322
8700    28.21215344
8720    28.15871386
8740    28.10541277
8760    28.05224949
8780    27.99922335
8800    27.94633369
8820    27.89357983
8840    27.84096113
8860    27.78847693
8880    27.73612657
8900    27.68390942
8920    27.63182483
8940    27.57987217
8960    27.52805081
8980    27.47636012
9000    27.42479948
9020    27.37336828
9040    27.32206589
9060    27.27089171
9080    27.21984514
9100    27.16892557
9120    27.11813241
9140    27.06746506
9160    27.01692294
9180    26.96650546
9200    26.91621204
9220    26.86604211
9240    26.81599509
9260    26.76607042
9280    26.71626752
9300    26.66658584
9320    26.61702482
9340    26.56758391
9360    26.51826256
9380    26.46906022
9400    26.41997636
9420    26.37101042
9440    26.32216188
9460    26.2734302
9480    26.22481486
9500    26.17631533
9520    26.12793109
9540    26.07966163
9560    26.03150642
9580    25.98346497
9600    25.93553675
9620    25.88772127
9640    25.84001803
9660    25.79242653
9680    25.74494627
9700    25.69757676
9720    25.65031752
9740    25.60316806
9760    25.55612789
9780    25.50919655
9800    25.46237355
9820    25.41565842
9840    25.3690507
9860    25.32254991
9880    25.2761556
9900    25.22986729
9920    25.18368454
9940    25.13760689
9960    25.09163389
9980    25.04576509
10000   25.00000004
10020   24.9543383
10040   24.90877943
10060   24.86332299
10080   24.81796855
10100   24.77271567
10120   24.72756393
10140   24.68251289
10160   24.63756213
10180   24.59271124
10200   24.54795978
10220   24.50330736
10240   24.45875354
10260   24.41429792
10280   24.3699401
10300   24.32567966
10320   24.2815162
10340   24.23744931
10360   24.19347861
10380   24.14960369
10400   24.10582416
10420   24.06213963
10440   24.01854971
10460   23.975054
10480   23.93165213
10500   23.88834372
10520   23.84512837
10540   23.80200573
10560   23.7589754
10580   23.71603702
10600   23.67319021
10620   23.6304346
10640   23.58776984
10660   23.54519555
10680   23.50271137
10700   23.46031694
10720   23.4180119
10740   23.3757959
10760   23.33366859
10780   23.2916296
10800   23.24967859
10820   23.20781521
10840   23.16603912
10860   23.12434998
10880   23.08274743
10900   23.04123114
10920   22.99980077
10940   22.95845599
10960   22.91719647
10980   22.87602186
11000   22.83493184
11020   22.79392608
11040   22.75300426
11060   22.71216604
11080   22.67141111
11100   22.63073915
11120   22.59014984
11140   22.54964285
11160   22.50921787
11180   22.4688746
11200   22.42861271
11220   22.3884319
11240   22.34833185
11260   22.30831227
11280   22.26837285
11300   22.22851327
11320   22.18873325
11340   22.14903248
11360   22.10941066
11380   22.06986749
11400   22.03040269
11420   21.99101595
11440   21.95170699
11460   21.91247551
11480   21.87332123
11500   21.83424385
11520   21.7952431
11540   21.75631868
11560   21.71747033
11580   21.67869774
11600   21.64000066
11620   21.60137879
11640   21.56283186
11660   21.5243596
11680   21.48596174
11700   21.44763799
11720   21.40938809
11740   21.37121178
11760   21.33310878
11780   21.29507883
11800   21.25712166
11820   21.21923701
11840   21.18142461
11860   21.14368422
11880   21.10601556
11900   21.06841838
11920   21.03089243
11940   20.99343744
11960   20.95605317
11980   20.91873936
12000   20.88149576
12020   20.84432212
12040   20.80721819
12060   20.77018373
12080   20.73321849
12100   20.69632222
12120   20.65949469
12140   20.62273564
12160   20.58604484
12180   20.54942205
12200   20.51286703
12220   20.47637955
12240   20.43995936
12260   20.40360623
12280   20.36731994
12300   20.33110023
12320   20.2949469
12340   20.2588597
12360   20.2228384
12380   20.18688279
12400   20.15099263
12420   20.11516769
12440   20.07940776
12460   20.04371261
12480   20.00808201
12500   19.97251576
12520   19.93701362
12540   19.90157537
12560   19.86620081
12580   19.83088972
12600   19.79564187
12620   19.76045706
12640   19.72533507
12660   19.69027569
12680   19.65527871
12700   19.62034392
12720   19.58547111
12740   19.55066006
12760   19.51591059
12780   19.48122247
12800   19.4465955
12820   19.41202949
12840   19.37752422
12860   19.34307949
12880   19.30869511
12900   19.27437088
12920   19.24010659
12940   19.20590204
12960   19.17175705
12980   19.1376714
13000   19.10364492
13020   19.0696774
13040   19.03576865
13060   19.00191848
13080   18.9681267
13100   18.93439311
13120   18.90071753
13140   18.86709977
13160   18.83353964
13180   18.80003695
13200   18.76659152
13220   18.73320317
13240   18.6998717
13260   18.66659693
13280   18.63337869
13300   18.60021679
13320   18.56711104
13340   18.53406128
13360   18.50106732
13380   18.46812898
13400   18.43524608
13420   18.40241846
13440   18.36964592
13460   18.33692831
13480   18.30426543
13500   18.27165713
13520   18.23910323
13540   18.20660355
13560   18.17415793
13580   18.14176619
13600   18.10942816
13620   18.07714369
13640   18.04491259
13660   18.01273471
13680   17.98060987
13700   17.94853791
13720   17.91651867
13740   17.88455198
13760   17.85263768
13780   17.82077561
13800   17.7889656
13820   17.75720749
13840   17.72550113
13860   17.69384635
13880   17.662243
13900   17.63069091
13920   17.59918993
13940   17.5677399
13960   17.53634068
13980   17.50499209
14000   17.47369399
14020   17.44244622
14040   17.41124863
14060   17.38010108
14080   17.34900339
14100   17.31795543
14120   17.28695705
14140   17.25600808
14160   17.2251084
14180   17.19425784
14200   17.16345626
14220   17.13270351
14240   17.10199944
14260   17.07134392
14280   17.04073679
14300   17.0101779
14320   16.97966713
14340   16.94920431
14360   16.91878932
14380   16.888422
14400   16.85810221
14420   16.82782983
14440   16.79760469
14460   16.76742668
14480   16.73729564
14500   16.70721143
14520   16.67717393
14540   16.64718299
14560   16.61723847
14580   16.58734025
14600   16.55748818
14620   16.52768212
14640   16.49792196
14660   16.46820755
14680   16.43853875
14700   16.40891544
14720   16.37933749
14740   16.34980475
14760   16.32031711
14780   16.29087443
14800   16.26147658
14820   16.23212344
14840   16.20281487
14860   16.17355074
14880   16.14433093
14900   16.11515531
14920   16.08602376
14940   16.05693614
14960   16.02789234
14980   15.99889223
15000   15.96993568
15020   15.94102257
15040   15.91215277
15060   15.88332617
15080   15.85454264
15100   15.82580206
15120   15.79710431
15140   15.76844926
15160   15.7398368
15180   15.71126681
15200   15.68273917
15220   15.65425375
15240   15.62581045
15260   15.59740913
15280   15.5690497
15300   15.54073202
15320   15.51245598
15340   15.48422147
15360   15.45602837
15380   15.42787657
15400   15.39976594
15420   15.37169639
15440   15.34366778
15460   15.31568002
15480   15.28773298
15500   15.25982656
15520   15.23196065
15540   15.20413512
15560   15.17634988
15580   15.14860481
15600   15.1208998
15620   15.09323474
15640   15.06560952
15660   15.03802404
15680   15.01047818
15700   14.98297184
15720   14.95550491
15740   14.92807729
15760   14.90068886
15780   14.87333952
15800   14.84602917
15820   14.81875769
15840   14.791525
15860   14.76433097
15880   14.73717551
15900   14.71005851
15920   14.68297988
15940   14.6559395
15960   14.62893728
15980   14.6019731
16000   14.57504688
16020   14.54815852
16040   14.5213079
16060   14.49449493
16080   14.46771951
16100   14.44098153
16120   14.41428091
16140   14.38761754
16160   14.36099133
16180   14.33440217
16200   14.30784997
16220   14.28133462
16240   14.25485604
16260   14.22841413
16280   14.20200879
16300   14.17563992
16320   14.14930743
16340   14.12301123
16360   14.09675122
16380   14.0705273
16400   14.04433938
16420   14.01818737
16440   13.99207118
16460   13.96599071
16480   13.93994586
16500   13.91393656
16520   13.8879627
16540   13.86202419
16560   13.83612094
16580   13.81025287
16600   13.78441988
16620   13.75862188
16640   13.73285878
16660   13.70713049
16680   13.68143692
16700   13.65577799
16720   13.63015361
16740   13.60456368
16760   13.57900812
16780   13.55348684
16800   13.52799976
16820   13.50254679
16840   13.47712784
16860   13.45174283
16880   13.42639166
16900   13.40107426
16920   13.37579054
16940   13.35054042
16960   13.3253238
16980   13.30014061
17000   13.27499076
17020   13.24987417
17040   13.22479076
17060   13.19974043
17080   13.17472312
17100   13.14973873
17120   13.12478719
17140   13.09986841
17160   13.07498231
17180   13.05012881
17200   13.02530783
17220   13.00051929
17240   12.97576311
17260   12.95103921
17280   12.9263475
17300   12.90168792
17320   12.87706037
17340   12.85246478
17360   12.82790108
17380   12.80336918
17400   12.77886901
17420   12.75440049
17440   12.72996354
17460   12.70555808
17480   12.68118404
17500   12.65684134
17520   12.6325299
17540   12.60824965
17560   12.58400052
17580   12.55978242
17600   12.53559528
17620   12.51143903
17640   12.48731359
17660   12.46321889
17680   12.43915485
17700   12.4151214
17720   12.39111846
17740   12.36714597
17760   12.34320384
17780   12.31929202
17800   12.29541041
17820   12.27155896
17840   12.24773758
17860   12.22394622
17880   12.20018478
17900   12.17645322
17920   12.15275144
17940   12.12907939
17960   12.10543699
17980   12.08182416
18000   12.05824085
18020   12.03468699
18040   12.01116249
18060   11.98766729
18080   11.96420133
18100   11.94076453
18120   11.91735683
18140   11.89397815
18160   11.87062844
18180   11.84730761
18200   11.8240156
18220   11.80075236
18240   11.7775178
18260   11.75431186
18280   11.73113447
18300   11.70798557
18320   11.6848651
18340   11.66177298
18360   11.63870915
18380   11.61567354
18400   11.59266609
18420   11.56968673
18440   11.54673541
18460   11.52381204
18480   11.50091657
18500   11.47804894
18520   11.45520908
18540   11.43239693
18560   11.40961241
18580   11.38685548
18600   11.36412606
18620   11.3414241
18640   11.31874952
18660   11.29610228
18680   11.27348229
18700   11.25088951
18720   11.22832388
18740   11.20578532
18760   11.18327377
18780   11.16078919
18800   11.1383315
18820   11.11590064
18840   11.09349655
18860   11.07111918
18880   11.04876846
18900   11.02644434
18920   11.00414674
18940   10.98187562
18960   10.95963091
18980   10.93741255
19000   10.91522049
19020   10.89305466
19040   10.87091501
19060   10.84880148
19080   10.82671401
19100   10.80465253
19120   10.78261701
19140   10.76060736
19160   10.73862355
19180   10.7166655
19200   10.69473317
19220   10.6728265
19240   10.65094542
19260   10.62908989
19280   10.60725984
19300   10.58545522
19320   10.56367598
19340   10.54192205
19360   10.52019338
19380   10.49848992
19400   10.47681162
19420   10.4551584
19440   10.43353023
19460   10.41192705
19480   10.39034879
19500   10.36879541
19520   10.34726685
19540   10.32576306
19560   10.30428399
19580   10.28282957
19600   10.26139976
19620   10.23999451
19640   10.21861375
19660   10.19725744
19680   10.17592552
19700   10.15461794
19720   10.13333465
19740   10.1120756
19760   10.09084073
19780   10.06962999
19800   10.04844333
19820   10.02728069
19840   10.00614204
19860   9.985027302
19880   9.963936441
19900   9.942869401
19920   9.921826132
19940   9.900806582
19960   9.879810701
19980   9.858838439
20000   9.837889745
20020   9.816964569
20040   9.796062861
20060   9.775184571
20080   9.754329649
20100   9.733498046
20120   9.712689712
20140   9.691904598
20160   9.671142654
20180   9.650403833
20200   9.629688085
20220   9.608995361
20240   9.588325613
20260   9.567678793
20280   9.547054851
20300   9.526453742
20320   9.505875415
20340   9.485319824
20360   9.464786921
20380   9.444276659
20400   9.423788989
20420   9.403323866
20440   9.382881241
20460   9.362461069
20480   9.342063302
20500   9.321687893
20520   9.301334797
20540   9.281003967
20560   9.260695356
20580   9.24040892
20600   9.22014461
20620   9.199902383
20640   9.179682193
20660   9.159483993
20680   9.139307739
20700   9.119153385
20720   9.099020887
20740   9.078910199
20760   9.058821276
20780   9.038754075
20800   9.01870855
20820   8.998684657
20840   8.978682351
20860   8.95870159
20880   8.938742327
20900   8.918804521
20920   8.898888127
20940   8.878993101
20960   8.8591194
20980   8.839266981
21000   8.8194358
21020   8.799625814
21040   8.779836981
21060   8.760069258
21080   8.740322601
21100   8.720596968
21120   8.700892318
21140   8.681208607
21160   8.661545793
21180   8.641903835
21200   8.62228269
21220   8.602682316
21240   8.583102673
21260   8.563543717
21280   8.544005409
21300   8.524487706
21320   8.504990567
21340   8.485513951
21360   8.466057818
21380   8.446622126
21400   8.427206835
21420   8.407811904
21440   8.388437292
21460   8.369082959
21480   8.349748866
21500   8.330434971
21520   8.311141236
21540   8.291867619
21560   8.272614081
21580   8.253380583
21600   8.234167084
21620   8.214973547
21640   8.19579993
21660   8.176646195
21680   8.157512303
21700   8.138398215
21720   8.119303893
21740   8.100229296
21760   8.081174387
21780   8.062139127
21800   8.043123478
21820   8.024127401
21840   8.005150858
21860   7.986193811
21880   7.967256222
21900   7.948338054
21920   7.929439268
21940   7.910559827
21960   7.891699693
21980   7.872858828
22000   7.854037196
22020   7.83523476
22040   7.816451481
22060   7.797687324
22080   7.778942251
22100   7.760216225
22120   7.741509209
22140   7.722821168
22160   7.704152064
22180   7.685501862
22200   7.666870524
*/
