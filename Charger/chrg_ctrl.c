/**
  ******************************************************************************
  * @file    chrg_ctrl.c
  * @author  Benedek Kupper
  * @version 1.0
  * @date    2018-03-10
  * @brief   DebugDongle charger controls implementation
  *
  * Copyright (c) 2018 Benedek Kupper
  *
  * Licensed under the Apache License, Version 2.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.apache.org/licenses/LICENSE-2.0
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */
#include <chrg_ctrl.h>
#include <bsp_io.h>

static ChargeCurrentType currentLimit = Ichg_100mA;
static ChargeCurrentType currentConfig = Ichg_0mA;

static void Charger_onSwitchChange(uint32_t x);

/**
 * @brief Initializes the hardware control of the battery charger IC.
 */
void Charger_Init(void)
{
    /* nPWR default: use as input */
    GPIO_vInitPin (USB_PWR_PIN, USB_PWR_CFG);

    /* TS default: drive 1 to enable charging */
    GPIO_vInitPin (CHARGER_CTRL_PIN, CHARGER_CTRL_CFG);

    /* ISET2 default: float to limit charging to 100mA */
    Charger_SetCurrent(Ichg_100mA);

    /* nCHG default: use as input */
    GPIO_vInitPin (CHARGER_STATUS_PIN, CHARGER_STATUS_CFG);

    /* User LED */
    GPIO_vInitPin (USER_LED_PIN, USER_LED_CFG);
    GPIO_vWritePin(USER_LED_PIN, 1);

#if (HW_REV > 0xA)
    /* Vout default: input */
    GPIO_vInitPin (VOUT_SELECT_PIN, VOUT_SELECT_IN_CFG);
    *GPIO_pxPinCallback(VOUT_SELECT_PIN) = Charger_onSwitchChange;
#else
    GPIO_vInitPin (VOUT_SELECT_PIN, VOUT_SELECT_OUT_CFG);

    /* Switch controls Vout as long as USB is not configured */
    GPIO_vInitPin (MODE_SWITCH_PIN, MODE_SWITCH_CFG);
    *GPIO_pxPinCallback(MODE_SWITCH_PIN) = Charger_onSwitchChange;
#endif
    NVIC_EnableIRQ(IRQN(VOUT_SELECT));

    /* Apply switch configuration now */
    Charger_onSwitchChange(VOUT_SELECT_LINE);
}

/**
 * @brief Sets the User LED according to the new state of the mode switch.
 * @param x: unused
 */
static void Charger_onSwitchChange(uint32_t x)
{
#if (HW_REV > 0xA)
    GPIO_vWritePin(USER_LED_PIN, GPIO_eReadPin(VOUT_SELECT_PIN));
#else
    Output_SetVoltage(1 - GPIO_eReadPin(MODE_SWITCH_PIN));
#endif
}

/**
 * @brief Returns the measured battery charge current.
 * @return The charge current in mA
 */
int Charger_GetCurrent_mA(void)
{
    if (GPIO_eReadPin(CHARGER_STATUS_PIN) != 0)
    {
        /* nCHG is high, charging is complete/stopped */
        return 0;
    }
    else
    {
        return Analog_GetValues()->Ichrg_mA;
    }
}

/**
 * @brief Returns the measured battery (charge) voltage.
 * @return The battery voltage in mV
 */
int Charger_GetVoltage_mV(void)
{
    return Analog_GetValues()->Vbat_mV;
}

/**
 * @brief Handles the activation of the charger USB interface:
 *         - Enables analog conversions
 *         - Disables the switch control of the Output voltage
 */
void Charger_SetConfig(void)
{
    if (currentLimit < Ichg_500mA)
    {
        currentLimit = Ichg_500mA;
    }
    Analog_Resume();
#if (HW_REV == 0xA)
    NVIC_DisableIRQ(IRQN(VOUT_SELECT));
#endif
}

/**
 * @brief Handles the deactivation of the charger USB interface:
 *         - Disables analog conversions
 *         - Disables the switch control of the Output voltage
 */
void Charger_ClearConfig(void)
{
    Analog_Halt();
#if (HW_REV > 0xA)
    Analog_IoutConfig(ENABLE);
    GPIO_vInitPin (VOUT_SELECT_PIN, VOUT_SELECT_IN_CFG);
#endif
    NVIC_EnableIRQ(IRQN(VOUT_SELECT));
}

