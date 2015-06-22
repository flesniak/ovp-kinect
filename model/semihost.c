#include <byteswap.h>
#include <gd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <vmi/vmiMessage.h>
#include <vmi/vmiOSAttrs.h>
#include <vmi/vmiOSLib.h>
#include <vmi/vmiPSE.h>
#include <vmi/vmiTypes.h>

#include <libfreenect.h>
#include <sys/time.h>

#include "../kinect-cfg.h"

typedef struct vmiosObjectS {
  //register descriptions for get/return arguments
  vmiRegInfoCP resultLow;
  vmiRegInfoCP resultHigh;
  vmiRegInfoCP stackPointer;

  //memory information for mapping
  void* videoBuffer; //mapped to simulation
  void* videoBackBuffer; //used by library
  void* depthBuffer; //mapped to simulation
  void* depthBackBuffer; //used by library
  Uns32 videoAddress;
  Uns32 depthAddress;
  memDomainP videoDomain;
  memDomainP depthDomain;

  //input parameters
  uint32_t bigEndianGuest;
  
  //runtime variables
  freenect_context *f_ctx;
  freenect_device *f_dev;

  pthread_t streamer;
  int streamerState;

  //state variables
  uint32_t videoRequest;
  uint32_t depthRequest;
  uint32_t enabled;
  uint32_t videoOn;
  uint32_t depthOn;
  uint32_t continuous;

  //stats
  unsigned int videoFrames;
  unsigned int depthFrames;
  unsigned int droppedVideoFrames;
  unsigned int droppedDepthFrames;
} vmiosObject;

static void getArg(vmiProcessorP processor, vmiosObjectP object, Uns32 *index, void* result, Uns32 argSize) {
  memDomainP domain = vmirtGetProcessorDataDomain(processor);
  Uns32 spAddr;
  vmirtRegRead(processor, object->stackPointer, &spAddr);
  vmirtReadNByteDomain(domain, spAddr+*index+4, result, argSize, 0, MEM_AA_FALSE);
  *index += argSize;
}

#define GET_ARG(_PROCESSOR, _OBJECT, _INDEX, _ARG) getArg(_PROCESSOR, _OBJECT, &_INDEX, &_ARG, sizeof(_ARG))

inline static void retArg(vmiProcessorP processor, vmiosObjectP object, Uns64 result) {
  vmirtRegWrite(processor, object->resultLow, &result);
  result >>= 32;
  vmirtRegWrite(processor, object->resultHigh, &result);
}

void byteswapCopy(void* dst, void* src, int len) {
  uint32_t* s = (uint32_t*)src;
  uint32_t* sEnd = s + len/4;
  uint32_t* d = (uint32_t*)dst;
  while( s != sEnd)
    *d++ = bswap_32(*s++);
}

void callback_video(freenect_device *dev, void *video, uint32_t timestamp) {
  vmiosObjectP object = (vmiosObjectP)freenect_get_user(dev);
  if( !object->continuous ) { //in continuous mode, library operates directly on mapped buffer -> no copy necessary
    if( object->videoRequest ) {
      if( object->bigEndianGuest )
        byteswapCopy(object->videoBuffer, object->videoBackBuffer, KINECT_VIDEO_BUFFER_SIZE);
      else
        memcpy(object->videoBuffer, object->videoBackBuffer, KINECT_VIDEO_BUFFER_SIZE);
      object->videoRequest = 0;
    } else
      object->droppedVideoFrames++;
  }
  object->videoFrames++;
}

void callback_depth(freenect_device *dev, void *depth, uint32_t timestamp) {
  vmiosObjectP object = (vmiosObjectP)freenect_get_user(dev);
  if( !object->continuous ) { //in continuous mode, library operates directly on mapped buffer -> no copy necessary
    if( object->depthRequest ) {
      if( object->bigEndianGuest ) //if byte swapping is needed, to copy & swap at once
        byteswapCopy(object->depthBuffer, object->depthBackBuffer, KINECT_DEPTH_BUFFER_SIZE);
      else
        memcpy(object->depthBuffer, object->depthBackBuffer, KINECT_DEPTH_BUFFER_SIZE);
      object->depthRequest = 0;
    } else
      object->depthFrames++;
  }
  object->depthFrames++;
}

