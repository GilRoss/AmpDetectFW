/*
 * OpticsDriver.cpp
 *
 *  Created on: Apr 27, 2018
 *      Author: z003xk2p
 */

#include "OpticsDriver.h"
#include "het.h"

bool        OpticsDriver::_integrationEnd = false;
uint16_t    OpticsDriver::_nActiveLedTemperature = 0;
uint16_t    OpticsDriver::_nActiveLedMonitorPDValue = 0;
uint16_t    OpticsDriver::_nActivePhotoDiodeTemperature = 0;
#define delay_uS 10000

#define LED_TEST (1)
#define NOT_PD_TEST (1)

/**
 * Name: OpticsDriver
 * Parameters:
 * Returns:
 * Description: Configures SPI bus and Turns off LED/PD
 */
OpticsDriver::OpticsDriver(uint32_t nSiteIdx)
{
#if 0
    int i = 1;
    int temp = 0;
    int j = 0;
    int upperlimit = 40000;
    int lowerlimit = 0;
    int stepsize = 500;
#endif
    /* Initialize LED and PD Board Driver */
    OpticsDriverInit();
#if 0
    while (1)
    {
        if (i > 6)
            i = 1;
        //else if (i == 5)
        //    i = 8;

        if (j == upperlimit)
        {
            temp = 1;
}
        else if (j == lowerlimit)
        {
            temp = 0;
            //SetLedsOff();
        }

        if (temp)
        {
            for (j = upperlimit; j>lowerlimit; j-=stepsize)
            {
                SetLedIntensity(i, j);
                vTaskDelay (5 / portTICK_PERIOD_MS);
                //SetLedIntensity(i, 250);
            //        SetLedIntensity(i, 500);
            //        SetLedIntensity(1, 1000);
            //    vTaskDelay (1000 / portTICK_PERIOD_MS);
                //SetLedIntensity(i, 0);
                //SetLedsOff();
                //vTaskDelay (10 / portTICK_PERIOD_MS);
                //i++;
                //SetLedIntensity(i, 0);
            }
        }
        else
        {
            for (j = lowerlimit; j <upperlimit; j+=stepsize)
            {
                SetLedIntensity(i, j);
                vTaskDelay (5 / portTICK_PERIOD_MS);
                //SetLedIntensity(i, 250);
            //        SetLedIntensity(i, 500);
            //        SetLedIntensity(1, 1000);
            //    vTaskDelay (1000 / portTICK_PERIOD_MS);
                //SetLedIntensity(i, 0);
                //SetLedsOff();
                //vTaskDelay (10 / portTICK_PERIOD_MS);
                //i++;
                //SetLedIntensity(i, 0);
            }
        }

        //SetLedIntensity(i, 0);
        //vTaskDelay (1000 / portTICK_PERIOD_MS);
        for (int count = 0; count < 1; count++)
        {
            //SetLedIntensity(i, 5000);
        //        SetLedIntensity(i, 500);
        //        SetLedIntensity(1, 1000);
            vTaskDelay (10 / portTICK_PERIOD_MS);
            SetLedIntensity(i, 0);
            //SetLedsOff();
            vTaskDelay (10 / portTICK_PERIOD_MS);
            SetLedIntensity(i, 10000);
            //vTaskDelay (10 / portTICK_PERIOD_MS);
        }

        i++;
        //for(int i=0; i<delay_uS; i++);
    }
#endif
#ifdef PD_TEST
    uint16_t pdValue = 0;
    uint16_t ledValue = 0;
    while (1)
    {
        for (int idx=0; idx<6; idx++)
        {
           //vTaskDelay (10 / portTICK_PERIOD_MS);
           //SetLedIntensity(idx, 1000);
           //pdValue = GetPhotoDiodeAdc(idx);
           pdValue = GetPhotoDiodeValue(idx, idx, 10000, 250);
           //ledValue = GetLedAdc(idx);
           vTaskDelay (10 / portTICK_PERIOD_MS);
           //SetLedsOff();

           // pdValue = GetPhotoDiodeValue(idx, idx, 10000, 5000);
        }
    }
#endif

}

/**
 * Name: GetDarkReading()
 * Parameters:
 * Returns:
 * Description:
 */
