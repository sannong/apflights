/**
 * \file host_conrtol.c 
 *  
 * \brief Source file for host control. 
 *  
 *	This file containes the main startup code and host control functions for    
 *	the referee light system.
 *	
 * \author Benjamin Sanda 
 * \date 17 Feb 13 
 *  
 */
 
#include <XBee.h>
#include <FlexiTimer2.h>
#include "host_control.h"
#include "common_control.h"
#include <avr/wdt.h>

/* #define  HOST_DEBUG */ 

typedef enum button_state_e
{
    BUTTON_CLEAR,
    BUTTON_RED,
    BUTTON_WHITE
}BUTTON_STATE_E;

typedef struct module_states_t
{
    uint32_t        link_state;
    uint32_t        module_index;
    BUTTON_STATE_E  button_state;
}MODULE_STATES_T;


MODULE_STATES_T module_states[NUM_MODULES]; 
unsigned int   button_count = 0; 
unsigned int   link_count = 0; 
volatile system_state control_state = STARTUP; 
unsigned int timer_state = TIMER_OFF;

/* glcbal XBEE variables */
XBee xbee = XBee();
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
RECV_MSG_T rx_msg;

/* forward declerations */
static void configurePins(void);
static void configureInterrupts(void);
static int sendMessage(uint8_t msg, unsigned int size, unsigned int destination);
static int recvMessage(int delay);
static void buttonPressBlue(void);
static void checkMessage();
static void softReset(void);
static void setLedState(void);
static void flashLed(int pin, int times, int wait);
static void controledReset(void);

void setup(void)
{
    int ret = ERROR;
    int i=0;

    /* configure the GPIO */
    configurePins();

    /* configure the interupts */
    configureInterrupts();

    /* initialize the xbee connection: interrupts must
       be on for xbee to work */
    xbee.begin(XBEE_SERIAL_RATE);

    /* activate the board LED to indicate setup is complete */
    digitalWrite(BOARD_LED, HIGH);
    
    /* activate the reset button LED to indicate system is ready */
    digitalWrite(BLUE_LED, HIGH);

    /* set initial LED states to all RED to indicate startup */
    digitalWrite(RED_LED_L, HIGH);
    digitalWrite(RED_LED_R, HIGH);
    digitalWrite(RED_LED_C, HIGH);
    
    delay(500);

    /* Establish the link with each module. Each module will send a "ready"
       command, verify an ACK from the host, then wait on a "clear" message from
       the host before entering the normal state/ */
    do
    {
        do
        {
            /* check for ready message from module */
            ret = recvMessage(0);
            
            if(control_state == RESET)
            {
              controledReset();
            }
        }while (ret != OK);
        
        if (rx_msg.size != INVALID_INDEX)
        {
            /* check for ready message */
            if (rx_msg.buf[0] == module_ready)
            {
                /* check this module isn't already connected */
                if (module_states[rx_msg.size].link_state != LINK_CONNECTED) 
                {
                    /* set the module state as connected */
                    module_states[rx_msg.size].link_state = LINK_CONNECTED; 
                    module_states[rx_msg.size].button_state = BUTTON_CLEAR; 
                    link_count++;
                    sendMessage(startup_ack, 1, rx_msg.source);
                }
            }
        }
    }while (link_count != NUM_MODULES); 
    
    delay(500);
    
    /* cycle through all LEDs to indicate ready and because it looks cool */
    for (i = 0; i < NUM_MODULES;i++)
    {
        module_states[i].button_state = BUTTON_RED; 
        setLedState(); 
        delay(500);
        module_states[i].button_state = BUTTON_CLEAR;
        setLedState();
        delay(500); 
    }
    for (i = 0; i < NUM_MODULES;i++)
    {
        module_states[i].button_state = BUTTON_WHITE;
        setLedState(); 
        delay(500); 
        module_states[i].button_state = BUTTON_CLEAR;
        setLedState();
        delay(500); 
    }
    
    digitalWrite(RED_LED_L, HIGH);
    digitalWrite(RED_LED_R, HIGH);
    digitalWrite(RED_LED_C, HIGH);
    digitalWrite(WHITE_LED_L, HIGH);
    digitalWrite(WHITE_LED_R, HIGH);
    digitalWrite(WHITE_LED_C, HIGH);
    delay(500);
    
    /* send all clear to place system into normal state */
    sendAllClear();

    if(control_state != RESET)
    {
      /* place the system into normal operation */
      control_state = NORMAL; 
    }
    
    return;
}

