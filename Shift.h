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

#ifndef SHIFT_H
#define SHIFT_H

#include <Arduino.h>
#include "LightShowCmd.h"
#include "PixelColor.h"

#include "StandardCplusplus.h"
#include "serstream"
using namespace std;

class Shift : public LightShowCmd {

protected:
	//boolean wrap;
	byte numPixels;
	PixelColor tempPixelColor;
	byte currPixelNum;
	
public:
	
	Shift();
	
	Shift(Adafruit_NeoPixel* strip,
	
			byte startPixelNum,
			byte endPixelNum,
			byte numPixelsToSkip,
			byte numIter,
			word animDelay,
			word pauseAfter,
			
			// boolBits
			boolean direction,
			boolean wrap,
			boolean isAnim);

	Shift(Adafruit_NeoPixel* strip, byte* stripCmdArray);

	void init(Adafruit_NeoPixel* strip,
	
			byte startPixelNum,
			byte endPixelNum,
			byte numPixelsToSkip,
			byte numIter,
			word animDelay,
			word pauseAfter,
			
			// boolBits
			boolean direction,
			boolean wrap,
			boolean isAnim);


	void step(boolean isShowStrip);
	void reset();
};

#endif
