#include "jtag_fsm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// TODO:
// Not yet tested features.
//	1. Idle with no reset.
//	2. with trailer, header pattern.
// Need to add run test	



// ------- DEFINES

#define TRST_IDX	0
#define TCK_IDX		(TRST_IDX + 1)
#define TMS_IDX		(TCK_IDX + 1)
#define TDI_IDX		(TMS_IDX + 1)
#define TDO_IDX		(TDI_IDX + 1) 

#define END_DR_IDX	0
#define END_IR_IDX	(END_DR_IDX + 1)

// ------ STATIC VARIABLES & STRUCTS

static jtag_state_t END_STATES[] = {
	IDLE,
	IDLE
};

static int JtagPinOrder[] = {
        TRST_IDX,
        TCK_IDX,
        TMS_IDX,
        TDI_IDX,
        TDO_IDX
};

static jtag_state_t jtag_current_state = XXX;
static vectors_t JtagVectors = {0, 0};
static signal_polarity_t reset_polarity = NEGATIVE; //POSITIVE;
static repeat_mode_t  vector_repeat = OFF;
static idle_noreset_t idle_reset = WITH_RESET; 

#define Jtag_MaxFrequency_default  100000
static unsigned long Jtag_MaxFrequency = Jtag_MaxFrequency_default;

static pad_pattern_t HIR = { 0, 0, 0, 0, 0 };
static pad_pattern_t HDR = { 0, 0, 0, 0, 0 };
static pad_pattern_t TIR = { 0, 0, 0, 0, 0 };
static pad_pattern_t TDR = { 0, 0, 0, 0, 0 };
static pad_pattern_t LAST_SIR = { 0, 0, 0, 0, 0 };
static pad_pattern_t LAST_SDR = { 0, 0, 0, 0, 0 };
static int error = 0;
static int run_test=0;
static jtag_state_t run_state=IDLE;
static jtag_state_t end_state=IDLE;

const char *str_jtag_states[] = {
        "XXX",

        "RESET",
        "IDLE",

        "DRSELECT",
        "DRCAPTURE",
        "DRSHIFT",
        "DREXIT1",
        "DRPAUSE",
        "DREXIT2",
        "DRUPDATE",

        "IRSELECT",
        "IRCAPTURE",
        "IRSHIFT",
        "IREXIT1",
        "IRPAUSE",
        "IREXIT2",
        "IRUPDATE",
	0
} ;


// ----------- FUNCTION PROTOTYPES

static void 		ClearJtagVectors(void);
static vectors_t * 	_Reset(void);
static vectors_t * 	_Idle(void);
static vectors_t * 	Reset(void);
static vectors_t * 	Idle(void);
static void 		UpdateVectors(vector_t data) ;
static jtag_state_t 	Jtag_GetNextState(jtag_state_t current_state, int TMS) ;
static vectors_t *	_Jtag_GotoState(jtag_state_t state) ; 
static vectors_t *	reg(jtag_state_t reg, int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK);
static vectors_t *	shiftIn(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK, jtag_state_t end_state);
int 			Jtag_SetPin_Order(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx);


