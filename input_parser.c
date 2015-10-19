#include <string.h>
#include "input_parser.h"
#include "svf_parser.h"

int init_parser( input_parser_t *parser, opts_t *opts) {
        //Decide the parser based on input file name.
        if ( strstr(opts->in, ".svf") ) {
                parser->fn_parser = parse_command;
        } else {
                return 1;
        }
        return 0;
} 

