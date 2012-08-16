-include Makefile.base

EXTRA_LIBS			= libeepp.a

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
	$(CPP) -o ./$(EXE) $(FOBJHELPERS) $(FOBJMODULES) $(FOBJTEST) $(LDFLAGS) $(EXTRA_LIBS) $(LIBS)

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
