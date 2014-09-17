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

#ifndef AUDIOVISUALIZER_H
#define AUDIOVISUALIZER_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

class AudioVisualizer {
private:
	bool visStrip[60];	
public:
	AudioVisualizer();
	void fillLine(byte section, byte height);
	void fillPeakPixel(byte section, byte height);
	void writeToStrip(Adafruit_NeoPixel* strip);
};

#endif