static void* streamerThread(void* objectV) {
  vmiosObjectP object = (vmiosObjectP)objectV;
  object->streamerState = 1; //thread is running

  vmiMessage("I", "KINECT_SH", "Streamer thread started, processing USB events");

  struct timeval tv = { .tv_sec = 5, .tv_usec = 0 };
  while( freenect_process_events_timeout(object->f_ctx, &tv) == 0 && object->streamerState == 1 ); //usb event loop

  if( object->streamerState == 1 ) //should still be running
    vmiMessage("W", "KINECT_SH", "Streamer thread stopped unexpectedly. USB error?");

  freenect_stop_video(object->f_dev);
  object->videoOn = 0;
  freenect_stop_depth(object->f_dev);
  object->depthOn = 0;

  object->streamerState = 0; //thread is stopped
  pthread_exit(0);
}

memDomainP getSimulatedVmemDomain(vmiProcessorP processor, char* name) {
  Addr lo, hi;
  Bool isMaster, isDynamic;
  memDomainP simDomain = vmipsePlatformPortAttributes(processor, name, &lo, &hi, &isMaster, &isDynamic);
  if( !simDomain )
    vmiMessage("F", "KINECT_SH", "Failed to obtain %s platform port", name);
  return simDomain;
}

static VMIOS_INTERCEPT_FN(initKinect) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 index = 0, kinectIndex = 0;
  GET_ARG(processor, object, index, kinectIndex);
  GET_ARG(processor, object, index, object->bigEndianGuest);

  vmiMessage("I", "KINECT_SH", "Using kinect number %d\n", kinectIndex);

  if( freenect_init(&object->f_ctx, 0) < 0 ) {
    vmiMessage("W", "KINECT_SH", "Failed to initialize freenect");
    retArg(processor, object, 1); //return failure
    return;
  }

  freenect_set_log_level(object->f_ctx, FREENECT_LOG_DEBUG); //FREENECT_LOG_WARNING
  freenect_select_subdevices(object->f_ctx, (freenect_device_flags)(FREENECT_DEVICE_CAMERA)); //we only use the camera for now

  int nr_devices = freenect_num_devices(object->f_ctx);

  if( nr_devices < 1 ) {
    vmiMessage("W", "KINECT_SH", "No Kinect devices found");
    retArg(processor, object, 1); //return failure
    return;
  }

  if( freenect_open_device(object->f_ctx, &object->f_dev, kinectIndex) < 0 ) {
    vmiMessage("W", "KINECT_SH", "Failed to open Kinect device");
    freenect_shutdown(object->f_ctx);
    retArg(processor, object, 1); //return failure
    return;
  }
  vmiMessage("I", "KINECT_SH", "Kinect opened\n");

  object->videoBuffer = calloc(1, KINECT_VIDEO_BUFFER_SIZE);
  object->videoBackBuffer = calloc(1, KINECT_VIDEO_BUFFER_SIZE);
  object->depthBuffer = calloc(1, KINECT_DEPTH_BUFFER_SIZE);
  object->depthBackBuffer = calloc(1, KINECT_DEPTH_BUFFER_SIZE);
  object->videoDomain = getSimulatedVmemDomain(processor, KINECT_VIDEO_BUS_NAME); //to check if bus is connected
  object->depthDomain = getSimulatedVmemDomain(processor, KINECT_DEPTH_BUS_NAME); //function will throw error if not
  if( !object->videoBuffer || !object->depthBuffer || !object->videoBackBuffer || !object->depthBackBuffer ) {
    vmiMessage("W", "KINECT_SH", "Failed to allocate internal framebuffers");
    retArg(processor, object, 1); //return failure
    return;
  }

  vmiMessage("I", "KINECT_SH", "Configuring libfreenect...\n");
  freenect_set_video_buffer(object->f_dev, object->videoBackBuffer);
  freenect_set_depth_buffer(object->f_dev, object->depthBackBuffer);
  freenect_set_video_callback(object->f_dev, callback_video);
  freenect_set_depth_callback(object->f_dev, callback_depth);
  freenect_set_video_mode(object->f_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_VIDEO_RGB));
  freenect_set_depth_mode(object->f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, FREENECT_DEPTH_11BIT));
  freenect_set_user(object->f_dev, object);
  freenect_set_led(object->f_dev,LED_YELLOW);

  vmiMessage("I", "KINECT_SH", "Launching streamer thread");
  pthread_create(&object->streamer, 0, streamerThread, (void*)object);

  retArg(processor, object, 0); //return success
}

