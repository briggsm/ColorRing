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

#include "Gradient.h"

Gradient::Gradient() {
	
}
	
Gradient::Gradient(PixelColor startPixelColor, PixelColor endPixelColor, byte numTweens) {
	this->startPixelColor = startPixelColor;
	this->endPixelColor = endPixelColor;
	this->numTweens = numTweens;  // Inclusive of first & last
	
	byte tempDiff;
	
	// Red
	if (endPixelColor.R - startPixelColor.R >= 0) {
		rIncrDir = true; // Byte Increases (gets brighter)
		tempDiff = endPixelColor.R - startPixelColor.R;
	} else {
		rIncrDir = false; // Byte Decreases (gets dimmer)
		tempDiff = startPixelColor.R - endPixelColor.R;
	}
	rIncrAmt1024 = (((unsigned long)tempDiff) * 1024) / (numTweens - 1);  // *1024 to keep some accuracy in the math ('cuz 1.5=>1, but 1500=>1500)
	
	// Green
	if (endPixelColor.G - startPixelColor.G >= 0) {
		gIncrDir = true; // Byte Increases (gets brighter)
		tempDiff = endPixelColor.G - startPixelColor.G;
	} else {
		gIncrDir = false; // Byte Decreases (gets dimmer)
		tempDiff = startPixelColor.G - endPixelColor.G;
	}
	gIncrAmt1024 = (((unsigned long)tempDiff) * 1024) / (numTweens - 1);
	
	// Blue
	if (endPixelColor.B - startPixelColor.B >= 0) {
		bIncrDir = true; // Byte Increases (gets brighter)
		tempDiff = endPixelColor.B - startPixelColor.B;
	} else {
		bIncrDir = false; // Byte Decreases (gets dimmer)
		tempDiff = startPixelColor.B - endPixelColor.B;
	}
	bIncrAmt1024 = (((unsigned long)tempDiff) * 1024) / (numTweens - 1);

}

PixelColor Gradient::getTweenPixelColor(byte tweenNum) {
	
	PixelColor newColor;
	
	if (tweenNum == 0) {
		newColor = startPixelColor;
	} else if (tweenNum >= numTweens - 1) {
		newColor = endPixelColor;
	} else {
		byte newR, newG, newB;
		
		// Red
		if (rIncrDir == true) {
			newR = startPixelColor.R + (tweenNum * rIncrAmt1024 / 1024);
		} else {
			newR = startPixelColor.R - (tweenNum * rIncrAmt1024 / 1024);
		}
		
		// Green
		if (gIncrDir == true) {
			newG = startPixelColor.G + (tweenNum * gIncrAmt1024 / 1024);
		} else {
			newG = startPixelColor.G - (tweenNum * gIncrAmt1024 / 1024);
		}
		
		// Blue
		if (bIncrDir == true) {
			newB = startPixelColor.B + (tweenNum * bIncrAmt1024 / 1024);
		} else {
			newB = startPixelColor.B - (tweenNum * bIncrAmt1024 / 1024);
		}

		newColor = PixelColor(newR, newG, newB);
	}
	
	//Serial.print("newColor: "); newColor.println();
	return newColor;
	
}

/********************
MultiGradient
********************/

MultiGradient::MultiGradient() {
	
}

MultiGradient::MultiGradient(PixelColor *colorSeriesArr, byte numColorsInSeries, byte numPixelsEachColor, bool gradiateLastPixelFirstColor) {
	numPixels = numColorsInSeries * numPixelsEachColor;  // Inclusive
	
	byte numGradients = gradiateLastPixelFirstColor ? numColorsInSeries : numColorsInSeries - 1;
	if (numGradients == 255) { numGradients = 0; }
	
	//cout << F("NumGradients: ") << (int)numGradients << endl;

	float fNumIncrPerGradientSection;

	if (gradiateLastPixelFirstColor) {
		fNumIncrPerGradientSection = numPixelsEachColor;
	} else {
		fNumIncrPerGradientSection = (numPixels-1) * 1.0 / (numColorsInSeries - 1);
	}
	
	//cout << F("fNumIncr: ") << fNumIncrPerGradientSection << endl;
	
	PixelColor startPixelColor, endPixelColor;
	byte startPixel, endPixel, numTweens;
	byte ctr = 0;
	for (int currGradNum = 0; currGradNum < numGradients; currGradNum++) {
		//cout << F("currGradNum: ") << (int)currGradNum << endl;
		
		startPixelColor = colorSeriesArr[currGradNum];
		if (gradiateLastPixelFirstColor && (currGradNum == numGradients - 1)) {
			endPixelColor = colorSeriesArr[0];
		} else {
			endPixelColor = colorSeriesArr[currGradNum+1];
		}
		
		startPixel = (int)(fNumIncrPerGradientSection * currGradNum);
		endPixel = (int)((fNumIncrPerGradientSection * (currGradNum + 1)) - 1);
		//cout << F("startPixel: ") << (int)startPixel << F(", endPixel: ") << (int)endPixel << endl;
		
		numTweens = endPixel - startPixel + 1 + 1;  // +1 to get to next gradient's START pixel, +1 more to be inclusive
		
		Gradient grad = Gradient(startPixelColor, endPixelColor, numTweens);
		
		for (int currTweenNum = 0; currTweenNum < numTweens-1; currTweenNum++) {
			PixelColor pc = grad.getTweenPixelColor(currTweenNum);
			//cout << F("ctr: ") << (int)ctr << endl;
			//cout << F("pc: "); pc.println();
			mgArr.push_back(pc);
			ctr++;
		}
	}
	
	// Last Pixel
	mgArr.push_back((gradiateLastPixelFirstColor ? colorSeriesArr[0] : colorSeriesArr[numColorsInSeries-1]));
	
	//cout << F("mgArr size: ") << mgArr.size() << endl;
}

byte MultiGradient::getNumPixels() {
	// Inclusive

	return numPixels;
}

PixelColor MultiGradient::getTweenPixelColor(byte tweenNum) {
	return mgArr[tweenNum % numPixels];
}