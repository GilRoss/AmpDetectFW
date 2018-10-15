#ifndef __OpticsDriver_H
#define __OpticsDriver_H

#include <cstdint>
#include <stdio.h>
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"
#include "gio.h"
#include "mibspi.h"
#include "het.h"
#include "rti.h"
#include "PcrProtocol.h"

/*#include    "stm32l4xx_hal.h"
#include    "stm32l4xx_hal_spi.h"
#include    "stm32l4xx_hal_spi_ex.h"
#include    "stm32l4xx_hal_uart.h"
#include    "stm32l4xx_hal_uart_ex.h"

#define     kAdcCS_Port     GPIOA
#define     kAdcCS_Pin      GPIO_PIN_0

#define     kSClk_Port      GPIOB
#define     kSClk_Pin       GPIO_PIN_0
#define     kMiso_Port      GPIOA
#define     kMiso_Pin       GPIO_PIN_1
#define     kMosi_Port      GPIOA
#define     kMosi_Pin       GPIO_PIN_4

#define     kLedSDI_Port    GPIOA
#define     kLedSDI_Pin     GPIO_PIN_5
#define     kLedCLK_Port    GPIOA
#define     kLedCLK_Pin     GPIO_PIN_6
#define     kLedLE_Port     GPIOA
#define     kLedLE_Pin      GPIO_PIN_7
#define     kLedOE_Port     GPIOA
#define     kLedOE_Pin      GPIO_PIN_8

#define     kLed_CS_Port        GPIOC
#define     kLed_CS_Pin         GPIO_PIN_8
#define     kLed_SClk_Port      GPIOC
#define     kLed_SClk_Pin       GPIO_PIN_6
#define     kLed_Miso_Port      GPIOB
#define     kLed_Miso_Pin       GPIO_PIN_8
#define     kLed_Mosi_Port      GPIOB
#define     kLed_Mosi_Pin       GPIO_PIN_9
#define     kLed_Ldac_Port      GPIOC
#define     kLed_Ldac_Pin       GPIO_PIN_5*/

