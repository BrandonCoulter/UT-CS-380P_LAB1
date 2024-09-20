#include <iostream>
#include "prefix_sum.h"
#include "helpers.h"

void* compute_prefix_sum(void *a)
{
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;

#ifdef __PRINT__
    std::cout << "Args: " << typeid(args).name() << std::endl;
    std::cout << "Threads: " << args->n_threads << std::endl;
    std::cout << "Vals: " << args->n_vals << std::endl;
    std::cout << "Spin: " << args->spin << std::endl;
    std::cout << "Op: " << args->op << std::endl;
    std::cout << "Loops: " << args->n_loops << std::endl;
    std::cout << "Barrier: " << args->g_barrier << std:: endl;
#endif
    std::cout << "Thread #" << args->index << " | Block Size: " << args->block_size << std::endl;
    int temp_ops = 0;
    if(args->index < args->n_threads){
        args->output_vals[args->start_int] = args->input_vals[args->start_int];
        for (int i = args->start_int + 1; i < args->start_int + args->block_size; ++i) {
            temp_ops ++;
            //y_i = y_{i-1}  <op>  x_i
            args->output_vals[i] = args->op(args->output_vals[i-1], args->input_vals[i], args->n_loops);
            args->temp_vals[args->index] = args->output_vals[i];
        }
    } else{
        args->temp_vals[args->index] = args->output_vals[0];
    }

    args->operations = temp_ops;
    // std::cout << args->operations << std::endl;

    pthread_barrier_wait(args->g_barrier);

    return 0;
}

void* compute_second_sweep(void *a)
{
    prefix_sum_args_t *args = (prefix_sum_args_t *)a;
    
    int temp_ops = 0;
    if (args->index > 0){
        args->output_vals[args->start_int] = args->output_vals[args->start_int] + args->temp_vals[args->index - 1];
        for (int i = args->start_int + 1; i < args->start_int + args->block_size; ++i) {
            args->output_vals[i] = args->op(args->output_vals[i-1], args->input_vals[i], args->n_loops);
            temp_ops++;
        }
    }

    args->operations = temp_ops;

    // std::cout << args->operations << std::endl;

    pthread_barrier_wait(args->g_barrier);

    return 0;
}
