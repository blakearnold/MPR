# We don't want make removing intermediary stamps
.SECONDARY :

# Prepare the out-of-tree build directory

prepare-%: $(stampdir)/stamp-prepare-%
	@# Empty for make to be happy
$(stampdir)/stamp-prepare-%: target_flavour = $*
$(stampdir)/stamp-prepare-%: $(confdir)/config $(confdir)/config.%
	@echo "Preparing $*..."
	install -d $(builddir)/build-$*
	cat $^ > $(builddir)/build-$*/.config
	$(kmake) O=$(builddir)/build-$* silentoldconfig prepare scripts
	touch $@


# Do the actual build, including image and modules
build-%: $(stampdir)/stamp-build-%
	@# Empty for make to be happy
$(stampdir)/stamp-build-%: target_flavour = $*
$(stampdir)/stamp-build-%: $(stampdir)/stamp-prepare-%
	@echo "Building $*..."
	$(kmake) -C $(builddir)/build-$* $(conc_level) $(build_image)
	$(kmake) -C $(builddir)/build-$* $(conc_level) modules
	@touch $@

# Install the finished build
install-%: pkgdir = $(CURDIR)/debian/linux-image-$(release)$(debnum)-$*
install-%: dbgpkgdir = $(CURDIR)/debian/linux-image-debug-$(release)$(debnum)-$*
install-%: basepkg = linux-headers-$(release)$(debnum)
install-%: hdrdir = $(CURDIR)/debian/$(basepkg)-$*/usr/src/$(basepkg)-$*
install-%: target_flavour = $*
install-%: $(stampdir)/stamp-build-% checks-%
	dh_testdir
	dh_testroot
	dh_clean -k -plinux-image-$(release)$(debnum)-$*
	dh_clean -k -plinux-headers-$(release)$(debnum)-$*
	dh_clean -k -plinux-image-debug-$(release)$(debnum)-$*

	# The main image
	install -m644 -D $(builddir)/build-$*/$(kernel_file) \
		$(pkgdir)/boot/$(install_file)-$(release)$(debnum)-$*
	install -m644 $(builddir)/build-$*/.config \
		$(pkgdir)/boot/config-$(release)$(debnum)-$*
	install -m644 $(abidir)/$* \
		$(pkgdir)/boot/abi-$(release)$(debnum)-$*
	$(kmake) -C $(builddir)/build-$* modules_install \
		INSTALL_MOD_PATH=$(pkgdir)/
ifeq ($(no_image_strip),)
	find $(pkgdir)/ -name \*.ko -print | xargs strip --strip-debug
endif
	# Some initramfs-tools specific modules
	# XXX: vesafb
	install -d $(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd
	ln -f $(pkgdir)/lib/modules/$(release)$(debnum)-$*/kernel/security/capability.ko \
		$(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd/
	# ln -f kernel/drivers/video/vesafb.ko \
	#	$(pkgdir)/lib/modules/$(release)$(debnum)-$*/initrd/

	# Debug image is simple
ifneq ($(do_debug_image),)
	install -m644 -D $(builddir)/build-$*/vmlinux \
		$(dbgpkgdir)//boot/vmlinux-debug-$(release)$(debnum)-$*
endif

	# The flavour specific headers image
	# XXX Would be nice if we didn't have to dupe the original builddir
	install -m644 -D $(builddir)/build-$*/.config \
		$(hdrdir)/.config
	$(kmake) O=$(hdrdir) silentoldconfig prepare scripts
	# We'll symlink this stuff
	rm -f $(hdrdir)/Makefile
	rm -rf $(hdrdir)/include2
	# Script to symlink everything up
	$(SHELL) debian/scripts/link-headers "$(hdrdir)" "$(basepkg)" \
		"$(build_arch)"  "$*"
	# Setup the proper asm symlink
	rm -f $(hdrdir)/include/asm
	ln -s asm-$(build_arch) $(hdrdir)/include/asm
	# The build symlink
	install -d debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*
	ln -s /usr/src/$(basepkg)-$* \
		debian/$(basepkg)-$*/lib/modules/$(release)$(debnum)-$*/build
	# And finally the symvers
	install -m644 $(builddir)/build-$*/Module.symvers \
		$(hdrdir)/Module.symvers


headers_tmp := $(CURDIR)/debian/tmp-headers
headers_dir := $(CURDIR)/debian/linux-libc-dev

header_make_args := -C $(CURDIR) O=$(headers_tmp) \
	EXTRAVERSION=$(debnum) INSTALL_HDR_PATH=$(headers_tmp)/install

install-arch-headers:
	dh_testdir
	dh_testroot
	dh_clean -k -plinux-libc-dev

	rm -rf $(headers_tmp)
	install -d $(headers_tmp) $(headers_dir)/usr/include/

	make $(header_make_args) ARCH=$(header_arch) $(defconfig)
	mv $(headers_tmp)/.config $(headers_tmp)/.config.old
	sed -e 's/^# \(CONFIG_MODVERSIONS\) is not set$$/\1=y/' \
	  -e 's/.*CONFIG_LOCALVERSION_AUTO.*/# CONFIG_LOCALVERSION_AUTO is not set/' \
	  $(headers_tmp)/.config.old > $(headers_tmp)/.config
	make $(header_make_args) ARCH=$(header_arch) silentoldconfig
	make $(header_make_args) ARCH=$(header_arch) headers_install

	mv $(headers_tmp)/install/include/asm* \
		$(headers_dir)/usr/include/
	mv $(headers_tmp)/install/include/linux \
		$(headers_dir)/usr/include/

	rm -rf $(headers_tmp)

binary-arch-headers: install-arch-headers
	dh_testdir
	dh_testroot

	dh_installchangelogs -plinux-libc-dev
	dh_installdocs -plinux-libc-dev
	dh_compress -plinux-libc-dev
	dh_fixperms -plinux-libc-dev
	dh_installdeb -plinux-libc-dev
	dh_gencontrol -plinux-libc-dev
	dh_md5sums -plinux-libc-dev
	dh_builddeb -plinux-libc-dev -- -Zbzip2 -z9

binary-%: pkgimg = linux-image-$(release)$(debnum)-$*
binary-%: pkghdr = linux-headers-$(release)$(debnum)-$*
binary-%: dbgpkg = linux-image-debug-$(release)$(debnum)-$*
binary-%: install-%
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
	dh_installdeb -p$(pkghdr)
	dh_gencontrol -p$(pkghdr)
	dh_md5sums -p$(pkghdr)
	dh_builddeb -p$(pkghdr) -- -Zbzip2 -z9

ifneq ($(do_debug_image),)
	dh_installchangelogs -p$(dbgpkg)
	dh_installdocs -p$(dbgpkg)
	dh_compress -p$(dbgpkg)
	dh_fixperms -p$(dbgpkg)
	dh_installdeb -p$(dbgpkg)
	dh_gencontrol -p$(dbgpkg)
	dh_md5sums -p$(dbgpkg)
	dh_builddeb -p$(dbgpkg) -- -Zbzip2 -z9
endif

$(stampdir)/stamp-flavours:
	@echo $(flavours) > $@

binary-arch: $(stampdir)/stamp-flavours $(addprefix binary-,$(flavours)) \
	     binary-arch-headers