// --------- CODES
void SetJtag_HeaderPattern(pad_pattern_t *dr_header, pad_pattern_t *ir_header) {
	if (dr_header != 0 && dr_header->length > 0) {
		if (HDR.tdi != 0) {
			free(HDR.tdi);
			free(HDR.tdo);
			free(HDR.mask);
			free(HDR.smask);
		}
		HDR.tdi  	= malloc(dr_header->length + 1);
		HDR.tdo  	= malloc(dr_header->length + 1);
		HDR.mask 	= malloc(dr_header->length + 1);
		HDR.smask	= malloc(dr_header->length + 1);
		HDR.length	= dr_header->length;
		
		strcpy(HDR.tdi,  dr_header->tdi);
		strcpy(HDR.tdo,  dr_header->tdo);
		strcpy(HDR.mask, dr_header->mask);
		strcpy(HDR.smask,dr_header->smask);
	}
	if (ir_header != 0 && ir_header->length > 0) {
		if (HIR.tdi != 0) {
			free(HIR.tdi);
			free(HIR.tdo);
			free(HIR.mask);
			free(HIR.smask);
		}
		HIR.tdi  	= malloc(ir_header->length + 1);
		HIR.tdo  	= malloc(ir_header->length + 1);
		HIR.mask 	= malloc(ir_header->length + 1);
		HIR.smask	= malloc(ir_header->length + 1);
		HIR.length	= ir_header->length;
		
		strcpy(HIR.tdi,  ir_header->tdi);
		strcpy(HIR.tdo,  ir_header->tdo);
		strcpy(HIR.mask, ir_header->mask);
		strcpy(HIR.smask,ir_header->smask);
	}
}

void SetJtag_TrailerPattern(pad_pattern_t *dr_trailer, pad_pattern_t *ir_trailer) {
	if (dr_trailer != 0 && dr_trailer->length > 0) {
		if (TDR.tdi != 0) {
			free(TDR.tdi);
			free(TDR.tdo);
			free(TDR.mask);
			free(TDR.smask);
		}
		TDR.tdi  	= malloc(dr_trailer->length + 1);
		TDR.tdo  	= malloc(dr_trailer->length + 1);
		TDR.mask 	= malloc(dr_trailer->length + 1);
		TDR.smask	= malloc(dr_trailer->length + 1);
		TDR.length	= dr_trailer->length;
		
		strcpy(TDR.tdi,  dr_trailer->tdi);
		strcpy(TDR.tdo,  dr_trailer->tdo);
		strcpy(TDR.mask, dr_trailer->mask);
		strcpy(TDR.smask,dr_trailer->smask);
	}
	if (ir_trailer != 0 && ir_trailer->length > 0) {
		if (TIR.tdi != 0) {
			free(TIR.tdi);
			free(TIR.tdo);
			free(TIR.mask);
			free(TIR.smask);
		}
		TIR.tdi  	= malloc(ir_trailer->length + 1);
		TIR.tdo  	= malloc(ir_trailer->length + 1);
		TIR.mask 	= malloc(ir_trailer->length + 1);
		TIR.smask	= malloc(ir_trailer->length + 1);
		TIR.length	= ir_trailer->length;
		
		strcpy(TIR.tdi,  ir_trailer->tdi);
		strcpy(TIR.tdo,  ir_trailer->tdo);
		strcpy(TIR.mask, ir_trailer->mask);
		strcpy(TIR.smask,ir_trailer->smask);
	}
}




void SetJtag_MaxFrequency(unsigned long jtagmaxFreq) {
	if ( jtagmaxFreq == 0 ) {
		Jtag_MaxFrequency = Jtag_MaxFrequency_default;
	}
	Jtag_MaxFrequency = jtagmaxFreq;
}

void SetJtag_EndDRState(jtag_state_t end_state) {
	END_STATES[END_DR_IDX] = end_state ;
}

void SetJtag_EndIRState(jtag_state_t end_state) {
	END_STATES[END_IR_IDX] = end_state ;
}

void SetJtag_EndState(jtag_state_t sir_end_state, jtag_state_t sdr_end_state) {
	END_STATES[END_DR_IDX] = sdr_end_state;
	END_STATES[END_IR_IDX] = sir_end_state;
} 

