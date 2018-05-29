/*
 * OpticsDriver.cpp
 *
 *  Created on: Apr 27, 2018
 *      Author: z003xk2p
 */

#include "OpticsDriver.h"
#include "het.h"

bool        OpticsDriver::_integrationEnd = false;
#define delay_uS 10000

/**
 * Name: OpticsDriver
 * Parameters:
 * Returns:
 * Description: Configures SPI bus and Turns off LED/PD
 */
OpticsDriver::OpticsDriver(uint32_t nSiteIdx)
{
    /* Initialize LED and PD Board Driver */
    OpticsDriverInit();

}

/**
 * Name: GetDarkReading()
 * Parameters:
 * Returns:
 * Description:
 */
uint32_t OpticsDriver::GetDarkReading(uint32_t nChanIdx)
{
    SetLedsOff(nChanIdx);

    //Get average of multiple readings.
    uint32_t nAve = 0;
    for (int i = 0; i < 10; i++)
        nAve += GetAdc(nChanIdx);

    return (nAve / 10);
}

/**
 * Name: GetIlluminatedReading()
 * Parameters:
 * Returns:
 * Description:
 */
uint32_t OpticsDriver::GetIlluminatedReading(uint32_t nChanIdx)
{
    //LED = on, wait for exposure time.
    SetLedState(nChanIdx, true);
    vTaskDelay(2);

    //Get average of multiple readings.
    uint32_t nAve = 0;
    for (int i = 0; i < 10; i++)
        nAve += GetAdc(nChanIdx);

    SetLedsOff(nChanIdx);
    return (nAve / 10);
}

/**
 * Name: SetLedState()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::SetLedState(uint32_t nChanIdx, bool bStateOn)
{
    /*        SetLedsOff();
            for (int i = 0; i < 8; i++)
            {
                GPIO_PinState nState = GPIO_PIN_RESET;
                if (bStateOn && ((7 - i) ==  nChanIdx))
                    nState = GPIO_PIN_SET;

                HAL_GPIO_WritePin(kLedSDI_Port, kLedSDI_Pin, nState);
                HAL_GPIO_WritePin(kLedSDI_Port, kLedSDI_Pin, nState);
                HAL_GPIO_WritePin(kLedCLK_Port, kLedCLK_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(kLedCLK_Port, kLedCLK_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(kLedCLK_Port, kLedCLK_Pin, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(kLedCLK_Port, kLedCLK_Pin, GPIO_PIN_RESET);
            }

            //Pulse latch enable and enable outputs.
            HAL_GPIO_WritePin(kLedSDI_Port, kLedSDI_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(kLedOE_Port, kLedOE_Pin, GPIO_PIN_RESET);*/
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
    uint16_t nBitPattern[2] = {0xFF00, 0x0000};

    nBitPattern[0] = (uint16_t)(nBitPattern[0] | (((uint16_t)kwrInputupdateN << 4) | nChanIdx));
    nBitPattern[1] = (uint16_t)nLedIntensity;
    gioSetBit(hetPORT1, 13, 0);
    mibspiSetData(mibspiREG3, kledDacGroup, nBitPattern);
    mibspiTransfer(mibspiREG3, kledDacGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kledDacGroup)));
    gioSetBit(hetPORT1, 13, 1);
}

/**
 * Name: SetLedsOff()
 * Parameters:
 * Returns:
 * Description: Turns off LED by setting intensity to 0.
 */
void OpticsDriver::SetLedsOff(uint32_t nChanIdx)
{
    SetLedIntensity(nChanIdx, 0);
}

/**
 * Name: GetPhotoDiodeValue()
 * Parameters:
 * Returns:
 * Description:
 */
