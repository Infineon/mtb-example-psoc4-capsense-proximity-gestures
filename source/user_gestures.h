/******************************************************************************
 * File Name:   user_gestures.h
 *
 * Description: This file contains all the function prototypes of
 *              gestures functions.
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
#ifndef SOURCE_USER_GESTURES_H_
#define SOURCE_USER_GESTURES_H_

/*******************************************************************************
 * Include header files
 ******************************************************************************/
#include "cy_pdl.h"
#include "cybsp.h"
#include "cycfg.h"
#include "cycfg_capsense.h"


/*******************************************************************************
 * Macros
 *******************************************************************************/
#define TURN_LED_OFF                     (1u)
#define TURN_LED_ON                      (0u)
#define GESTURE_STATE_DEBOUNCE           (1u)

#define LED_ON_TIME_LOOPS                (5u)

/* Total count of LEDs in each axis */
#define LED_COUNT                        (3u)

/* Macros for LED names */
#define FIRST_LED                        (0u)
#define SECOND_LED                       (1u)
#define THIRD_LED                        (2u)

/* Macro used to clear the LED loop time counter */
#define RESET_COUNTER                    (0u)

/* This macro determines how the LEDs are driven when a gesture is detected */
#define LED_DRIVE_DURING_GESTURE         (0u)
#define LED_DRIVE_AFTER_GESTURE          (1u)
#define LED_DRIVE_SEQUENCE               (LED_DRIVE_DURING_GESTURE)

/* This macro determines the type of gesture detected by the device */
#define XAXIS                            (1u)
#define YAXIS                            (0u)
#define GESTURE_AXIS                     (XAXIS)

/*******************************************************************************
 *   Data Type Definitions
 *******************************************************************************/
/* Enum used for different states in state machine */
typedef enum
{
    ZONE_ONE        = 0x01u,
    ZONE_TWO        = 0x02u,
    ZONE_THREE      = 0x03u,
    INVALID_ZONE    = 0xFFu
}ZONE_NAMES;

/* Enum used for sesnor numbering for Kit-24 */
typedef enum
{
    LEFT_SENSOR     = CY_CAPSENSE_PS1_WDGT_ID,
    RIGHT_SENSOR    = CY_CAPSENSE_PS2_WDGT_ID,
    BOTTOM_SENSOR   = CY_CAPSENSE_PS3_WDGT_ID,
    TOP_SENSOR      = CY_CAPSENSE_PS4_WDGT_ID
} SENSOR_NAMES;

/* Enum used to turn ON and OFF LEDs */
typedef enum
{
    LED_ON          = 0x00,
    LED_OFF         = 0x01
}LED_STATES;

/* Structure for gesture tracking */
typedef struct {
    /* Contains previous state data in state machine */
    ZONE_NAMES previous_zone;
    /* Contains the current state data in state machine */
    ZONE_NAMES current_zone;
    /* Used for tracking forward gesture */
    bool  forward_gesture_tracking;
    /* Indicates whether forward gesture is completed or not */
    bool  forward_gesture_complete;
    /* Used for tracking backward gesture */
    bool  backward_gesture_tracking;
    /* Indicates whether backward gesture is completed or not */
    bool  backward_gesture_complete;
    /* Debounce for states in forward direction */
    uint8_t forward_debounce_counter;
    /* Debounce for states in backward direction */
    uint8_t backward_debounce_counter;
}gesture;

typedef enum
{
    LEFT_TO_RIGHT         = 0x00u,
    RIGHT_TO_LEFT         = 0x01u,
    BOTTOM_TO_TOP         = 0x02u,
    TOP_TO_BOTTOM         = 0x03u,
    TURN_ALL_LEDS_OFF     = 0xFFu
} LED_DRIVE;


/*******************************************************************************
 * Function Prototypes
 *******************************************************************************/
void gesture_detection(SENSOR_NAMES, SENSOR_NAMES, gesture*);

/* API contains the common code in all the states for forward gesture */
void gesture_forward_detection(gesture*, ZONE_NAMES);

/* API contains the common code in all the states for backward gesture */
void gesture_backward_detection(gesture*, ZONE_NAMES);

/* Initialize the gesture structure with default values */
void gesture_variable_init(gesture*);

/* Drives LEDs based on the gestures and macro LED_DRIVE_AFTER_GESTURE */
void drive_led(ZONE_NAMES);

#endif /* SOURCE_USER_GESTURES_H_ */


/* [] END OF FILE */
