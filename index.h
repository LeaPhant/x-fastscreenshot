#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMAX(a,b) ((a) > (b) ? (a) : (b))

typedef struct image {
	int width;
	int height;
  int size;
	char * data;
	int depth;
	int bytes_per_line;
	int bits_per_pixel;
} Image ;
