#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "math.h"

#include "mpi.h"
#include "gmp.h"

int give_factorial_number(int accuracy);
void CalculatePartExponentSum(mpf_t part_sum, int factorial_number, int start, int end, int task_id, int commsize);

int give_factorial_number(int accuracy) {
    if (accuracy < 1000) 
        return 100;
    else return sqrt(accuracy * 3) + 1; //accuracy <= (n + 1)*log(n / e) <= (n + 1)^2 / e
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

void CalculatePartExponentSum(mpf_t part_sum, int factorial_number, int start, int end, int task_id, int commsize)
{
    mpz_t sum; // sum is sum from start! to end! for example 1 + 200 + 200 * 199 + 200 * 199 * 198... till 200! / 100!
    mpz_init(sum);
	mpz_set_ui(sum, 1); // 1 is the last element in sum

    int current_num = end - 1;

    mpz_t factorial; // factoral is counted from end ex 200 * 199 * 198 * ...
    mpz_init(factorial);
	mpz_set_ui(factorial, end - 1);
    if ((end - start) != 1) {
        mpz_add(sum, sum, factorial);
        current_num--;
    }

    for (int i = 0; i < end - start - 2; ++i) {
        mpz_mul_ui(factorial, factorial, current_num);
        mpz_add(sum, sum, factorial);
        --current_num;
        if (current_num == 0) {
            break;
        }
    }

    if (commsize > 1) {
        if (task_id == 0) {
            char* factorial_str = mpz_get_str(NULL, 10, factorial);
            MPI_Send(factorial_str, strlen(factorial_str) + 1, MPI_CHAR, task_id + 1, 0, MPI_COMM_WORLD);
        }
        if (task_id > 0) {
            MPI_Status status;

            int rcv_fact_len = 0;
            MPI_Probe(task_id - 1, 0, MPI_COMM_WORLD, &status);
            MPI_Get_count(&status, MPI_CHAR, &rcv_fact_len);

            char* rcv_factorial_str = (char*)calloc(rcv_fact_len + 1, sizeof(char));
            MPI_Recv(rcv_factorial_str, rcv_fact_len + 1, MPI_CHAR, task_id - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            mpz_t prev_factorial;
            mpz_init_set_str(prev_factorial, rcv_factorial_str, 10);

            mpz_mul_ui(factorial, factorial, start);
            mpz_mul(factorial, factorial, prev_factorial); // 200! = 200 * 199 * ... * 101 * 100!, 100! we get from another process

            if (task_id != (commsize - 1)) {
                char* factorial_str = mpz_get_str(NULL, 10, factorial);
                int fact_len = strlen(factorial_str);
                MPI_Send(factorial_str, fact_len + 1, MPI_CHAR, task_id + 1, 0, MPI_COMM_WORLD);
            }

            free(rcv_factorial_str);
            mpz_clear(prev_factorial);
        }
    }

    mpf_t sum_f;
    mpf_init(sum_f);
    mpf_set_z(sum_f, sum);

    mpf_t factorial_f;
    mpf_init(factorial_f);
    mpf_set_z(factorial_f, factorial);

    mpf_div(part_sum, sum_f, factorial_f);
    
    mpz_clears(factorial, sum, NULL);
    mpf_clears(factorial_f, sum_f, NULL);
}


int main(int argc, char** argv){
    
    double t1, t2, dt;

    int mpi_status = MPI_Init(&argc, &argv);
    if (mpi_status != MPI_SUCCESS) {
        printf("MPI_Init failed");
        return EXIT_FAILURE;
    }

    int accuracy = atoi(argv[1]);
    if (accuracy <= 0) {
        printf("Bad input, accuracy = %s", argv[1]);
        MPI_Finalize();
        return EXIT_FAILURE;
    }
    mpf_set_default_prec(64 + ceil(4 * accuracy));
    int factorial_number = GetFactorialNumberNewtonMethod(accuracy);
    const int root_task_id = 0;
    int commsize = 0;
    int task_id = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &commsize);
    MPI_Comm_rank(MPI_COMM_WORLD, &task_id);

    int distance = factorial_number / commsize;
    int remainder = factorial_number % commsize;
    int start = 1 + distance * task_id;
    int end = 1 + distance * (task_id + 1);

    // If the remainder is not zero, we add 1 term of the sum to each process starting from zero
    if (remainder != 0 && task_id == commsize - 1) end += remainder;

    mpf_t result_sum;
    mpf_init(result_sum);
    mpf_set_ui(result_sum, 1); // 0!

    mpf_t part_sum;
    mpf_init(part_sum);

    CalculatePartExponentSum(part_sum, factorial_number, start, end, task_id, commsize);

    MPI_Status status;

    if (task_id == root_task_id) {
        mpf_add(result_sum, result_sum, part_sum);

        char *rvc_buf = (char*)calloc(accuracy + 8, sizeof(char));

        for (int i = 1; i < commsize; ++i) {
            MPI_Recv(rvc_buf, accuracy + 8, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status);
            mpf_set_str(part_sum, rvc_buf, 10);
            mpf_add(result_sum, result_sum, part_sum);
        }
        free(rvc_buf);
        printf("hello\n");
        gmp_printf ("[RESULT] Exp = %.*Ff\n", accuracy, result_sum);
    }
    else {
        char *mpi_buf = (char*)calloc(accuracy + 8, sizeof(char));
        char *format_str = (char*)calloc(accuracy + 12, sizeof(char));

        size_t format_str_size = accuracy + 12;

        snprintf(format_str, format_str_size, "%%.%dFf", accuracy);
        gmp_snprintf(mpi_buf, accuracy + 8, format_str, part_sum);
        MPI_Send(mpi_buf, accuracy + 8, MPI_CHAR, 0, 0, MPI_COMM_WORLD);

        free(format_str);
        free(mpi_buf);
    }

    mpf_clear(result_sum);
    mpf_clear(part_sum);

    MPI_Finalize();

    return 0;
}