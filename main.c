#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "main.h"
#include "errors.h"
#include "jtag_driver.h"



#define match(str1, str2)       ( strcmp(str1, str2) == 0 )

int parse_cmdline( int argc, char *argv[], opts_t *opts) {
        int i;
        int error = 0;
        for (i = 1; i<argc;i++) {
                if ( match(argv[i], "-if") ) {
                        i++;
                        opts->in = argv[i];
                } else if ( match(argv[i], "-of") ) {
                        i++;
                        opts->out = argv[i];
                } else if ( match(argv[i], "-hw") ) {
                        i++;
                        opts->hw_driver = atoi(argv[i]);
                } else if ( match(argv[i], "-debug") ) {
                        opts->debug_mode = 1;
                } else if ( match(argv[i], "-trst") ) {
                        i++;
                        opts->trst_idx = atoi(argv[i]);
                } else if ( match(argv[i], "-tms") ) {
                        i++;
                        opts->tms_idx = atoi(argv[i]);
                } else if ( match(argv[i], "-tdi") ) {
                        i++;
                        opts->tdi_idx = atoi(argv[i]);
                } else if ( match(argv[i], "-tdo") ) {
                        i++;
                        opts->tdo_idx = atoi(argv[i]);
                } else if ( match(argv[i], "-tck") ) {
                        i++;
                        opts->tck_idx = atoi(argv[i]);
                } else if ( match(argv[i], "-repeat" ) ) {
                        i++;
                        if ( match(argv[i], "ON") ) {
                                opts->repeat = ON;
                        } else if ( match(argv[i], "OFF" ) ) {
                                opts->repeat = OFF;
                        } else {
                                error = 1;
                                break;
                        }
                } else {
			printf ("o Error: Unknown option %s\n", argv[i]);
                        error = 1;
                        break;
                }
        }

	if ( opts->in == 0 || opts->out == 0) {
		printf ("o Error: Input/Output file not specified\n");
		error = 1;
	}
	// TODO: Check if jtagpins are assigned correctly, ie.if 1 of the pins is assigned, then all should be assigned, etc.
	

        if (error) {
                return 1;
        }
	return 0;
}

int main(int argc, char *argv[]) {
	FILE *fp; 

	opts_t opts;
	opts.in = opts.out = 0;
	opts.hw_driver = NO_HW;
	opts.repeat = OFF;
	opts.debug_mode = 0;
	opts.trst_idx = opts.tms_idx = opts.tdi_idx = opts.tdo_idx = opts.tck_idx = -1;

	pattern_writer_t writer;
	input_parser_t   parser;

	if ( parse_cmdline(argc, argv, &opts) ) {
		return 1;
	}

	init_writer(&writer, &opts); 
	init_parser(&parser, &opts);

	fp = fopen(opts.in, "r");

	if (fp == 0) {
		printf ("Error: Failed to input file %s\n", opts.in );
		return 1;
	}

	if ( writer.fn_setOutFile(opts.out) ) {
		printf ("Failed to open %s for writing\n", opts.out);
		return 1;
	}

	SetJtag_RepeatMode(opts.repeat);

	char *line = 0;
	ssize_t	n;
	size_t len;

	error_code_t err;
	int cycles = 0, vector_count = 0;

	char *orig_line;
	while ( getline(&line, &len, fp) > 0 ) {
		trim(&line);
		orig_line = malloc(strlen(line) + 1);
		strcpy(orig_line, line) ;
		vectors_t *vs = execute_line(line, &opts,  &writer, &parser, &err) ;
		if ( err == NO_ERROR && vs != 0) {
			vector_line_t *v = Jtag_GetVector(vs);
			writer.fn_writeComment(orig_line);
			while (v) {
				cycles += v->loopcnt;
				vector_count++;
				writer.fn_writeVector(v, opts.debug_mode);
				v = Jtag_GetVector(vs);
			}
		} else if ( err != NO_ERROR) {
			err = 1;
			printf ("Error executing %s\n", orig_line);
			break;
		}
		free(orig_line);
		line = 0;
	} 	
	if (writer.fn_closeOutFile()) {
		err = 1;
	}
	if (err != NO_ERROR) {
		//TODO: Delete the file
	}
	fclose(fp);
	printf ("Finished writing pattern %s\n", opts.out );
	printf ("\tTotal Vectors: %d\n", vector_count);
	printf ("\tTotal Cycles: %d\n", cycles);

	return err;
}
