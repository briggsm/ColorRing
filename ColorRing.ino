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
	This is the starting point for the code which is loaded onto the Arduino
	borad. It contains the setup() and loop() functions needed for any
	Arduino program.
*/

#include <Adafruit_CC3000.h>
#include <SPI.h>
#include "utility/debug.h"  // for getFreeRam()
#include <Ethernet.h>
#include <CC3000_MDNS.h>
#include "HttpHandler.h"

#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "PixelColor.h"
#include "EcmServer.h"

#include "SetSeqPixels.h"
#include "Shift.h"
#include "Flow.h"

#include <StandardCplusplus.h>
#include <serstream>
#include <vector>

using namespace std;

namespace std {
	ohserialstream cout(Serial);
}

#include "AllDefs.h"

// Create CC3000 instance
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);

// Create HTTP Handler instance
HttpHandler hh = HttpHandler();

// Server instance
Adafruit_CC3000_Server httpServer(LISTEN_PORT_HTTP);
EcmServer ecmServer(LISTEN_PORT_ECM);

// DNS responder instance
MDNSResponder mdns;


// GLOBAL VARIABLES
int OpMode;
int OutExternalCtrlMode;
int OutExternalCtrlModeFlowSpeed;
int OutExternalCtrlModeFlowNumSections;
int InExternalCtrlMode;
int InExternalCtrlModeFlowSpeed;
int InExternalCtrlModeFlowNumSections;
PixelColor Color;


