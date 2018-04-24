#ifndef __OpticsDriver_H
#define __OpticsDriver_H

#include <cstdint>
#include "FreeRTOS.h"
#include "os_task.h"
#include "os_semphr.h"

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


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
class OpticsDriver
{
public:
    enum    {kBlue1 = 0, kGreen, kRed1, kBrown, kRed2, kBlue2, kNumOptChans};

    OpticsDriver(uint32_t nSiteIdx = 0)
        :_nLedStateMsk(0)
    {
        //Configure SClk, Miso, Mosi and ADC/LED chip select pins.
/*        __HAL_RCC_GPIOA_CLK_ENABLE();
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_GPIOC_CLK_ENABLE();
        GPIO_InitTypeDef  GPIO_InitStruct;
        GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
        GPIO_InitStruct.Pull  = GPIO_PULLUP;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        
        //Outputs
        GPIO_InitStruct.Pin = kAdcCS_Pin;
        HAL_GPIO_Init(kAdcCS_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kSClk_Pin;
        HAL_GPIO_Init(kSClk_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kMosi_Pin;
        HAL_GPIO_Init(kMosi_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLedSDI_Pin;
        HAL_GPIO_Init(kLedSDI_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLedCLK_Pin;
        HAL_GPIO_Init(kLedCLK_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLedLE_Pin;
        HAL_GPIO_Init(kLedLE_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLedOE_Pin;
        HAL_GPIO_Init(kLedOE_Port, &GPIO_InitStruct);
        
        GPIO_InitStruct.Pin = kLed_CS_Pin;
        HAL_GPIO_Init(kLed_CS_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLed_SClk_Pin;
        HAL_GPIO_Init(kLed_SClk_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLed_Mosi_Pin;
        HAL_GPIO_Init(kLed_Mosi_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLed_Ldac_Pin;
        HAL_GPIO_Init(kLed_Ldac_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = GPIO_PIN_10;
        HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
       
        //Inputs
        GPIO_InitStruct.Mode  = GPIO_MODE_INPUT;
        GPIO_InitStruct.Pin = kMiso_Pin;
        HAL_GPIO_Init(kMiso_Port, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = kLed_Miso_Pin;
        HAL_GPIO_Init(kLed_Miso_Port, &GPIO_InitStruct);
        
        //Initial state for chip selects.
        HAL_GPIO_WritePin(kAdcCS_Port, kAdcCS_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(kLed_CS_Port, kLed_CS_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(kLed_Ldac_Port, kLed_Ldac_Pin, GPIO_PIN_SET);
*/
        SetLedsOff();
    }
       
    uint32_t GetDarkReading(uint32_t nChanIdx)
    {
        SetLedsOff();

        //Get average of multiple readings.
        uint32_t nAve = 0;
        for (int i = 0; i < 10; i++)
            nAve += GetAdc(nChanIdx);

        return (nAve / 10);
    }
       
    uint32_t GetIlluminatedReading(uint32_t nChanIdx)
    {
        //LED = on, wait for exposure time.
        SetLedState(nChanIdx, true);
        vTaskDelay(2);
        
        //Get average of multiple readings.
        uint32_t nAve = 0;
        for (int i = 0; i < 10; i++)
            nAve += GetAdc(nChanIdx);

        SetLedsOff();
        return (nAve / 10);
    }
    
    void SetLedState(uint32_t nChanIdx, bool bStateOn = true)
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
    
    void SetLedState2(uint32_t nChanIdx, uint32_t nIntensity, uint32_t nDuration_us)
    {
        SetLedIntensity(nChanIdx, nIntensity);
        
        //If the user wants to energize LED for a specified amount of time.
        if (nDuration_us > 0)
        {
            for (int i = 0; i < (int)nDuration_us; i++);
            SetLedIntensity(nChanIdx, 0);
        }
   }
    