uint32_t OpticsDriver::GetDarkReading(const OpticalRead& optRead)
{
    SetLedsOff();
    uint32_t nValue = GetPhotoDiodeValue(   optRead.GetLedIdx(), optRead.GetDetectorIdx(),
                                            optRead.GetDetectorIntegrationTime(),
                                            0);
    return nValue;
}

/**
 * Name: GetIlluminatedReading()
 * Parameters:
 * Returns:
 * Description:
 */
uint32_t OpticsDriver::GetIlluminatedReading(const OpticalRead& optRead)
{
    SetLedsOff();
    uint32_t nValue = GetPhotoDiodeValue(   optRead.GetLedIdx(), optRead.GetDetectorIdx(),
                                            optRead.GetDetectorIntegrationTime(),
                                            optRead.GetLedIntensity());
    return nValue;
}

uint32_t OpticsDriver::GetActiveLedMonitorPDValue(void)
{
    return (uint32_t)_nActiveLedMonitorPDValue;
}

uint32_t OpticsDriver::GetActiveLedTemp(void)
{
    return (uint32_t)_nActiveLedTemperature;
}

uint32_t OpticsDriver::GetActivePhotoDiodeTemp(void)
{
    return (uint32_t)_nActivePhotoDiodeTemperature;
}

/**
 * Name: SetLedState()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::SetLedState(uint32_t nChanIdx, bool bStateOn)
{

}

/**
 * Name: SetLedState2()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::SetLedState2(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration_us)
{

    SetLedIntensity(nChanIdx, nIntensity);

    //If the user wants to energize LED for a specified amount of time.
    if (nDuration_us > 0)
    {
        for (int i = 0; i < (int)nDuration_us; i++);
        SetLedIntensity(nChanIdx, 0);
    }
}

/**
 * Name: SetLedIntensity()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::SetLedIntensity(uint32_t nChanIdx, uint32_t nLedIntensity)
{

    uint16_t ledData[2] = {0x0000, 0x0000}; // Write input/DAC
    uint32_t gpioOutputState = 0x00000000;

#if 0

    if (nLedIntensity == 0)
    {
       //gpioOutputState = SetLedOutputState(7);
       //gioSetPort(hetPORT1, gpioOutputState);
        SetLedsOff();
    }
    else
    {
        switch(nChanIdx)
        {
            case 1:
                gioSetBit(hetPORT1, LED_CTRL_S2, 0);
                gioSetBit(hetPORT1, LED_CTRL_S1, 0);
                gioSetBit(hetPORT1, LED_CTRL_S0, 0);
                break;
            case 2:
                gioSetBit(hetPORT1, LED_CTRL_S2, 0);
                gioSetBit(hetPORT1, LED_CTRL_S1, 0);
                gioSetBit(hetPORT1, LED_CTRL_S0, 1);
                break;
            case 3:
                gioSetBit(hetPORT1, LED_CTRL_S2, 0);
                gioSetBit(hetPORT1, LED_CTRL_S1, 1);
                gioSetBit(hetPORT1, LED_CTRL_S0, 0);
                break;
            case 4:
                gioSetBit(hetPORT1, LED_CTRL_S2, 0);
                gioSetBit(hetPORT1, LED_CTRL_S1, 1);
                gioSetBit(hetPORT1, LED_CTRL_S0, 1);
                break;
            case 5:
                gioSetBit(hetPORT1, LED_CTRL_S2, 1);
                gioSetBit(hetPORT1, LED_CTRL_S1, 0);
                gioSetBit(hetPORT1, LED_CTRL_S0, 0);
                break;
            case 6:
                gioSetBit(hetPORT1, LED_CTRL_S2, 1);
                gioSetBit(hetPORT1, LED_CTRL_S1, 0);
                gioSetBit(hetPORT1, LED_CTRL_S0, 1);
                break;
            default:
                gioSetBit(hetPORT1, LED_CTRL_S2, 1);
                gioSetBit(hetPORT1, LED_CTRL_S1, 1);
                gioSetBit(hetPORT1, LED_CTRL_S0, 1);
                break;
        }
        ledData[0] = 0x3000 + ((uint16_t)nLedIntensity >> 4);
        ledData[1] = (uint16_t)nLedIntensity << 12;

        gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 0);
        mibspiSetData(mibspiREG3, kledDacGroup, ledData);
    mibspiTransfer(mibspiREG3, kledDacGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kledDacGroup)));
        gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 1);
}


#endif
    /* Caps Led intensity to maximum allowed if user input higher value */
