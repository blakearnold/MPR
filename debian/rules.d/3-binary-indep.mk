build-indep:

docpkg = linux-doc-$(release)
docdir = $(CURDIR)/debian/$(docpkg)/usr/share/doc/$(docpkg)
install-doc:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(docpkg)

	# First the html docs
	install -d $(docdir)/linux-doc-tmp
	$(kmake) O=$(docdir)/linux-doc-tmp htmldocs
	mv $(docdir)/linux-doc-tmp/Documentation/DocBook \
		$(docdir)/html
	rm -rf $(docdir)/linux-doc-tmp

	# Copy the rest
	cp -a Documentation $(docdir)
	rm -rf $(docdir)/DocBook

srcpkg = linux-source-$(release)
srcdir = $(CURDIR)/debian/$(srcpkg)/usr/src/$(srcpkg)
install-source:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(srcpkg)

	install -d $(srcdir)
	find . -path './debian/*' -prune -o \
		-path './.*' -prune -o -print | \
		cpio -pd --preserve-modification-time $(srcdir)
	(cd $(srcdir)/..; tar cf - $(srcpkg)) | bzip2 -9c > \
		$(srcdir).tar.bz2
	rm -rf $(srcdir)

indep_hdrpkg = linux-headers-$(release)$(debnum)
indep_hdrdir = $(CURDIR)/debian/$(indep_hdrpkg)/usr/src/$(indep_hdrpkg)
install-headers:
	dh_testdir
	dh_testroot
	dh_clean -k -p$(indep_hdrpkg)

	install -d $(indep_hdrdir)
	find . -path './debian/*' -prune -o -path './include/*' -prune \
	  -o -path './scripts/*' -prune -o -type f \
	  \( -name 'Makefile*' -o -name 'Kconfig*' -o -name 'Kbuild*' -o \
	     -name '*.sh' -o -name '*.pl' -o -name '*.lds' \) \
	  -print | cpio -pd --preserve-modification-time $(indep_hdrdir)
	# Copy headers for LUM
	cp -a drivers/media/dvb/dvb-core/*.h $(indep_hdrdir)/drivers/media/dvb/dvb-core
	cp -a drivers/media/video/*.h $(indep_hdrdir)/drivers/media/video
	cp -a drivers/media/dvb/dvb-core/*.h $(indep_hdrdir)/drivers/media/dvb/dvb-core
	cp -a drivers/media/dvb/frontends/*.h $(indep_hdrdir)/drivers/media/dvb/frontends
	cp -a scripts include $(indep_hdrdir)

install-indep: install-source install-headers install-doc

binary-headers: install-headers
	dh_testdir
	dh_testroot
	dh_installchangelogs -p$(indep_hdrpkg)
	dh_installdocs -p$(indep_hdrpkg)
	dh_compress -p$(indep_hdrpkg)
	dh_fixperms -p$(indep_hdrpkg)
	dh_installdeb -p$(indep_hdrpkg)
	dh_gencontrol -p$(indep_hdrpkg)
	dh_md5sums -p$(indep_hdrpkg)
	dh_builddeb -p$(indep_hdrpkg)

binary-indep: install-indep
	dh_testdir
	dh_testroot

	dh_installchangelogs -i
	dh_installdocs -i
	dh_compress -i
	dh_fixperms -i
	dh_installdeb -i
	dh_gencontrol -i
	dh_md5sums -i
	dh_builddeb -i