/**
 * Main program state machine loop
 * 
 */
void loop(void)
{
    int ret = ERROR;
    unsigned int i = 0;

    switch (control_state)
    {
    case NORMAL:
        {
            /* check for messages */
            ret = recvMessage(0);

            if (ret == OK)
            {
                /* see what we got, this will change the operating mode if
                   needed automatically */
                checkMessage();
            }

            break;
        }
    case CLEAR:
        {
            /* clear LEDs and send clear to modules */
            sendAllClear();
            delay(300);
            sendAllClear();
            
            if(control_state != RESET)
            {
              control_state = NORMAL;
            }
            break;
        }
    /* case when the LEDs have been activated and we are waiting to turn them
       back off */
    case WAITING:
        {
            /* delay for X secs */
            delay(LED_ON_DELAY);
            
            if(control_state != RESET)
            {
              control_state = CLEAR; 
            }
            break;
        }
   case BUTTON_PRESSED:
        {
            /* check to see if all modules have pressed a button, if so light
               the LEDs accordingly */
            if (button_count == NUM_MODULES)
            {
                /* stop the timer */
                FlexiTimer2::stop();
                timer_state = TIMER_OFF; 

                /* set the LEDs */
                setLedState();

                if(control_state != RESET)
                {
                  /* enter waiting mode */
                  control_state = WAITING; 
                }
            }
            /* not all buttons pressed yet, go back to normal and start
            the timer if not already started */
            else
            {
                if (timer_state == TIMER_OFF)
                {
                    /* start the timer */
                    FlexiTimer2::set(BUTTON_PRESS_TIMEOUT, watchdog_func); 
                    FlexiTimer2::start(); 
                    timer_state = TIMER_ON;
                }

                if(control_state != RESET)
                {
                  control_state = NORMAL;
                }
            }
            break;
        }
    case RESET:
        {

            controledReset();

        }
    default:
        {
            /* this should never happen: fatal error */
            softReset();
        }
    }
}


/** 
 * \brief Configures the GPIO dirrections and modes. 
 *  
 * This function configure the boards GPIO as needed. The following pins are     
 * configured: 
 *  - Pin 2: Blue button, configured as input with pullup resistor.
 *  - Pin 3: Red LED left, configured as output.
 *  - Pin 4: Blue button, configured as output.
 *  - Pin 5: Red LED right, configured as output.
 *  - Pin 6: Red LED center, configured as output.
 *  - Pin 9: White LED left, configured as output.
 *  - Pin 10: White LED right, configured as output.
 *  - Pin 11: White LED center, configured as output.
 *  - Pin 13: Onboard staus LED, configured as output.
 *  
 *  Once configured the output pin are set low (off).
 *
 */
static void configurePins(void)
{
    /* pin 2: blue button with pullup */
    pinMode(BLUE_BUTTON, INPUT_PULLUP);
    /* pin 4: blue LED  */
    pinMode(BLUE_LED, OUTPUT); 
    /* pin 3: red led left */
    pinMode(RED_LED_L, OUTPUT);
    /* pin 5: red led right */
    pinMode(RED_LED_R, OUTPUT);
    /* pin 6: red led center */
    pinMode(RED_LED_C, OUTPUT);
    /* pin 9: white led left*/
    pinMode(WHITE_LED_L, OUTPUT);
    /* pin 10: white led right */
    pinMode(WHITE_LED_R, OUTPUT);
    /* pin 11: white led center */
    pinMode(WHITE_LED_C, OUTPUT);
    /* pin 13: onboard status LED */
    pinMode(BOARD_LED, OUTPUT); 

    /* turn off the LEDs */
    digitalWrite(BLUE_LED, LOW); 
    digitalWrite(RED_LED_L, LOW); 
    digitalWrite(RED_LED_R, LOW); 
    digitalWrite(RED_LED_C, LOW); 
    digitalWrite(WHITE_LED_L, LOW); 
    digitalWrite(WHITE_LED_R, LOW); 
    digitalWrite(WHITE_LED_C, LOW); 
    digitalWrite(BOARD_LED, LOW);

    return;
}

