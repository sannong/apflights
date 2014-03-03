apflights
=========

Powerlifting Judging Light Control System - Arduino/Xbee
========================================================

This is a simple light control system I developed for powerlifting meets using an Arduino and Xbee wireless modules. In powerlifting there are three judges and each is assigned a red and a white light. They use these lights to indicate thier judgment on each lift, red for no lift, white for good lift. Most existing systems provide this functionality using traditional incandecent bulbs and some aragment of extention cords and light switches. It's ugly, it's dangerous (wires running all over the place), and it's a pain to transport. 

My solution for Michigan APF (the powerlifting federation in Michigan I help run) was to create a wireless, LED based system. This system would provide each judge a small wireless module with two buttons, one red, one white. This module wirelessly connects to a base station which controls an array of red and white LEDs. No wires, no mess, very clean.

This system also has the advantage of conforming exactly to the rules of the APF (this may vary amoung other federations, but the behaviour is configurable). The rules state that the judging lights may only turn on, symultaniously, once all three judges have entered thier decision. That is the lights can't turn on one at a time as each judge makes up thier mind. They must turn on all at once. This is to prevent the decisions of one judge from influencing the others. Most existant systems did not meet that requirement, or did so by providing the head judge a seperate master on/off switch which would prevent the lights from turning on before the head judge decided.

The system is based on four Arduino Unos (one for each judge, one for the base station), each with a Xbee wireless shield:

Judges Control: Small box containing arduino uno, Xbee shield, and battery. Contains two illuminated buttons (one red, one white), and one blue status LED (used to indicate when a button has been pressed).

Base Station: Contains arduino uno, Xbee shield, FET power control board (used to control the judging LEDs), and PC power supply (used to power the system and LEDs).

Basic Operation
===============

Each judge gets a module. When a lift is complete they press the button coresponding to their decision. All judges must enter their vote within 10 seconds of the first vote being cast or the system will judge it a false press and reset. Once all 3 votes are in the base will illuminate the coresponding LEDs for 8 seconds, and then turn the LEDs off and reset. The system is then ready for the next lift. There is no ability to change a vote once it has been entered.

The code consists of module and host sections. The module code controls the judges boxes, and the host code controls the base station. The host comes up and begins a initialization and registration sequence. Each module checks in and registers with the base. Once complete the system runs a quick test of the LEDs, and then enters normal operation.

In normal mode each module acts as a state machine. In standby they wait for one of the two buttons (red or white) to be pressed. Once pressed (via interrupt) the module sends a state update to the base station using a simple handshaking protocol. The base station registers the button press and takes aproporate action. The module then enters a delay phase untill it recieves a clear from the base station.

The base station waits, collecting the updates from the modules untill each module has sent a vote. Once all three have been entered it acitvates the LED lights using the FET board for 8 seconds. Once the time is complete it turns off the LEDs and sends a clear to all modules to place the system back into standby.

The base station also monitors the watchdog timer on vote entry. All three votes must occur within 10 seconds of the first vote. If not, the base station will reset and clear the modules.
