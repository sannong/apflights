/**
 * \file module_conrtol.c 
 *  
 * \brief Source file for general module control. 
 *  
 *	This file contains the main startup code and module control functions for    
 *	the referee light system.
 *	
 * \author Benjamin Sanda 
 * \date 17 Feb 13 
 *  
 */

#include <XBee.h>
#include <FlexiTimer2.h>
#include "module_control.h"
#include "common_control.h"
#include <avr/wdt.h>

volatile system_state control_state = STARTUP; 
/* XBEE Globals */
XBee xbee = XBee(); 
XBeeResponse response = XBeeResponse();
Rx16Response rx16 = Rx16Response();
RECV_MSG_T rx_msg; 

/* forward decelerations */
static void flashLed(int pin, int times, int wait);
static void configurePins(void);
static void configureInterrupts(void);
static int sendMessage(uint8_t msg, unsigned int size, unsigned int destination);
static int recvMessage(int delay);
static void buttonPressWhite(void);
static void buttonPressRed(void);
inline static void ledStateButton(void);
inline static void ledStateNormal(void);
static void checkMessage();
static void softReset(void);
static void watchdog_func(void);
static void sendMessageButton(void);

void setup(void) 
{
    int ret = ERROR; 
    
    /* configure the GPIO */
    configurePins();
    
    /* configure the interrupts */
    configureInterrupts();

    /* initialize the xbee connection: interrupts must
       be on for xbee to work */
    Serial.begin(XBEE_SERIAL_RATE);
    xbee.setSerial(Serial);

    /* activate the board LED to indicate setup is complete */
    digitalWrite(BOARD_LED, HIGH);

    while (control_state == STARTUP)
    {
        /* inform the host we are ready, continue to retry until the host 
        responds*/
        flashLed(INDICATOR_LED, 1, 200); 
        delay(1000u);
        ret = sendMessage(module_ready, 1, HOST_ADDRESS);

        if (ret == OK)
        {
            ret = recvMessage(150u);

            if (ret == OK)
            {
                /* check to see if clear was received and if so place into startup ack mode*/ 
                checkMessage(); 
            }
        }
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
 
            if(ret == OK)
            {
                checkMessage(); 
            }

            break;
        }
    case WAITING:
        {    
            /* check for clear message */
            do
            {
                ret = recvMessage(0);
            }
            while (ret != OK);

            checkMessage();

            break;
        }
    case W_BUTTON_PRESSED:
    case R_BUTTON_PRESSED:
        {
          /* set the LEDs to indicate button press */
          ledStateButton();
          
          /* start the timer */
          FlexiTimer2::set(BUTTON_PRESS_TIMEOUT, watchdog_func); 
          FlexiTimer2::start();
          
        if (control_state == W_BUTTON_PRESSED)
        {
            control_state = W_BUTTON_SEND;
        }
        else if (control_state == R_BUTTON_PRESSED)
        {
            control_state = R_BUTTON_SEND;
        }
        else
        {
          control_state = RESET;
        }
            
          break;         
        }
    case W_BUTTON_SEND:
    {
        /* send the control message to the host */
          ret = sendMessage(white_button, 1, HOST_ADDRESS);

          if (ret == OK)
          {
              ret = recvMessage(150u);

              if (ret == OK)
              {
                  checkMessage();
              }         
          }

           break; 
    }
    case R_BUTTON_SEND:
    {
        /* send the control message to the host */
        ret = sendMessage(red_button, 1, HOST_ADDRESS);
        
        if (ret == OK)
        {
            ret = recvMessage(150u);
        
            if (ret == OK)
            {
                checkMessage();
            }
        }
        
         break; 
    }
    case RESET:
        {
            softReset();
        }
    default:
        {
            /* this should never happen: fatal error */
            softReset();
        }
    }
}


/** 
 * \brief Configures the GPIO directions and modes. 
 *  
 * This function configure the boards GPIO as needed. The following pins are     
 * configured: 
 *  - Pin 2: White button, configured as input with pullup resistor.
 *  - Pin 3: Red button, configured as input with pullup resistor.
 *  - Pin 4: White button LED, configured as output.
 *  - Pin 5: Red button LED, configured as output.
 *  - Pin 6: Blue indicator LED, configured as output.
 *  - Pin 13: Onboard status LED, configured as output.
 *  
 *  Once configured the output pin are set low (off).
 *
 */