/**
 * @brief Sets the battery current limit based on the USB downstream port type.
 * @param UsbCharger: the type of the connected USB port
 */
void Charger_SetType(USB_ChargerType UsbCharger)
{
    switch (UsbCharger)
    {
        case USB_BCD_DEDICATED_CHARGING_PORT:
        case USB_BCD_CHARGING_DOWNSTREAM_PORT:
        {
            currentLimit = Ichg_800mA;
            break;
        }
        default:
        {
            if (currentLimit > Ichg_100mA)
            {
                Charger_SetCurrent(currentLimit);
            }
            currentLimit = Ichg_100mA;
            break;
        }
    }
}

/**
 * @brief Sets the new current level on the charger IC.
 * @param CurrentLevel: the selected current level
 */
void Charger_SetCurrent(ChargeCurrentType CurrentLevel)
{
    switch (CurrentLevel)
    {
        case Ichg_0mA:
            /* Setting TS pin to low disables charging */
            GPIO_vWritePin (CHARGER_CTRL_PIN, 0);
            break;

        case Ichg_100mA:
            GPIO_vWritePin (CHARGER_CTRL_PIN, 1);
            /* Float ISET2 pin to set 100mA current */
            GPIO_vDeinitPin(CHARGER_CURRENT_PIN);
            break;

        case Ichg_500mA:
            GPIO_vWritePin(CHARGER_CTRL_PIN, 1);
            /* Pull high ISET2 pin to set 500mA current */
            GPIO_vInitPin (CHARGER_CURRENT_PIN, CHARGER_CURRENT_CFG);
            GPIO_vWritePin(CHARGER_CURRENT_PIN, 1);
            break;

        case Ichg_800mA:
            GPIO_vWritePin(CHARGER_CTRL_PIN, 1);
            /* Pull low ISET2 pin to set current according to ISET (800mA here) */
            GPIO_vInitPin (CHARGER_CURRENT_PIN, CHARGER_CURRENT_CFG);
            GPIO_vWritePin(CHARGER_CURRENT_PIN, 0);
            break;
    }
    currentConfig = CurrentLevel;
}

/**
 * @brief Sets the Output voltage.
 * @param Voltage: the new voltage to provide
 */
void Output_SetVoltage(OutputVoltageType Voltage)
{
#if (HW_REV > 0xA)
    NVIC_DisableIRQ(IRQN(VOUT_SELECT));

    if (Voltage != Vout_off)
    {
        GPIO_vWritePin(IOUT_PIN, 0);
        Analog_IoutConfig(ENABLE);
        GPIO_vWritePin(USER_LED_PIN, 2 - Voltage);
        GPIO_vInitPin (VOUT_SELECT_PIN, VOUT_SELECT_OUT_CFG);
        GPIO_vWritePin(VOUT_SELECT_PIN, Voltage - 1);
    }
    else
    {
        Analog_IoutConfig(DISABLE);
        GPIO_vWritePin(USER_LED_PIN, 1);
        GPIO_vInitPin (IOUT_PIN, IOUT_CTRL_CFG);
        GPIO_vWritePin(IOUT_PIN, 1);
    }
#else
    GPIO_vWritePin(USER_LED_PIN, 1 - Voltage);
    GPIO_vWritePin(VOUT_SELECT_PIN, Voltage);
#endif
}

/**
 * @brief Determines the current output voltage configuration.
 * @return The current output voltage setting
 */
OutputVoltageType Output_GetVoltage(void)
{
    OutputVoltageType Voltage;
#if (HW_REV > 0xA)
    if (GPIO_eReadPin(IOUT_PIN))
    {
        Voltage = Vout_off;
    }
    else
#endif
    if (GPIO_eReadPin(VOUT_SELECT_PIN))
    {
        Voltage = Vout_5V;
    }
    else
    {
        Voltage = Vout_3V3;
    }
    return Voltage;
}

/**
 * @brief Determines USB power connection state.
 * @return true if USB power is present, false if not
 */
bool Charger_UsbPowerPresent(void)
{
    return !GPIO_eReadPin(USB_PWR_PIN);
}

/**
 * @brief Disable power outputs.
 */
void Charger_Suspend(void)
{
    GPIO_vWritePin (CHARGER_CTRL_PIN, 0);
    Charger_ClearConfig();
}

/**
 * @brief Restore power outputs to their active state.
 */
void Charger_Resume(void)
{
    Charger_SetCurrent(currentConfig);
    Charger_SetConfig();
}
