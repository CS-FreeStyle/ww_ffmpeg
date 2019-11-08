#include "my_lavlib.h"

#include <unistd.h>
#include <fcntl.h>

int encode_frame_to_packet(AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt) {

  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;

  /* generate synthetic video */
  frame->pts += 1;

  if(avcodec_send_frame(codec_context, frame) < 0){
    fprintf(stderr,"error sending frame to encoder\n");
    goto error;
  }

  // error here on first frame
  int retval;
  if((retval = avcodec_receive_packet(codec_context, pkt)) < 0){
    // fprintf(stderr,"error receiving packet from encoder %d %d\n",retval, AVERROR(EAGAIN));
    goto error;
  }

  return 0;
error:
  return -1;
}

AVCodecContext *get_codec_context(int width, int height, int fps, int bitrate)
{
  AVCodec *codec = NULL;
  AVCodecContext *codec_context = NULL;
  AVDictionary *codec_options = NULL;

  if((codec = avcodec_find_encoder(AV_CODEC_ID_H264)) == NULL){
      fprintf(stderr,"unable to find codec\n");
      goto error;
  }

  if((codec_context = avcodec_alloc_context3(codec)) == NULL){
      fprintf(stderr,"unable to allocate codec\n");
      goto error;
  }

  av_dict_set(&codec_options, "preset", "veryfast", 0);

  codec_context->bit_rate = bitrate;
  codec_context->width = width;
  codec_context->height = height;
  codec_context->time_base= (AVRational){1,fps};
  codec_context->gop_size = 10;
  codec_context->max_b_frames=1;
  codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

  if(avcodec_open2(codec_context, codec, &codec_options) < 0){
      fprintf(stderr,"could not open codec\n");
      goto error;
  }

  return codec_context;
error:
  return NULL;
}

AVFrame *get_av_frame(AVCodecContext *codec_context) {

  AVFrame *frame = NULL;

  if((frame = av_frame_alloc()) == NULL){
      fprintf(stderr, "unable to allocate frame\n");
      goto error;
  }

  frame->height = codec_context->height;
  frame->width = codec_context->width;
  frame->format = codec_context->pix_fmt;
  frame->pts = 0;

  if(av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, frame->format, 1) < 0){
      fprintf(stderr,"failed to allocate memory for video frame\n");
      goto error;
  }

  return frame;
error:

  if(frame){
    av_freep(&frame->data[0]);
    av_free(frame);
  }
  return NULL;
}


