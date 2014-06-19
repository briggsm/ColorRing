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

#ifndef GRADIENT_H
#define GRADIENT_H

#include <Arduino.h>
#include "PixelColor.h"

#include "StandardCplusplus.h"
#include "serstream"
#include <vector>
using namespace std;


class Gradient {

private:
	PixelColor startPixelColor;
	PixelColor endPixelColor;
	byte numTweens;  // Inclusive of first & last
	
	boolean rIncrDir, gIncrDir, bIncrDir;
	unsigned long rIncrAmt1024, gIncrAmt1024, bIncrAmt1024; // *1024 to keep some math accuracy
	
public:
	Gradient();
	Gradient(PixelColor startPixelColor, PixelColor endPixelColor, byte numTweens);
	
	PixelColor getTweenPixelColor(byte tweenNum);
};


class MultiGradient {
private:
	byte numPixels;  // Inclusive
	std::vector<PixelColor> mgArr;
public:
	MultiGradient();
	MultiGradient(PixelColor *colorSeriesArr, byte numColorsInSeries, byte numPixelsEachColor, bool gradiateLastPixelFirstColor);
	
	byte getNumPixels();
	PixelColor getTweenPixelColor(byte tweenNum);
};

#endif // GRADIENT_H
