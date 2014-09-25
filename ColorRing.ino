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

#include <SPI.h>
#include "utility/debug.h"  // for getFreeRam()
#include "HttpHandler.h"

#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "PixelColor.h"
#include "RTColorServer.h"

#include "SetSeqPixels.h"
#include "Shift.h"
#include "Flow.h"

// FFT
#include <avr/pgmspace.h>
#include <ffft.h>
#include <math.h>
#include "AudioVisualizer.h"

#include "Wire.h" //??? needed for RTClib.h ??? Guess so, for now.
#include "RTClib.h"

#include <StandardCplusplus.h>
#include <serstream>
#include <vector>

using namespace std;

namespace std {
	ohserialstream cout(Serial);
}

#include "AllDefs.h"


#include <Arduino.h>
#ifndef CORE_WILDFIRE 
	#include <ColorRing_CC3000.h>
	#include <ColorRing_CC3000_MDNS.h>
	ColorRing_CC3000 cc3000 = ColorRing_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIV2);
	ColorRing_CC3000_Server httpServer(LISTEN_PORT_HTTP);
#else
	#include <WildFire_CC3000.h>
	#include <WildFire_CC3000_MDNS.h>
	#include <WildFire_CC3000_Server.h>
	WildFire_CC3000 cc3000;
	WildFire_CC3000_Server httpServer(LISTEN_PORT_HTTP);
#endif

//=== DEBUG ===
#define SKIP_CC3000 false  // for normal use, should be false.
	
	
// === FFT ===
int16_t       capture[FFT_N];    // Audio capture buffer
complex_t     bfly_buff[FFT_N];  // FFT "butterfly" buffer
uint16_t      spectrum[FFT_N/2]; // Spectrum output buffer
volatile byte samplePos = 0;     // Buffer position counter
byte
	//peak[8],      // Peak level of each column; used for falling dots
	dotCount = 0, // Frame counter for delaying dot-falling speed
	colCount = 0; // Frame counter for storing past column data
int
	col[8][10],   // Column levels for the prior 10 frames
	minLvlAvg[8], // For dynamic adjustment of low & high ends of graph,
	maxLvlAvg[8], // pseudo rolling averages for the prior few frames.
	colDiv[8];    // Used when filtering FFT output to 8 columns
PROGMEM const uint8_t
	  // This is low-level noise that's subtracted from each FFT output column:
	  noise[64]={ 
		  		  8,6,6,5,3,4,4,4,3,4,4,3,2,3,3,4,
		  		//58,58,58,58,58,58,58,58,3,4,4,3,2,3,3,4,
	              2,1,2,1,3,2,3,2,1,2,3,1,2,3,4,4,
	              3,2,2,2,2,2,2,1,3,2,2,2,2,2,2,2,
	              2,2,2,2,2,2,2,2,2,2,2,2,2,3,3,4 },
	  // These are scaling quotients for each FFT output column, sort of a
	  // graphic EQ in reverse.  Most music is pretty heavy at the bass end.
	  eq[64]={
		  //255, 175,218,225,220,198,147, 99, 68, 47, 33, 22, 14,  8,  4,  2,
		  //0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
		  255,245,224,225,220,198,147, 99, 69, 48, 33, 22, 14,  8,  4,  2,
	      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
	      0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0 },
	  // When filtering down to 8 columns, these tables contain indexes
	  // and weightings of the FFT spectrum output values to use.  Not all
	  // buckets are used -- the bottom-most and several at the top are
	  // either noisy or out of range or generally not good for a graph.
	  col0data[] = {  2,  1,  // # of spectrum bins to merge, index of first
	    111,   8 },           // Weights for each bin
	  col1data[] = {  4,  1,  // 4 bins, starting at index 1
	     19, 186,  38,   2 }, // Weights for 4 bins.  Got it now?
	  col2data[] = {  5,  2,
	     11, 156, 118,  16,   1 },
	  col3data[] = {  8,  3,
	      5,  55, 165, 164,  71,  18,   4,   1 },
	  col4data[] = { 11,  5,
	      3,  24,  89, 169, 178, 118,  54,  20,   6,   2,   1 },
	  col5data[] = { 17,  7,
	      2,   9,  29,  70, 125, 172, 185, 162, 118, 74,
	     41,  21,  10,   5,   2,   1,   1 },
	  col6data[] = { 25, 11,
	      1,   4,  11,  25,  49,  83, 121, 156, 180, 185,
	    174, 149, 118,  87,  60,  40,  25,  16,  10,   6,
	      4,   2,   1,   1,   1 },
	  col7data[] = { 37, 16,
	      1,   2,   5,  10,  18,  30,  46,  67,  92, 118,
	    143, 164, 179, 185, 184, 174, 158, 139, 118,  97,
	     77,  60,  45,  34,  25,  18,  13,   9,   7,   5,
	      3,   2,   2,   1,   1,   1,   1 };
	  // And then this points to the start of the data for each of the columns:
	  //*colData[] = {
	  //  col0data, col1data, col2data, col3data,
	  //  col4data, col5data, col6data, col7data };
PROGMEM const uint8_t* const colData[] = {
	col0data, col1data, col2data, col3data,
	col4data, col5data, col6data, col7data };
// end FFT
		


// Create HTTP Handler instance
HttpHandler hh = HttpHandler();

// Real Time Color Server
RTColorServer rtColorServer(LISTEN_PORT_ECM);


// DNS responder instance
MDNSResponder mdns;


// === PV's (Persistant Variables) => Stored in EEPROM ===
// Accessed via "variable" request (must be an int):
int OpMode;
int OutExternalCtrlMode;
int OutExternalCtrlModeFlowSpeed;
int OutExternalCtrlModeFlowNumSections;
int InExternalCtrlMode;
int InExternalCtrlModeFlowSpeed;
int InExternalCtrlModeFlowNumSections;
int UseNtpServer;  // 1=>true, 0=>false
int TzAdj;  // positive or negative integer
int IsDst;  // 1=>true, 0=>false
int HandSizeHour;
int HandSizeMin;
int HandSizeSec;
int DispHourHandOut;  // 1=>true, 0=>false
int DispHourHandIn;
int DispMinHandOut;
int DispMinHandIn;
int DispSecHandOut;
int DispSecHandIn;
// Clap for Time
int EnableClap;  // 1=>true, 0=>false
int ClapAmpThreshold;  // byte
int ClapMinDelayUntilNext;  // byte
int ClapWindowForNext;  // byte
int ClapShowTimeNumSeconds;  // byte


// Accessed via "byte array" request (anything other than an int)
DateTime CurrTime;
PixelColor OutColored5sColor, InColored5sColor;
PixelColor HandColorHour, HandColorMin, HandColorSec;

byte opModeOutside = 0;
byte opModeInside = 0;

