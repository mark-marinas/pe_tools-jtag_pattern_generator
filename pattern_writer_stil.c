#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "linklist.h"
#include "pattern_writer_stil.h"
#include "signal_defs.h"


FILE *stil_out = 0;
char *fname=0;
char *wft_name = "jtag_timing";
char *jtag_grp_name = "JTAG_PINS";

int TRST_IDX 	= 0;
int TCK_IDX     = 1;
int TMS_IDX     = 2; 
int TDI_IDX     = 3; 
int TDO_IDX     = 4; 

int hasTRST = 1;
int jtag_pin_count = 5;
static int header_written = 0;


linklist_t initial_comments = { 0, 0 };



pin_t jtag_pins[] = { 
	 { "TRST", IN, 0 }
	,{ "TCK",  IN, 1 }
	,{ "TMS",  IN, 2 }
	,{ "TDI",  IN, 3 }
	,{ "TDO", OUT, 4 }
};

pin_t jtag_pins_inorder[5] ;


static void close_delete_file() {
	if (stil_out != 0 && fname != 0 ) {
		fclose(stil_out);
		remove(fname);

		free(fname);
	}	
}

static int write_signalGroups() {
	int i=0;
	FILE *out = stil_out ;

	if (out == 0) {
		return 1;
	}
	

	if ((fprintf (out, "SignalGroups {\n" ) ) < 0 ) {
		close_delete_file();
		return 1;
	}

	if ( ( fprintf (out, "\t%s \'%s", jtag_grp_name, jtag_pins_inorder[i++].name ) ) < 0 ) {
		close_delete_file();
		return 1;
	}
	for (	; i<jtag_pin_count; i++ ) {
		if (( fprintf (out, " + %s", jtag_pins_inorder[i].name ) ) < 0 ) {
			close_delete_file();
			return 1;
		}
	}
	if ( (fprintf (out, "\';\n}\n\n") ) < 0 ) {
		close_delete_file();
		return 1;
	}
	return 0;
}


static int write_signals() {
	int i;
	FILE *out = stil_out;
	if (out == 0) {
		return 1;
	}


	if ((fprintf (out, "Signals {\n" )) < 0 ) {
		close_delete_file();
		return 1;
	}

	for (	i = 0; i<jtag_pin_count; i++ ) {
		if (
			(fprintf ( out, "\t%s %s;\n",  jtag_pins_inorder[i].name, toString_direction(jtag_pins_inorder[i].direction) ) ) < 0
		) {
			close_delete_file();
			return 1;

		}
	}
	if ( (fprintf (out, "}\n\n" )) < 0 ) {
		close_delete_file();
		return 1;
	}
	return 0;
}

static int write_header() {
	FILE *out = stil_out;
	if (out == 0) {
		return 1;
	}
	if (header_written == 1){
		return 0;
	}

	int i;
	int e;
	if ( (e = arrange_pins( jtag_pins_inorder, jtag_pins, jtag_pin_count )) != 0 ) {
		return e;	
	}
	if ( write_signals() ) {
		return 1;
	} else if ( write_signalGroups() ) {
		return 1;
	} 

	if ( fprintf (out, "Pattern {\n") < 0 ) {
		close_delete_file();
		return 1;
	}
	if ( fprintf (out, "\tW %s;\n", wft_name) < 0 ) {
		close_delete_file();
		return 1;
	}
	header_written = 1;
	//Write whatever comment there is before writing the first vector
	//After writing the header.
	char *comment = (char *) dequeue_linklist(&initial_comments);

	while (comment) {
		writeComment(comment);
		comment = dequeue_linklist(&initial_comments);
	}
	return 0;
}

static int DisableReset() {
	int i=0;
	hasTRST = 0;
	jtag_pin_count = 4;

	for (i=0; i<jtag_pin_count; i++) {
		jtag_pins[i] = jtag_pins[i+1];		
		jtag_pins[i].idx = jtag_pins[i].idx - 1;
	}

	TCK_IDX--;
	TMS_IDX--;
	TDI_IDX--;
	TDO_IDX--;
	return 0;

}

