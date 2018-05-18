SUBDIRS = doc src

bindist:
	$(SHELL) @BINDIST_SCRIPT@ $(STRIP) $(top_srcdir) nozip
bindistzip:
	$(SHELL) @BINDIST_SCRIPT@ $(STRIP) $(top_srcdir) zip

EXTRA_DIST = \
    AUTHORS COPYING HACKING INSTALL NEWS PHILOSOPHY README \
    make-bindist_install.sh \
    make-bindist_msdos.sh \
    make-bindist_win32.sh \
    .gitignore
