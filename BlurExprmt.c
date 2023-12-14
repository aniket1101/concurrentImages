#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "Utils.h"
#include "Picture.h"
#include "PicProcess.h"

// ---------- MAIN PROGRAM ---------- \\

#define THOUSAND 1000

static void blur_func(void (*partition_func) (struct picture *pic));

  int main(int argc, char **argv){

    printf("Support Code for Running the Blur Optimisation Experiments... \n");
    
    printf("Sequential blur takes:\n");
    blur_func(&blur_picture);

    printf("Maximumly parallelised blur takes:\n");
    blur_func(&parallel_blur_picture);

    printf("Blurring by column takes:\n");
    blur_func(&col_blur_picture);

    printf("Blurring by row takes:\n");
    blur_func(&row_blur_picture);

    printf("Blurring by each quarter takes:\n");
    blur_func(&quarter_blur_picture);
  }

static void blur_func(void (*partition_func) (struct picture *pic)) {
    long long avg_time = 0;
    struct timeval start, stop;
    struct picture pic;

    /* Loops 100 times to see how long each partition implementation takes 
       to blur. */
    for (int i = 0; i < 100; i++) {
        init_picture_from_file(&pic, "test_images/frank.jpg");
        gettimeofday(&start, NULL);
        partition_func(&pic);
        gettimeofday(&stop, NULL);
        avg_time += (stop.tv_sec - start.tv_sec) * THOUSAND +
                    (stop.tv_usec - start.tv_usec) / THOUSAND;
    }

    // Compute average time by dividing by number of tests
    avg_time /= 100;
    printf("%llu milliseconds\n", avg_time);
}