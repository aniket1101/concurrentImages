#include "PicProcess.h"
#include "ThreadPool.h"

#define NO_RGB_COMPONENTS 3
#define BLUR_REGION_SIZE 9
#define NO_quarterS 4


void invert_picture(struct picture *pic) {
    // iterate over each pixel in the picture
    for (int i = 0; i < pic->width; i++) {
        for (int j = 0; j < pic->height; j++) {
            struct pixel rgb = get_pixel(pic, i, j);

            // invert RGB values of pixel
            rgb.red = MAX_PIXEL_INTENSITY - rgb.red;
            rgb.green = MAX_PIXEL_INTENSITY - rgb.green;
            rgb.blue = MAX_PIXEL_INTENSITY - rgb.blue;

            // set pixel to inverted RBG values
            set_pixel(pic, i, j, &rgb);
        }
    }
}

void grayscale_picture(struct picture *pic) {
    // iterate over each pixel in the picture
    for (int i = 0; i < pic->width; i++) {
        for (int j = 0; j < pic->height; j++) {
            struct pixel rgb = get_pixel(pic, i, j);

            // compute gray average of pixel's RGB values
            int avg = (rgb.red + rgb.green + rgb.blue) / NO_RGB_COMPONENTS;
            rgb.red = avg;
            rgb.green = avg;
            rgb.blue = avg;

            // set pixel to gray-scale RBG value
            set_pixel(pic, i, j, &rgb);
        }
    }
}

void rotate_picture(struct picture *pic, int angle) {
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    int new_width = tmp.width;
    int new_height = tmp.height;

    // adjust output picture size as necessary
    if (angle == 90 || angle == 270) {
        new_width = tmp.height;
        new_height = tmp.width;
    }
    clear_picture(pic);
    init_picture_from_size(pic, new_width, new_height);

    // iterate over each pixel in the picture
    for (int i = 0; i < new_width; i++) {
        for (int j = 0; j < new_height; j++) {
            struct pixel rgb;
            // determine rotation angle and execute corresponding pixel update
            switch (angle) {
                case (90):
                    rgb = get_pixel(&tmp, j, new_width - 1 - i);
                    break;
                case (180):
                    rgb = get_pixel(&tmp, new_width - 1 - i, new_height - 1 - j);
                    break;
                case (270):
                    rgb = get_pixel(&tmp, new_height - 1 - j, i);
                    break;
                default:
                    printf("[!] rotate is undefined for angle %i (must be 90, 180 or 270)\n", angle);
                    clear_picture(&tmp);
                    exit(IO_ERROR);
            }
            set_pixel(pic, i, j, &rgb);
        }
    }

    // temporary picture clean-up
    clear_picture(&tmp);
}

void flip_picture(struct picture *pic, char plane) {
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    // iterate over each pixel in the picture
    for (int i = 0; i < tmp.width; i++) {
        for (int j = 0; j < tmp.height; j++) {
            struct pixel rgb;
            // determine flip plane and execute corresponding pixel update
            switch (plane) {
                case ('V'):
                    rgb = get_pixel(&tmp, i, tmp.height - 1 - j);
                    break;
                case ('H'):
                    rgb = get_pixel(&tmp, tmp.width - 1 - i, j);
                    break;
                default:
                    printf("[!] flip is undefined for plane %c\n", plane);
                    clear_picture(&tmp);
                    exit(IO_ERROR);
            }
            set_pixel(pic, i, j, &rgb);
        }
    }

    // temporary picture clean-up
    clear_picture(&tmp);
}

void blur_picture(struct picture *pic) {
    // make temporary copy of picture to work from
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    // iterate over each pixel in the picture (ignoring boundary pixels)
    for (int i = 1; i < tmp.width - 1; i++) {
        for (int j = 1; j < tmp.height - 1; j++) {

            // set-up a local pixel on the stack
            struct pixel rgb;
            int sum_red = 0;
            int sum_green = 0;
            int sum_blue = 0;

            // check the surrounding pixel region
            for (int n = -1; n <= 1; n++) {
                for (int m = -1; m <= 1; m++) {
                    rgb = get_pixel(&tmp, i + n, j + m);
                    sum_red += rgb.red;
                    sum_green += rgb.green;
                    sum_blue += rgb.blue;
                }
            }

            // compute average pixel RGB value
            rgb.red = sum_red / BLUR_REGION_SIZE;
            rgb.green = sum_green / BLUR_REGION_SIZE;
            rgb.blue = sum_blue / BLUR_REGION_SIZE;

            // set pixel to region average RBG value
            set_pixel(pic, i, j, &rgb);
        }
    }

    // temporary picture clean-up
    clear_picture(&tmp);
}