    void SetLedIntensity(uint32_t nChanIdx, uint32_t nLedIntensity)
    {
        //Assume user wants to turn LED off.
/*        uint32_t nBitPattern = ((uint32_t)0x03 << 4) + nChanIdx;
        if (nLedIntensity != 0)
            nBitPattern = ((uint32_t)0x03 << 4) + nChanIdx;
        nBitPattern = ((nBitPattern << 16) + nLedIntensity) << 8;
        
        HAL_GPIO_WritePin(kLed_CS_Port, kLed_CS_Pin, GPIO_PIN_RESET);
        for (int i = 0; i < 24; i++)
        {
            GPIO_PinState nState = GPIO_PIN_RESET;
            if (nBitPattern & 0x80000000)
                nState = GPIO_PIN_SET;
            nBitPattern <<= 1;
          
            HAL_GPIO_WritePin(kLed_Mosi_Port, kLed_Mosi_Pin, nState);
            HAL_GPIO_WritePin(kLed_SClk_Port, kLed_SClk_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(kLed_SClk_Port, kLed_SClk_Pin, GPIO_PIN_RESET);
        }
        
        //Set CS high, then pulse latch enable.
        HAL_GPIO_WritePin(kLed_CS_Port, kLed_CS_Pin, GPIO_PIN_SET);
        for (int i = 0; i < 400; i++);
        HAL_GPIO_WritePin(kLed_Ldac_Port, kLed_Ldac_Pin, GPIO_PIN_RESET);
        for (int i = 0; i < 20; i++);
        HAL_GPIO_WritePin(kLed_Ldac_Port, kLed_Ldac_Pin, GPIO_PIN_SET);*/
    }
    
    void SetLedsOff()
    {
/*        HAL_GPIO_WritePin(kLedOE_Port, kLedOE_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(kLedCLK_Port, kLedCLK_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(kLedSDI_Port, kLedSDI_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(kLedLE_Port, kLedLE_Pin, GPIO_PIN_RESET);*/
    }
    
    uint32_t GetAdc(uint32_t nChanIdx)
    {
//        uint8_t     arChanIdxToAdcIdx[8] = {0x05, 0x06, 0x04, 0x02, 0x01, 0x00, 0x03, 0x07};
        uint32_t    nAdcVal = 0;
/*        uint32_t    nTxValue = 0x87000000 | (arChanIdxToAdcIdx[nChanIdx] << 28);
        
        HAL_GPIO_WritePin(kSClk_Port, kSClk_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(kAdcCS_Port, kAdcCS_Pin, GPIO_PIN_RESET);//Assert chip select
        for (int i = 0; i < 32; i++)
        {
            nAdcVal <<= 1;
            if (nTxValue & 0x80000000)
                HAL_GPIO_WritePin(kMosi_Port, kMosi_Pin, GPIO_PIN_SET);
            else
                HAL_GPIO_WritePin(kMosi_Port, kMosi_Pin, GPIO_PIN_RESET);
            nTxValue <<= 1;
            HAL_GPIO_WritePin(kSClk_Port, kSClk_Pin, GPIO_PIN_SET);
            GPIO_PinState nState = HAL_GPIO_ReadPin(kMiso_Port, kMiso_Pin);
            nState = HAL_GPIO_ReadPin(kMiso_Port, kMiso_Pin);
            HAL_GPIO_WritePin(kSClk_Port, kSClk_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(kSClk_Port, kSClk_Pin, GPIO_PIN_RESET);
            if (nState)
                nAdcVal |= 0x01;
        }            
        HAL_GPIO_WritePin(kAdcCS_Port, kAdcCS_Pin, GPIO_PIN_SET);  //deassert chip select
*/
        return (nAdcVal >> 7) & 0xFFFF;
    }

    
protected:
  
private:
    uint32_t            _nLedStateMsk;
//    SPI_HandleTypeDef   _hSpi;
};

#endif // __OpticsDriver_H
