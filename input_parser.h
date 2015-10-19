#ifndef __INPUT_PARSER_H__
#define __INPUT_PARSER_H__

#include "main.h"

typedef struct {
	int (*fn_parser)(char *str, char *orig_str, void **data) ; 	
} input_parser_t ;

int init_parser( input_parser_t *parser, opts_t *opts);

#endif
