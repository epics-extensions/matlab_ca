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
FLAGS = -v

# Includes -------------------------------------------
# EPICS Base
FLAGS += -I$(EPICS_BASE_RELEASE)/include
FLAGS += -I$(EPICS_BASE_RELEASE)/include/os/$(HOST_ARCH)
FLAGS += -DEPICS_DLL_NO

# Libraries ------------------------------------------
# EPICS Base
FLAGS += -L$(EPICS_BASE_RELEASE)/lib/$(EPICS_HOST_ARCH) -ldbStaticHost -lCom -lca

$(OUT):
	mkdir $(OUT)

$(OUT)/mca.mexglx: mca.cpp MCAError.cpp Channel.cpp ChannelAccess.cpp
	mex $(FLAGS) mca.cpp MCAError.cpp Channel.cpp ChannelAccess.cpp -o $(OUT)/mca.mexglx

install:
	cp O.$(EPICS_HOST_ARCH)/mca.mexglx $(EPICS_EXTENSIONS)/lib/$(EPICS_HOST_ARCH)

clean:
	-rm -rf $(OUT)

rebuild: clean all