int SetJtag_ResetPolarity(signal_polarity_t polarity, int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx ) {

	reset_polarity = polarity;

	if (reset_polarity == ABSENT) {
		if ( tck_idx == -1 ) {
			return ( Jtag_SetPin_Order(-1, TCK_IDX-1, TMS_IDX-1, TDI_IDX-1, TDO_IDX-1) ) ; 
		} else {
			return ( Jtag_SetPin_Order(-1, tck_idx, tms_idx, tdi_idx, tdo_idx) ) ; 
		}
	} else {
		if ( tck_idx == -1 ) {
			return ( Jtag_SetPin_Order(TRST_IDX, TCK_IDX, TMS_IDX, TDI_IDX, TDO_IDX) ) ; 
		} else {
			return ( Jtag_SetPin_Order(trst_idx, tck_idx, tms_idx, tdi_idx, tdo_idx) ) ; 
		}
	}
}

void SetIdleNoReset(idle_noreset_t idle_reset_mode) {
	idle_reset = idle_reset_mode;
}

void SetJtag_RepeatMode(repeat_mode_t mode) {
	vector_repeat = mode;	
}

static jtag_state_t Jtag_GetNextState(jtag_state_t current_state, int TMS) {
	switch (TMS) {
		case 0:
			switch (current_state) {
				case RESET:
					return IDLE;
				case IDLE:
					return  IDLE;
				case DRSELECT:
					return DRCAPTURE;
				case DRCAPTURE:
					return DRSHIFT;
				case DRSHIFT:
					return DRSHIFT;
				case DREXIT1:
					return DRPAUSE;
				case DRPAUSE:
					return DRPAUSE;
				case DREXIT2:
					return DRSHIFT;
				case DRUPDATE:
					return IDLE;

				case IRSELECT:
					return IRCAPTURE;
				case IRCAPTURE:
					return IRSHIFT;
				case IRSHIFT:
					return IRSHIFT;
				case IREXIT1:
					return IRPAUSE;
				case IRPAUSE:
					return IRPAUSE;
				case IREXIT2:
					return IRSHIFT;
				case IRUPDATE:
					return IDLE;
			}
			break;
		case 1:
			switch (current_state) {
				case RESET:
					return RESET;
				case IDLE:
					return DRSELECT;

				case DRSELECT:
					return IRSELECT;
				case DRCAPTURE:
					return DREXIT1;
				case DRSHIFT:
					return DREXIT1;
				case DREXIT1:
					return DRUPDATE;
				case DRPAUSE:
					return DREXIT2;
				case DREXIT2:
					return DRUPDATE;
				case DRUPDATE:
					return DRSELECT;

				case IRSELECT:
					return RESET;
				case IRCAPTURE:
					return IREXIT1;
				case IRSHIFT:
					return IREXIT1;
				case IREXIT1:
					return IRUPDATE;
				case IRPAUSE:
					return IREXIT2;
				case IREXIT2:
					return IRUPDATE;
				case IRUPDATE:
					return DRSELECT;

			}
			break;
		default:
			break;
	}
	return XXX;
}



static void UpdateVectors(vector_t data) {
	vector_line_t *vecline;
	vector_line_t *vecline_last;
	if (vector_repeat == OFF) {
		vecline = malloc ( sizeof(vector_line_t)  );
		vecline->loopcnt = 1;
		vecline->curr_state = jtag_current_state ;
		memcpy(vecline->data, data, strlen(data) + 1 );	
		enqueue_linklist(&JtagVectors, vecline);
	} else {
		vecline_last = gettail_linklist(&JtagVectors);
		if (	vecline_last != 0 &&
	  	    	strcmp(vecline_last->data, data) == 0
		   ) {
			vecline_last->loopcnt++;
			vecline_last->curr_state = jtag_current_state ;
		} else {
			vecline = malloc ( sizeof(vector_line_t)  );
			vecline->loopcnt = 1;
			vecline->curr_state = jtag_current_state ;
			memcpy(vecline->data, data, strlen(data) + 1 );	
			enqueue_linklist(&JtagVectors, vecline);
		}		
	}
	//TODO: Use this for debug 
	//printf ("%s\n" , str_jtag_states[jtag_current_state]);
}

