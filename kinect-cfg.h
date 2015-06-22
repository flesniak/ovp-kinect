#ifndef KINECT_CFG
#define KINECT_CFG

#define KINECT_DEFAULT_INDEX               0

//PERIPHERAL PARAMETERS
#define KINECT_REGS_DEFAULT_ADDRESS        0x90000000U
#define KINECT_VIDEO_DEFAULT_ADDRESS       0x90100000U
#define KINECT_DEPTH_DEFAULT_ADDRESS       0x90200000U
#define KINECT_REGS_SIZE                   16

#define KINECT_REGS_BUS_NAME               "CFGBUS"
#define KINECT_VIDEO_BUS_NAME              "VIDEOBUS"
#define KINECT_DEPTH_BUS_NAME              "DEPTHBUS"

//usually 640x480 pixels 24bpp 0xrrggbb (in RGB mode, notation in little endian)
//for other modes (bayer, yuv etc see libfreenect documentation and src/cameras.c)
#define KINECT_VIDEO_WIDTH                 640 //1280x1024 video mode not implemented
#define KINECT_VIDEO_HEIGHT                480
#define KINECT_VIDEO_IR_HEIGHT             488
#define KINECT_VIDEO_MAX_HEIGHT            KINECT_VIDEO_IR_HEIGHT

#define KINECT_VIDEO_MAX_BYTES_PER_PIXEL   3
#define KINECT_VIDEO_SCANLINE_BYTES        (KINECT_VIDEO_WIDTH*KINECT_VIDEO_MAX_BYTES_PER_PIXEL)
#define KINECT_VIDEO_BUFFER_SIZE           (KINECT_VIDEO_SCANLINE_BYTES*KINECT_VIDEO_MAX_HEIGHT)
#define KINECT_VIDEO_RMASK                 0x00ff0000
#define KINECT_VIDEO_GMASK                 0x0000ff00
#define KINECT_VIDEO_BMASK                 0x000000ff

//missing defines for depth buffer size/layout/etc
#define KINECT_DEPTH_WIDTH                 640
#define KINECT_DEPTH_HEIGHT                480
#define KINECT_DEPTH_MAX_BYTES_PER_PIXEL   2
#define KINECT_DEPTH_BUFFER_SIZE           (KINECT_DEPTH_WIDTH*KINECT_DEPTH_HEIGHT*KINECT_DEPTH_MAX_BYTES_PER_PIXEL)

//format indices from libfreenect, just for reference
#define KINECT_VIDEO_RGB                   0 //3 bytes per pixel
#define KINECT_VIDEO_BAYER                 1 //1 byte per pixel
#define KINECT_VIDEO_IR_8BIT               2 //1 byte per pixel, IR height!
#define KINECT_VIDEO_IR_10BIT              3 //2 bytes per pixel, IR height!
#define KINECT_VIDEO_IR_10BIT_PACKED       4 //10/8 bytes per pixel, IR height!
#define KINECT_VIDEO_YUV_RGB               5 //3 bytes per pixel
#define KINECT_VIDEO_YUV_RAW               6 //2 bytes per pixel
#define KINECT_VIDEO_FORMAT_COUNT          7

#define KINECT_VIDEO_DEFAULT_FORMAT        KINECT_VIDEO_RGB

#define KINECT_DEPTH_11BIT                 0 //2 bytes per pixel
#define KINECT_DEPTH_10BIT                 1 //2 bytes per pixel
#define KINECT_DEPTH_11BIT_PACKED          2 //11/8 bytes per pixel
#define KINECT_DEPTH_10BIT_PACKED          3 //10/8 bytes per pixel
#define KINECT_DEPTH_REGISTERED            4 //2 bytes per pixel
#define KINECT_DEPTH_MM                    5 //2 bytes per pixel
#define KINECT_DEPTH_FORMAT_COUNT          6

#define KINECT_DEPTH_DEFAULT_FORMAT        KINECT_DEPTH_REGISTERED

#define KINECT_CR_OFFSET_INT               0
#define KINECT_CR_OFFSET_BYTES             (4*KINECT_CR_OFFSET_INT)
#define KINECT_CR_ADDR                     (KINECT_REGS_DEFAULT_ADDRESS+KINECT_CR_OFFSET_BYTES)
#define KINECT_CR_ENABLED_OFFSET           0
#define KINECT_CR_ENABLED_MASK             0x00000001
#define KINECT_CR_VIDEO_OFFSET             1
#define KINECT_CR_VIDEO_MASK               0x00000002
#define KINECT_CR_DEPTH_OFFSET             2
#define KINECT_CR_DEPTH_MASK               0x00000004
#define KINECT_CR_CONTINUOUS_OFFSET        3
#define KINECT_CR_CONTINUOUS_MASK          0x00000008
#define KINECT_CR_REQUEST_OFFSET           31
#define KINECT_CR_REQUEST_MASK             0x80000000

#define KINECT_VIDEO_AR_OFFSET_INT         1
#define KINECT_VIDEO_AR_OFFSET_BYTES       (4*KINECT_VIDEO_AR_OFFSET_INT)
#define KINECT_VIDEO_AR_ADDR               (KINECT_REGS_DEFAULT_ADDRESS+KINECT_VIDEO_AR_OFFSET_BYTES)
#define KINECT_VIDEO_AR_ADDR_OFFSET        0
#define KINECT_VIDEO_AR_ADDR_MASK          0xffffffff

#define KINECT_DEPTH_AR_OFFSET_INT         2
#define KINECT_DEPTH_AR_OFFSET_BYTES       (4*KINECT_DEPTH_AR_OFFSET_INT)
#define KINECT_DEPTH_AR_ADDR               (KINECT_REGS_DEFAULT_ADDRESS+KINECT_DEPTH_AR_OFFSET_BYTES)
#define KINECT_DEPTH_AR_ADDR_OFFSET        0
#define KINECT_DEPTH_AR_ADDR_MASK          0xffffffff

#define KINECT_FMTREG_OFFSET_INT           3
#define KINECT_FMTREG_OFFSET_BYTES         (4*KINECT_FMTREG_OFFSET_INT)
#define KINECT_FMTREG_ADDR                 (KINECT_REGS_DEFAULT_ADDRESS+KINECT_IR_OFFSET_BYTES)
#define KINECT_FMTREG_VIDEO_OFFSET         0
#define KINECT_FMTREG_VIDEO_MASK           0x0000ffff
#define KINECT_FMTREG_DEPTH_OFFSET         15
#define KINECT_FMTREG_DEPTH_MASK           0xffff0000

#endif //KINECT_CFG
