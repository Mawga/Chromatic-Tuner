# Chromatic-Tuner
Chromatic Tuner Constructed on FPGA for UCSB's Fall 2017 ECE 153a
# Code
The files and corresponding headers written for this project: bsp.c, chromatic_tuner.c, note.c, tunermain.c
fft.c was optimized for more responsive tuning
# Hardware
The chromatic tuner comprised of a rotary encoder, lcd screen, and the Nexys 4 Artix-7 FPGA
# States
The chromatic tuner has five different states of behavior (chromatic_tuner.c)

Rotary encoder click cycled through the different states below
## Title_Screen
Displayed title screen on LCD.
## Octave_Selection_Screen
Rotary encoder is used to select octave of frequency to tune
## Error_Estimation_Screen
Displays note closest to the played frequency
A bar at the bottom indicates how far the played frequency is from displayed note
## A4_Tune_Screen and Blink_Screen
Rotary encoder is used to select base frequency to tune to (default A4)
A4_Tune_Screen enters Blink_Screen state clearing LCD display
States change every 0.5 seconds for a 1 second blink