static void ClearJtagVectors(void) {
	void *dummy = dequeue_linklist(&JtagVectors);
	while (dummy) {
		dummy = dequeue_linklist(&JtagVectors);
	}
}

static vectors_t * _Reset(void) {
	int vector_number;

	char vector[JTAG_PINS_COUNT + 1];
	vector[JtagPinOrder[TRST_IDX]] = (reset_polarity == POSITIVE) ? '0':'1' ;
	vector[JtagPinOrder[TCK_IDX]] = '1';
	vector[JtagPinOrder[TMS_IDX]] = '1';
	vector[JtagPinOrder[TDI_IDX]] = '0';
	vector[JtagPinOrder[TDO_IDX]] = 'X';
	vector[JTAG_PINS_COUNT] = '\0';


	for (vector_number = 0; vector_number < 5; vector_number++) {
		jtag_current_state = Jtag_GetNextState (jtag_current_state, 1);	
		UpdateVectors(vector);
		if ( jtag_current_state == RESET) {
			break;
		}
	}
	jtag_current_state = RESET;
	return &JtagVectors;
}

static vectors_t *Reset(void) {
	ClearJtagVectors();
	return ( _Reset() );
}

static vectors_t * _Idle(void) {
        char vector[JTAG_PINS_COUNT + 1];
        vector[JtagPinOrder[TRST_IDX]] = (reset_polarity == POSITIVE) ? '0':'1' ;
        vector[JtagPinOrder[TCK_IDX]] = '1';
        vector[JtagPinOrder[TMS_IDX]] = '0';
        vector[JtagPinOrder[TDI_IDX]] = '0';
        vector[JtagPinOrder[TDO_IDX]] = 'X';
        vector[JTAG_PINS_COUNT] = '\0';

	if (jtag_current_state == IDLE) {
		return &JtagVectors;	
	}

	if ( idle_reset == WITH_RESET || jtag_current_state == XXX ) {
		//Reset
		_Jtag_GotoState(RESET);
		//Followed by TMS=0
		jtag_current_state = Jtag_GetNextState(jtag_current_state, 0);
		UpdateVectors(vector);
	}  else {
		if (jtag_current_state >= DRSELECT && jtag_current_state <= DRUPDATE) {
			_Jtag_GotoState(DRUPDATE);
			jtag_current_state = Jtag_GetNextState(jtag_current_state, 0);
			UpdateVectors(vector);
		} else if (jtag_current_state >= IRSELECT && jtag_current_state <= IRUPDATE) {
			_Jtag_GotoState(IRUPDATE);
			jtag_current_state = Jtag_GetNextState(jtag_current_state, 0);
			UpdateVectors(vector);
		} else if (jtag_current_state == RESET) {
			jtag_current_state = Jtag_GetNextState(jtag_current_state, 0);
			UpdateVectors(vector);
		}
	}

	jtag_current_state = IDLE ;

	return &JtagVectors;	
}

static vectors_t *Idle(void) {
	ClearJtagVectors();
	return ( _Idle() );
}

int Jtag_SetPin_Order(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx) {
	int pin_number;
	for (pin_number = 0; pin_number < JTAG_PINS_COUNT; pin_number++) {
		JtagPinOrder[pin_number] = -1;
	}


	JtagPinOrder[TRST_IDX] = ( trst_idx == -1) ? (JTAG_PINS_COUNT-1): trst_idx  ;
	JtagPinOrder[TCK_IDX]  = tck_idx;
	JtagPinOrder[TMS_IDX]  = tms_idx;
	JtagPinOrder[TDI_IDX]  = tdi_idx;
	JtagPinOrder[TDO_IDX]  = tdo_idx;

	for (pin_number = 0; pin_number < JTAG_PINS_COUNT; pin_number++) {
		//If JtagPinOrder is -1, then its not assigned.
		if (JtagPinOrder[pin_number] == -1) {
			return 1;
		} else {
			//Check if the index is repeated.
			int next_pin_number;
			for (next_pin_number = pin_number + 1; next_pin_number < JTAG_PINS_COUNT; next_pin_number++) {
				if (JtagPinOrder[pin_number] == JtagPinOrder[next_pin_number]) {
					return 1;
				}
			}	
		}
	}
	return 0;
}


