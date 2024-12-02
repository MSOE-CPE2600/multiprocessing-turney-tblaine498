/// 
//  mandel.c
//  Based on example code found here:
//  https://users.cs.fiu.edu/~cpoellab/teaching/cop4610_fall22/project3.html
//
//  Converted to use jpg instead of BMP and other minor changes
//  
///
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>
#include "jpegrw.h"

#define NUM_FRAMES 50

// local routines
static int iteration_to_color( int i, int max );
static int iterations_at_point( double x, double y, int max );
static void *compute_image(void *arg);
static void show_help();

typedef struct thread_args {
	imgRawImage* img;
	double xmin;
	double xmax;
	double ymin;
	double ymax;
	int max;
} thread_args;

int main( int argc, char *argv[] )
{
	char c;

	// These are the default configuration values used
	// if no command line arguments are given.
	const char *base_name = "mandel"; 
    const char *extension = ".jpg"; 
    char outfile[15];
	double xcenter = 0;
	double ycenter = 0;
	double xscale = 4;
	double yscale = 0; // calc later
	int    image_width = 1000;
	int    image_height = 1000;
	int    max = 1000;
	int    num_children = 1;
	int    num_threads = 1;

	// For each command line argument given,
	// override the appropriate configuration value.

	while((c = getopt(argc,argv,"t:c:x:y:s:W:H:m:o:h"))!=-1) {
		switch(c) 
		{
			case 't':
				num_threads = atoi(optarg);
				break;
			case 'c':
				num_children = atoi(optarg);
				break;
			case 'x':
				xcenter = atof(optarg);
				break;
			case 'y':
				ycenter = atof(optarg);
				break;
			case 's':
				xscale = atof(optarg);
				break;
			case 'W':
				image_width = atoi(optarg);
				break;
			case 'H':
				image_height = atoi(optarg);
				break;
			case 'm':
				max = atoi(optarg);
				break;
			case 'o':
				base_name = optarg;
				break;
			case 'h':
				show_help();
				exit(1);
				break;
		}
	}

    // Split the iterations among multiple processes
    int iterations_per_process = NUM_FRAMES / num_children;
	int remainder = NUM_FRAMES % num_children;  // Remainder to distribute
	
	for (int p = 0; p < num_children; p++)
	{
		pid_t pid = fork();

		// Check to see if fork worked
        if (pid < 0) 
		{
            perror("fork");
            exit(1);
        }

        if (pid == 0)
		{
			// Calculate the start and end indices for each process
			int extra_iterations = 0;
			if (p < remainder)
			{
				extra_iterations = p;
			} 
			else 
			{
				extra_iterations = remainder;
			}

			int start = p * iterations_per_process + extra_iterations + 1;  // Start index with extra iterations
			int end = start + iterations_per_process - 1;  // Default end index based on iterations per process

			// Add one more iteration for processes with remainder
			if (p < remainder)
			{
				end++;
			}

			// Update xscale so processes starts at proper point
			for (int i = 0; i < start; i++)
			{
				xscale *= 1.1;
			}

            // Iterate over the image range
            for (int i = start; i <= end; i++)
			{
                snprintf(outfile, sizeof(outfile), "%s%02d%s", base_name, i, extension);

                // Calculate y scale based on x scale (settable) and image sizes in X and Y
                yscale = xscale / image_width * image_height;

                // Display the configuration of the image.
                printf("mandel: x=%lf y=%lf xscale=%lf yscale=%lf max=%d outfile=%s\n",xcenter,ycenter,xscale,yscale,max,outfile);

                // Create a raw image of the appropriate size.
                imgRawImage* img = initRawImage(image_width, image_height);

                // Fill it with black
                setImageCOLOR(img, 0);

				pthread_t threads[num_threads];
				pthread_t threads_ids[num_threads];
				thread_args params;
				params.img = img;
				params.xmin = xcenter - xscale / 2;
				params.xmax = xcenter + xscale / 2;
				params.ymin = ycenter - yscale / 2;
				params.ymax = ycenter + yscale / 2;
				params.max = max;

				for (int t = 0; t < num_threads; t++)
				{
					threads_ids[num_threads] = t;
					pthread_create(&threads[t], NULL, compute_image, &params);
				}

				for (int i = 0; i < num_threads; i++)
				{
					pthread_join(threads[i], NULL);
				}
                // Compute the Mandelbrot image
                // compute_image(img, xcenter - xscale / 2, xcenter + xscale / 2, 
                //               ycenter - yscale / 2, ycenter + yscale / 2, max);

                // Save the image in the stated file.
                storeJpegImageFile(img, outfile);

                // Free the mallocs
                freeRawImage(img);

				// Changes the scale for each new image
				xscale *= 1.1;
            }
            exit(0);  // Exit the child process after completing its work
        }
    }

    // Waits for children to finish
    for (int p = 0; p < num_children; p++)
	{
        wait(NULL);
    }

    return 0;
}

