#include <peripheral/bhm.h>
#include <string.h>

#include "byteswap.h"
#include "kinectinput.igen.h"
#include "../kinect-cfg.h"

typedef _Bool bool;
static bool bigEndianGuest;

Uns32 initKinect(Uns32 id, Uns32 bigEndianGuest) {
  bhmMessage("F", "KINECT_PSE", "Failed to intercept : initKinect");
  return 0;
}

void mapMemory(Uns32 video, Uns32 depth) {
  bhmMessage("F", "KINECT_PSE", "Failed to intercept : mapMemory");
}

void configure(Uns32 enable, Uns32 videoOn, Uns32 depthOn, Uns32 continuous) {
  bhmMessage("F", "KINECT_PSE", "Failed to intercept : configure");
}

//requests a new frame if request==1, in any case returns if the last requested frame is ready
Uns32 requestFrame(Uns32 request) {
  bhmMessage("F", "KINECT_PSE", "Failed to intercept : requestFrame");
  return 0;
}

void setFormats(Uns32 videoFormat, Uns32 depthFormat) {
  bhmMessage("F", "KINECT_PSE", "Failed to intercept : setFormats");
}

PPM_REG_READ_CB(readReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : //configuration register
      //bhmMessage("I", "KINECT_PSE", "Read from CR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      CFGBUS_AB0_data.CR.value = (CFGBUS_AB0_data.CR.value & ~KINECT_CR_REQUEST_MASK) | (requestFrame(0) << KINECT_CR_REQUEST_OFFSET); //fetch current frame request state from semihost
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.CR.value) : CFGBUS_AB0_data.CR.value;
      break;
    case 1 : //video address register
      //bhmMessage("I", "KINECT_PSE", "Read from VIDEO_AR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.VIDEO_AR.value) : CFGBUS_AB0_data.VIDEO_AR.value;
      break;
    case 2 : //depth address register
      //bhmMessage("I", "KINECT_PSE", "Read from DEPTH_AR for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.DEPTH_AR.value) : CFGBUS_AB0_data.DEPTH_AR.value;
      break;
    case 3 : //format register
      //bhmMessage("I", "KINECT_PSE", "Read from FMTREG for %d bytes at 0x%08x", bytes, (Uns32)addr);
      return bigEndianGuest ? bswap_32(CFGBUS_AB0_data.FMTREG.value) : CFGBUS_AB0_data.FMTREG.value;
      break;
    default:
      bhmMessage("W", "KINECT_PSE", "Invalid user data on readReg\n");
  }

  return 0;
}

PPM_REG_WRITE_CB(writeReg) {
  Uns32 reg = (Uns32)user;
  switch( reg ) {
    case 0 : { //configuration register
      //bhmMessage("I", "KINECT_PSE", "Write to CR at 0x%08x data 0x%08x", (Uns32)addr, data);
      Uns32 newdata = bigEndianGuest ? bswap_32(data) : data;
      if( newdata & KINECT_CR_REQUEST_MASK )
        requestFrame(1); //request a new frame if appropriate bit is set
      newdata &= (~KINECT_CR_REQUEST_MASK); //mask out request bit
      if( newdata != CFGBUS_AB0_data.CR.value ) //if settings are changed, push settings to semihost
        configure((newdata >> KINECT_CR_ENABLED_OFFSET) & 1,
                  (newdata >> KINECT_CR_VIDEO_OFFSET) & 1,
                  (newdata >> KINECT_CR_DEPTH_OFFSET) & 1,
                  (newdata >> KINECT_CR_CONTINUOUS_OFFSET) & 1);
      CFGBUS_AB0_data.CR.value = newdata; //save settings
      break;
    }
    case 1 : { //video address register
      //bhmMessage("I", "KINECT_PSE", "Write to VIDEO_AR at 0x%08x data 0x%08x", (Uns32)addr, data);
      Uns32 newvaddr = bigEndianGuest ? bswap_32(data) : data;
      if( newvaddr != CFGBUS_AB0_data.VIDEO_AR.value )
        mapMemory(newvaddr, CFGBUS_AB0_data.DEPTH_AR.value);
      CFGBUS_AB0_data.VIDEO_AR.value = newvaddr;
      break;
    }
    case 2 : { //depth address register
      //bhmMessage("I", "KINECT_PSE", "Write to DEPTH_AR at 0x%08x data 0x%08x", (Uns32)addr, data);
      Uns32 newdaddr = bigEndianGuest ? bswap_32(data) : data;
      if( newdaddr != CFGBUS_AB0_data.DEPTH_AR.value )
        mapMemory(CFGBUS_AB0_data.VIDEO_AR.value, newdaddr);
      CFGBUS_AB0_data.DEPTH_AR.value = newdaddr;
      break;
    }
    case 3 : { //format register
      //bhmMessage("I", "KINECT_PSE", "Write to FMTREG at 0x%08x data 0x%08x", (Uns32)addr, data);
      Uns32 newfmts = bigEndianGuest ? bswap_32(data) : data;
      if( newfmts != CFGBUS_AB0_data.FMTREG.value )
        setFormats(newfmts & KINECT_FMTREG_VIDEO_MASK, newfmts >> KINECT_FMTREG_DEPTH_OFFSET);
      CFGBUS_AB0_data.FMTREG.value = newfmts;
      break;
    }
    default:
      bhmMessage("W", "KINECT_PSE", "Invalid user data on writeReg\n");
  }
}

