CXX = /usr/bin/g++
SOURCEFILES = rumble.cpp dbus.cpp
HEADERS = rumble.hpp
LIBS = `pkg-config glib-2.0 gio-2.0 dbus-1 --libs --cflags`

rumblenotify: $(SOURCEFILES) $(HEADERFILES)
	$(CXX) -Wall -Wextra -std=c++11 -o rumblenotify $(SOURCEFILES) $(LIBS)
