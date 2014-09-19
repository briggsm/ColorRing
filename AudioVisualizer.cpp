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

#include "AudioVisualizer.h"
#include "PixelColor.h"

AudioVisualizer::AudioVisualizer() {
	// Clear
	for (byte i = 0; i < 64; i++) {
		visMatrix[i] = false;
	}
}

void AudioVisualizer::fillLine(byte section, byte height) {
	for (byte i = 0; i < height; i++) {
		if (i < 8) {
			visMatrix[section * 8 + i] = true;
		}
	}
}

void AudioVisualizer::fillPeakPixel(byte section, byte height) {
	if (height > 0 && height <= 8) {
		visMatrix[section * 8 + height - 1] = true;
	}
}

void AudioVisualizer::writeToStrip(Adafruit_NeoPixel* strip) {
	PixelColor pc;
	byte switchExpr;
	for (byte i = 0; i < 60; i++) {
		if (i < 30) {
			if (visMatrix[29 - i]) {
				switchExpr = (29 - i) % 8;
			} else {
				//continue;  // to next for loop iter
				switchExpr = 8;  // Black
			}
		} else {
			// i >= 30
			if (visMatrix[i+2]) {
				switchExpr = (i + 2) % 8;
			} else {
				//continue;  // to next for loop iter
				switchExpr = 8;  // Black
			}
		}
		
		switch (switchExpr) {
			case 0:
			case 1:
			case 2:
				pc = PixelColor(0x00, 0xFF, 0x00);  // Green
				break;
			case 3:
			case 4:
				pc = PixelColor(0x00, 0x00, 0xFF);  // Blue
				break;
			case 5:
			case 6:
			case 7:
				pc = PixelColor(0xFF, 0x00, 0x00);  // Red
				break;
			default:
				pc = PixelColor(0x00, 0x00, 0x00);  // Black
				break;
		}
		strip->setPixelColor(i, pc.getLongVal());
	}
}

float AudioVisualizer::getAmplitude() {
	// Return amplitude as a %'age (0-100)
	byte numBars = 0;
	for (byte i = 0; i < 64; i++) {
		//Serial.print("i:"); Serial.println(i);
		if (visMatrix[i]) {
			//Serial.println("incr");
			numBars++;
		}
	}
	
	//Serial.print("numBars:"); Serial.println(numBars);
	//if (numBars < 8) { numBars = 0; }  // cuz might be dropping peak pixel hanging around
	return numBars / 64.0 * 100 * 2;  // *2 fudge cuz this algo isn't perfect
}