PPM_CONSTRUCTOR_CB(constructor) {
  bhmMessage("I", "KINECT_PSE", "Constructing");
  periphConstructor();

  bigEndianGuest = bhmBoolAttribute("bigEndianGuest");
  Uns32 kinectIndex;
  if( !bhmIntegerAttribute("kinectIndex", &kinectIndex) ) {
    kinectIndex = KINECT_DEFAULT_INDEX;
    bhmMessage("W", "TFT_PSE", "Could not read kinectIndex parameter, using default (%d)", KINECT_DEFAULT_INDEX);
  }

  bhmMessage("I", "KINECT_PSE", "Initializing Kinect");
  Uns32 error = initKinect(kinectIndex, bigEndianGuest); //do byte swapping when guest is big endian

  if( !error )
    bhmMessage("I", "KINECT_PSE", "Kinect initialized successfully");
  else
    bhmMessage("F", "KINECT_PSE", "Failed to initialize Kinect");

  //initialize registers to reset values
  CFGBUS_AB0_data.CR.value = (1 << KINECT_CR_ENABLED_OFFSET) |
			     (1 << KINECT_CR_VIDEO_OFFSET) |
                             (0 << KINECT_CR_DEPTH_OFFSET) |
                             (0 << KINECT_CR_CONTINUOUS_OFFSET);
  configure((CFGBUS_AB0_data.CR.value & KINECT_CR_ENABLED_MASK) >> KINECT_CR_ENABLED_OFFSET,
            (CFGBUS_AB0_data.CR.value & KINECT_CR_VIDEO_MASK) >> KINECT_CR_VIDEO_OFFSET,
            (CFGBUS_AB0_data.CR.value & KINECT_CR_DEPTH_MASK) >> KINECT_CR_DEPTH_OFFSET,
            (CFGBUS_AB0_data.CR.value & KINECT_CR_CONTINUOUS_MASK) >> KINECT_CR_CONTINUOUS_OFFSET);

  CFGBUS_AB0_data.VIDEO_AR.value = KINECT_VIDEO_DEFAULT_ADDRESS;
  CFGBUS_AB0_data.DEPTH_AR.value = KINECT_DEPTH_DEFAULT_ADDRESS;
  CFGBUS_AB0_data.FMTREG.value = KINECT_VIDEO_RGB | KINECT_DEPTH_11BIT << KINECT_FMTREG_DEPTH_OFFSET;
  mapMemory(CFGBUS_AB0_data.VIDEO_AR.value, CFGBUS_AB0_data.DEPTH_AR.value); //initial memory mapping
}

PPM_DESTRUCTOR_CB(destructor) {
  bhmMessage("I", "KINECT_PSE", "Destructing");
}

