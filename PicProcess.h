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
  int initi;
  int initj;
  int endi;
  int endj;
 };

  // picture transformation routines
  void invert_picture(struct picture *pic);
  void grayscale_picture(struct picture *pic);
  void rotate_picture(struct picture *pic, int angle);
  void flip_picture(struct picture *pic, char plane);
#endif

