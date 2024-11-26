# System Programming Lab 11 Multiprocessing

### Overview of Program
The edited mandel.c program creates a sequence of images of the Mandelbrot set. It takes in a bunch
of parameters to change the set. One of the parameters determines the number of processes to create the image
set. This is done by forking a number of times to get multiple processes to speed up the program.

### Graph of Results
![Graph Screenshot](CPE2600_lab11.png)

### Discussion of Results
The graph shows what looks like an exponential decay. As the number of processes increases the time it takes to 
complete the set decreases. But like previously mentioned the decay is exponential as it decreases most by adding a few extra processes but there seems to be diminishing returns as more processes are added. 