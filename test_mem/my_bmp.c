struct bmp_header{
  uint16_t id;
  uint32_t size;
  uint16_t dm1;
  uint16_t dm2;
  uint32_t offset;
};

struct dib_header{
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint16_t planes;
  uint16_t pxl_bits;
};