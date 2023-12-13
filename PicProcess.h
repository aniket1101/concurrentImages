#ifndef PICLIB_H
#define PICLIB_H

#include "Picture.h"
#include "Utils.h"
#include <pthread.h>

/* Structure used to pass additional information about the picture as an argument into pthread_create(). */
struct picture_information {
  struct picture *picture;
  struct picture *tmp;
  int i;                    /* I coordinate that will be changed in blur pixel. */
  int j;                    /* J coordinate that will be changed in blur pixel. */
  int starti;               /* Starting i coordinate for when blurring by quarter. */
  int startj;               /* Starting j coordinate for when blurring by quarter. */
  int endi;                 /* End i coordinate for when blurring by quarter. */
  int endj;                 /* End j coordinate for when blurring by quarter. */
};

// picture transformation routines
void invert_picture(struct picture *pic);
void grayscale_picture(struct picture *pic);
void rotate_picture(struct picture *pic, int angle);
void flip_picture(struct picture *pic, char plane);
void blur_picture(struct picture *pic);
void blur_pixel(struct picture_information *inf);

void parallel_blur_picture(struct picture *pic);
bool new_individual_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i, int j);
void blur_and_free_pixel(struct picture_information *inf);

void blur_picture_by_col(struct picture *pic);
bool new_col_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i);
void blur_and_free_col(struct picture_information *inf);

void blur_picture_by_row(struct picture *pic);
bool new_row_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int j);
void blur_and_free_row(struct picture_information *inf);

void blur_picture_by_quarter(struct picture *pic);
bool new_quarter_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, const int quarter[]);
void blur_and_free_quarter(struct picture_information *inf);
#endif


