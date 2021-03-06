/*  SDK Included Files */
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_i2c.h"
#include "fsl_tpm.h"
#include <MKL25Z4.h>
#include <stdlib.h>

#include "pin_mux.h"
#include "fsl_gpio.h"
#include "fsl_port.h"
#include "utils.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
/* I2C source clock */
#define ACCEL_I2C_CLK_SRC I2C0_CLK_SRC
#define ACCEL_I2C_CLK_FREQ CLOCK_GetFreq(I2C0_CLK_SRC)
#define I2C_BAUDRATE 100000U

#define I2C_RELEASE_SDA_PORT PORTE
#define I2C_RELEASE_SCL_PORT PORTE
#define I2C_RELEASE_SDA_GPIO GPIOE
#define I2C_RELEASE_SDA_PIN 25U
#define I2C_RELEASE_SCL_GPIO GPIOE
#define I2C_RELEASE_SCL_PIN 24U
#define I2C_RELEASE_BUS_COUNT 100U
#define I2C_BAUDRATE 100000U
#define FXOS8700_WHOAMI 0xC7U
#define MMA8451_WHOAMI 0x1AU
#define ACCEL_STATUS 0x00U
#define ACCEL_XYZ_DATA_CFG 0x0EU
#define ACCEL_CTRL_REG1 0x2AU
/* FXOS8700 and MMA8451 have the same who_am_i register address. */
#define ACCEL_WHOAMI_REG 0x0DU
#define ACCEL_READ_TIMES 10U

/*******************************************************************************
 * Definitions p2
 ******************************************************************************/
#define BOARD_TPM_BASEADDR TPM2
#define BOARD_TPM_CHANNEL1 1U
#define BOARD_TPM_CHANNEL0 0U

#define BOARD_TPM_BASEADDR2 TPM0

/* Get source clock for TPM driver */
#define TPM_SOURCE_CLOCK CLOCK_GetFreq(kCLOCK_PllFllSelClk)

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static bool I2C_ReadAccelWhoAmI(void);
static bool I2C_WriteAccelReg(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value);
static bool I2C_ReadAccelRegs(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize);

/*******************************************************************************
 * Variables
 ******************************************************************************/

/*  FXOS8700 and MMA8451 device address */
const uint8_t g_accel_address[] = {0x1CU, 0x1DU, 0x1EU, 0x1FU};

i2c_master_handle_t g_m_handle;

uint8_t g_accel_addr_found = 0x00;

volatile bool completionFlag = false;
volatile bool nakFlag = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

static void i2c_release_bus_delay(void)
{
    uint32_t i = 0;
    for (i = 0; i < I2C_RELEASE_BUS_COUNT; i++)
    {
        __NOP();
    }
}

void BOARD_I2C_ReleaseBus(void)
{
    uint8_t i = 0;
    gpio_pin_config_t pin_config;
    port_pin_config_t i2c_pin_config = {0};

    /* Config pin mux as gpio */
    i2c_pin_config.pullSelect = kPORT_PullUp;
    i2c_pin_config.mux = kPORT_MuxAsGpio;

    pin_config.pinDirection = kGPIO_DigitalOutput;
    pin_config.outputLogic = 1U;
    CLOCK_EnableClock(kCLOCK_PortE);
    PORT_SetPinConfig(I2C_RELEASE_SCL_PORT, I2C_RELEASE_SCL_PIN, &i2c_pin_config);
    PORT_SetPinConfig(I2C_RELEASE_SDA_PORT, I2C_RELEASE_SDA_PIN, &i2c_pin_config);

    GPIO_PinInit(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, &pin_config);
    GPIO_PinInit(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, &pin_config);

    /* Drive SDA low first to simulate a start */
    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    /* Send 9 pulses on SCL and keep SDA low */
    for (i = 0; i < 9; i++)
    {
        GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
        i2c_release_bus_delay();

        GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
        i2c_release_bus_delay();

        GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
        i2c_release_bus_delay();
        i2c_release_bus_delay();
    }

    /* Send stop */
    GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 0U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SCL_GPIO, I2C_RELEASE_SCL_PIN, 1U);
    i2c_release_bus_delay();

    GPIO_WritePinOutput(I2C_RELEASE_SDA_GPIO, I2C_RELEASE_SDA_PIN, 1U);
    i2c_release_bus_delay();
}

