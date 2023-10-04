/******************************************************************************
 * File Name:   user_gestures.c
 *
 * Description: This file contains required functions for detecting gestures.
 *
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

#include "user_gestures.h"


/*******************************************************************************
 * Global Definitions
 *******************************************************************************/
LED_DRIVE led_drive_sequence = TURN_ALL_LEDS_OFF;

/* LEC counter to track LED drive time after gesture complete */
uint8_t led_counter = RESET_COUNTER;

/*******************************************************************************
 * Function Name: gesture_detection
 ********************************************************************************
 * Summary:
 *  This function detects forward and backward gestures using two
 *  proximity sensors.
 *
 *******************************************************************************/
void gesture_detection(SENSOR_NAMES sensor1, SENSOR_NAMES sensor2, gesture* axis_ptr)
{
    ZONE_NAMES current_zone = INVALID_ZONE;
    /* This function has different cases based on the proximity sensor status
     *    and each state follow similar steps which are given below */
    /* Steps for any case
     *    1) Reset debounce counters on first entry to this state
     *    2) Update the current zone
     *    3) Based on current and previous zone decide if it is forward or backward gesture
     *    4) Apply debounce before taking final decision on gesture tracking
     *    5) Update the previous zone */

    /* sensor1 ON, sensor2 OFF , Zone 1 */
    if(Cy_CapSense_IsWidgetActive(sensor1, &cy_capsense_context) && (Cy_CapSense_IsWidgetActive(sensor2, &cy_capsense_context)==0))
    {
        current_zone = ZONE_ONE;
        /*Check forward and backward gesture in this zone*/
        gesture_forward_detection(axis_ptr, current_zone);
        gesture_backward_detection(axis_ptr, current_zone);
    }
    /* sensor1 OFF, sensor2 ON , Zone 3 */
    else if((Cy_CapSense_IsWidgetActive(sensor1, &cy_capsense_context) == 0) && Cy_CapSense_IsWidgetActive(sensor2, &cy_capsense_context))
    {
        current_zone = ZONE_THREE;
        /*Check forward and backward gesture in this zone*/
        gesture_forward_detection(axis_ptr, current_zone);
        gesture_backward_detection(axis_ptr, current_zone);
    }
    /* sensor1 ON, sensor2 ON , Zone 2 */
    else if(Cy_CapSense_IsWidgetActive(sensor1, &cy_capsense_context) && Cy_CapSense_IsWidgetActive(sensor2, &cy_capsense_context))
    {
        current_zone = ZONE_TWO;
        /* Check forward and backward gesture in this zone */
        gesture_forward_detection(axis_ptr, current_zone);
        gesture_backward_detection(axis_ptr, current_zone);
    }
    else
    {
        /* Both proximity sensors are OFF */
        if (axis_ptr->forward_gesture_tracking)
        {
            if (axis_ptr->previous_zone == ZONE_THREE)
            {
                /* If previous zone is ZONE_THREE (Only sensor2 is ON) then forward gesture is complete */
                axis_ptr->forward_gesture_complete = true;
                /* Decide the LED direction based on X-Axis or Y-Axis */
                if (sensor1 == LEFT_SENSOR)
                {
                    /* Based on sensors being used to detect X-Axis or Y-Axis is determined */
                    led_drive_sequence = LEFT_TO_RIGHT;
                }
                else
                {
                    led_drive_sequence = BOTTOM_TO_TOP;
                }
                /* Reset the LED on time counter to start the time */
                led_counter = RESET_COUNTER;
            }
        }

        if (axis_ptr->backward_gesture_tracking)
        {
            if (axis_ptr->previous_zone == ZONE_ONE)
            {
                /* If previous zone is ZONE_ONE (Only sensor1 is ON) then backward gesture is complete */
                axis_ptr->backward_gesture_complete = true;
                /* Decide the LED direction based on X-Axis or Y-Axis */
                if (sensor1 == LEFT_SENSOR)
                {
                    /* Based on sensors being used to detect X-Axis or Y-Axis is determined */
                    led_drive_sequence = RIGHT_TO_LEFT;
                }
                else
                {
                    led_drive_sequence = TOP_TO_BOTTOM;
                }
                led_counter = RESET_COUNTER;
            }
        }
        /* Reset the gesture to start tracking the next gestures */
        gesture_variable_init(axis_ptr);
    }
    /* Drive LEDs */
    drive_led(current_zone);
}


/*******************************************************************************
 * Function Name: gesture_forward_detection
 ********************************************************************************
 * Summary:
 *  This function is written to prevent code duplication for forward gesture
 *    tracking. If the difference between the current zone and previous zone
 *    is >1 then the gesture tracking is turned OFF, Debounce for each state is
 *  also applied.
 *
 *******************************************************************************/
