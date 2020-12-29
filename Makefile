TARGET = rfusb-tunnel

SRCDIR = src
OBJDIR = obj
LIBDIR = lib
INCDIR = include 

LIBS   = 
ARCHIVE = $(shell date +%Y%m%d)_$(TARGET).tgz

GIT = git
CC  ?= gcc
CXX ?= g++
CPP ?= cpp
LD   = $(CXX)
STRIP ?= strip

CINCS    = $(foreach d, $(INCDIR), -I$d)
CFLAGS   = -g -Wall -fPIC $(CINCS) -Wno-write-strings

LIBDIRS  = $(foreach d, $(LIBDIR), -L$d)
LDFLAGS  = $(LIBDIRS) $(LIBS)

SRCS := $(foreach FILE,$(shell find $(SRCDIR) -name *.cpp | xargs), \
	$(subst $(SRCDIR)/, , $(FILE)))

OBJS := $(addprefix $(OBJDIR)/,$(SRCS:.cpp=.o))

VERSION = $(shell $(GIT) describe --always)

################################################################################
# Targets
#

all: $(TARGET)

$(TARGET): init $(OBJS)
	$(LD) $(OBJS) -o $(TARGET) $(LDFLAGS)

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	$(CXX) $(CFLAGS) -c $< -o $@

init:
	@if [ ! -e $(OBJDIR) ]; then mkdir $(OBJDIR); fi;
	@$(foreach DIR,$(sort $(dir $(SRCS))), if [ ! -e $(OBJDIR)/$(DIR) ]; \
		then mkdir $(OBJDIR)/$(DIR); fi; )

version:
	sed 's/".*";$$/"$(VERSION)";/' src/version.template > src/version.cpp

clean:
	rm -rf $(OBJDIR)/*
	rm -rf $(TARGET)

