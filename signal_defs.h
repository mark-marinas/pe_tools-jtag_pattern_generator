#ifndef __SIGNAL_DEFSH__
#define __SIGNAL_DEFSH__

#define toString_direction(direction) signal_direction_string[direction]

typedef enum {
        POSITIVE,
        NEGATIVE,
	HIGHZ,
	ABSENT
} signal_polarity_t;


typedef enum {
	IN,
	INOUT,
	OUT,
	Z
} signal_direction_t ;

typedef struct {
	char *name;
	signal_direction_t direction;
	int idx;
} pin_t ;

extern char *signal_direction_string[] ;



int arrange_pins(pin_t *dst, pin_t *src, int size);

#endif