#define     kledDacGroup        (0)
#define     kpdAdcGroup         (1)
#define     maxLedIntensity     (40000)
#define     maxMuxChannel       (8)


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class OpticsDriver
{
public:
    enum    {kBlue1 = 0, kGreen, kRed1, kBrown, kRed2, kBlue2, kNumOptChans, kNumOpticalAdcChans=8};
    enum    {kwrInputRegN = 0, kupdateDACRegN, kwrInputupdateAll, kwrInputupdateN, kpwrDownN, kpwrDownChip, kselectIntRef, kselectExtRef, kNoOp};
    enum AdcCfgBit {KEEP_CFG, OVERWRITE_CFG};
    enum AdcBwSelectBit {QUARTER_BW, FULL_BW};
    enum AdcReadBackBit {READ_BACK_EN, READ_BACK_DISABLE};
    enum AdcRefSelectionBits {
            INT_REF2_5_AND_TEMP_SENS,   // REF = 2.5 V buffered output.
            INT_REF4_096_AND_TEMP_SENS, // REF = 4.096 V buffered output.
            EXT_REF_AND_TEMP_SENS,      // Internal buffer disabled
            EXT_REF_AND_TEMP_SENS_BUFF, // Internal buffer and temperature sensor enabled.
            EXT_REF = 6,                // Int ref, int buffer, and temp sensor disabled.
            EXT_REF_BUFF                // Int buffer enabled. Int ref and temp sensor disabled.
        };
    enum AdcChSeqBits {
            DISABLE_SEQ,
            UPDATE_CFG,
            SCAN_IN_CH_AND_TEMP,
            SCAN_IN_CH
        };
    enum AdcInChCfgBits {
            BIPOLAR_DIFF_PAIRS  = 0, // INx referenced to VREF/2 ± 0.1 V.
            BIPOLAR             = 2, // INx referenced to COM = VREF/2 ± 0.1 V.
            TEMP_SENSOR         = 3, // Temperature sensor
            UNIPOLAR_DIFF_PAIRS = 4, // INx referenced to GND ± 0.1 V.
            UNIPOLAR_REF_TO_COM = 6, // INx referenced to COM = GND ± 0.1 V.
            UNIPOLAR_REF_TO_GND = 7 // INx referenced to GND
         };
    enum PDAdcChannels {
            PDINPUTA1 = 0,
            PDINPUTA2,
            PDINPUTA3,
            PDINPUTB1,
            PDINPUTB2,
            PDINPUTB3,
            PD_TEMP_A,
            PD_TEMP_B
        };
    enum AdcCtrlRegisterShifts {
            READ_BACK_SHIFT   =  0,
            SEQ_EN_SHIFT      =  1,
            REF_SEL_SHIFT     =  3,
            FULL_BW_SEL_SHIFT =  6,
            IN_CH_SEL_SHIFT   =  7,
            IN_CH_CFG_SHIFT   = 10,
            CFG_SHIFT         = 13
        };
    enum pdIntegratorState {
        RESET_STATE = 0,
        HOLD_STATE = 1,
        INTEGRATE_STATE = 2
    };
    enum pdIntegratorSwitch {
        RESET_SW = 0,
        HOLD_SW = 1
    };
    enum pdShiftRegisterPins {
        PDSR_DATA_PIN = PIN_HET_0,
        PDSR_CLK_PIN = PIN_HET_1,
        PDSR_LATCH_PIN = PIN_HET_2
    };
    enum spiChipSelect {
        LED_ADC_CS_PIN = PIN_CS0,
        PD_ADC_CS_PIN = PIN_CS1,
        LED_DAC_CS_PIN = PIN_CS2,
    };
    enum ledControlPins{
        LED_CTRL_S0 = PIN_HET_11,
        LED_CTRL_S1 = PIN_HET_12,
        LED_CTRL_S2 = PIN_HET_13
    };
    enum ledpdMisoPins {
        LED_PD_ADC_MISO_ENABLE_PIN = PIN_ENA
    };
    enum ledAdcChannels{LED1_TEMP = 0, LED2_TEMP, LED3_TEMP, LED4_TEMP, LED5_TEMP, LED6_TEMP, MONITOR_PD, LED_CURRENT};
    enum pdTempSelectPins{
        PD_TEMP_SW_CTRL_A = PIN_HET_9,
        PD_TEMP_SW_CTRL_B = PIN_HET_10
    };
    enum ledMuxMask
    {
        LED_MUX_MASK = 0xFFFFC7FF
    };


    //bool _integrationEnd;

    OpticsDriver(uint32_t nSiteIdx = 0);
       
    uint32_t GetDarkReading(const OpticalRead& optRead);
    uint32_t GetIlluminatedReading(const OpticalRead& optRead);
    void SetLedState(uint32_t nChanIdx, bool bStateOn = true);
    void SetLedState2(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration_us);
    void SetLedIntensity(uint32_t nChanIdx, uint32_t nLedIntensity);
    void SetLedsOff();
    uint32_t GetPhotoDiodeValue(uint32_t nledChanIdx, uint32_t npdChanIdx, uint32_t nDuration_us, uint32_t nLedIntensity);
    void OpticsDriverInit();
    void ADC7689ReadWrite(uint16_t* config, uint16_t* data, spiChipSelect chipSelect);
    void PhotoDiodeAdcConfig();
    void LedAdcConfig();
    void SetIntegratorState(pdIntegratorState state, uint32_t npdChanIdx);
    static void OpticsIntegrationDoneISR();
    uint16_t GetPhotoDiodeAdc(uint32_t nChanIdx);
    uint16_t GetLedAdc(uint32_t nChanIdx);
    uint16_t GetPhotoDiodeTemp(uint32_t nChanIdx);
    uint32_t SetLedOutputState(uint32_t nChanIdx);
    uint32_t GetActiveLedMonitorPDValue(void);
    uint32_t GetActiveLedTemp(void);
    uint32_t GetActivePhotoDiodeTemp(void);
    void IntegrateCommand(uint32_t nDuration_us);
    
protected:
  
private:
    uint32_t            _nLedStateMsk;
    static bool         _integrationEnd;
    static uint16_t     _nActiveLedTemperature;
    static uint16_t     _nActiveLedMonitorPDValue;
    static uint16_t     _nActivePhotoDiodeTemperature;
//    SPI_HandleTypeDef   _hSpi;
};

#endif // __OpticsDriver_H
