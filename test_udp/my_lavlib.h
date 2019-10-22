#include <libavutil/imgutils.h>
#include <libavutil/avconfig.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>




int load_image_into_frame(AVFrame *frame, const char *filename);
int encode_frame_to_packet(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);
int get_image_from_file(FILE *file, const char *filename, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);

int write_delayed_frames_to_file(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);

AVCodecContext *get_codec_context(int width, int height, int fps);
AVFrame *get_av_frame(AVCodecContext *codec_context);