#include <iostream>
#include <argparse.h>
#include <threads.h>
#include <io.h>
#include <chrono>
#include <cstring>
#include "operators.h"
#include "helpers.h"
#include "prefix_sum.h"

int main(int argc, char **argv)
{
    // Parse args
    struct options_t opts;
    get_opts(argc, argv, &opts);

    bool sequential = false;
    if (opts.n_threads == 0) {
        opts.n_threads = 1;
        sequential = true;
    }

    // Setup args & read input data
    int n_vals;
    int *input_vals, *output_vals;
    read_file(&opts, &n_vals, &input_vals, &output_vals);

    if (n_vals <= opts.n_threads){
        opts.n_threads = n_vals - 1;
    }

    pthread_barrier_t glob_barrier;
    pthread_barrier_t *barrier = &glob_barrier;

    pthread_barrier_init(barrier, NULL, opts.n_threads);

    // Setup threads
    pthread_t *threads = sequential ? NULL : alloc_threads(opts.n_threads);;
    pthread_t *threads2 = sequential ? NULL : alloc_threads(opts.n_threads);;

    prefix_sum_args_t *ps_args = alloc_args(opts.n_threads);

    //"op" is the operator you have to use, but you can use "add" to test
    int (*scan_operator)(int, int, int);
    scan_operator = op; 
    // scan_operator = add; // FIXME: Change this back to op operator

    fill_args(ps_args, opts.n_threads, n_vals, input_vals, output_vals,
        opts.spin, scan_operator, opts.n_loops, &glob_barrier);

    // Start timer
    auto start = std::chrono::high_resolution_clock::now();

    int total_operations = 0;

    if (sequential)  {
        //sequential prefix scan
        output_vals[0] = input_vals[0];
        for (int i = 1; i < n_vals; ++i) {
            //y_i = y_{i-1}  <op>  x_i
            output_vals[i] = scan_operator(output_vals[i-1], input_vals[i], ps_args->n_loops);
            total_operations++;
        }
    }
    else {
        // Declare variables for splitting input into blocks
        int start_int = 0;
        int base_block_size = ps_args->n_vals / opts.n_threads;
        int temp_block_size = base_block_size;
        int remaining = ps_args->n_vals - (base_block_size * opts.n_threads);

        int temp_vals[opts.n_threads]; // Declare temporary array for each thread to dump the partial prefix sum
        ps_args->block_size = base_block_size; // Set the block_size

        // Loop through each thread and set the attributes (block size, index, etc)
        for(int i = 0; i < ps_args->n_threads; i++){
            total_operations++;
            ps_args[i].operations = 0;
            ps_args[i].temp_vals = temp_vals;
            ps_args[i].index = i;
            ps_args[i].start_int = start_int;
            if (start_int + temp_block_size > ps_args->n_vals){
                temp_block_size = ps_args->n_vals - start_int;
            }
            // If the input can't be equally split amongst threads, add remainder to each thread until it it gone
            if (remaining){
                temp_block_size++;
                remaining--;
            }
            start_int += temp_block_size;
            ps_args[i].block_size = temp_block_size;

            temp_block_size = base_block_size;

        }

        // Start the threads to compute the partial prefix sum
        start_threads(threads, opts.n_threads, ps_args, compute_prefix_sum);

        // Wait for threads to finish
        join_threads(threads, opts.n_threads);

        // Sequential code, used to finish calculations for partail prefix sum before second sweep
        total_operations += ps_args[0].operations;
        // std::cout << "Thread #" << ps_args[0].index << " " << ps_args[0].operations << std::endl;
        ps_args[0].operations = 0;
        for (int j = 1; j < opts.n_threads; j++){
            total_operations++;
            // std::cout << "Thread #" << ps_args[j].index << " " << ps_args[j].operations << std::endl;
            total_operations += ps_args[j].operations;
            ps_args[j].operations = 0;
            ps_args->temp_vals[j] = ps_args->temp_vals[j] + ps_args->temp_vals[j-1];
        }
        
        // Start the threads to compute the second sweep of the prefix sum
        start_threads(threads2, opts.n_threads, ps_args, compute_second_sweep);

        // Wait for threads to finish
        join_threads(threads2, opts.n_threads);
        
        pthread_barrier_destroy(barrier);
        for (int k = 0; k < opts.n_threads; k++){
            // std::cout << "Thread #" << ps_args[k].index << " " << ps_args[k].operations << std::endl;
            total_operations += ps_args[k].operations;
        }

    }

    std::cout << "Total Operations: " << total_operations << std::endl;

    //End timer and print out elapsed
    auto end = std::chrono::high_resolution_clock::now();
    auto diff = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    std::cout << "time: " << diff.count() << std::endl;

    // Write output data
    write_file(&opts, &(ps_args[0]));

    // Free other buffers
    free(threads);
    free(threads2);
    free(ps_args);
}