void OpticsDriver::GetPhotoDiodeValue(uint32_t nledChanIdx, uint32_t npdChanIdx, uint32_t nDuration_us, uint32_t nLedIntensity, uint16_t *data)
{
    uint16_t adcValue = 0x0000;
    hetSIGNAL_t signal;
    uint32_t adcChannel = 0;

    signal.duty = 50;
    signal.period = nDuration_us;
    _integrationEnd = FALSE;

    switch(npdChanIdx)
    {
    case 1:
        npdChanIdx = 0x0001 << PDINPUTA1;
        adcChannel = 0;
        break;
    case 2:
        npdChanIdx = 0x0001 << PDINPUTB1;
        adcChannel = 3;
        break;
    case 3:
        npdChanIdx = 0x0001 << PDINPUTA2;
        adcChannel = 1;
        break;
    case 4:
        npdChanIdx = 0x0001 << PDINPUTB2;
        adcChannel = 4;
        break;
    case 5:
        npdChanIdx = 0x0001 << PDINPUTA3;
        adcChannel = 2;
        break;
    case 6:
        npdChanIdx = 0x0001 << PDINPUTB3;
        adcChannel = 5;
        break;
    default:
        npdChanIdx = 0x0001 << PDINPUTA1;
        break;
    }

    /* Reset Integrator first */
    SetIntegratorState(RESET_STATE, npdChanIdx);
    gioSetBit(hetPORT1, LATCH_PIN, 1); //Enable Reset State
    for(int i=0; i<delay_uS; i++); //10 ms delay

    /* Turn On LED */
    SetLedIntensity(nledChanIdx, nLedIntensity);
    for(int i=0; i<delay_uS*10; i++); //Hold for 10 ms time before reading

    /* Set Duration for Integration */
    pwmSetSignal(hetRAM1, pwm0, signal);
    pwmEnableNotification(hetREG1, pwm0, pwmEND_OF_PERIOD);

    SetIntegratorState(INTEGRATE_STATE, npdChanIdx);
    /* Wait until interrupt occurs */
    while (!_integrationEnd);
    _integrationEnd = FALSE;
    gioSetBit(hetPORT1, LATCH_PIN, 1); //Enable Integration State (start integrating)

    SetIntegratorState(HOLD_STATE, npdChanIdx); //Configure Hold state

    /* Wait for integrationTimeExpired flag to be set */
    while (!_integrationEnd);
    pwmDisableNotification(hetREG1, pwm0, pwmEND_OF_BOTH);

    for(int i=0; i<delay_uS; i++);
    /* Turn Off LED */
    SetLedsOff(nledChanIdx);

    for(int i=0; i<delay_uS; i++); //Hold for 10 ms time before reading

    adcValue = GetAdc(adcChannel);
    for(int i=0; i<delay_uS; i++);

    SetIntegratorState(RESET_STATE, npdChanIdx);
    gioSetBit(hetPORT1, LATCH_PIN, 1); //Enable Reset State

    *data = adcValue;
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
    uint32_t gpioDirectionConfig = 0x00000000;
    uint32_t gpioOutputState = 0x00000000;

    /* Set GPIO pin direction */
    gpioDirectionConfig |= (1<<PIN_HET_12);
    gpioDirectionConfig |= (1<<PIN_HET_13);
    gpioDirectionConfig |= (1<<PIN_HET_14);
    gpioDirectionConfig |= (1<<PIN_HET_24);
    gpioDirectionConfig |= (1<<PIN_HET_26);
    gpioDirectionConfig |= (1<<PIN_HET_28);

    /* Set GPIO output state */
    gpioOutputState |= (1<<PIN_HET_12);
    gpioOutputState |= (1<<PIN_HET_13);
    gpioOutputState |= (1<<PIN_HET_14);
    gpioOutputState |= (0<<PIN_HET_24);
    gpioOutputState |= (0<<PIN_HET_26);
    gpioOutputState |= (1<<PIN_HET_28); //Latch pin is high to start with

    gioSetDirection(hetPORT1, gpioDirectionConfig);
    gioSetPort(hetPORT1, gpioOutputState);

    /* Configure ADC on Photo Diode board */
    AdcConfig();
}

