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
	for (byte i = 0; i < 60; i++) {
		visStrip[i] = false;
	}
}

void AudioVisualizer::fillLine(byte section, byte height) {
	for (byte i = 0; i < height; i++) {
		visStrip[section * 8 + i] = true;
	}
}

void AudioVisualizer::fillPeakPixel(byte section, byte height) {
	visStrip[section * 8 + height] = true;
}

void AudioVisualizer::writeToStrip(Adafruit_NeoPixel* strip) {
	PixelColor pc;
	for (byte i = 0; i < strip->numPixels(); i++) {
		if (visStrip[i]) {
			switch (i % 8) {
				case 0:
				case 1:
				case 2:
					pc = PixelColor(0x00, 0xFF, 0x00);  // Green
					break;
				case 3:
				case 4:
					pc = PixelColor(0xFF, 0xFF, 0x00);  // Yellow
					break;
				default:
					pc = PixelColor(0xFF, 0x00, 0x00);  // Red
					break;
			}
			strip->setPixelColor(i, pc.getLongVal());
		} else {
			// Just incase
			strip->setPixelColor(i, 0,0,0);  // Black
		}
	}
}