/* Takes in a picture_information structure and blurs the pixel at the given i and j coordinates. */
void blur_pixel(struct picture_information *inf) {
    struct picture *pic = inf->picture;
    struct picture *tmp = inf->tmp;
    int i = inf->i;
    int j = inf->j;

    struct pixel rgb;
    int sum_red = 0;
    int sum_green = 0;
    int sum_blue = 0;

    /* check the surrounding pixel region */
    for (int n = -1; n <= 1; n++) {
        for (int m = -1; m <= 1; m++) {
            rgb = get_pixel(tmp, i + n, j + m);
            sum_red += rgb.red;
            sum_green += rgb.green;
            sum_blue += rgb.blue;
        }
    }

    /* compute average pixel RGB value */
    rgb.red = sum_red / BLUR_REGION_SIZE;
    rgb.green = sum_green / BLUR_REGION_SIZE;
    rgb.blue = sum_blue / BLUR_REGION_SIZE;

    /* set pixel to region average RBG value */
    set_pixel(pic, i, j, &rgb);
}

/* Blurs the picture by creating a thread for every pixel. */
void parallel_blur_picture(struct picture *pic) {
    /* make temporary copy of picture to work from */
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    /* Initialise new thread pool. */
    struct thread_pool thread_pool;
    initialise_thread_pool(&thread_pool);

    /* iterate over each pixel in the picture (ignoring boundary pixels) */
    for (int i = 1; i < tmp.width - 1; i++) {
        for (int j = 1; j < tmp.height - 1; j++) {
            pthread_t thread;
            /* Creates a new thread and will keep attempting to by calling try_join_threads which removes any terminated threads. */
            while (!new_individual_thread(&thread, pic, &tmp, i, j)) {
                try_join_threads(&thread_pool);
            }
            add_to_thread_pool(&thread_pool, thread);
        }
    }
    join_threads(&thread_pool);

    /* Temporary picture clean-up */
    clear_picture(&tmp);
}

/* Creates a new thread which then runs blur_and_free_pixel with the picture_information structure also created. */
bool new_individual_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i, int j) {
    struct picture_information *inf = malloc(sizeof(struct picture_information));
    inf->picture = pic;
    inf->tmp = tmp;
    inf->i = i;
    inf->j = j;

    int ans = pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_pixel, inf);
    /* If an error is returned then free the structure and return false. */
    if (ans != 0) {
        free(inf);
        return false;
    }
    return true;
}
/* Blurs the pixel and then frees the structure. */
void blur_and_free_pixel(struct picture_information *inf) {
    blur_pixel(inf);
    free(inf);
}

/* Blurs the picture by creating a thread for every column. */
void blur_picture_by_col(struct picture *pic) {
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    /* Initialise new thread pool. */
    struct thread_pool thread_pool;
    initialise_thread_pool(&thread_pool);

    /* iterate over each pixel in the picture (ignoring boundary pixels) */
    for (int i = 1; i < tmp.width - 1; i++) {
        pthread_t thread;
        /* Creates a new thread and will keep attempting to by calling try_join_threads which removes any terminated threads. */
        while (!new_col_thread(&thread, pic, &tmp, i)) {
            try_join_threads(&thread_pool);
        }
        add_to_thread_pool(&thread_pool, thread);
    }
    join_threads(&thread_pool);

    /* Temporary picture clean-up. */
    clear_picture(&tmp);
}

/* Creates a new thread which then runs blur_and_free_col with the picture_information structure also created. */
bool new_col_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int i) {
    struct picture_information *inf = malloc(sizeof(struct picture_information));
    inf->picture = pic;
    inf->tmp = tmp;
    inf->i = i;

    if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_col, inf) != 0) {
        /* If an error is returned then free the structure and return false. */
        free(inf);
        return false;
    }
    return true;
}

/* Blurs the column and then frees the structure. */
void blur_and_free_col(struct picture_information *inf) {
    struct picture *tmp = inf->tmp;

    /* Calls blur_pixel on every individual pixel in the column. */
    for (int j = 1; j < tmp->height - 1; j++) {
        inf->j = j;
        blur_pixel(inf);
    }
    free(inf);
}

