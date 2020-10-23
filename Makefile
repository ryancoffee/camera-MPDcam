CC=g++
DEBUG=-g -ggdb
SRCDIR=./src/
OUTDIR=./bin/64
LIBDIR=/MPDcam/spc3/Linux/lib/64/
CFLAGS=-Wall -pedantic -I./include -m64
EXECUTABLE=SDK_Example_tofile
SOURCETEST=SDK_Example_tofile.c
OBJECT=SDK_Example_tofile.o

all:$(SOURCETEST) $(EXECUTABLE) 
	
$(EXECUTABLE):  
	#$(CC) $(DEBUG) -Wl,-rpath,. $(CFLAGS) $(OUTDIR)/$(OBJECT) -o $(OUTDIR)/$@ -L$(LIBDIR) -lSPC3 -lokFrontPanel -ldl -lm
	#rm $(OUTDIR)/*.o
	$(CC) $(DEBUG) -Wl,-rpath,. $(CFLAGS) $(OUTDIR)/$(OBJECT) -o $@ -L$(LIBDIR) -lSPC3 -lokFrontPanel -ldl -lm
	rm $(OUTDIR)/*.o

$(SOURCETEST):
	mkdir -p $(OUTDIR)
	cp -f $(LIBDIR)/* $(OUTDIR)
	$(CC) $(DEBUG) -c $(CFLAGS) $(SRCDIR)$@ -o $(OUTDIR)/$(@:.c=.o)
clean:
	rm -rf $(OUTDIR)
