#include "qpn_port.h"
#include "bsp.h"
#include "xintc.h"
#include "xil_exception.h"
#include "xspi.h"
#include "lcd.h"
#include "xgpio.h"
#include "xil_cache.h"
#include "chromatic_tuner.h"



/*****************************/

/* Define all variables and Gpio objects here  */

#define GPIO_CHANNEL1 1

static XTmrCtr tmr;
static XIntc intc;
static XGpio TwistInput;
static XGpio PushInput;
static XGpio dc;
static XSpi spi;
XSpi_Config *spiConfig;
u32 controlReg;

int factor;

extern int state;
char buffer[6];
int octave = 0;
int a4tune = 440;
int clockwise;
int counterclockwise;
int onflag;
int offflag;
int blink;
int clockflag;

int buttonpushed = 0;
int idle = 0;
int pushenchigh = 0;
int pushenclow = 0;

int rotarypushflag = 0;
int rotarytwistflag = 0;

// Create two interrupt controllers XIntc
// Create two static XGpio variables
// Suggest Creating two int's to use for determining the direction of twist

void read_fsl_values(float* q, int n, int decimate) {
	int i;
	unsigned int x;
	for(i = 0; i < n*decimate; i += decimate) {
		int_buffer[i/decimate] = stream_grabber_read_sample(i);
		//xil_printf("%d\n",int_buffer[i]);
		x = int_buffer[i/decimate];
		q[i/decimate] = 3.3*x/67108864.0; // 3.3V and 2^26 bit precision.
	}
}

// Construct LUT for fft optimization
void LUTconstruct()
{
	int b = 1;
	int k = 0;
	for (int j=0; j<9; j++){
		for(int i=0; i<512; i+=2){
			if (i%(512/b)==0 && i!=0)
				k++;
			// functions in trig.c
			cosLUT[j][k] = cosine(-PI*k/b);
			sinLUT[j][k] = sine(-PI*k/b);
		}
		b*=2;
		k=0;
	}
}

void rotaryup()
{
	// If in octave selection state chromatic_tuner.c for state behavior
	if (state == 1)
	{
		// Increment selected octave
		if (octave < 9)
			++octave;
		if (octave < 7)
			factor = 8;
		else if (octave < 8)
			factor = 4;
		else if (octave < 9)
			factor = 2;
		else
			factor = 1;
		// set buffer to be displayed to new octave
		itoa(octave,buffer,10);
	}
	// If in tuning state chromatic_tuner.c for state behavior
	else if (state == 3)
	{
		// Increment displayed frequency
		if (a4tune < 460)
			++a4tune;
		// set buffer to be displayed to new frequency
		itoa(a4tune,buffer,10);
	}
}

