#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "jtag_driver.h"
#include "linklist.h"
#include "svf_parser.h"
#include "string_manip.h"
#include "signal_defs.h"

int  SetPinOrder ( opts_t *opts, pattern_writer_t *writer ) {
	if (opts->tck_idx != -1) {
		if ( Jtag_SetPin_Order(opts->trst_idx, opts->tck_idx, opts->tms_idx, opts->tdi_idx, opts->tdo_idx) ) {
			return 1;
		}	
		if ( writer->fn_setPinOrder(opts->trst_idx, opts->tck_idx, opts->tms_idx, opts->tdi_idx, opts->tdo_idx) ) {
			return 1;
		}
	}
	return 0;
}


vectors_t *execute_line(char *line, opts_t *opts, pattern_writer_t *writer, input_parser_t *parser,  error_code_t *error_code)  {
	char *orig_line = 0;
	void *data = 0;
	jtag_commands_t cmd = UNKNOWN;	
	command_data_t  *command=0;
	int return_a_vector = 0;
	int error = 0;
	static int firstVector = 0;

	*error_code = NO_ERROR;

	vectors_t *vectors = 0;
	
	if (line == 0) {
		//*error_code = EMPTY_LINE;
		return vectors;
	}

	orig_line = malloc(strlen(line) + 1);
	strcpy(orig_line, line);


	trim(&line);

	replace(&line, ")", " ");
	replace(&line, "(", " ");
	replace(&line, ";", " ");

	if (strlen(line) == 0 || line[0] == '#' ) {
		//*error_code = EMPTY_LINE;
		return vectors ;
	}
	//printf ("command: %s\n", orig_line);

	cmd = parser->fn_parser( line, orig_line, &data );
	switch (cmd) {
		case ENDIR:
			command = (command_data_t *) data;
			SetJtag_EndIRState( command->state_1 );
			break;
		case ENDDR:
			command = (command_data_t *) data;
			SetJtag_EndDRState( command->state_1 );
			break;
		case HDR:
		case HIR:
		case TDR:
		case TIR:
			command = (command_data_t *) data;
			pad_pattern_t pad = {
				command->tdi,
				command->tdo,
				command->mask,
				command->smask,
				(int) command->num
			} ;
			if (cmd == HDR) {
				SetJtag_HeaderPattern(&pad, 0);
			} else if (cmd == HIR) {
				SetJtag_HeaderPattern(0,&pad);
			} else if (cmd == TDR) {
				SetJtag_TrailerPattern(&pad, 0);
			} else if (cmd == TIR) {
				SetJtag_TrailerPattern(&pad, 0);
			}
			break;
		case SDR:
			if ( firstVector == 0 ) {
				if ( SetPinOrder (opts, writer) ) {
					error = 1;
				}
				firstVector = 1;
			}
			command = (command_data_t *) data;
			if ( (vectors = dreg( (int) command->num, command->tdi, command->tdo, command->smask, command->mask) ) == 0 ) {
				error = 1;
			}
			return_a_vector = 1;
			break;
		case SIR:
			if ( firstVector == 0 ) {
				if ( SetPinOrder (opts, writer) ) {
					error = 1;
				}
				firstVector = 1;
			}
			command = (command_data_t *) data;
			if ( (vectors = ireg( (int) command->num, command->tdi, command->tdo, command->smask, command->mask ) ) == 0 ) {
				error = 1;
			}
			return_a_vector = 1;
			break;
		case RUNTEST:
			if ( firstVector == 0 ) {
				if ( SetPinOrder (opts, writer) ) {
					error = 1;
				}
				firstVector = 1;
			}
			command = (command_data_t *) data;
			if ( (vectors = runstate( (int) command->num, command->t1, command->t2, command->state_1, command->state_2, (run_clk_t) command->d1)) == 0 ) {
				error = 1;
			}
			return_a_vector = 1;
			break;
		case FREQUENCY:
			command = (command_data_t *) data;
			SetJtag_MaxFrequency(command->num);
			break;
		case STATE: {
			if ( firstVector == 0 ) {
				if ( SetPinOrder (opts, writer) ) {
					error = 1;
				}
				firstVector = 1;
			}
			linklist_t *l = (linklist_t *) data;
			if ( (vectors =  GotoStates(l) ) == 0) {
				error = 1;
			}
			return_a_vector = 1;
			free(l);
			data = 0;
			break; 

		}
		case TRST:
			command = (command_data_t *) data;
			if ( firstVector == 0 ) {
				if ( SetJtag_ResetPolarity(command->d1, opts->trst_idx, opts->tck_idx, opts->tms_idx, opts->tdi_idx, opts->tdo_idx) ) {
					error = 1;
					break;		
				}	

				if ( (command->d1 ) == ABSENT ) {
					if ( writer->fn_setPinOrder( -1, opts->tck_idx, opts->tms_idx, opts->tdi_idx, opts->tdo_idx) ) {
						error = 1;
						break;
					}
				} else if (opts->tck_idx != -1) {
					if ( writer->fn_setPinOrder( opts->trst_idx, opts->tck_idx, opts->tms_idx, opts->tdi_idx, opts->tdo_idx) ) {
						error = 1;
						break;
					}
				}
				firstVector = 1;
			} else {
				error = 1;
			}
			break;
		default:
			error = 1;
			printf ("Unknown command %s\n", orig_line);
	}


	if (error) {
		*error_code = error;
	}

	if (return_a_vector) {
		if ( (vectors == 0) ) { 
			//Empty Vector, error should be set.
			*error_code = EMPTY_VECTORS;
		}
	}

	free(orig_line);
	if (data) {
		command_data_t *dat = (command_data_t *)data;
		if (dat->tdi) {
			free(dat->tdi);
		}
		if (dat->tdo) {
			free(dat->tdo);
		}
		if (dat->mask) {
			free(dat->mask);
		}
		if (dat->smask) {
			free(dat->smask);
		}
		free(dat);
	}

	return vectors;
}