#if 1
    if (nLedIntensity > maxLedIntensity)
    {
        nLedIntensity = maxLedIntensity;
    }

    if (nLedIntensity == 0)
    {
        //gpioOutputState = SetLedOutputState(7);
        //gioSetPort(hetPORT1, gpioOutputState);
        SetLedsOff();
    }
    else
    {
        gpioOutputState = SetLedOutputState(nChanIdx);
        gioSetPort(hetPORT1, gpioOutputState);

        ledData[0] = 0x3000 + ((uint16_t)nLedIntensity >> 4);
        ledData[1] = (uint16_t)nLedIntensity << 12;

        gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 0);
        mibspiSetData(mibspiREG3, kledDacGroup, ledData);
        mibspiTransfer(mibspiREG3, kledDacGroup);
        while(!(mibspiIsTransferComplete(mibspiREG3, kledDacGroup)));
        gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 1);
    }
#endif
}

/**
 * Name: SetLedsOff()
 * Parameters: void
 * Returns: void
 * Description: Turns off LED by setting intensity to 0.
 */
void OpticsDriver::SetLedsOff()
{
    uint32_t gpioOutputState = 0x00000000;

    gpioOutputState = gioGetPort(hetPORT1) & LED_MUX_MASK;
    gpioOutputState |= (7 << LED_CTRL_S0);
    //gpioOutputState = SetLedOutputState(7);
    gioSetPort(hetPORT1, gpioOutputState);

    //gioSetBit(hetPORT1, LED_CTRL_S0, 1);
    //gioSetBit(hetPORT1, LED_CTRL_S1, 1);
    //gioSetBit(hetPORT1, LED_CTRL_S2, 1);
}

/**
 * Name: SetLedOutputState()
 * Parameters: uint32_t nChanIdx
 * Returns: void
 * Description:
 */
uint32_t OpticsDriver::SetLedOutputState (uint32_t nChanIdx)
{
    uint32_t gpioOutputState = 0x00000000;

    gpioOutputState = gioGetPort(hetPORT1) & LED_MUX_MASK;
    gpioOutputState |= (nChanIdx << LED_CTRL_S0);

    return gpioOutputState;
}

/**
 * Name: GetPhotoDiodeValue()
 * Parameters:
 * Returns:
 * Description:
 */
