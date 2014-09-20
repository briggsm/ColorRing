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

#ifndef SETSEQPIXELS_H
#define SETSEQPIXELS_H

#include <Arduino.h>
#include "LightShowCmd.h"

//#include "StandardCplusplus.h"
//#include "serstream"
//using namespace std;


class SetSeqPixels : public LightShowCmd {

private:
	byte currPixelNum;
	
public:
	SetSeqPixels();
	
	SetSeqPixels(Adafruit_NeoPixel* strip,
						
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
						PixelColor *colorSeriesArr);
						
	SetSeqPixels(Adafruit_NeoPixel* strip, byte* stripCmdArray);
	
	void init(Adafruit_NeoPixel* strip,
						
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
						PixelColor *colorSeriesArr);

	void step(boolean isShowStrip);
	void reset();
	//int getCurrPixelColor();
};

#endif // SETSEQPIXELS_H
