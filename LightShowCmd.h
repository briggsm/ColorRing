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

#ifndef LIGHTSHOWCMD_H
#define LIGHTSHOWCMD_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "PixelColor.h"
#include "Gradient.h"

//#include "StandardCplusplus.h"
//#include "serstream"
//using namespace std;

#include "AllDefs.h"

class LightShowCmd {
protected:
	// ======================
	// === Cmd Parameters ===
	// ======================
	
	byte cmdType;
	Adafruit_NeoPixel* strip;
	
	byte startPixelNum;
	byte endPixelNum;
	
	byte numSections;
	byte numPixelsEachColor;
	byte colorSeriesNumIter;
	
	byte numPixelsToSkip;
	word numIter;
	
	unsigned long animDelay;
	word pauseAfter;
	
	// boolBits
	bool destructive;
	bool direction;
	bool wrap;
	bool isAnim;
	bool clearStripBefore;
	bool gradiate;
	bool gradiateLastPixelFirstColor;
	
	byte numColorsInSeries;
	PixelColor colorSeriesArr[7]; // up to MAX 7 different color series. (Should be enough for each type of cmd).
	
	
	// ==============
	// === Others ===
	// ==============
	
	// Gradiate
	MultiGradient multiGradient;
	PixelColor currPixelColor;
	
	boolean isFirstTimeAfterFinished;
	int cmdFinishedTime;
	
public:
	boolean isPauseMode;
	byte currIteration;
	//string inOutStr;
        
	LightShowCmd();
	bool isMoreSteps();
	
	virtual void step(boolean isShowStrip) = 0;
	void exec(boolean isShowStrip);
	
	static byte fixPixelNum(byte pixelNum);
	void showAndWait();

	virtual void reset() = 0;
		
	bool bitChecker(byte bit, byte by);
	//void clearStrip(Adafruit_NeoPixel* strip);
	void clearStrip();
	byte getCmdType();
	//string getCmdTypeStr();
	void lscResetCommon();
	void lscStepPreCommon();
	void lscStepPostCommon(boolean isShowStrip);
	
	bool getIsAnim();
	unsigned long getAnimDelay();
	void setBoolBitVars(byte boolBits, bool &destructive, bool &direction, bool &wrap, bool &isAnim, bool &clearStripBefore, bool &gradiate, bool &gradiateLastPixelFirstColor);

};

#endif // LIGHTSHOWCMD_H
