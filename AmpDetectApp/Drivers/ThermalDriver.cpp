#include "ThermalDriver.h"
#include "gio.h"
#include "mibspi.h"

///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
Pid          ThermalDriver::_pid(0.000050, 10000, -10000, 1, 0, 0);    //Peltier.
//Pid          ThermalDriver::_pid(0.000050, 10000, -10000, 1.25, 17000, 0);    //Peltier.
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

        if (_nIsrCount == 0)
            _nBlockTemp_cnts = (int32_t)GetA2D(2);
        if (_nIsrCount == 0xFF)
            _nSampleTemp_cnts = (int32_t)GetA2D(3);
        _nIsrCount = (_nIsrCount + 1) & 0x1FF;
    }
    else
        gioSetBit(mibspiPORT5, PIN_SIMO, 0);
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
uint32_t ThermalDriver::GetA2D(int channel)
{
    uint16 data;
    uint16 cfg = 0;
    cfg |= OVERWRITE_CFG << CFG_SHIFT;
    cfg |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    cfg |= channel << IN_CH_SEL_SHIFT;
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
        return s_convTable[0].temp[standard];
    }
    for (i = 0; s_convTable[i].rt; i++) {
        if (s_convTable[i].rt >= rt && rt >= s_convTable[i + 1].rt) {
            if (s_convTable[i + 1].rt == 0) {
                temp = s_convTable[i].temp[standard];
                break;
            }
            else {
                temp = s_convTable[i + 1].temp[standard]
                  + (rt - s_convTable[i + 1].rt)
                  * (s_convTable[i].temp[standard] - s_convTable[i + 1].temp[standard])
                  / (s_convTable[i].rt - s_convTable[i + 1].rt);
                 break;
            }
        }
    }
    if (!s_convTable[i].rt) {
        return s_convTable[i].temp[standard];
    }
    return (int32_t)(temp * 1000);
}

