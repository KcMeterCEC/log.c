#!/bin/sh
gcc -c -g log.c -o log.o
gcc -c -g main.c -o main.o
gcc log.o main.o -pthread -o log
