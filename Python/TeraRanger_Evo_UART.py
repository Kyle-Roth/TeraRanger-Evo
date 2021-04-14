#!/usr/bin/env python3
###### TeraRanger Evo Example Code STD #######
#                                            #
# All rights reserved Terabee France (c) 2018#
#                                            #
############ www.terabee.com #################

import serial
import numpy as np
import serial.tools.list_ports
import crcmod.predefined  # To install: pip install crcmod
from scipy.fftpack import fft, fftfreq
import matplotlib.pyplot as plt

class TeraRanger:

    port = None
    evo = None
    data = []
    freqs = []
    bpm = None

    def __init__(self):

        print('Initializing Evo Data Stream')

        self.port = self.findEvo()

        if self.port == 'NULL':
            print("Could not find Evo")
        else:
            print("Found Evo")
            self.evo = self.openEvo()

    def findEvo(self):
        # Find Live Ports, return port name if found, NULL if not
        print('Scanning all live ports on this PC')
        ports = list(serial.tools.list_ports.comports())
        for p in ports:
            # print p # This causes each port's information to be printed out.
            if "5740" in p[2]:
                print('Evo found on port ' + p[0])
                return p[0]
        return 'NULL'

    def openEvo(self):
        portname = self.port
        print('Attempting to open port...')
        # Open the Evo and catch any exceptions thrown by the OS
        print(portname)
        self.evo = serial.Serial(portname, baudrate=115200, timeout=2)
        # Send the command "Binary mode"
        set_bin = (0x00, 0x11, 0x02, 0x4C)
        # Flush in the buffer
        self.evo.flushInput()
        # Write the binary command to the Evo
        self.evo.write(set_bin)
        # Flush out the buffer
        self.evo.flushOutput()
        print('Serial port opened')
        return self.evo

    def get_evo_range(self):
        evo_serial = self.evo
        crc8_fn = crcmod.predefined.mkPredefinedCrcFun('crc-8')
        # Read one byte
        data = evo_serial.read(1)
        if data == b'T':
            # After T read 3 bytes
            frame = data + evo_serial.read(3)
            if frame[3] == crc8_fn(frame[0:3]):
                # Convert binary frame to decimal in shifting by 8 the frame
                rng = frame[1] << 8
                rng = rng | (frame[2] & 0xFF)
            else:
                return "CRC mismatch. Check connection or make sure only one progam access the sensor port."
        # Check special cases (limit values)
        else:
            return "Waiting for frame header"

        # Checking error codes
        if rng == 65535: # Sensor measuring above its maximum limit
            dec_out = float('inf')
        elif rng == 1: # Sensor not able to measure
            dec_out = float('nan')
        elif rng == 0: # Sensor detecting object below minimum range
            dec_out = -float('inf')
        else:
            # Convert frame in meters
            dec_out = rng / 1000.0
        return dec_out

    def streamData(self):
        print('Starting Evo Data Stream')

        while True:
            try:
                temp = self.get_evo_range(self.evo)
                self.data.append(temp)

                # Calculate Forier Transform
                X = fft(self.data)
                self.freqs = fftfreq(len(self.data), d = 1/115200) # d = 1/baudrate = sample rate

                 # print the positive frequency that has the most correlation
                self.bpm = 120*self.freqs[np.argmax(np.abs(X[0:int(len(X)/2)]))]
            except serial.serialutil.SerialException:
                print("Device disconnected (or multiple access on port). Exiting...")
                break

    def _close(self):
        self.evo.close()

if __name__ == "__main__":

    Evo60 = TeraRanger()
    data = freqs = X = []

    while True:
        temp = Evo60.get_evo_range()
        if temp != 'Waiting for frame header':
            data.append(50*temp)
            # print(temp)
        
        if len(data) == 1500:
            # Calculate Forier Transform
            print(data)
            X = fft(data)
            freqs = fftfreq(len(data), d = 1/115200) # d = 1/baudrate = sample rate
        


             # print the positive frequency that has the most correlation
            bpm = 120*freqs[np.argmax(np.abs(X[1:int(len(X)/2)]))]

            print(freqs)
            print(f"{bpm} bpm");
            break
            
            
    fig, ax = plt.subplots()
    ax.plot(data)
    
    fig2,ax2 = plt.subplots()
    ax2.plot(freqs[1:],np.abs(X[1:]))
    plt.show()