int init_rtsp_stream(RTSPStream *rtsp_stream, int width, int height, int fps, int bitrate, const char *endpoint){

  rtsp_stream->codec_ctx = NULL;
  rtsp_stream->endpoint = NULL;
  rtsp_stream->frame = NULL;
  rtsp_stream->ofmt_ctx = NULL;
  rtsp_stream->out_stream = NULL;
  rtsp_stream->frame_index = 0;
  rtsp_stream->fps = fps;

  av_register_all();
	avformat_network_init();
  rtsp_stream->endpoint = endpoint;

  if((rtsp_stream->codec_ctx = get_codec_context(width, height, fps, bitrate)) == NULL){
    fprintf(stderr,"unable to obtain encoding context\n");
    goto error;
  }

  if((rtsp_stream->frame = get_av_frame(rtsp_stream->codec_ctx)) == NULL){
    fprintf(stderr,"unable to allocate frame\n");
    goto error;
  }

  avformat_alloc_output_context2(&rtsp_stream->ofmt_ctx, NULL, "rtsp", rtsp_stream->endpoint);
	if (!rtsp_stream->ofmt_ctx) {
		fprintf(stderr,"could not create output context\n");
    goto error;
	}

  rtsp_stream->out_stream = avformat_new_stream(rtsp_stream->ofmt_ctx, rtsp_stream->codec_ctx->codec);
	if (!rtsp_stream->out_stream) {
		fprintf(stderr,"failed allocating output stream\n");
		goto error;
  }

  if(avcodec_parameters_from_context(rtsp_stream->out_stream->codecpar, rtsp_stream->codec_ctx) < 0){
		fprintf(stderr,"failed to copy context from input to output stream codec context\n");
		goto error;
  }

  av_dump_format(rtsp_stream->ofmt_ctx, 0, rtsp_stream->endpoint, 1);

	if (!(rtsp_stream->ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
		if((avio_open(&rtsp_stream->ofmt_ctx->pb, rtsp_stream->endpoint, AVIO_FLAG_WRITE)) < 0){
			fprintf(stderr,"could not open output url '%s'\n", rtsp_stream->endpoint);
      goto error;
    }
	}

  return 0;

error:

  if(rtsp_stream->frame){
    av_freep(&rtsp_stream->frame->data[0]);
    av_free(rtsp_stream->frame);
  }

  if(rtsp_stream->codec_ctx) {
    avcodec_close(rtsp_stream->codec_ctx);
    av_free(rtsp_stream->codec_ctx);
  }

  if(rtsp_stream->ofmt_ctx && !(rtsp_stream->ofmt_ctx->flags & AVFMT_NOFILE)){
		avio_close(rtsp_stream->ofmt_ctx->pb);
  }

  if(rtsp_stream->ofmt_ctx){
	  avformat_free_context(rtsp_stream->ofmt_ctx);
  }

  return -1;
}

int start_rtsp_stream(RTSPStream *rtsp_stream){
  if((avformat_write_header(rtsp_stream->ofmt_ctx, NULL)) < 0){
		fprintf(stderr,"error occurred when opening output URL\n");
    return -1;
  }
  return 0;
}

int write_image_to_rtsp_stream(RTSPStream *rtsp_stream, BMPImage *image){
  if(get_image_from_memory(image, rtsp_stream->frame, rtsp_stream->codec_ctx, &rtsp_stream->pkt) < 0){
    return -1;
  }

	if (av_interleaved_write_frame(rtsp_stream->ofmt_ctx, &rtsp_stream->pkt) < 0) {
		printf( "Error muxing packet\n");
     return -1;
	}
		
	av_packet_unref(&rtsp_stream->pkt);

  rtsp_stream->frame_index++;

}

int get_image_from_memory(BMPImage *image, AVFrame *frame, AVCodecContext *codec_ctx, AVPacket *pkt){

  if(load_image_into_frame(frame, image) < 0){
      fprintf(stderr,"failed to load image into frame\n");
      goto error;
  }

  if(encode_frame_to_packet(frame, codec_ctx, pkt) < 0){
    // fprintf(stderr,"unable to write frame to packet\n");
    goto error;
  }

  return 0;
error:
  return -1;
}

int load_image_into_frame(AVFrame *frame, BMPImage *image){

  static struct SwsContext *sws_ctx;

  uint8_t* inData[1];
  int check;
  int linesize[1];

  inData[0] = (uint8_t *)image->data;
  linesize[0] = 4 * image->header.width_px;

  if((sws_ctx = sws_getContext(image->header.width_px, image->header.height_px, AV_PIX_FMT_BGRA,
                                frame->width, frame->height, frame->format,
                                SWS_BICUBIC, NULL, NULL, NULL)) == NULL){
    fprintf(stderr,"unable to initialize scaling context\n");
    goto error;
  }


  sws_scale(sws_ctx,(const uint8_t * const *)inData, linesize,
            0, image->header.height_px, frame->data, frame->linesize);

  sws_freeContext(sws_ctx);
  return 0;

error:
  sws_freeContext(sws_ctx);
  return -1;
}

int end_rtsp_stream(RTSPStream *rtsp_stream){
  av_write_trailer(rtsp_stream->ofmt_ctx);
}

int free_rtsp_stream(RTSPStream *rtsp_stream){
  if(rtsp_stream->frame){
    av_freep(&rtsp_stream->frame->data[0]);
    av_free(rtsp_stream->frame);
  }

  if(rtsp_stream->codec_ctx) {
    avcodec_close(rtsp_stream->codec_ctx);
    av_free(rtsp_stream->codec_ctx);
  }

  if(rtsp_stream->ofmt_ctx && !(rtsp_stream->ofmt_ctx->flags & AVFMT_NOFILE)){
		avio_close(rtsp_stream->ofmt_ctx->pb);
  }

  if(rtsp_stream->ofmt_ctx){
	  avformat_free_context(rtsp_stream->ofmt_ctx);
  }
}