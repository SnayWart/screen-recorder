CXX = g++
CC = gcc
MOC = /usr/lib/qt6/moc

PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
APPDIR ?= $(PREFIX)/share/applications

PKGS = Qt6Widgets libavcodec libavformat libavutil libswscale libswresample libpulse libpulse-simple wayland-client
PKG_CFLAGS = $(shell pkg-config --cflags $(PKGS))
PKG_LIBS = $(shell pkg-config --libs $(PKGS))

CXXFLAGS = -O3 -march=native -std=c++17 -Wall -Wextra -fPIC -I. -Isrc $(PKG_CFLAGS)
CFLAGS = -O3 -march=native -fPIC -Isrc $(shell pkg-config --cflags wayland-client)
LDFLAGS = $(PKG_LIBS) -pthread

SRCS_CXX = src/audio_capture.cpp \
           src/wayland_capture.cpp \
           src/encoder.cpp \
           src/recorder_engine.cpp \
           src/gui_window.cpp \
           src/moc_gui_window.cpp \
           src/main.cpp

SRCS_C = src/screencopy_protocol.c

OBJS = $(SRCS_CXX:.cpp=.o) $(SRCS_C:.c=.o)
TARGET = bin/screen-recorder

all: dirs $(TARGET)

dirs:
	@mkdir -p bin

src/moc_gui_window.cpp: src/gui_window.h
	$(MOC) $< | perl -0777 -pe 's{/\*.*?\*/}{}gs; s{//.*}{}g' > $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $@

install: all
	@mkdir -p $(BINDIR) $(APPDIR)
	install -m 755 $(TARGET) $(BINDIR)/screen-recorder
	install -m 644 screen-recorder.desktop $(APPDIR)/screen-recorder.desktop

uninstall:
	rm -f $(BINDIR)/screen-recorder
	rm -f $(APPDIR)/screen-recorder.desktop

clean:
	rm -rf bin $(OBJS) src/moc_gui_window.cpp

.PHONY: all dirs install uninstall clean
