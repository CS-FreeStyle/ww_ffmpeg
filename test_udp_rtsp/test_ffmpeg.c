#include <stdio.h>
#include <libavutil/imgutils.h>
#include <libavformat/avformat.h>
#include <libavutil/time.h>
#include "my_lavlib.h"

int main(int argc, char **argv){

  FILE *infile = NULL;
  const char *infilename = "bmp_files.txt";
  char buffer[2048];

  // const char *filename = "game.mpg";
  FILE *file=NULL;
  int res, retval=-1;
  AVCodecContext *codec_context= NULL;
  AVFrame *frame=NULL;
  AVPacket pkt;
  uint8_t endcode[] = { 0, 0, 1, 0xb7 };

  const char *out_filename;
  AVFormatContext *ofmt_ctx = NULL;
  AVOutputFormat *ofmt = NULL;
  AVStream *video_st;
  int64_t start_time=0;
  int64_t curr_time = 0;
  int ret;

  if(argc == 2){
    out_filename = argv[1];
  } else {
    out_filename = "rtsp://192.168.8.133:5545/live";
  }

	av_register_all();
	avformat_network_init();

  if((codec_context = get_codec_context(1280, 720, 25)) == NULL){
    fprintf(stderr,"unable to obtain encoding context\n");      
  }

  // if((file = fopen(filename, "wb")) == NULL){
  //   fprintf(stderr,"unable to open destination file %s\n", filename);
  // }

  if((frame = get_av_frame(codec_context)) == NULL){
    fprintf(stderr,"unable to allocate frame\n");
  }

  // temporary measure until image2pipe is working correctly
  if((infile = fopen(infilename, "rb")) == NULL){
    fprintf(stderr,"unable to open input file %s\n", infilename);
  }
  
  memset(buffer,0,2048);
  // if(fgets(buffer, 1024, infile) == NULL){
  //   fprintf(stderr, "unable to get input file\n");
  // }

  // if((res = get_image_from_file(file, buffer, frame, codec_context, &pkt)) < 0){
  //   fprintf(stderr,"unable to write image to file\n");
  // }


  /*
   * missing section could cause problem
   */

  // avformat_alloc_output_context2(&ofmt_ctx, NULL, "mpegts", out_filename);
  avformat_alloc_output_context2(&ofmt_ctx, NULL, "rtsp", out_filename);
	if (!ofmt_ctx) {
		fprintf(stderr,"Could not create output context\n");
		ret = AVERROR_UNKNOWN;
		return -1;
	}

  ofmt = ofmt_ctx->oformat;
  AVStream *out_stream = avformat_new_stream(ofmt_ctx, codec_context->codec);
	if (!out_stream) {
		printf( "Failed allocating output stream\n");
		ret = AVERROR_UNKNOWN;
    return -1;
			// goto end;
  }
  ret = avcodec_copy_context(out_stream->codec, codec_context);
	if (ret < 0) {
		printf( "Failed to copy context from input to output stream codec context\n");
		// goto end;
    return -1;
	}
	out_stream->codec->codec_tag = 0;
	if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER){
		out_stream->codec->flags |= CODEC_FLAG_GLOBAL_HEADER;
  }
  /*
   * missing section could cause problem
   */

  av_dump_format(ofmt_ctx, 0, out_filename, 1);

	//Open output URL
	if (!(ofmt->flags & AVFMT_NOFILE)) {
		ret = avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE);
		if (ret < 0) {
			printf( "Could not open output URL '%s'", out_filename);
			// goto end;
		}
	}

	//Write file header
  // error here
	ret = avformat_write_header(ofmt_ctx, NULL);
	if (ret < 0) {
		fprintf(stderr,"Error occurred when opening output URL\n");
		// goto end;
	}

	start_time=av_gettime();
  curr_time = 0;

  /*
   * may work up to this point
   */
  int frame_index = 0;
  // AVBitStreamFilterContext* h264bsfc =  av_bitstream_filter_init("h264_mp4toannexb");
  while(fgets(buffer, 1024, infile) != NULL){
    buffer[strlen(buffer)-1] = 0;
    // printf("%s\n",buffer);
    if((res = get_image_from_file(file, buffer, frame, codec_context, &pkt)) < 0){
      fprintf(stderr,"unable to write image to file\n");
    }
    memset(buffer,0,2048);

    // res = av_bitstream_filter_filter(h264bsfc, codec_context, NULL, &pkt.data, &pkt.size, pkt.data, pkt.size, 0);
    // if( ret < 0 ){
    //   printf("Can't filter\n");
    //   return -1;
    // }
		//Write PTS
		AVRational time_base1=codec_context->time_base;
		//Duration between 2 frames (us)
		int64_t calc_duration=(double)AV_TIME_BASE/av_q2d((AVRational){1,25});
		//Parameters
		pkt.pts=(double)(frame_index*calc_duration)/(double)(av_q2d(time_base1)*AV_TIME_BASE);
		pkt.dts=pkt.pts;
		pkt.duration=(double)calc_duration/(double)(av_q2d(time_base1)*AV_TIME_BASE);

    ret = av_interleaved_write_frame(ofmt_ctx, &pkt);
		if (ret < 0) {
			printf( "Error muxing packet\n");
      return -1;
			break;
		}
		
		av_free_packet(&pkt);

    printf("Send %8d video frames to output URL\n",frame_index);
    frame_index++;

  }

  av_write_trailer(ofmt_ctx);

  // if((res = write_delayed_frames_to_file(file, frame, codec_context, &pkt)) < 0){
  //     fprintf(stderr,"failed to write delayed frames");
  // }

  // fwrite(endcode, 1, sizeof(endcode), file);

  retval = 0;

  error:
  // if (file)
  //   fclose(file);

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