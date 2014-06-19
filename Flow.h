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

#ifndef FLOW_H
#define FLOW_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "LightShowCmd.h"
#include "PixelColor.h"
#include "utility/debug.h"  // for getFreeRam()

#include "StandardCplusplus.h"
#include "serstream"
using namespace std;

class Flow : public LightShowCmd {

protected:
	//byte numSections;
	
public:
	
	Flow();

	Flow(Adafruit_NeoPixel* strip,
						
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
			PixelColor *colorSeriesArr);
	
	Flow(Adafruit_NeoPixel* strip, byte* stripCmdArray);
	
	void init(Adafruit_NeoPixel* strip,
						
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
				PixelColor *colorSeriesArr);
	
	void step(boolean isShowStrip);
	void singleFlow(Adafruit_NeoPixel* strip, byte sourcePixelNum, byte drainPixelNum, PixelColor pixelColor, boolean direction, byte numPixelsToSkip);
	void reset();
};

#endif
