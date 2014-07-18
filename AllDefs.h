// temp
/*
	Copyright 2014 Mark Briggs

	This file is part of ColorRing.

	ColorRing is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	any later version.

	ColorRing is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	There is a copy of the GNU General Public License in the
	accompanying COPYING file
*/

/*
	This file simply contains all the #define's for all the code,
	in order to keep things consolidated.

	The ones you'd most likely need to change are located near the
	top of this file.
*/

#ifndef ALLDEFS_H
#define ALLDEFS_H

#include "pw.h"

#define WLAN_SECURITY   WLAN_SEC_WPA2

// The port to listen for incoming TCP/UDP connections 
#define LISTEN_PORT_HTTP           80  // TCP
#define LISTEN_PORT_ECM 	       2811  // UDP

// NeoPixel Strip 
#define NUM_LEDS 60
#define OUT_STRIP_PIN 6
#define IN_STRIP_PIN 7

// These are the pins for the CC3000 chip if you are using a breakout board
#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

// === EEPROM Addresses ===
// Storage for General - Available size: 16 bytes
#define EEP_OPMODE 0  

// Storage for Outside strip - Available size: 32 bytes
#define EEP_OUT_EXTERNALCTRLMODE 0x10
#define EEP_OUT_EXTERNALCTRLMODE_FLOWSPEED 0x11
#define EEP_OUT_EXTERNALCTRLMODE_FLOWNUMSECTIONS 0x12
#define EEP_OUT_COLORED5S_COLOR 0x13  // (3 bytes)
// Note: next starts at byte 0x16!

// Storage for Inside strip - Available size: 32 bytes
#define EEP_IN_EXTERNALCTRLMODE 0x30
#define EEP_IN_EXTERNALCTRLMODE_FLOWSPEED 0x31
#define EEP_IN_EXTERNALCTRLMODE_FLOWNUMSECTIONS 0x32
#define EEP_IN_COLORED5S_COLOR 0x33  // (3 bytes)
// Note: next starts at byte 0x36!

#define EEP_STRIPCMDS 0x50  // Size: 640 (320 for OUTSIDE, 320 for INSIDE)

#define MAX_STRIPCMD_SIZE 32
#define MAX_NUM_STRIPCMDS 10  // 10 per each strip

#define WIFI_PACKET_SET_OPMODE 0xAA
#define WIFI_PACKET_SET_OUT_EXTERNALCTRLMODE 0xBB
#define WIFI_PACKET_SET_IN_EXTERNALCTRLMODE 0xBC
#define WIFI_PACKET_SET_OUT_COLORED5S_COLOR 0xBD
#define WIFI_PACKET_SET_IN_COLORED5S_COLOR 0xBE
#define WIFI_PACKET_SET_COLOR 0xCC
#define WIFI_PACKET_SET_STRIPCMD 0xDD
//#define WIFI_PACKET_SET_IN_STRIPCMD 0xDE

// === OPMODEs ===
//#define OPMODE_OUT_INTERNAL_IN_INTERNAL 0
//#define OPMODE_OUT_INTERNAL_IN_EXTERNAL 1
//#define OPMODE_OUT_EXTERNAL_IN_INTERNAL 2
//#define OPMODE_OUT_EXTERNAL_IN_EXTERNAL 3
#define OPMODE_INTERNAL 0
#define OPMODE_EXTERNAL 1
#define OPMODE_CLOCK 2
#define OPMODE_COLORED5S 3

// === External Ctrl Modes ===
#define EXTERNALCTRLMODE_STRIPCOLOR 0
#define EXTERNALCTRLMODE_FLOW 1

#define UDP_READ_BUFFER_SIZE 20

#define OUTSIDE_STRIP true
#define INSIDE_STRIP false

// Light Show Cmd Defs
#define CLEAR true
#define NOCLEAR false
#define CW true
#define CCW false
#define DESTRUCTIVE true
#define NONDESTRUCTIVE false
#define WRAP true
#define NOWRAP false
#define ANIMATED true
#define NONANIMATED false
#define GRADIATE true
#define NOGRADIATE false
#define GRADIATE_LASTPIXEL_FIRSTCOLOR true
#define GRADIATE_LASTPIXEL_LASTCOLOR false

#define SHOWSTRIP true
#define NOSHOWSTRIP false

// BoolBit Defs
#define BOOLBIT_DESTRUCTIVE 0
#define BOOLBIT_DIRECTION 1
#define BOOLBIT_WRAP 2
#define BOOLBIT_ISANIM 3
#define BOOLBIT_CLEARSTRIP 4
#define BOOLBIT_GRADIATE 5
#define BOOLBIT_GRADIATE_LASTPIXEL_FIRSTCOLOR 6

// Colors - which are saved in EEPROM (but not within strip cmds)
#define OUT_COLORED5S_COLOR 0
#define IN_COLORED5S_COLOR 1

#endif  // ALLDEFS_H