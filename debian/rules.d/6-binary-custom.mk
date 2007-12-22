# This could be somewhere else, but we stub this file so that the include
# in debian/rules doesn't have an empty list.
binary-custom: $(addprefix custom-binary-,$(custom_flavours))
build-custom: $(addprefix custom-build-,$(custom_flavours))

# Custom targets can dep on these targets to help things along. They can
# also override it with a :: target for addition processing.
custom-prepare-%: $(stampdir)/stamp-custom-prepare-%
	@# Empty for make to be happy
$(stampdir)/stamp-custom-prepare-%: target_flavour = $*
$(stampdir)/stamp-custom-prepare-%: origsrc = $(builddir)/custom-source-$*
$(stampdir)/stamp-custom-prepare-%: srcdir = $(builddir)/custom-build-$*
$(stampdir)/stamp-custom-prepare-%: debian/binary-custom.d/%/config.$(arch) \
		debian/binary-custom.d/%/patchset
	@echo "Preparing custom $*..."
	rm -rf $(origsrc)
	install -d $(origsrc)
	install -d $(srcdir)
	find . \( -path ./debian -o -path ./.git -o -name .gitignore \) \
		-prune -o -print | cpio -dumpl $(origsrc)
	for patch in `ls debian/binary-custom.d/$*/patchset/*.patch | sort`; do \
		echo $$patch; \
		(cd $(origsrc); patch -p1) < $$patch ;\
	done
	cat $< > $(srcdir)/.config
	$(kmake) -C $(origsrc) O=$(srcdir) silentoldconfig prepare scripts
	touch $@

custom-build-%: $(stampdir)/stamp-custom-build-%
	@# Empty for make to be happy
$(stampdir)/stamp-custom-build-%: target_flavour = $*
$(stampdir)/stamp-custom-build-%: origsrc = $(builddir)/custom-source-$*
$(stampdir)/stamp-custom-build-%: srcdir = $(builddir)/custom-build-$*
$(stampdir)/stamp-custom-build-%: bimage = $(call custom_override,build_image,$*)
$(stampdir)/stamp-custom-build-%: $(stampdir)/stamp-custom-prepare-%
	@echo "Building custom $*..."
	$(kmake) -C $(origsrc) O=$(srcdir) $(conc_level)
	$(kmake) -C $(origsrc) O=$(srcdir) $(conc_level) modules
	@touch $@

custom-install-%: pkgdir = $(CURDIR)/debian/linux-image-$(release)$(debnum)-$*
custom-install-%: basepkg = linux-headers-$(release)$(debnum)
custom-install-%: hdrdir = $(CURDIR)/debian/$(basepkg)-$*/usr/src/$(basepkg)-$*
custom-install-%: target_flavour = $*
custom-install-%: origsrc = $(builddir)/custom-source-$*
custom-install-%: srcdir = $(builddir)/custom-build-$*
custom-install-%: kfile = $(call custom_override,kernel_file,$*)
custom-install-%: $(stampdir)/stamp-custom-build-%
	dh_testdir
	dh_testroot
	dh_clean -k -plinux-image-$(release)$(debnum)-$*
	dh_clean -k -plinux-headers-$(release)$(debnum)-$*

	# The main image
	# xen doesnt put stuff in the same directory. its quirky that way
	if [ $(target_flavour) == "xen" ];  then \
		install -m644 -D $(srcdir)/vmlinuz $(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$* ; \
	else \
		install -m644 -D $(srcdir)/$(kfile) $(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$* ; \
	fi

	install -m644 $(srcdir)/.config \
		$(pkgdir)/boot/config-$(release)$(debnum)-$*
	install -m644 $(srcdir)/System.map \
		$(pkgdir)/boot/System.map-$(release)$(debnum)-$*
	$(kmake) -C $(origsrc) O=$(srcdir) modules_install \
		INSTALL_MOD_PATH=$(pkgdir)/
	rm -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/build
	rm -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/source
ifeq ($(no_image_strip),)
	find $(pkgdir)/ -name \*.ko -print | xargs strip --strip-debug
endif
	# Some initramfs-tools specific modules
	install -d $(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd
	#ln -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/kernel/security/capability.ko \
	#	$(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd/

	# Now the image scripts
	install -d $(pkgdir)/DEBIAN
	for script in postinst postrm preinst prerm; do                         \
	  sed -e 's/=V/$(release)$(debnum)-$*/g' -e 's/=K/$(install_file)/g'    \
	      -e 's/=L/$(loader)/g'             -e 's@=B@$(build_arch)@g'       \
		debian/control-scripts/$$script > $(pkgdir)/DEBIAN/$$script;     \
	  chmod 755 $(pkgdir)/DEBIAN/$$script;                                  \
	done

	# The flavour specific headers image
	# XXX Would be nice if we didn't have to dupe the original builddir
	install -m644 -D $(srcdir)/.config \
		$(hdrdir)/.config
	$(kmake) -C $(origsrc) O=$(hdrdir) silentoldconfig prepare scripts
	# We'll symlink this stuff
	rm -f $(hdrdir)/Makefile
	rm -rf $(hdrdir)/include2
	# Script to symlink everything up
	$(SHELL) debian/scripts/link-headers "$(hdrdir)" "$(basepkg)" \
		"$(origsrc)" "$(build_arch)"  "$*"
	# Setup the proper asm symlink
	rm -f $(hdrdir)/include/asm
	ln -s asm-$(asm_link) $(hdrdir)/include/asm
	# The build symlink
	install -d debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*
	ln -s /usr/src/$(basepkg)-$* \
		debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*/build
	# And finally the symvers
	install -m644 $(srcdir)/Module.symvers \
		$(hdrdir)/Module.symvers

custom-binary-%: pkgimg = linux-image-$(release)$(debnum)-$*
custom-binary-%: pkghdr = linux-headers-$(release)$(debnum)-$*
custom-binary-%: custom-install-%
	dh_testdir
	dh_testroot

	dh_installchangelogs -p$(pkgimg)
	dh_installdocs -p$(pkgimg)
	dh_compress -p$(pkgimg)
	dh_fixperms -p$(pkgimg)
	dh_installdeb -p$(pkgimg)
	dh_gencontrol -p$(pkgimg)
	dh_md5sums -p$(pkgimg)
	dh_builddeb -p$(pkgimg) -- -Zbzip2 -z9

	dh_installchangelogs -p$(pkghdr)
	dh_installdocs -p$(pkghdr)
	dh_compress -p$(pkghdr)
	dh_fixperms -p$(pkghdr)
	dh_shlibdeps -p$(pkghdr)
	dh_installdeb -p$(pkghdr)
	dh_gencontrol -p$(pkghdr)
	dh_md5sums -p$(pkghdr)
	dh_builddeb -p$(pkghdr)