void rotarydown()
{
	// If in octave selection state chromatic_tuner.c for state behavior
	if (state == 1)
	{
		// Decrement selected octave
		if (octave > 0)
			--octave;
		if (octave < 7)
			factor = 8;
		else if (octave < 8)
			factor = 4;
		else if (octave < 9)
			factor = 2;
		else
			factor = 1;
		// set buffer to be displayed to new octave
		itoa(octave,buffer,10);
	}
	// If in tuning state chromatic_tuner.c for state behavior
	else if (state == 3)
	{
		// Decrement displayed frequency
		if (a4tune > 420)
			--a4tune;
		// set buffer to be displayed to new frequency
		itoa(a4tune,buffer,10);
	}
}
/*..........................................................................*/
void BSP_init(void) {
	/* Setup LED's, etc */
	Xil_ICacheEnable();
	Xil_DCacheEnable();
	/* Setup interrupts and reference to interrupt handler function(s)  */
	
	//Initialize GPIO Devices ann connect to corresponding handlers
	XIntc_Initialize(&intc, XPAR_INTC_0_DEVICE_ID);
	XGpio_Initialize(&PushInput, XPAR_PUSH_DEVICE_ID);
	XIntc_Connect(&intc, XPAR_MICROBLAZE_0_AXI_INTC_PUSH_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler)PushHandler, &PushInput);
	XGpio_Initialize(&TwistInput, XPAR_TWIST_DEVICE_ID);
	XIntc_Connect(&intc, XPAR_MICROBLAZE_0_AXI_INTC_TWIST_IP2INTC_IRPT_INTR,
			(Xil_ExceptionHandler)TwistHandler, &TwistInput);
	XTmrCtr_Initialize(&tmr, XPAR_AXI_TIMER_0_DEVICE_ID);
	XIntc_Connect(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR,(XInterruptHandler)XTmrCtr_InterruptHandler,&tmr);
	XGpio_Initialize(&dc, XPAR_SPI_DC_DEVICE_ID);
	// Row of LEDs start off
	XGpio_SetDataDirection(&dc, 1, 0x0);

	// Construct LUT for fft optimization
	LUTconstruct();


	/*
	 * Initialize the interrupt controller driver so that it's ready to use.
	 * specify the device ID that was generated in xparameters.h
	 *
	 * Initialize GPIO and connect the interrupt controller to the GPIO.
	 *
	 */

}
/*..........................................................................*/
void QF_onStartup(void) {                 /* entered with interrupts locked */
	init_platform();
	spiConfig = XSpi_LookupConfig(XPAR_SPI_DEVICE_ID);
	XSpi_CfgInitialize(&spi, spiConfig, spiConfig->BaseAddress);
	XSpi_Reset(&spi);
	controlReg = XSpi_GetControlReg(&spi);
	XSpi_SetControlReg(&spi,
			(controlReg | XSP_CR_ENABLE_MASK | XSP_CR_MASTER_MODE_MASK) &
			(~XSP_CR_TRANS_INHIBIT_MASK));
	XSpi_SetSlaveSelectReg(&spi, ~0x01);

	// Draw startup screen
	initLCD();
	clrScr();
	setColor(178, 34, 34);
	fillRect(0, 0, 240, 320);
	setColor(176, 224, 230);
	setColorBg(178, 34, 34);

	setFont(BigFont);
	lcdPrint("Chromatic", 5, 30);
	lcdPrint("Tuner", 5, 50);

	XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_PUSH_IP2INTC_IRPT_INTR);
	XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_TWIST_IP2INTC_IRPT_INTR);
	// Start interupt controller
	XIntc_Start(&intc, XIN_REAL_MODE);
	XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_AXI_TIMER_0_INTERRUPT_INTR);

	// register handler with Microblaze
	// Global enable of interrupt
	XGpio_InterruptGlobalEnable(&PushInput);
	XGpio_InterruptGlobalEnable(&TwistInput);
	// Enable interrupt on the GPIO
	XGpio_InterruptEnable(&PushInput, 1);
	XGpio_InterruptEnable(&TwistInput, 1);
	Xil_ExceptionInit();
	Xil_ExceptionRegisterHandler(XIL_EXCEPTION_ID_INT,
			(Xil_ExceptionHandler)XIntc_InterruptHandler, &intc);
	Xil_ExceptionEnable();
	XTmrCtr_SetHandler(&tmr, TmrHandler,
			&tmr);
	XTmrCtr_SetOptions(&tmr, 0, XTC_INT_MODE_OPTION | XTC_AUTO_RELOAD_OPTION);
	// Time  = (reset value - set value)/clock frequency
	// Time = (0xFFFFFFFF - 0xFFF85EDF)/(100 MHz) = 5 ms
	XTmrCtr_SetResetValue(&tmr, 0, 0xFFF85EDF);
	XTmrCtr_Start(&tmr, 0);

	// Variables for reading Microblaze registers to debug interrupts.
	//	{
	//		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
	//		u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
	//		u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
	//		u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
	//		u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
	//		u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
	//		u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
	//		u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
	//		u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
	//		u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
	//		u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
	//		u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
	//		u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003; // & 0xMASK
	//		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000; // & 0xMASK
	//	}



}


void QF_onIdle(void) {        /* entered with interrupts locked */

	QF_INT_UNLOCK();                       /* unlock interrupts */

	{

		// If encoder was turned CW
		if (onflag)
		{
			// Run CW encoder behavior
			rotaryup();
			QActive_postISR((QActive *)&AO_ChromaticTuner, ENCODER_UP);
			onflag = 0;
		}
		// If encoder was turned CCW
		else if (offflag)
		{
			// Run CCW encoder behavior
			rotarydown();
			QActive_postISR((QActive *)&AO_ChromaticTuner, ENCODER_DOWN);
			offflag = 0;
		}

		// QActive_postISR((QActive *)&AO_Lab2A, ENCODER_DOWN); is used to post an event to your FSM



		// 			Useful for Debugging, and understanding your Microblaze registers.
		//    		u32 axi_ISR =  Xil_In32(intcPress.BaseAddress + XIN_ISR_OFFSET);
		//    	    u32 axi_IPR =  Xil_In32(intcPress.BaseAddress + XIN_IPR_OFFSET);
		//    	    u32 axi_IER =  Xil_In32(intcPress.BaseAddress + XIN_IER_OFFSET);
		//
		//    	    u32 axi_IAR =  Xil_In32(intcPress.BaseAddress + XIN_IAR_OFFSET);
		//    	    u32 axi_SIE =  Xil_In32(intcPress.BaseAddress + XIN_SIE_OFFSET);
		//    	    u32 axi_CIE =  Xil_In32(intcPress.BaseAddress + XIN_CIE_OFFSET);
		//    	    u32 axi_IVR =  Xil_In32(intcPress.BaseAddress + XIN_IVR_OFFSET);
		//    	    u32 axi_MER =  Xil_In32(intcPress.BaseAddress + XIN_MER_OFFSET);
		//    	    u32 axi_IMR =  Xil_In32(intcPress.BaseAddress + XIN_IMR_OFFSET);
		//    	    u32 axi_ILR =  Xil_In32(intcPress.BaseAddress + XIN_ILR_OFFSET) ;
		//    	    u32 axi_IVAR = Xil_In32(intcPress.BaseAddress + XIN_IVAR_OFFSET);
		//
		//    	    // Expect to see 0x00000001
		//    	    u32 gpioTestIER  = Xil_In32(sw_Gpio.BaseAddress + XGPIO_IER_OFFSET);
		//    	    // Expect to see 0x00000001
		//    	    u32 gpioTestISR  = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_ISR_OFFSET ) & 0x00000003;
		//
		//    	    // Expect to see 0x80000000 in GIER
		//    		u32 gpioTestGIER = Xil_In32(sw_Gpio.BaseAddress  + XGPIO_GIE_OFFSET ) & 0x80000000;


	}
}

