#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define MAX(a,b) ((a) > (b) ? a : b)
#define MIN(a,b) ((a) < (b) ? a : b)

/*
 * Cluster 
 */
typedef struct {
	uint32_t num_points;
	uint8_t r, g, b;
	uint32_t mean_r, mean_g, mean_b;
} cluster;

/*
 * RGB (pixel)
 */
typedef struct {
    	uint8_t b, g, r;
} rgb;

/*
 * Image 
 */
typedef struct {
	uint32_t length;
	uint32_t width;
	uint32_t height;
	uint8_t header[54];
	
  rgb* pixels;
	
  FILE* fp;
} image;


/*
 * Read File function
 * 
 * mImage->pixels = malloc(mImage->length * sizeof(rgb));
 *
 */
int read_file(int argc, char *argv[], image* mImage)
{
	if((mImage->fp=fopen(argv[3], "r")) == NULL) {
		printf("Error opening BMP.\n");
		exit(6);
	}
	if(fseek(mImage->fp, 18, SEEK_SET) != 0) {
		printf("Error fseek.\n");
		exit(6);
	}
	if(fread(&mImage->width, sizeof(mImage->width), 1, mImage->fp) != 1) {
		printf("Error Writing File.\n");
		exit(6);
	}
	if(fseek(mImage->fp, 22, SEEK_SET) != 0) {
		printf("Error fseek.\n");
		exit(6);
	}
	if(fread(&mImage->height, sizeof(mImage->height), 1, mImage->fp) != 1) {
		printf("Error Reading H File.\n");
		exit(6);
	}
 
	mImage->length = (mImage->width * mImage->height);
 
	mImage->pixels  = malloc(mImage->length * sizeof(rgb));

	if(mImage->pixels == NULL){
		printf("Not enough memory.\n");
		return 6;
	}

	if(fseek(mImage->fp, 0, SEEK_SET) != 0){
		printf("Error fseek.\n");
		return 6;
	}
	if(fread(&mImage->header, sizeof(uint8_t), sizeof(mImage->header), mImage->fp) != sizeof(mImage->header)){
		printf("Error reading header.\n");
		return 6;
	}

	int i,c;
 
	for(i = 0; i < mImage->length; i++){
		c = getc(mImage->fp);
		if(c == EOF){
			printf("Error Reading File.\n");
			return 5;
		}
		mImage->pixels[i].b = c;
		c = getc(mImage->fp);
		if(c == EOF){
			printf("Error Reading File.\n");
			return 5;
		}
		mImage->pixels[i].g = c;
		c = getc(mImage->fp);
		if(c == EOF){
			printf("Error Reading File.\n");
			return 5;
		}
		mImage->pixels[i].r = c;
	}
 
	if(fclose(mImage->fp) != 0){
		printf("Error closing File.\n");
		return 1;
	}
	return (0);
}

/*
 * Compute Checksum
 * Checksum is the total sum of the product (r,g,b) of the centroids final value.
 *
 */
uint32_t getChecksum(cluster* centroids, uint8_t k){
  uint32_t i,j;
  uint32_t sum = 0;
  
  for (i = 0; i < k; i++)
  {
    printf("Centroide %u : R[%u]\tG[%u]\tB[%u]\n", i, centroids[i].r, centroids[i].g, centroids[i].b);
    sum += (centroids[i].r * centroids[i].g * centroids[i].b);
  }
  return sum;
}