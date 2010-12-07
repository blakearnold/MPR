######################### -*- Mode: Makefile-Gmake -*- ########################
## image.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 16:47:18 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Sun Sep 24 14:12:22 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 12
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This file is responsible for creating the kernel-image packages 
## 
## arch-tag: ad956b4e-0c5a-4689-b643-7051cc8857cf 
## 
## 
## This program is free software; you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation; either version 2 of the License, or
## (at your option) any later version.
##
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
##
## You should have received a copy of the GNU General Public License
## along with this program; if not, write to the Free Software
## Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
##
###############################################################################

install/$(i_package): 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(if $(subst $(strip $(UTS_RELEASE_VERSION)),,$(strip $(version))), \
		echo "The UTS Release version in $(UTS_RELEASE_HEADER)"; \
		echo "     \"$(strip $(UTS_RELEASE_VERSION))\" "; \
		echo "does not match current version:"; \
		echo "     \"$(strip $(version))\" "; \
		echo "Please correct this."; \
		exit 2,)
	rm -f -r ./$(TMPTOP) ./$(TMPTOP).deb
	$(eval $(which_debdir))
	$(make_directory) $(TMPTOP)/$(IMAGEDIR)
	$(make_directory) $(DOCDIR)/examples
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	$(install_file) Documentation/Changes $(DOCDIR)/
	gzip -9qf $(DOCDIR)/Changes
endif
	$(install_file) debian/changelog        $(DOCDIR)/changelog.Debian
	gzip -9qf                               $(DOCDIR)/changelog.Debian
ifdef loaderdoc
	$(install_file) $(DEBDIR)/docs/ImageLoaders/$(loaderdoc) $(DOCDIR)/$(loaderdoc)
	gzip -9qf                                                $(DOCDIR)/$(loaderdoc)
endif
ifeq ($(strip $(KERNEL_ARCH)),um)
	$(make_directory) $(UML_DIR)
	$(make_directory) $(MAN1DIR)
	$(install_file) $(DEBDIR)/docs/linux.1 $(MAN1DIR)/linux-$(version).1
	gzip -9fq                              $(MAN1DIR)/linux-$(version).1
endif
	$(install_script) $(DEBDIR)/examples/sample.force-build-link.sh      \
                                                      $(DOCDIR)/examples/
	$(install_file) $(DEBDIR)/pkg/image/README    $(DOCDIR)/debian.README
	gzip -9qf                                     $(DOCDIR)/debian.README
	$(install_file) $(DEBDIR)/pkg/image/copyright $(DOCDIR)/copyright
	echo "This was produced by kernel-package version $(kpkg_version)." > \
	           $(DOCDIR)/Buildinfo
	chmod 0644 $(DOCDIR)/Buildinfo
ifeq ($(strip $(KERNEL_ARCH)),um)
	$(install_file) $(config)        $(DOCDIR)/config-$(version)
else
  ifneq ($(strip $(CONFIG_XEN)),)
     ifeq ($(strip $(CONFIG_XEN_PRIVILEGED_GUEST)),)
	$(install_file) $(config)        $(TMPTOP)/$(IMAGEDIR)/config-xenu-$(version)
     else
	$(install_file) $(config)        $(TMPTOP)/$(IMAGEDIR)/config-xen0-$(version)
     endif
  else
	$(install_file) $(config)        $(TMPTOP)/$(IMAGEDIR)/config-$(version)
  endif
endif
	$(install_file) conf.vars        $(DOCDIR)/conf.vars
	gzip -9qf                        $(DOCDIR)/conf.vars
	$(install_file) debian/buildinfo $(DOCDIR)/buildinfo
	gzip -9qf                        $(DOCDIR)/buildinfo
	if test -f debian/official && test -f debian/README.Debian ; then \
           $(install_file) debian/README.Debian  $(DOCDIR)/README.Debian ; \
         gzip -9qf                               $(DOCDIR)/README.Debian;\
	fi
	if test -f README.Debian ; then \
           $(install_file) README.Debian $(DOCDIR)/README.Debian.1st;\
           gzip -9qf                     $(DOCDIR)/README.Debian.1st;\
	fi
	if test -f Debian.src.changelog; then \
	  $(install_file) Debian.src.changelog  $(DOCDIR)/; \
           gzip -9qf                             $(DOCDIR)/Debian.src.changelog;\
	fi
ifeq ($(strip $(HAVE_EXTRA_DOCS)),YES)
	$(install_file) $(extra_docs) 	         $(DOCDIR)/
