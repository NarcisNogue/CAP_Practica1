#include "kmeanslib.c"
#include <omp.h>

/*
 * Return the index number of the closest centroid to a given pixel (p)
 * input @param: p 				--> pixel pointer
 * input @param: centroids 		--> cluster array pointer
 * input @param: num_clusters	--> amount of centroids in @param: centroids
 *
 * output: uint8_t --> Index of the closest centroid
 *
 */
uint8_t find_closest_centroid(rgb* p, cluster* centroids, uint8_t num_clusters){
	uint32_t min = UINT32_MAX;
	uint32_t dis;
	uint8_t closest = 0, j;
	int16_t diffR, diffG, diffB;	

	for(j = 0; j < num_clusters; j++) 
	{
		diffR = centroids[j].r - p->r;
		diffG = centroids[j].g - p->g;
		diffB = centroids[j].b - p->b;
		// No sqrt required.
		dis = diffR*diffR + diffG*diffG + diffB*diffB;
		
		if(dis < min) 
		{
			min = dis;
			closest = j;
		}
	}
	return closest;
}

/*
 * Main function k-means	
 * input @param: K 			--> number of clusters
 * input @param: centroids --> the centroids
 * input @param: num_pixels --> number of total pixels
 * input @param: pixels 	--> pinter to array of rgb (pixels)
 */
void kmeans(uint8_t k, cluster* centroids, uint32_t num_pixels, rgb* pixels){	
	uint8_t condition, changed, closest;
	uint32_t i, j, random_num;
	
	printf("STEP 1: K = %d\n", k);
	k = MIN(k, num_pixels);
	
	// Init centroids	
	printf("STEP 2: Init centroids\n");
	// #pragma omp parallel for
	for(i = 0; i < k; i++) 
	{
		random_num = rand() % num_pixels;
		centroids[i].r = pixels[random_num].r;
		centroids[i].g = pixels[random_num].g;
		centroids[i].b = pixels[random_num].b;
	}

	// K-means iterative procedures start
	printf("STEP 3: Updating centroids\n\n");
	i = 0;
	int max_threads = omp_get_max_threads();
	cluster* centroids_private;
	centroids_private = malloc(k * max_threads * sizeof(cluster));
	do 
  	{
		condition = 0;
		// Reset centroids
		// Find closest cluster for each pixel
		for(j = 0; j < k; j++) 
		{
			centroids[j].mean_r = 0;
			centroids[j].mean_g = 0;
			centroids[j].mean_b = 0;
			centroids[j].num_points = 0;
		}

		for(j = 0; j < k * max_threads; j++) 
		{
			centroids_private[j].mean_r = 0;
			centroids_private[j].mean_g = 0;
			centroids_private[j].mean_b = 0;
			centroids_private[j].num_points = 0;
		}
		int th_num;
		#pragma omp parallel for private(closest, th_num)
		for(j = 0; j < num_pixels; j++) 
		{
			th_num = omp_get_thread_num();
			// printf("%d\n", th_num);
			closest = find_closest_centroid(&pixels[j], centroids, k);
			centroids_private[k*th_num + closest].mean_r += pixels[j].r;
			centroids_private[k*th_num + closest].mean_g += pixels[j].g;
			centroids_private[k*th_num + closest].mean_b += pixels[j].b;
			centroids_private[k*th_num + closest].num_points++;
		}
		for(i = 0; i < k; i++)
		{
			for(j = 0; j < max_threads; j++)
			{
				centroids[i].mean_r += centroids_private[j*k + i].mean_r;
				centroids[i].mean_g += centroids_private[j*k + i].mean_g;
				centroids[i].mean_b += centroids_private[j*k + i].mean_b;
				centroids[i].num_points += centroids_private[j*k + i].num_points;
			}
		}
	
		for(j = 0; j < k; j++) 
		{
			if(centroids[j].num_points == 0) 
			{
				continue;
			}
			centroids[j].mean_r = centroids[j].mean_r/centroids[j].num_points;
			centroids[j].mean_g = centroids[j].mean_g/centroids[j].num_points;
			centroids[j].mean_b = centroids[j].mean_b/centroids[j].num_points;
			changed = centroids[j].mean_r != centroids[j].r || centroids[j].mean_g != centroids[j].g || centroids[j].mean_b != centroids[j].b;
			condition = condition || changed;
			centroids[j].r = centroids[j].mean_r;
			centroids[j].g = centroids[j].mean_g;
			centroids[j].b = centroids[j].mean_b;
		}
		i++;
	} while(condition);
	free(centroids_private);
	printf("Number of K-Means iterations: %d\n\n", i);
}

