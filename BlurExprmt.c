#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>
#include "Utils.h"
#include "Picture.h"

// ---------- MAIN PROGRAM ---------- \\

#define NO_RGB_COMPONENTS 3
#define BLUR_REGION_SIZE 9
#define NO_QUARTERS 4
#define THOUSAND 1000

struct pic_info {
  struct picture *pic;
  struct picture *tmp;
  int i;
  int j;
  int initi;
  int initj;
  int endi;
  int endj;
};

// Represents a thread pool using a linked-list as a medium
struct t_pool {
  struct node *head; /* Head of the linked list. */
  struct node *tail; /* Tail of the linked list. */
};

struct node {
  struct node *prev; /* Previous node in the linked list. */
  struct node *next; /* Next node in the linked list. */
  pthread_t thread;  /* Thread beloning to the current node. */
};

static void blur_func(void (*partition_func) (struct picture *pic));

// Thread Pool Functions
static bool thread_pool_init(struct t_pool *pool);
static struct node *create_node(pthread_t thread);
static void remove_node(struct node *node);
static bool add_thread_to_pool(pthread_t thread, struct t_pool *pool);
static void threads_join(struct t_pool *pool);
static void tryjoin_threads(struct t_pool *pool);

// Picture transformation routines
static void blur_picture(struct picture *pic);
static void blur_pixel(struct pic_info *info);

// Parallelisation functions
static void parallel_blur_picture(struct picture *pic);
static bool new_pixel_thread(pthread_t *thread, struct picture *pic,
                    struct picture *tmp, int i, int j);
static void blur_and_free_pixel(struct pic_info *info);

static void row_blur_picture(struct picture *pic);
static bool new_row_thread(pthread_t *thread, struct picture *pic,
                  struct picture *tmp, int j);
static void blur_and_free_row(struct pic_info *info);

static void col_blur_picture(struct picture *pic);
static bool new_col_thread(pthread_t *thread, struct picture *pic,
                  struct picture *tmp, int i);
static void blur_and_free_col(struct pic_info *info);

static void quarter_blur_picture(struct picture *pic);
static bool new_quarter_thread(pthread_t *thread, struct picture *pic,
                      struct picture *tmp, const int quarters[]);
static void blur_and_free_quarter(struct pic_info *info);

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

/* Takes in various partition implementation functions and returns the average 
   time taken for it to blur a provided image across 100 test cases. */
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

/* Sequentially blurs a picture. */
static void blur_picture(struct picture *pic){
  // make new temporary picture to work in
  struct picture tmp;
  init_picture_from_size(&tmp, pic->width, pic->height);

  // iterate over each pixel in the picture
  for(int i = 0 ; i < tmp.width; i++){
    for(int j = 0 ; j < tmp.height; j++){
    
      // set-up a local pixel on the stack
      struct pixel rgb = get_pixel(pic, i, j);
      
      // don't need to modify boundary pixels
      if(i != 0 && j != 0 && i != tmp.width - 1 && j != tmp.height - 1){
      
        // set up running RGB component totals for pixel region
        int sum_red = rgb.red;
        int sum_green = rgb.green;
        int sum_blue = rgb.blue;
    
        // check the surrounding pixel region
        for(int n = -1; n <= 1; n++){
          for(int m = -1; m <= 1; m++){
            if(n != 0 || m != 0){
              rgb = get_pixel(pic, i+n, j+m);
              sum_red += rgb.red;
              sum_green += rgb.green;
              sum_blue += rgb.blue;
            }
          }
        }
    
        // compute average pixel RGB value
        rgb.red = sum_red / BLUR_REGION_SIZE;
        rgb.green = sum_green / BLUR_REGION_SIZE;
        rgb.blue = sum_blue / BLUR_REGION_SIZE;
      }
    
      // set pixel to computed region RBG value (unmodified if boundary)
      set_pixel(&tmp, i, j, &rgb);
    }
  }
  
  // clean-up the old picture and replace with new picture
  clear_picture(pic);
  overwrite_picture(pic, &tmp);
}

