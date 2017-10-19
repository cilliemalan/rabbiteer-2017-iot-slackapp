
CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -std=c++11 -Wall
LDFLAGS=-g -std=c++11 -Wall
LDLIBS=-lboost_system -lcpprest -lcrypto -lssl

SRCS=main.cpp
OBJS=$(subst .cpp,.o,$(SRCS))
PCH=pch.h.gch

all: iotslack

iotslack: $(OBJS) $(PCH)
	$(CXX) $(LDFLAGS) -o iotslack $(OBJS) $(LDLIBS) 

depend: .depend

.depend: $(SRCS)
	$(CXX) $(CPPFLAGS) -MM $^>./.depend;

clean:
	$(RM) $(OBJS) $(PCH) iotslack

distclean: clean
	$(RM) *~ .depend

$(PCH): pch.h
	g++ $(CPPFLAGS) pch.h

include .depend
