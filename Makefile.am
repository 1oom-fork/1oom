SUBDIRS = doc src

if COMPILE_WIN32
bindist:
	$(SHELL) $(top_srcdir)/make-bindist_win32.sh $(STRIP) $(top_srcdir) nozip
bindistzip:
	$(SHELL) $(top_srcdir)/make-bindist_win32.sh $(STRIP) $(top_srcdir) zip
endif

EXTRA_DIST = AUTHORS CODE_OF_CONDUCT COPYING HACKING INSTALL NEWS PHILOSOPHY README TODO .gitignore