static vectors_t *_Jtag_GotoState(jtag_state_t state) {
        char vector[JTAG_PINS_COUNT + 1];
        vector[JtagPinOrder[TRST_IDX]] = (reset_polarity == POSITIVE) ? '0':'1' ;
        vector[JtagPinOrder[TCK_IDX]] = '1';
        vector[JtagPinOrder[TMS_IDX]] = '0';
        vector[JtagPinOrder[TDI_IDX]] = '0';
        vector[JtagPinOrder[TDO_IDX]] = 'X';
        vector[JTAG_PINS_COUNT] = '\0';

	if (state == jtag_current_state ) {
		if (run_test) {
			if ( Jtag_GetNextState(jtag_current_state, 0) == state ) {
        			vector[JtagPinOrder[TMS_IDX]] = '0';
				UpdateVectors(vector);			
				return &JtagVectors;
			} else if ( ( Jtag_GetNextState(jtag_current_state, 1) ) == state ) {
        			vector[JtagPinOrder[TMS_IDX]] = '1';
				UpdateVectors(vector);			
				return &JtagVectors;
			} else {
				error = 1;
				return (vectors_t *) 0;
			}
		} else {
			return &JtagVectors;
		}
	}

	/*
	if (state == jtag_current_state) {
		return &JtagVectors;
	} 
	*/

	if (jtag_current_state == XXX) {
		_Reset();
		return ( _Jtag_GotoState(state) );
	}

	jtag_state_t next_state;
	if ( state >= DRSELECT && state <= DRUPDATE ) { //Goto DR Branch
		if ( jtag_current_state >= DRSELECT && jtag_current_state <= DRUPDATE ) { //Already in DR Branch, if you can on shifting 0, you will move down the state machine
			if (state > jtag_current_state) {
				while ( state != jtag_current_state) {
					int tms = 0;
					next_state = Jtag_GetNextState(jtag_current_state, tms);
					//If you are trap on the same state, or you went backwards, then try a 1 on TMS.
					if (next_state <= jtag_current_state ) {
						tms = 1;
						next_state = Jtag_GetNextState(jtag_current_state, tms);
					}
					jtag_current_state = next_state;
        				vector[JtagPinOrder[TMS_IDX]] = '0' + tms;
					UpdateVectors(vector);			
				}
			} else {
				_Jtag_GotoState(DRUPDATE);

				next_state = Jtag_GetNextState(jtag_current_state, 1);
				jtag_current_state = next_state;
        			vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
				UpdateVectors(vector);			

				_Jtag_GotoState(state);
			}
		} else if ( jtag_current_state >= IRSELECT && jtag_current_state <= IRUPDATE ) { //On another branch.
			_Jtag_GotoState(IRUPDATE);

			next_state = Jtag_GetNextState(jtag_current_state, 1);
			jtag_current_state = next_state;
        		vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
			UpdateVectors(vector);			

			_Jtag_GotoState(state);
		} else if ( jtag_current_state == RESET) {
			_Idle() ;
			_Jtag_GotoState(state);
		} else if ( jtag_current_state == IDLE ) {
			next_state = Jtag_GetNextState(jtag_current_state, 1);
			jtag_current_state = next_state;
        		vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
			UpdateVectors(vector);			

			_Jtag_GotoState(state);
		}
	} else if ( state >= IRSELECT && state <= IRUPDATE ) { //Goto IR Branch
		if ( jtag_current_state >= IRSELECT && jtag_current_state <= IRUPDATE ) { //Already in IR Branch, if you can on shifting 0, you will move down the state machine
			if (state > jtag_current_state) {
				while ( state != jtag_current_state) {
					int tms = 0;
					next_state = Jtag_GetNextState(jtag_current_state, tms);
					//If you are trap on the same state, or you went backwards, then try a 1 on TMS.
					if (next_state <= jtag_current_state ) {
						tms = 1;
						next_state = Jtag_GetNextState(jtag_current_state, tms);
					}
					jtag_current_state = next_state;
        				vector[JtagPinOrder[TMS_IDX]] = '0' + tms;
					UpdateVectors(vector);			
				}
			} else {
				_Jtag_GotoState(IRUPDATE);

				next_state = Jtag_GetNextState(jtag_current_state, 1);
				jtag_current_state = next_state;
        			vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
				UpdateVectors(vector);			

				_Jtag_GotoState(state);
			}
		} else if ( jtag_current_state >= DRSELECT && jtag_current_state <= DRUPDATE ) { //On another branch.
			_Jtag_GotoState(DRUPDATE);

			next_state = Jtag_GetNextState(jtag_current_state, 1);
			jtag_current_state = next_state;
        		vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
			UpdateVectors(vector);			

			next_state = Jtag_GetNextState(jtag_current_state, 1);
			jtag_current_state = next_state;
        		vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
			UpdateVectors(vector);			

			_Jtag_GotoState(state);
               } else if ( jtag_current_state == RESET) {
                        _Idle() ;
                        _Jtag_GotoState(state);
               } else if ( jtag_current_state == IDLE ) {
                        next_state = Jtag_GetNextState(jtag_current_state, 1);
                        jtag_current_state = next_state;
                        vector[JtagPinOrder[TMS_IDX]] = '0' + 1;
                        UpdateVectors(vector);

                        next_state = Jtag_GetNextState(jtag_current_state, 1);
                        jtag_current_state = next_state;
                        UpdateVectors(vector);

                        _Jtag_GotoState(state);
               }
	} else if ( state == RESET) {
		_Reset();	
	} else if ( state == IDLE) {
		_Idle();
	}

	if ( jtag_current_state != state) {
		error = 1; 

                ClearJtagVectors();
                return (vectors_t*) 0;
	}
	return &JtagVectors ;
}

