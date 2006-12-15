#!/bin/sh

MEX="mkoctfile --mex" FLAGS="-DmexAtExit=atexit" make MEXOUT=mex install
rm -f Channel.o MCAError.o TestHash.cpp mca.o
