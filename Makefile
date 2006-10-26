# Matlab 'mex' must be in the path,
# and you have to either configure the following
# or set them as environment variables.

#EPICS_BASE = /Users/kasemir/epics/R3.14.8.2/base
#EPICS_EXTENSIONS = /Users/kasemir/epics/R3.14.8.2/extensions
#MEX=/Applications/MATLAB72/bin/mex

ifeq ($(EPICS_HOST_ARCH),darwin-ppc)
OS_CLASS = Darwin
MEXOUT = mexmac
endif

ifeq ($(EPICS_HOST_ARCH),linux-x86)
OS_CLASS = Linux
MEXOUT = mexglx
endif

ifndef MEX
MEX=mex
endif

ifndef MEXOUT
$(error Check the Makefile, handle your EPICS_HOST_ARCH)
endif

all:    matlab

OUT=O.$(EPICS_HOST_ARCH)

matlab: $(OUT) $(OUT)/mca.mexglx

# Matlab has a compilation tool called mex
# which handles all the magic.
# In theory, we only provide the include & link
# directives for EPICS.
# Even better: At least on Unix, the mex tool
# understands the -v, -I, -L & -l syntax.

# Do you want verbose compilation?
FLAGS += -v

# Includes -------------------------------------------
# EPICS Base
FLAGS += -I$(EPICS_BASE)/include
FLAGS += -I$(EPICS_BASE)/include/os/$(OS_CLASS)
FLAGS += -DEPICS_DLL_NO

# Libraries ------------------------------------------
# EPICS Base
FLAGS += -L$(EPICS_BASE)/lib/$(EPICS_HOST_ARCH) -ldbStaticHost -lCom -lca

$(OUT):
	mkdir $(OUT)

$(OUT)/mca.mexglx: mca.cpp MCAError.cpp Channel.cpp ChannelAccess.cpp
	$(MEX) $(FLAGS) mca.cpp MCAError.cpp Channel.cpp ChannelAccess.cpp -o $(OUT)/mca.$(MEXOUT)

install: matlab
	cp O.$(EPICS_HOST_ARCH)/mca.$(MEXOUT) $(EPICS_EXTENSIONS)/lib/$(EPICS_HOST_ARCH)

clean:
	-rm -rf $(OUT)

rebuild: clean all
