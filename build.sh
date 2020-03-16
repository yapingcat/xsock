#!/bin/bash

g++ -g -c -std=c++11 -DHAVE_POLL xsock.cpp  -o xsock.o
ar rcs -o libxsock.a xsock.o