void gesture_forward_detection(gesture* axis_ptr, ZONE_NAMES current_zone)
{
    bool state_condition = false;
    /* First entry into this state reset the state debounce counters */
    if(axis_ptr->current_zone != current_zone)
    {
        axis_ptr->forward_debounce_counter = GESTURE_STATE_DEBOUNCE;
        axis_ptr->backward_debounce_counter = GESTURE_STATE_DEBOUNCE;
    }
    /* Update current zone, previous and current zone will be used to track
      whether gesture is forward or backward */
    axis_ptr->current_zone = current_zone;

    if (axis_ptr->forward_gesture_tracking)
    {
        if (axis_ptr->previous_zone != axis_ptr->current_zone)
        {
            if (axis_ptr->current_zone - axis_ptr->previous_zone != 1)
            {
                axis_ptr->forward_gesture_tracking = false;
            }
        }
    }
    /* Track forward gesture */
    /* Condition = Previously no signal on both prox sensor, currently only signal on sensor1 for first zone
    For other Zones current - previous should be equal to one to continue tracking gestures */
    if(current_zone == ZONE_ONE)
    {
        if(INVALID_ZONE == axis_ptr->previous_zone)
        {
            state_condition = true;
        }
    }
    else
    {
        state_condition = axis_ptr->forward_gesture_tracking;
    }

    if(state_condition)
    {
        /* Apply state debounce */
        if(axis_ptr->forward_debounce_counter >0)
        {
            axis_ptr->forward_debounce_counter--;
        }
    }
    else
    {
        /* Reset debounce counter for invalid condition */
        axis_ptr->forward_debounce_counter = GESTURE_STATE_DEBOUNCE;
    }

    /* Condition valid for forward gesture for debounce time, record gesture tracking */
    if(axis_ptr->forward_debounce_counter == 0)
    {
        axis_ptr->previous_zone = current_zone;
        if(current_zone == ZONE_ONE)
        {
            /* Valid zone signal conditions for debounce time start tracking the forward gesture */
            axis_ptr->forward_gesture_tracking = true;
        }
    }
}


/*******************************************************************************
 * Function Name: gesture_backward_detection
 ********************************************************************************
 * Summary:
 *  This function is written to prevent code duplication for backward gesture
 *  tracking. If the difference between the previous zone and current zone
 *  is >1 then the gesture tracking is turned OFF.
 *
 *******************************************************************************/
void gesture_backward_detection(gesture* axis_ptr, ZONE_NAMES current_zone)
{
    bool state_condition = false;

    if (axis_ptr->backward_gesture_tracking)
    {
        if (axis_ptr->previous_zone != axis_ptr->current_zone)
        {
            if (axis_ptr->previous_zone - axis_ptr->current_zone != 1)
            {
                axis_ptr->backward_gesture_tracking = false;
            }
        }
    }

    /* Track backward gesture */
    /* Condition = Previously no signal on both prox sensor, currently only signal on sensor2 for first zone
    For other Zones previous - cureent should be equal to one to continue tracking gestures */
    if(current_zone == ZONE_THREE)
    {
        if(INVALID_ZONE == axis_ptr->previous_zone)
        {
            state_condition = true;
        }
    }
    else
    {
        state_condition = axis_ptr->backward_gesture_tracking;
    }

    /* Debounce check for backward gesture in this state */
    if(state_condition)
    {
        if(axis_ptr->backward_debounce_counter >0)
        {
            axis_ptr->backward_debounce_counter--;
        }
    }
    else
    {
        axis_ptr->backward_debounce_counter = GESTURE_STATE_DEBOUNCE;
    }
    /* Valid backward gesture detected for debounce times */
    if(axis_ptr->backward_debounce_counter == 0)
    {
        axis_ptr->previous_zone = current_zone;
        if(current_zone == ZONE_THREE)
        {
            axis_ptr->backward_gesture_tracking = true;
        }
    }
}


/*******************************************************************************
 * Function Name: gesture_detection
 ********************************************************************************
 * Summary:
 *  All the gesture tracking variables are initialized to default values.
 *
 *******************************************************************************/
void gesture_variable_init(gesture* axis_ptr)
{
    /* Set the current and previous zone to invalid */
    axis_ptr->previous_zone = INVALID_ZONE;
    axis_ptr->current_zone = INVALID_ZONE;
    /* Turn OFF forward tracking and tracking complete variables */
    axis_ptr->forward_gesture_tracking = false;
    axis_ptr->forward_gesture_complete = false;
    /* Turn OFF backward tracking and tracking complete variables */
    axis_ptr->backward_gesture_tracking = false;
    axis_ptr->backward_gesture_complete = false;
    /* Reset X-Axis and Y-Axis debounce counters */
    axis_ptr->forward_debounce_counter = GESTURE_STATE_DEBOUNCE;
    axis_ptr->backward_debounce_counter = GESTURE_STATE_DEBOUNCE;
}


/*******************************************************************************
 * Function Name: drive_led
 ********************************************************************************
 * Summary:
 *  Turning LEDs ON/OFF based on the gestures detected.
 *
 *******************************************************************************/
