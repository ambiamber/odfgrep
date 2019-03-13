man_MANS = odfgrep.1
man_HTMLS = odfgrep.1.html
EXTRA_DIST = $(man_MANS)

html-local : $(man_HTMLS)

%.html: %
	$(RMAN) -f HTML $< > $@
