
#ifndef chromatictuner_h
#define chromatictuner_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum ChromaticTunerSignals {
	ENCODER_UP = Q_USER_SIG,
	ENCODER_DOWN,
	ENCODER_CLICK,
	CLOCK_TICK,
};


extern struct ChromaticTunerTag AO_ChromaticTuner;


void ChromaticTuner_ctor(void);
void ModeDisplay(u16 mode);
void GpioHandler(void *CallbackRef);
void PushHandler(void *CallbackRef);
void TmrHandler(void *CallbackRef, u8 TmrCtrNumber);
void BtnHandler(void *CallbackRef);
void TwistHandler(void *CallbackRef);
void DrawBackground();

#endif  