conversion ThermalDriver::s_convTable[] = {
    { .rt = 336479.00, .temp = { -40, -40.0 } },
    { .rt = 314904.00, .temp = { -39, -38.2 } },
    { .rt = 294848.00, .temp = { -38, -36.4 } },
    { .rt = 276194.00, .temp = { -37, -34.6 } },
    { .rt = 258838.00, .temp = { -36, -32.8 } },
    { .rt = 242681.00, .temp = { -35, -31.0 } },
    { .rt = 227632.00, .temp = { -34, -29.2 } },
    { .rt = 213610.00, .temp = { -33, -27.4 } },
    { .rt = 200539.00, .temp = { -32, -25.6 } },
    { .rt = 188349.00, .temp = { -31, -23.8 } },

    { .rt = 176974.00, .temp = { -30, -22.0 } },
    { .rt = 166356.00, .temp = { -29, -20.2 } },
    { .rt = 156441.00, .temp = { -28, -18.4 } },
    { .rt = 147177.00, .temp = { -27, -16.6 } },
    { .rt = 138518.00, .temp = { -26, -14.8 } },
    { .rt = 130421.00, .temp = { -25, -13.0 } },
    { .rt = 122847.00, .temp = { -24, -11.2 } },
    { .rt = 115759.00, .temp = { -23,  -9.4 } },
    { .rt = 109122.00, .temp = { -22,  -7.6 } },
    { .rt = 102906.00, .temp = { -21,  -5.8 } },

    { .rt =  97081.00, .temp = { -20,  -4.0 } },
    { .rt =  91621.00, .temp = { -19,  -2.2 } },
    { .rt =  86501.00, .temp = { -18,  -0.4 } },
    { .rt =  81698.00, .temp = { -17,   1.4 } },
    { .rt =  77190.00, .temp = { -16,   3.2 } },
    { .rt =  72957.00, .temp = { -15,   5.0 } },
    { .rt =  68982.00, .temp = { -14,   6.8 } },
    { .rt =  65246.00, .temp = { -13,   8.6 } },
    { .rt =  61736.00, .temp = { -12,  10.4 } },
    { .rt =  58434.00, .temp = { -11,  12.2 } },

    { .rt =  55329.00, .temp = { -10,  14.0 } },
    { .rt =  52407.00, .temp = {  -9,  15.8 } },
    { .rt =  49656.00, .temp = {  -8,  17.6 } },
    { .rt =  47066.00, .temp = {  -7,  19.4 } },
    { .rt =  44626.00, .temp = {  -6,  21.2 } },
    { .rt =  42327.00, .temp = {  -5,  23.0 } },
    { .rt =  40159.00, .temp = {  -4,  24.8 } },
    { .rt =  38115.00, .temp = {  -3,  26.6 } },
    { .rt =  36187.00, .temp = {  -2,  28.4 } },
    { .rt =  34368.00, .temp = {  -1,  30.2 } },
    { .rt =  32650.00, .temp = {   0,  32.0 } },
    { .rt =  31029.00, .temp = {   1,  33.8 } },
    { .rt =  29498.00, .temp = {   2,  35.6 } },
    { .rt =  28052.00, .temp = {   3,  37.4 } },
    { .rt =  26685.00, .temp = {   4,  39.2 } },
    { .rt =  25392.00, .temp = {   5,  41.0 } },
    { .rt =  24170.00, .temp = {   6,  42.8 } },
    { .rt =  23013.00, .temp = {   7,  44.6 } },
    { .rt =  21918.00, .temp = {   8,  46.4 } },
    { .rt =  20882.00, .temp = {   9,  48.2 } },
    { .rt =  19901.00, .temp = {  10,  50.0 } },
    { .rt =  18971.00, .temp = {  11,  51.8 } },
    { .rt =  18090.00, .temp = {  12,  53.6 } },
    { .rt =  17255.00, .temp = {  13,  55.4 } },
    { .rt =  16463.00, .temp = {  14,  57.2 } },
    { .rt =  15712.00, .temp = {  15,  59.0 } },
    { .rt =  14999.00, .temp = {  16,  60.8 } },
    { .rt =  14323.00, .temp = {  17,  62.6 } },
    { .rt =  13681.00, .temp = {  18,  64.4 } },
    { .rt =  13072.00, .temp = {  19,  66.2 } },
    { .rt =  12493.00, .temp = {  20,  68.0 } },
    { .rt =  11942.00, .temp = {  21,  69.8 } },
    { .rt =  11419.00, .temp = {  22,  71.6 } },
    { .rt =  10922.00, .temp = {  23,  73.4 } },
    { .rt =  10450.00, .temp = {  24,  75.2 } },
    { .rt =  10000.00, .temp = {  25,  77.0 } },
    { .rt =   9572.00, .temp = {  26,  78.8 } },
    { .rt =   9165.00, .temp = {  27,  80.6 } },
    { .rt =   8777.00, .temp = {  28,  82.4 } },
    { .rt =   8408.00, .temp = {  29,  84.2 } },
    { .rt =   8057.00, .temp = {  30,  86.0 } },
    { .rt =   7722.00, .temp = {  31,  87.8 } },
    { .rt =   7402.00, .temp = {  32,  89.6 } },
    { .rt =   7098.00, .temp = {  33,  91.4 } },
    { .rt =   6808.00, .temp = {  34,  93.2 } },
    { .rt =   6531.00, .temp = {  35,  95.0 } },
    { .rt =   6267.00, .temp = {  36,  96.8 } },
    { .rt =   6015.00, .temp = {  37,  98.6 } },
    { .rt =   5775.00, .temp = {  38, 100.4 } },
    { .rt =   5545.00, .temp = {  39, 102.2 } },
    { .rt =   5326.00, .temp = {  40, 104.0 } },
    { .rt =   5117.00, .temp = {  41, 105.8 } },
    { .rt =   4917.00, .temp = {  42, 107.6 } },
    { .rt =   4725.00, .temp = {  43, 109.4 } },
    { .rt =   4543.00, .temp = {  44, 111.2 } },
    { .rt =   4368.00, .temp = {  45, 113.0 } },
    { .rt =   4201.00, .temp = {  46, 114.8 } },
    { .rt =   4041.00, .temp = {  47, 116.6 } },
    { .rt =   3888.00, .temp = {  48, 118.4 } },
    { .rt =   3742.00, .temp = {  49, 120.2 } },
    { .rt =   3602.00, .temp = {  50, 122.0 } },
    { .rt =   3468.00, .temp = {  51, 123.8 } },
    { .rt =   3340.00, .temp = {  52, 125.6 } },
    { .rt =   3217.00, .temp = {  53, 127.4 } },
    { .rt =   3099.00, .temp = {  54, 129.2 } },
    { .rt =   2986.00, .temp = {  55, 131.0 } },
    { .rt =   2878.00, .temp = {  56, 132.8 } },
    { .rt =   2774.00, .temp = {  57, 134.6 } },
    { .rt =   2675.00, .temp = {  58, 136.4 } },
    { .rt =   2579.00, .temp = {  59, 138.2 } },
    { .rt =   2488.00, .temp = {  60, 140.0 } },
    { .rt =   2400.00, .temp = {  61, 141.8 } },
    { .rt =   2316.00, .temp = {  62, 143.6 } },
    { .rt =   2235.00, .temp = {  63, 145.4 } },
    { .rt =   2157.00, .temp = {  64, 147.2 } },
    { .rt =   2083.00, .temp = {  65, 149.0 } },
    { .rt =   2011.00, .temp = {  66, 150.8 } },
    { .rt =   1942.00, .temp = {  67, 152.6 } },
    { .rt =   1876.00, .temp = {  68, 154.4 } },
    { .rt =   1813.00, .temp = {  69, 156.2 } },
    { .rt =   1752.00, .temp = {  70, 158.0 } },
    { .rt =   1693.00, .temp = {  71, 159.8 } },
    { .rt =   1637.00, .temp = {  72, 161.6 } },
    { .rt =   1582.00, .temp = {  73, 163.4 } },
    { .rt =   1530.00, .temp = {  74, 165.2 } },
    { .rt =   1480.00, .temp = {  75, 167.0 } },
    { .rt =   1432.00, .temp = {  76, 168.8 } },
    { .rt =   1385.00, .temp = {  77, 170.6 } },
    { .rt =   1340.00, .temp = {  78, 172.4 } },
    { .rt =   1297.00, .temp = {  79, 174.2 } },
    { .rt =   1255.00, .temp = {  80, 176.0 } },
    { .rt =   1215.00, .temp = {  81, 177.8 } },
    { .rt =   1177.00, .temp = {  82, 179.6 } },
    { .rt =   1140.00, .temp = {  83, 181.4 } },
    { .rt =   1104.00, .temp = {  84, 183.2 } },
    { .rt =   1070.00, .temp = {  85, 185.0 } },
    { .rt =   1037.00, .temp = {  86, 186.8 } },
    { .rt =   1005.00, .temp = {  87, 188.6 } },
    { .rt =    973.80, .temp = {  88, 190.4 } },
    { .rt =    944.10, .temp = {  89, 192.2 } },
    { .rt =    915.50, .temp = {  90, 194.0 } },
    { .rt =    887.80, .temp = {  91, 195.8 } },
    { .rt =    861.20, .temp = {  92, 197.6 } },
    { .rt =    835.40, .temp = {  93, 199.4 } },
    { .rt =    810.60, .temp = {  94, 201.2 } },
    { .rt =    786.60, .temp = {  95, 203.0 } },
    { .rt =    763.50, .temp = {  96, 204.8 } },
    { .rt =    741.20, .temp = {  97, 206.6 } },
    { .rt =    719.60, .temp = {  98, 208.4 } },
    { .rt =    698.70, .temp = {  99, 210.2 } },
    { .rt =    678.60, .temp = { 100, 212.0 } },
    { .rt =    659.10, .temp = { 101, 213.8 } },
    { .rt =    640.30, .temp = { 102, 215.6 } },
    { .rt =    622.20, .temp = { 103, 217.4 } },
    { .rt =    604.60, .temp = { 104, 219.2 } },
    { .rt =    587.60, .temp = { 105, 221.0 } },
    { .rt =    571.20, .temp = { 106, 222.8 } },
    { .rt =    555.30, .temp = { 107, 224.6 } },
    { .rt =    539.90, .temp = { 108, 226.4 } },
    { .rt =    525.00, .temp = { 109, 228.2 } },
    { .rt =    510.60, .temp = { 110, 230.0 } },
    { .rt =    496.70, .temp = { 111, 231.8 } },
    { .rt =    483.20, .temp = { 112, 233.6 } },
    { .rt =    470.10, .temp = { 113, 235.4 } },
    { .rt =    457.50, .temp = { 114, 237.2 } },
    { .rt =    445.30, .temp = { 115, 239.0 } },
    { .rt =    433.40, .temp = { 116, 240.8 } },
    { .rt =    421.90, .temp = { 117, 242.6 } },
    { .rt =    410.80, .temp = { 118, 244.4 } },
    { .rt =    400.00, .temp = { 119, 246.2 } },
    { .rt =    389.60, .temp = { 120, 248.0 } },
    { .rt =    379.40, .temp = { 121, 249.8 } },
    { .rt =    369.60, .temp = { 122, 251.6 } },
    { .rt =    360.10, .temp = { 123, 253.4 } },
    { .rt =    350.90, .temp = { 124, 255.2 } },
    { .rt =    341.90, .temp = { 125, 257.0 } },
    { .rt =    0,                             },
};