Adafruit_NeoPixel outStrip = Adafruit_NeoPixel(NUM_LEDS, OUT_STRIP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel inStrip  = Adafruit_NeoPixel(NUM_LEDS, IN_STRIP_PIN,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel* strip;

uint8_t packet[40];

int maxNumStripCmds;
int maxStripCmdSize;

vector<LightShowCmd*> stripCmds(MAX_NUM_STRIPCMDS * 2); // Note: 1st half is OUTSIDE strip, 2nd half is INSIDE strip

long outPreviousMillis = 0;
long inPreviousMillis = 0;
unsigned long currentMillis = 0;
long outAnimDelay = 0;
long inAnimDelay = 0;

long lastOecmFlowMs = 0;
long lastIecmFlowMs = 0;
PixelColor lastOecmFlowColor(0,0,0);
PixelColor lastIecmFlowColor(0,0,0);


void setup(void)
	{  
	Serial.begin(115200);

	cout << F("Free RAM Start Setup: ") << getFreeRam() << endl;

	// Print out the EEPROM
	eepromPrint(656, "hex");

	// Probably don't NEED to do this, but good just to have a sure starting point.
	// Clear stripCmds array.
	for (int i = 0; i < MAX_NUM_STRIPCMDS * 2; i++) {
		stripCmds[i] = NULL;
	}

	// Read all data from EEPROM
	readAllFromEEPROM();

	// Init variables and expose them to HTTP Handler
	maxNumStripCmds = MAX_NUM_STRIPCMDS;
	maxStripCmdSize = MAX_STRIPCMD_SIZE;
	hh.variable("maxNumStripCmds", &maxNumStripCmds);
	hh.variable("maxStripCmdSize", &maxStripCmdSize);
	hh.variable("opMode", &OpMode);
	hh.variable("outExternalCtrlMode", &OutExternalCtrlMode);
	hh.variable("outExternalCtrlModeFlowSpeed", &OutExternalCtrlModeFlowSpeed);
	hh.variable("outExternalCtrlModeFlowNumSections", &OutExternalCtrlModeFlowNumSections);
	hh.variable("inExternalCtrlMode", &InExternalCtrlMode);
	hh.variable("inExternalCtrlModeFlowSpeed", &InExternalCtrlModeFlowSpeed);
	hh.variable("inExternalCtrlModeFlowNumSections", &InExternalCtrlModeFlowNumSections);

	// Function to be exposed
	hh.function("packet", parsePacket);
	hh.function("setHackNameToCmd", setHackNameToCmd);

	// Give name and ID to device
	//hh.set_id("777");
	hh.set_name("ColorRing");

	// Set up CC3000 and get connected to the wireless network.
	Serial.println(F("\nInitializing the CC3000..."));
	if (!cc3000.begin())
	{
		Serial.println(F("Couldn't begin()! Check your wiring?"));
		while(1);
	}

	Serial.println(F("\nDeleting old connection profiles"));
	if (!cc3000.deleteProfiles()) {
		Serial.println(F("Failed!"));
		while(1);
	}

	Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
	if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
		Serial.println(F("Failed!"));
		while(1);
	}
	Serial.println(F("Connected!"));

	Serial.println(F("Request DHCP"));
	while (!cc3000.checkDHCP())
	{
		delay(100);
	}

	// Print CC3000 IP address. Enable if mDNS doesn't work
	while (! displayConnectionDetails()) {
		delay(1000);
	}

	// Start multicast DNS responder
	if (!mdns.begin("colorring", cc3000)) {
		while(1); 
	}

	// Start server
	httpServer.begin();
	ecmServer.begin();
	Serial.println(F("Server(s) Listening for connections..."));

	// === LED Strips ===
	outStrip.begin();  // all off
	outStrip.show();
	inStrip.begin();  // all off
	inStrip.show();

	Serial.print(F("Free RAM End Setup: ")); Serial.println(getFreeRam(), DEC);

}

void loop() {
  
	//Serial.print("Free RAM loop: "); Serial.println(getFreeRam(), DEC);
  
	// Handle any multicast DNS requests
	mdns.update();

	//Serial.print("Free RAM Before http: "); Serial.println(getFreeRam(), DEC);
	// Handle http calls (for setting modes & stripCmds)
	Adafruit_CC3000_ClientRef httpClient = httpServer.available();
	hh.handle(httpClient);
	//Serial.print("Free RAM After http: "); Serial.println(getFreeRam(), DEC);


	// ===================================================
	// === Handle any EXTERNAL Ctrl Mode (UDP) Packets ===
	// ===================================================
	PixelColor newColor;
	bool isOutside;

	// Outside strip
	byte opModeOutside = ((OpMode & 0xF0) >> 4);
	byte opModeInside = OpMode & 0x0F;
		
	//if (OpMode == OPMODE_OUT_EXTERNAL_IN_INTERNAL || OpMode == OPMODE_OUT_EXTERNAL_IN_EXTERNAL) {	
	if (opModeOutside == OPMODE_EXTERNAL) {
		isOutside = true;
		strip = &outStrip;
		
		if (OutExternalCtrlMode == EXTERNALCTRLMODE_STRIPCOLOR) {
			//EXTERNALCTRLMODE_STRIPCOLOR
			
			if (ecmServer.handleNewColorPacket(newColor, isOutside)) {
				byte numColorsInSeries = 1;
				PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = newColor;
				SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				ecmSsp.exec(SHOWSTRIP);
			}
		} else {
			// EXTERNALCTRLMODE_FLOW
			
			if (ecmServer.handleNewColorPacket(newColor, isOutside)) {
				lastOecmFlowColor = newColor;
			}
			
			if (millis() - lastOecmFlowMs > OutExternalCtrlModeFlowSpeed) {
				lastOecmFlowMs = millis();
				PixelColor colorSeriesArr[1] = lastOecmFlowColor;
				Flow ecmFlow(strip, 0, NUM_LEDS-1, OutExternalCtrlModeFlowNumSections, 1, 1, 0, 0, 0, CW, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 1, colorSeriesArr);
				ecmFlow.step(SHOWSTRIP); // or exec()
			}
		}
	}
	
	// Inside strip
	//if (OpMode == OPMODE_OUT_INTERNAL_IN_EXTERNAL || OpMode == OPMODE_OUT_EXTERNAL_IN_EXTERNAL) {
	if (opModeInside == OPMODE_EXTERNAL) {
		isOutside = false;
		strip = &inStrip;
		
		if (InExternalCtrlMode == EXTERNALCTRLMODE_STRIPCOLOR) {
			//EXTERNALCTRLMODE_STRIPCOLOR
			
			if (ecmServer.handleNewColorPacket(newColor, isOutside)) {
				byte numColorsInSeries = 1;
				PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = newColor;
				SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				ecmSsp.exec(SHOWSTRIP);
			}
		} else {
			// EXTERNALCTRLMODE_FLOW
			
			if (ecmServer.handleNewColorPacket(newColor, isOutside)) {
				lastIecmFlowColor = newColor;
			}
			
			if (millis() - lastIecmFlowMs > InExternalCtrlModeFlowSpeed) {
				lastIecmFlowMs = millis();
				PixelColor colorSeriesArr[1] = lastIecmFlowColor;
				Flow ecmFlow(strip, 0, NUM_LEDS-1, InExternalCtrlModeFlowNumSections, 1, 1, 0, 0, 0, CW, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 1, colorSeriesArr);
				ecmFlow.step(SHOWSTRIP); // or exec()
			}
		}
	}
	
	// =================================================================
	// === Do INTERNAL (inside & outside strip cmds) - if applicable ===
	// =================================================================
	currentMillis = millis();

	// Outside strip
	//if (OpMode == OPMODE_OUT_INTERNAL_IN_INTERNAL || OpMode == OPMODE_OUT_INTERNAL_IN_EXTERNAL) {
	if (opModeOutside == OPMODE_INTERNAL) {
		if (currentMillis - outPreviousMillis > outAnimDelay) {
			outAnimDelay = stripStep(OUTSIDE_STRIP);
			outPreviousMillis = currentMillis;
		}
	}

	// Inside strip
	//if (OpMode == OPMODE_OUT_INTERNAL_IN_INTERNAL || OpMode == OPMODE_OUT_EXTERNAL_IN_INTERNAL) {
	if (opModeInside == OPMODE_INTERNAL) {
		if (currentMillis - inPreviousMillis > inAnimDelay) {
			inAnimDelay = stripStep(INSIDE_STRIP);
			inPreviousMillis = currentMillis;
		}
	}
}

// Print connection details of the CC3000 chip
bool displayConnectionDetails(void) {
	uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;

	if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv)) {
		Serial.println(F("Unable to retrieve the IP Address!\r\n"));
		return false;
	} else {
		Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
		Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
		Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
		Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
		Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
		Serial.println();
		return true;
	}
}

