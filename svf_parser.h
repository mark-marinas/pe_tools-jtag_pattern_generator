#ifndef __SVF_PARSER_H__
#define __SVF_PARSER_H__

#include "jtag_fsm.h"

typedef jtag_state_t	states_t;

typedef enum {
        ENDIR, 
        ENDDR,
        HDR,
        HIR,
        TDR,	
        TIR,
        SDR,	
        SIR,
        RUNTEST, 
        STATE, 
        TRST,
        FREQUENCY,
        UNKNOWN
} jtag_commands_t;

typedef struct {
        long num;
        char *tdi, *tdo, *mask, *smask;
        states_t state_1;
        states_t state_2;
	float t1, t2;
	int d1;
} command_data_t ;

typedef struct {
        jtag_commands_t command;
        char            *command_string;
        int             min_args, max_args;
} command_t ;


jtag_commands_t parse_command(char *str, char *orig_str, void **data) ;



#endif
