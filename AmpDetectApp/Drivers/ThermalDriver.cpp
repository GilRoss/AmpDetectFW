#include "ThermalDriver.h"
#include "gio.h"
#include "mibspi.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Pid          ThermalDriver::_pid(0.000050, 5000, -5000, 0.7, 7000, 0);    //Peltier.
//Pid          ThermalDriver::_pid(0.000050, 10000, -10000, 1.8, 30800, 0);    //Fixed 2ohm load.
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

    gioSetBit(mibspiPORT5, PIN_SIMO, 0);    //Disable TEC.

    //Initialize DAC8563
    SendDacMsg(CMD_RESET, 0, POWER_ON_RESET);
    SendDacMsg(CMD_ENABLE_INT_REF, 0, DISABLE_INT_REF_AND_RESET_DAC_GAINS_TO_1);
    SendDacMsg(CMD_SET_LDAC_PIN, 0, SET_LDAC_PIN_INACTIVE_DAC_B_INACTIVE_DAC_A);
    SendDacMsg(CMD_POWER_DAC, 0, POWER_DOWN_DAC_B_HI_Z);
    SetCurrentControlVar((0xFFFF / 2) + 180);
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
    if (_bCurrentPidEnabled)
    {
        //Get current A/D counts.
        _nA2DCounts = (int32_t)GetA2D(1) - 21950;
        _nA2DCounts += (-25 *_nA2DCounts) / 1000;
        _nControlVar = _pid.calculate((double)_nSetpoint_mA, (double)_nA2DCounts);
        SetCurrentControlVar((0xFFFF / 2) + 180 + (int32_t)_nControlVar);
        gioSetBit(mibspiPORT5, PIN_SIMO, 1);
    }
    else
    {
        gioSetBit(mibspiPORT5, PIN_SIMO, 0);
        SetCurrentControlVar((0xFFFF / 2) + 180);
    }

    if (_nIsrCount == 0)
        _nBlockTemp_cnts = (int32_t)GetA2D(2);
    if (_nIsrCount == (400/2))
        _nSampleTemp_cnts = (int32_t)GetA2D(3);
    _nIsrCount = (_nIsrCount == 400) ? 0 : _nIsrCount + 1;
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
void ThermalDriver::SetPidParams(PidType nType, const PidParams& params)
{

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
    float nVoltage_V = _nBlockTemp_cnts * (5.0 / 65535);
    int32_t nBlockTemp_mC =  convertVoltageToTemp(nVoltage_V);

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
    uint16_t    arTxBuf[2];
    uint16_t    arRxBuf[2];

    arTxBuf[0] = cfg >> 8;
    arTxBuf[1] = cfg & 0x00FF;

    gioSetBit(mibspiPORT5, PIN_SOMI, 0);          //Conversion pin
    mibspiSetData(mibspiREG1, 1, arTxBuf);
    mibspiTransfer(mibspiREG1, 1);
    while (! mibspiIsTransferComplete(mibspiREG1, 1));
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Conversion pin
    mibspiGetData(mibspiREG1, 1, arRxBuf);
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Wait 4 us.
    gioSetBit(mibspiPORT5, PIN_SOMI, 1);          //Conversion pin

    *pData = (arRxBuf[0] << 8) + arRxBuf[1];
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
uint32_t ThermalDriver::GetA2D(int nChanIdx)
{
    uint16 data;
    uint16 cfg = 0;
    cfg |= OVERWRITE_CFG << CFG_SHIFT;
    cfg |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    cfg |= nChanIdx << IN_CH_SEL_SHIFT;
    cfg |= FULL_BW << FULL_BW_SEL_SHIFT;
    cfg |= EXT_REF << REF_SEL_SHIFT;
    cfg |= DISABLE_SEQ << SEQ_EN_SHIFT;
    cfg |= READ_BACK_DISABLE << READ_BACK_SHIFT;
    cfg <<= 2;

    //Make certain the Somi of Spi1 is connected to the TEC ADC.
    gioSetBit(mibspiPORT3, PIN_ENA, Spi1SomiSrc::kTecAdc);

    ReadDacMsg(cfg, &data); // conv DATA(n-1), clock out CFG(n), clock in DATA(n-2)
    ReadDacMsg(0x00, &data); // conv DATA(n), clock out CFG(n+1), clock in DATA(n-1)
    ReadDacMsg(0x00, &data); // conv DATA(n+1), clock out CFG(n+2), clock in DATA(n)

    return data;
}

// TEMP_AINx = 2.048 * Rt/(107000 + Rt)
// 1/TEMP_AINx = (107000 / Rt + 1) / 2.048
// 1 / Rt = (2.048 /TEMP_AINx - 1) / 107000
int32_t ThermalDriver::convertVoltageToTemp(float ain, int standard)
{
    float rt = 10700 / ((2.048 / ain) - 1);
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

/*conversion ThermalDriver::s_convTable[] = {
    { .rt = 336479.00, .temp_C = { -40, -40.0 } },
    { .rt = 314904.00, .temp_C = { -39, -38.2 } },
    { .rt = 294848.00, .temp_C = { -38, -36.4 } },
    { .rt = 276194.00, .temp_C = { -37, -34.6 } },
    { .rt = 258838.00, .temp_C = { -36, -32.8 } },
    { .rt = 242681.00, .temp_C = { -35, -31.0 } },
    { .rt = 227632.00, .temp_C = { -34, -29.2 } },
    { .rt = 213610.00, .temp_C = { -33, -27.4 } },
    { .rt = 200539.00, .temp_C = { -32, -25.6 } },
    { .rt = 188349.00, .temp_C = { -31, -23.8 } },

    { .rt = 176974.00, .temp_C = { -30, -22.0 } },
    { .rt = 166356.00, .temp_C = { -29, -20.2 } },
    { .rt = 156441.00, .temp_C = { -28, -18.4 } },
    { .rt = 147177.00, .temp_C = { -27, -16.6 } },
    { .rt = 138518.00, .temp_C = { -26, -14.8 } },
    { .rt = 130421.00, .temp_C = { -25, -13.0 } },
    { .rt = 122847.00, .temp_C = { -24, -11.2 } },
    { .rt = 115759.00, .temp_C = { -23,  -9.4 } },
    { .rt = 109122.00, .temp_C = { -22,  -7.6 } },
    { .rt = 102906.00, .temp_C = { -21,  -5.8 } },

    { .rt =  97081.00, .temp_C = { -20,  -4.0 } },
    { .rt =  91621.00, .temp_C = { -19,  -2.2 } },
    { .rt =  86501.00, .temp_C = { -18,  -0.4 } },
    { .rt =  81698.00, .temp_C = { -17,   1.4 } },
    { .rt =  77190.00, .temp_C = { -16,   3.2 } },
    { .rt =  72957.00, .temp_C = { -15,   5.0 } },
    { .rt =  68982.00, .temp_C = { -14,   6.8 } },
    { .rt =  65246.00, .temp_C = { -13,   8.6 } },
    { .rt =  61736.00, .temp_C = { -12,  10.4 } },
    { .rt =  58434.00, .temp_C = { -11,  12.2 } },

    { .rt =  55329.00, .temp_C = { -10,  14.0 } },
    { .rt =  52407.00, .temp_C = {  -9,  15.8 } },
    { .rt =  49656.00, .temp_C = {  -8,  17.6 } },
    { .rt =  47066.00, .temp_C = {  -7,  19.4 } },
    { .rt =  44626.00, .temp_C = {  -6,  21.2 } },
    { .rt =  42327.00, .temp_C = {  -5,  23.0 } },
    { .rt =  40159.00, .temp_C = {  -4,  24.8 } },
    { .rt =  38115.00, .temp_C = {  -3,  26.6 } },
    { .rt =  36187.00, .temp_C = {  -2,  28.4 } },
    { .rt =  34368.00, .temp_C = {  -1,  30.2 } },
    { .rt =  32650.00, .temp_C = {   0,  32.0 } },
    { .rt =  31029.00, .temp_C = {   1,  33.8 } },
    { .rt =  29498.00, .temp_C = {   2,  35.6 } },
    { .rt =  28052.00, .temp_C = {   3,  37.4 } },
    { .rt =  26685.00, .temp_C = {   4,  39.2 } },
    { .rt =  25392.00, .temp_C = {   5,  41.0 } },
    { .rt =  24170.00, .temp_C = {   6,  42.8 } },
    { .rt =  23013.00, .temp_C = {   7,  44.6 } },
    { .rt =  21918.00, .temp_C = {   8,  46.4 } },
    { .rt =  20882.00, .temp_C = {   9,  48.2 } },
    { .rt =  19901.00, .temp_C = {  10,  50.0 } },
    { .rt =  18971.00, .temp_C = {  11,  51.8 } },
    { .rt =  18090.00, .temp_C = {  12,  53.6 } },
    { .rt =  17255.00, .temp_C = {  13,  55.4 } },
    { .rt =  16463.00, .temp_C = {  14,  57.2 } },
    { .rt =  15712.00, .temp_C = {  15,  59.0 } },
    { .rt =  14999.00, .temp_C = {  16,  60.8 } },
    { .rt =  14323.00, .temp_C = {  17,  62.6 } },
    { .rt =  13681.00, .temp_C = {  18,  64.4 } },
    { .rt =  13072.00, .temp_C = {  19,  66.2 } },
    { .rt =  12493.00, .temp_C = {  20,  68.0 } },
    { .rt =  11942.00, .temp_C = {  21,  69.8 } },
    { .rt =  11419.00, .temp_C = {  22,  71.6 } },
    { .rt =  10922.00, .temp_C = {  23,  73.4 } },
    { .rt =  10450.00, .temp_C = {  24,  75.2 } },
    { .rt =  10000.00, .temp_C = {  25,  77.0 } },
    { .rt =   9572.00, .temp_C = {  26,  78.8 } },
    { .rt =   9165.00, .temp_C = {  27,  80.6 } },
    { .rt =   8777.00, .temp_C = {  28,  82.4 } },
    { .rt =   8408.00, .temp_C = {  29,  84.2 } },
    { .rt =   8057.00, .temp_C = {  30,  86.0 } },
    { .rt =   7722.00, .temp_C = {  31,  87.8 } },
    { .rt =   7402.00, .temp_C = {  32,  89.6 } },
    { .rt =   7098.00, .temp_C = {  33,  91.4 } },
    { .rt =   6808.00, .temp_C = {  34,  93.2 } },
    { .rt =   6531.00, .temp_C = {  35,  95.0 } },
    { .rt =   6267.00, .temp_C = {  36,  96.8 } },
    { .rt =   6015.00, .temp_C = {  37,  98.6 } },
    { .rt =   5775.00, .temp_C = {  38, 100.4 } },
    { .rt =   5545.00, .temp_C = {  39, 102.2 } },
    { .rt =   5326.00, .temp_C = {  40, 104.0 } },
    { .rt =   5117.00, .temp_C = {  41, 105.8 } },
    { .rt =   4917.00, .temp_C = {  42, 107.6 } },
    { .rt =   4725.00, .temp_C = {  43, 109.4 } },
    { .rt =   4543.00, .temp_C = {  44, 111.2 } },
    { .rt =   4368.00, .temp_C = {  45, 113.0 } },
    { .rt =   4201.00, .temp_C = {  46, 114.8 } },
    { .rt =   4041.00, .temp_C = {  47, 116.6 } },
    { .rt =   3888.00, .temp_C = {  48, 118.4 } },
    { .rt =   3742.00, .temp_C = {  49, 120.2 } },
    { .rt =   3602.00, .temp_C = {  50, 122.0 } },
    { .rt =   3468.00, .temp_C = {  51, 123.8 } },
    { .rt =   3340.00, .temp_C = {  52, 125.6 } },
    { .rt =   3217.00, .temp_C = {  53, 127.4 } },
    { .rt =   3099.00, .temp_C = {  54, 129.2 } },
    { .rt =   2986.00, .temp_C = {  55, 131.0 } },
    { .rt =   2878.00, .temp_C = {  56, 132.8 } },
    { .rt =   2774.00, .temp_C = {  57, 134.6 } },
    { .rt =   2675.00, .temp_C = {  58, 136.4 } },
    { .rt =   2579.00, .temp_C = {  59, 138.2 } },
    { .rt =   2488.00, .temp_C = {  60, 140.0 } },
    { .rt =   2400.00, .temp_C = {  61, 141.8 } },
    { .rt =   2316.00, .temp_C = {  62, 143.6 } },
    { .rt =   2235.00, .temp_C = {  63, 145.4 } },
    { .rt =   2157.00, .temp_C = {  64, 147.2 } },
    { .rt =   2083.00, .temp_C = {  65, 149.0 } },
    { .rt =   2011.00, .temp_C = {  66, 150.8 } },
    { .rt =   1942.00, .temp_C = {  67, 152.6 } },
    { .rt =   1876.00, .temp_C = {  68, 154.4 } },
    { .rt =   1813.00, .temp_C = {  69, 156.2 } },
    { .rt =   1752.00, .temp_C = {  70, 158.0 } },
    { .rt =   1693.00, .temp_C = {  71, 159.8 } },
    { .rt =   1637.00, .temp_C = {  72, 161.6 } },
    { .rt =   1582.00, .temp_C = {  73, 163.4 } },
    { .rt =   1530.00, .temp_C = {  74, 165.2 } },
    { .rt =   1480.00, .temp_C = {  75, 167.0 } },
    { .rt =   1432.00, .temp_C = {  76, 168.8 } },
    { .rt =   1385.00, .temp_C = {  77, 170.6 } },
    { .rt =   1340.00, .temp_C = {  78, 172.4 } },
    { .rt =   1297.00, .temp_C = {  79, 174.2 } },
    { .rt =   1255.00, .temp_C = {  80, 176.0 } },
    { .rt =   1215.00, .temp_C = {  81, 177.8 } },
    { .rt =   1177.00, .temp_C = {  82, 179.6 } },
    { .rt =   1140.00, .temp_C = {  83, 181.4 } },
    { .rt =   1104.00, .temp_C = {  84, 183.2 } },
    { .rt =   1070.00, .temp_C = {  85, 185.0 } },
    { .rt =   1037.00, .temp_C = {  86, 186.8 } },
    { .rt =   1005.00, .temp_C = {  87, 188.6 } },
    { .rt =    973.80, .temp_C = {  88, 190.4 } },
    { .rt =    944.10, .temp_C = {  89, 192.2 } },
    { .rt =    915.50, .temp_C = {  90, 194.0 } },
    { .rt =    887.80, .temp_C = {  91, 195.8 } },
    { .rt =    861.20, .temp_C = {  92, 197.6 } },
    { .rt =    835.40, .temp_C = {  93, 199.4 } },
    { .rt =    810.60, .temp_C = {  94, 201.2 } },
    { .rt =    786.60, .temp_C = {  95, 203.0 } },
    { .rt =    763.50, .temp_C = {  96, 204.8 } },
    { .rt =    741.20, .temp_C = {  97, 206.6 } },
    { .rt =    719.60, .temp_C = {  98, 208.4 } },
    { .rt =    698.70, .temp_C = {  99, 210.2 } },
    { .rt =    678.60, .temp_C = { 100, 212.0 } },
    { .rt =    659.10, .temp_C = { 101, 213.8 } },
    { .rt =    640.30, .temp_C = { 102, 215.6 } },
    { .rt =    622.20, .temp_C = { 103, 217.4 } },
    { .rt =    604.60, .temp_C = { 104, 219.2 } },
    { .rt =    587.60, .temp_C = { 105, 221.0 } },
    { .rt =    571.20, .temp_C = { 106, 222.8 } },
    { .rt =    555.30, .temp_C = { 107, 224.6 } },
    { .rt =    539.90, .temp_C = { 108, 226.4 } },
    { .rt =    525.00, .temp_C = { 109, 228.2 } },
    { .rt =    510.60, .temp_C = { 110, 230.0 } },
    { .rt =    496.70, .temp_C = { 111, 231.8 } },
    { .rt =    483.20, .temp_C = { 112, 233.6 } },
    { .rt =    470.10, .temp_C = { 113, 235.4 } },
    { .rt =    457.50, .temp_C = { 114, 237.2 } },
    { .rt =    445.30, .temp_C = { 115, 239.0 } },
    { .rt =    433.40, .temp_C = { 116, 240.8 } },
    { .rt =    421.90, .temp_C = { 117, 242.6 } },
    { .rt =    410.80, .temp_C = { 118, 244.4 } },
    { .rt =    400.00, .temp_C = { 119, 246.2 } },
    { .rt =    389.60, .temp_C = { 120, 248.0 } },
    { .rt =    379.40, .temp_C = { 121, 249.8 } },
    { .rt =    369.60, .temp_C = { 122, 251.6 } },
    { .rt =    360.10, .temp_C = { 123, 253.4 } },
    { .rt =    350.90, .temp_C = { 124, 255.2 } },
    { .rt =    341.90, .temp_C = { 125, 257.0 } },
    { .rt =    0,                             },
};*/