int SetPin_Order(int trst_idx, int tck_idx, int tms_idx, int tdi_idx, int tdo_idx)  {
	if (trst_idx == -1) {
		DisableReset();
		if ( tck_idx != -1) {
			jtag_pins[TCK_IDX].idx = tck_idx;
        		jtag_pins[TMS_IDX].idx = tms_idx;
        		jtag_pins[TDI_IDX].idx = tdi_idx;
        		jtag_pins[TDO_IDX].idx = tdo_idx;
		} else {
			jtag_pins[TCK_IDX].idx = TCK_IDX;
        		jtag_pins[TMS_IDX].idx = TMS_IDX;
        		jtag_pins[TDI_IDX].idx = TDI_IDX;
        		jtag_pins[TDO_IDX].idx = TDO_IDX;
		}
	} else {
		jtag_pins[TRST_IDX].idx= trst_idx;
		jtag_pins[TCK_IDX].idx = tck_idx;
        	jtag_pins[TMS_IDX].idx = tms_idx;
        	jtag_pins[TDI_IDX].idx = tdi_idx;
        	jtag_pins[TDO_IDX].idx = tdo_idx;
	}
	return 0;
}

int Stil_SetOutfilename(char *filename) {
	fname=malloc(strlen(filename) + 1);
	strcpy(fname, filename);

	stil_out = fopen(fname, "w") ;
	if (stil_out == 0) {
		free(fname);
		return 1;
	}
	init_linklist(&initial_comments);
	return 0;
}

int Stil_CloseOutfile(){
	FILE *out = stil_out;
	if (out == 0) {
		return 1;
	}
	if (fprintf (out, "}\n") < 0 ) {
		close_delete_file();
		return 1;
	}
	if (stil_out) {
		fclose(stil_out);
		free(fname);
	}
	return 0;
}

int writeVector(vector_line_t * v, int dbg_mode) {
	FILE *out = stil_out; 
	if (out == 0) {
		return 1;
	}
	//Write the header only where the first vector is to be written
	write_header();

	int tabs = 1;
	if (v->loopcnt > 1) {
		if (fprintf (out, "\tLoop %d {\n", v->loopcnt) < 0 ) {
			close_delete_file();
			return 1;
		}
		tabs++;
	}
	
	int tab; 
	for (tab=0; tab<tabs; tab++) {
		if ( fprintf(out, "\t") < 0 ) {
			close_delete_file();
			return 1;
		}
	}


	if (jtag_pin_count == 4) {
		chop((v->data));	
	}
	
	if ( fprintf (out, "V {  %s = %s;}", jtag_grp_name, v->data ) < 0 ) {
		close_delete_file();
		return 1;
	}

	if (dbg_mode) {
		if ( fprintf (out, "/* %s */\n", get_jtag_state_string(v->curr_state) ) < 0 ) {
			close_delete_file();
			return 1;
		} 
	} else {
		if ( fprintf (out, "\n", jtag_grp_name, v->data ) < 0 ) {
			close_delete_file();
			return 1;
		}
	}



	if (v->loopcnt > 1) {
		if (fprintf (out, "\t}\n") < 0 ) {
			close_delete_file();
			return 1;
		}
		tabs++;
	}
	return 0;

}

int writeComment(char *comment) {
	FILE *out =  stil_out; 
	if (out == 0) {
		return 1;
	}
	//Hold of writing  the comment until the header is written
	if (header_written == 0) {
		char *str = malloc(strlen(comment) + 1);
		strcpy(str, comment);
		enqueue_linklist(&initial_comments, (void *) str);
		return 0;
	}


	if ( fprintf(out, "/* %s */\n", comment) < 0 ) {
		close_delete_file();
		return 1;
	}	
	return 0;
}

#ifdef __STIL_WRITER_TEST__
int main() {
	SetPin_Order(-1, 2, 3, 1, 0);

	if (write_header()) {
		printf ("error\n");
	}
	return 0;
}



#endif
