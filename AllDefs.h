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

// ========================
// === EEPROM Addresses ===
// ========================
// Storage for General - Available size: 32 bytes
#define EEP_GENERAL_START 0x00
#define EEP_GENERAL_SIZE 0x20

#define EEP_OPMODE 0x00
#define EEP_USE_NTP_SERVER 0x01
#define EEP_TZ_ADJ 0x02
#define EEP_IS_DST 0x03
#define EEP_HANDSIZE_HOUR 0x04
#define EEP_HANDSIZE_MIN 0x05
#define EEP_HANDSIZE_SEC 0x06
#define EEP_HANDCOLOR_HOUR 0x07  // (3 bytes)
#define EEP_HANDCOLOR_MIN 0x0A  // (3 bytes)
#define EEP_HANDCOLOR_SEC 0x0D  // (3 bytes)
/*
#define EEP_HOUR_HAND_OUT 0x10
#define EEP_HOUR_HAND_IN 0x11
#define EEP_MIN_HAND_OUT 0x12
#define EEP_MIN_HAND_IN 0x13
#define EEP_SEC_HAND_OUT 0x14
#define EEP_SEC_HAND_IN 0x15
*/

// Storage for Outside strip - Available size: 32 bytes
#define EEP_OUT_START (EEP_GENERAL_START + EEP_GENERAL_SIZE)
#define EEP_OUT_SIZE 0x20

#define EEP_OUT_EXTERNALCTRLMODE (EEP_OUT_START + 0x00)
#define EEP_OUT_EXTERNALCTRLMODE_FLOWSPEED (EEP_OUT_START + 0x01)
#define EEP_OUT_EXTERNALCTRLMODE_FLOWNUMSECTIONS (EEP_OUT_START + 0x02)
#define EEP_OUT_COLORED5S_COLOR (EEP_OUT_START + 0x03)  // (3 bytes)
#define EEP_HOUR_HAND_OUT (EEP_OUT_START + 0x06)
#define EEP_MIN_HAND_OUT (EEP_OUT_START + 0x07)
#define EEP_SEC_HAND_OUT (EEP_OUT_START + 0x08)

// Storage for Inside strip - Available size: 32 bytes
#define EEP_IN_START (EEP_OUT_START + EEP_OUT_SIZE)
#define EEP_IN_SIZE 0x20

#define EEP_IN_EXTERNALCTRLMODE (EEP_IN_START + 0x00)
#define EEP_IN_EXTERNALCTRLMODE_FLOWSPEED (EEP_IN_START + 0x01)
#define EEP_IN_EXTERNALCTRLMODE_FLOWNUMSECTIONS (EEP_IN_START + 0x02)
#define EEP_IN_COLORED5S_COLOR (EEP_IN_START + 0x03)  // (3 bytes)
#define EEP_HOUR_HAND_IN (EEP_IN_START + 0x06)
#define EEP_MIN_HAND_IN (EEP_IN_START + 0x07)
#define EEP_SEC_HAND_IN (EEP_IN_START + 0x08)

// Storage for Strip Cmds
#define EEP_STRIPCMDS_START (EEP_IN_START + EEP_IN_SIZE)
#define EEP_STRIPCMDS_SZIE 0x280  // 640 (320 for OUTSIDE, 320 for INSIDE)

#define EEP_STRIPCMDS EEP_STRIPCMDS_START  // Size: 640 (320 for OUTSIDE, 320 for INSIDE)

#define MAX_STRIPCMD_SIZE 32
#define MAX_NUM_STRIPCMDS 10  // 10 per each strip

#define WIFI_PACKET_DUMP_EEPROM_SM 0x90  // (144) SM => Serial Monitor
#define WIFI_PACKET_SET_OPMODE 0xAA
#define WIFI_PACKET_SET_OUT_EXTERNALCTRLMODE 0xBB
#define WIFI_PACKET_SET_IN_EXTERNALCTRLMODE 0xBC
#define WIFI_PACKET_SET_OUT_COLORED5S_COLOR 0xBD
#define WIFI_PACKET_SET_IN_COLORED5S_COLOR 0xBE
//#define WIFI_PACKET_SET_COLOR 0xCC
#define WIFI_PACKET_SET_USE_NTP_SERVER 0xC0  // 192
#define WIFI_PACKET_SET_TIME 0xC1  // 193
#define WIFI_PACKET_SET_TZ_ADJ 0xC2  // 194
#define WIFI_PACKET_SET_IS_DST 0xC3  // 195
#define WIFI_PACKET_SET_HAND_SIZES 0xC4  // 196
#define WIFI_PACKET_SET_HAND_COLORS 0xC5  // 197
#define WIFI_PACKET_SET_DISP_CLOCK_HANDS_OUT_IN 0xC6  // 198
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
#define HANDCOLOR_HOUR 2
#define HANDCOLOR_MIN 3
#define HANDCOLOR_SEC 4

#define COLOR_USAGE_NONE 0
#define COLOR_USAGE_OUT_ECM 1
#define COLOR_USAGE_IN_ECM 2
#define COLOR_USAGE_OUT_COLORED5S 3
#define COLOR_USAGE_IN_COLORED5S 4
#define COLOR_USAGE_HOUR_HAND 5
#define COLOR_USAGE_MIN_HAND 6
#define COLOR_USAGE_SEC_HAND 7


// NTP Server
#define NTP_GRAB_FREQ_MS 86400000  //24*60*60*1000 = 24 hours
#define NTP_NUM_TRIES 3  // Try to get time from NTP server 3 times.
#define SECONDS_FROM_1970_TO_2000 946684800
#define HOUR 0
#define MIN 1
#define SEC 2


#endif  // ALLDEFS_H