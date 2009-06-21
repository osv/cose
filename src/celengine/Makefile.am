noinst_LIBRARIES = libcelengine.a
noinst_HEADERS = \
	precession.h \
	catalogxref.h \
	CGBuffer.h \
	axisarrow.h \
	cmdparser.h \
	globular.h \
	opencluster.h \
	meshmanager.h \
	rendcontext.h \
	timelinephase.h \
	regcombine.h \
	star.h \
	frame.h \
	spiceinterface.h \
	visibleregion.h \
	tokenizer.h \
	atmosphere.h \
	starname.h \
	rotationmanager.h \
	body.h \
	trajmanager.h \
	image.h \
	modelfile.h \
	nebula.h \
	glcontext.h \
	dsoname.h \
	astro.h \
	staroctree.h \
	stardb.h \
	dsodb.h \
	3dsmesh.h \
	starbrowser.h \
	gl.h \
	univcoord.h \
	dispmap.h \
	deepskyobj.h \
	command.h \
	timeline.h \
	vecgl.h \
	referencemark.h \
	fragmentprog.h \
	vsop87.h \
	solarsys.h \
	execenv.h \
	spiceorbit.h \
	asterism.h \
	render.h \
	nutation.h \
	multitexture.h \
	glext.h \
	particlesystem.h \
	solarsysxml.h \
	vertexlist.h \
	dsooctree.h \
	marker.h \
	scriptobject.h \
	console.h \
	simulation.h \
	vertexprog.h \
	boundaries.h \
	stellarclass.h \
	planetgrid.h \
	virtualtex.h \
	scriptrotation.h \
	particlesystemfile.h \
	scriptorbit.h \
	universe.h \
	galaxy.h \
	samporbit.h \
	lodspheremesh.h \
	surface.h \
	name.h \
	mesh.h \
	frametree.h \
	constellation.h \
	skygrid.h \
	glshader.h \
	renderinfo.h \
	shadermanager.h \
	spicerotation.h \
	selection.h \
	model.h \
	execution.h \
	lightenv.h \
	octree.h \
	spheremesh.h \
	observer.h \
	samporient.h \
	overlay.h \
	orbit.h \
	customorbit.h \
	celestia.h \
	jpleph.h \
	location.h \
	renderglsl.h \
	vertexbuf.h \
	texture.h \
	rotation.h \
	starcolors.h \
	parser.h \
	parseobject.h \
	texmanager.h \
	customrotation.h 

dosstuff = celengine.dsp engine.mak
noinst_DATA = $(dosstuff)

EXTRA_DIST = \
	$(noinst_DATA) \
	3dsmesh.cpp \
	solarsysxml.cpp

INCLUDES = -I..

if ENABLE_SPICE
SPICESOURCES = spiceinterface.cpp spiceorbit.cpp spicerotation.cpp
endif

if ENABLE_CELX
SCRIPT_OBJ_SOURCES = scriptobject.cpp scriptorbit.cpp scriptrotation.cpp
endif

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
	customorbit.cpp \
	customrotation.cpp \
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
	glext.cpp \
	globular.cpp \
	glshader.cpp \
	image.cpp \
	jpleph.cpp \
	location.cpp \
	lodspheremesh.cpp \
	marker.cpp \
	mesh.cpp \
	meshmanager.cpp \
	model.cpp \
	modelfile.cpp \
	multitexture.cpp \
	nebula.cpp \
	nutation.cpp \
	observer.cpp \
	opencluster.cpp \
	orbit.cpp \
	overlay.cpp \
	parseobject.cpp \
	parser.cpp \
	planetgrid.cpp \
	precession.cpp \
	regcombine.cpp \
	rendcontext.cpp \
	render.cpp \
	renderglsl.cpp \
	rotation.cpp \
	rotationmanager.cpp \
	samporbit.cpp \
	samporient.cpp \
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
	vertexlist.cpp \
	vertexprog.cpp \
	virtualtex.cpp \
	visibleregion.cpp \
        vsop87.cpp\
	$(SPICESOURCES)\
	$(SCRIPT_OBJ_SOURCES)

AM_CXXFLAGS = @CXXFLAGS@\
	@CELESTIA_CXXFLAGS@

LIBS = @LIBS@ \
	@CELESTIA_LIBS@

