SUBDIRS = doc src

bindist:
	$(SHELL) @BINDIST_SCRIPT@ $(STRIP) $(top_srcdir) nozip
bindistzip:
	$(SHELL) @BINDIST_SCRIPT@ $(STRIP) $(top_srcdir) zip

EXTRA_DIST = AUTHORS COPYING HACKING INSTALL NEWS PHILOSOPHY README .gitignore
