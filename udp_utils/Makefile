EXE = listener talker

COMPILER = gcc
COMPILER_OPTS =
LINKER = gcc
LINKER_OPTS =

all : $(EXE)

listener : listener.c
	$(COMPILER) $(COMPILER_OPTS) -o listener listener.c

talker : talker.c
	$(COMPILER) $(COMPILER_OPTS) -o talker talker.c

clean :
	-rm -f *.o $(EXE)
