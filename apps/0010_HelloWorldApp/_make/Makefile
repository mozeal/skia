#
# Cross Platform Makefile
# Compatible with MSYS2/MINGW, Ubuntu 14.04.1 and Mac OS X
#
# You will need SDL2 (http://www.libsdl.org):
# Linux:
#   apt-get install libsdl2-dev libsdl2-mixer-dev
# Mac OS X:
#   brew install sdl2
# MSYS2:
#   pacman -S mingw-w64-i686-SDL2
#

#CXX = g++
#CXX = clang++

EXE = HelloWorldApp
OPTIONS = 

UNAME_S := $(shell uname -s)
UNAME_M := $(shell uname -m)

OBJ_DIR = obj

LIB_DIR = lib

ROOT_DIR = ../../..

LIBS_DIR = 
OUT_DIR = $(ROOT_DIR)/out
TOOLS_DIR = $(ROOT_DIR)/tools
THIRDPARTY_DIR = $(ROOT_DIR)/third_party

EXTERNALS_DIR = $(THIRDPARTY_DIR)/externals
SKIA_DIR = $(ROOT_DIR)

APPS_DIR = ../..
APP_ROOT_DIR = ..
APP_SRC_DIR = $(APP_ROOT_DIR)/src

GLAD_DIR = $(ROOT_DIR)/apps/glad

SOURCES = \
	$(APP_SRC_DIR)/HelloWorld.cpp

CXXFLAGS += -Wall -Wformat -Wattributes \
          -I$(APP_ROOT_DIR) -I$(APP_SRC_DIR) -I$(SRC_DIR) -I$(ROOT_DIR)/src -I$(APPS_DIR) 
	  
CPPFLAGS = -std=c++17
CFLAGS = 
BUILD = 
CPU = 
PLATFORM =
ARM = armv6

LIBS = 

##---------------------------------------------------------------------
## glad
##---------------------------------------------------------------------
#Using OpenGL loader: GLAD2
#LOADER_GLAD=OPENGL_LOADER_GLAD

#LOADER_GLAD=OPENGLES2_LOADER_GLAD
#
#CXXFLAGS += -I$(GLAD_DIR)/include
#
#ifeq ($(LOADER_GLAD), OPENGL_LOADER_GLAD)
#	# 1. glad gl
#	CXXFLAGS += -DOPENGL_LOADER_GLAD=1
#	SOURCES += $(GLAD_DIR)src/gl.c
#endif
#
#ifeq ($(LOADER_GLAD), OPENGLES2_LOADER_GLAD)
#	# 2. glad egl, gles2
#	CXXFLAGS += -DOPENGLES2_LOADER_GLAD=1
#	SOURCES += $(GLAD_DIR)/src/egl.c 
#	SOURCES += $(GLAD_DIR)/src/gles2.c
#endif
#
##---------------------------------------------------------------------
## SDL
##---------------------------------------------------------------------

CXXFLAGS += -D__USE_SDL__=1 

##---------------------------------------------------------------------
## Skia
##---------------------------------------------------------------------

CXXFLAGS += -I$(SKIA_DIR) -DSK_GL -DGR_GL_CHECK_ERROR=0 -DGR_GL_LOG_CALLS=0 -D__USE_SKIA__=1
LIBS += -lskia -lskshaper -lskunicode -lskresources
#  -limgui -lsksg -lpathkit

##---------------------------------------------------------------------

OBJS = $(addsuffix .o, $(addprefix $(OBJ_DIR)/, $(basename $(notdir $(SOURCES)))))


##---------------------------------------------------------------------
## BUILD FLAGS PER PLATFORM
##---------------------------------------------------------------------

ifeq ($(UNAME_S), Linux) #LINUX
ifeq ($(UNAME_M), x86_64)
	ECHO_MESSAGE = "linux"
	CPU = x64
	PLATFORM = linux
	LIBS += -L$(ROOT_DIR)/out/linux-debug 