void OpticsDriver::AdcConfig(void)
{
    uint16_t *adcConfigPointer = NULL;
    uint16_t adcConfig = 0x0000;

    adcConfigPointer = &adcConfig;

    adcConfig |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig |= PDINPUTA1 << IN_CH_SEL_SHIFT;
    adcConfig |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig |= EXT_REF << REF_SEL_SHIFT;
    adcConfig |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig |= READ_BACK_DISABLE << READ_BACK_SHIFT;
    adcConfig <<= 2;

    /* Send 2 dummy conversion to update config register on Photo Diode ADC */
    /* First dummy conversion */
    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start sending command
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    for(int i=0; i<1000; i++); //Wait for sometime for conv/acq to complete

    /* Wait for conversion to complete ~ 3.2 uS */
    adcConfig = 0;
    /* Second dummy conversion */
    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start dummy conversion
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    for(int i=0; i<1000; i++);

    /* Set Configuration Value */
    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start dummy conversion
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    for(int i=0; i<1000; i++);
}

/**
 * Name: GetAdc()
 * Parameters: uint32_t nChanIdx: ADC channel to read
 * Returns: uint16_t nAdcVal: ADC value
 * Description: Obtain value from ADC
 */
uint16_t OpticsDriver::GetAdc(uint32_t nChanIdx)
{
    uint16_t nAdcVal = 0;
    uint16_t *data = NULL;
    uint16_t *adcConfigPointer = NULL;
    uint16_t adcConfig = 0x0000;

    data = &nAdcVal;
    adcConfigPointer = &adcConfig;

    /* Get Adc Value */
    adcConfig |= OVERWRITE_CFG << CFG_SHIFT;
    adcConfig |= UNIPOLAR_REF_TO_GND << IN_CH_CFG_SHIFT;
    adcConfig |= (uint16_t) nChanIdx << IN_CH_SEL_SHIFT;
    adcConfig |= FULL_BW << FULL_BW_SEL_SHIFT;
    adcConfig |= EXT_REF << REF_SEL_SHIFT;
    adcConfig |= DISABLE_SEQ << SEQ_EN_SHIFT;
    adcConfig |= READ_BACK_DISABLE << READ_BACK_SHIFT;
    adcConfig <<= 2;

    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start dummy conversion
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    mibspiGetData(mibspiREG3, kpdAdcGroup, data); //Get ADC Value
    for(int i=0; i<1000; i++);

    adcConfig = 0;

    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start dummy conversion
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    mibspiGetData(mibspiREG3, kpdAdcGroup, data); //Get ADC Value
    for(int i=0; i<1000; i++);

    mibspiSetData(mibspiREG3, kpdAdcGroup, adcConfigPointer); //Set Config command
    gioSetBit(hetPORT1, 14, 0); //Start dummy conversion
    mibspiTransfer(mibspiREG3, kpdAdcGroup);
    while(!(mibspiIsTransferComplete(mibspiREG3, kpdAdcGroup)));
    gioSetBit(hetPORT1, 14, 1);
    mibspiGetData(mibspiREG3, kpdAdcGroup, data); //Get ADC Value
    for(int i=0; i<1000; i++);

    nAdcVal = *data;
    return nAdcVal;
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
    gioSetBit(hetPORT1, LATCH_PIN, 0);
    gioSetBit(hetPORT1, DATA_PIN, 0);

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
        gioSetBit(hetPORT1, CLK_PIN, 0);
        /* Checks if serial data is 1 or 0 shifts accordingly */
        if (serialDataIn & (1<<i))
        {
            pinState = 1;
        }
        else
        {
            pinState = 0;
        }
        gioSetBit(hetPORT1, DATA_PIN, pinState);
        gioSetBit(hetPORT1, CLK_PIN, 1);
        gioSetBit(hetPORT1, DATA_PIN, 0);
    }
    gioSetBit(hetPORT1, CLK_PIN, 0);
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
   gioSetBit(hetPORT1, PIN_HET_28, 1);
   /* Set flag pwmNotification */
   _integrationEnd = true;

/* USER CODE END */
}

