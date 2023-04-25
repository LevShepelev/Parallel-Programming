#!/bin/bash
#PBS -l walltime=00:00:05,nodes=1:ppn=4
#PBS -N HelloWorldTest
#PBS -q batch

cd .
mpirun  -np 10 ./Sum 100
