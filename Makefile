-include Makefile.base

ifndef LIBS_PATH
  LIBS_PATH = /home/programming/eepp/libs/$(BUILD_OS)/
endif

ifeq ($(DEBUGBUILD), yes)
	DEBUGSTR		= -debug
else
	DEBUGSTR		=
endif

EXTRA_LIBS			= $(LIBS_PATH)libeepp-static$(DEBUGSTR).a $(LIBS_PATH)helpers/libchipmunk$(DEBUGSTR).a $(LIBS_PATH)helpers/libglew$(DEBUGSTR).a $(LIBS_PATH)helpers/libhaikuttf$(DEBUGSTR).a $(LIBS_PATH)helpers/libjpeg-compressor$(DEBUGSTR).a $(LIBS_PATH)helpers/liblibzip$(DEBUGSTR).a $(LIBS_PATH)helpers/libSOIL2$(DEBUGSTR).a $(LIBS_PATH)helpers/libstb_vorbis$(DEBUGSTR).a $(LIBS_PATH)helpers/libzlib$(DEBUGSTR).a

EXE     			= eeiv-$(RELEASETYPE)$(OSEXTENSION)
SRCTEST     		= $(wildcard ./src/*.cpp)
OBJTEST     		= $(SRCTEST:.cpp=.o)

ifeq ($(ARCH),)
OBJDIR				= obj/$(BUILD_OS)/$(RELEASETYPE)/
else
OBJDIR				= obj/$(BUILD_OS)/$(RELEASETYPE)/$(ARCH)/
endif

FOBJTEST     		= $(patsubst ./%, $(OBJDIR)%, $(SRCTEST:.cpp=.o) )
FOBJALL 			= $(FOBJTEST)
DEPSALL				= $(FOBJTEST:.o=.d)

all: game

dirs:
	@$(MKDIR) $(OBJDIR)/src/

$(FOBJTEST):
	$(CPP) -o $@ -c $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(CFLAGS) $(OTHERINC) $(DEBUGFLAGS) $(EXTRA_FLAGS)
	@$(CPP) -MT $@ -MM $(patsubst $(OBJDIR)%.o,%.cpp,$@) $(OTHERINC) > $(patsubst %.o,%.d,$@) $(DEBUGFLAGS) $(EXTRA_FLAGS)

$(EXE): $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST)
	$(CPP) -o ./$(EXE) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST) $(LDFLAGS) $(EXTRA_LIBS) $(DYLIBS)

os:
	@echo $(BUILD_OS)

objdir:
	@echo $(OBJDIR)

game: cleanbin dirs $(EXE)

docs:
	doxygen ./Doxyfile

clean:
	$(RM) $(FOBJALL) $(DEPSALL)

cleanbin:
	$(RM) ./$(EXE)

cleanall: clean
	$(RM) $(LIBNAME)
	$(RM) ./$(EXE)
	$(RM) ./log.log

depends:
	@echo $(DEPSALL)

install:
	@($(CP) $(LIBNAME) $(DESTLIBDIR) $(INSTALL))

-include $(DEPSALL)