/** 
 * \brief Configures the interrupts which drive the button presses and timers. 
 *  
 * Two external interrupts are used tied to GPIO pins 2 and 3 as follows: 
 *  - Pin 2 (int.0): White button press, configured to trigger on falling edge.
 *  - Pin 3 (int.1): Red button press, configured to trigger on falling edge.
 * 
 */
static void configureInterrupts(void)
{
    noInterrupts();

    /* attach the ISR rountines */
    /* BLue Button: int.0 = pin 2 */
    attachInterrupt(0, buttonPressBlue, FALLING);

    interrupts();

    return;
}

/** 
 * \brief ISR for white button press. 
 *
 */
static void buttonPressBlue(void)
{
        /* set the operating mode */
        control_state = RESET;
}

static int sendMessage(uint8_t msg, unsigned int size, unsigned int destination) 
{
    int ret = ERROR;
    uint8_t buf = 0;
    TxStatusResponse txStatus = TxStatusResponse();
    Tx16Request tx;

    /* ste the message buffer */
    buf = msg;

    /* set the TX request */
    tx = Tx16Request(destination, &buf, size);
    
    /* send the message */    
    xbee.send(tx); 
    
    if (destination != BROADCAST_ADDRESS)
    {
        /* wait for the ack */
        if (xbee.readPacket(250u))
        {
            /* check that it is the ack: TX_STATUS_RESPONSE */
            if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE)
            {
                /* get the responce */
                xbee.getResponse().getZBTxStatusResponse(txStatus);

                /* get the delivery status, the fifth byte */
                if (txStatus.getStatus() == SUCCESS)
                {
                    ret = OK;
                }
            }
        }
        else if (xbee.getResponse().isError())
        {
            ret = xbee.getResponse().getErrorCode();
        }
    }
    else
    {
        /* brodcast tx, no ACK returned */
        ret = OK;
    }

    return(ret);
}

static int recvMessage(int delay)
{
    int ret = ERROR;

    if (delay == 0)
    {
        /* read a message */
        xbee.readPacket(); 
    }
    else
    {
        /* read a message, with delay */
        xbee.readPacket(delay); 
    }
    
    if (xbee.getResponse().isAvailable())
    {
        // got something
        if (xbee.getResponse().getApiId() == RX_16_RESPONSE)
        {
            // got a rx packet
            xbee.getResponse().getRx16Response(rx16);
            rx_msg.source = rx16.getRemoteAddress16();
            rx_msg.buf = rx16.getData();

            if (rx_msg.source == LEFT_ADDRESS)
            {
                rx_msg.size = LEFT_MODULE; 
            }
            else if (rx_msg.source == RIGHT_ADDRESS)
            {
                rx_msg.size = RIGHT_MODULE; 
            }
            else if (rx_msg.source == CENTER_ADDRESS)
            {
                rx_msg.size = CENTER_MODULE; 
            }
            else
            {
                rx_msg.size = INVALID_INDEX;
            }
            ret = OK;
        }
        else if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE)
        {
            /* tx ack, just ignore for now */
            ret = NO_RX;
        }
    }
    else if (xbee.getResponse().isError())
    {
        ret = xbee.getResponse().getErrorCode();
    }
    else
    {
        /* no responce */
        ret = NO_RX;
    }

    return (ret);
}

/**
 * Checks incomming message 
 *  
 * This function checks the incomming message. If a clear is recived then the 
 * module is placed in normal mode, if a reset is recieved then the module is 
 * reset. 
 * 
 */
static void checkMessage()
{

    if (rx_msg.size != INVALID_INDEX)
    {
        if (rx_msg.buf[0] == module_ready)
        {
            control_state = CLEAR;
        }
        else if (rx_msg.buf[0] == white_button)
        {
            if (module_states[rx_msg.size].button_state == BUTTON_CLEAR) 
            {
                module_states[rx_msg.size].button_state = BUTTON_WHITE; 
                button_count++;
                control_state = BUTTON_PRESSED;
            }
            sendMessage(button_ack, 1, rx_msg.source); 
        }
        else if (rx_msg.buf[0] == red_button)
        {
            if (module_states[rx_msg.size].button_state == BUTTON_CLEAR) 
            {
                module_states[rx_msg.size].button_state = BUTTON_RED; 
                button_count++;
                control_state = BUTTON_PRESSED;
            }
            sendMessage(button_ack, 1, rx_msg.source); 
        }
        else
        {
            /* error, disregard */
        }
    }

    return;
}

