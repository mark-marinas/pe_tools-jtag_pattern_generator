#ifndef __STIL_H__
#define __STIL_H__

#include "jtag_fsm.h"

/*
Description:
	Set the output pattern filename
*/
int Stil_SetOutfilename(char *filename); 

/*
Description:
	Close the output file
*/
int Stil_CloseOutfile(void);

/* 
Description:
	Set the order of the Jtag pins as they appear in the output file.
	This pin order will be set at the same time as the pin order in the jtag_fsm.c,
	so this actually just matters on the signal groups of the stil file.
Parameters:
	index of the pin. trst_idx should be equal to -1 if there is no TRST
*/ 
int SetPin_Order(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx) ;


/*
Description:
	Write a vectors to the stil file.
Parameters:
	vector_line_t *
	debug mode
		0 - no debug. just write the vector.
		1 - write the state.
*/
int writeVector(vector_line_t * v, int dbg_mode);

/*

*/
int writeComment(char *comment);


#endif