/* Blur an individual pixel (and hence its surrounding area). */
static void blur_pixel(struct pic_info *info) {
  struct picture *pic = info->pic;
  struct picture *tmp = info->tmp;
  int i = info->i;
  int j = info->j;

  struct pixel pixel;  
  int total_red = 0;
  int total_green = 0;
  int total_blue = 0;

  // Apply loop to area around pixel to check and sum rgb values
  for(int n = -1; n <= 1; n++){
    for(int m = -1; m <= 1; m++){
      pixel = get_pixel(tmp, i + n, j + m);
      total_red += pixel.red;
      total_green += pixel.green;
      total_blue += pixel.blue;
    }
  }

  // Calculate average rgb values
  pixel.red = total_red / BLUR_REGION_SIZE;
  pixel.green = total_green / BLUR_REGION_SIZE;
  pixel.blue = total_blue / BLUR_REGION_SIZE;

  // Set average rgb values to complete pixel blur
  set_pixel(pic, i, j, &pixel);
}

/* =============THREAD CREATION FUNCTIONS============= */

/* Create a new thread according to specified parameters and passing in a
    pic_info as the argument for the thread. To be used for a pixel. */
static bool new_pixel_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i, int j) {
  struct pic_info *info = (struct pic_info*) malloc(sizeof(struct pic_info));

  // Assign values to new pic_info
  info->pic = pic;
  info->tmp = tmp;
  info->i = i;
  info->j = j;

  // Check that the new thread can be created properly and exit otherwise
  if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_pixel, info) != 0) {
    // Free resources
    free(info);
    return false;
  }
  return true;
}

/* Create a new thread according to specified parameters and passing in a
  pic_info as the argument for the thread. To be used for a row. */
static bool new_row_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int j) {
  struct pic_info *info = (struct pic_info*) malloc(sizeof(struct pic_info));

  // Assign values to new pic_info 
  info->pic = pic;
  info->tmp = tmp;
  info->j = j;

  // Check that the new thread can be created properly and exit otherwise
  if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_row, info) != 0) {
    // Free resources
    free(info);
    return false;
  }
  return true;
} 

/* Create a new thread according to specified parameters and passing in a
pic_info as the argument for the thread. To be used for a column. */
static bool new_col_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i) {
  struct pic_info *info = (struct pic_info*) malloc(sizeof(struct pic_info));

  // Assign values to new pic_info 
  info->pic = pic;
  info->tmp = tmp;
  info->i = i;

  // Check that the new thread can be created properly and exit otherwise
  if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_col, info) != 0) {
    // Free resources
    free(info);
    return false;
  }
  return true;
} 

static bool new_quarter_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, const int quarters[]) {
  struct pic_info *info = malloc(sizeof(struct pic_info));

  // Assign values to new pic_info 
  info->pic = pic;
  info->tmp = tmp;
  info->initi = quarters[0];
  info->initj = quarters[1];
  info->endi = quarters[2];
  info->endj = quarters[3];

  // Check that the new thread can be created properly and exit otherwise 
  if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_quarter, info) != 0) {
    // Free resources
    free(info);
    return false;
  }
  return true;
}


/* =============BLUR FUNCTIONS============= */

static void parallel_blur_picture(struct picture *pic){
  // make new temporary picture to work in
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;
  
  // Initialise thread pool
  struct t_pool pool;
  thread_pool_init(&pool);

  // Iterate over each pixel in the picture, except boundary pixels
  for(int i = 1; i < tmp.width - 1; i++){
    for(int j = 1; j < tmp.height - 1; j++){  
      pthread_t thread;

      // Check that a new thread thread can be created
      while(!new_pixel_thread(&thread, pic, &tmp, i, j)) {
        tryjoin_threads(&pool);
      }
      add_thread_to_pool(thread, &pool);
    }
  } 

  threads_join(&pool);   
  
  // clean-up the temporary picture
  clear_picture(&tmp);
}

