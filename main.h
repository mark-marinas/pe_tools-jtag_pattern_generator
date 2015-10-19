#ifndef __MAIN_H__
#define __MAIN_H__

#include "hw_driver.h"
#include "jtag_fsm.h"

typedef struct {
	char *in, *out;
	hw_driver_t hw_driver;
	repeat_mode_t repeat;
	int debug_mode; 	
	int trst_idx, tms_idx, tdi_idx, tdo_idx, tck_idx;
} opts_t;





#endif
