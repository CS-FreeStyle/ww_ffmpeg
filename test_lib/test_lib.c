#include <stdio.h>
#include <string.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include "my_lavlib.h"

#include <unistd.h>
#include <fcntl.h>

int main(int argc, char **argv){

  FILE *infile = NULL;
  int bmpfile;
  const char *infilename = "bmp_files.txt";
  char buffer[2048];

  RTSPStream rtsp_stream;
  BMPImage image;

  int retval;

  if(argc != 2){
    fprintf(stderr,"bad invocation\n");
    return -1;
  }

  // API CALL
  retval = init_rtsp_stream(&rtsp_stream, 1280, 720, 25, 3000000, argv[1]);
  printf("init_rtsp_stream = %d\n", retval);

  // API CALL
  retval = start_rtsp_stream(&rtsp_stream);
  printf("start_rtsp_stream = %d\n", retval);

  if((infile = fopen(infilename, "rb")) == NULL){
    fprintf(stderr,"unable to open input file %s\n", infilename);
  }

  memset(buffer,0,2048);
  while(fgets(buffer, 1024, infile) != NULL){
    buffer[strlen(buffer)-1] = 0;

    if((bmpfile = open(buffer, O_RDONLY)) < 0){
        fprintf(stderr,"error opening file\n");
        // goto error;
    }

    if((read(bmpfile, &image.header, BMP_HEADER_SIZE)) != BMP_HEADER_SIZE){
        fprintf(stderr,"error reading bmp header\n");
        // goto error;
    }

    int i;
    image.data = (char *)malloc(image.header.size - image.header.offset);
    for(i = image.header.width_px*(image.header.height_px-1)*4; i >= 0; i-=image.header.width_px*4){
        if((read(bmpfile, &image.data[i], image.header.width_px*4)) != image.header.width_px*4){
        fprintf(stderr, "unable to read rgba data\n");
        //   goto error;
        }
    }
    close(bmpfile);
    memset(buffer,0,2048);

    // API CALL
    if(write_image_to_rtsp_stream(&rtsp_stream, &image) != -1){
      printf("Sent %8d video frames to output URL\n",rtsp_stream.frame_index);
    }

    free(image.data);
  }

  fclose(infile);

  retval = end_rtsp_stream(&rtsp_stream);
  printf("end_rtsp_stream = %d\n", retval);

  retval = free_rtsp_stream(&rtsp_stream);
  printf("free_rtsp_stream = %d\n", retval);

  return 0;
}