Adafruit_NeoPixel outStrip = Adafruit_NeoPixel(NUM_LEDS, OUT_STRIP_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel inStrip  = Adafruit_NeoPixel(NUM_LEDS, IN_STRIP_PIN,  NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel* strip;

uint8_t packet[40];

int maxNumStripCmds;
int maxStripCmdSize;

vector<LightShowCmd*> stripCmds(MAX_NUM_STRIPCMDS * 2); // Note: 1st half is OUTSIDE strip, 2nd half is INSIDE strip

unsigned long outPreviousMillis = 0;
unsigned long inPreviousMillis = 0;
unsigned long currentMillis = 0;
unsigned long outAnimDelay = 0;
unsigned long inAnimDelay = 0;

unsigned long lastOecmFlowMs = 0;
unsigned long lastIecmFlowMs = 0;
PixelColor lastOecmFlowColor(0,0,0);
PixelColor lastIecmFlowColor(0,0,0);

PixelColor outEcmColor(0,0,0);
PixelColor inEcmColor(0,0,0);

// NTP Server
#ifndef CORE_WILDFIRE
	ColorRing_CC3000_Client ntpClient;
#else
	WildFire_CC3000_Client ntpClient;
#endif
	
const unsigned long
  connectTimeout  = 3L * 1000L, // Max time to wait for server connection
  responseTimeout = 1L * 1000L; // Max time to wait for data from server
bool ntpGrabbedOnce;
unsigned long lastGrabFromNtpSketchTime;
unsigned long lastGrabbedNtpTime;
byte ntpTriesRemaining;
unsigned long lastManualSetTimeMS;
unsigned long lastGrabbedManualTime;
byte lastSecondVal;
//unsigned long currTime;
//DateTime currTime;
//byte timeHours, timeMins, timeSecs;

bool clockRTColorChanged = false;

// Clap
bool isFirstClap = true;
unsigned long firstClapTimeMS = 0;
bool isClapTriggerClockEnabled = false;
unsigned long clapTriggerClockTimeMS = 0;
byte tempClapOpModeOutside = 0;
byte tempClapOpModeInside = 0;


void setup(void)
	{  
	Serial.begin(115200);

	cout << F("Free RAM Start Setup: ") << getFreeRam() << endl;
	
	
	
	// === FFT ===
	uint8_t i, j, nBins, binNum, *data;
	
	//memset(peak, 0, sizeof(peak));
	memset(col , 0, sizeof(col));
	
	for(i=0; i<8; i++) {
		minLvlAvg[i] = 0;
		maxLvlAvg[i] = 512;
		data         = (uint8_t *)pgm_read_word(&colData[i]);
		nBins        = pgm_read_byte(&data[0]) + 2;
		binNum       = pgm_read_byte(&data[1]);
		for(colDiv[i]=0, j=2; j<nBins; j++)
			colDiv[i] += pgm_read_byte(&data[j]);
	}

	//matrix.begin(0x70);

	// Init ADC free-run mode; f = ( 16MHz/prescaler ) / 13 cycles/conversion 
	ADMUX  = ADC_CHANNEL; // Channel sel, right-adj, use AREF pin
	ADCSRA = _BV(ADEN)  | // ADC enable
		_BV(ADSC)  | // ADC start
		_BV(ADATE) | // Auto trigger
		_BV(ADIE)  | // Interrupt enable
		_BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // 128:1 / 13 = 9615 Hz
	ADCSRB = 0;                // Free run mode, no high MUX bit
	DIDR0  = 1 << ADC_CHANNEL; // Turn off digital input for ADC pin
	//////TIMSK0 = 0;                // Timer0 off

	sei(); // Enable interrupts
	// === end FFT ===
	

	// === LED Strips ===
	outStrip.begin();  // all off
	outStrip.show();
	inStrip.begin();  // all off
	inStrip.show();

	// Setup "show progress" steps during setup() function.
	byte setupStep = 0;
	strip = &outStrip;
	clearStrip(strip);
	showSetupProgress(setupStep++);

	// Print out the EEPROM
	eepromPrint(0x2E0, "hex");

	// Probably don't NEED to do this, but good just to have a sure starting point.
	// Clear stripCmds array.
	for (int i = 0; i < MAX_NUM_STRIPCMDS * 2; i++) {
		stripCmds[i] = NULL;
	}

	// Read all data from EEPROM
	readAllFromEEPROM();
	showSetupProgress(setupStep++);
	
	// Init opMode's
	opModeOutside = ((OpMode & 0xF0) >> 4);
	opModeInside = OpMode & 0x0F;
	tempClapOpModeOutside = opModeOutside;
	tempClapOpModeInside = opModeInside;

	// Init variables and expose them to HTTP Handler
	// NOTE: REMEMBER TO CHANGE "#define NUMBER_VARIABLES" (in HttpHandler.h)!
	//   Note: After adding variable here, also modify:
	//		x- readXYZFromEEPROM()
	//		- realAllFromEEPROM()
	//		- executePacket()
	//		- AllDefs.h
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
	hh.variable("enableClap", &EnableClap);
	hh.variable("clapAmpThreshold", &ClapAmpThreshold);
	hh.variable("clapMinDelayUntilNext", &ClapMinDelayUntilNext);
	hh.variable("clapWindowForNext", &ClapWindowForNext);
	hh.variable("clapShowTimeNumSeconds", &ClapShowTimeNumSeconds);
	hh.variable("useNtpServer", &UseNtpServer);
	hh.variable("tzAdj", &TzAdj);
	hh.variable("isDst", &IsDst);
	hh.variable("handSizeHour", &HandSizeHour);
	hh.variable("handSizeMin", &HandSizeMin);
	hh.variable("handSizeSec", &HandSizeSec);
	hh.variable("dispHourHandOut", &DispHourHandOut);
	hh.variable("dispHourHandIn", &DispHourHandIn);
	hh.variable("dispMinHandOut", &DispMinHandOut);
	hh.variable("dispMinHandIn", &DispMinHandIn);
	hh.variable("dispSecHandOut", &DispSecHandOut);
	hh.variable("dispSecHandIn", &DispSecHandIn);
	// Function to be exposed
	hh.function("packet", parsePacket);
	hh.function("setHackNameToCmd", setHackNameToCmd);
	hh.function("setHackNameToColor", setHackNameToColor);
	hh.function("setHackNameToTime", setHackNameToTime);

	// Give name and ID to device
	//hh.set_id("777");
	hh.set_name("ColorRing");
	showSetupProgress(setupStep++);
	
	
	if (!SKIP_CC3000) {
		// Set up CC3000 and get connected to the wireless network.
		Serial.println(F("\nInitializing the CC3000..."));
		if (!cc3000.begin())
		{
			Serial.println(F("Couldn't begin()! Check your wiring?"));
			while(1);
		}
		showSetupProgress(setupStep++);

		Serial.println(F("\nDeleting old connection profiles"));
		if (!cc3000.deleteProfiles()) {
			Serial.println(F("Failed!"));
			while(1);
		}
		showSetupProgress(setupStep++);

		Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
		if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
			Serial.println(F("Failed!"));
			while(1);
		}
		Serial.println(F("Connected!"));
		showSetupProgress(setupStep++);

		//cc3000.setDHCP();

		Serial.println(F("Request DHCP"));
		while (!cc3000.checkDHCP())
		{
			delay(100);
		}
		showSetupProgress(setupStep++);

		// Print CC3000 IP address. Enable if mDNS doesn't work
		while (! displayConnectionDetails()) {
			delay(1000);
		}

		// Start multicast DNS responder
		if (!mdns.begin("colorring", cc3000)) {
			while(1); 
		}
		showSetupProgress(setupStep++);

		// Start server
		httpServer.begin();
		rtColorServer.begin();
		Serial.println(F("Server(s) Listening for connections..."));
		showSetupProgress(setupStep++);
	
		// NTP Server
		CurrTime = DateTime(0);
		ntpGrabbedOnce = false;
		ntpTriesRemaining = NTP_NUM_TRIES;
		lastGrabFromNtpSketchTime = 0;
		lastGrabbedNtpTime = 0;
		lastManualSetTimeMS = 0;
		lastGrabbedManualTime = 0;
		lastSecondVal = 0;
	}

	Serial.print(F("Free RAM End Setup: ")); Serial.println(getFreeRam(), DEC);

}

void loop() {
	//bool isOutside;
	//byte opModeOutside = ((OpMode & 0xF0) >> 4);
	//byte opModeInside = OpMode & 0x0F;
	
	//Serial.print("Free RAM loop: "); Serial.println(getFreeRam(), DEC);
	if (!SKIP_CC3000) {
		// Handle any multicast DNS requests
		mdns.update();
	
		//Serial.print("Free RAM Before http: "); Serial.println(getFreeRam(), DEC);
	
		// Handle http calls (for setting modes & stripCmds)
		#ifndef CORE_WILDFIRE
			ColorRing_CC3000_ClientRef httpClient = httpServer.available();
		#else
			WildFire_CC3000_ClientRef httpClient = httpServer.available();
		#endif
	
		hh.handle(httpClient);
	
		//Serial.print("Free RAM After http: "); Serial.println(getFreeRam(), DEC);
	
	
		// ===============================================
		// === Handle all RealTime Color packets (UDP) ===
		// ===============================================
		PixelColor newColor;
		byte colorUsage = COLOR_USAGE_NONE;
		if (rtColorServer.handleNewColorPacket(newColor, colorUsage)) {
			switch (colorUsage) {
				case COLOR_USAGE_OUT_ECM:
					outEcmColor = newColor;
					break;
				case COLOR_USAGE_IN_ECM:
					inEcmColor = newColor;
					break;
				case COLOR_USAGE_OUT_COLORED5S:
					OutColored5sColor = newColor;
					break;
				case COLOR_USAGE_IN_COLORED5S:
					InColored5sColor = newColor;
					break;
				case COLOR_USAGE_HOUR_HAND:
					HandColorHour = newColor;
					clockRTColorChanged = true;
					break;
				case COLOR_USAGE_MIN_HAND:
					HandColorMin = newColor;
					clockRTColorChanged = true;
					break;
				case COLOR_USAGE_SEC_HAND:
					HandColorSec = newColor;
					clockRTColorChanged = true;
					break;
			}
			// Note: for the PV's (Persistant Variables), DON'T update the EEPROM each time. Would probably burn out EEPROM too fast. Asking user to click Submit button to "lock it in".
		}

		// ===========================================================
		// === Handle EXTERNAL Ctrl Mode (inside & outside strips) ===
		// ===========================================================
		
		// Outside strip
		if (opModeOutside == OPMODE_EXTERNAL) {
			strip = &outStrip;
		
			if (OutExternalCtrlMode == EXTERNALCTRLMODE_STRIPCOLOR) {
				//EXTERNALCTRLMODE_STRIPCOLOR
				byte numColorsInSeries = 1;
				PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = outEcmColor;
				//SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				ecmSsp.exec(SHOWSTRIP);
			} else {
				// EXTERNALCTRLMODE_FLOW
				lastOecmFlowColor = outEcmColor;
			
				if (millis() - lastOecmFlowMs > (unsigned long)OutExternalCtrlModeFlowSpeed) {
					lastOecmFlowMs = millis();
					PixelColor colorSeriesArr[1] = lastOecmFlowColor;
					Flow ecmFlow(strip, 0, NUM_LEDS-1, OutExternalCtrlModeFlowNumSections, 1, 1, 0, 0, 0, CW, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 1, colorSeriesArr);
					ecmFlow.step(SHOWSTRIP); // or exec()
				}
			}
		}
	
		// Inside strip
		if (opModeInside == OPMODE_EXTERNAL) {
			strip = &inStrip;
		
			if (InExternalCtrlMode == EXTERNALCTRLMODE_STRIPCOLOR) {
				//EXTERNALCTRLMODE_STRIPCOLOR
				byte numColorsInSeries = 1;
				//PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = newColor;
				PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = inEcmColor;
				//SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				SetSeqPixels ecmSsp(strip, 0, NUM_LEDS, 1, 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
				ecmSsp.exec(SHOWSTRIP);
			} else {
				// EXTERNALCTRLMODE_FLOW
				lastIecmFlowColor = inEcmColor;
			
				if (millis() - lastIecmFlowMs > (unsigned long)InExternalCtrlModeFlowSpeed) {
					lastIecmFlowMs = millis();
					PixelColor colorSeriesArr[1] = lastIecmFlowColor;
					Flow ecmFlow(strip, 0, NUM_LEDS-1, InExternalCtrlModeFlowNumSections, 1, 1, 0, 0, 0, CW, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 1, colorSeriesArr);
					ecmFlow.step(SHOWSTRIP); // or exec()
				}
			}
		}
	}
	
	// =================================================================
	// === Do INTERNAL (inside & outside strip cmds) - if applicable ===
	// =================================================================
	currentMillis = millis();

	// Outside strip
	if (opModeOutside == OPMODE_INTERNAL) {
		if (currentMillis - outPreviousMillis > outAnimDelay) {
			outAnimDelay = stripStep(OUTSIDE_STRIP);
			outPreviousMillis = currentMillis;
		}
	}

	// Inside strip
	if (opModeInside == OPMODE_INTERNAL) {
		if (currentMillis - inPreviousMillis > inAnimDelay) {
			inAnimDelay = stripStep(INSIDE_STRIP);
			inPreviousMillis = currentMillis;
		}
	}
	
	
	// =============
	// === CLOCK ===
	// =============
	if (opModeOutside == OPMODE_CLOCK || opModeInside == OPMODE_CLOCK) {
		if (UseNtpServer) {
			if (!ntpGrabbedOnce || millis() - lastGrabFromNtpSketchTime > NTP_GRAB_FREQ_MS) {
				if (ntpTriesRemaining > 0) {
			
					unsigned long t = getTime();  // Query time server
					if (t) {
						Serial.println(F("SUCCESSFULLY GRABBED TIME FROM NTP SERVER!"));
						ntpGrabbedOnce = true;
						lastGrabFromNtpSketchTime = millis();
						lastGrabbedNtpTime = t;
						ntpTriesRemaining = NTP_NUM_TRIES;
					
						//Debug
						DateTime dt = DateTime(t);
						Serial.print(dt.hour()); Serial.print(F(":"));
						Serial.print(dt.minute()); Serial.print(F(":"));
						Serial.println(dt.second());
					
					} else {
						ntpTriesRemaining--;
						Serial.print(F("FAILED TO GRAB TIME FROM NTP SERVER. "));
						Serial.print(ntpTriesRemaining);
						Serial.println(F(" tries remaining."));
						delay(2000);  // pause for a bit. Will usually never hit this code, but if does, only once / day.
					}
				} else {
					Serial.println(F("NTP SERVER GRAB TOTALLY FAILED. GIVING UP (until next try)."));
					// Pretend that we got the time, so we don't keep trying
					ntpGrabbedOnce = true;
					lastGrabFromNtpSketchTime = millis();
					ntpTriesRemaining = NTP_NUM_TRIES;
				}
			}
			
			CurrTime = DateTime(lastGrabbedNtpTime + (TzAdj * 60 * 60) + (IsDst * 60 * 60) + (millis() - lastGrabFromNtpSketchTime) / 1000);
		
			//Debug
			//Serial.print(CurrTime.hour()); Serial.print(F(":"));
			//Serial.print(CurrTime.minute()); Serial.print(F(":"));
			//Serial.println(CurrTime.second());
		} else {
			// Time must be set manually

			// Debug
			//Serial.print(F("unixtime: ")); Serial.println(CurrTime.unixtime());
			//Serial.print(F("lastManualSetTimeMS: ")); Serial.println(lastManualSetTimeMS);
			//Serial.print(F("millis: ")); Serial.println(millis());
			//Serial.print(F("math: ")); Serial.println(CurrTime.unixtime() + (millis() - lastManualSetTimeMS) / 1000);
			
			CurrTime = DateTime(lastGrabbedManualTime + (millis() - lastManualSetTimeMS) / 1000);
			
			//Debug
			//Serial.print(CurrTime.hour()); Serial.print(F(":"));
			//Serial.print(CurrTime.minute()); Serial.print(F(":"));
			//Serial.println(CurrTime.second());
		}
		
		displayTimeToStrips();
	}
	
	// ===================
	// === COLORED 5's ===
	// ===================
	if (opModeOutside == OPMODE_COLORED5S || opModeInside == OPMODE_COLORED5S) {
		byte startPixelNum = 0;
		byte numPixelsEachColor = 1;
		byte colorSeriesNumIter = 12;
		byte numPixelsToSkip = 4;
		word numIter = ITER_ENOUGH;
		word animDelay = 0;
		word pauseAfter = 0;
		
		bool destructive = true;
		bool direction = CW;
		bool isAnim = false;
		bool clearStripBefore = true;
		bool gradiate = false;
		bool gradiateLastPixelFirstColor = false;
		
		byte numColorsInSeries = 1;
		PixelColor colorSeriesArr[numColorsInSeries];
	
		if (opModeOutside == OPMODE_COLORED5S) {
			strip = &outStrip;
			colorSeriesArr[0] = OutColored5sColor;
			SetSeqPixels ssp(strip, startPixelNum, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, numIter, animDelay, pauseAfter, destructive, direction, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
			ssp.exec(SHOWSTRIP);
		}
	
		if (opModeInside == OPMODE_COLORED5S) {
			strip = &inStrip;
			colorSeriesArr[0] = InColored5sColor;
			SetSeqPixels ssp(strip, startPixelNum, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, numIter, animDelay, pauseAfter, destructive, direction, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
			ssp.exec(SHOWSTRIP);
		}
	}
	
	// ==========================
	// === Microphone related ===
	// ==========================
	
	// ==============================
	// === Audio Visualizer / FFT ===
	// ==============================
	//Serial.println("Audio Visualizer loop");
	if (opModeOutside == OPMODE_AUDIOVISUALIZER || opModeInside == OPMODE_AUDIOVISUALIZER ||
		opModeOutside == OPMODE_AUDIOLEVEL 		|| opModeInside == OPMODE_AUDIOLEVEL	  ||
		EnableClap == true || isClapTriggerClockEnabled == true) {
		//Serial.println("MIC related");
		//uint8_t  i, x, L, *data, nBins, binNum, weighting, c;
		uint8_t  i, x, L, *data, nBins, binNum, c;
		uint16_t minLvl, maxLvl;
		//int16_t minLvl, maxLvl;
		//int      level, y, sum;
		int      level, sum;

		while(ADCSRA & _BV(ADIE)); // Wait for audio sampling to finish

		fft_input(capture, bfly_buff);   // Samples -> complex #s
		samplePos = 0;                   // Reset sample counter
		ADCSRA |= _BV(ADIE);             // Resume sampling interrupt
		fft_execute(bfly_buff);          // Process complex data
		fft_output(bfly_buff, spectrum); // Complex -> spectrum

		// Remove noise and apply EQ levels
		for(x=0; x<FFT_N/2; x++) {
			L = pgm_read_byte(&noise[x]);
			spectrum[x] = (spectrum[x] <= L) ? 0 : (((spectrum[x] - L) * (256L - pgm_read_byte(&eq[x]))) >> 8);
		}

		AudioVisualizer aVis = AudioVisualizer();  // init & clear
	
		//Serial.print("========== Frame (colCount): "); Serial.println(colCount);
	
		// Downsample spectrum output to 8 columns:
		for(x=0; x<8; x++) {
			data   = (uint8_t *)pgm_read_word(&colData[x]);
			nBins  = pgm_read_byte(&data[0]) + 2;
			binNum = pgm_read_byte(&data[1]);
			for(sum=0, i=2; i<nBins; i++)
				sum += spectrum[binNum++] * pgm_read_byte(&data[i]); // Weighted
			col[x][colCount] = sum / colDiv[x];                    // Average
			minLvl = maxLvl = col[x][0];
			for(i=1; i<10; i++) { // Get range of prior 10 frames
				if(col[x][i] < minLvl)      minLvl = col[x][i];
				else if(col[x][i] > maxLvl) maxLvl = col[x][i];
			}
			//Serial.print("minLvl:"); Serial.print(minLvl); Serial.print(", maxLvl:"); Serial.println(maxLvl);
			
			// minLvl and maxLvl indicate the extents of the FFT output, used
			// for vertically scaling the output graph (so it looks interesting
			// regardless of volume level).  If they're too close together though
			// (e.g. at very low volume levels) the graph becomes super coarse
			// and 'jumpy'...so keep some minimum distance between them (this
			// also lets the graph go to zero when no sound is playing):
			if((maxLvl - minLvl) < 8) maxLvl = minLvl + 8;
			minLvlAvg[x] = (minLvlAvg[x] * 7 + minLvl) >> 3; // Dampen min/max levels
			maxLvlAvg[x] = (maxLvlAvg[x] * 7 + maxLvl) >> 3; // (fake rolling average)
			//Serial.print("minLvlAvgX:"); Serial.print(minLvlAvg[x]); Serial.print(", maxLvlAvgX:"); Serial.println(maxLvlAvg[x]);

			//Serial.print("col[x][colCount]:"); Serial.println(col[x][colCount]);
			
			// Second fixed-point scale based on dynamic min/max levels:
			level = 10L * (col[x][colCount] - minLvlAvg[x]) / (long)(maxLvlAvg[x] - minLvlAvg[x]);

			//Serial.print("x:"); Serial.print(x); Serial.print(", level:"); Serial.println(level);
			
			// Clip output and convert to byte:
			if(level < 0L)      c = 0;
			else if(level > 10) c = 10; // Allow dot to go a couple pixels off top
			else                c = (uint8_t)level;

			//if(c > peak[x]) peak[x] = c; // Keep dot on top

			aVis.fillLine(x, c);  // Fill Section 'x' up to 'c' LEDs

			//aVis.fillPeakPixel(x, peak[x]);
		}

		if (opModeOutside == OPMODE_AUDIOVISUALIZER) {
			strip = &outStrip;
			aVis.writeToStrip(strip);
			strip->show();
		}
		if (opModeInside == OPMODE_AUDIOVISUALIZER) {
			strip = &inStrip;
			aVis.writeToStrip(strip);
			strip->show();
		}
	
		/*
		// Every third frame, make the peak pixels drop by 1:
		if(++dotCount >= 3) {
			dotCount = 0;
			for(x=0; x<8; x++) {
				if(peak[x] > 0) peak[x]--;
			}
		}
		*/
		
		if(++colCount >= 10) colCount = 0;
		
		// ===================
		// === Audio Level ===
		// ===================
		// NOTE: Doing audio HERE because analogRead() does not work (freezes) when the ADC interrupts are enabled (which are used for FFT).
		//		This is unfortunate, 'cuz the FFT stuff is way overkill just to get amplitude, but it still works (pretty well).
		if (opModeOutside == OPMODE_AUDIOLEVEL || opModeInside == OPMODE_AUDIOLEVEL) {
				
			//Serial.println("Audio Level");
			float amp = aVis.getAmplitude();  // as %'age (0-100)
			//Serial.print("amp:"); Serial.println(amp);
			byte stripLevel = (byte)(amp / 100.0 * 60);
			//Serial.print("stripLevel:"); Serial.println(stripLevel);
			
			if (opModeOutside == OPMODE_AUDIOLEVEL) {
				strip = &outStrip;
			}
			if (opModeInside == OPMODE_AUDIOLEVEL) {
				strip = &inStrip;
			}
			
			clearStrip(strip);
		
			PixelColor colorSeriesArr[6];
			colorSeriesArr[0] = BLUE;
			colorSeriesArr[1] = GREEN;
			colorSeriesArr[2] = YELLOW;
			colorSeriesArr[3] = ORANGE;
			colorSeriesArr[4] = VIOLET;
			colorSeriesArr[5] = RED;
			SetSeqPixels *audioLevelSsp = new SetSeqPixels(strip, 0, 10, 1, 0, stripLevel, 0, 0, DESTRUCTIVE, CW, NONANIMATED, CLEAR, GRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 6, colorSeriesArr); //Red at beginning
			//SetSeqPixels *audioLevelSsp = new SetSeqPixels(strip, stripLevel, 10, 1, 0, stripLevel, 0, 0, DESTRUCTIVE, CCW, NONANIMATED, CLEAR, GRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, 6, colorSeriesArr); // Red at end
			audioLevelSsp->exec(SHOWSTRIP);
			delete(audioLevelSsp);
		}
		
		// =====================
		// === Clap for Time ===
		// =====================
		// Turn off the triggered clock if it's done showing it's time
		if (isClapTriggerClockEnabled && millis() - clapTriggerClockTimeMS > (unsigned int)ClapShowTimeNumSeconds * 1000) {
			isClapTriggerClockEnabled = false;
			opModeOutside = tempClapOpModeOutside;
			opModeInside = tempClapOpModeInside;
		}
		
		if (EnableClap && !isClapTriggerClockEnabled) {
			const byte clapLengthMS = 100;
			//Serial.println("Clap is Enabled!");
			float amp = aVis.getAmplitude();  // as %'age (0-100)
			//Serial.print("amp:"); Serial.println(amp);
			byte stripLevel = (byte)(amp / 100.0 * 60);
			//if (stripLevel >= 2) {
			//	Serial.print("stripLevel:"); Serial.print(stripLevel); Serial.print(", currTimeMS:"); Serial.println(millis());
			//}
			
			// Make sure clapLength has passed before moving on to 2nd clap
			if (!isFirstClap && millis() - firstClapTimeMS < clapLengthMS) {
				return;  // go on to next iteration of loop()
			}
			
			// Check for timeout after 1st clap
			word maxTimeForBothClapsMS = ClapMinDelayUntilNext*10 + ClapWindowForNext*10;
			if (!isFirstClap && (millis() - firstClapTimeMS > maxTimeForBothClapsMS)) {
				Serial.println("timed out waiting for 2nd clap");
				isFirstClap = true;
			}
			
			if (stripLevel >= ClapAmpThreshold) {
				if (isFirstClap) {
					Serial.println("1st Clap");
					firstClapTimeMS = millis();
					
					//Serial.print(" delaying "); Serial.print(clapLengthMS); Serial.println("ms");
					//delay(clapLengthMS);  // probably shoudln't hardcode delay like this...
					
					isFirstClap = false;  // on to 2nd clap
				} else {
					Serial.println("2nd clap");
					// 2nd Clap
					word delayMS = millis() - firstClapTimeMS;
					if (delayMS > ((unsigned int)ClapMinDelayUntilNext*10) && (delayMS <= maxTimeForBothClapsMS)) {  // *10 to get to MS
						// Trigger - show time
						Serial.println("**Trigger Show Time**");
						clapTriggerClockTimeMS = millis();
						isClapTriggerClockEnabled = true;
						tempClapOpModeOutside = opModeOutside;
						tempClapOpModeInside = opModeInside;
						opModeOutside = OPMODE_CLOCK;
						opModeInside = OPMODE_CLOCK;
					} else {
						Serial.println("2nd clap not within window (clap came too fast):");
						Serial.print(F(" firstClapTimeMS:")); Serial.print(firstClapTimeMS); Serial.print(F("delayMS:")); Serial.println(delayMS);
						Serial.print(F(" ClapMinDelayUntilNext(MS):")); Serial.print(ClapMinDelayUntilNext*10); Serial.print(F("ClapWindowForNext(MS):")); Serial.println(ClapWindowForNext*10);
					}
					
					isFirstClap = true;
				}
			}
		}
	}
	// === end MICROPHONE related ===
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
		case WIFI_PACKET_DUMP_EEPROM_SM:  //0x90 (144)
			cout << F(" Dumping EEPROM to Serial Monitor:") << endl;
			
			eepromPrint(0x2E0, "hex");
			
			break;
		case WIFI_PACKET_SET_OPMODE:  // 0xAA (170)
			
			OpMode = packet[1];
			
			cout << F(" Setting OpMode... to: ") << OpMode << endl;
			
			// Write the opMode out to EEPROM & Global variable
			EEPROM.write(EEP_OPMODE, OpMode);
			
			// Update opMode vars
			opModeOutside = ((OpMode & 0xF0) >> 4);
			opModeInside = OpMode & 0x0F;
			
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
		case WIFI_PACKET_SET_USE_NTP_SERVER:  // 0xC0 (192)
			cout << F(" Setting UseNtpServer...") << endl;
			
			UseNtpServer = packet[1];
			
			EEPROM.write(EEP_USE_NTP_SERVER, UseNtpServer);
			
			break;
		case WIFI_PACKET_SET_TZ_ADJ:  // 0xC2 (194)
			cout << F(" Setting TzAdj...") << endl;
			
			
			//byte val = packet[1];  // for some reason, can't put this all on 1 line.??? 
			byte val;
			val = packet[1];
			
			if (val >= 128) {  // if negative
				TzAdj = val - 0xFF - 1;
			} else {
				TzAdj = val;
			}
			
			//Serial.print(F("TzAdj: ")); Serial.println(TzAdj);
			EEPROM.write(EEP_TZ_ADJ, TzAdj);
			
			break;
		case WIFI_PACKET_SET_IS_DST:  // 0xC3 (195)
			cout << F(" Setting IsDst...") << endl;
		
			IsDst = packet[1];
			EEPROM.write(EEP_IS_DST, IsDst);
		
			break;
		case WIFI_PACKET_SET_HAND_SIZES:  // 0xC4 (196)
			cout << F(" Setting HandSizes...") << endl;
	
			HandSizeHour = packet[1];
			HandSizeMin = packet[2];
			HandSizeSec = packet[3];
			
			EEPROM.write(EEP_HANDSIZE_HOUR, HandSizeHour);
			EEPROM.write(EEP_HANDSIZE_MIN, HandSizeMin);
			EEPROM.write(EEP_HANDSIZE_SEC, HandSizeSec);
	
			break;
		case WIFI_PACKET_SET_HAND_COLORS:  // 0xC5 (197)
			cout << F(" Setting HandColors...") << endl;

			HandColorHour = PixelColor(packet[1], packet[2], packet[3]);
			HandColorMin = PixelColor(packet[4], packet[5], packet[6]);
			HandColorSec = PixelColor(packet[7], packet[8], packet[9]);
		
			EEPROM.write(EEP_HANDCOLOR_HOUR, HandColorHour.R);	EEPROM.write(EEP_HANDCOLOR_HOUR+1, HandColorHour.G);	EEPROM.write(EEP_HANDCOLOR_HOUR+2, HandColorHour.B);
			EEPROM.write(EEP_HANDCOLOR_MIN, HandColorMin.R);	EEPROM.write(EEP_HANDCOLOR_MIN+1, HandColorMin.G);		EEPROM.write(EEP_HANDCOLOR_MIN+2, HandColorMin.B);
			EEPROM.write(EEP_HANDCOLOR_SEC, HandColorSec.R);	EEPROM.write(EEP_HANDCOLOR_SEC+1, HandColorSec.G);		EEPROM.write(EEP_HANDCOLOR_SEC+2, HandColorSec.B);

			break;
		case WIFI_PACKET_SET_DISP_CLOCK_HANDS_OUT_IN:  //0xC6 (198)
			cout << F(" Setting DispClockHand (Outside and Inside)...") << endl;
			
			DispHourHandOut = packet[1];
			DispHourHandIn = packet[2];
			DispMinHandOut = packet[3];
			DispMinHandIn = packet[4];
			DispSecHandOut = packet[5];
			DispSecHandIn = packet[6];
			
			EEPROM.write(EEP_HOUR_HAND_OUT, DispHourHandOut);
			EEPROM.write(EEP_HOUR_HAND_IN, DispHourHandIn);
			EEPROM.write(EEP_MIN_HAND_OUT, DispMinHandOut);
			EEPROM.write(EEP_MIN_HAND_IN, DispMinHandIn);
			EEPROM.write(EEP_SEC_HAND_OUT, DispSecHandOut);
			EEPROM.write(EEP_SEC_HAND_IN, DispSecHandIn);
			
			break;
		case WIFI_PACKET_SET_TIME:  // 0xC1 (193)
			cout << F(" Setting Time...") << endl;
			
			//cout << "p1:" << (int)packet[1] << endl;
			//cout << "p2:" << (int)packet[2] << endl;
			//cout << "p3:" << (int)packet[3] << endl;
			
			CurrTime = DateTime(CurrTime.year(), CurrTime.month(), CurrTime.day(), packet[1], packet[2], packet[3]);
			
			lastManualSetTimeMS = millis();
			lastGrabbedManualTime = CurrTime.unixtime();
		
			break;
		case WIFI_PACKET_SET_OUT_COLORED5S_COLOR:  //0xBD (189)
			cout << F(" Setting OutColored5sColor...") << endl;
			
			// save to EEPROM & Global variable
			OutColored5sColor = PixelColor(packet[1], packet[2], packet[3]);

			EEPROM.write(EEP_OUT_COLORED5S_COLOR + 0, OutColored5sColor.R);
			EEPROM.write(EEP_OUT_COLORED5S_COLOR + 1, OutColored5sColor.G);
			EEPROM.write(EEP_OUT_COLORED5S_COLOR + 2, OutColored5sColor.B);
			
			break;
			
		case WIFI_PACKET_SET_IN_COLORED5S_COLOR:  //0xBE (190)
			cout << F(" Setting InColored5sColor...") << endl;
		
			// save to EEPROM & Global variable
			InColored5sColor = PixelColor(packet[1], packet[2], packet[3]);

			EEPROM.write(EEP_IN_COLORED5S_COLOR + 0, InColored5sColor.R);
			EEPROM.write(EEP_IN_COLORED5S_COLOR + 1, InColored5sColor.G);
			EEPROM.write(EEP_IN_COLORED5S_COLOR + 2, InColored5sColor.B);
		
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
		case WIFI_PACKET_SET_CLAP_PARAMS:  // 0xE0 (224)
			EnableClap = packet[1];
			ClapAmpThreshold = packet[2];
			ClapMinDelayUntilNext = packet[3];
			ClapWindowForNext = packet[4];
			ClapShowTimeNumSeconds = packet[5];
	
			Serial.println(F(" Setting Clap Params..."));
		
			// save to EEPROM & Global variable
			EEPROM.write(EEP_ENABLE_CLAP, EnableClap);
			EEPROM.write(EEP_CLAP_AMP_THRESHOLD, ClapAmpThreshold);
			EEPROM.write(EEP_CLAP_MIN_DELAY_UNTIL_NEXT, ClapMinDelayUntilNext);
			EEPROM.write(EEP_CLAP_WINDOW_FOR_NEXT, ClapWindowForNext);
			EEPROM.write(EEP_CLAP_SHOW_TIME_NUM_SECONDS, ClapShowTimeNumSeconds);
			
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
	
	return 1;
}

int setHackNameToColor(String desiredColorConst) {  // e.g. "0", "1", etc.
	String colorStr;
	PixelColor pc;
	
	int colInt = desiredColorConst.toInt();
	switch (colInt) {
		case OUT_COLORED5S_COLOR:
			pc = OutColored5sColor;
			break;
		case IN_COLORED5S_COLOR:
			pc = InColored5sColor;
			break;
		case HANDCOLOR_HOUR:
			pc = HandColorHour;
			break;
		case HANDCOLOR_MIN:
			pc = HandColorMin;
			break;
		case HANDCOLOR_SEC:
			pc = HandColorSec;
			break;
	}
	
	colorStr = String(pc.R) + "," + String(pc.G) + "," + String(pc.B);
	
	hh.set_name(colorStr);
	
	return 1;
}

int setHackNameToTime(String na) {
	String timeStr = "";
	
	timeStr += CurrTime.hour(); timeStr += ",";
	timeStr += CurrTime.minute(); timeStr += ",";
	timeStr += CurrTime.second();
	
	hh.set_name(timeStr);
	
	return 1;
}

void eepromPrint(int numVals, String base) {
	// base can be either "hex" or "dec"
	
	//int address = 0;
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
	string inOutStr = isOutside ? "OUTSIDE" : "INSIDE";  // Keep for reference.
	//cout << "**stripStep(" << inOutStr << ")" << endl;
	
	if (isOutside) {
		strip = &outStrip;
	} else {
		strip = &inStrip;
	}
	
	long animDelay = 0;
	boolean stepTaken = false;
	
	int startCmdPos = isOutside ? 0 : MAX_NUM_STRIPCMDS;
	int endCmdPos = isOutside ? MAX_NUM_STRIPCMDS-1 : (MAX_NUM_STRIPCMDS * 2) - 1;
	//cout << "startCmdPos: " << (int)startCmdPos << ", endCmdPos: " << (int)endCmdPos << endl;
	
	for (int i = startCmdPos; i <= endCmdPos; i++) {
		if (stripCmds[i]) {
			// Debug output
			// Keep.
			//if (stripCmds[i]->currIteration == 0 && stripCmds[i]->isMoreSteps()) {
				// First time only
				//cout << F("Starting cmd '#") << i << F("' (") << stripCmds[i]->getCmdTypeStr() << F(") on '") << inOutStr << F(" strip.") << endl;
			//}
			
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
		// Keep.
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
	readStripCmdsFromEEPROM();

	// Read all PV's (Persistant Variables) from EEPROM
	byte r,g,b;
	
	// OpMode
	OpMode = EEPROM.read(EEP_OPMODE);
		// also set opMode vars
		//opModeOutside = ((OpMode & 0xF0) >> 4);
		//opModeInside = OpMode & 0x0F;
	
	// ECM's
	OutExternalCtrlMode = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE);
	OutExternalCtrlModeFlowSpeed = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE_FLOWSPEED);
	OutExternalCtrlModeFlowNumSections = EEPROM.read(EEP_OUT_EXTERNALCTRLMODE_FLOWNUMSECTIONS);
	InExternalCtrlMode = EEPROM.read(EEP_IN_EXTERNALCTRLMODE);
	InExternalCtrlModeFlowSpeed = EEPROM.read(EEP_IN_EXTERNALCTRLMODE_FLOWSPEED);
	InExternalCtrlModeFlowNumSections = EEPROM.read(EEP_IN_EXTERNALCTRLMODE_FLOWNUMSECTIONS);
	
	// Colored 5's
	r = EEPROM.read(EEP_OUT_COLORED5S_COLOR);
	g = EEPROM.read(EEP_OUT_COLORED5S_COLOR+1);
	b = EEPROM.read(EEP_OUT_COLORED5S_COLOR+2);
	OutColored5sColor = PixelColor(r,g,b);
	r = EEPROM.read(EEP_IN_COLORED5S_COLOR);
	g = EEPROM.read(EEP_IN_COLORED5S_COLOR+1);
	b = EEPROM.read(EEP_IN_COLORED5S_COLOR+2);
	InColored5sColor = PixelColor(r,g,b);
	
	// Clap for Time
	EnableClap = EEPROM.read(EEP_ENABLE_CLAP);
	ClapAmpThreshold = EEPROM.read(EEP_CLAP_AMP_THRESHOLD);
	ClapMinDelayUntilNext = EEPROM.read(EEP_CLAP_MIN_DELAY_UNTIL_NEXT);
	ClapWindowForNext = EEPROM.read(EEP_CLAP_WINDOW_FOR_NEXT);
	ClapShowTimeNumSeconds = EEPROM.read(EEP_CLAP_SHOW_TIME_NUM_SECONDS);
	
	// UseNtpServer
	UseNtpServer = EEPROM.read(EEP_USE_NTP_SERVER);
	
	// TzAdj
	byte val = EEPROM.read(EEP_TZ_ADJ);
	if (val >= 128) {  // if negative number
		TzAdj = val - 0xFF - 1;
	} else {
		TzAdj = val;
	}
	
	// IsDst
	IsDst = EEPROM.read(EEP_IS_DST);
	
	// === Hand Properties (sizes & colors) ===
	// HandSizes
	HandSizeHour = EEPROM.read(EEP_HANDSIZE_HOUR);
	HandSizeMin = EEPROM.read(EEP_HANDSIZE_MIN);
	HandSizeSec = EEPROM.read(EEP_HANDSIZE_SEC);
	
	// HandColors
	r = EEPROM.read(EEP_HANDCOLOR_HOUR);
	g = EEPROM.read(EEP_HANDCOLOR_HOUR+1);
	b = EEPROM.read(EEP_HANDCOLOR_HOUR+2);
	HandColorHour = PixelColor(r,g,b);
	r = EEPROM.read(EEP_HANDCOLOR_MIN);
	g = EEPROM.read(EEP_HANDCOLOR_MIN+1);
	b = EEPROM.read(EEP_HANDCOLOR_MIN+2);
	HandColorMin = PixelColor(r,g,b);
	r = EEPROM.read(EEP_HANDCOLOR_SEC);
	g = EEPROM.read(EEP_HANDCOLOR_SEC+1);
	b = EEPROM.read(EEP_HANDCOLOR_SEC+2);
	HandColorSec = PixelColor(r,g,b);
	
	// Disp on which strips
	DispHourHandOut = EEPROM.read(EEP_HOUR_HAND_OUT);
	DispHourHandIn = EEPROM.read(EEP_HOUR_HAND_IN);
	DispMinHandOut = EEPROM.read(EEP_MIN_HAND_OUT);
	DispMinHandIn = EEPROM.read(EEP_MIN_HAND_IN);
	DispSecHandOut = EEPROM.read(EEP_SEC_HAND_OUT);
	DispSecHandIn = EEPROM.read(EEP_SEC_HAND_IN);
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


void displayTimeToStrips() {
	//string inOutStr = isOutside ? "OUTSIDE" : "INSIDE";  // Keep for reference.

	if (CurrTime.second() != lastSecondVal || clockRTColorChanged) {
		// Update only once / second (unless RealTime color is changing)
		lastSecondVal = CurrTime.second();
		clockRTColorChanged = false;

		//byte opModeOutside = ((OpMode & 0xF0) >> 4);
		//byte opModeInside = OpMode & 0x0F;

		byte handVal[3];
		byte handSize[3];
		PixelColor handColor[3];
		byte handDispOut[3];
		byte handDispIn[3];
	
		byte h = CurrTime.hour();
		if (h > 11) { h -= 12; }
	
		handVal[HOUR] = (h * 5) + (CurrTime.minute() / 12);
		handVal[MIN] = CurrTime.minute();
		handVal[SEC] = CurrTime.second();
	
		handSize[HOUR] = HandSizeHour;
		handSize[MIN] = HandSizeMin;
		handSize[SEC] = HandSizeSec;
	
		handColor[HOUR] = HandColorHour;
		handColor[MIN] = HandColorMin;
		handColor[SEC] = HandColorSec;
	
		handDispOut[HOUR] = DispHourHandOut;
		handDispIn[HOUR] = DispHourHandIn;
		handDispOut[MIN] = DispMinHandOut;
		handDispIn[MIN] = DispMinHandIn;
		handDispOut[SEC] = DispSecHandOut;
		handDispIn[SEC] = DispSecHandIn;

		byte clkHand;
		//uint32_t currHandColor;
		//uint8_t pixelNum;

		// Clear Strip(s) (if applicable)
		if (opModeOutside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
			strip = &outStrip;
			clearStrip(strip);
		}

		if (opModeInside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
			strip = &inStrip;
			clearStrip(strip);
		}
	
		byte numColorsInSeries = 1;
		PixelColor colorSeriesArr[numColorsInSeries];
		for (clkHand = HOUR; clkHand <= SEC; clkHand++) {  // 0 to 2
			byte startPixelNum = handVal[clkHand] - (handSize[clkHand] / 2);
			startPixelNum = LightShowCmd::fixPixelNum(startPixelNum);
			colorSeriesArr[0] = handColor[clkHand];
		
			// Outside strip
			if (opModeOutside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
				strip = &outStrip;
				if (handDispOut[clkHand] == 1) {
					//SetSeqPixels handSsp(strip, startPixelNum, 1, handSize[clkHand], 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
					//handSsp.exec(NOSHOWSTRIP);
					SetSeqPixels *handSsp = new SetSeqPixels(strip, startPixelNum, 1, handSize[clkHand], 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
					handSsp->exec(NOSHOWSTRIP);
					delete(handSsp);
				}
			}
		
			// Inside strip
			if (opModeInside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
				strip = &inStrip;
				if (handDispIn[clkHand] == 1) {
					//SetSeqPixels handSsp(strip, startPixelNum, 1, handSize[clkHand], 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
					//handSsp.exec(NOSHOWSTRIP);
					SetSeqPixels *handSsp = new SetSeqPixels(strip, startPixelNum, 1, handSize[clkHand], 0, ITER_ENOUGH, 0, 0, DESTRUCTIVE, CW, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
					handSsp->exec(NOSHOWSTRIP);
					delete(handSsp);
				}
			}
		}
		
		if (opModeOutside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
			strip = &outStrip;
			strip->show();
		}
		if (opModeInside == OPMODE_CLOCK || isClapTriggerClockEnabled) {
			strip = &inStrip;
			strip->show();
		}
	}
}

// Minimalist time server query; adapted from Adafruit Gutenbird sketch,
// which in turn has roots in Arduino UdpNTPClient tutorial.
unsigned long getTime(void) {

  uint8_t       buf[48];
  unsigned long ip, startTime, t = 0L;

  Serial.print(F("Locating time server..."));

  // Hostname to IP lookup; use NTP pool (rotates through servers)
  char ipStr[] = "pool.ntp.org";
  //if(cc3000.getHostByName("pool.ntp.org", &ip)) {
  if(cc3000.getHostByName(ipStr, &ip)) {
    static const char PROGMEM
      timeReqA[] = { 227,  0,  6, 236 },
      timeReqB[] = {  49, 78, 49,  52 };

    Serial.println(F("\r\nAttempting connection..."));
    startTime = millis();
    do {
      ntpClient = cc3000.connectUDP(ip, 123);
    } while((!ntpClient.connected()) &&
            ((millis() - startTime) < connectTimeout));

    if(ntpClient.connected()) {
      Serial.print(F("connected!\r\nIssuing request..."));

      // Assemble and issue request packet
      memset(buf, 0, sizeof(buf));
      memcpy_P( buf    , timeReqA, sizeof(timeReqA));
      memcpy_P(&buf[12], timeReqB, sizeof(timeReqB));
      ntpClient.write(buf, sizeof(buf));

      Serial.print(F("\r\nAwaiting response..."));
      memset(buf, 0, sizeof(buf));
      startTime = millis();
      while((!ntpClient.available()) &&
            ((millis() - startTime) < responseTimeout));
      if(ntpClient.available()) {
        ntpClient.read(buf, sizeof(buf));
        t = (((unsigned long)buf[40] << 24) |
             ((unsigned long)buf[41] << 16) |
             ((unsigned long)buf[42] <<  8) |
              (unsigned long)buf[43]) - 2208988800UL;
        Serial.print(F("OK\r\n"));
      }
      ntpClient.close();
    }
  }
  if(!t) Serial.println(F("error"));
  return t;
}

void showSetupProgress(byte step) {
	// Light up LED's on outside strip, during setup(), just so we know we're making progress.
	strip = &outStrip;
	
	PixelColor pc;
	switch (step % 3) {
		case 0:
			pc = PixelColor(0xFF, 0x00, 0x00);  // Red
			break;
		case 1:
			pc = PixelColor(0x00, 0xFF, 0x00);  // Green
			break;
		default:
			pc = PixelColor(0x00, 0x00, 0xFF);  // Blue
			break;
	}
	byte numSeq = 3;
	for (byte i = 0; i < numSeq; i++) {
		strip->setPixelColor(step * numSeq + i, pc.getLongVal());
	}
	strip->show();
}

ISR(ADC_vect) { // Audio-sampling interrupt
	static const int16_t noiseThreshold = 4;
	int16_t              sample         = ADC; // 0-1023

	capture[samplePos] = ((sample > (512-noiseThreshold)) && (sample < (512+noiseThreshold))) ? 0 : sample - 512; // Sign-convert for FFT; -512 to +511

	if(++samplePos >= FFT_N) ADCSRA &= ~_BV(ADIE); // Buffer full, interrupt off
}