void drive_led(ZONE_NAMES current_zone)
{
    /* Turn off all the LEDs and based on conditions turn ON required before driving LEDs */
    LED_STATES LED1 = LED_OFF;
    LED_STATES LED2 = LED_OFF;
    LED_STATES LED3 = LED_OFF;
    LED_STATES LED4 = LED_OFF;
    LED_STATES LED5 = LED_OFF;

#if(LED_DRIVE_SEQUENCE)
    {
        /* Three LEDs are used as visual feedback in X direction and three in Y direction
        LEDs are driven in the same direction as gesture
        LED1, LED2 and LED3 are placed from left to right
        LED5, LED2 and LED4 are placed from bottom to top
        Each LED is driven for 'N' loop times as configured in the macro */

        switch(led_drive_sequence)
        {
        case LEFT_TO_RIGHT:
        {
            /* Left to Right gesture is detected turn ON LEDs one after the other in the same order */
            if ((led_counter / LED_ON_TIME_LOOPS) == FIRST_LED)
            {
                LED1 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == SECOND_LED)
            {
                LED2 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == THIRD_LED)
            {
                LED3 = LED_ON;
            }
            break;
        }
        case RIGHT_TO_LEFT:
        {
            /* Right to Left gesture is detected turn ON LEDs one after the other in the same order */
            if ((led_counter / LED_ON_TIME_LOOPS) == FIRST_LED)
            {
                LED3 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == SECOND_LED)
            {
                LED2 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == THIRD_LED)
            {
                LED1 = LED_ON;
            }
            break;
        }
        case BOTTOM_TO_TOP:
        {
            /* Bottom to Top gesture is detected turn ON LEDs one after the other in the same order */
            if ((led_counter / LED_ON_TIME_LOOPS) == FIRST_LED)
            {
                LED5 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == SECOND_LED)
            {
                LED2 = LED_ON;
            }
            else if ((led_counter / LED_ON_TIME_LOOPS) == THIRD_LED)
            {
                LED4 = LED_ON;
            }
            break;
        }
        case TOP_TO_BOTTOM:
        {
            /* Top to Bottom gesture is detected turn ON LEDs one after the other in the same order */
            if ((led_counter / LED_ON_TIME_LOOPS) == FIRST_LED)
            {
                LED4 = LED_ON;
            }
            if ((led_counter / LED_ON_TIME_LOOPS) == SECOND_LED)
            {
                LED2 = LED_ON;
            }
            if ((led_counter / LED_ON_TIME_LOOPS) == THIRD_LED)
            {
                LED5 = LED_ON;
            }
            break;
        }
        default:
        {
            LED1 = LED_OFF;
            LED2 = LED_OFF;
            LED3 = LED_OFF;
            LED4 = LED_OFF;
            LED5 = LED_OFF;
        }
        }
        /* LED counter to track loop timings for LED drive */
        if (led_drive_sequence != TURN_ALL_LEDS_OFF)
        {
            led_counter++;
            if(led_counter >= LED_ON_TIME_LOOPS * LED_COUNT)
            {
                /* LED drive based on gesture is completed, change the LED sequence
                to turn OFF all the LEDs */
                led_drive_sequence = TURN_ALL_LEDS_OFF;
            }
        }
    }
#else
    {
        /* Three positions are detected using two proximity sensors
         *    LED1, LED2 and LED3 are the LEDs on the X-axis from left to right
         *    LED5, LED2 and LED4 are the LEDs on the Y-axis from bottom to top
         *    When X-axis gestures are being detected LEDs drive conditions are given below
         *    Position1 - left sensor ON and right sensor OFF - LED1 turned ON
         *    Position2 - left sensor ON and right sensor ON - LED2 turned ON
         *    Position3 - left sensor OFF and right sensor ON - LED3 turned ON
         *    When Y-axis gestures are being detected LEDs drive conditions are given below
         *    Position1 - bottom sensor ON and top sensor OFF - LED5 turned ON
         *    Position2 - bottom sensor ON and top sensor ON - LED2 turned ON
         *    Position3 - bottom sensor OFF and top sensor ON - LED4 turned ON */
        switch(current_zone)
        {
        case ZONE_ONE:
        {
#if(GESTURE_AXIS)
            {
                LED1 = LED_ON;
            }
#else
            {
                LED5 = LED_ON;
            }
#endif
            break;
        }
        case ZONE_TWO:
        {
            LED2 = LED_ON;
            break;
        }
        case ZONE_THREE:
        {
#if(GESTURE_AXIS)
            {
                LED3 = LED_ON;
            }

#else
            {
                LED4 = LED_ON;
            }
#endif
            break;
        }
        default:
        {
            LED1 = LED_OFF;
            LED2 = LED_OFF;
            LED3 = LED_OFF;
            LED4 = LED_OFF;
            LED5 = LED_OFF;
        }
        }
    }
#endif
    /* Drive all the LEDs */
    Cy_GPIO_Write(LED1_PORT, LED1_NUM, LED1);
    Cy_GPIO_Write(LED2_PORT, LED2_NUM, LED2);
    Cy_GPIO_Write(LED3_PORT, LED3_NUM, LED3);
    Cy_GPIO_Write(LED4_PORT, LED4_NUM, LED4);
    Cy_GPIO_Write(LED5_PORT, LED5_NUM, LED5);

}


/* [] END OF FILE */
