imodelnewperipheral -name kinectinput -vendor itiv.kit.edu -library peripheral -version 1.0 -constructor constructor -destructor destructor
iadddocumentation   -name Licensing   -text "Open Source Apache 2.0"
iadddocumentation   -name Description -text "Kinect to OVP input peripheral"

  imodeladdbusslaveport -name CFGBUS -size 16
  imodeladdaddressblock -name AB0          -width 32     -offset 0 -size 16
  imodeladdmmregister   -name CR           -offset 0     -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name ENABLED    -bitoffset 0  -width 1  -access rw -mmregister CFGBUS/AB0/CR
      iadddocumentation -name Description  -text "Enable streaming"
    imodeladdfield        -name VIDEO      -bitoffset 1  -width 1  -access rw -mmregister CFGBUS/AB0/CR
      iadddocumentation -name Description  -text "Enable video streaming (default on)"
    imodeladdfield        -name DEPTH      -bitoffset 2  -width 1  -access rw -mmregister CFGBUS/AB0/CR
      iadddocumentation -name Description  -text "Enable depth streaming (default off)"
    imodeladdfield        -name CONTINUOUS -bitoffset 3  -width 1  -access rw -mmregister CFGBUS/AB0/CR
      iadddocumentation -name Description  -text "Enable continuous streaming, no frame requests necessary"
    imodeladdfield        -name REQUEST    -bitoffset 31 -width 1 -access rw -mmregister CFGBUS/AB0/CR
      iadddocumentation -name Description  -text "Request a new frame (video + depth) (no continuous streaming)"
  imodeladdmmregister   -name VIDEO_AR     -offset 4     -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name BASEADDR   -bitoffset 0  -width 32 -access rw -mmregister CFGBUS/AB0/VIDEO_AR
      iadddocumentation -name Description  -text "Video framebuffer target address in host memory"
  imodeladdmmregister   -name DEPTH_AR     -offset 8     -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name BASEADDR   -bitoffset 0  -width 32 -access rw -mmregister CFGBUS/AB0/DEPTH_AR
      iadddocumentation -name Description  -text "Depth framebuffer target address in host memory"
  imodeladdmmregister   -name FMTREG       -offset 12     -width 32 -access rw -readfunction readReg -writefunction writeReg -userdata 0
    imodeladdfield        -name VIDEOFMT   -bitoffset 0  -width 16 -access rw -mmregister CFGBUS/AB0/FMTREG
      iadddocumentation -name Description  -text "Video format (libfreenect index)"
    imodeladdfield        -name DEPTHFMT   -bitoffset 16 -width 16 -access rw -mmregister CFGBUS/AB0/FMTREG
      iadddocumentation -name Description  -text "Depth format (libfreenect index)"

  imodeladdbusslaveport -name VIDEOBUS -size 0xe1000 -remappable -mustbeconnected
    iadddocumentation -name Description -text "Video memory, sized 640x480x3Byte=921600Byte"
  imodeladdbusslaveport -name DEPTHBUS -size 0x96000 -remappable -mustbeconnected
    iadddocumentation -name Description -text "Depth memory, sized 640x480x2Byte=614400Byte"

  imodeladdformal -name "kinectIndex" -type uns32
  imodeladdformal -name "bigEndianGuest" -type bool
