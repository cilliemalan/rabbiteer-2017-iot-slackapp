CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-g -std=c++11 -Wall
LDFLAGS=-g -std=c++11 -Wall
LDLIBS=-lboost_system -lcpprest -lcrypto -lssl

SRCS=main.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

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
