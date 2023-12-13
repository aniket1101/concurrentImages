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
 };

  // picture transformation routines
  void invert_picture(struct picture *pic);
  void grayscale_picture(struct picture *pic);
  void rotate_picture(struct picture *pic, int angle);
  void flip_picture(struct picture *pic, char plane);
  void blur_picture(struct picture *pic);
  void parallel_blur_picture(struct picture *pic);

  // Parallelisation functions
  bool new_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i, int j);
  void blur_pixel(struct pic_info *info);
#endif

