ifeq ($(shell uname),Darwin)
    PLATFORM = MacOS
else
    PLATFORM = Linux
endif

# C++ compiler.
CXX := g++ -std=c++11

ifndef CONFIG
    CONFIG := Release
endif

# The name of your application.
APP_NAME := blocks.pd_darwin

# The path to temporary build files.
OBJECT_DIR := build/$(CONFIG)

JUCE_OUTDIR := build/$(PLATFORM)
JUCE_OBJDIR := build/$(CONFIG)

JUCE_INCLUDES := -IBLOCKS-SDK/SDK
JUCE_SDKDEFINES := -DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 -DJUCE_STANDALONE_APPLICATION=1

JUCE_CXXFLAGS = -std=c++11 $(DEPFLAGS) -march=native $(JUCE_SDKDEFINES) $(JUCE_INCLUDES)

ifeq ($(PLATFORM),MacOS)
	APP_NAME := blocks.pd_darwin
	LIBS := -framework Cocoa -framework CoreAudio -framework CoreMIDI -framework Accelerate -framework AudioToolbox
	LDFLAGS := -undefined dynamic_lookup
	SUFFIX := mm
else
  APP_NAME := blocks.pd_linux
	LIBS := -L/usr/X11R6/lib/ $(shell pkg-config --libs alsa libcurl x11) -ldl -lpthread -lrt
	JUCE_CXXFLAGS += -DLINUX=1
	LDFLAGS := -export-dynamic -shared
	SUFFIX := cpp
endif


ifeq ($(CONFIG),Debug)
  JUCE_CXXFLAGS += -DDEBUG=1 -D_DEBUG=1 -g -ggdb -O0
endif

ifeq ($(CONFIG),Release)
  JUCE_CXXFLAGS += -DNDEBUG=1 -Os
endif

JUCE_MODULES := juce_audio_basics juce_audio_devices juce_core juce_events
JUCE_SOURCE := $(foreach MODULE_NAME,$(JUCE_MODULES),../BLOCKS-SDK-master/SDK/$(MODULE_NAME)/$(MODULE_NAME).cpp)
JUCE_OBJECTS := $(foreach MODULE_NAME,$(JUCE_MODULES),$(JUCE_OBJDIR)/juce/$(MODULE_NAME).o)
JUCE_OBJECTS += $(JUCE_OBJDIR)/blocks/juce_blocks_basics.o

SOURCE_FILES := JuceThread BlockFinder BlockComponent LightpadProgram blocks
JUCE_OBJECTS += $(foreach SOURCE_FILE, $(SOURCE_FILES), $(JUCE_OBJDIR)/external/$(SOURCE_FILE).o)

VPATH:= $(foreach MODULE_NAME,$(JUCE_MODULES),BLOCKS-SDK/SDK/$(MODULE_NAME))
VPATH+= BLOCKS-SDK/SDK/juce_blocks_basics

##############################################################################
# Build rules                                                                #
##############################################################################

.PHONY: clean

$(JUCE_OUTDIR)/$(APP_NAME): $(JUCE_OBJECTS)
	@mkdir -p $(dir $@)
	$(CXX) $(LIBS) $^ -o $@ $(LDFLAGS)
	rm -rf $(JUCE_OBJDIR)

$(JUCE_OBJDIR)/external/%.o: %.mm
	@mkdir -p $(dir $@)
	$(CXX) $(JUCE_CXXFLAGS) -o $@ -c $<

$(JUCE_OBJDIR)/external/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(JUCE_CXXFLAGS) -o $@ -c $<

$(JUCE_OBJDIR)/blocks/%.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(JUCE_CXXFLAGS) -o $@ -c $<

$(JUCE_OBJDIR)/juce/%.o: %.$(SUFFIX)
	@mkdir -p $(dir $@)
	$(CXX) $(JUCE_CXXFLAGS) -o $@ -c $<

clean:
	rm -rf $(JUCE_OBJDIR)
