# Special target to create an AmigaOS snapshot installation
# AmigaOS shell doesn't like indented comments.
amigaosdist: $(EXECUTABLE) $(PLUGINS)
	mkdir -p $(AMIGAOSPATH)
	mkdir -p $(AMIGAOSPATH)/extras
#	cp ${srcdir}/dists/amiga/residualvm_drawer.info $(AMIGAOSPATH).info
#	cp ${srcdir}/dists/amiga/residualvm.info $(AMIGAOSPATH)/$(EXECUTABLE).info
# Copy mandatory installation files.
# Prepare README.md for AmigaGuide conversion.
	cat ${srcdir}/README.md | sed -f ${srcdir}/dists/amiga/convertRM.sed > README.conv
# AmigaOS AREXX has a problem when ${srcdir} is '.'.
# It will break with a "Program not found" error.
#
# Skip the following for now, since the script breaks installation.
# (It was not designed to work with residualvm's readme).
# If i ever get around adapting/enhancing it to work with residualvm's
# readme i'll revisit this.
#
# Copy the script to cwd and, once it has finished, remove it.
#	cp ${srcdir}/dists/amiga/RM2AG.rexx .
#	rx RM2AG.rexx README.conv $(AMIGAOSPATH)
#	rm README.conv
#	rm RM2AG.rexx
ifdef DIST_FILES_DOCS
	mkdir -p $(AMIGAOSPATH)/doc
	cp -r $(srcdir)/doc/ $(AMIGAOSPATH)
	cp $(DIST_FILES_DOCS) $(AMIGAOSPATH)/doc/
endif
ifdef DIST_FILES_ENGINEDATA
	cp $(DIST_FILES_ENGINEDATA) $(AMIGAOSPATH)/extras/
endif
ifdef DIST_FILES_NETWORKING
	cp $(DIST_FILES_NETWORKING) $(AMIGAOSPATH)/extras/
endif
ifdef DIST_FILES_VKEYBD
	cp $(DIST_FILES_VKEYBD) $(AMIGAOSPATH)/extras/
endif
ifdef DIST_FILES_THEMES
	mkdir -p $(AMIGAOSPATH)/themes
	cp $(DIST_FILES_THEMES) $(AMIGAOSPATH)/themes/
endif
# Strip and copy engine plugins.
ifdef DYNAMIC_MODULES
	mkdir -p $(AMIGAOSPATH)/plugins
	$(foreach plugin, $(PLUGINS), $(STRIP) $(plugin) -o $(AMIGAOSPATH)/$(plugin);)
# Extract and install compiled-in shared libraries.
# Not every AmigaOS installation, especially vanilla ones,
# come with every mandatory shared library.
	mkdir -p $(AMIGAOSPATH)/sobjs
	cp ${srcdir}/dists/amiga/Ext_Inst_so.rexx .
	rx Ext_Inst_so.rexx $(EXECUTABLE) $(AMIGAOSPATH)
	rm Ext_Inst_so.rexx
endif
	$(STRIP) $(EXECUTABLE) -o $(AMIGAOSPATH)/$(EXECUTABLE)

# Special target to cross create an AmigaOS snapshot installation
amigaoscross: $(EXECUTABLE)
	mkdir -p ResidualVM
	mkdir -p ResidualVM/themes
	mkdir -p ResidualVM/extras
	cp $(EXECUTABLE) ResidualVM/ResidualVM
	cp ${srcdir}/icons/residualvm_drawer.info ResidualVM.info
	cp ${srcdir}/icons/residualvm.info ResidualVM/ResidualVM.info
	cp $(DIST_FILES_THEMES) ResidualVM/themes/
ifdef DIST_FILES_ENGINEDATA
	cp $(DIST_FILES_ENGINEDATA) ResidualVM/extras/
endif
	cp $(srcdir)/AUTHORS ResidualVM/AUTHORS.txt
	cp $(srcdir)/COPYING ResidualVM/COPYING.txt
	cp $(srcdir)/COPYING.BSD ResidualVM/COPYING.BSD.txt
	cp $(srcdir)/COPYING.LGPL ResidualVM/COPYING.LGPL.txt
	cp $(srcdir)/COPYING.FREEFONT ResidualVM/COPYING.FREEFONT.txt
	cp $(srcdir)/COPYING.ISC ResidualVM/COPYING.ISC.txt
	cp $(srcdir)/COPYING.LUA ResidualVM/COPYING.LUA.txt
	cp $(srcdir)/COPYING.MIT ResidualVM/COPYING.MIT.txt
	cp $(srcdir)/COPYING.TINYGL ResidualVM/COPYING.TINYGL.txt
	cp $(srcdir)/COPYRIGHT ResidualVM/COPYRIGHT.txt
	cp $(srcdir)/KNOWN_BUGS ResidualVM/KNOWN_BUGS.txt
	cp $(srcdir)/NEWS.md ResidualVM/NEWS.txt
	cp $(srcdir)/doc/QuickStart ResidualVM/QuickStart.txt
	cp $(srcdir)/README.md ResidualVM/README.txt
	zip -r residualvm-amigaos4.zip ResidualVM ResidualVM.info
