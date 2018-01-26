This folder contains firmware and its source for reset user code.

This is not elegant solution, because it does not zeroise entire user code.
Firmware is a program, which configures Arduino Mega development board
Yellow LED as output and turns it off. Then program exits.

Currently it seems to be only feasible way to reset Arduino board without
external programmer.
See this Arduino Stack Exchange topic:
https://arduino.stackexchange.com/questions/460/how-do-i-erase-the-memory-of-the-arduino-uno