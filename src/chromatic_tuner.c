
#define AO_CHROMATICTUNER

#include "qpn_port.h"
#include "bsp.h"
#include "chromatic_tuner.h"
#include "lcd.h"

extern int factor;
extern int octave;
extern int a4tune;
extern int buttonpushed;
extern char buffer[6];
char errorVal[6];
int prevError = 0;
int cents = 0;
int state = 0;
int note;

// Corresponding notes for tuner
static char notes[12][3]={"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

typedef struct ChromaticTunerTag  {
	QActive super;
}  ChromaticTuner;

/* Setup state machines */
/**********************************************************************/
static QState ChromaticTuner_initial (ChromaticTuner *me);
static QState ChromaticTuner_on      (ChromaticTuner *me);
static QState Title_Screen  (ChromaticTuner *me);
static QState Octave_Selection_Screen  (ChromaticTuner *me);
static QState Error_Estimation_Screen  (ChromaticTuner *me);
static QState A4_Tune_Screen  (ChromaticTuner *me);
static QState Blink_Screen  (ChromaticTuner *me);


/**********************************************************************/


ChromaticTuner AO_ChromaticTuner;

void DrawBackground()
{
	setColor(178, 34, 34);
	fillRect(0, 0, 240, 100);
	setColor(176, 224, 230);
	setColorBg(178, 34, 34);
}


void ChromaticTuner_ctor(void)  {
	ChromaticTuner *me = &AO_ChromaticTuner;
	QActive_ctor(&me->super, (QStateHandler)&ChromaticTuner_initial);
}


QState ChromaticTuner_initial(ChromaticTuner *me) {
	xil_printf("\n\rInitialization");
	return Q_TRAN(&ChromaticTuner_on);
}

QState ChromaticTuner_on(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  xil_printf("\n\rOn");
				  }

		case Q_INIT_SIG: {
					 return Q_TRAN(&Title_Screen);
				 }
	}

	return Q_SUPER(&QHsm_top);
}


/* Create ChromaticTuner_on state and do any initialization code if needed */
/******************************************************************/

QState Title_Screen(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  state = 0;
					  return Q_HANDLED();
				  }

		case ENCODER_UP: {
					 return Q_HANDLED();
				 }

		case ENCODER_DOWN: {
					   return Q_HANDLED();
				   }

		case ENCODER_CLICK:  {
					     DrawBackground();
					     lcdPrint("Octave: ", 5, 30);
					     itoa(octave,buffer,10);
					     lcdPrint(buffer,115,30);
					     return Q_TRAN(&Octave_Selection_Screen);
				     }
		case CLOCK_TICK: {
					 return Q_HANDLED();

				 }

	}

	return Q_SUPER(&ChromaticTuner_on);

}

// State 1
QState Octave_Selection_Screen(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  state = 1;
					  return Q_HANDLED();
				  }

		case ENCODER_UP: {
					 lcdPrint(buffer,115,30);
					 return Q_HANDLED();
				 }

		case ENCODER_DOWN: {
					   lcdPrint(buffer,115,30);
					   return Q_HANDLED();
				   }
		// Draw and switch to Error Estimation state
		case ENCODER_CLICK:  {
					     DrawBackground();
					     lcdPrint("Octave: ", 5, 30);
					     lcdPrint(buffer,115,30);
					     lcdPrint("Note: ", 5, 50);
					     lcdPrint("Error: ", 5, 70);
					     return Q_TRAN(&Error_Estimation_Screen);
				     }
	}

	return Q_SUPER(&ChromaticTuner_on);

}

