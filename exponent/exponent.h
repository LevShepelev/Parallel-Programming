#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>
#include <mpi.h>
#include <gmp.h>

int give_factorial_number(int accuracy);
void CalculatePartExponentSum(mpf_t part_sum, int factorial_number, int start, int end, int task_id, int commsize);
