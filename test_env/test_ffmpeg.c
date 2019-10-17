#include <stdio.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include "my_lavfutils.h"
#include "my_lablib.h"

int main(int argc, char **argv){

  FILE *infile = NULL;
  const char *infilename = "bmp_files.txt";
  char buffer[2048];

  const char *filename = "game.mpg";
  FILE *file=NULL;
  int res, retval=-1;
  AVCodecContext *codec_context= NULL;
  AVFrame *frame=NULL;
  AVPacket pkt;
  uint8_t endcode[] = { 0, 0, 1, 0xb7 };

  if((codec_context = get_codec_context(1280, 720, 25)) == NULL){
    fprintf(stderr,"unable to obtain encoding context\n");      
  }

  if((file = fopen(filename, "wb")) == NULL){
    fprintf(stderr,"unable to open destination file %s\n", filename);
  }

  if((frame = get_av_frame(codec_context)) == NULL){
    fprintf(stderr,"unable to allocate frame\n");
  }

//   if((res = write_image_to_file(file, "pipe:", 25, frame, codec_context, &pkt)) < 0){
//       fprintf(stderr,"unable to write image to file\n");
//   }

// testing with a single frame
// if((res = write_image_to_file(file, "/home/jlucas/Documents/bmps/2019-09-10-21-07-15-121012.bmp", 1, frame, codec_context, &pkt)) < 0){
//     fprintf(stderr,"unable to write image to file\n");
// }
// if((res = write_image_to_file(file, "/home/jlucas/Documents/bmps/2019-09-10-21-07-15-121012.bmp", 1, frame, codec_context, &pkt)) < 0){
//     fprintf(stderr,"unable to write image to file\n");
// }

  // temporary measure until image2pipe is working correctly
  if((infile = fopen(infilename, "rb")) == NULL){
    fprintf(stderr,"unable to open input file %s\n", infilename);
  }
  
  memset(buffer,0,2048);
  while(fgets(buffer, 1024, infile) != NULL){
    buffer[strlen(buffer)-1] = 0;
    // printf("%s\n",buffer);
    if((res = write_image_to_file(file, buffer, 1, frame, codec_context, &pkt)) < 0){
      fprintf(stderr,"unable to write image to file\n");
    }
    memset(buffer,0,2048);
  }

  if((res = write_delayed_frames_to_file(file, frame, codec_context, &pkt)) < 0){
      fprintf(stderr,"failed to write delayed frames");
  }

  fwrite(endcode, 1, sizeof(endcode), file);

  retval = 0;

  error:
  if (file)
    fclose(file);
  if (codec_context) {
    avcodec_close(codec_context);
    av_free(codec_context);
  }
  if (frame) {
    av_freep(&frame->data[0]);
    av_free(frame);
  }
  return retval;
    // av_register_all();    
    // printf("i survived\n");
}