/*
 * MAIN FUNCTION KMEANS
 *
 */
int main(int argc, char *argv[]) {
	
	uint8_t closest, test, k;
	uint32_t i, j;
	FILE *fp;
	cluster* centroids;
	rgb pixel;

	if (argc != 4 && argc != 5){
	printf("Usage: %s test|exec k imagen.bmp [salida.bmp]\n\nTest: No output \nExec: Image + cluster.txt generated\n", argv[0]);
		exit(1);
	}
	
	test = strcmp("test", argv[1]) == 0;

	printf("TEST = %d\n\n", test);
	
	if(!test && argc != 5){
		printf("Output file not specified.\n");
		exit (-1);
	}

	srand(1);

	image mImage;

	read_file(argc, argv, &mImage);

	printf("WIDTH : %d\n", mImage.width );
	printf("HEIGHT: %d\n", mImage.height);
	printf("LENGHT: %d\n\n", mImage.length);
	fflush(stdout); 	
	
	// Check K integrity value
	if(sscanf(argv[2], "%" SCNu8, &k) != 1){
		printf("K must be a number.\n");
		return -2;
	}
	if(k <= 0){
		printf("K must be > 0\n");
		return -3;
	}

	// Malloc centroids
	centroids = malloc(k * sizeof(cluster));
	if(centroids == NULL){	
		printf("Not enough memory.\n");
		return -4;
	}

	struct timeval start, end;

//==================================================================
	gettimeofday(&start, 0);
	
	kmeans(k, centroids, mImage.length, mImage.pixels);

	gettimeofday(&end, 0);
//==================================================================

	uint32_t checksum = getChecksum(centroids, k);
	end.tv_sec = end.tv_sec - start.tv_sec;
	end.tv_usec = end.tv_usec - start.tv_usec;
	if (end.tv_usec < 0) {  end.tv_usec += 1000000; end.tv_sec--; }

 	printf("\nTime to Kmeans is %ld seconds and %ld microseconds\nChecksum value = %u\n", end.tv_sec, end.tv_usec, checksum);

 	// WRITE OUTPUT
	if(!test){
		if((fp=fopen(argv[4], "w")) == NULL){
			printf("Cannot create output file, do you have write permissions for current directory?.\n");
			return 1;
		}
		if(fwrite(mImage.header, sizeof(uint8_t), sizeof(mImage.header), fp) != sizeof(mImage.header)){
			printf("Error writing BMP.\n");
			return 1;
		}
		for(i = 0; i < mImage.width; i++){
			for(j = 0; j < mImage.height; j++){
				closest = find_closest_centroid(&mImage.pixels[i * mImage.height + j], centroids, k);
				pixel.r = centroids[closest].r;
				pixel.g = centroids[closest].g;
				pixel.b = centroids[closest].b;
				//printf("%d, %d, %d\n", pixel.r, pixel.g, pixel.b);
				
				if(fwrite(&pixel, sizeof(uint8_t), 3, fp) != 3){
					printf("Error writing BMP.\n");
					return 1;
				}
			}
		}
		if(fclose(fp) != 0){
			printf("Error closing file.\n");
			return 1;
		}
		if((fp=fopen("clusters.txt", "w")) == NULL){
			printf("Clusters.txt cannot be created. Do you have space | owner in current directory?\n");
			return 1;
		}
		
		fprintf(fp, "Red\tGreen\tBlue\n");

		for(j = 0; j < k; j++){
			fprintf(fp, "%d\t%d\t%d\n", centroids[j].r, centroids[j].g, centroids[j].b);
		}

		if(fclose(fp) != 0){
			printf("Error closing file.\n");
			return 1;
		}
	}

	free(mImage.pixels);
	free(centroids);
	return 0;
}
