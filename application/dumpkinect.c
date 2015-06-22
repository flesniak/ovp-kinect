#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../kinect-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

int main() {
  volatile unsigned int* cr      = (unsigned int*)KINECT_CR_ADDR;
           unsigned int* videoIn = (unsigned int*)KINECT_VIDEO_DEFAULT_ADDRESS;
           unsigned int* depthIn = (unsigned int*)KINECT_DEPTH_DEFAULT_ADDRESS;
           unsigned int* fbOut   = (unsigned int*)DVI_VMEM_ADDRESS;

  printf("Reading kinect status register: 0x%08x\n", *cr);
  printf("Enabling kinect, video and depth streaming\n");
  *cr = KINECT_CR_ENABLED_MASK | KINECT_CR_VIDEO_MASK | KINECT_CR_DEPTH_MASK;

  int frames = 0;
  printf("Start dumping frames...\n");
  while( 1 ) {
    *cr |= KINECT_CR_REQUEST_MASK; //request frame
    while( *cr & KINECT_CR_REQUEST_MASK ); //poll for requested frame

    //dump video frame, 3 bytes per pixel
    unsigned char* vin = (unsigned char*)videoIn;
    for( unsigned int y = 0; y < KINECT_VIDEO_HEIGHT; y++ )
      for( unsigned int x = 0; x < KINECT_VIDEO_WIDTH; x+=4 ) { //width must be dividable by 4 for the algorithm to work
        *(fbOut+y*DVI_VMEM_WIDTH+x+0) = (unsigned int) vin[2] << 16 |  vin[1] << 8 | vin[0];
        *(fbOut+y*DVI_VMEM_WIDTH+x+1) = (unsigned int) vin[5] << 16 |  vin[4] << 8 | vin[3];
        *(fbOut+y*DVI_VMEM_WIDTH+x+2) = (unsigned int) vin[8] << 16 |  vin[7] << 8 | vin[6];
        *(fbOut+y*DVI_VMEM_WIDTH+x+3) = (unsigned int)vin[11] << 16 | vin[10] << 8 | vin[9];
      }

    //dump depth frame


    //printf("dumpvideo: in 0x%08x out 0x%08x\n", *fbIn, *fbOut);
    frames++;
    if( (frames & 0xF) == 0 )
      printf("Dumped %d frames\n", frames);
  }

  return 0;
}
