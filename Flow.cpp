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

#include "Flow.h"
#include "Shift.h"
#include "SetSeqPixels.h"

Flow::Flow() {
  
}

Flow::Flow(Adafruit_NeoPixel* strip,
						
			byte startPixelNum,
			byte endPixelNum,
			byte numSections,
			byte numPixelsEachColor,
			byte colorSeriesNumIter,
			byte numPixelsToSkip,
			word animDelay,
			word pauseAfter,
	
			// boolBits
			bool direction,
			bool clearStripBefore,
			bool gradiate,
			bool gradiateLastPixelFirstColor,
	
			byte numColorsInSeries,
			PixelColor *colorSeriesArr)
{
	init(strip, startPixelNum, endPixelNum, numSections, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, animDelay, pauseAfter, direction, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
}

Flow::Flow(Adafruit_NeoPixel* strip, byte* stripCmdArray) {

	byte startPixelNum = stripCmdArray[1];
	byte endPixelNum = stripCmdArray[2];
	byte numSections = stripCmdArray[3];
	byte numPixelsEachColor = stripCmdArray[4];
	byte colorSeriesNumIter = stripCmdArray[5];
	byte numPixelsToSkip = stripCmdArray[6];
		
	word animDelay = (stripCmdArray[7] << 8) + stripCmdArray[8];
	word pauseAfter = (stripCmdArray[9] << 8) + stripCmdArray[10];
	
	// boolBits
	byte boolBits = stripCmdArray[11];
	/*
	//bool destructive = bitChecker(BOOLBIT_DESTRUCTIVE, boolBits);
	bool direction = bitChecker(BOOLBIT_DIRECTION, boolBits);
	//bool wrap = bitChecker(BOOLBIT_WRAP, boolBits);
	//bool isAnim = bitChecker(BOOLBIT_ISANIM, boolBits);
	bool clearStripBefore = bitChecker(BOOLBIT_CLEARSTRIP, boolBits);
	bool gradiate = bitChecker(BOOLBIT_GRADIATE, boolBits);
	bool gradiateLastPixelFirstColor = bitChecker(BOOLBIT_GRADIATE_LASTPIXEL_FIRSTCOLOR, boolBits);
	*/
	setBoolBitVars(boolBits, destructive, direction, wrap, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor);
	
	// colorSeriesArr
	byte numColorsInSeries = stripCmdArray[12];
	byte csaFirstBytePos = 13;
	//byte maxNumColors = (MAX_STRIPCMD_SIZE - csaFirstBytePos) / 3;  // stripCmd's are 32-bytes total (MAX_STRIPCMD_SIZE).
	//PixelColor colorSeriesArr[maxNumColors];
	for (int i = 0; i < numColorsInSeries; i++) {
		byte bOffset = csaFirstBytePos + (3*i);
		colorSeriesArr[i] = PixelColor(stripCmdArray[bOffset], stripCmdArray[bOffset+1], stripCmdArray[bOffset+2]);
	}
	
	init(strip, startPixelNum, endPixelNum, numSections, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, animDelay, pauseAfter, direction, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
}

void Flow::init(Adafruit_NeoPixel* strip,
						
			byte startPixelNum,
			byte endPixelNum,
			byte numSections,
			byte numPixelsEachColor,
			byte colorSeriesNumIter,
			byte numPixelsToSkip,
			word animDelay,
			word pauseAfter,
	
			// boolBits
			bool direction,
			bool clearStripBefore,
			bool gradiate,
			bool gradiateLastPixelFirstColor,
	
			byte numColorsInSeries,
			PixelColor *colorSeriesArr)
{
  	//Note 'animation' is assumed.
	
	this->cmdType = 3;
	this->strip = strip;
	
	this->startPixelNum = startPixelNum;
	this->endPixelNum = endPixelNum;
	
	this->numSections = numSections;
	this->numPixelsEachColor = numPixelsEachColor;
	this->colorSeriesNumIter = colorSeriesNumIter;

	this->numPixelsToSkip = numPixelsToSkip;
	this->numIter = numColorsInSeries * numPixelsEachColor * colorSeriesNumIter;
	
	this->animDelay = animDelay;
	this->pauseAfter = pauseAfter;

	// boolBits
	this->destructive = true;  // Hard-coded.
	this->direction = direction;
	this->wrap = NULL;  // N/A
	this->isAnim = true;  // hard-code this, becuase it HAS to be animated.
	this->clearStripBefore = clearStripBefore;
	this->gradiate = gradiate;
	this->gradiateLastPixelFirstColor = gradiateLastPixelFirstColor;
	
	this->numColorsInSeries = numColorsInSeries;
	for (int i = 0; i < numColorsInSeries; i++) {
		this->colorSeriesArr[i] = colorSeriesArr[i];
	}
	
	
	
	multiGradient = MultiGradient(colorSeriesArr, numColorsInSeries, numPixelsEachColor, gradiateLastPixelFirstColor);
	
	// Init other variables
	//this->inOutStr = strip->getPin() == OUT_STRIP_PIN ? "OUTSIDE" : "INSIDE";
}

void Flow::step(boolean isShowStrip) {
	//cout << "Flow::step(" << (int)isShowStrip << ")" << endl;
	
	if (!isPauseMode) {
		//cout << "Flow::step(" << (int)isShowStrip << ") [not paused]" << endl;
		
		lscStepPreCommon();
		
	
		if (gradiate) {
			currPixelColor = multiGradient.getTweenPixelColor(currIteration);
		} else {
			// Don't worry about gradients
			currPixelColor = colorSeriesArr[(currIteration / numPixelsEachColor) % numColorsInSeries];
		}
		//cout << F("currPixelColor: "); currPixelColor.println();
		
	
		vector<byte> startPixelNums(numSections);
		vector<byte> endPixelNums(numSections);
		vector<boolean> directions(numSections);
		
		// Do all the math
		byte numPixelsPerSection = (endPixelNum - startPixelNum + 1) / numSections;
		for (int i = 0; i < numSections; i++) {
			startPixelNums[i] = startPixelNum + (i * numPixelsPerSection);
			//cout << "startPixelNums[" << i << "]: " << (int)startPixelNums[i] << endl;
			endPixelNums[i] = startPixelNums[i] + numPixelsPerSection - 1;
			//cout << "endPixelNums[" << i << "]: " << (int)endPixelNums[i] << endl;
			//directions[i] = (i % 2 == 0) ? CW : CCW;
			directions[i] = (i % 2 == 0) ? direction : !direction;
			//cout << "directions[" << i << "]: " << (int)directions[i] << endl;
		}
	
		//Serial.print("Free RAM Flow: "); Serial.println(getFreeRam(), DEC);
	
		// Execute
		for (int i = 0; i < numSections; i++) {
			singleFlow(strip, startPixelNums[i], endPixelNums[i], currPixelColor, directions[i], numPixelsToSkip);
		}
		
		
		lscStepPostCommon(isShowStrip);
	}
}

void Flow::singleFlow(Adafruit_NeoPixel* strip, byte startPixelNum, byte endPixelNum, PixelColor pixelColor, boolean direction, byte numPixelsToSkip) {
	// First Shift
	//Shift shift(strip, startPixelNum, endPixelNum, 0, 1, 0, 0, direction, NOWRAP, NONANIMATED);
	//shift.exec(NOSHOWSTRIP);
	Shift *shift = new Shift(strip, startPixelNum, endPixelNum, 0, 1, 0, 0, direction, NOWRAP, NONANIMATED);
	shift->exec(NOSHOWSTRIP);
	delete(shift);
	
	// Then set "Source" pixel
	byte sourcePixelNum = direction ? startPixelNum : endPixelNum;
	/*
	byte numColorsInSeries = 1;
	PixelColor colorSeriesArr[numColorsInSeries]; colorSeriesArr[0] = pixelColor;
	SetSeqPixels ssp(strip, sourcePixelNum, 1, 1, 0, 0, 0, DESTRUCTIVE, direction, NONANIMATED, NOCLEAR, NOGRADIATE, GRADIATE_LASTPIXEL_LASTCOLOR, numColorsInSeries, colorSeriesArr);
	ssp.exec(NOSHOWSTRIP); // or ssp.step()
	*/
	strip->setPixelColor(sourcePixelNum, pixelColor.getLongVal());  // Don't ->show() yet
}

void Flow::reset() {
	//cout << "Flow::reset()" << endl;
	
	lscResetCommon();

	// Nothing else needed, I think
}

/*
void printStrip(Adafruit_NeoPixel* strip, int numPixels) {
	//uint8_t pixels[60*3] = strip->getPixels();

	for (int i = 0; i < numPixels; i++) {
		uint32_t color = strip->getPixelColor(i);
		PixelColor pc = PixelColor(color);
		//Serial.print(pixels[i]); Serial.print(",");
		Serial.print(i); Serial.print(": "); Serial.print(pc.R); Serial.print(","); Serial.print(pc.G); Serial.print(","); Serial.print(pc.B); Serial.println();
	}
}
*/
