#ifndef __JTAG_DRIVER_H__
#define __JTAG_DRIVER_H__

#include "jtag_fsm.h"
#include "errors.h"
#include "input_parser.h"
#include "pattern_writer.h"
#include "main.h"

vectors_t *execute_line(char *line, opts_t *opts, pattern_writer_t *writer, input_parser_t *parser, error_code_t *error_code);

#endif
