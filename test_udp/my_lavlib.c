#include "my_lavlib.h"
#include "my_lavfutils.h"


int load_image_into_frame(AVFrame *frame, const char *filename)
{
  int retval = -1, res;
  static struct SwsContext *sws_ctx;
  uint8_t *image_data[4];
  int linesize[4];
  int source_width, source_height;
  enum AVPixelFormat source_fmt;

  if((res = ff_load_image(image_data, linesize, &source_width, &source_height, &source_fmt, filename, NULL)) < 0){
      fprintf(stderr,"failed to load image\n");
      goto error;
  }

  if (source_fmt != frame->format) {
    if((sws_ctx = sws_getContext(source_width, source_height, source_fmt,
        frame->width, frame->height, frame->format,
        SWS_BICUBIC, NULL, NULL, NULL)) == NULL){
          fprintf(stderr,"unable to initialize scaling context\n");
          goto error;
        }
    // check(sws_ctx, "unable to initialize scaling context");

        sws_scale(sws_ctx,
        (const uint8_t * const *)image_data, linesize,
        0, source_height, frame->data, frame->linesize);
  }

  retval = 0;
error:
  av_freep(&image_data[0]);
  sws_freeContext(sws_ctx);
  return retval;
}

int encode_frame_to_packet(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt) {
  int res, got_output = 1;
  av_init_packet(pkt);
  pkt->data = NULL;
  pkt->size = 0;

  /* generate synthetic video */
  frame->pts += 1;

  // old deprecated function
  // if((res = avcodec_encode_video2(codec_context, pkt, frame, &got_output)) < 0){
  //     fprintf(stderr,"error encoding frame\n");
  //     goto error;
  // }

  res = avcodec_send_frame(codec_context, frame);
  if (res < 0) {
    // fprintf(stderr,"error encoding frame\n");
    got_output = 0;
    // goto error;
  }

  // error here on first frame
  res = avcodec_receive_packet(codec_context, pkt);
  if (res < 0) {
    // fprintf(stderr,"error encoding frame\n");
    got_output = 0;
    // goto error;
  }

  // if (got_output) {
  //   fwrite(pkt->data, 1, pkt->size, file);
  //   av_packet_unref(pkt);
  // }
  return 0;
error:
  return -1;
}

int get_image_from_file(FILE *file, const char *filename, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt) {
  int res, i;

  if((res = load_image_into_frame(frame, filename)) < 0){
      fprintf(stderr,"failed to load image into frame\n");
      goto error;
  }

  if((res = encode_frame_to_packet(file, frame, codec_context, pkt)) < 0){
    fprintf(stderr,"unable to write frame to file\n");
    goto error;
  }

  return 0;
error:
  return -1;
}

int write_delayed_frames_to_file(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt) {
  int res, got_output;

  for (got_output = 1; got_output;) {
    // deprecated function
    // if((res = avcodec_encode_video2(codec_context, pkt, NULL, &got_output)) < 0){
    //     fprintf(stderr,"error encoding frame\n");
    // }

    res = avcodec_send_frame(codec_context, NULL);
    if (res < 0) {
      // fprintf(stderr,"error encoding frame\n");
      got_output = 0;
      // goto error;
    }
    res = avcodec_receive_packet(codec_context, pkt);
    if (res < 0) {
      // fprintf(stderr,"error encoding frame\n");
      got_output = 0;
      // goto error;
    }

    if (got_output) {
      fwrite(pkt->data, 1, pkt->size, file);
      av_packet_unref(pkt);
    }
  }

  return 0;
error:
  return -1;
}

AVCodecContext *get_codec_context(int width, int height, int fps)
{
  int res;
  // av_register_all();
//   avcodec_register_all();

  AVCodec *codec;
  AVCodecContext *codec_context = NULL;

  if((codec = avcodec_find_encoder(AV_CODEC_ID_MPEG1VIDEO)) == NULL){
      fprintf(stderr,"unable to find codec\n");
      goto error;
  }

  // if((codec = avcodec_find_encoder(AV_CODEC_ID_H264)) == NULL){
  //     fprintf(stderr,"unable to find codec\n");
  //     goto error;
  // }

  if((codec_context = avcodec_alloc_context3(codec)) == NULL){
      fprintf(stderr,"unable to allocate codec\n");
      goto error;
  }

  codec_context->bit_rate = 400000;
  codec_context->width = width;
  codec_context->height = height;
  codec_context->time_base= (AVRational){1,fps};
  codec_context->gop_size = 10;
  codec_context->max_b_frames=1;
  codec_context->pix_fmt = AV_PIX_FMT_YUV420P;

  // if (codec->id == AV_CODEC_ID_H264){
  //   av_opt_set(codec_context->priv_data, "preset", "veryfast", 0);
  // }


  if((res = avcodec_open2(codec_context, codec, NULL)) < 0){
      fprintf(stderr,"could not open codec\n");
      goto error;
  }

  return codec_context;
error:
  return NULL;
}

AVFrame *get_av_frame(AVCodecContext *codec_context) {
  int res;
  AVFrame *frame;

  if((frame = av_frame_alloc()) == NULL){
      fprintf(stderr, "unable to allocate frame\n");
      goto error;
  }

  frame->height = codec_context->height;
  frame->width = codec_context->width;
  frame->format = codec_context->pix_fmt;
  frame->pts = 0;

  if((res = av_image_alloc(frame->data, frame->linesize, frame->width, frame->height, frame->format, 1)) < 0){
      fprintf(stderr,"failed to allocate memory for video frame\n");
      goto error;
  }

  return frame;
error:
  return NULL;
}