/**
 * \file module_conrtol.h 
 *  
 * \brief Header file for general module control 
 *  
 * \author Benjamin Sanda 
 * \date 17 Feb 13 
 *  
 */

#ifndef MODULE_CONTROL_H_
#define MODULE_CONTROL_H_

#define BUTTON_PRESS_TIMEOUT    (25*ONE_SEC_MS)
#define BUTTON_SEND_ATTEMPTS    20u

/* Pin defines */
#define WHITE_BUTTON    2u
#define RED_BUTTON      3u
#define WHITE_LED       4u
#define RED_LED         5u
#define INDICATOR_LED   6u
#define BOARD_LED       13u

/* Control States */
enum system_state
{
    STARTUP,
    STARTUP_ACK,
    NORMAL,
    W_BUTTON_PRESSED,
    R_BUTTON_PRESSED,
    RESET,
    WAITING,
    CLEAR};

#endif /* MODULE_CONTROL_H_ */
