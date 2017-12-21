PRJTYPE = Executable
ADDINCS = ../include
LIBS = assetloader macu physfs openal orb vorbis ogg freetype png jpeg tiff zlib gfxwnd glfw glad
ifeq ($(TARGET_OS), Windows_NT)
	LIBS += glu32 opengl32 gdi32 winmm ole32 shell32 user32
else
	LIBS += GLU GL X11 Xcursor Xinerama Xrandr Xxf86vm Xi pthread m dl
endif
MOREDEPS = ..
EXTDEPS = macu::0.0.2dev assetloader::dev gfxwnd::0.0.1dev
