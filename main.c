#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "mpi.h"
#include "gmp.h"

int GetFactorialNumberNewtonMethod(int accuracy);
void CalculatePartExponentSum(mpf_t partial_sum, int fact_num, int start, int end, int my_rank, int commsize);
void CalculateFullExponentSum(mpf_t partial_sum, int fact_num, int start, int end, int my_rank, int commsize, int accuracy);

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int accuracy = strtol(argv[argc - 1], NULL, 10);

    mpf_set_default_prec(64 + ceil(4 * accuracy));

    int fact_num = GetFactorialNumberNewtonMethod(accuracy);

    int commsize = 0;
    int my_rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int main_part = fact_num / commsize;
    int remainder = fact_num % commsize;
    int start = 1 + main_part * my_rank;
    int end = 1 + main_part * (my_rank + 1);

    if (remainder != 0 && my_rank == commsize - 1) end += remainder;

    mpf_t partial_sum;
    mpf_init(partial_sum);
    mpf_set_ui(partial_sum, 0);

    CalculatePartExponentSum(partial_sum, fact_num, start, end, my_rank, commsize);
    CalculateFullExponentSum(partial_sum, fact_num, start, end, my_rank, commsize, accuracy);

    mpf_clear(partial_sum);

    MPI_Finalize();

    return 0;
}


int GetFactorialNumberNewtonMethod(int accuracy) 
{
    double x_0 = 2.0;
    double cnst = (double)accuracy * log(10.0);
    double epsilon = 1.0;

    double prev_x = x_0;
    double cur_x = 0;

    while (1) {
        cur_x = prev_x - ((prev_x * log(prev_x) - prev_x - cnst) / log(prev_x));
        if (abs(cur_x - prev_x) < epsilon) {
            return (int)ceil(cur_x);
        }
        prev_x = cur_x;
    }

    return -1;
}

void CalculatePartExponentSum(mpf_t partial_sum, int fact_num, int start, int end, int my_rank, int commsize) {
    mpz_t sum;
    mpz_init(sum);
	mpz_set_ui(sum, 1);

    int current_num = end - 1;

    mpz_t reverse_factorial;
    mpz_init(reverse_factorial);
	mpz_set_ui(reverse_factorial, end - 1);
    if ((end - start) != 1) {
        mpz_add(sum, sum, reverse_factorial);
        current_num -= 1;
    }

    for (int idx = 0; idx < end - start - 2; ++idx) {
        mpz_mul_ui(reverse_factorial, reverse_factorial, current_num);
        mpz_add(sum, sum, reverse_factorial);
        current_num -= 1;
        if (current_num == 0) break;
    }

    if (commsize > 1) {
        if (my_rank == 0) {
            char* factorial_str = mpz_get_str(NULL, 10, reverse_factorial);
            MPI_Send(factorial_str, strlen(factorial_str) + 1, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
        }
        if (my_rank > 0) {
            MPI_Status status;

            int rcv_fact_len = 0;
            MPI_Probe(my_rank - 1, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_CHAR, &rcv_fact_len);

            char* rcv_factorial_str = (char*)calloc(rcv_fact_len + 1, sizeof(char));
            MPI_Recv(rcv_factorial_str, rcv_fact_len + 1, MPI_CHAR, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            mpz_t prev_factorial;
            mpz_init_set_str(prev_factorial, rcv_factorial_str, 10);

            mpz_mul_ui(reverse_factorial, reverse_factorial, start);
            mpz_mul(reverse_factorial, reverse_factorial, prev_factorial);

            if (my_rank != (commsize - 1)) {
                char* factorial_str = mpz_get_str(NULL, 10, reverse_factorial);
                int fact_len = strlen(factorial_str);
                MPI_Send(factorial_str, fact_len + 1, MPI_CHAR, my_rank + 1, 0, MPI_COMM_WORLD);
            }

            free(rcv_factorial_str);
            mpz_clear(prev_factorial);
        }
    }

    mpf_t sum_f;
    mpf_init(sum_f);
    mpf_set_z(sum_f, sum);

    mpf_t reverse_factorial_f;
    mpf_init(reverse_factorial_f);
    mpf_set_z(reverse_factorial_f, reverse_factorial);

    mpf_div(partial_sum, sum_f, reverse_factorial_f);
    
    mpz_clears(reverse_factorial, sum, NULL);
    mpf_clears(reverse_factorial_f, sum_f, NULL);
}

void CalculateFullExponentSum(mpf_t partial_sum, int fact_num, int start, int end, int my_rank, int commsize, int accuracy) {
    mpf_t result_sum;
    mpf_init(result_sum);
    mpf_set_ui(result_sum, 1);

    MPI_Status status;

    if (my_rank == 0) {
        mpf_add(result_sum, result_sum, partial_sum);

        char *rvc_buf = (char*)calloc(accuracy, sizeof(char));

        for (int idx = 1; idx < commsize; ++idx) {
            MPI_Recv(rvc_buf, accuracy, MPI_CHAR, idx, 0, MPI_COMM_WORLD, &status);
            mpf_set_str(partial_sum, rvc_buf, 10);
            mpf_add(result_sum, result_sum, partial_sum);
        }
        free(rvc_buf);
        gmp_printf ("EXPONENT = %.*Ff\n", accuracy, result_sum);
    }
    else {
        char *mpi_buf = (char*)calloc(accuracy, sizeof(char));
        char *format_str = (char*)calloc(accuracy, sizeof(char));

        size_t format_str_size = accuracy;

        snprintf(format_str, format_str_size, "%%.%dFf", accuracy);
        gmp_snprintf(mpi_buf, accuracy, format_str, partial_sum);
        MPI_Send(mpi_buf, accuracy, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        free(format_str);
        free(mpi_buf);
    }
    mpf_clear(result_sum);
}