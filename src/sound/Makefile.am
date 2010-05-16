noinst_HEADERS = \
	samplemanager.h \
	snd_local.h \
	mixer.h

INCLUDES = -I.. -I$(top_srcdir)/thirdparty/clunk

LIBS = @LIBS@ \
	@SDL_LIBS@

if ENABLE_CGAME
noinst_LIBRARIES = libsound.a

if ENABLE_SNDSSE
SSEFLAGS = -DUSE_SIMD
SSESOURCES = $(top_srcdir)/thirdparty/clunk/sse_fft_context.cpp
endif

libsound_a_SOURCES = $(top_srcdir)/thirdparty/clunk/buffer.cpp \
	$(top_srcdir)/thirdparty/clunk/clunk_ex.cpp \
	$(top_srcdir)/thirdparty/clunk/context.cpp \
	$(top_srcdir)/thirdparty/clunk/distance_model.cpp \
	$(top_srcdir)/thirdparty/clunk/logger.cpp \
	$(top_srcdir)/thirdparty/clunk/object.cpp \
	$(top_srcdir)/thirdparty/clunk/sample.cpp \
	$(top_srcdir)/thirdparty/clunk/sdl_ex.cpp \
	$(top_srcdir)/thirdparty/clunk/source.cpp \
	$(top_srcdir)/thirdparty/clunk/stream.cpp \
	$(top_srcdir)/thirdparty/clunk/kemar.c	\
	$(top_srcdir)/thirdparty/clunk/contrib/ogg_stream.cpp \
	mixer.cpp \
	samplemanager.cpp \
	$(SSESOURCES)

endif

AM_CXXFLAGS = @CXXFLAGS@ \
	@SDL_CFLAGS@ \
	@SND_CXXFLAGS@ \
	$(SSEFLAGS)
