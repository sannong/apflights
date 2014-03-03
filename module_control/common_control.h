
/**
 * \file common_conrtol.h 
 *  
 * \brief Header file for common controls 
 *  
 * \author Benjamin Sanda 
 * \date 17 Feb 13 
 *  
 */

#ifndef COMMON_CONTROL_H_
#define COMMON_CONTROL_H_

/* XBEE communication BAUD rate*/
#define XBEE_SERIAL_RATE 38400

/* General Defines */
#define ONE_SEC_MS              1000u
#define OK                      0
#define ERROR                   -1
#define NO_RX                   1u
#define LINK_CONNECTED          1u
#define LINK_NOCONNECT          0u
#define NUM_MODULES             3u
#define TIMER_ON                0x1
#define TIMER_OFF               0x0
#define INVALID_INDEX           0xFF

/* Module defines */
#define HOST_MODULE             3u
#define LEFT_MODULE             0u
#define RIGHT_MODULE            1u
#define CENTER_MODULE           2u

#define HOST_ADDRESS            0x1
#define LEFT_ADDRESS            0x2
#define RIGHT_ADDRESS           0x3
#define CENTER_ADDRESS          0x4
#define BRODCAST_ADDRESS        0xFFFF

/* defines for each module message */
#define module_ready    1u
#define white_button    2u
#define red_button      3u
#define module_clear    4u
#define module_reset    5u 
#define button_ack      6u
#define startup_ack      7u

typedef struct recv_msg_t
{
    uint8_t *buf;
    unsigned int source;
    unsigned int size;
}RECV_MSG_T; 

#endif /* COMMON_CONTROL_H_ */
