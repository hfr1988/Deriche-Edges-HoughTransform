//
// Created by Arjun on 2/20/17.
//

#ifndef HOUGHTRANSFORM_HOUGH_H
#define HOUGHTRANSFORM_HOUGH_H

#define MAX_BRIGHTNESS 255.0
#define MIN_BRIGHTNESS 0.0

#define HOUGH_THRESHOLD 1

#define NORMALIZE_ALPHA 1

#define VERBOSE 0

#include <math.h>
#include "bmp_clean.h"
#include "helper.h"


void performHough(size_t x, size_t y, double brightness, size_t diag_len,  double *accumulator) {

    int angle;
    for (angle = -90; angle < 90; angle++)
    {
        const double theta = (M_PI * angle) / 180.0;
        const double rho = (x * cos(theta)) + (y * sin(theta));

        const size_t rho_offset = (size_t) ((round(rho + diag_len) * 180.0));
        const size_t angle_offset = angle + 90;

        *(accumulator + rho_offset + angle_offset) += (1);

        if(VERBOSE) printf("rho: %f, theta :%d, rho_offset:%lu, angle_offset:%lu, acc:%f\n", rho, angle, rho_offset, angle_offset, *(accumulator + rho_offset + angle_offset));
    }
}

Mat houghtransform(Mat *grayscale_img) {

    size_t i;

    /* get image size parameters */
    const size_t img_width = grayscale_img->width;
    const size_t img_height = grayscale_img->height;
    const size_t img_pixel_count = img_height * img_width;

    /* calculate diagonal length for hough-transformed image */
    const size_t diagonal_length = (size_t) hypot(img_height, img_width);

    /* initialize output matrix with parameters */
    const size_t width  = 180;
    const size_t height = 2 * diagonal_length;

    printf("hough image length:%lu, %lu\n", diagonal_length, width);

    // create matrix, pass 1 to initialize it as empty
    Mat houghMatrix = createMatrix(width, height, 1);

    /* Read read input image pixel values, and perform hough transform on each value that exceeds threshold */
    for (i = 0; i < img_pixel_count; i++)
    {
        const double pixel_value = grayscale_img->data[i];
        printf("pixel_value:%f\n", pixel_value);
        if (pixel_value > HOUGH_THRESHOLD)
        {
            const size_t y = (i / img_width);
            const size_t x = (i % img_width);
            performHough(x, y, (MAX_BRIGHTNESS-pixel_value), diagonal_length, houghMatrix.data);
        }
    }
    return houghMatrix;
}

//Mat color2gray(Bitmap *image) {
//
//    size_t i;
//    const size_t width = image->width;
//    const size_t height = image->height;
//
//    double most_bright_pixel = MIN_BRIGHTNESS;
//    const size_t pixel_count = (width * height);
//
//    Mat grayImage = createMatrix(width, height, 0);
//
//    /* calculate brightness for all pixels & find the brightest pixel */
//    for(i = 0; i < pixel_count; i++)
//    {
//        size_t idx = 3*i;
//        const double R = image->data[idx++];  // 3i + 0
//        const double G = image->data[idx++];  // 3i + 1
//        const double B = image->data[idx];    // 3i + 2
//
//        /* store image magnitude in gray image mat */
//        grayImage.data[i] = sqrt((R*R) + (G*G) + (B*B));
//
//        /* find the brightest pixel */
//        most_bright_pixel = (grayImage.data[i] > most_bright_pixel) ? grayImage.data[i] : most_bright_pixel;
//    }
//
//    /** calculate normalization factor alpha **/
//    if(NORMALIZE_ALPHA)
//    {
//        const double ALPHA = (MAX_BRIGHTNESS / most_bright_pixel);
//        for (i = 0; i < pixel_count; i++)
//        {
//            grayImage.data[i] = MAX_BRIGHTNESS - (grayImage.data[i] * ALPHA);
//            if(VERBOSE) printf("gray:%f\n", grayImage.data[i]);
//        }
//    }
//
//    return grayImage;
//}
Mat color2gray(unsigned char *image_data) {

    Mat grayImage = createMatrix((size_t) InfoHeader.Width, (size_t) InfoHeader.Height, 0);
    const size_t pixel_count = grayImage.width * grayImage.height;
    double most_bright_pixel = 0.0;

    /* calculate brightness for all pixels & find the brightest pixel */
    size_t i;
    for(i = 0; i < pixel_count; i++)
    {
        const size_t idx = 3*i;
        const double R = (double)((unsigned char) image_data[idx]);     // 3i + 0
        const double G = (double)((unsigned char) image_data[idx+1]); // 3i + 1
        const double B = (double)((unsigned char) image_data[idx+2]); // 3i + 2

        /* store image magnitude in gray image mat */
        grayImage.data[i] = sqrt((R*R) + (G*G) + (B*B));

        /* find the brightest pixel */
        most_bright_pixel = (grayImage.data[i] > most_bright_pixel) ? grayImage.data[i] : most_bright_pixel;
    }

    /** calculate normalization factor alpha **/
    if(NORMALIZE_ALPHA)
    {
        const double ALPHA = (MAX_BRIGHTNESS / most_bright_pixel);
        for (i = 0; i < pixel_count; i++)
        {
            grayImage.data[i] = MAX_BRIGHTNESS - (grayImage.data[i] * ALPHA);
            if(VERBOSE) printf("gray:%f\n", grayImage.data[i]);
        }
    }

    return grayImage;
}