static void i2c_master_callback(I2C_Type *base, i2c_master_handle_t *handle, status_t status, void *userData)
{
    /* Signal transfer success when received success status. */
    if (status == kStatus_Success)
    {
        completionFlag = true;
    }
    /* Signal transfer success when received success status. */
    if ((status == kStatus_I2C_Nak) || (status == kStatus_I2C_Addr_Nak))
    {
        nakFlag = true;
    }
}

static bool I2C_ReadAccelWhoAmI(void)
{
    /*
    How to read the device who_am_I value ?
    Start + Device_address_Write , who_am_I_register;
    Repeart_Start + Device_address_Read , who_am_I_value.
    */
    uint8_t who_am_i_reg = ACCEL_WHOAMI_REG;
    uint8_t who_am_i_value = 0x00;
    uint8_t accel_addr_array_size = 0x00;
    bool find_device = false;
    uint8_t i = 0;
    uint32_t sourceClock = 0;

    i2c_master_config_t masterConfig;

    /*
     * masterConfig.baudRate_Bps = 100000U;
     * masterConfig.enableStopHold = false;
     * masterConfig.glitchFilterWidth = 0U;
     * masterConfig.enableMaster = true;
     */
    I2C_MasterGetDefaultConfig(&masterConfig);

    masterConfig.baudRate_Bps = I2C_BAUDRATE;

    sourceClock = ACCEL_I2C_CLK_FREQ;

    I2C_MasterInit(BOARD_ACCEL_I2C_BASEADDR, &masterConfig, sourceClock);

    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = g_accel_address[0];
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = 0;
    masterXfer.subaddressSize = 0;
    masterXfer.data = &who_am_i_reg;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferNoStopFlag;

    accel_addr_array_size = sizeof(g_accel_address) / sizeof(g_accel_address[0]);

    for (i = 0; i < accel_addr_array_size; i++)
    {
        masterXfer.slaveAddress = g_accel_address[i];

        I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

        /*  wait for transfer completed. */
        while ((!nakFlag) && (!completionFlag))
        {
        }

        nakFlag = false;

        if (completionFlag == true)
        {
            completionFlag = false;
            find_device = true;
            g_accel_addr_found = masterXfer.slaveAddress;
            break;
        }
    }

    if (find_device == true)
    {
        masterXfer.direction = kI2C_Read;
        masterXfer.subaddress = 0;
        masterXfer.subaddressSize = 0;
        masterXfer.data = &who_am_i_value;
        masterXfer.dataSize = 1;
        masterXfer.flags = kI2C_TransferRepeatedStartFlag;

        I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

        /*  wait for transfer completed. */
        while ((!nakFlag) && (!completionFlag))
        {
        }

        nakFlag = false;

        if (completionFlag == true)
        {
            completionFlag = false;
            if (who_am_i_value == FXOS8700_WHOAMI)
            {
                PRINTF("Found an FXOS8700 on board , the device address is 0x%x . \r\n", masterXfer.slaveAddress);
                return true;
            }
            else if (who_am_i_value == MMA8451_WHOAMI)
            {
                PRINTF("Found an MMA8451 on board , the device address is 0x%x . \r\n", masterXfer.slaveAddress);
                return true;
            }
            else
            {
                PRINTF("Found a device, the WhoAmI value is 0x%x\r\n", who_am_i_value);
                PRINTF("It's not MMA8451 or FXOS8700. \r\n");
                PRINTF("The device address is 0x%x. \r\n", masterXfer.slaveAddress);
                return false;
            }
        }
        else
        {
            PRINTF("Not a successful i2c communication \r\n");
            return false;
        }
    }
    else
    {
        PRINTF("\r\n Do not find an accelerometer device ! \r\n");
        return false;
    }
}

static bool I2C_WriteAccelReg(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t value)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));

    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Write;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = &value;
    masterXfer.dataSize = 1;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;

    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}