// State 2
QState Error_Estimation_Screen(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  state = 2;
					  while (buttonpushed == 0)
					  {
						  read_fsl_values(q, SAMPLES, factor);
						  stream_grabber_start();
						  sample_f = 100*1000*1000/2048.0;
						  //xil_printf("sample frequency: %d \r\n",(int)sample_f);

						  //zero w array
						  for(l=0;l<SAMPLES;l++)
							  w[l]=0;

						  frequency=fft(q,w,SAMPLES,M,sample_f/factor);
						  //xil_printf("frequency: %d Hz\r\n", (int)(frequency+.5));
						  note = findNote(frequency, octave, a4tune, &cents);
						  //xil_printf("%d\n",note);
						  itoa(cents,errorVal,10);
						  setColor(178, 34, 34);
						  fillRect(85, 50, 240, 69);
						  setColor(176, 224, 230);
						  lcdPrint(notes[note], 85, 50);

						  // Draw green bar to right of center direction
						  if (cents > 50)
						  {
							  setColor(178, 34, 34);
							  fillRect(100,70,240,100);
							  fillRect(120,300,120+(int)(prevError*2.4),320);
							  setColor(176, 224, 230);
							  lcdPrint(errorVal, 100, 70);
							  fillRect(120,300,240,320);
							  prevError = 50;
						  }
						  // Draw red error bar to left of center direction
						  else if (cents < -50)
						  {
							  setColor(178, 34, 34);
							  fillRect(100,70,240,100);
							  fillRect(120,300,120+(int)(prevError*2.4),320);
							  setColor(176, 224, 230);
							  lcdPrint(errorVal, 100, 70);
							  fillRect(0,300,120,320);
							  prevError = -50;
						  }
						  // Erase drawn bar according to encoder direction
						  // Progressing the bar to the left or to the right
						  else
						  {
							  setColor(178, 34, 34);
							  fillRect(100,70,240,100);
							  fillRect(120,300,120+(int)(prevError*2.4),320);
							  setColor(176, 224, 230);
							  lcdPrint(errorVal, 100, 70);
							  fillRect(120,300,120+(int)(cents*2.4),320);
							  prevError = cents;
						  }
						  //xil_printf("%d\n",cents);
						  //xil_printf("%d\n", note);
					  }
					  return Q_HANDLED();
				  }

		case ENCODER_UP: {
					 return Q_HANDLED();
				 }

		case ENCODER_DOWN: {

					   return Q_HANDLED();
				   }
		case ENCODER_CLICK:  {
					     DrawBackground();
					     setColor(178, 34, 34);
					     fillRect(0,300,240,320);
					     setColor(176, 224, 230);
					     lcdPrint("Tune Of A4: ", 5, 30);
					     itoa(a4tune,buffer,10);
					     lcdPrint(buffer,180,30);
					     return Q_TRAN(&A4_Tune_Screen);
				     }
	}

	return Q_SUPER(&ChromaticTuner_on);

}

// State 3
QState A4_Tune_Screen(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  state = 3;
					  return Q_HANDLED();
				  }

		case ENCODER_UP: {
					// Buffer contains tuner frequency
					 lcdPrint(buffer,180,30);
					 return Q_HANDLED();
				 }

		case ENCODER_DOWN: {
					// Buffer contains tuner frequency
					   lcdPrint(buffer,180,30);
					   return Q_HANDLED();
				   }

		case CLOCK_TICK: {
					// Enter empty screen state
					 state = 0;
					 setColor(178, 34, 34);
					 fillRect(180, 30, 240, 90);
					 setColor(176, 224, 230);
					 state = 3;
					 return Q_TRAN(&Blink_Screen);
				 }

		case ENCODER_CLICK:  {
					     state = 0;
					     DrawBackground();
					     lcdPrint("Chromatic", 5, 30);
					     lcdPrint("Tuner", 5, 50);
					     return Q_TRAN(&Title_Screen);
				     }
	}

	return Q_SUPER(&ChromaticTuner_on);

}

// On timer interrupt
QState Blink_Screen(ChromaticTuner *me) {
	switch (Q_SIG(me)) {
		case Q_ENTRY_SIG: {
					  state = 3;
					  return Q_HANDLED();
				  }

		case ENCODER_UP: {
					// Buffer contains tuner frequency
					// Screen is in non blink state while tuned
					 lcdPrint(buffer,180,30);
					 return Q_TRAN(&A4_Tune_Screen);
				 }

		case ENCODER_DOWN: {
					// Buffer contains tuner frequency
					// Screen is in non blink state while tuned
					   lcdPrint(buffer,180,30);
					   return Q_TRAN(&A4_Tune_Screen);
				   }

		case CLOCK_TICK: {
					// Return to fully drawn screen state
					 state = 0;
					 setColor(176, 224, 230);
					 lcdPrint(buffer,180,30);
					 return Q_TRAN(&A4_Tune_Screen);
				 }

		case ENCODER_CLICK:  {
					     DrawBackground();
					     lcdPrint("Chromatic", 5, 30);
					     lcdPrint("Tuner", 5, 50);
					     return Q_TRAN(&Title_Screen);
				     }
	}

	return Q_SUPER(&ChromaticTuner_on);

}


