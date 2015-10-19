#ifndef __ERRORS_H__
#define __ERRORS_H__

typedef enum {
	NO_ERROR,
	EMPTY_LINE,
	UNIMPLEMENTED_FEATURE,
	EMPTY_VECTORS,

} error_code_t;


typedef struct {
	error_code_t errorCode;
	char *msg;
} error_t;

extern error_t errors[]; 

#define errToString(code)	errors[code]


#endif
