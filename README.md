apflights
=========

Powerlifting Judging Light Control System - Arduino/Xbee
========================================================

This is a simple wireless LED judging light control system I developed for powerlifting meets using an Arduino Uno and Xbee wireless modules. In powerlifting there are three judges and each is assigned a red and a white light. They use these lights to indicate their judgment on each lift, red for no lift, white for good lift. Most existing systems provide this functionality using traditional incandescent bulbs and some arrangement of extension cords and light switches. It's ugly, it's dangerous (wires running all over the place), and it's a pain to transport. 

My solution for Michigan APF was to create a wireless, LED based system. This system provides each judge a small, battery powered, wireless module with two buttons, one red, one white. This module wirelessly connects to a base station which controls an array of red and white LEDs. The judges simply press the button corresponding to their judgment, and the base station illuminates the correct lights. No wires, no mess, very clean.

This system also has the advantage of conforming exactly to the rules of the APF (this may vary among other federations, but the behavior is configurable). The rules state that the judging lights may only turn on simultaneously, once all three judges have entered their decision. That is the lights can't turn on one at a time as each judge makes up their mind. They must turn on all at once. This is to prevent the decisions of one judge from influencing the others. Most existent systems did not meet that requirement, or did so by providing the head judge a separate master on/off switch which would prevent the lights from turning on before the head judge decided. In this design the base station acts as the master controller and only illuminates the lights once it has received the selection from all three judges.

This is the initial and currently only iteration of this design. COTS parts were selected to quickly produce a working system. Because of this there is excess cost and functionality which could be avoided with a fully custom PCB design. Future work will focus on creating a custom version of the system and reducing the overall cost.

Hardware
===============

The system is based on four Arduino Unos (one for each judge, one for the base station/host), each with a Xbee wireless shield:

Judges Control (module): Small box containing Arduino uno, Xbee shield, and Li-Ion battery. Contains two illuminated buttons (one red, one white), and one blue status LED (used to indicate when a button has been pressed). The battery has been selected to give a runtime of > 24 hrs per charge and may be recharged in a few hours using commercial Li-Ion chargers.

Base Station (host): Contains Arduino uno, Xbee shield, FET power control board (used to control the judging LEDs), and PC power supply (used to power the system and LEDs). A basic ATX PC power supply. This was used because the COTS FET board available from Sparkfun incorporates an ATX connector. It supplies far more power than is needed, but is a quick and simple solution. The ATX supply powers both the base station circuity and the LED judging lights.

LED Lights: The system provides 5 or 12 V DC to power the LED judging lights. Any lights which take 5 or 12 V DC may be used. In this design LED bar lights, originally designed to go under/around cabinetry for accent lighting, are used. A simple 8 wire conductor connects the lights to the base station. 

Basic Operation
===============

Each judge gets a module. When a lift is complete they press the button corresponding to their decision. Once a judge presses a button, the button's turn off and the blue indicator LED on the module turns on indicating the vote has been submitted. In this state pressing the buttons has no effect. All judges must enter their vote within 10 seconds of the first vote being cast or the system will judge it an erroneous press and reset. Once all 3 votes are submitted the base station will illuminate the corresponding indicator LEDs for 8 seconds (configurable), and then turn the LEDs off and reset the modules. The blue LED on each module will turn off and the buttons will turn back on indicating the module is ready. The system is then ready for the next lift. There is no ability to change a judges vote once it has been entered.

The code consists of module and host sections. The module code controls the judges boxes, and the host code controls the base station. The host comes up and begins a initialization and registration sequence. Each module checks in and registers with the base. During this registration process the blue LED indicator on each module will blink periodically. When the blue LED turns solid it is an indication that the module has completed registration with the base station. Once complete the system runs a quick test of the LEDs, and then enters normal operation. At this point the red and white buttons on the modules will illuminate and the blue LED will turn off indicating the module is ready. There is no requirement regarding what turns on first or in what sequence. The base station may be turned on, followed by the modules, or the modules may all be on, and then the base station powered up. The system is designed to handle any sequence.

In normal mode each module acts as a state machine. In standby they wait for one of the two buttons (red or white) to be pressed. Once pressed (via interrupt) the module sends a state update to the base station using a simple handshaking protocol. The base station registers the button press and takes appropriate action. The module then enters a hold phase until it receives a clear from the base station in which all button functionality is locked out.

The base station waits, collecting the updates from the modules until each module has sent a vote. Once all three have been entered it activates the LED lights using the FET board for 8 seconds. Once the time is complete it turns off the LEDs and sends a clear to all modules to place the system back into standby.

The base station also monitors the watchdog timer on vote entry. All three votes must occur within 10 seconds of the first vote. If not, the base station will reset and clear the modules.

Adruino Programming Notes
===============

The module Arduino's are programmed with the module source code, and the host is programmed with the host code. All modules are programmed with the same image (determination of left, right, center is made automatically using the Xbee address). 

Note: The Arduino source files end in .ino not .c. They are C files, but the Arduino IDE likes them named .ino. They are just normal C files though and can be edited as such.

Xbee Programming Notes
===============

The Xbee modules are programmed using the XCTU utility available from Digi and the SparkFun XBee Explorer USB board. The Xbee module is placed in the Explorer board and programed via USB from a PC. The Xbee modules must be programed with the Xbee address and BAUD rate defined in the common_control.h header files. The default values are:

define XBEE_SERIAL_RATE        9600

define HOST_ADDRESS            0x1

define LEFT_ADDRESS            0x2

define RIGHT_ADDRESS           0x3

define CENTER_ADDRESS          0x4

define BRODCAST_ADDRESS        0xFFFF

If the address and BAUD defined in the header is not used communication will fail. Also, although possible, I do not recommend increasing the Xbee BAUD rate above 9600. Higher rates tax the Arduino and can lead to unreliable communication between the host and modules. An example programming file is provided in the repository.

Construction Notes
===============

For the LED indicator lights an eight conductor cable (bought at Lowes) was used providing 6 ground connections, and two power (12 V). The lights are reverse wired with the ground switched through each FET. This was done because the FET's on the power shield are split into a 12 V and 5 V group (3 each). We need 12 V for all LEDs, thus the 12 V input of two of the FETs is used for all the LEDS, and the grounds are fed to the source of each FET.

The current design does not have a convenient way to charge/connect/disconnect the module batteries without manually opening the case. This is a current drawback of the design but was done to prevent tampering during operation. Future designs may improve on this. The modules also include a magnetic reed switch mounted on the bottom inside of the case. This switch is wired inline with the battery and is used to externally reset or turn off the module. If a magnet is held near the switch the battery will be disconnected and the module will turn off. This was done to provide a means to perform a hard reset of the module without opening the case (it could also be used to turn off the module for storage if the magnet is kept in contact). A physical on/off switch could also be used, but a reed switch was used so there would be no accessible switch which could be tampered with or accidentally hit during operation. 

How you actually mount and display the LED lights is entirely up to you. For our system I carved and finished a big block of walnut and used that as the backboard to mount the lights onto (see pictures in the img folder). I then mounted a speaker stand adapter to the bottom which just sits on a standard AV speaker stand. For the base station I used a large plastic outdoor conduit box and drilled holes in the sides as needed.

The choice of lights is also customizable. They just have to be 12 V or 5 V LEDs. You can use the bar type I used, or any that fit your aesthetic and budget. One other option we almost went with were automotive truck LED indicators.

Questions/Comments
===============

Fell free to email the author Ben Sanda with any questions or comments: bensanda@michiganapf.com