vectors_t *Jtag_GotoState(jtag_state_t state) {
	ClearJtagVectors();	
	return (_Jtag_GotoState(state));
}


static vectors_t *shiftIn(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK, jtag_state_t end_state) {
        char vector[JTAG_PINS_COUNT + 1];
        vector[JtagPinOrder[TRST_IDX]] = (reset_polarity == POSITIVE) ? '0':'1' ;
        vector[JtagPinOrder[TCK_IDX]] = '1';
        vector[JtagPinOrder[TMS_IDX]] = '0';
        vector[JtagPinOrder[TDI_IDX]] = '0';
        vector[JtagPinOrder[TDO_IDX]] = 'X';
        vector[JTAG_PINS_COUNT] = '\0';

	jtag_state_t next_state;	
	int exit_after_shift = ( ( end_state == IREXIT1  ) || (end_state == DREXIT1)  );
	int upd_after_shift  = ( ( end_state == IRUPDATE ) || (end_state == DRUPDATE) );
	

	int i;
	for (i=0; i<length; i++) {
		char tdi = IN[length-1-i];
		char tdo = (OUT_MASK[length-1-i] == '1') ? 
				( OUT[length-1-i] == '1' || OUT[length-1-i] == 'H' ) ?
					'H':'L'
				: 'X' ;

		vector[JtagPinOrder[TDI_IDX]] = tdi;
		vector[JtagPinOrder[TDO_IDX]] = tdo;	
	
		if ( (i == length-1) && ( exit_after_shift || upd_after_shift ) ) {
                	next_state = Jtag_GetNextState(jtag_current_state, 1);
                	jtag_current_state = next_state;
        		vector[JtagPinOrder[TMS_IDX]] = '1';
		} else {	
                	next_state = Jtag_GetNextState(jtag_current_state, 0);
                	jtag_current_state = next_state;
		}
                UpdateVectors(vector);
	}

	if (upd_after_shift) {
        	vector[JtagPinOrder[TMS_IDX]] = '1';
        	vector[JtagPinOrder[TDI_IDX]] = '0';
        	vector[JtagPinOrder[TDO_IDX]] = 'X';

               	next_state = Jtag_GetNextState(jtag_current_state, 1);
               	jtag_current_state = next_state;
                UpdateVectors(vector);
	}
}

