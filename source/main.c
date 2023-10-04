/******************************************************************************
 * File Name:   main.c
 *
 * Description: This is the source code for the PSoC 4: CAPSENSE Proximity
 * Gestures Example for ModusToolbox.
 *
 * Related Document: See README.md
 *
 *
 *******************************************************************************
 * Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
 * an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
 *
 * This software, including source code, documentation and related
 * materials ("Software") is owned by Cypress Semiconductor Corporation
 * or one of its affiliates ("Cypress") and is protected by and subject to
 * worldwide patent protection (United States and foreign),
 * United States copyright laws and international treaty provisions.
 * Therefore, you may use this Software only as provided in the license
 * agreement accompanying the software package from which you
 * obtained this Software ("EULA").
 * If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
 * non-transferable license to copy, modify, and compile the Software
 * source code solely for use in connection with Cypress's
 * integrated circuit products.  Any reproduction, modification, translation,
 * compilation, or representation of this Software except as specified
 * above is prohibited without the express written permission of Cypress.
 *
 * Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
 * reserves the right to make changes to the Software without notice. Cypress
 * does not assume any liability arising out of the application or use of the
 * Software or any product or circuit described in the Software. Cypress does
 * not authorize its products for use in any products where a malfunction or
 * failure of the Cypress product may reasonably be expected to result in
 * significant property damage, injury or death ("High Risk Product"). By
 * including Cypress's product in a High Risk Product, the manufacturer
 * of such system or application assumes all risk of such use and in doing
 * so agrees to indemnify Cypress against all liability.
 *******************************************************************************/


/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"
#include "user_gestures.h"


/*******************************************************************************
 * Macros
 *******************************************************************************/
#define CAPSENSE_MSC0_INTR_PRIORITY      (3u)
#define CAPSENSE_MSC1_INTR_PRIORITY      (3u)
#define CY_ASSERT_FAILED                 (0u)
#define MSC_CAPSENSE_WIDGET_INACTIVE     (0u)

/* EZI2C interrupt priority must be higher than CAPSENSE interrupt */
#define EZI2C_INTR_PRIORITY              (2u)


/*******************************************************************************
 *   Extern Variable and Constant Declarations
 *******************************************************************************/
/* Gesture structure instance for X-Axis gesture detection */
gesture x_axis;
/* Gesture structure instance for Y-Axis gesture detection */
gesture y_axis;

SENSOR_NAMES start_sensor_number;
SENSOR_NAMES end_sensor_number;

uint8_t sensor_number;

/*******************************************************************************
 * Global Definitions
 *******************************************************************************/
cy_stc_scb_ezi2c_context_t ezi2c_context;

/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
static void initialize_capsense(void);
static void capsense_msc0_isr(void);
static void capsense_msc1_isr(void);
static void ezi2c_isr(void);
static void initialize_capsense_tuner(void);


/*******************************************************************************
 * Function Name: main
 ********************************************************************************
 * Summary:
 *  System entrance point. This function performs
 *  - initial setup of device
 *  - initialize CAPSENSE
 *  - initialize tuner communication
 *  - scan touch input continuously
 *
 * Return:
 *  int
 *
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

    /* Initialize the device and board peripherals */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize EZI2C */
    initialize_capsense_tuner();

    /* Initialize MSC CAPSENSE */
    initialize_capsense();

    /* Based on the board and gesture direction assign the proper sensor numbers */
#if(GESTURE_AXIS)
    start_sensor_number = LEFT_SENSOR;
    end_sensor_number = RIGHT_SENSOR;
#else
    start_sensor_number = BOTTOM_SENSOR;
    end_sensor_number = TOP_SENSOR;
#endif

    /* Start the first scan */
    Cy_CapSense_ScanAllSlots(&cy_capsense_context);

    for (;;)
    {
        for(sensor_number = start_sensor_number ;sensor_number <= end_sensor_number; sensor_number++)
        {
            /* Scan sensor */
            Cy_CapSense_ScanWidget(sensor_number, &cy_capsense_context);

            /* Wait till sensor is scanned */
            while(Cy_CapSense_IsBusy(&cy_capsense_context))
            {
            }

            /* Update the Sensor digital count reference */
            Cy_CapSense_UpdateWidgetBaseline(sensor_number, &cy_capsense_context);

            Cy_CapSense_ProcessWidget(sensor_number, &cy_capsense_context);
        }

        /* Detect x-axis gestures */
#if(GESTURE_AXIS)
        gesture_detection(start_sensor_number,end_sensor_number, &x_axis);
#else
        gesture_detection(start_sensor_number,end_sensor_number, &y_axis);
#endif

        /* Establishes synchronized communication with the CAPSENSE Tuner tool */
        Cy_CapSense_RunTuner(&cy_capsense_context);

        /* Start the next scan */
        Cy_CapSense_ScanAllSlots(&cy_capsense_context);

    }
}


