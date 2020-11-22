CC=g++
DEBUG=-g -ggdb
SRCDIR=./src/
OUTDIR=./bin/64
LIBDIR=/MPDcam/spc3/Linux/lib/64/
CVFLAGS=`pkg-config --cflags --libs /opt/installation/OpenCV-master/lib64/pkgconfig/opencv4.pc`
CFLAGS=-Wall -pedantic -I./include -m64 -std=c++11
EXECUTABLE=mpdHist_tofile
SOURCETEST=mpdHist_tofile.c
OBJECT=mpdHist_tofile.o

all:$(SOURCETEST) $(EXECUTABLE) 
	
$(EXECUTABLE):  
	#$(CC) $(DEBUG) -Wl,-rpath,. $(CFLAGS) $(OUTDIR)/$(OBJECT) -o $(OUTDIR)/$@ -L$(LIBDIR) -lSPC3 -lokFrontPanel -ldl -lm
	#rm $(OUTDIR)/*.o
	$(CC) $(DEBUG) -Wl,-rpath,. $(CFLAGS) $(CVFLAGS) $(OUTDIR)/$(OBJECT) -o $@ -L$(LIBDIR) -lSPC3 -lokFrontPanel -ldl -lm
	rm $(OUTDIR)/*.o

$(SOURCETEST):
	mkdir -p $(OUTDIR)
	cp -f $(LIBDIR)/* $(OUTDIR)
	$(CC) $(DEBUG) -c $(CFLAGS) $(CVFLAGS) $(SRCDIR)$@ -o $(OUTDIR)/$(@:.c=.o)
clean:
	rm -rf $(OUTDIR)
