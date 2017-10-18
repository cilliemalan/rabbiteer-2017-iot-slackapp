CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -std=c++14 $(shell root-config --cflags)
LDFLAGS=-g $(shell root-config --ldflags)
LDLIBS=$(shell root-config --libs)

SRCS=main.cpp
OBJS=$(subst .cc,.o,$(SRCS))

all: iotslack

iotslack: $(OBJS)
	$(CXX) $(LDFLAGS) -o iotslack $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(RM) ./.depend
	$(CXX) $(CPPFLAGS) -MM $^>>./.depend;

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) *~ .depend

include .depend