static bool I2C_ReadAccelRegs(I2C_Type *base, uint8_t device_addr, uint8_t reg_addr, uint8_t *rxBuff, uint32_t rxSize)
{
    i2c_master_transfer_t masterXfer;
    memset(&masterXfer, 0, sizeof(masterXfer));
    masterXfer.slaveAddress = device_addr;
    masterXfer.direction = kI2C_Read;
    masterXfer.subaddress = reg_addr;
    masterXfer.subaddressSize = 1;
    masterXfer.data = rxBuff;
    masterXfer.dataSize = rxSize;
    masterXfer.flags = kI2C_TransferDefaultFlag;

    /*  direction=write : start+device_write;cmdbuff;xBuff; */
    /*  direction=recive : start+device_write;cmdbuff;repeatStart+device_read;xBuff; */

    I2C_MasterTransferNonBlocking(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, &masterXfer);

    /*  wait for transfer completed. */
    while ((!nakFlag) && (!completionFlag))
    {
    }

    nakFlag = false;

    if (completionFlag == true)
    {
        completionFlag = false;
        return true;
    }
    else
    {
        return false;
    }
}
/*!
 * @brief Main function
 * This function reads the accelerator values (after initializing the I2C communication) and updating the LED
 * duty cycles based on those values.
 */
