#ifndef __JTAG_FSM_H__ 
#define __JTAG_FSM_H__

#include "linklist.h"
#include "signal_defs.h"

#define Jtag_GetVector	dequeue_linklist
#define JTAG_PINS_COUNT 5
#define get_jtag_state_string(state)	str_jtag_states[state]

typedef linklist_t  vectors_t ;
typedef char * vector_t;

extern const char *str_jtag_states[];

typedef enum {
	TCK,
	SCK,
	NONE
} run_clk_t;

typedef enum {
	XXX,

	RESET, 
	IDLE,

	DRSELECT,
	DRCAPTURE,
	DRSHIFT,
	DREXIT1,
	DRPAUSE,
	DREXIT2,
	DRUPDATE,

	IRSELECT,
	IRCAPTURE,
	IRSHIFT,
	IREXIT1,
	IRPAUSE,
	IREXIT2,
	IRUPDATE	
} jtag_state_t;

typedef struct {
	char data[JTAG_PINS_COUNT + 1];
	int  loopcnt;
	jtag_state_t curr_state;	
} vector_line_t;

typedef enum {
	ON,
	OFF
} repeat_mode_t;

typedef enum {
	NO_RESET,
	WITH_RESET
} idle_noreset_t;

typedef struct {
        vector_t tdi, tdo, mask, smask;
        int length;
} pad_pattern_t;


/*
Description: By default, FSM goes to IDLE by going to RESET, followed by TMS=0.
	     Use this function to avoid going to RESET, if possible.
*/
void SetIdleNoReset(idle_noreset_t idle_reset_mode);

/*
Description: Shift instruction to IR.
Parameters:
	IN - Null terminated string for instruction to be shifted to TDI.
	OUT- Null terminated string for insturction to be strobed from TDO while shifting in IN.
	END_STATE - where to end after Shifting.
*/
vectors_t *ireg(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK);

/*
Description: Shifts data to DR.
Parameters:
	IN - Null terminated string for data to be shifted to TDI.
	OUT- Null terminated string for data to be strobed from TDO while shifting in IN  
	END_STATE - where to end after Shifting
*/
vectors_t *dreg(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK);

/*
Description: Sets the order of the Jtag Pins as it appears in the vector.
	     by defaults its TRST=0, TCK=1, TMS=2, TDI=3, TDO=4	
Parameters:
	TRST_IDX, TCK_IDX, TMS_IDX, TDI_IDX, TDO_IDX
Returns:
	0 for sucess
	1 for error
*/
int Jtag_SetPin_Order(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx) ; 


/*
Description: Goto any state on the Jtag state machine.
Parameters:
	state where to to go.
*/

vectors_t *Jtag_GotoState(jtag_state_t state);

/*
Description: 
	Sets the state where jtag goes to after a shift-ir or shift-dr instruction
	by defaults, it goes to IDLE.
Parameters:
	sir_end_state - end state after a shift IR instruction
	sdr_end_state - end state after a shift DR instruction	
*/
void SetJtag_EndState(jtag_state_t sir_end_state, jtag_state_t sdr_end_state);
void SetJtag_EndIRState(jtag_state_t end_state);
void SetJtag_EndDRState(jtag_state_t end_state);



/*
Description:
	Sets the polarity of the reset signal, either POSITIVE (active high),
	or NEGATIVE (active low). By default, its POSITIVE.
*/
int SetJtag_ResetPolarity(signal_polarity_t polarity, int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx );


/*
Description:
	Sets if vectors should use repeat if successive vector lines are the same.
	by default, its set to ON.
*/
void SetJtag_RepeatMode(repeat_mode_t mode);

/*
Description:
	Sets max jtag frequency. Useful for calculating how many cycles to run during RUNTEST.
*/
void SetJtag_MaxFrequency(unsigned long jtagmaxFreq);

/*
Description:
	Sets the Header pattern. Headers are by default empty.
	All the elements on the Header should be set, ie. if tdi is be set, tdo has to be set, mask has to be set,etc....
Parameters:
	dr_header - pointer to the dr header pattern. This will be copied over to the static dr header pattern.
	ir_header - pointer to the ir header pattern. This will be copied over to the static ir header pattern.
	set to a null pointer if the header is to be empty.
*/
void SetJtag_HeaderPattern(pad_pattern_t *dr_header, pad_pattern_t *ir_header); 

/*
Description:
	Sets the Trailer pattern. Headers are by default empty.
	All the elements on the Trailer should be set, ie. if tdi is be set, tdo has to be set, mask has to be set,etc....
Parameters:
	dr_header - pointer to the dr header pattern. This will be copied over to the static dr header pattern.
	ir_header - pointer to the ir header pattern. This will be copied over to the static ir header pattern.
	set to a null pointer if the header is to be empty.
*/
void SetJtag_TrailerPattern(pad_pattern_t *dr_trailer, pad_pattern_t *ir_trailer); 
		

/*
Description:
	Get the current set of vectors.
*/

vectors_t *GetVectors(void) ;

vectors_t *runstate(int runcount, float min_time, float max_time, jtag_state_t runstate, jtag_state_t endstate, run_clk_t clk);
vectors_t *GotoStates(linklist_t *l );


#endif
