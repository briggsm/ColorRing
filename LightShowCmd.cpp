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

#include "LightShowCmd.h"

LightShowCmd::LightShowCmd() {
	
	this->cmdType = 0;
	this->strip = 0;
	
	this->startPixelNum = 0;
	this->endPixelNum = 0;
	
	this->numSections = 1;
	this->numPixelsEachColor = 0;
	this->colorSeriesNumIter = 0;

	this->numPixelsToSkip = 0;
	this->numIter = 0;
	
	this->animDelay = 0;
	this->pauseAfter = 0;
	
	// boolBits
	this->destructive = true;
	this->direction = true;
	this->wrap = true;
	this->isAnim = false;
	this->clearStripBefore = false;
	this->gradiate = false;
	this->gradiateLastPixelFirstColor = false;
	
	// Init other variables
	this->currIteration = 0;
	//this->inOutStr = "None";
	isFirstTimeAfterFinished = true;
	isPauseMode = false;
	cmdFinishedTime = 0;
}

bool LightShowCmd::isMoreSteps() {

	if (numIter == 0) { return false; }
	
	if (currIteration < numIter) {
		return true;
	} else {
		isPauseMode = true;
		int currentMillis = millis();
		if (isFirstTimeAfterFinished) {
			cmdFinishedTime = currentMillis;
			isFirstTimeAfterFinished = false;
			if (pauseAfter == 0) {
				return false;
			} else {
				return true;
			}
		} else {
			if (currentMillis - cmdFinishedTime < pauseAfter) {
				return true;
			} else {
				return false;
			}
		}
	}
	
	return false; // Shouldn't get here
}

byte LightShowCmd::fixPixelNum(byte pixelNum) {
	// Note: Deciding to calc based on if it's closer to 255 or closer to 1

	if (pixelNum >= 60) {
		if (pixelNum <= 158) { // (255-59) / 2) + 60 = 158
			pixelNum -= ((pixelNum / 60) * 60);
		} else {
			pixelNum -= 196; // 255-59
		}
	}

	return pixelNum;
}

void LightShowCmd::showAndWait() {
	//cout << "strip->show() - LSC::showAndWait()" << endl;
	strip->show();
	if (isAnim) {
		Serial.println(F("** Probably shouldn't get here in this application **"));
		delay(animDelay);
	}
}

void LightShowCmd::exec(boolean isShowStrip) {
	//Serial.println("LightShowCmd::exec()");
	if (!isPauseMode) {
		//cout << "LightShowCmd::exec(" << (int)isShowStrip << ") [not paused]" << endl;
		while (isMoreSteps() && !isPauseMode) {
			//cout << "in while" << endl;
			step(NOSHOWSTRIP);
		
			// Animation (if necessary)
			if (isAnim) { showAndWait(); }
		}
		
		if (isShowStrip) {
			//cout << "strip->show() - LSC::exec()" << endl;
			strip->show();
		}
	}
}

bool LightShowCmd::bitChecker(byte bit, byte by) {
	// Look up a bit within a byte. If 1, returns true. Otherwise false.
	// bit => 0..7
	// by => 0x00..0xFF
	
	return ((by & (1 << bit)) >> bit) == 1 ? true : false;
}

//void LightShowCmd::clearStrip(Adafruit_NeoPixel* strip) {  // ????? don't need this strip var right?  cuz it's a member variable ????? take out?
void LightShowCmd::clearStrip() {
	// Keep, maybe.
	//cout << "LSC::clearStrip(" << inOutStr << ")" << endl;
	
	for (uint8_t i = 0; i < strip->numPixels(); i++) {
		strip->setPixelColor(i, 0, 0, 0);
	}
}


byte LightShowCmd::getCmdType() {
	return cmdType;
}

/*
string LightShowCmd::getCmdTypeStr() {
	switch (cmdType) {
		case 0:
			return "SSP";
		case 2:
			return "Shift";
		case 3:
			return "Flow";
	}
	return "Invalid Cmd";
}
*/

void LightShowCmd::lscResetCommon() {
	currIteration = 0;
	isFirstTimeAfterFinished = true;
	isPauseMode = false;
}

void LightShowCmd::lscStepPreCommon() {
	if (currIteration == 0) {  // first step
		//if (clearStripBefore) { clearStrip(strip); }
		if (clearStripBefore) { clearStrip(); }
	}
}

void LightShowCmd::lscStepPostCommon(boolean isShowStrip) {
	currIteration++;
	
	if (isShowStrip) {
		//cout << "strip->show() - SSP::step()" << endl;
		strip->show();
	}
}

bool LightShowCmd::getIsAnim() {
	return isAnim;
}

unsigned long LightShowCmd::getAnimDelay() {
	return animDelay;
}

void LightShowCmd::setBoolBitVars(byte boolBits, bool &destructive, bool &direction, bool &wrap, bool &isAnim, bool &clearStripBefore, bool &gradiate, bool &gradiateLastPixelFirstColor) {
	destructive = bitChecker(BOOLBIT_DESTRUCTIVE, boolBits);
	direction = bitChecker(BOOLBIT_DIRECTION, boolBits);
	wrap = bitChecker(BOOLBIT_WRAP, boolBits);
	isAnim = bitChecker(BOOLBIT_ISANIM, boolBits);
	clearStripBefore = bitChecker(BOOLBIT_CLEARSTRIP, boolBits);
	gradiate = bitChecker(BOOLBIT_GRADIATE, boolBits);
	gradiateLastPixelFirstColor = bitChecker(BOOLBIT_GRADIATE_LASTPIXEL_FIRSTCOLOR, boolBits);
}