/*
Return the number of iterations at point x, y
in the Mandelbrot space, up to a maximum of max.
*/

int iterations_at_point( double x, double y, int max )
{
	double x0 = x;
	double y0 = y;

	int iter = 0;

	while( (x*x + y*y <= 4) && iter < max ) {

		double xt = x*x - y*y + x0;
		double yt = 2*x*y + y0;

		x = xt;
		y = yt;

		iter++;
	}

	return iter;
}

/*
Compute an entire Mandelbrot image, writing each point to the given bitmap.
Scale the image to the range (xmin-xmax,ymin-ymax), limiting iterations to "max".

NOTE: the asterisk before compute_image allows it to be based in as a parameter the pthread_create 
*/
void *compute_image(void *arg)
{
	/*
		*args is a void pointer, the compiler only knows it points to some address
		by doing the line below it creates a new pointer to the same address but this time
		specifiy that it is of struct thread_arg type.
	*/
	thread_args *args = (thread_args *)arg;

	int i,j;

	int width = args->img->width;
	int height = args->img->height;

	// For every pixel in the image...

	for(j=0;j<height;j++) {

		for(i=0;i<width;i++) {

			// Determine the point in x,y space for that pixel.
			double x = args->xmin + i*(args->xmax-args->xmin)/width;
			double y = args->ymin + j*(args->ymax-args->ymin)/height;

			// Compute the iterations at that point.
			int iters = iterations_at_point(x,y,args->max);

			// Set the pixel in the bitmap.
			setPixelCOLOR(args->img,i,j,iteration_to_color(iters,args->max));
		}
	}
	return NULL;
}

/*
Convert a iteration number to a color.
Here, we just scale to gray with a maximum of imax.
Modify this function to make more interesting colors.
*/
int iteration_to_color( int iters, int max )
{
	int color = 0xFFFFFF*iters/(double)max;
	return color;
}


// Show help message
void show_help()
{
	printf("Use: mandel [options]\n");
	printf("Where options are:\n");
	printf("-m <max>      The maximum number of iterations per point. (default=1000)\n");
	printf("-x <coord>    X coordinate of image center point. (default=0)\n");
	printf("-y <coord>    Y coordinate of image center point. (default=0)\n");
	printf("-s <scale>    Scale of the image in Mandlebrot coordinates (X-axis). (default=4)\n");
	printf("-W <pixels>   Width of the image in pixels. (default=1000)\n");
	printf("-H <pixels>   Height of the image in pixels. (default=1000)\n");
	printf("-o <file>     Set output file. (default=mandel.bmp)\n");
	printf("-c <children> Number of children used to create the 50 frames for the mandel movie. (default=1)\n");
	printf("-t <threads>  Number of threads used to create a single image. (default=1)\n");
	printf("-h          Show this help text.\n");
	printf("\nSome examples are:\n");
	printf("mandel -x -0.5 -y -0.5 -s 0.2\n");
	printf("mandel -x -.38 -y -.665 -s .05 -m 100\n");
	printf("mandel -x 0.286932 -y 0.014287 -s .0005 -m 1000\n\n");
}
