apflights
=========

Powerlifting Judging Light Control System - Arduino/Xbee
========================================================

This is a simple light control system I developed for powerlifting meets using an Arduino and Xbee wireless modules. In powerlifting there are three judges and each is assigned a red and a white light. They use these lights to indicate their judgment on each lift, red for no lift, white for good lift. Most existing systems provide this functionality using traditional incandescent bulbs and some arrangement of extension cords and light switches. It's ugly, it's dangerous (wires running all over the place), and it's a pain to transport. 

My solution for Michigan APF was to create a wireless, LED based system. This system would provide each judge a small wireless module with two buttons, one red, one white. This module wirelessly connects to a base station which controls an array of red and white LEDs. No wires, no mess, very clean. This is the initial and currenlty only itteration of this design. COTS parts were selected to quickly produce a working system. Future work will focus on creating a more custom spun version of the system and reducing the overall cost.

This system also has the advantage of conforming exactly to the rules of the APF (this may vary among other federations, but the behavior is configurable). The rules state that the judging lights may only turn on, simultaneously, once all three judges have entered their decision. That is the lights can't turn on one at a time as each judge makes up their mind. They must turn on all at once. This is to prevent the decisions of one judge from influencing the others. Most existent systems did not meet that requirement, or did so by providing the head judge a separate master on/off switch which would prevent the lights from turning on before the head judge decided.

The system is based on four Arduino Unos (one for each judge, one for the base station/host), each with a Xbee wireless shield:

Judges Control (module): Small box containing arduino uno, Xbee shield, and battery. Contains two illuminated buttons (one red, one white), and one blue status LED (used to indicate when a button has been pressed).

Base Station (host): Contains arduino uno, Xbee shield, FET power control board (used to control the judging LEDs), and PC power supply (used to power the system and LEDs).

Basic Operation
===============

Each judge gets a module. When a lift is complete they press the button corresponding to their decision. All judges must enter their vote within 10 seconds of the first vote being cast or the system will judge it a false press and reset. Once all 3 votes are in the base will illuminate the corresponding indicator LEDs for 8 seconds, and then turn the LEDs off and reset. The system is then ready for the next lift. There is no ability to change a vote once it has been entered.

The code consists of module and host sections. The module code controls the judges boxes, and the host code controls the base station. The host comes up and begins a initialization and registration sequence. Each module checks in and registers with the base. Once complete the system runs a quick test of the LEDs, and then enters normal operation.

In normal mode each module acts as a state machine. In standby they wait for one of the two buttons (red or white) to be pressed. Once pressed (via interrupt) the module sends a state update to the base station using a simple handshaking protocol. The base station registers the button press and takes appropriate action. The module then enters a delay phase until it receives a clear from the base station.

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

For the LED indicator lights an eight conductor cable (bought at Lowes) was used providing 6 ground connections, and two power (12 V). The lights are backward wired with the ground switched through each FET. This was done because the FET's on the power shield are split into a 12 V and 5 V group (3 each). We need 12 V for all LEDs, thus the 12 V input of two of the FETs is used for all the LEDS, and the grounds are fed to the source of each FET.

The current design does not have a convenient way to charge/connect/disconnect the module batteries without manually opening the case. This is a current drawback of the design but was done to prevent tampering during operation. Future designs may improve on this.