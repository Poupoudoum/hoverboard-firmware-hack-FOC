#include <stdio.h>
#include <stdlib.h> // for abs()
#include "stm32f1xx_hal.h"
#include "defines.h"
#include "setup.h"
#include "chips.h"
#include "config.h"
#include "util.h"
#include "BLDC_controller.h"      /* BLDC's header file */
#include "rtwtypes.h"
#include "comms.h"



extern P    rtP_Left;
extern P    rtP_Right;

extern int16_t speedAvg;  

extern int16_t    speed;
extern int16_t    steer;

extern uint8_t     inIdx;               // input index used for dual-inputs
extern uint8_t     inIdx_prev;
extern InputStruct input1[];            // input structure
extern InputStruct input2[];            // input structure

static MultipleTap MultipleThrotle;
uint32_t nextBurstMs = 0;
uint32_t burstMaxMs = 0;
boolean_T burst = false;

#define BURST_DURATION  5000 ; //miliseconds
#define BURST_REFRESH  45000 ; //miliseconds




void chips_tunning_motorcontrol() {
    int16_t multiple;
    int16_t imax = I_MOT_MAX;
    uint32_t ms = HAL_GetTick();
    
    ////////////////// BURST ON START
    if (speedAvg < N_STARTUP_BOOST) {
        imax = I_MOT_BURST;
    } else if (speedAvg < N_STARTUP_BOOST * 2) {
        multiple = (N_STARTUP_BOOST - (speedAvg - N_STARTUP_BOOST)) * 100 / N_STARTUP_BOOST; //multiple between 0 and 100
        imax = I_MOT_MAX + ((I_MOT_BURST - I_MOT_MAX) * multiple / 100);
    }
    ////////////////// END BURST ON START
    
    ////////////////// BURST ON DOUBLE TAP THROTLE
    multipleTapDet(input2[inIdx].cmd, ms, &MultipleThrotle);
    if (!MultipleThrotle.b_multipleTap) {  
        // BURST !!!
        burst = true;
    }
    if (burst) {
        //check burst time init
        if (!burstMaxMs && nextBurstMs < ms) {
            burstMaxMs = ms + BURST_DURATION;
            nextBurstMs = ms + BURST_REFRESH;
            beepCount(0, 5, 1);
        }
        //check burst duration
        if (ms > burstMaxMs) {
            burst = false;
            burstMaxMs = 0;
            beepCount(2, 5, 1);
        } else {
            imax = I_MOT_BURST;
        }
    }
    ////////////////// END BURST ON DOUBLE TAP THROTLE
    
    //@todo DECREASE MAX AMPS ON LOW BAT
    
    //@todo LOCK DIFFERENTIAL (MAX POWER ON NON SLIPPING WHEEL)
    
    rtP_Left.i_max = rtP_Right.i_max = (imax * A2BIT_CONV) << 4;        // fixdt(1,16,4)
}