int parsePacket(String p) {
	p.trim();
	p.replace(" HTTP/", "");  // For some reason, this is appended at end of string. Take it off.
	Serial.print("parsePacket(): "); Serial.println(p);

	int commaPosition;
	int size = 0;
	do {
		commaPosition = p.indexOf(',');
		if(commaPosition != -1) {
			packet[size] = str2int(p.substring(0, commaPosition));
			size++;
			p = p.substring(commaPosition+1, p.length());
		} else {
			if(p.length() > 0) {
				packet[size] = str2int(p);
				size++;
			}
		}

	} while(commaPosition >= 0);

	executePacket();
	
	// Re-set "name" for Http Handler, in case it still has the 128 bytes (or less) of cmdBytes data
	hh.set_name("ColorRing");

	//eepromPrint(0x290, "hex"); // good for debug, but slow'ish...
	
	//Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
	return 1;
}

void executePacket() {
	Serial.println("executePacket()");

	switch (packet[0]) {
		case WIFI_PACKET_SET_OPMODE:  // 0xAA (170)
			
			OpMode = packet[1];
			
			cout << F(" Setting OpMode... to: ") << OpMode << endl;
			
			// Write the opMode out to EEPROM & Global variable
			EEPROM.write(EEP_OPMODE, OpMode);
			
			// Clear both strips
			//   user may or may not want to do this
			/*
			strip = &outStrip;
			clearStrip(strip);
			strip->show();
			strip = &inStrip;
			clearStrip(strip);
			strip->show();
			*/
			
			break;
		case WIFI_PACKET_SET_OUT_EXTERNALCTRLMODE: //0xBB (187)
			
			OutExternalCtrlMode = packet[1];
		
			cout << F(" Setting OutExternalCtrlMode... to: ") << OutExternalCtrlMode << endl;
			
			// save to EEPROM & Global variable
			EEPROM.write(EEP_OUT_EXTERNALCTRLMODE, OutExternalCtrlMode);
			
			switch (OutExternalCtrlMode) {
				case EXTERNALCTRLMODE_STRIPCOLOR:  // 0
					
					break;
				case EXTERNALCTRLMODE_FLOW:  // 1
					int speed = packet[2];
					int numSections = packet[3];
					
					EEPROM.write(EEP_OUT_EXTERNALCTRLMODE_FLOWSPEED, speed);
					EEPROM.write(EEP_OUT_EXTERNALCTRLMODE_FLOWNUMSECTIONS, numSections);
					OutExternalCtrlModeFlowSpeed = speed;
					OutExternalCtrlModeFlowNumSections = numSections;
					
					break;
			}
			break;
		case WIFI_PACKET_SET_IN_EXTERNALCTRLMODE: //0xBC (188)
			
			InExternalCtrlMode = packet[1];
			
			cout << F(" Setting InExternalCtrlMode... to: ") << InExternalCtrlMode << endl;

			// save to EEPROM & Global variable
			EEPROM.write(EEP_IN_EXTERNALCTRLMODE, InExternalCtrlMode);
	
			switch (InExternalCtrlMode) {
				case EXTERNALCTRLMODE_STRIPCOLOR:  // 0
			
					break;
				case EXTERNALCTRLMODE_FLOW:  // 1
					int speed = packet[2];
					int numSections = packet[3];
			
					EEPROM.write(EEP_IN_EXTERNALCTRLMODE_FLOWSPEED, speed);
					EEPROM.write(EEP_IN_EXTERNALCTRLMODE_FLOWNUMSECTIONS, numSections);
					InExternalCtrlModeFlowSpeed = speed;
					InExternalCtrlModeFlowNumSections = numSections;
			
					break;
			}
			break;

		case WIFI_PACKET_SET_STRIPCMD:  // 0xDD (221)
			byte cmdPos;
			cmdPos = packet[1];
			
			cout << F(" Setting StripCmd (cmdPos: ") << (int)cmdPos << F(")...") << endl;
			
			if (cmdPos < MAX_NUM_STRIPCMDS * 2) {
				for (int byteOffset = 0; byteOffset < MAX_STRIPCMD_SIZE; byteOffset++) {
					EEPROM.write(EEP_STRIPCMDS + (cmdPos * MAX_STRIPCMD_SIZE) + byteOffset, packet[2 + byteOffset]);  // Will write more than it needs usually, but that's ok.
				}
				// Read from EEPROM to get new cmd into stripCmds array.
				readStripCmdsFromEEPROM();
			}
			
			break;
	}
}

