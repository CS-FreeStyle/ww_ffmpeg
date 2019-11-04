#include <libavutil/imgutils.h>
#include <libavutil/avconfig.h>
#include <libswscale/swscale.h>
#include <libavformat/avformat.h>

#pragma pack(push)  // save the original data alignment
#pragma pack(1)     // Set data alignment to 1 byte boundary

#define BMP_HEADER_SIZE 54

typedef struct {
    uint16_t type;              // Magic identifier: 0x4d42
    uint32_t size;              // File size in bytes
    uint16_t reserved1;         // Not used
    uint16_t reserved2;         // Not used
    uint32_t offset;            // Offset to image data in bytes from beginning of file
    uint32_t dib_header_size;   // DIB Header size in bytes
    int32_t  width_px;          // Width of the image
    int32_t  height_px;         // Height of image
    uint16_t num_planes;        // Number of color planes
    uint16_t bits_per_pixel;    // Bits per pixel
    uint32_t compression;       // Compression type
    uint32_t image_size_bytes;  // Image size in bytes
    int32_t  x_resolution_ppm;  // Pixels per meter
    int32_t  y_resolution_ppm;  // Pixels per meter
    uint32_t num_colors;        // Number of colors
    uint32_t important_colors;  // Important colors
} BMPHeader;


int load_image_into_frame(AVFrame *frame, const char *filename);
int encode_frame_to_packet(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);
int get_image_from_file(FILE *file, const char *filename, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);

int write_delayed_frames_to_file(FILE *file, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);

AVCodecContext *get_codec_context(int width, int height, int fps);
AVFrame *get_av_frame(AVCodecContext *codec_context);