int cmpfunc (const void * a, const void * b)
{
//    printf("a:%f, b:%f\n", *(double*)a, *(double*)b);
    return (int)( *(double*)a - *(double*)b );
}

typedef struct {
    size_t pos;
    double val;
} Peak;

double getKernelMean(double *data, size_t offset, const size_t width, const size_t pixel_count, const size_t kernel_size) {

    //printf("running get kernel mean!\n");
    int x, y;

    int hlf_krnl = (int) kernel_size/2;
    double mean = 0.0;

    Peak localPeak;
    localPeak.val = 0;

    size_t count = 0;

    for(x = -1 * hlf_krnl; x <= hlf_krnl; x++)
    {
        for(y = -1 * hlf_krnl; y <= hlf_krnl; y++)
        {
            if(offset + x + (width*y) >= pixel_count)
            {
                continue;
            }

            count++;
            const double val = *(data + offset + x + (width*y));

            mean += val;

            if(val > localPeak.val)
            {
                localPeak.val = val;
                localPeak.pos = offset + x + (width*y);
            }

        }
    }
    mean /= count;

    if(localPeak.val > 10 * mean && localPeak.val > 25 && localPeak.val !=255.0) {
        printf("offset:%lu, mean:%f, count:%lu, max:%f\n", offset, mean, count, localPeak.val);
        *(localPeak.pos + data) = 255.0;
    }

    //printf("x:%d, y:%d, val:%f\n", localPeak.x, localPeak.y, localPeak.val);
    return mean;
}

int detect_polygon_edge_count(Mat *image){

    const size_t height = image->height;
    const size_t width = image->width;
    const double *data = image->data;

    const size_t pixel_count = width * height;
    const size_t kernel_size = 20;//(size_t) sqrt(pixel_count) / 10;
    const size_t hlf_krnl = kernel_size/2;
    size_t x, y;

    printf("Since pixel_count is %lu, kernel size is :%lu\n", pixel_count, kernel_size);

    for(x = hlf_krnl; x < (width); x+=kernel_size)
    {
        for(y = hlf_krnl; y < (height); y+=kernel_size)
        {
            const size_t idx = (y * width) + x;
            const double brightness = image->data[idx];
            const double max = getKernelMean(image->data, idx, width, pixel_count, kernel_size);

            //printf("kernal val:%f\n", max);
//            if (brightness >= (4 * mean) && mean != 0.0)
//            {
//
//                ///image->data[idx] = 255.0;
//            }
//            else
//                image->data[idx] = 0.0;

        }
    }
    //qsort(image->data, pixel_count, sizeof(double), cmpfunc);
    //double median = image->data[0];

    const double mean = 0.0;//sum/(pixel_count);
    int count = 0;
    for(x = 0; x < pixel_count; x++)
    {
        count = (image->data[x] > mean * 51.0) ? count+1 : count;
    }
    //printf("max:%f, sum:%f, mean:%f, median:%f, count:%d\n", max, sum, mean, median, count);
    return count;
}
double detect_polygon_orientation(Mat *image){
    return -1;
}
#endif //HOUGHTRANSFORM_HOUGH_H
//max = (image->data[idx] > max) ? image->data[idx] : max;
//sum += (image->data[idx]);
//printf("x:%lu, y:%lu, idx:%lu, mag:%f\n", x,y,idx,image->data[idx]);
