# Makefile
# Author: Dennis J. McWherter, Jr.
CXX=mpicxx
CXXFLAGS=-Wall -O0 -ggdb
INC=
LIBS=
OBJS=AnalyzeData.o Configuration.o Coord3D.o DCUtil.o main.o Master.o ParserBase.o Slave.o UTChemParser.o
EXE=../bin/datacorrelation

all: $(OBJS)
	$(CXX) $(CXXFLAGS) $(INC) -o $(EXE) $(LIBS) $(OBJS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJS) $(EXE)

