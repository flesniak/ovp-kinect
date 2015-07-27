#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "../kinect-cfg.h"
#include "../../xilinx-dvi/dvi-mem.h"

int main() {
  volatile unsigned int* cr      = (unsigned int*)KINECT_CR_ADDR;
  volatile unsigned int* fr      = (unsigned int*)KINECT_FMTREG_ADDR;
           unsigned int* videoIn = (unsigned int*)KINECT_VIDEO_DEFAULT_ADDRESS;
           unsigned int* depthIn = (unsigned int*)KINECT_DEPTH_DEFAULT_ADDRESS;
           unsigned int* fbOut   = (unsigned int*)DVI_VMEM_ADDRESS;

  printf("Reading kinect status register: 0x%08x\n", *cr);
  printf("Enabling kinect, video and depth streaming\n");
  *cr = KINECT_CR_ENABLED_MASK | KINECT_CR_VIDEO_MASK | KINECT_CR_DEPTH_MASK;
  printf("Forcing format to default\n");
  *fr = 0;

  for( unsigned int y = 20; y < 30; y++ ) {
    for( unsigned int x = 20; x < 30; x++ )
      *(fbOut+y*DVI_VMEM_WIDTH+x) = 0xffffff; //white
    for( unsigned int x = 30; x < 40; x++ )
      *(fbOut+y*DVI_VMEM_WIDTH+x) = 0xff0000; //red
    for( unsigned int x = 40; x < 50; x++ )
      *(fbOut+y*DVI_VMEM_WIDTH+x) = 0xff00;   //green
    for( unsigned int x = 50; x < 60; x++ )
      *(fbOut+y*DVI_VMEM_WIDTH+x) = 0xff;     //blue
  }

  int frames = 0;
  printf("Start dumping frames...\n");
  while( 1 ) {
    *cr |= KINECT_CR_REQUEST_MASK; //request frame
    while( *cr & KINECT_CR_REQUEST_MASK ); //poll for requested frame

    //dump video frame, 3 bytes per pixel
    unsigned char* vin = (unsigned char*)videoIn;
    //works for little-endian framebuffer
    /*for( unsigned int y = 0; y < KINECT_VIDEO_HEIGHT; y++ )
      for( unsigned int x = 0; x < KINECT_VIDEO_WIDTH; x+=4 ) { //width must be dividable by 4 for the algorithm to work
        *(fbOut+y*DVI_VMEM_WIDTH+x+0) = (unsigned int) vin[0] << 16 | vin[1]  << 8 | vin[2];
        *(fbOut+y*DVI_VMEM_WIDTH+x+1) = (unsigned int) vin[3] << 16 | vin[4]  << 8 | vin[5];
        *(fbOut+y*DVI_VMEM_WIDTH+x+2) = (unsigned int) vin[6] << 16 | vin[7]  << 8 | vin[8];
        *(fbOut+y*DVI_VMEM_WIDTH+x+3) = (unsigned int) vin[9] << 16 | vin[10] << 8 | vin[11];
        vin += 12;
      }*/
    //works for big-endian framebuffer
    for( unsigned int y = 0; y < KINECT_VIDEO_HEIGHT; y++ )
      for( unsigned int x = 0; x < KINECT_VIDEO_WIDTH; x+=4 ) { //width must be dividable by 4 for the algorithm to work
        *(fbOut+y*DVI_VMEM_WIDTH+x+0) = (unsigned int) vin[3] << 16 | vin[2]  << 8 | vin[1];
        *(fbOut+y*DVI_VMEM_WIDTH+x+1) = (unsigned int) vin[0] << 16 | vin[7]  << 8 | vin[6];
        *(fbOut+y*DVI_VMEM_WIDTH+x+2) = (unsigned int) vin[5] << 16 | vin[4]  << 8 | vin[11];
        *(fbOut+y*DVI_VMEM_WIDTH+x+3) = (unsigned int) vin[10] << 16 | vin[9] << 8 | vin[8];
        vin += 12;
      }
    
    //dump depth frame
    unsigned int* din = depthIn;
    for( unsigned int y = 0; y < KINECT_DEPTH_HEIGHT; y++ )
      for( unsigned int x = 0; x < KINECT_DEPTH_WIDTH; x+=2 ) { //width must be dividable by 4 for the algorithm to work
        unsigned int d0 = (*din & 0x0000ffff) >> 3;
        unsigned int d1 = (*din & 0xffff0000) >> 19;
        *(fbOut+y*DVI_VMEM_WIDTH+640+x) = d0 << 16 | d0 << 8 | d0;
        *(fbOut+y*DVI_VMEM_WIDTH+641+x) = d1 << 16 | d1 << 8 | d1;
        din++;
      }

    //printf("dumpvideo: in 0x%08x out 0x%08x\n", *fbIn, *fbOut);
    frames++;
    if( (frames & 0xF) == 0 )
      printf("Dumped %d frames\n", frames);
  }

  return 0;
}
