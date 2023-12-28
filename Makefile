PREFIX?=	/usr/local

PROG = sim4
OBJS = sim4.o
DEFINES =
#CFLAGS = -O3 -Wall -DNDEBUG $(DEFINES)
CFLAGS = -g -Wall $(DEFINES)
LIBS = -lm

all: $(PROG)

$(PROG): $(OBJS);    $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

plot: $(PROG)
	./$(PROG) > plot.txt
	gnuplot sim4.gnuplot > plot.pdf

plot2: $(PROG)
	./$(PROG) -p > plot.txt
	gnuplot sim4.gnuplot > plot2.pdf

install: $(PROG)
	$(INSTALL) $(COPY) -m 0755 $(PROG) $(PREFIX)/bin

clean:;	-rm -f $(PROG) *.o core *.core *~
