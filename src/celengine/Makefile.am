noinst_LIBRARIES = libcelengine.a

noinst_HEADERS = \
	eigenport.h \
	catalogxref.h \
	CGBuffer.h \
	axisarrow.h \
	cmdparser.h \
	globular.h \
	geometry.h \
	modelgeometry.h \
	opencluster.h \
	meshmanager.h \
	rendcontext.h \
	timelinephase.h \
	regcombine.h \
	star.h \
	frame.h \
	visibleregion.h \
	tokenizer.h \
	atmosphere.h \
	starname.h \
	rotationmanager.h \
	body.h \
	trajmanager.h \
	image.h \
	nebula.h \
	glcontext.h \
	dsoname.h \
	astro.h \
	staroctree.h \
	stardb.h \
	dsodb.h \
	starbrowser.h \
	univcoord.h \
	dispmap.h \
	deepskyobj.h \
	command.h \
	timeline.h \
	vecgl.h \
	referencemark.h \
	fragmentprog.h \
	solarsys.h \
	execenv.h \
	asterism.h \
	render.h \
	multitexture.h \
	particlesystem.h \
	dsooctree.h \
	marker.h \
	console.h \
	simulation.h \
	vertexprog.h \
	boundaries.h \
	stellarclass.h \
	planetgrid.h \
	virtualtex.h \
	particlesystemfile.h \
	universe.h \
	galaxy.h \
	lodspheremesh.h \
	surface.h \
	name.h \
	frametree.h \
	constellation.h \
	skygrid.h \
	glshader.h \
	renderinfo.h \
	shadermanager.h \
	selection.h \
	execution.h \
	lightenv.h \
	octree.h \
	spheremesh.h \
	observer.h \
	overlay.h \
	celestia.h \
	location.h \
	renderglsl.h \
	vertexbuf.h \
	texture.h \
	starcolors.h \
	parser.h \
	parseobject.h \
	texmanager.h

dosstuff = celengine.dsp engine.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = \
	$(noinst_DATA) 

libcelengine_a_SOURCES = \
	asterism.cpp \
	astro.cpp \
	axisarrow.cpp \
	body.cpp \
	boundaries.cpp \
	catalogxref.cpp \
	cmdparser.cpp \
	command.cpp \
	console.cpp \
	constellation.cpp \
	dds.cpp \
	deepskyobj.cpp \
	dispmap.cpp \
	dsodb.cpp \
	dsoname.cpp \
	dsooctree.cpp \
	execution.cpp \
	fragmentprog.cpp \
	frame.cpp \
	frametree.cpp \
	galaxy.cpp \
	glcontext.cpp \
	globular.cpp \
	glshader.cpp \
	image.cpp \
	location.cpp \
	lodspheremesh.cpp \
	marker.cpp \
	meshmanager.cpp \
	modelgeometry.cpp \
	multitexture.cpp \
	nebula.cpp \
	observer.cpp \
	opencluster.cpp \
	overlay.cpp \
	parseobject.cpp \
	parser.cpp \
	planetgrid.cpp \
	regcombine.cpp \
	rendcontext.cpp \
	render.cpp \
	renderglsl.cpp \
	rotationmanager.cpp \
	selection.cpp \
	shadermanager.cpp \
	simulation.cpp \
	skygrid.cpp \
	solarsys.cpp \
	spheremesh.cpp \
	star.cpp \
	starbrowser.cpp \
	starcolors.cpp \
	stardb.cpp \
	starname.cpp \
	staroctree.cpp \
	stellarclass.cpp \
	texmanager.cpp \
	texture.cpp \
	timeline.cpp \
	timelinephase.cpp \
	tokenizer.cpp \
	trajmanager.cpp \
	univcoord.cpp \
	universe.cpp \
	vertexprog.cpp \
	virtualtex.cpp \
	visibleregion.cpp \
	$(top_srcdir)/thirdparty/glew/src/glew.c \
	$(top_srcdir)/thirdparty/curveplot/src/curveplot.cpp

AM_CXXFLAGS = @CELESTIA_CXXFLAGS@ \
	@CXXFLAGS@

INCLUDES = -I.. \
	-I$(top_srcdir)/thirdparty/Eigen \
	-I$(top_srcdir)/thirdparty/curveplot/include \
	-I$(top_srcdir)/thirdparty/glew/include

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

