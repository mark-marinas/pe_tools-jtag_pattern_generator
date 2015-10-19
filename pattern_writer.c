#include <string.h>

#include "pattern_writer.h"
#include "pattern_writer_stil.h"



int init_writer(pattern_writer_t *writer, opts_t *opts) {
        //Decide the pattern writer based on output filename
        if ( strstr(opts->out, ".stil") ) {
                writer->fn_writeComment = writeComment;
                writer->fn_writeVector  = writeVector;
                writer->fn_setPinOrder  = SetPin_Order;
                writer->fn_setOutFile   = Stil_SetOutfilename;
		writer->fn_closeOutFile = Stil_CloseOutfile;
        } else {
                return 1;
        }
        return 0;
}