/* Do not touch Q_onAssert */
/*..........................................................................*/
void Q_onAssert(char const Q_ROM * const Q_ROM_VAR file, int line) {
	(void)file;                                   /* avoid compiler warning */
	(void)line;                                   /* avoid compiler warning */
	QF_INT_LOCK();
	for (;;) {
	}
}

/* Interrupt handler functions here.  Do not forget to include them in lab2a.h!
   To post an event from an ISR, use this template:
   QActive_postISR((QActive *)&AO_Lab2A, SIGNALHERE);
   Where the Signals are defined in lab2a.h  */

/******************************************************************************
 *
 * This is the interrupt handler routine for the GPIO for this example.
 *
 ******************************************************************************/

void TmrHandler(void *CallbackRef, u8 TmrCtrNumber)	{
	if (idle == 1)
		XIntc_Enable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_PUSH_IP2INTC_IRPT_INTR);

	// Check if state has been changed to A4_Tune_Screen
	if (state == 3)
	{
		// Once changed to state A4_Tune_Screen blink the screen on every 100 timer interrupt
		if (buttonpushed == 1)
			buttonpushed = 0;
		++blink;
		// Timer interrupt called every 5 ms
		// Screen will change state every .5 seconds, blinking every second
		if (blink >= 100)
		{
			QActive_postISR((QActive *)&AO_ChromaticTuner, CLOCK_TICK);
			blink = 0;
		}
	}

	XTmrCtr_Reset(&tmr, 0);
}

void PushHandler(void *CallbackRef) {
	u32 readdata;
	XGpio_SetDataDirection(&PushInput, 1, 0xFFFFFFFF);
	readdata = XGpio_DiscreteRead(&PushInput, 1);

	
	if (state == 2)
		buttonpushed = 1;

	// Debounce push on encoder push
	if (readdata == 1 && pushenchigh == 0 && pushenclow == 0)
	{
		++pushenchigh;
	}
	if (readdata == 0 && pushenclow == 0 && pushenchigh == 1)
	{
		++pushenclow;
	}
	if (pushenchigh > 1 || pushenclow > 1)
	{
		pushenchigh = 0;
		pushenclow = 0;
	}
	if (pushenclow == 1 && pushenchigh == 1)
	{
		// If successful push then activate push behavior
		QActive_postISR((QActive *)&AO_ChromaticTuner, ENCODER_CLICK);
		pushenclow = 0;
		pushenchigh = 0;
		idle = 1;
		XIntc_Disable(&intc, XPAR_MICROBLAZE_0_AXI_INTC_PUSH_IP2INTC_IRPT_INTR);
	}

	XTmrCtr_Reset(&tmr, 0);
	//Clear the Interrupt
	XGpio_InterruptClear(&PushInput, 1);
}

void TwistHandler(void *CallbackRef) {
	// Call to debounce rotary twist input
	debounceTwistInterrupt();
	XGpio_InterruptClear(&TwistInput, 1);
}

void debounceTwistInterrupt(){
	// Rotary encoder sends sequence 1-0-2-3 indicates a successful clockwise turn
	// 3-2-0-1 counter clockwise
	u32 readdata;

	XGpio_SetDataDirection(&TwistInput, 1, 0xFFFFFFFF);
	readdata = XGpio_DiscreteRead(&TwistInput, 1);
	switch(readdata)
	{
		case 0b01:
			if (counterclockwise)
			{
				++counterclockwise;
			}
			else
			{
				clockwise = 1;
			}
			break;
		case 0b00:
			if(clockwise)
			{
				++clockwise;
			}
			else if (counterclockwise)
			{
				++counterclockwise;
			}
			break;
		case 0b10:
			if(clockwise)
			{
				++clockwise;
			}
			else
			{
				counterclockwise = 1;
			}
			break;
		case 0b11:
			//Successful clockwise turn
			if (clockwise > 2)
				onflag = 1;
			//Successful counter clockwise turn
			if (counterclockwise > 2)
				offflag = 1;

			clockwise = 0;
			counterclockwise = 0;
			break;
		default:
			break;
	}
	/* Clear the Interrupt */
	XGpio_InterruptClear(&TwistInput, 1);

}