static vectors_t *reg(jtag_state_t reg, int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK) {
	pad_pattern_t *header, *trailer, *last;
	jtag_state_t state_after_shift;
	jtag_state_t capture_reg;

	if (reg == DRSHIFT) {
		header = &HDR;
		trailer = &TDR;
		last = &LAST_SDR;
		state_after_shift = DRUPDATE;
		capture_reg = DRCAPTURE;
	} else {
		header = &HIR;
		trailer = &TIR;
		last = &LAST_SIR;
		state_after_shift = IRUPDATE;
		capture_reg = IRCAPTURE;
	}

	_Jtag_GotoState(capture_reg);

	//Add +2. 1 for the NULL terminator, 1 for the PAD.
	//Use strcpy to take advantage of the null terminator, thus not having to remember where the last vector stopped.
	//The pad is because TDO becomes available immediately on the falling edge of the clock following the CAPTURE-SHIFT transition.
	//Whereas TDI is latched only on the next rising edge of the clock.
	//So effectively, we have to right-shift TDO by 1 cycle. The MBS of TDO should be an X.
	//And to allign TDI, the LSB of TDI should be 0.

	int len = header->length + strlen(IN)             + trailer->length + 2;
	vector_t tdi   = malloc(len);  memset( tdi,   0, len );
	vector_t tdo   = malloc(len);  memset( tdo,   0, len );
	vector_t mask  = malloc(len);  memset( mask,  0, len );
	vector_t smask = malloc(len);  memset( smask, 0, len );

	// TDI- 	9876543210X
	//SMASK-	9876543210X

	// TDO-		X9876543210 
	// MASK- 	X9876543210
	tdo[0]     = mask[0] 	  = '0';

	//trailer
	if ( trailer->length > 0 ) {
		strcpy(tdi, 	trailer->tdi) ;
		strcpy(smask, 	trailer->smask) ;
		strcat(tdo, 	trailer->tdo) ;
		strcat(mask, 	trailer->mask) ;
	}	

	//Data 
	vector_t in, out, in_mask, out_mask;
	if (IN){
		free(last->tdi);
		last->tdi = malloc(length + 1);
		last->length = length;
		strcpy(last->tdi, IN);
	}
	in = last->tdi ;

	if (OUT) {
		free(last->tdo);
		last->tdo = malloc(length + 1);
		last->length = length;
		strcpy(last->tdo, OUT);
	}
	out = last->tdo;

	if (IN_MASK) {
		free(last->smask);
		last->smask = malloc(length + 1);
		last->length = length;
		strcpy(last->smask, IN_MASK);
	}
	in_mask = last->smask;

	if (OUT_MASK) {
		free(last->mask);
		last->mask = malloc(length + 1);
		last->length = length;
		strcpy(last->mask, OUT_MASK);
	}
	out_mask = last->mask;

	strcat(tdi, 	in) ;
	strcat(tdo, 	out) ;
	strcat(mask, 	out_mask) ;
	strcat(smask, 	in_mask) ;

	//header
	if ( header->length > 0 ) {
		strcat(tdi, 	header->tdi)   ;
		strcat(tdo, 	header->tdo) 	;
		strcat(mask, 	header->mask)  ;
		strcat(smask, 	header->smask) ;
	}

        tdi[len-2] = smask[len-2] = '0';

	//Subtract 1 from len to remove the null terminator.
	shiftIn(len-1,	tdi,	tdo,	smask,	mask,	state_after_shift);

	free(tdi);
	free(tdo);
	free(mask);
	free(smask);

	//End State
	if (reg == DRSHIFT) {
		_Jtag_GotoState(END_STATES[END_DR_IDX]);
	} else {
		_Jtag_GotoState(END_STATES[END_IR_IDX]);
	}

	return &JtagVectors; 
}

