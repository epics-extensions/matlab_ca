# Matlab 'mex' must be in the path,
# and you have to either configure the following
# or set them as environment variables.

# For Windows, use cmd.exe as shell
ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
SHELL = cmd.exe
.SHELLFLAGS = /c
endif

EPICS_HOST_ARCH = windows-x64
EPICS_BASE = C:\Users\***\epics-base
EPICS_EXTENSIONS = C:\Users\***\epics-modules
MEX="C:\Program Files\MATLAB\R2022b\bin\mex.bat"
	
ifeq (darwin, $(findstring darwin,$(EPICS_HOST_ARCH)))
OS_CLASS = Darwin
CMPLR_CLASS = clang
MEXOUT = mexmac
# For Octave:
# MEXOUT = mex
# MEX=mkoctfile --mex
endif

ifeq (linux, $(findstring linux,$(EPICS_HOST_ARCH)))
OS_CLASS = Linux
CMPLR_CLASS = gcc
MEXOUT = mexglx
endif

ifeq (solaris, $(findstring solaris,$(EPICS_HOST_ARCH)))
OS_CLASS = solaris
MEXOUT = mexglx
endif

ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
OS_CLASS = WIN32
CMPLR_CLASS = msvc
MEXOUT = mexw64
endif

ifndef MEX
MEX=mex
endif

ifndef MEXOUT
$(error Check the Makefile, handle your EPICS_HOST_ARCH)
endif

all:    matlab

OUT=O.$(EPICS_HOST_ARCH)

matlab: $(OUT) $(OUT)/mca.$(MEXOUT)

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
ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
# Windows-specific include paths
FLAGS += -I$(EPICS_BASE)\include\compiler\msvc
FLAGS += -I$(EPICS_BASE)\include\os\WIN32
else
# Unix-like systems
FLAGS += -I$(EPICS_BASE)/include/compiler/$(CMPLR_CLASS)
FLAGS += -I$(EPICS_BASE)/include/os/$(OS_CLASS)
endif
FLAGS += -DEPICS_DLL_NO

# Windows-specific defines
ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
FLAGS += -DDB_TEXT_GLBLSOURCE -DCA_ERROR_GLBLSOURCE -DMSCC -DWIN64 -DWIN32
endif

# Libraries ------------------------------------------
# EPICS Base
ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
FLAGS += -L$(EPICS_BASE)\lib\$(EPICS_HOST_ARCH)
# Try basic libraries first
FLAGS += -lCom -lca
else
FLAGS += -L$(EPICS_BASE)/lib/$(EPICS_HOST_ARCH)
FLAGS += -lCom -lca
endif
# No longer needed?
#FLAGS += -ldbStaticHost

# Define platform-specific commands
ifeq (windows, $(findstring windows,$(EPICS_HOST_ARCH)))
MKDIR_CMD = powershell -Command "if (!(Test-Path '$(OUT)')) { New-Item -ItemType Directory -Path '$(OUT)' }"
COPY_CMD = powershell -Command "Copy-Item '$(OUT)\mca.$(MEXOUT)' '$(EPICS_EXTENSIONS)\lib\$(EPICS_HOST_ARCH)\'"
CLEAN_CMD = powershell -Command "if (Test-Path '$(OUT)') { Remove-Item -Recurse -Force '$(OUT)' }"
else
MKDIR_CMD = mkdir -p $(OUT)
COPY_CMD = cp $(OUT)/mca.$(MEXOUT) $(EPICS_EXTENSIONS)/lib/$(EPICS_HOST_ARCH)
CLEAN_CMD = rm -rf $(OUT)
endif

$(OUT):
	$(MKDIR_CMD)

$(OUT)/mca.$(MEXOUT): mca.cpp MCAError.cpp Channel.cpp
	$(MEX) $(FLAGS) mca.cpp MCAError.cpp Channel.cpp -output $(OUT)/mca

install: matlab
	$(COPY_CMD)

clean:
	-$(CLEAN_CMD)

rebuild: clean all

tar: clean
	cd ..;cp -r mca /tmp
	cd /tmp;rm -f mca/mexopts.sh
	cd /tmp;rm -rf mca/.settings/CVS
	cd /tmp;rm -rf mca/alt_compile/CVS
	cd /tmp;rm -rf mca/CVS
	cd /tmp;rm -rf mca/examples/CVS
	cd /tmp;rm -rf mca/matlab/CVS
	cd /tmp;rm -rf mca/matunit/CVS
	cd /tmp;rm -rf mca/tests/CVS
	cd /tmp;tar zcf mca.tgz mca
	cd /tmp;rm -rf mca
