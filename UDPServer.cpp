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

#include "UDPServer.h"

UDPServer::UDPServer(uint16_t port) {
	_port = port;
	_socket = -1;
}

bool UDPServer::begin() {
	//Serial.print("START of udpServer::Begin() _socket: "); Serial.println(_socket);
	// Open the socket if it isn't already open.
	if (_socket == -1) {
		// Create the UDP socket
		int soc = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		if (soc < 0) {
			Serial.println("socket() call failed");
			return false;
		}

		sockaddr_in address;
		memset(&address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(_port);
		address.sin_addr.s_addr = 0;  // 0 => auto use own ip address
		socklen_t len = sizeof(address);
		if (bind(soc, (sockaddr*) &address, sizeof(address)) < 0) {
			Serial.println("bind() call failed");
			return false;
		}
		
		_socket = soc;
	}

	Serial.print("END of udpServer::begin() _socket: "); Serial.println(_socket);

	return true;
}

bool UDPServer::available() {
	timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 5000;
	fd_set reads;
	FD_ZERO(&reads);
	FD_SET(_socket, &reads);
	select(_socket + 1, &reads, NULL, NULL, &timeout);
	if (!FD_ISSET(_socket, &reads)) {
		// No data to read.
		//Serial.println("No data to read.");
		return false;
	}
	
	return true;
}

int UDPServer::readData(char *buffer, int bufferSize) {
	// If there is data, then stores it into buffer &
	// returns the length of buffer. (-1 if none)
	
	if (available()) {  // Make sure data is really available

		int n = recv(_socket, buffer, bufferSize, 0);
		
		if (n < 1) {
			// Error getting data.
			Serial.println("Error getting data");
			return -1;
		}
		
		return n;
	}
	
	return -1;
}