# -lEGL
else
	ifeq ($(UNAME_M), aarch64)
		CPU = arm64
		# ----------------------- Wayland ----------------------------
		LIBS += -L/usr/lib/aarch64-linux-gnu/vivante 
			
		#GBM
		CXXFLAGS += $(shell pkg-config --cflags gbm)
		LIBS += $(shell pkg-config --libs gbm)
	
		#EGL_WAYLAND (-DWL_EGL_PLATFORM => glad/egl.h)
		CXXFLAGS += $(shell pkg-config --cflags egl_wayland)
		LIBS += $(shell pkg-config --libs egl_wayland)
		
		#GLESv2_wl :
		CXXFLAGS += $(shell pkg-config --cflags glesv2_wl)
		LIBS += $(shell pkg-config --libs glesv2_wl)
		# --------------------------------------------------------------		
	else ifeq ($(UNAME_M), armv7l)
		CPU = armv7
	else
		CPU = $(UNAME_M)
	endif
	ECHO_MESSAGE = "Linux ARM"
	PLATFORM = linux
	LIBS += -L/usr/local/lib
	LIBS += -L$(LIBS_DIR)/$(PLATFORM)/$(CPU)/release -lSDL2
	CXXFLAGS +=  -D_REENTRANT
endif
	LIBS += -lGL -ldl -lX11 -lpthread -lfreetype -lfontconfig  -lpthread 
#-lEGL -lXext 

	CXXFLAGS += -Wno-unused-but-set-variable 
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(UNAME_S), Darwin) #APPLE
	ECHO_MESSAGE = "Mac OS X"
	PLATFORM = macos
	ifeq ($(UNAME_M), x86_64)
		CPU = x64
	else
		CPU = arm64
	endif

	LIBS += -framework Metal -framework MetalKit -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework Carbon
	LIBS += -framework AudioToolbox -framework CoreAudio -framework CoreFoundation -framework CoreHaptics -framework ForceFeedback
	LIBS += -framework GameController -framework QuartzCore -liconv -lexpat 
	LIBS += -L$(LIBS_DIR)/$(PLATFORM)/$(CPU)/release
	LIBS += -lSDL2 

	CXXFLAGS += -I/usr/local/include -I/opt/local/include
	CXXFLAGS += -fvisibility=hidden
	CFLAGS = $(CXXFLAGS)
endif

ifeq ($(findstring MINGW,$(UNAME_S)),MINGW)
	ECHO_MESSAGE = "MinGW"
	PLATFORM = windows
	
	LIBS += -lgdi32 -lopengl32 -limm32 
	LIBS += -L$(LIBS_DIR)/$(PLATFORM)/x64/release
	LIBS += `sdl2-config --libs` 

	CXXFLAGS += `sdl2-config --cflags` -D_THREAD_SAFE
	
	CFLAGS = $(CXXFLAGS)
endif

##---------------------------------------------------------------------
## BUILD RULES for App
##---------------------------------------------------------------------

$(OBJ_DIR)/%.o:$(APP_SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<	
	
$(OBJ_DIR)/%.o:$(GLAD_DIR)/src/%.c
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c -o $@ $<	

##---------------------------------------------------------------------
## BUILD RULES for SK_UI
##---------------------------------------------------------------------

release: CXXFLAGS += -O3 -g0
release: BUILD = release
release: $(EXE)

sim: CXXFLAGS += -D__USE_EDISON_SIMULATOR__=1
sim: CXXFLAGS += -O3 -g0
sim: BUILD = release
sim: $(EXE)

simdebug: CXXFLAGS += -D__USE_EDISON_SIMULATOR__=1
simdebug: CXXFLAGS += -DDEBUG -O0 -g3
simdebug: BUILD = debug
simdebug: $(EXE)

debug: CXXFLAGS += -DDEBUG -O0 -g3
debug: BUILD = debug
debug: $(EXE)

all: $(release)
	@echo Build complete for $(ECHO_MESSAGE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $^ $(CXXFLAGS) $(LIBS)

clean:
	rm -f $(EXE) $(OBJS) 

reset:
	rm ./imgui.ini

resources:
	ln -s ../../../resources .

