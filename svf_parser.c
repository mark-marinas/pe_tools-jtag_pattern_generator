#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "linklist.h"
#include "svf_parser.h"
#include "string_manip.h"
#include "signal_defs.h"

// Local variables.
#define match(str1, str2)	( !strcmp(str1, str2) )
#define isDigit(c)		( (c >= '0' && c<='9') )
#define within(num, low, high)	( (num >= low ) && (num <= high) )

command_t commands[] =  {
         { ENDIR,       "ENDIR",        1, 1}
        ,{ ENDDR,       "ENDDR",        1, 1}
        ,{ HDR,         "HDR",          1, 5}
        ,{ HIR,         "HIR",          1, 5}
        ,{ TDR,         "TDR",          1, 5}
        ,{ TIR,         "TIR",          1, 5}
        ,{ SDR,         "SDR",          1, 5}
        ,{ SIR,         "SIR",          1, 5}
        ,{ RUNTEST,     "RUNTEST",      2, 6} //TODO: Implement this.
        ,{ STATE,       "STATE",        1,99} 
        ,{ TRST,        "TRST",         1, 1}
        ,{ FREQUENCY,   "FREQUENCY",    0, 1}
        ,{ UNKNOWN,     "UKNOWN",       0, 0}
} ;

static jtag_commands_t cmd_parser_end_state(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_pad_pattern(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_shift(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_runtest(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_state(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_rst(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_frequency(jtag_commands_t command, char *str, void **retdata) ;
static jtag_commands_t cmd_parser_unknown(jtag_commands_t command, char *str, void **retdata) ;

static jtag_commands_t (*cmd_parser[])(jtag_commands_t command, char *str, void **retdata) = {
	 cmd_parser_end_state
	,cmd_parser_end_state
	,cmd_parser_pad_pattern
	,cmd_parser_pad_pattern
	,cmd_parser_pad_pattern
	,cmd_parser_pad_pattern
	,cmd_parser_shift
	,cmd_parser_shift
	,cmd_parser_runtest
	,cmd_parser_state
	,cmd_parser_rst
	,cmd_parser_frequency
	,cmd_parser_unknown	
} ;

//Function Prototypes
static states_t get_state(char *token);
static int get_commands(jtag_commands_t command, char *str, void **data);
static jtag_commands_t get_command(char *token);

//Codes

/*

	end_state is saved in state_1.
*/

static void freemem(void **data) {
	if ( *data) {
		free(*data);
	}
}



static jtag_commands_t cmd_parser_end_state(jtag_commands_t command, char *str, void **retdata) {
	int counter = 0;
	int args = 0;
	int error = 0;
	int min_args = commands[command].min_args;
	int max_args = commands[command].max_args;
	command_data_t *data = 0;
	states_t	end_state = UNKNOWN;

	char *token = strtok(0, " \t");	
	while (token) {
		trim(&token);
		if (counter == 0) {
			if ( (end_state = get_state(token) ) == XXX ) {
				error = 1;
				break;
			}
		}

		args++;
		counter++;
		token = strtok(0, " \t");	
	}

	if (error) {
		return UNKNOWN;
	}
	if ( within(args, min_args, args+1) == 0) {
		return UNKNOWN;
	}
	if ( ( data = malloc(sizeof(command_data_t)) )  == 0) {
		return UNKNOWN;
	}
	data->state_1 = end_state;
	*retdata = data;
	return command;
}

static int get_field(char *key, char *str, char **destination) {
	int error = 0;
	char *value;
	if ( match(key, str) ) {
		value = strtok(0," \t");
		if (value == 0) {
			error = 1;
		} else {
			trim(&value);
		}
	} else {
		error = 1;
	}

	if (error) {
		return 1;
	}

	if ( *destination = malloc( (strlen(value) + 1) ) ) {
		strcpy(*destination, value);
	} else {
		return 1;
	}

	return 0;
} 

static jtag_commands_t cmd_parser_pad_pattern(jtag_commands_t command, char *str, void **retdata) {
	int counter = 0;
	int args = 0;
	int error = 0;
	int min_args = commands[command].min_args;
	int max_args = commands[command].max_args;
	command_data_t *data = 0;

	int length = 0;
	char *tdi=0, *tdo=0, *smask=0, *mask=0;

	char *token = strtok(0, " \t");
	while (token) {
		trim(&token);
		if ( !get_field("TDI", token, &tdi) ) {
			args++;
		} else if ( !get_field("TDO", token, &tdo) ) {
			args++;
		} else if ( !get_field("MASK", token, &mask) ) { 
			args++;
		} else if ( !get_field("SMASK", token, &smask) ) {
			args++;
		} else if ( match("0", token) ) {
			args++;
		} else if ( ( length = atoi(token) ) > 0 ){
			args++;
		} else {
			error = 1;
			break;
		}
		token = strtok(0, " \t");
	}	

	if ( ( within(args, min_args, max_args) ==  0) || error ){
		freemem((void **) &tdi);
		freemem((void **) &tdo);
		freemem((void **) &mask);
		freemem((void **) &smask);
		return UNKNOWN;
	}

	if (length > 0) {
		int total_length;
		total_length = 	( tdi )   ? strlen(tdi) : 0;
		total_length += ( tdo )   ? strlen(tdo) : 0;
		total_length += ( mask )  ? strlen(mask) : 0;
		total_length += ( smask ) ? strlen(smask) : 0;
		
		if (total_length != length*4) {
			freemem((void **) &tdi);
			freemem((void **) &tdo);
			freemem((void **) &mask);
			freemem((void **) &smask);
			return UNKNOWN;
		}
	}

	data = malloc(sizeof(command_data_t));
	if (data == 0) {
		freemem((void **) &tdi);
		freemem((void **) &tdo);
		freemem((void **) &mask);
		freemem((void **) &smask);
		return UNKNOWN;
	}
	data->num = (long) length;
	data->tdi = tdi;
	data->tdo = tdo;
 	data->mask= mask;
	data->smask=smask;
	*retdata = data;

	return command; 	
}

static jtag_commands_t cmd_parser_shift(jtag_commands_t command, char *str, void **retdata) {
	return ( cmd_parser_pad_pattern(command, str, retdata) ) ;
}


/*
        data->num     = run_count;
        data->d1      = run_test_mode ;
        data->state_1 = run_state;
        data->state_2 = end_state;
        data->t1      = min_time;
        data->t2      = max_time;
*/

static jtag_commands_t cmd_parser_runtest(jtag_commands_t command, char *str, void **retdata) {
	int counter = 0;
	int args = 0;
	int error = 0;
	int min_args = commands[command].min_args;
	int max_args = commands[command].max_args;
	command_data_t *data = 0;

	states_t run_state=XXX, end_state=XXX;
	float	min_time = 0;
	float 	max_time = 0;
	long	run_count = 0;

	char *token = strtok(0," \t");
	run_clk_t run_test_mode = NONE  ;

	if (strstr(str,"TCK")) {
		run_test_mode = TCK;	
	} else if (strstr(str,"SCK")) {
		run_test_mode = SCK;
	} 

	while (token) {
		trim(&token);

		//run_state is only allowed as first argument
		if ( (counter == 0) && ((run_state = get_state(token)) != XXX) ) {
			args++;
			counter++;
		} else {
			//this could be either the run_count or min_time.
			if (counter <= 1) {
				if (run_test_mode == NONE) {
					if ( (min_time = atof(token)) > 0 ) {
						args++;
						counter++;
					} else {
						error = 1;
						break;
					}
			
					token = strtok(0, " \t");
					if (token == 0) {
						error = 1;
						break;
					}
					trim(&token);
					if (match(token,"SEC")) {
						counter++;
					} else {
						error = 1;
						break;
					}	
				} else {
					if ( (run_count = atoi(token)) > 0 ) {
						args++;
						counter++;
					} else {
						error = 1;
						break;
					}
					token = strtok(0," \t");
					if (token == 0) {
						error = 1;
						break;
					}
					trim(&token);

					if (match(token, "SCK") || match(token, "TCK") ) {
						args++;
						counter++;
					} else {
						error = 1;
						break;
					}
				}
			} else {
				char *max=0, *min=0, *end=0;
				if ( !get_field("MAXIMUM", token, &max) ) {
					max_time = atof(max);
					free(max);
					if ( max_time == 0) {
						error = 1;
						break;	
					}
					args++;
					token = strtok(0, " \t");
					if (token == 0 || match(token, "SEC") == 0) {
						error = 1;
						break;
					}
				} else if ( !get_field("ENDSTATE", token, &end) ) {
					if ( (end_state = get_state(end)) != XXX) {
						args++;
					} else {
						error = 1;
						break;
					}
				} else  {
					if ( run_test_mode == NONE) {
						error = 1;
						break;	
					} else {
						args++;
						min_time = atof(token);
						if (min_time == 0) {
							error = 1;
							break;
						}	
						token = strtok(0, " \t");
						if (token == 0 || match(token, "SEC") == 0 ) {
							error = 1;
							break;
						}
					}
				}
			}	
		}
		token = strtok(0, " \t");
	}

	if (error) {
		return UNKNOWN;
	}

	if (within(args, min_args, max_args) == 0) {
		return UNKNOWN;
	}

	data = malloc(sizeof(command_data_t));		
	if (data == 0) {
		return UNKNOWN;
	}

        data->num     = run_count;
        data->d1      = run_test_mode ;
        data->state_1 = run_state;
        data->state_2 = end_state;
        data->t1      = min_time;
        data->t2      = max_time;

	*retdata = data;

	return command;
}

static jtag_commands_t cmd_parser_state(jtag_commands_t command, char *str, void **retdata) {
        int args = 0;
        int error = 0;
        int min_args = commands[command].min_args;
        int max_args = commands[command].max_args;

	linklist_t *l = malloc(sizeof(linklist_t));	
	init_linklist(l);	

	char *token = strtok(0, " \t");
	states_t state;
	states_t *s = 0;
	while (token) {
		trim(&token);
		if ( ( state = get_state(token) ) != XXX ) {
			s = malloc(sizeof(states_t));
			*s = state;
			enqueue_linklist(l, (void *) s);
			args++;
		} else {
			error = 1;
			break;
		}		

		token = strtok(0, " \t");
	}
	if (error || (within(args, min_args, max_args) == 0) ) {
		while (dequeue_linklist(l));
		free(l);
		return UNKNOWN;
	}
	*retdata = l;
	return command;
}

static jtag_commands_t cmd_parser_rst(jtag_commands_t command, char *str, void **retdata) {
        int min_args = commands[command].min_args;
        int max_args = commands[command].max_args;
	int args = 0;
	int error = 0;
	command_data_t *data;

	signal_polarity_t reset_polarity;

	char *token = strtok(0, " \t");
	while (token) {
		trim(&token);
		if (match(token, "ON")) {
			reset_polarity = NEGATIVE;
		} else if ( match(token, "OFF") ) {
			reset_polarity = POSITIVE;
		} else if ( match(token, "Z") ) {
			reset_polarity = HIGHZ;
		} else if ( match(token, "ABSENT") ) {
			reset_polarity = ABSENT;
		} else {
			error = 1;
			break;
		}
		token = strtok(0, " \t");
		args++;
	}

	if (error) {
		return UNKNOWN;
	}

	if (within(args, min_args, max_args) == 0) {
		return UNKNOWN;
	}

	data = malloc(sizeof(command_data_t));
	data->d1 = reset_polarity;
	*retdata = data;

	return command;	
}

static jtag_commands_t cmd_parser_frequency(jtag_commands_t command, char *str, void **retdata) {
        int counter = 0;
        int args = 0;
        int error = 0;
        int min_args = commands[command].min_args;
        int max_args = commands[command].max_args;
        command_data_t *data = 0;
	long freq = 0;
		
	char *token = strtok(0, " \t");
	while (token) {
		trim(&token);
		if ( (freq = atol(token) ) > 0 ) {
			args++;
			token = strtok(0, " \t");
			if ( token == 0 || (match(token,"Hz") == 0 ) ) {
				error = 1;
				break;	
			}
		} else {
			error = 1;
			break;
		}
		token = strtok(0, " \t");
	}
	if (error == 1) {
		return UNKNOWN;
	}
	if (within(args, min_args, max_args) == 0) {
		return UNKNOWN;
	}

	data = malloc(sizeof(command_data_t));
	data->num = freq;
	*retdata = data;

	return command;
}

static jtag_commands_t cmd_parser_unknown(jtag_commands_t command, char *str, void **retdata) {

}

static states_t get_state(char *token) {
	int i = 0;
	while (str_jtag_states[i]) {
		if (match(str_jtag_states[i], token)) {
			return (states_t) i;
		}
		i++;
	}
	return (states_t) XXX;
}

static int get_commands(jtag_commands_t command, char *str, void **data) {
	if ( cmd_parser[command](command, str, data) == UNKNOWN ) {
		return 1;	
	}	
	return 0;
}

static jtag_commands_t get_command(char *token) {
	int i;
	for (i=0; i<sizeof(commands)/sizeof(command_t); i++) {
		if (match ("UNKNOWN", commands[i].command_string)) {
			break;
		} else if ( match(token, commands[i].command_string ) ) {
			break;
		}
	}
	return (jtag_commands_t) i;
}

jtag_commands_t parse_command(char *str, char *orig_str, void **data) {
	jtag_commands_t command = UNKNOWN;
	char *token = 0;

	token = strtok(str, " \t");
	command = get_command(token);

	if ( get_commands(command, orig_str, data) ) {
		return UNKNOWN;
	} else {
		return command;
	}
}

//static int get_commands(jtag_commands_t command, void *data, int min_args, int max_args) {

#ifdef __DEBUG_SVFPARSER__

int main() {
	FILE *svf_file = fopen("jtag.svf", "r");
	if (svf_file == 0) {
		printf ("Error: Failed to open file\n");
		return 1;
	}
	char *line = 0;
	char *orig_line = 0;
	ssize_t i = 0;
	size_t j= 0;
	void *data=0;

	jtag_commands_t cmd;	
	command_data_t  *command;
	while ( getline(&line, &i, svf_file) > 0) {
		orig_line = malloc(strlen(line) + 1);
		strcpy(orig_line, line);
		trim(&line);

		replace(&line, ")", " ");
		replace(&line, "(", " ");
		replace(&line, ";", " ");

		if (strlen(line) == 0 || line[0] == '#' ) {
			continue;
		}
		cmd = parse_command( line, orig_line, &data );
		switch (cmd) {
			case ENDIR:
				command = (command_data_t *) data;
				printf ("o ENDIR %s\n", str_jtag_states[command->state_1] );	
				break;
			case ENDDR:
				command = (command_data_t *) data;
				printf ("o ENDDR %s\n", str_jtag_states[command->state_1] );	
				break;
			case HDR:
				command = (command_data_t *) data;
				printf ("o HDR\n");
				printf ("\tlength=%d\n",(int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case HIR:
				command = (command_data_t *) data;
				printf ("o HIR\n");
				printf ("\tlength=%d\n", (int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case TDR:
				command = (command_data_t *) data;
				printf ("o TDR\n");
				printf ("\tlength=%d\n",(int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case TIR:
				command = (command_data_t *) data;
				printf ("o TIR\n");
				printf ("\tlength=%d\n",(int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case SDR:
				command = (command_data_t *) data;
				printf ("o SDR\n");
				printf ("\tlength=%d\n", (int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case SIR:
				command = (command_data_t *) data;
				printf ("o SIR\n");
				printf ("\tlength=%d\n", (int) command->num);
				printf ("\ttdi=%s\n",   (command->tdi == 0)?"null":command->tdi );
				printf ("\ttdo=%s\n",   (command->tdo == 0)?"null":command->tdo );
				printf ("\tmask=%s\n",  (command->mask == 0)?"null":command->mask );
				printf ("\tsmask=%s\n", (command->smask == 0)?"null":command->smask );
				break;
			case RUNTEST:
				command = (command_data_t *) data;
				int run_count = (int) command->num, d1=command->d1;
				char *run_state = str_jtag_states[command->state_1], *end_state = str_jtag_states[command->state_2] ;
				float min_time = command->t1, max_time = command->t2;

				printf ("o RUNTEST: \n");
				printf ("\trun_count=%d\n", run_count);
				printf ("\trun_test_mode=%d\n", d1);;
				printf ("\trun_state=%s\n", run_state);
				printf ("\tend_state=%s\n", end_state);
				printf ("\tmin_time=%f\n", min_time );
				printf ("\tmax_time=%f\n", max_time );
				break;
			case FREQUENCY:
				printf ("");
				float *command = (float *) data;
				printf ("o FREQUENCY: %f\n", *command);
				break; 
			case STATE:
				printf ("o STATES: ");
				linklist_t *l = (linklist_t *) data;
				states_t *s = (states_t *) dequeue_linklist(l);
				while ( s ) {
					printf ("%p\n", s);
					printf ("%s ", str_jtag_states[*s]);	
					s = (states_t *) dequeue_linklist(l);
				}	
				printf ("\n");
				break;	
			case TRST:
				printf ("o TRST: " );
				signal_polarity_t *pol = (signal_polarity_t *) data;
				printf ("%d\n", *pol);
				break;
			default:
				printf ("Unknown command %s\n", orig_line);
		}
		line = 0;
	} 
	//TODO:
	//freeup void *data recursively.

	fclose(svf_file);
	return 0;
}

#endif
