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

#include "Shift.h"

// Note: order of "start" and "end" pixelNum matters. start at "start" and go right (CW) until reach "end"
// E.g. 5->15, simple => 10 LEDs, then shift CW or CCW according to 'direction'
// E.g. 15->5, start at 15, go right, until reach 5 => 50 LEDs. Then shift CW or CCW acoring to 'direction'
// E.g. 55->5, start at 55, go right, until reach 5 => 10 LEDs. Then shift CW or CCW acoring to 'direction'

Shift::Shift() {
  
}

Shift::Shift(Adafruit_NeoPixel* strip,
	
			byte startPixelNum,
			byte endPixelNum,
			byte numPixelsToSkip,
			word numIter,
			word animDelay,
			word pauseAfter,
			
			// boolBits
			boolean direction,
			boolean wrap,
			boolean isAnim)
{
	init(strip, startPixelNum, endPixelNum, numPixelsToSkip, numIter, animDelay, pauseAfter, direction, wrap, isAnim);
}

Shift::Shift(Adafruit_NeoPixel* strip, byte* stripCmdArray) {

	byte startPixelNum = stripCmdArray[1];
	byte endPixelNum = stripCmdArray[2];
	byte numPixelsToSkip = stripCmdArray[3];
	word numIter = (stripCmdArray[4] << 8) + stripCmdArray[5];

	word animDelay = (stripCmdArray[6] << 8) + stripCmdArray[7];
	word pauseAfter = (stripCmdArray[8] << 8) + stripCmdArray[9];
	
	byte boolBits = stripCmdArray[10];
	/*
	//bool destructive = bitChecker(BOOLBIT_DESTRUCTIVE, boolBits);
	bool direction = bitChecker(BOOLBIT_DIRECTION, boolBits);
	bool wrap = bitChecker(BOOLBIT_WRAP, boolBits);
	bool isAnim = bitChecker(BOOLBIT_ISANIM, boolBits);
	//bool clearStripBefore = bitChecker(BOOLBIT_CLEARSTRIP, boolBits);
	//bool gradiate = bitChecker(BOOLBIT_GRADIATE, boolBits);
	//bool gradiateLastPixelFirstColor = bitChecker(BOOLBIT_GRADIATE_LASTPIXEL_FIRSTCOLOR, boolBits);
	*/
	setBoolBitVars(boolBits, destructive, direction, wrap, isAnim, clearStripBefore, gradiate, gradiateLastPixelFirstColor);
	
	init(strip, startPixelNum, endPixelNum, numPixelsToSkip, numIter, animDelay, pauseAfter, direction, wrap, isAnim);
}

void Shift::init(Adafruit_NeoPixel* strip,
	
			byte startPixelNum,
			byte endPixelNum,
			byte numPixelsToSkip,
			word numIter,
			word animDelay,
			word pauseAfter,
			
			// boolBits
			boolean direction,
			boolean wrap,
			boolean isAnim)
{
	this->cmdType = 2;
	this->strip = strip;
	
	this->startPixelNum = startPixelNum;
	this->endPixelNum = endPixelNum;
	
	this->numSections = NULL;  // N/A
	this->numPixelsEachColor = NULL;  // N/A
	this->colorSeriesNumIter = NULL;  // N/A
	
	this->numPixelsToSkip = numPixelsToSkip;
	this->numIter = numIter;
	this->animDelay = animDelay;
	this->pauseAfter = pauseAfter;
	
	// boolBits
	this->destructive = NULL;  // N/A
	this->direction = direction;
	this->wrap = wrap;
	this->isAnim = isAnim;
	this->clearStripBefore = false;  // Hard-code (can never clear strip before a shift!)
	this->gradiate = NULL;  // N/A
	this->gradiateLastPixelFirstColor = NULL;  // N/A

	// Init other variables
	//this->inOutStr = strip->getPin() == OUT_STRIP_PIN ? "OUTSIDE" : "INSIDE";
	
	// Calc total number of pixels
	if (endPixelNum >= startPixelNum) {
		numPixels = endPixelNum - startPixelNum + 1;
	} else {
		numPixels = (60 - startPixelNum) + endPixelNum + 1;
	}
}

void Shift::step(boolean isShowStrip) {
	// Note: see note at top for how to handle "start" and "end" pixels.
	
	//cout << "Shift::step(" << (int)isShowStrip << ")" << endl;
	
	if (!isPauseMode) {
		//cout << "Shift::step(" << (int)isShowStrip << ") [not paused]" << endl;

		lscStepPreCommon();
		
	
		for (int i = 0; i <= numPixelsToSkip; i++) {
			// Start and the 'end' and go backwards (opposite of the 'direction')
			currPixelNum = (direction) ? endPixelNum : startPixelNum;
			//cout << F("currPixelNum: ") << (int)currPixelNum << endl;
			
			if (wrap) {
				// Save away 'last' pixel for 'maybe' use later on.
				tempPixelColor = PixelColor(strip->getPixelColor(currPixelNum));
			}
	
			//cout << F("shift dir: ") << (int)direction << endl;
	
			// Shift all but the very last pixel ('cuz that would go a little too far)
			for (int ctr = 0; ctr < numPixels - 1; ctr++) {
				if (direction) {
					strip->setPixelColor(currPixelNum, strip->getPixelColor(fixPixelNum(currPixelNum - 1)));
					currPixelNum = fixPixelNum(--currPixelNum);
				} else {
					strip->setPixelColor(currPixelNum, strip->getPixelColor(fixPixelNum(currPixelNum + 1)));
					currPixelNum = fixPixelNum(++currPixelNum);
				}

				//cout << F("currPixelNum2: ") << (int)currPixelNum << endl;
			}
	
			// Fill in the very last pixel based on 'wrap'
			if (currPixelNum > 59) currPixelNum = 0;  //??????????!!!!!!!!! dont' hard code this value??????
			if (wrap) {
				strip->setPixelColor(currPixelNum, tempPixelColor.getLongVal());
			} else {
				strip->setPixelColor(currPixelNum, 0x00000000);
			}
		}	
	

		lscStepPostCommon(isShowStrip);
	}
}

void Shift::reset() {
	//cout << "Shift::reset()" << endl;
	
	lscResetCommon();
	// Nothing else needed, I think
}
