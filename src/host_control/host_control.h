
/**
 * \file host_conrtol.h 
 *  
 * \brief Header file for host control 
 *  
 * \author Benjamin Sanda 
 * \date 17 Feb 13 
 *  
 */

#ifndef HOST_CONTROL_H_
#define HOST_CONTROL_H_

/* General Defines */
#define BUTTON_PRESS_TIMEOUT    (10*ONE_SEC_MS)
#define LED_ON_DELAY            (8*ONE_SEC_MS)


/* Pin defines */
#define BLUE_BUTTON             2u
#define BLUE_LED                4u
#define RED_LED_L               3u
#define RED_LED_R               5u
#define RED_LED_C               6u
#define WHITE_LED_L             9u
#define WHITE_LED_R             10u
#define WHITE_LED_C             11u
#define BOARD_LED               13u

/* Control States */
enum system_state
    {
    STARTUP,
    NORMAL,
    BUTTON_PRESSED,
    WAITING,
    RESET,
    CLEAR
    };

#endif /* MODULE_CONTROL_H_ */
