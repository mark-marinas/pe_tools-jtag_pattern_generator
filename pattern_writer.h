#ifndef __PATTERN_WRITER_H__
#define __PATTERN_WRITER_H__

#include "jtag_fsm.h"
#include "main.h"

typedef struct {
	int (*fn_writeComment)(char *comment);
	int (*fn_writeVector)(vector_line_t * v, int dbg_mode) ;
	int (*fn_setPinOrder)(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx);	
	int (*fn_setOutFile)(char *filename);
	int (*fn_closeOutFile)(void);
} pattern_writer_t;


int init_writer(pattern_writer_t *writer, opts_t *opts) ;


#endif