static void configurePins(void)
{
    /* pin 2: white button with pullup */
    pinMode(WHITE_BUTTON, INPUT_PULLUP);
    /* pin 3: red button with pullup */
    pinMode(RED_BUTTON, INPUT_PULLUP);
    /* pin 4: white button LED */
    pinMode(WHITE_LED, OUTPUT);
    /* pin 5: red button LED */
    pinMode(RED_LED, OUTPUT);
    /* pin 6: State indicator LED */
    pinMode(INDICATOR_LED, OUTPUT);
    /* pin 13: onboard status LED */
    pinMode(BOARD_LED, OUTPUT);

    /* turn off the LEDs */
    digitalWrite(WHITE_LED, LOW);
    digitalWrite(RED_LED, LOW);
    digitalWrite(BOARD_LED, LOW);
    digitalWrite(INDICATOR_LED, LOW);

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

    /* attach the ISR routines */
    /* White Button: int.0 = pin 2 */
    attachInterrupt(0, buttonPressWhite, FALLING);
    /* Red Button: int.1 = pin 3 */
    attachInterrupt(1, buttonPressRed, FALLING);
    
    interrupts();

    return;
}

/** 
 * \brief ISR for white button press. 
 *
 */
static void buttonPressWhite(void)
{
    if (control_state == NORMAL)
    {
        /* set the operating mode */
        control_state = W_BUTTON_PRESSED;
    }
}

/** 
 * \brief ISR for red button .
 *
 */
static void buttonPressRed(void)
{
    if (control_state == NORMAL)
    {
        /* set the operating mode */
        control_state = R_BUTTON_PRESSED;
    }
}


/**
 * Sets the LEDs into button pressed state 
 *  
 * Sets the LEDs to the state indicating a button has been pressed: 
 *  - Button LEDS: OFF
 *  - Status LED: ON
 */
__inline__ static void ledStateButton(void)
{
    /* set the buttons off and the status LED on */
    digitalWrite(INDICATOR_LED, HIGH);
    digitalWrite(WHITE_LED, LOW);
    digitalWrite(RED_LED, LOW);
}

/**
 * Sets the LEDs into normal state 
 *  
 * Sets the LEDs to the state indicating normal (ready for button press) state: 
 *  - Button LEDS: ON
 *  - Status LED: OFF
 */
__inline__ static void ledStateNormal(void)
{
    /* set the buttons on and the status LED off */
    digitalWrite(INDICATOR_LED, LOW);
    digitalWrite(WHITE_LED, HIGH);
    digitalWrite(RED_LED, HIGH);
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

static int sendMessage(uint8_t msg, unsigned int size, unsigned int destination)
{
    int ret = ERROR;
    uint8_t buf = 0;
    TxStatusResponse txStatus = TxStatusResponse();
    Tx16Request tx;
    unsigned int count = 0;

    /* set the message buffer */
    buf = msg;

    do
    {

        /* set the TX request */
        tx = Tx16Request(destination, &buf, size);

        /* send the message */
        xbee.send(tx);

        /* wait for the ack */
        if (xbee.readPacket(250u))
        {
            /* check that it is the ack: TX_STATUS_RESPONSE */
            if (xbee.getResponse().getApiId() == TX_STATUS_RESPONSE)
            {
                /* get the response */
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

        count++;

    }while((ret != OK) && (count < RETRY_COUNT));

    return (ret);
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
        /* read a message with delay */
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
        /* no response */
        ret = NO_RX;
    }

    return (ret);
}

/**
 * Checks incoming message 
 *  
 * This function checks the incoming message. If a clear is revived then the 
 * module is placed in normal mode, if a reset is received then the module is 
 * reset. 
 * 
 */
static void checkMessage()
{
    if (rx_msg.source == HOST_ADDRESS)
    {
        /* check the message */       
        if (rx_msg.buf[0] == module_clear)
        {
            /* clear received, go back to normal mode */
            /* stop the timer */
            FlexiTimer2::stop(); 
            ledStateNormal(); 
            control_state = NORMAL;
        }
        else if (rx_msg.buf[0] == button_ack) 
        {
            control_state = WAITING; 
        }
        else if (rx_msg.buf[0] == module_reset)
        {
            control_state = RESET; 
        }
    }
}

static void softReset(void)
{
    delay(1000);
    FlexiTimer2::stop(); 
    Serial.end();   
    wdt_enable(WDTO_15MS);
}

static void watchdog_func(void)
{
    softReset();
}
