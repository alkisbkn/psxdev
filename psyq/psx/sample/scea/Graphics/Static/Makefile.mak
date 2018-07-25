ASM  = asmpsx
CC   = ccpsx
LINK = psylink

CCOPTIONS  =  -c -g -comments-c++
ASMOPTIONS = /l /o c+,h+,at- /zd
LINKOPTS   = /m
TEST_OBJS  = main.obj graph.obj datafile.obj 


all  : main.cpe
       echo Done.

main.obj: main.c main.h
        $(CC) $(CCOPTIONS) main.c -omain.obj

graph.obj: graph.c
	$(CC) $(CCOPTIONS) graph.c  -ograph.obj

datafile.obj: datafile.asm
        asmpsx /l datafile.asm,datafile.obj

main.cpe: $(TEST_OBJS) main.lnk
	$(LINK) $(LINKOPTS) @main.lnk,main.cpe,main.sym,main.map

