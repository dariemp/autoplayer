
CC            = gcc
LINK          = gcc
CFLAGS        = -pipe -O2 -pthread -Wall -W $(DEFINES)
CXXFLAGS      = -pipe -O2 -pthread -Wall -W $(DEFINES)
INCPATH       = -I. -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LFLAGS        = -Wl,-O1
LIBS          = -lgio-2.0 -lgobject-2.0 -lglib-2.0 
AR            = ar cqs
RANLIB        = 
TAR           = tar -cf
COMPRESS      = gzip -9f
COPY          = cp -f
SED           = sed
COPY_FILE     = $(COPY)
COPY_DIR      = $(COPY) -r
STRIP         = strip
INSTALL_FILE  = install -m 644 -p
INSTALL_DIR   = $(COPY_DIR)
INSTALL_PROGRAM = install -m 755 -p
DEL_FILE      = rm -f
SYMLINK       = ln -f -s
DEL_DIR       = rmdir
MOVE          = mv -f
CHK_DIR_EXISTS= test -d
MKDIR         = mkdir -p

####### Output directory

OBJECTS_DIR   = ./

####### Files

SOURCES       = autoplayer.c 
OBJECTS       = autoplayer.o
DESTDIR       = 
TARGET        = autoplayer-glib-c

first: all
####### Implicit rules

.SUFFIXES: .o .c

.c.o:
	$(CC) -c $(CFLAGS) $(INCPATH) -o "$@" "$<"

####### Build rules

all: Makefile $(TARGET)

$(TARGET):  $(OBJECTS)  
	$(LINK) $(LFLAGS) -o $(TARGET) $(OBJECTS) $(OBJCOMP) $(LIBS)

dist: 
	@$(CHK_DIR_EXISTS) .tmp/autoplayer-glib-c1.0.0 || $(MKDIR) .tmp/autoplayer-glib-c1.0.0 
	$(COPY_FILE) --parents $(SOURCES) $(DIST) .tmp/autoplayer-glib-c1.0.0/ && (cd `dirname .tmp/autoplayer-glib-c1.0.0` && $(TAR) autoplayer-glib-c1.0.0.tar autoplayer-glib-c1.0.0 && $(COMPRESS) autoplayer-glib-c1.0.0.tar) && $(MOVE) `dirname .tmp/autoplayer-glib-c1.0.0`/autoplayer-glib-c1.0.0.tar.gz . && $(DEL_FILE) -r .tmp/autoplayer-glib-c1.0.0


clean:
	-$(DEL_FILE) $(OBJECTS)
	-$(DEL_FILE) $(TARGET)


check: first

####### Compile

autoplayer.o: autoplayer.c
	$(CC) -c $(CFLAGS) $(INCPATH) -o autoplayer.o autoplayer.c

####### Install

install:   FORCE

uninstall:   FORCE

FORCE:
