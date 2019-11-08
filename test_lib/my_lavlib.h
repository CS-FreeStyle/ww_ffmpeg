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

typedef struct{
    BMPHeader header;
    char *data;
} BMPImage;

typedef struct{
    AVCodecContext *codec_ctx;
    AVFrame *frame;
    AVPacket pkt;
    AVStream *out_stream;
    AVFormatContext *ofmt_ctx;
    const char *endpoint;
    int frame_index;
    int fps;
} RTSPStream;

// typedef struct _XImage {
// 	int width, height;		/* size of image */
// 	int xoffset;			/* number of pixels offset in X direction */
// 	int format;			/* XYBitmap, XYPixmap, ZPixmap */
// 	char *data;			/* pointer to image data */
// 	int byte_order;			/* data byte order, LSBFirst, MSBFirst */
// 	int bitmap_unit;		/* quant. of scanline 8, 16, 32 */
// 	int bitmap_bit_order;		/* LSBFirst, MSBFirst */
// 	int bitmap_pad;			/* 8, 16, 32 either XY or ZPixmap */
// 	int depth;			/* depth of image */
// 	int bytes_per_line;		/* accelerator to next scanline */
// 	int bits_per_pixel;		/* bits per pixel (ZPixmap) */
// 	unsigned long red_mask;		/* bits in z arrangement */
// 	unsigned long green_mask;
// 	unsigned long blue_mask;
// 	XPointer obdata;		/* hook for the object routines to hang on */
// 	struct funcs {			/* image manipulation routines */
// 		struct _XImage *(*create_image)();
// 		int (*destroy_image)();
// 		unsigned long (*get_pixel)();
// 		int (*put_pixel)();
// 		struct _XImage *(*sub_image)();
// 		int (*add_pixel)();
// 	} f;
// } XImage;


int init_rtsp_stream(RTSPStream *rtsp_stream, int width, int height, int fps, int bitrate, const char *endpoint);
int start_rtsp_stream(RTSPStream *rtsp_stream);
int write_image_to_rtsp_stream(RTSPStream *rtsp_stream, BMPImage *image);
int end_rtsp_stream(RTSPStream *rtsp_stream);
int free_rtsp_stream(RTSPStream *rtsp_stream);

AVCodecContext *get_codec_context(int width, int height, int fps, int bitrate);
AVFrame *get_av_frame(AVCodecContext *codec_context);

int get_image_from_memory(BMPImage *image, AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);
int load_image_into_frame(AVFrame *frame, BMPImage *image);
int encode_frame_to_packet(AVFrame *frame, AVCodecContext *codec_context, AVPacket *pkt);

