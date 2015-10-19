#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "signal_defs.h"

char *signal_direction_string[] = {
	"IN",
	"INOUT",
	"OUT",
	"HIGHZ"
} ;

#define NO_ERROR	0
#define IDX_GT_MAX	1
#define IDX_DUPLICATE	2

int arrange_pins(pin_t *dst, pin_t *src, int size) {
	int i;
	memset(dst, 0, sizeof(pin_t)*size );
	for (i = 0; i<size; i++) {
		if ( src[i].idx >= size ) {
			return IDX_GT_MAX;
		} else if ( dst[src[i].idx].name != 0 ) {
			return IDX_DUPLICATE;
		} else {
			dst[src[i].idx] = src[i] ;
		}		
	}
	return NO_ERROR;
}