static VMIOS_INTERCEPT_FN(configure) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 index = 0, enable = 0, videoOn = 0, depthOn = 0, continuous = 0;
  GET_ARG(processor, object, index, enable);
  GET_ARG(processor, object, index, videoOn);
  GET_ARG(processor, object, index, depthOn);
  GET_ARG(processor, object, index, continuous);

  vmiMessage("I", "KINECT_SH", "Configure enable %d video %d depth %d continuous %d", enable, videoOn, depthOn, continuous);

  if( !enable )
    depthOn = videoOn = 0; //disable both streams on disable

  if( videoOn != object->videoOn ) {
    if( videoOn )
      freenect_start_video(object->f_dev);
    else
      freenect_stop_video(object->f_dev);
    object->videoOn = videoOn;
  }

  if( depthOn != object->depthOn ) {
    if( depthOn )
      freenect_start_depth(object->f_dev);
    else
      freenect_stop_depth(object->f_dev);
    object->depthOn = depthOn;
  }

  object->continuous = continuous;

  freenect_set_led(object->f_dev,LED_GREEN);
}

static VMIOS_INTERCEPT_FN(setFormats) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 index = 0, videoFormat = 0, depthFormat = 0;
  GET_ARG(processor, object, index, videoFormat);
  GET_ARG(processor, object, index, depthFormat);

  vmiMessage("I", "KINECT_SH", "Updating video format to %d and depth format to %d", videoFormat, depthFormat);
  freenect_set_video_mode(object->f_dev, freenect_find_video_mode(FREENECT_RESOLUTION_MEDIUM, videoFormat));
  freenect_set_depth_mode(object->f_dev, freenect_find_depth_mode(FREENECT_RESOLUTION_MEDIUM, depthFormat));
}

Uns32 mapMemoryHandler(memDomainP domain, Uns32 oldAddress, Uns32 newAddress, void* buffer, Uns32 size) {
  if( oldAddress ) {
    vmiMessage("I", "KINECT_SH", "Unaliasing previously mapped memory at 0x%08x", oldAddress);
    vmirtUnaliasMemory(domain, oldAddress, oldAddress+size-1);
    vmirtMapMemory(domain, oldAddress, oldAddress+size-1, MEM_RAM);
    vmirtWriteNByteDomain(domain, oldAddress, buffer, size, 0, MEM_AA_FALSE);
  }
  vmirtReadNByteDomain(domain, newAddress, buffer, size, 0, MEM_AA_FALSE);
  vmirtMapNativeMemory(domain, newAddress, newAddress+size-1, buffer);
  return newAddress;
}

static VMIOS_INTERCEPT_FN(mapMemory) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 index = 0, newVideoAddress = 0, newDepthAddress = 0;
  GET_ARG(processor, object, index, newVideoAddress);
  GET_ARG(processor, object, index, newDepthAddress);

  object->videoAddress = mapMemoryHandler(object->videoDomain, object->videoAddress, newVideoAddress, object->videoBuffer, KINECT_VIDEO_BUFFER_SIZE);
  vmiMessage("I", "KINECT_SH", "Mapped external video buffer to addr 0x%08x", newVideoAddress);
  object->depthAddress = mapMemoryHandler(object->depthDomain, object->depthAddress, newDepthAddress, object->depthBuffer, KINECT_DEPTH_BUFFER_SIZE);
  vmiMessage("I", "KINECT_SH", "Mapped external depth buffer to addr 0x%08x", newDepthAddress);
}