uint32_t OpticsDriver::GetPhotoDiodeValue(uint32_t nledChanIdx, uint32_t npdChanIdx, uint32_t nDuration_us, uint32_t nLedIntensity)
{
    uint16_t adcValue = 0x0000;
    hetSIGNAL_t signal;
    uint32_t adcChannel = 0;

    signal.duty = 50;
    signal.period = nDuration_us;
    _integrationEnd = false;

    switch(npdChanIdx)
    {
    case 0:
        npdChanIdx = 0x0001 << PDINPUTA1;
        adcChannel = 0;
        break;
    case 1:
        npdChanIdx = 0x0001 << PDINPUTB1;
        adcChannel = 3;
        break;
    case 2:
        npdChanIdx = 0x0001 << PDINPUTA2;
        adcChannel = 1;
        break;
    case 3:
        npdChanIdx = 0x0001 << PDINPUTB2;
        adcChannel = 4;
        break;
    case 4:
        npdChanIdx = 0x0001 << PDINPUTA3;
        adcChannel = 2;
        break;
    case 5:
        npdChanIdx = 0x0001 << PDINPUTB3;
        adcChannel = 5;
        break;
    default:
        npdChanIdx = 0x0001 << PDINPUTA1;
        break;
    }

    /* Reset Integrator first */
    SetIntegratorState(RESET_STATE, npdChanIdx);
    gioSetBit(hetPORT1, PDSR_LATCH_PIN, 1); //Enable Reset State
    for(int i=0; i<delay_uS*5; i++); //Hold in reset state for 5 ms

    /* Turn On LED */
    SetLedIntensity(nledChanIdx, nLedIntensity);
    for(int i=0; i<delay_uS*10; i++); //Hold for 10 ms time after turning on LED

    /* Set Duration for Integration */
    pwmSetSignal(hetRAM1, pwm0, signal);
    pwmEnableNotification(hetREG1, pwm0, pwmEND_OF_PERIOD);

    SetIntegratorState(INTEGRATE_STATE, npdChanIdx);
    /* Wait until interrupt occurs */
    while (!_integrationEnd);
    _integrationEnd = false;
    gioSetBit(hetPORT1, PDSR_LATCH_PIN, 1); //Enable Integration State (start integrating)

    /* CRITICAL TIMING --> These commands need to be completed in integration time specified */
    //////////////////////////////////////////////////////////////////////////////////////////
    SetIntegratorState(HOLD_STATE, npdChanIdx); //Configure Hold state

    /* Read LED Temperature, LED Monitoring PhotoDiode, PhotoDiode Temperature */
    _nActiveLedMonitorPDValue = GetLedAdc(MONITOR_PD); // Get Monitor PD value
    _nActiveLedTemperature = GetLedAdc(nledChanIdx);
    _nActivePhotoDiodeTemperature = GetPhotoDiodeTemp(npdChanIdx);
    //////////////////////////////////////////////////////////////////////////////////////////

    /* Wait for integrationTimeExpired flag to be set */
    while (!_integrationEnd);
    pwmDisableNotification(hetREG1, pwm0, pwmEND_OF_BOTH);

    for(int i=0; i<delay_uS; i++); //1 ms delay
    /* Turn Off LED */
    SetLedsOff();

    for(int i=0; i<delay_uS; i++); //Hold for 1 ms time before reading

    adcValue = GetPhotoDiodeAdc(adcChannel);

    //for(int i=0; i<delay_uS; i++);

    //SetIntegratorState(RESET_STATE, npdChanIdx);
    //gioSetBit(hetPORT1, LATCH_PIN, 1); //Enable Reset State

    return (uint32_t)adcValue;
}


/**
 * Name: OpticsDriverInit()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::OpticsDriverInit(void)
{
    /* Configure following GPIOs
     * LED GPIO: N2HET1[13] -> CS (Output/High); N2HET1[12] -> LDAC (Output/High)
     * PD ADC GPIO: N2HET1[14] -> CNV (Output/Low)
     * PD SR GPIO: N2HET1[24] -> DS (Serial Data In); N2HET1[26] -> SHCP (Shift Reg Clock Input)
     *             N2HET1[28] -> STCP (Storage Reg Clock Input)
     */
    uint16_t configData_w_Reset[2] = {0x4080, 0x0000}; //Stand alone mode; Gain = 2*Vref; Ref = Enabled; Operation = Normal Mode; Reset Input/DAC registers

    //Configure LED DAC: AD5683R
    gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 0);
    mibspiSetData(mibspiREG3, kledDacGroup, configData_w_Reset);
    mibspiTransfer(mibspiREG3, kledDacGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kledDacGroup)));
    gioSetBit(mibspiPORT3, LED_DAC_CS_PIN, 1);

    SetLedsOff();
    /* Configure ADC on Photo Diode board */
    PhotoDiodeAdcConfig();
    LedAdcConfig();

    /* Disable PWM notification */
    pwmDisableNotification(hetREG1, pwm0, pwmEND_OF_BOTH);
}

void OpticsDriver::ADC7689ReadWrite(uint16_t* config, uint16_t* data, spiChipSelect chipSelect)
{
    /* Send 2 dummy conversion to update config register on Photo Diode ADC */
    /* First dummy conversion */
    mibspiSetData(mibspiREG3, kpdAdcGroup, config); //Set Config command
    gioSetBit(mibspiPORT3, chipSelect, 0); //Start sending command
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(mibspiPORT3, chipSelect, 1);
    mibspiGetData(mibspiREG3, kpdAdcGroup, data);
    for(int i=0; i<50; i++); //Wait for sometime for conv/acq to complete

}

