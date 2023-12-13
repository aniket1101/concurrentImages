#ifndef PICLIB_H
#define PICLIB_H

#include "Picture.h"
#include "Utils.h"
#include <pthread.h>

 struct pic_info {
  struct picture *pic;
  struct picture *tmp;
  int i;
  int j;
  int starti;
  int startj;
  int endi;
  int endj;
 };

  // picture transformation routines
  void invert_picture(struct picture *pic);
  void grayscale_picture(struct picture *pic);
  void rotate_picture(struct picture *pic, int angle);
  void flip_picture(struct picture *pic, char plane);
  void blur_picture(struct picture *pic);
  void blur_pixel(struct pic_info *info);

  // Parallelisation functions
  void parallel_blur_picture(struct picture *pic);

  void parallel_blur_picture(struct picture *pic);
  bool new_pixel_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i, int j);
  void blur_and_free_pixel(struct pic_info *info);

  void row_blur_picture(struct picture *pic);
  bool new_row_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int j);
  void blur_and_free_row(struct pic_info *info);

  void col_blur_picture(struct picture *pic);
  bool new_col_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i);
  void blur_and_free_col(struct pic_info *info);

  void quarter_blur_picture(struct picture *pic);
  bool new_quarter_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, const int quarters[]);
  void blur_and_free_quarter(struct pic_info *info);
#endif