int setHackNameToCmd(String cmdPosStr) {
	cmdPosStr.replace(" HTTP/", "");
	Serial.print("cmdPosStr: "); Serial.println(cmdPosStr);
	int cmdPos = cmdPosStr.toInt();
	
	String cmdBytesStr = "";
	int val;
	int cmdStartPtr = EEP_STRIPCMDS + (cmdPos * MAX_STRIPCMD_SIZE);
	for (int byteOffset = 0; byteOffset < MAX_STRIPCMD_SIZE; byteOffset++) {
		val = EEPROM.read(cmdStartPtr + byteOffset);
		cmdBytesStr += String(val);
		if (byteOffset < MAX_STRIPCMD_SIZE - 1) {
			cmdBytesStr += ",";
		}
	}

	Serial.print("cmdBytesStr: "); Serial.println(cmdBytesStr);
	hh.set_name(cmdBytesStr);
}

void eepromPrint(int numVals, String base) {
	// base can be either "hex" or "dec"
	
	int address = 0;
	byte val;
	
	Serial.println("Contents of EEPROM:");
	
	// Print headers
	Serial.print("Add\t");
	char buf[8];
	for (int i = 0; i < 16; i++) {
		if (base == "dec") {
			sprintf(buf, "%d", i);
			//Serial.print(i); Serial.print("\t");
		} else {
			sprintf(buf, "%.1X", i);
		}
		Serial.print(buf); Serial.print("\t");
	}
	Serial.println();
	Serial.print("\t");
	for (int i = 0; i < 16; i++) {
		Serial.print("--------");
	}
	Serial.println();
	
	// Print data
	Serial.print("0|\t");
	for (int address = 0; address < numVals; address++) {
		val = EEPROM.read(address);
		//Serial.print(val); Serial.print("\t");

		if (base == "dec") {
			sprintf(buf, "%d", val);
		} else {
			sprintf(buf, "%.2X", val);
		}
		Serial.print(buf); Serial.print("\t");
		
		if (address % 16 == 15) {
			Serial.println();
			if (base == "dec") {
				sprintf(buf, "%d", val);
			} else {
				sprintf(buf, "%X", (address / 16) + 1);
			}
			Serial.print(buf); Serial.print("|\t");
		}
	}
	Serial.println();
}

void clearStrip(Adafruit_NeoPixel* strip) {
	for (uint8_t i = 0; i < strip->numPixels(); i++) {
		strip->setPixelColor(i, 0, 0, 0);
	}
}

int str2int(String s) {
	// converts Arduino "String" to an "int"
	
	int i;
	
	if (s.startsWith("0x")) {
		// Hex String
		char buf[20];
		s.toCharArray(buf, s.length() + 1);
		i = strtoul(buf, NULL, 16);
	} else {
		// Integer String
		i = s.toInt();
	}
	
	return i;
}

