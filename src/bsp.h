/*****************************************************************************
* bsp.h for Lab2A of ECE 153a at UCSB
* Date of the Last Update:  October 23,2014
*****************************************************************************/
#ifndef bsp_h
#define bsp_h
#include "xtmrctr.h"
#include <stdio.h>
#include "platform.h"
#include <mb_interface.h>

#include "xparameters.h"
#include <xil_types.h>
#include <xil_assert.h>

#include <xio.h>
#include "fft.h"
#include "trig.h"
#include "complex.h"
#include "note.h"
#include "stream_grabber.h"

#define SAMPLES 512 // AXI4 Streaming Data FIFO has size 512
#define M 9 //2^m=samples
#define CLOCK 100000000.0 //clock speed

int int_buffer[SAMPLES];
static float q[SAMPLES];
static float w[SAMPLES];
float sample_f;
int l;
int ticks; //used for timer
uint32_t Control;
float frequency;

                                              


/* bsp functions ..........................................................*/

void BSP_init(void);
void ISR_gpio(void);
void ISR_timer(void);

#define BSP_showState(prio_, state_) ((void)0)


#endif                                                             /* bsp_h */


