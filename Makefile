SRCS = tmain.c nurbs.c render.c ../j4cDAC/driver/libetherdream/etherdream.c
CFLAGS = -std=c99 -Wall -I../j4cDAC/driver/libetherdream -I../j4cDAC/common

reticulate: $(SRCS)
	clang $(CFLAGS) $^ -o $@

clean:
	rm -f reticulate