endif
ifneq ($(filter kfreebsd-gnu, $(DEB_HOST_GNU_SYSTEM)):$(strip $(shell grep -E ^[^\#]*CONFIG_MODULES $(CONFIG_FILE))),:)
  ifeq  ($(DEB_HOST_GNU_SYSTEM):$(strip $(HAVE_NEW_MODLIB)),linux:)
	$(mod_inst_cmds)
  else
# could have also said DEPMOD=/bin/true instead of moving files
    ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
      ifneq ($(strip $(KERNEL_CROSS)),)
	mv System.map System.precious
      endif
	$(MAKE) $(EXTRAV_ARG) INSTALL_MOD_PATH=$(INSTALL_MOD_PATH)           \
                $(CROSS_ARG) ARCH=$(KERNEL_ARCH) modules_install
      ifneq ($(strip $(KERNEL_CROSS)),)
	mv System.precious System.map
      endif
    else
      ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
	mkdir -p $(INSTALL_MOD_PATH)/boot/defaults
	install -o root -g root -m 644                        \
                $(architecture)/conf/GENERIC.hints            \
                $(INSTALL_MOD_PATH)/boot/device.hints
	install -o root -g root -m 644 boot/forth/loader.conf \
                         $(INSTALL_MOD_PATH)/boot/loader.conf
	touch $(INSTALL_MOD_PATH)/boot/loader.conf
	install -o root -g root -m 644 boot/forth/loader.conf \
                $(INSTALL_MOD_PATH)/boot/defaults/loader.conf
	$(PMAKE) -C $(architecture)/compile/GENERIC install \
                    DESTDIR=$(INSTALL_MOD_PATH)
      endif
    endif
  endif
	test ! -e $(TMPTOP)/lib/modules/$(version)/source ||                        \
	   mv $(TMPTOP)/lib/modules/$(version)/source ./debian/source-link
	test ! -e $(TMPTOP)/lib/modules/$(version)/build ||                         \
	   mv $(TMPTOP)/lib/modules/$(version)/build ./debian/build-link
  ifeq ($(strip $(KERNEL_ARCH)),um)
	-/sbin/depmod -q -FSystem.map -b $(TMPTOP) \
           $(version)-$$(sed q $(UTS_RELEASE_HEADER) | sed s/\"//g | awk -F\- '{print $$2}')
  else
    ifeq ($(DEB_BUILD_GNU_TYPE),$(DEB_HOST_GNU_TYPE))
	-/sbin/depmod -q -FSystem.map -b $(TMPTOP) $(version);
    endif
  endif
	test ! -e ./debian/source-link ||                                              \
	   mv ./debian/source-link $(TMPTOP)/lib/modules/$(version)/source
	test ! -e  ./debian/build-link ||                                              \
	   mv  ./debian/build-link $(TMPTOP)/lib/modules/$(version)/build
endif
ifeq ($(strip $(NEED_DIRECT_GZIP_IMAGE)),YES)
	gzip -9vc $(kimagesrc) > $(kimagedest)
else
	cp $(kimagesrc) $(kimagedest)
endif
ifeq ($(strip $(KERNEL_ARCH)),um)
	chmod 755 $(kimagedest);
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
	strip --strip-unneeded --remove-section=.note --remove-section=.comment  $(kimagedest);
endif
else
	chmod 644 $(kimagedest);
endif
ifeq ($(strip $(HAVE_COFF_IMAGE)),YES)
	cp $(coffsrc)   $(coffdest)
	chmod 644       $(coffdest)
endif
ifeq ($(strip $(int_install_vmlinux)),YES)
ifneq ($(strip $(kelfimagesrc)),)
	cp $(kelfimagesrc) $(kelfimagedest)
	chmod 644 $(kelfimagedest)
endif
endif
	if test -d $(SRCTOP)/debian/image.d ; then                        \
             TMPTOP=$(TMPTOP) version=$(version) IMAGE_TOP=$(TMPTOP)      \
                   run-parts --verbose $(SRCTOP)/debian/image.d ;         \
         fi
	if [ -x debian/post-install ]; then                               \
		TMPTOP=$(TMPTOP) STEM=$(INT_STEM) version=$(version)      \
		IMAGE_TOP=$(TMPTOP) debian/post-install;                  \
	fi
ifeq ($(strip $(NEED_IMAGE_POST_PROCESSING)),YES)
	$(DO_IMAGE_POST_PROCESSING)
endif
	test ! -s applied_patches || cp applied_patches                        \
                        $(TMPTOP)/$(IMAGEDIR)/patches-$(version)
	test ! -s applied_patches || chmod 644                                 \
                        $(TMPTOP)/$(IMAGEDIR)/patches-$(version)
ifneq ($(strip $(KERNEL_ARCH)),um)
  ifneq ($(strip $(CONFIG_XEN)),)
     ifeq ($(strip $(CONFIG_XEN_PRIVILEGED_GUEST)),)
	test ! -f System.map ||  cp System.map                         \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-xenu-$(version);
	test ! -f System.map ||  chmod 644                             \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-xenu-$(version);
     else
	test ! -f System.map ||  cp System.map                         \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-xen0-$(version);
	test ! -f System.map ||  chmod 644                             \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-xen0-$(version);
     endif
  else
	test ! -f System.map ||  cp System.map                         \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-$(version);
	test ! -f System.map ||  chmod 644                             \
                        $(TMPTOP)/$(IMAGEDIR)/System.map-$(version);
  endif
else
	if [ -d $(INSTALL_MOD_PATH)/lib/modules/$(version) ] ; then    \
          (cd $(INSTALL_MOD_PATH)/lib/modules/$(version);              \
           tar cf - . | (cd $(UML_DIR)/; umask 000; tar xsf -));       \
        fi
	rm -rf $(INSTALL_MOD_PATH)/lib
endif
	# For LKCD enabled kernels
	test ! -f Kerntypes ||  cp Kerntypes                                   \
                        $(TMPTOP)/$(IMAGEDIR)/Kerntypes-$(version)
	test ! -f Kerntypes ||  chmod 644                                      \
                        $(TMPTOP)/$(IMAGEDIR)/Kerntypes-$(version)
ifeq ($(strip $(delete_build_link)),YES)
	rm -f $(TMPTOP)/lib/modules/$(version)/build
endif

debian/$(i_package): testroot
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(make_directory) $(TMPTOP)/DEBIAN
ifneq ($(strip $(KERNEL_ARCH)),um)
  ifeq ($(strip $(CONFIG_XEN)),)
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'   \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g'  -e 's@=B@$(KERNEL_ARCH)@g'    \
         $(DEBDIR)/pkg/image/postinst > $(TMPTOP)/DEBIAN/postinst
	chmod 755 $(TMPTOP)/DEBIAN/postinst
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g'  -e 's@=B@$(KERNEL_ARCH)@g'    \
         $(DEBDIR)/pkg/image/config > $(TMPTOP)/DEBIAN/config
	chmod 755 $(TMPTOP)/DEBIAN/config
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
         $(DEBDIR)/pkg/image/postrm > $(TMPTOP)/DEBIAN/postrm
	chmod 755 $(TMPTOP)/DEBIAN/postrm
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
         $(DEBDIR)/pkg/image/preinst > $(TMPTOP)/DEBIAN/preinst
	chmod 755 $(TMPTOP)/DEBIAN/preinst
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
         $(DEBDIR)/pkg/image/prerm > $(TMPTOP)/DEBIAN/prerm
	chmod 755 $(TMPTOP)/DEBIAN/prerm
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
          $(DEBDIR)/templates.in   > ./debian/templates.master
	$(INSTALL_TEMPLATE)
	$(install_file) ./debian/templates.master $(TMPTOP)/DEBIAN/templates
  else
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
          $(DEBDIR)/pkg/virtual/xen/postinst > $(TMPTOP)/DEBIAN/postinst
	chmod 755 $(TMPTOP)/DEBIAN/postinst
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
         $(DEBDIR)/pkg/virtual/xen/prerm > $(TMPTOP)/DEBIAN/prerm
	chmod 755 $(TMPTOP)/DEBIAN/prerm
  endif
else
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's@=B@$(KERNEL_ARCH)@g'     \
            -e 's/=S/$(no_symlink)/g' -e 's/=OF/$(AM_OFFICIAL)/g'    \
          $(DEBDIR)/pkg/virtual/um/postinst > $(TMPTOP)/DEBIAN/postinst
	chmod 755 $(TMPTOP)/DEBIAN/postinst
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
          $(DEBDIR)/pkg/virtual/um/prerm > $(TMPTOP)/DEBIAN/prerm
	chmod 755 $(TMPTOP)/DEBIAN/prerm
endif
ifneq ($(strip $(image_clean_hook)),)
	(cd $(TMPTOP); test -x $(image_clean_hook) && $(image_clean_hook))
endif
	dpkg-gencontrol -DArchitecture=$(DEB_HOST_ARCH) -isp         \
                        -p$(package) -P$(TMPTOP)/
	$(create_md5sums)              $(TMPTOP)
	chmod -R og=rX                 $(TMPTOP)
	chown -R root:root             $(TMPTOP)
	dpkg --build                   $(TMPTOP) $(DEB_DEST)
ifeq ($(strip $(do_clean)),YES)
	$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) clean
	rm -f stamp-$(package)
endif

binary/$(i_package):
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(require_root)
	$(eval $(deb_rule))
	$(root_run_command) debian/$(package)

#Local variables:
#mode: makefile
#End:
