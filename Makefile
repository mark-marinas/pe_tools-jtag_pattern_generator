CC=gcc
SRCS=svf_parser.c string_manip.c signal_defs.c pattern_writer_stil.c pattern_writer.c main.c linklist.c errors.c input_parser.c jtag_driver.c jtag_fsm.c
HEADERS=svf_parser.h string_manip.h signal_defs.h pattern_writer_stil.h pattern_writer.h main.h linklist.h errors.h input_parser.h jtag_driver.h jtag_fsm.h
EXE=JtagPatGen.exe

EXE: $(SRCS) $(HEADERS)
	$(CC) $(SRCS) -o $(EXE)


clean:
	rm -f $(exe)