/* Blurs the picture by creating a thread for every column. */
static void col_blur_picture(struct picture *pic) {
  struct picture tmp;
  tmp.img = copy_image(pic->img);
  tmp.width = pic->width;
  tmp.height = pic->height;

  // Initialise  thread pool.
  struct t_pool pool;
  thread_pool_init(&pool);

  // Iterate over each pixel in the picture, except boundary pixels
  for (int i = 1; i < tmp.width - 1; i++) {
      pthread_t thread;

      // Check that a new thread thread can be created
      while (!new_col_thread(&thread, pic, &tmp, i)) {
          tryjoin_threads(&pool);
      }
      add_thread_to_pool(thread, &pool);
  }
  threads_join(&pool);

  // clean-up the old picture and replace with new picture
  clear_picture(&tmp);
  overwrite_picture(pic, &tmp);
}

/* Blurs the picture by creating a thread for every row. */
static void row_blur_picture(struct picture *pic) {
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    // Initialise  thread pool.
    struct t_pool pool;
    thread_pool_init(&pool);

    // Iterate over each pixel in the picture, except boundary pixels
    for (int i = 1; i < tmp.height - 1; i++) {
        pthread_t thread;

        // Check that a new thread thread can be created
        while (!new_row_thread(&thread, pic, &tmp, i)) {
            tryjoin_threads(&pool);
        }
        add_thread_to_pool(thread, &pool);
    }
    threads_join(&pool);

    // clean-up the old picture and replace with new picture
    clear_picture(&tmp);
    overwrite_picture(pic, &tmp);
}

/* Blurs the picture by creating a thread for every quarter. */
static void quarter_blur_picture(struct picture *pic) {
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    // Get important points of the picture

    int mid_w = tmp.width / 2;
    int mid_h = tmp.height / 2;

    // Creates  arrays for each quarter with the relevant information
    int q1[4] = {0, 0, mid_w, mid_h};
    int q2[4] = {mid_w, 1, tmp.width - 1, mid_h};
    int q3[4] = {1, mid_h, mid_w, tmp.height - 1};
    int q4[4] = {mid_w, mid_h, tmp.width - 1, tmp.height - 1};

    int* quarters[NO_QUARTERS] = {q1, q2, q3, q4};

    // Initialise  thread pool.
    struct t_pool pool;
    thread_pool_init(&pool);

    // Iterate over each pixel in the picture, except boundary pixels
    for (int i = 0; i < NO_QUARTERS; i++) {
        pthread_t thread;

        // Check that a new thread thread can be created
        while (!new_quarter_thread(&thread, pic, &tmp, quarters[i])) {
            tryjoin_threads(&pool);
        }
        add_thread_to_pool(thread, &pool);
    }
    threads_join(&pool);

    // clean-up the old picture and replace with new picture
    clear_picture(&tmp);
    overwrite_picture(pic, &tmp);
}


/* =============BLUR AND FREE FUNCTIONS============= */

/* Blurs a pixel and then frees it. */
static void blur_and_free_pixel(struct pic_info *info) {
  blur_pixel(info);
  free(info);
}

/* Blurs a column and then frees it. */
static void blur_and_free_col(struct pic_info *info) {
  struct picture *tmp = info->tmp;

  // Blurs every individual pixel in the column
  for (int col = 1; col < tmp->height - 1; col++) {
      info->j = col;
      blur_pixel(info);
  }
  free(info);
}

/* Blurs a row and then frees it. */
static void blur_and_free_row(struct pic_info *info) {
  struct picture *tmp = info->tmp;

  // Blurs every individual pixel in the row
  for (int row = 1; row < tmp->width - 1; row++) {
      info->i = row;
      blur_pixel(info);
  }
  free(info);
}

/* Blurs a quarter and then frees it. */
static void blur_and_free_quarter(struct pic_info *info) {

  for (int i = info->initi; i < info->endi; i++) {
      for (int j = info->initj; j < info->endj; j++) {
          // Blurs every pixel in every quarter
          info->i = i;
          info->j = j;
          blur_pixel(info);
      }
  }
  free(info);
}