int stripStep(boolean isOutside) {
	string inOutStr = isOutside ? "OUTSIDE" : "INSIDE";
	//cout << "**stripStep(" << str << ")" << endl;
	
	if (isOutside) {
		strip = &outStrip;
	} else {
		strip = &inStrip;
	}
	
	long animDelay = 0;
	boolean stepTaken = false;
	
	int startCmdPos = isOutside ? 0 : MAX_NUM_STRIPCMDS;
	int endCmdPos = isOutside ? MAX_NUM_STRIPCMDS-1 : (MAX_NUM_STRIPCMDS * 2) - 1;
	//cout << "startCmdPos: " << startCmdPos << ", endCmdPos: " << endCmdPos << endl;
	
	for (int i = startCmdPos; i <= endCmdPos; i++) {
		if (stripCmds[i]) {
			
			// Debug output
			if (stripCmds[i]->currIteration == 0 && stripCmds[i]->isMoreSteps()) {
				// First time only
				// Keep, maybe.
				//cout << F("Starting cmd '#") << i << F("' (") << stripCmds[i]->getCmdTypeStr() << F(") on '") << stripCmds[i]->inOutStr << F(" strip.") << endl;
				
			}
			
			if (stripCmds[i]->isMoreSteps()) {
				if (!stripCmds[i]->getIsAnim()) {
					stripCmds[i]->exec(SHOWSTRIP);
				} else {
					stripCmds[i]->step(SHOWSTRIP);
					animDelay = stripCmds[i]->getAnimDelay();
				}
				stepTaken = true;
				
				break;  // out of for loop
			}
		}
	}
	if (!stepTaken) {
		// Keep, maybe.
		//cout << "Done with all cmds on " << inOutStr << " strip. (no step taken). RESETTING all cmds for this strip." << endl;

		// Reset all cmds (just for current strip)
		for (int i = startCmdPos; i <= endCmdPos; i++) {
			if (stripCmds[i]) {
				stripCmds[i]->reset();
			}
		}
	}
	
	return animDelay;
}

void readAllFromEEPROM() {
	readOpModeFromEEPROM();
	readEcmsFromEEPROM();
	readStripCmdsFromEEPROM();
}

void readOpModeFromEEPROM() {
	OpMode = EEPROM.read(EEP_OPMODE);
}

void readEcmsFromEEPROM() {
	// Read the External Cntl Mode's (outside & inside) from EEPROM.
	OutExternalCtrlMode = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE);
	OutExternalCtrlModeFlowSpeed = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE_FLOWSPEED);
	OutExternalCtrlModeFlowNumSections = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE_FLOWNUMSECTIONS);
	InExternalCtrlMode = EEPROM.read(EEP_IN_EXTERNALCTRLMODE);
	InExternalCtrlModeFlowSpeed = EEPROM.read(EEP_IN_EXTERNALCTRLMODE_FLOWSPEED);
	InExternalCtrlModeFlowNumSections = EEPROM.read(EEP_IN_EXTERNALCTRLMODE_FLOWNUMSECTIONS);
}

void readStripCmdsFromEEPROM() {
	cout << F("Read StripCmds from EEPROM:") << endl;
	byte stripCmdArray[MAX_STRIPCMD_SIZE];
	
	// OUTSIDE & INSIDE
	boolean isOutside;
	for (int cmdPos = 0; cmdPos < (MAX_NUM_STRIPCMDS * 2); cmdPos++) {
		if (cmdPos < MAX_NUM_STRIPCMDS) {
			isOutside = true;
			strip = &outStrip;
		} else {
			isOutside = false;
			strip = &inStrip;
		}

		int cmdStartPtr = EEP_STRIPCMDS + (cmdPos * MAX_STRIPCMD_SIZE);
		
		for (int byteOffset = 0; byteOffset < MAX_STRIPCMD_SIZE; byteOffset++) {
			stripCmdArray[byteOffset] = EEPROM.read(cmdStartPtr + byteOffset);
		}
		
		// Delete all existing strip cmds.
		delete stripCmds[cmdPos];
		stripCmds[cmdPos] = NULL;
		
		byte cmd = stripCmdArray[0];
		//Serial.print("cmd: "); Serial.println(cmd);
		switch (cmd) {
			case 0:  // SetSeqPixels
				cout << F(" SetSeqPixels(") << cmdPos << ")" << endl;
				stripCmds[cmdPos] = new SetSeqPixels(strip, stripCmdArray);
				break;
			case 1:  // BuildColorGradient
				/*
				cout << F(" BuildColorGradient(") << cmdPos << ")" << endl;
				stripCmds[cmdPos] = new BuildColorGradient(strip, stripCmdArray);
				*/
				break;
			case 2:  // Shift
				cout << F(" Shift(") << cmdPos << ")" << endl;
				stripCmds[cmdPos] = new Shift(strip, stripCmdArray);
				break;
			case 3:  // Flow
				//Serial.println("case3: Flow");
				cout << F(" Flow(") << cmdPos << ")" << endl;
				stripCmds[cmdPos] = new Flow(strip, stripCmdArray);
				break;
		}
	}
}