int main(void)
{
	//LED Initializes
    bool isThereAccel = false;
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_I2C_ReleaseBus();
    BOARD_I2C_ConfigurePins();
    BOARD_InitDebugConsole();
    //LED_Initialize();


    /********************************************************/
    //TPM parameter declarations
    tpm_config_t tpmInfo;
    tpm_config_t tpmInfo2;
    tpm_chnl_pwm_signal_param_t tpmParam1;
    tpm_chnl_pwm_signal_param_t tpmParam2;
    tpm_chnl_pwm_signal_param_t tpmParam3;
    tpm_pwm_level_select_t pwmLevel = kTPM_LowTrue;

    /* Configure tpm params with frequency 24kHZ */
    tpmParam1.chnlNumber = (tpm_chnl_t)BOARD_TPM_CHANNEL1;
    tpmParam1.level = pwmLevel;
    tpmParam1.dutyCyclePercent = 10U;

    tpmParam2.chnlNumber = (tpm_chnl_t)BOARD_TPM_CHANNEL0;
    tpmParam2.level = pwmLevel;
    tpmParam2.dutyCyclePercent = 10U;

    tpmParam3.chnlNumber = (tpm_chnl_t)BOARD_TPM_CHANNEL1;
    tpmParam3.level = pwmLevel;
    tpmParam3.dutyCyclePercent = 10U;


    /* Select the clock source for the TPM counter as kCLOCK_PllFllSelClk */
    CLOCK_SetTpmClock(1U);

    TPM_GetDefaultConfig(&tpmInfo);
    TPM_GetDefaultConfig(&tpmInfo2);
        /* Initialize TPM module */
    TPM_Init(BOARD_TPM_BASEADDR, &tpmInfo);
    TPM_Init(BOARD_TPM_BASEADDR2, &tpmInfo2);

    TPM_SetupPwm(BOARD_TPM_BASEADDR, &tpmParam1, 1U, kTPM_CenterAlignedPwm, 24000U, TPM_SOURCE_CLOCK);
    TPM_SetupPwm(BOARD_TPM_BASEADDR, &tpmParam2, 1U, kTPM_CenterAlignedPwm, 24000U, TPM_SOURCE_CLOCK);
    TPM_SetupPwm(BOARD_TPM_BASEADDR2, &tpmParam3, 1U, kTPM_CenterAlignedPwm, 24000U, TPM_SOURCE_CLOCK);

    TPM_StartTimer(BOARD_TPM_BASEADDR, kTPM_SystemClock);
    TPM_StartTimer(BOARD_TPM_BASEADDR2, kTPM_SystemClock);
    /*******************************************************/


    PRINTF("\r\nI2C example -- Read Accelerometer Value\r\n");

    I2C_MasterTransferCreateHandle(BOARD_ACCEL_I2C_BASEADDR, &g_m_handle, i2c_master_callback, NULL);
    isThereAccel = I2C_ReadAccelWhoAmI();

    /*  read the accel xyz value if there is accel device on board */
    if (true == isThereAccel)
    {
        uint8_t databyte = 0;
        uint8_t write_reg = 0;
        uint8_t readBuff[7];
        int16_t x, y, z;
        uint8_t status0_value = 0;
        uint32_t i = 0U;

        /*  please refer to the "example FXOS8700CQ Driver Code" in FXOS8700 datasheet. */
        /*  write 0000 0000 = 0x00 to accelerometer control register 1 */
        /*  standby */
        /*  [7-1] = 0000 000 */
        /*  [0]: active=0 */
        write_reg = ACCEL_CTRL_REG1;
        databyte = 0;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);

        /*  write 0000 0001= 0x01 to XYZ_DATA_CFG register */
        /*  [7]: reserved */
        /*  [6]: reserved */
        /*  [5]: reserved */
        /*  [4]: hpf_out=0 */
        /*  [3]: reserved */
        /*  [2]: reserved */
        /*  [1-0]: fs=01 for accelerometer range of +/-4g range with 0.488mg/LSB */
        /*  databyte = 0x01; */
        write_reg = ACCEL_XYZ_DATA_CFG;
        databyte = 0x01;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);
        /*  write 0000 1101 = 0x0D to accelerometer control register 1 */
        /*  [7-6]: aslp_rate=00 */
        /*  [5-3]: dr=001 for 200Hz data rate (when in hybrid mode) */
        /*  [2]: lnoise=1 for low noise mode */
        /*  [1]: f_read=0 for normal 16 bit reads */
        /*  [0]: active=1 to take the part out of standby and enable sampling */
        /*   databyte = 0x0D; */
        write_reg = ACCEL_CTRL_REG1;
        databyte = 0x0d;
        I2C_WriteAccelReg(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, write_reg, databyte);
        PRINTF("The accel values:\r\n");
        while(1)
        {
            status0_value = 0;
            /*  wait for new data are ready. */
            while (status0_value != 0xff)
            {
                I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, ACCEL_STATUS, &status0_value, 1);
            }

            /*  Multiple-byte Read from STATUS (0x00) register */
            I2C_ReadAccelRegs(BOARD_ACCEL_I2C_BASEADDR, g_accel_addr_found, ACCEL_STATUS, readBuff, 7);

            status0_value = readBuff[0];
            //formats x, y, z to be a 16 bit integer with a shifted 8 bit OR'd with an 8 bit
            x = ((int16_t)(((readBuff[1] * 256U) | readBuff[2]))) / 4U;
            y = ((int16_t)(((readBuff[3] * 256U) | readBuff[4]))) / 4U;
            z = ((int16_t)(((readBuff[5] * 256U) | readBuff[6]))) / 4U;


            PRINTF("%5d\n",x);
            /*
             * discrete normalization of x,y,z to fit iin the desired PWM range.
             */
            if (x > 1000) x = 1000;
            else if (x < -1000) x = -1000;
            x += 1000;
            x /= 20;

            if (y > 1000) y = 1000;
            else if (y < -1000) y = -1000;
            y += 1000;
            y /= 20;

            if (z > 1000) z = 1000;
            else if (z < -1000) z = -1000;
            z += 1000;
            z /= 20;
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL1, 0U);
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL0, 0U);
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR2, (tpm_chnl_t)BOARD_TPM_CHANNEL1, 0U);


             /* Update PWM duty cycle */
            TPM_UpdatePwmDutycycle(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL1, kTPM_CenterAlignedPwm,x);
            TPM_UpdatePwmDutycycle(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL0, kTPM_CenterAlignedPwm,y);
            TPM_UpdatePwmDutycycle(BOARD_TPM_BASEADDR2, (tpm_chnl_t)BOARD_TPM_CHANNEL1, kTPM_CenterAlignedPwm,z);



            /* Start channel output with updated dutycycle */
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL1, pwmLevel);
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR, (tpm_chnl_t)BOARD_TPM_CHANNEL0, pwmLevel);
            TPM_UpdateChnlEdgeLevelSelect(BOARD_TPM_BASEADDR2, (tpm_chnl_t)BOARD_TPM_CHANNEL1, pwmLevel);
        }
    }

    PRINTF("\r\nEnd of I2C example .\r\n");
    while (1)
    {
    }
}