void OpticsDriver::PhotoDiodeAdcConfig(void)
{
    uint16_t adcConfig[2] = {0x0000, 0x0000};
    uint16_t nAdcVal[2] = {0x0000, 0x0000};

    adcConfig[0] |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig[0] |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig[0] |= PDINPUTA2 << IN_CH_SEL_SHIFT;
    adcConfig[0] |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig[0] |= EXT_REF << REF_SEL_SHIFT;
    adcConfig[0] |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig[0] |= READ_BACK_EN << READ_BACK_SHIFT;
    adcConfig[0] <<= 2;

    // Set MCU_SPI3_SOMI_SW low to receive data from PD ADC
    gioSetBit(mibspiPORT3, LED_PD_ADC_MISO_ENABLE_PIN, 0);

    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);

#if 0
    /* Check if command is written correctly */
    if (adcConfig[0] == nAdcVal[1])
    {
        printf("Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
    }
    else
    {
        printf("Write to ADC not successful!!!\n");
        printf("Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
    }
#endif
}

void OpticsDriver::LedAdcConfig(void)
{
    uint16_t adcConfig[2] = {0x0000, 0x0000};
    uint16_t nAdcVal[2] = {0x0000, 0x0000};

    adcConfig[0] |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig[0] |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig[0] |= PDINPUTA1 << IN_CH_SEL_SHIFT;
    adcConfig[0] |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig[0] |= INT_REF4_096_AND_TEMP_SENS << REF_SEL_SHIFT;
    adcConfig[0] |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig[0] |= READ_BACK_EN << READ_BACK_SHIFT;
    adcConfig[0] <<= 2;

    // Set MCU_SPI3_SOMI_SW low to receive data from LED ADC
    gioSetBit(mibspiPORT3, LED_PD_ADC_MISO_ENABLE_PIN, 1);

    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);
}

/**
 * Name: GetPhotoDiodeAdc()
 * Parameters: uint32_t nChanIdx: ADC channel to read
 * Returns: uint16_t nAdcVal: ADC value
 * Description: Obtain value from ADC
 */
uint16_t OpticsDriver::GetPhotoDiodeAdc(uint32_t nChanIdx)
{
    uint16_t nAdcVal[2] = {0x0000, 0x0000};
    uint16_t adcConfig[2] = {0x0000, 0x0000};

    /* Get Adc Value */
    adcConfig[0] |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig[0] |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig[0] |= (uint16_t) nChanIdx << IN_CH_SEL_SHIFT;
    adcConfig[0] |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig[0] |= EXT_REF << REF_SEL_SHIFT;
    adcConfig[0] |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig[0] |= READ_BACK_EN << READ_BACK_SHIFT;
    adcConfig[0] <<= 2;

    // Set MCU_SPI3_SOMI_SW low to receive data from PD ADC
    gioSetBit(mibspiPORT3, LED_PD_ADC_MISO_ENABLE_PIN, 0);

    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, PD_ADC_CS_PIN);

    //gioSetBit(mibspiPORT3, LED_PD_ADC_MISO_ENABLE_PIN, 1);

#if 0
    /* Check if command is written correctly */
    if (adcConfig[0] == nAdcVal[1])
    {
        printf("PD Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
    }
    else
   {
       printf("Write to PD ADC not successful!!! ");
       printf("PD Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
   }
#endif

    return nAdcVal[0];
}

/**
 * Name: GetPhotoDiodeTemp()
 * Parameters: uint32_t nChanIdx: ADC channel to read
 * Returns: uint16_t nAdcVal: ADC value
 * Description: Obtain value from ADC
 */
uint16_t OpticsDriver::GetPhotoDiodeTemp(uint32_t nChanIdx)
{
    uint16_t nAdcVal = 0;
    switch (nChanIdx)
    {
    case 0:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 0);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 0);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_A);
        break;
    case 1:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 0);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 0);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_B);
        break;
    case 2:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 1);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 0);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_A);
        break;
    case 3:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 1);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 0);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_B);
        break;
    case 4:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 0);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 1);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_A);
        break;
    case 5:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 0);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 1);
        nAdcVal = GetPhotoDiodeAdc(PD_TEMP_B);
        break;
    default:
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_A, 0);
        gioSetBit(hetPORT1, PD_TEMP_SW_CTRL_B, 0);
    }
    return nAdcVal;
}
/**
 * Name: GetLedAdc()
 * Parameters: uint32_t nChanIdx: ADC channel to read
 * Returns: uint16_t nAdcVal: ADC value
 * Description: Obtain value from ADC
 */
