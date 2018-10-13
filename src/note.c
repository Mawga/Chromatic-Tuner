#include "note.h"
#include <math.h>
#include "xil_printf.h"
//#include "lcd.h"

//array to store note names for findNote
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

//finds and prints note of frequency and deviation from note according to equation
// f = a*2^(-3/4 + k - 4)
// f: frequency
// a: value of A4 selected on A4 tuning screen
// k: octave selected on octave selection screen
int findNote(float f, int octave, int a, int * cents) {
	float c = (float)a*pow(2.0,(-9.0/12.0+(float)octave-4.0));
	float highC = c*halfway;
	float lowC= c/halfway;
	int centslocal = 1;
	int note = 0;

	while (highC < f && note < 11)
	{
		c = c*root2;
		lowC = lowC*root2;
		highC = highC*root2;
		++note;
	}

	while(lowC < f)
	{
		lowC = lowC * centvalue;
		++centslocal;
	}

	centslocal = centslocal - 50;
	*cents = centslocal;

	return note;
}
