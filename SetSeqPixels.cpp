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

#include "SetSeqPixels.h"

SetSeqPixels::SetSeqPixels() {
	
}

SetSeqPixels::SetSeqPixels(Adafruit_NeoPixel* strip,
						
						byte startPixelNum,
						byte numPixelsEachColor,
						byte colorSeriesNumIter,
						byte numPixelsToSkip,
						word numIter,
						word animDelay,
						word pauseAfter,
						
						// boolBits
						bool destructive,
						bool direction,
						bool isAnim,
						bool clearStripBefore,
						bool gradiate,
						bool gradiateLastPixelFirstColor,
						
						byte numColorsInSeries,
						PixelColor *colorSeriesArr)
{
	init(strip, startPixelNum, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, numIter, animDelay, pauseAfter, destructive, direction, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
}

SetSeqPixels::SetSeqPixels(Adafruit_NeoPixel* strip, byte* stripCmdArray) {
	
	byte startPixelNum = stripCmdArray[1];
	byte numPixelsEachColor = stripCmdArray[2];
	byte colorSeriesNumIter = stripCmdArray[3];
	byte numPixelsToSkip = stripCmdArray[4];
	word numIter = (stripCmdArray[5] << 8) + stripCmdArray[6];
		
	word animDelay = (stripCmdArray[7] << 8) + stripCmdArray[8];
	word pauseAfter = (stripCmdArray[9] << 8) + stripCmdArray[10];
	
	// boolBits
	byte boolBits = stripCmdArray[11];
	/*
	bool destructive = bitChecker(BOOLBIT_DESTRUCTIVE, boolBits);
	bool direction = bitChecker(BOOLBIT_DIRECTION, boolBits);
	//bool wrap = bitChecker(BOOLBIT_WRAP, boolBits);
	bool isAnim = bitChecker(BOOLBIT_ISANIM, boolBits);
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
	
	init(strip, startPixelNum, numPixelsEachColor, colorSeriesNumIter, numPixelsToSkip, numIter, animDelay, pauseAfter, destructive, direction, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor, numColorsInSeries, colorSeriesArr);
}

void SetSeqPixels::init(Adafruit_NeoPixel* strip,
						
						byte startPixelNum,
						byte numPixelsEachColor,
						byte colorSeriesNumIter,
						byte numPixelsToSkip,
						word numIter,
						word animDelay,
						word pauseAfter,
						
						// boolBits
						bool destructive,
						bool direction,
						bool isAnim,
						bool clearStripBefore,
						bool gradiate,
						bool gradiateLastPixelFirstColor,
						
						byte numColorsInSeries,
						PixelColor *colorSeriesArr)
{
	this->cmdType = 0;
	this->strip = strip;
	
	this->startPixelNum = startPixelNum;
	this->numPixelsEachColor = numPixelsEachColor;
	this->colorSeriesNumIter = colorSeriesNumIter;
	this->numPixelsToSkip = numPixelsToSkip;
	this->animDelay = animDelay;
	this->pauseAfter = pauseAfter;
	
	// boolBits
	this->destructive = destructive;
	this->direction = direction;
	this->wrap = NULL;  // N/A
	this->isAnim = isAnim;
	this->clearStripBefore = clearStripBefore;
	this->gradiate = gradiate;
	this->gradiateLastPixelFirstColor = gradiateLastPixelFirstColor;
	
	this->numColorsInSeries = numColorsInSeries;
	for (int i = 0; i < numColorsInSeries; i++) {
		this->colorSeriesArr[i] = colorSeriesArr[i];
	}
	

	// Gradient Stuff
	multiGradient = MultiGradient(colorSeriesArr, numColorsInSeries, numPixelsEachColor, gradiateLastPixelFirstColor);
	
	// Init other Variables
	this->currIteration = 0;
	this->currPixelNum = startPixelNum;
	if (numIter == ITER_ENOUGH) {
		this->numIter = numColorsInSeries * numPixelsEachColor * colorSeriesNumIter;
	} else {
		// to cut the # of iter's short.
		this->numIter = numIter;
	}
	//this->inOutStr = strip->getPin() == OUT_STRIP_PIN ? "OUTSIDE" : "INSIDE";
}

void SetSeqPixels::step(boolean isShowStrip) {
	//cout << "SetSeqPixels::step(" << (int)isShowStrip << ")" << endl;
	
	if (!isPauseMode) {
		//cout << "SetSeqPixels::step(" << (int)isShowStrip << ") [not paused]" << endl;
		
		lscStepPreCommon();
		
	
		//cout << F("currIteration: ") << (int)currIteration << endl;

		// Fix pixelNum (if necessary)
		currPixelNum = fixPixelNum(currPixelNum);
		//cout << F("currPixelNum: ") << (int)currPixelNum << endl;

		// Set currPixelColor	
		if (gradiate) {
			currPixelColor = multiGradient.getTweenPixelColor(currIteration);
		} else {
			// Don't worry about gradients
			currPixelColor = colorSeriesArr[(currIteration / numPixelsEachColor) % numColorsInSeries];
		}
		//cout << F("currPixelColor: "); currPixelColor.println();

		// Add the Colors (if non-destructive)
		if (!destructive) currPixelColor.addThisColor(PixelColor(strip->getPixelColor(currPixelNum)));

		// Set Strip's Pixel Color
		strip->setPixelColor(currPixelNum, currPixelColor.getLongVal());

		// Next Pixel
		currPixelNum = direction ? currPixelNum + numPixelsToSkip + 1 : currPixelNum - numPixelsToSkip - 1;
		//cout << "next pixelNum: " << (int)currPixelNum << endl;
		
		
		lscStepPostCommon(isShowStrip);
	}
}

void SetSeqPixels::reset() {
	//Serial.println("SSP::reset()");
	lscResetCommon();
	currPixelNum = startPixelNum;
}