vectors_t *ireg(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK) {
	ClearJtagVectors();
	vectors_t *v = reg(IRSHIFT, length, IN, OUT, IN_MASK, OUT_MASK);

	if (error) {
		ClearJtagVectors();
		return (vectors_t*) 0;	
	} 

	return v;

}

vectors_t *dreg(int length, vector_t IN, vector_t OUT, vector_t IN_MASK, vector_t OUT_MASK) {
	ClearJtagVectors();
	vectors_t *v = reg(DRSHIFT, length, IN, OUT, IN_MASK, OUT_MASK) ;

	if (error) {
		ClearJtagVectors();
		return (vectors_t*) 0;	
	}

	return v;
}

vectors_t *runstate(int runcount, float min_time, float max_time, jtag_state_t runstate, jtag_state_t endstate, run_clk_t clk) {
	if (clk == SCK) {
		return (vectors_t *) 0;
	}			

	int jtag_cycles;
	float run_time;
	if (clk == NONE) {
		if ( (run_time = min_time) <= 0) {
			return (vectors_t *) 0;	
		} else if ( max_time > 0 ) {
			if ( min_time > max_time) {
				return (vectors_t *) 0;	
			}
			run_time = ( run_time + max_time ) / 2;		
		} 
		jtag_cycles = (int) Jtag_MaxFrequency*run_time;
	} else {
		if ( (jtag_cycles = runcount) <= 0 ) {
			return (vectors_t *) 0;	
		} else {
			run_time = (float) jtag_cycles/Jtag_MaxFrequency;
			
			if ( min_time > max_time ) {
				return (vectors_t *) 0;	
			}
			if (max_time == 0) {
				max_time = run_time + 1;
			}
			if ( run_time < min_time || run_time > max_time ) {
				return (vectors_t *) 0;
			}
		}					 
	}		
	if ( runstate != XXX ) {
		run_state = runstate;
	} 
	if (  endstate != XXX ) {
		end_state = endstate;
	}
	ClearJtagVectors();
	_Jtag_GotoState(run_state);
	if ( error == 1) {
		ClearJtagVectors();
		return ((vectors_t *) 0);
	}
	run_test = 1;
	int cycle=0;
	while ( cycle < jtag_cycles ) {
		cycle++;
		_Jtag_GotoState(run_state);
		if (error == 1) {
			run_test = 0;
			ClearJtagVectors();
			return ((vectors_t *) 0);
		}		
	} 
	run_test = 0;
	_Jtag_GotoState(end_state);
	if (error == 1) {
		ClearJtagVectors();
		return ((vectors_t *) 0);
	}		
	return &JtagVectors;
}

vectors_t *GotoStates(linklist_t *l ) {
	jtag_state_t *s = (jtag_state_t *) dequeue_linklist(l);
        int state_num = 0;
        while ( s ) {
        	if (state_num == 0) {
       			if ( Jtag_GotoState(*s) == 0 ) {
				error = 1;
        		}
                	state_num++;
                } else {
                        if ( _Jtag_GotoState(*s) == 0 ) {
                	        error = 1;
                       	}
                }
                s = (jtag_state_t *) dequeue_linklist(l);
        }
	if (error) {
		return ( (vectors_t *) 0);
	}
	return &JtagVectors;
}

vectors_t *GetVectors(void) {
	return &JtagVectors; 
}