/* Blurs the picture by creating a thread for every row. */
void blur_picture_by_row(struct picture *pic) {
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    /* Initialise new thread pool. */
    struct thread_pool thread_pool;
    initialise_thread_pool(&thread_pool);

    /* Iterate over each pixel in the picture (ignoring boundary pixels). */
    for (int j = 1; j < tmp.height - 1; j++) {
        pthread_t thread;
        /* Creates a new thread and will keep attempting to by calling try_join_threads which removes any terminated threads. */
        while (!new_row_thread(&thread, pic, &tmp, j)) {
            try_join_threads(&thread_pool);
        }
        add_to_thread_pool(&thread_pool, thread);
    }
    join_threads(&thread_pool);

    /* Temporary picture clean-up. */
    clear_picture(&tmp);
}

/* Creates a new thread which then runs blur_and_free_row with the picture_information structure also created. */
bool new_row_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, int j) {
    struct picture_information *inf = malloc(sizeof(struct picture_information));
    inf->picture = pic;
    inf->tmp = tmp;
    inf->j = j;

    if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_row, inf) != 0) {
        /* If an error is returned then free the structure and return false. */
        free(inf);
        return false;
    }
    return true;
}

/* Blurs the row and then frees the structure. */
void blur_and_free_row(struct picture_information *inf) {
    struct picture *tmp = inf->tmp;
    /* Calls blur_pixel on each individual pixel in the row. */
    for (int i = 1; i < tmp->width - 1; i++) {
        inf->i = i;
        blur_pixel(inf);
    }
    free(inf);
}

/* Blurs the picture by creating a thread for each of the four quarters. */
void blur_picture_by_quarter(struct picture *pic) {
    struct picture tmp;
    tmp.img = copy_image(pic->img);
    tmp.width = pic->width;
    tmp.height = pic->height;

    /* Initialise new thread pool. */
    struct thread_pool thread_pool;
    initialise_thread_pool(&thread_pool);

    /* Midpoints of the picture. */
    int mid_width = tmp.width / 2;
    int mid_height = tmp.height / 2;

    /* Creates four arrays of each of the four quarters with their starting and ending i and j coordinates. */
    int tl[4] = {0, 0, mid_width, mid_height};
    int tr[4] = {mid_width, 1, tmp.width - 1, mid_height};
    int bl[4] = {1, mid_height, mid_width, tmp.height - 1};
    int br[4] = {mid_width, mid_height, tmp.width - 1, tmp.height - 1};

    int* quarters[NO_quarterS] = {tl, tr, bl, br};

    /* Iterate over each pixel in the picture (ignoring boundary pixels). */
    for (int i = 0; i < NO_quarterS; i++) {
        pthread_t thread;
        /* Creates a new thread and will keep attempting to by calling try_join_threads which removes any terminated threads. */
        while (!new_quarter_thread(&thread, pic, &tmp, quarters[i])) {
            try_join_threads(&thread_pool);
        }
        add_to_thread_pool(&thread_pool, thread);
    }
    join_threads(&thread_pool);

    /* Temporary picture clean-up. */
    clear_picture(&tmp);
}

/* Creates a new thread which then runs blur_and_free_quarter with the picture_information structure also created. */
bool new_quarter_thread(pthread_t *thread, struct picture *pic, struct picture *tmp, const int quarter[]) {
    struct picture_information *inf = malloc(sizeof(struct picture_information));
    inf->picture = pic;
    inf->tmp = tmp;
    /* Sets the starting and ending i and j coordinates for the thread's picture_information. */
    inf->starti = quarter[0];
    inf->startj = quarter[1];
    inf->endi = quarter[2];
    inf->endj = quarter[3];

    if (pthread_create(thread, NULL, (void *(*)(void *)) blur_and_free_quarter, inf) != 0) {
        free(inf);
        return false;
    }
    return true;
}

/* Blurs the quarter and then frees the structure. */
void blur_and_free_quarter(struct picture_information *inf) {
    for (int i = inf->starti; i < inf->endi; i++) {
        for (int j = inf->startj; j < inf->endj; j++) {
            /* Calls blur_pixel on every individual pixel within the quarter. */
            inf->i = i;
            inf->j = j;
            blur_pixel(inf);
        }
    }
    free(inf);
}