static VMIOS_INTERCEPT_FN(requestFrame) {
  (void)thisPC; (void)context; (void)userData; (void)atOpaqueIntercept;
  Uns32 request = 0, index = 0;
  GET_ARG(processor, object, index, request);
  if( request )
    object->videoRequest = object->depthRequest = 1;
  retArg(processor, object, object->videoRequest || object->depthRequest);
}

static VMIOS_CONSTRUCTOR_FN(constructor) {
  (void)parameterValues;
  vmiMessage("I" ,"KINECT_SH", "Constructing");

  const char *procType = vmirtProcessorType(processor);
  memEndian endian = vmirtGetProcessorDataEndian(processor);

  if( strcmp(procType, "pse") )
      vmiMessage("F", "KINECT_SH", "Processor must be a PSE (is %s)\n", procType);
  if( endian != MEM_ENDIAN_NATIVE )
      vmiMessage("F", "KINECT_SH", "Host processor must same endianity as a PSE\n");

  //get register description and store to object for further use
  object->resultLow = vmirtGetRegByName(processor, "eax");
  object->resultHigh = vmirtGetRegByName(processor, "edx");
  object->stackPointer = vmirtGetRegByName(processor, "esp");
  //TODO get guest endianess directly from the simulated processor instead of a formal? is this possible?

  //initialize object
  object->videoBuffer = 0;
  object->videoBackBuffer = 0;
  object->depthBuffer = 0;
  object->depthBackBuffer = 0;
  object->videoAddress = 0;
  object->depthAddress = 0;
  object->videoDomain = 0;
  object->depthDomain = 0;
  object->bigEndianGuest = 0;
  object->f_ctx = 0;
  object->f_dev = 0;
  object->streamer = 0;
  object->streamerState = 0;
  object->videoRequest = 0;
  object->depthRequest = 0;
  object->enabled = 0;
  object->videoOn = 0;
  object->depthOn = 0;
  object->continuous = 0;
  object->videoFrames = 0;
  object->depthFrames = 0;
  object->droppedVideoFrames = 0;
  object->droppedDepthFrames = 0;
}

static VMIOS_DESTRUCTOR_FN(destructor) {
  (void)processor;
  vmiMessage("I" ,"KINECT_SH", "Shutting down");
  object->streamerState = 2; //set stop condition
  pthread_join(object->streamer, 0);
  if( freenect_close_device(object->f_dev) )
    vmiMessage("W", "KINECT_SH", "Failed to close Kinect device");
  if( freenect_shutdown(object->f_ctx) )
    vmiMessage("W", "KINECT_SH", "Failed to close Kinect device");
  if( object->videoBuffer )
    free(object->videoBuffer);
  if( object->depthBuffer )
    free(object->depthBuffer);
  if( object->videoBackBuffer )
    free(object->videoBackBuffer);
  if( object->depthBackBuffer )
    free(object->depthBackBuffer);
  vmiMessage("I", "KINECT_SH", "Shutdown complete: %d video / %d depth frames captured, thereof %d / %d frames dropped", object->videoFrames, object->depthFrames, object->droppedVideoFrames, object->droppedDepthFrames);
}

vmiosAttr modelAttrs = {
    .versionString  = VMI_VERSION,                // version string (THIS MUST BE FIRST)
    .modelType      = VMI_INTERCEPT_LIBRARY,      // type
    .packageName    = "kinectinput",              // description
    .objectSize     = sizeof(vmiosObject),        // size in bytes of object

    .constructorCB  = constructor,                // object constructor
    .destructorCB   = destructor,                 // object destructor

    .morphCB        = 0,                          // morph callback
    .nextPCCB       = 0,                          // get next instruction address
    .disCB          = 0,                          // disassemble instruction

    // -------------------          -------- ------  -----------------
    // Name                         Address  Opaque  Callback
    // -------------------          -------- ------  -----------------
    .intercepts = {
        {"initKinect",         0,       True,   initKinect,   0, 0 },
        {"configure",          0,       True,   configure,    0, 0 },
        {"setFormats",         0,       True,   setFormats,   0, 0 },
        {"mapMemory",          0,       True,   mapMemory,    0, 0 },
        {"requestFrame",       0,       True,   requestFrame, 0, 0 },
        {0}
    }
};
