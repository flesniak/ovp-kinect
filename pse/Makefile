IMPERAS_HOME := $(shell getpath.exe "$(IMPERAS_HOME)")
include $(IMPERAS_HOME)/ImperasLib/buildutils/Makefile.pse

.PHONY += igen

igen:
	igen.exe -b kinectinput.tcl -writec kinectinput --overwrite