/*******************************************************************************
 * Function Name: initialize_capsense
 ********************************************************************************
 * Summary:
 *  This function initializes the CAPSENSE and configures the CAPSENSE
 *  interrupt.
 *
 *******************************************************************************/
static void initialize_capsense(void)
{
    cy_capsense_status_t status = CY_CAPSENSE_STATUS_SUCCESS;

    /* CAPSENSE interrupt configuration MSC 0 */
    const cy_stc_sysint_t capsense_msc0_interrupt_config =
    {
        .intrSrc = CY_MSC0_IRQ,
        .intrPriority = CAPSENSE_MSC0_INTR_PRIORITY,
    };

    /* CAPSENSE interrupt configuration MSC 1 */
    const cy_stc_sysint_t capsense_msc1_interrupt_config =
    {
        .intrSrc = CY_MSC1_IRQ,
        .intrPriority = CAPSENSE_MSC1_INTR_PRIORITY,
    };

    /* Capture the MSC HW block and initialize it to the default state. */
    status = Cy_CapSense_Init(&cy_capsense_context);

    if (CY_CAPSENSE_STATUS_SUCCESS == status)
    {
        /* Initialize CAPSENSE interrupt for MSC 0 */
        Cy_SysInt_Init(&capsense_msc0_interrupt_config, capsense_msc0_isr);
        NVIC_ClearPendingIRQ(capsense_msc0_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc0_interrupt_config.intrSrc);

        /* Initialize CAPSENSE interrupt for MSC 1 */
        Cy_SysInt_Init(&capsense_msc1_interrupt_config, capsense_msc1_isr);
        NVIC_ClearPendingIRQ(capsense_msc1_interrupt_config.intrSrc);
        NVIC_EnableIRQ(capsense_msc1_interrupt_config.intrSrc);

        /* Initialize the CAPSENSE firmware modules. */
        status = Cy_CapSense_Enable(&cy_capsense_context);
    }

    if(status != CY_CAPSENSE_STATUS_SUCCESS)
    {
        /* This status could fail before tuning the sensors correctly.
         * Ensure that this function passes after the CAPSENSE sensors are tuned
         * as per procedure give in the Readme.md file */
    }
}


/*******************************************************************************
 * Function Name: capsense_msc0_isr
 ********************************************************************************
 * Summary:
 *  Function for handling interrupts from CAPSENSE MSC0 block.
 *
 *******************************************************************************/
static void capsense_msc0_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC0_HW, &cy_capsense_context);
}


/*******************************************************************************
 * Function Name: capsense_msc1_isr
 ********************************************************************************
 * Summary:
 *  Function for handling interrupts from CAPSENSE MSC1 block.
 *
 *******************************************************************************/
static void capsense_msc1_isr(void)
{
    Cy_CapSense_InterruptHandler(CY_MSC1_HW, &cy_capsense_context);
}


/*******************************************************************************
 * Function Name: initialize_capsense_tuner
 ********************************************************************************
 * Summary:
 * EZI2C module to communicate with the CAPSENSE Tuner tool.
 *
 *******************************************************************************/
static void initialize_capsense_tuner(void)
{
    cy_en_scb_ezi2c_status_t status = CY_SCB_EZI2C_SUCCESS;

    /* EZI2C interrupt configuration structure */
    const cy_stc_sysint_t ezi2c_intr_config =
    {
        .intrSrc = CYBSP_EZI2C_IRQ,
        .intrPriority = EZI2C_INTR_PRIORITY,
    };

    /* Initialize the EZI2C firmware module */
    status = Cy_SCB_EZI2C_Init(CYBSP_EZI2C_HW, &CYBSP_EZI2C_config, &ezi2c_context);

    if(status != CY_SCB_EZI2C_SUCCESS)
    {
        CY_ASSERT(CY_ASSERT_FAILED);
    }

    Cy_SysInt_Init(&ezi2c_intr_config, ezi2c_isr);
    NVIC_EnableIRQ(ezi2c_intr_config.intrSrc);

    /* Set the CAPSENSE data structure as the I2C buffer to be exposed to the
     * master on primary slave address interface. Any I2C host tools such as
     * the Tuner or the Bridge Control Panel can read this buffer but you can
     * connect only one tool at a time.
     */
    Cy_SCB_EZI2C_SetBuffer1(CYBSP_EZI2C_HW, (uint8_t *)&cy_capsense_tuner,
        sizeof(cy_capsense_tuner), sizeof(cy_capsense_tuner),
        &ezi2c_context);

    Cy_SCB_EZI2C_Enable(CYBSP_EZI2C_HW);
}


/*******************************************************************************
 * Function Name: ezi2c_isr
 ********************************************************************************
 * Summary:
 *  Function for handling interrupts from EZI2C block.
 *
 *******************************************************************************/
static void ezi2c_isr(void)
{
    Cy_SCB_EZI2C_Interrupt(CYBSP_EZI2C_HW, &ezi2c_context);
}


/* [] END OF FILE */