static void softReset(void)
{
    Serial.end();
    wdt_enable(WDTO_15MS);
}

static void watchdog_func(void)
{
    /* set to normal_clear mode */
    control_state = CLEAR; 
}

/**
 * \brief Sends the clear message to all modules and resets the LEDs 
 *  
 * This fuction sends the clear message to each module to place them back into 
 * the normal state. It also resets the LEDs to all off. 
 * 
 */
static void sendAllClear()
{
    int i=0;
    
    /* stop the timer */
    FlexiTimer2::stop();
    timer_state = TIMER_OFF;

    /* set all LEDs to off */
    for (i = 0; i < NUM_MODULES; i++)
    {
        module_states[i].button_state = BUTTON_CLEAR;
    }
    
    button_count=0;
    
    /* send clear to all modules */
    sendMessage(module_clear, 1, LEFT_ADDRESS);
    sendMessage(module_clear, 1, RIGHT_ADDRESS);
    sendMessage(module_clear, 1, CENTER_ADDRESS);
    
    setLedState(); 
    
    return;
}

static void setLedState(void)
{

    int i = 0;

    for (i = 0; i < NUM_MODULES; i++) 
    {
        switch (module_states[i].button_state)
        {
        case BUTTON_CLEAR:
            {
                if (i == LEFT_MODULE) 
                {
                    digitalWrite(RED_LED_L, LOW);
                    digitalWrite(WHITE_LED_L, LOW);
                }
                else if (i == RIGHT_MODULE) 
                {
                    digitalWrite(RED_LED_R, LOW);
                    digitalWrite(WHITE_LED_R, LOW);
                }
                else
                {
                    digitalWrite(RED_LED_C, LOW);
                    digitalWrite(WHITE_LED_C, LOW);
                }

                break;
            }
        case BUTTON_RED:
            {
                if (i == LEFT_MODULE) 
                {
                    digitalWrite(RED_LED_L, HIGH);
                    digitalWrite(WHITE_LED_L, LOW);
                }
                else if (i == RIGHT_MODULE) 
                {
                    digitalWrite(RED_LED_R, HIGH);
                    digitalWrite(WHITE_LED_R, LOW);
                }
                else
                {
                    digitalWrite(RED_LED_C, HIGH);
                    digitalWrite(WHITE_LED_C, LOW);
                }

                break;
            }
        case BUTTON_WHITE:
            {
                if (i == LEFT_MODULE) 
                {
                    digitalWrite(RED_LED_L, LOW);
                    digitalWrite(WHITE_LED_L, HIGH);
                }
                else if (i == RIGHT_MODULE) 
                {
                    digitalWrite(RED_LED_R, LOW);
                    digitalWrite(WHITE_LED_R, HIGH);
                }
                else
                {
                    digitalWrite(RED_LED_C, LOW);
                    digitalWrite(WHITE_LED_C, HIGH);
                }

                break;
            }
        default:
            {
                break;
            }
        }
    }

    return;
}

/** 
 * \brief Flashes a specified LED for a given number of times.
 *
 * This function will flash a specified LED a given number of times with a given 
 * frequency. 
 *  
 * \param pin   Pin to flash
 * \param times Number of times to flash the LED
 * \param wait  How long to delay between on/off states (1/2 desired period)
 */
static void flashLed(int pin, int times, int wait)
{
    for (int i = 0; i < times; i++)
    {
        digitalWrite(pin, HIGH);
        delay(wait);
        digitalWrite(pin, LOW);

        if (i + 1 < times)
        {
            delay(wait);
        }
    }
}


static void controledReset(void)
{
        /* send clear */
        sendAllClear();

        /* delay to allow clear to propigate */
        delay(1000);

        /* send reset to all modules */
        sendMessage(module_reset, 1, BRODCAST_ADDRESS);
        
        delay(1000);

        /* reset, this will cause a full reset via the watchdog timer */
        softReset();
}