uint16_t OpticsDriver::GetLedAdc(uint32_t nChanIdx)
{
    uint16_t adcConfig[2] = {0x0000, 0x0000};
    uint16_t nAdcVal[2] = {0x0000, 0x0000};

    /* Get Adc Value */
    adcConfig[0] |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig[0] |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig[0] |= (uint16_t) nChanIdx << IN_CH_SEL_SHIFT;
    adcConfig[0] |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig[0] |= INT_REF4_096_AND_TEMP_SENS << REF_SEL_SHIFT;
    adcConfig[0] |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig[0] |= READ_BACK_EN << READ_BACK_SHIFT;
    adcConfig[0] <<= 2;

    // Set MCU_SPI3_SOMI_SW low to receive data from PD ADC
    gioSetBit(mibspiPORT3, LED_PD_ADC_MISO_ENABLE_PIN, 1);

    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);
    ADC7689ReadWrite(adcConfig, nAdcVal, LED_ADC_CS_PIN);

#if 0
    /* Check if command is written correctly */
    if (adcConfig[0] == nAdcVal[1])
    {
        printf("LED Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
    }
    else
   {
       printf("Write to LED ADC not successful!!! ");
       printf("LED Write word is 0x%x and Read word is 0x%x\n", adcConfig[0], nAdcVal[1]);
   }
#endif

    return nAdcVal[0];
}

/**
 * Name: SetIntegratorState()
 * Parameters:
 * Returns:
 * Description: Utilizes shift register to set integrator state
 */
void OpticsDriver::SetIntegratorState(pdIntegratorState state, uint32_t npdChanIdx)
{
    uint8_t pinState = 0;
    uint16_t serialDataIn = 0;
    /* Initialize GPIOs for Shift Register: Pin */
    /* Pull-down Latch pin, Clk pin and Data pin */
    gioSetBit(hetPORT1, PDSR_LATCH_PIN, 0);
    gioSetBit(hetPORT1, PDSR_DATA_PIN, 0);

    switch (state)
    {
        case RESET_STATE:
            serialDataIn = 0x0000;
            break;
        case HOLD_STATE:
            serialDataIn = 0xFFFF;
            break;
        case INTEGRATE_STATE:
            //serialDataIn = 0x0000 | (uint8_t) npdChanIdx;
            serialDataIn = 0x00FF;
            break;
        default:
            break;
    }

    /* shift data into shift register */
    for (int i = 15; i>=0; i--)
    {
        gioSetBit(hetPORT1, PDSR_CLK_PIN, 0);
        /* Checks if serial data is 1 or 0 shifts accordingly */
        if (serialDataIn & (1<<i))
        {
            pinState = 1;
        }
        else
        {
            pinState = 0;
        }
        gioSetBit(hetPORT1, PDSR_DATA_PIN, pinState);
        gioSetBit(hetPORT1, PDSR_CLK_PIN, 1);
        gioSetBit(hetPORT1, PDSR_DATA_PIN, 0);
    }
    gioSetBit(hetPORT1, PDSR_CLK_PIN, 0);
    //gioSetBit(hetPORT1, LATCH_PIN, 1);
}

extern "C" void OpticsIntegrationDoneISR();
void OpticsIntegrationDoneISR()
{
    OpticsDriver::OpticsIntegrationDoneISR();
}

void OpticsDriver::OpticsIntegrationDoneISR()
{
/*  enter user code between the USER CODE BEGIN and USER CODE END. */
/* USER CODE BEGIN (35) */
   /* Trigger Hold on Integrated Value */
   gioSetBit(hetPORT1, PDSR_LATCH_PIN, 1);
   /* Set flag pwmNotification */
   _integrationEnd = true;

/* USER CODE END */
}

