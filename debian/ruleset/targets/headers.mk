######################### -*- Mode: Makefile-Gmake -*- ########################
## headers.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 16:23:51 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Sep  6 11:39:14 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 7
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This file is responsible for creating the kernel-headers packages 
## 
## arch-tag: 2280e193-fbb3-4990-ac8c-d0504ee9bab5
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

install/$(h_package):
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(if $(subst $(strip $(UTS_RELEASE_VERSION)),,$(strip $(version))), \
		echo "The UTS Release version in $(UTS_RELEASE_HEADER)"; \
		echo "     \"$(strip $(UTS_RELEASE_VERSION))\" "; \
		echo "does not match current version:"; \
		echo "     \"$(strip $(version))\" "; \
		echo "Please correct this."; \
		exit 2,)
	rm -rf $(TMPTOP)
	$(make_directory) $(SRCDIR)
	$(make_directory) $(DOCDIR)/examples
#	$(make_directory) $(TMPTOP)/etc/kernel/postinst.d
	$(make_directory) $(SRCDIR)/arch/$(KERNEL_ARCH)
	$(make_directory) $(SRCDIR)/arch/$(KERNEL_ARCH)/kernel/
	$(eval $(which_debdir))
	$(install_file) debian/changelog                $(DOCDIR)/changelog.Debian
	$(install_file) $(DEBDIR)/pkg/headers/README    $(DOCDIR)/debian.README
	$(install_file) $(config)  	                $(DOCDIR)/config-$(version)
	$(install_file) conf.vars  	                $(DOCDIR)/conf.vars
	$(install_file) CREDITS                         $(DOCDIR)/
	$(install_file) MAINTAINERS                     $(DOCDIR)/
	$(install_file) REPORTING-BUGS                  $(DOCDIR)/
	$(install_file) README                          $(DOCDIR)/
	if test -f debian/official && test -f           debian/README.Debian ; then   \
           $(install_file) debian/README.Debian         $(DOCDIR)/README.Debian;  \
           $(install_file) README.Debian                $(DOCDIR)/README.Debian;  \
	fi
	if test -f README.Debian ; then                                                 \
           $(install_file) README.Debian                $(DOCDIR)/README.Debian.1st;\
	fi
	gzip -9qfr                                      $(DOCDIR)/
	echo "This was produced by kernel-package version: $(kpkg_version)." >         \
	                                                   $(DOCDIR)/Buildinfo
	chmod 0644                                         $(DOCDIR)/Buildinfo
	$(install_file) $(DEBDIR)/pkg/headers/copyright    $(DOCDIR)/copyright
	$(install_file) Makefile                           $(SRCDIR)
	test ! -e Rules.make || $(install_file) Rules.make $(SRCDIR)
	test ! -e .kernelrelease || $(install_file) .kernelrelease $(SRCDIR)
	test ! -e arch/$(KERNEL_ARCH)/Makefile     ||                             \
                                $(install_file) arch/$(KERNEL_ARCH)/Makefile      \
                                                     $(SRCDIR)/arch/$(KERNEL_ARCH)
	test ! -e arch/$(KERNEL_ARCH)/Makefile.cpu ||                             \
                                $(install_file) arch/$(KERNEL_ARCH)/Makefile.cpu  \
                                                     $(SRCDIR)/arch/$(KERNEL_ARCH)
	test ! -e Rules.make     || $(install_file) Rules.make     $(SRCDIR)
	test ! -e Module.symvers || $(install_file) Module.symvers $(SRCDIR)
ifneq ($(strip $(int_follow_symlinks_in_src)),)
	-tar cfh - include       |   (cd $(SRCDIR); umask 000; tar xsf -)
	-tar cfh - scripts       |   (cd $(SRCDIR); umask 000; tar xsf -)
	(cd $(SRCDIR)/include;   rm -rf asm; ln -s asm-$(KERNEL_ARCH) asm)
	find . -path './scripts/*'   -prune -o -path './Documentation/*' -prune -o  \
               -path './debian/*'    -prune -o -type f                              \
               \( -name Makefile -o  -name 'Kconfig*' \) -print  |                  \
                  cpio -pdL --preserve-modification-time $(SRCDIR);
	test ! -d arch/$(KERNEL_ARCH)/include || find arch/$(KERNEL_ARCH)/include   \
               -print | cpio -pdL --preserve-modification-time $(SRCDIR);
	test ! -d arch/$(KERNEL_ARCH)/scripts || find arch/$(KERNEL_ARCH)/scripts   \
               -print | cpio -pdL --preserve-modification-time $(SRCDIR);
else
	-tar cf - include |        (cd $(SRCDIR); umask 000; tar xsf -)
	-tar cf - scripts |        (cd $(SRCDIR); umask 000; tar xsf -)
	# Undo the move away of the scripts dir Makefile
	test ! -f $(SRCDIR)/scripts/package/Makefile.dist ||                  \
           mv  -f $(SRCDIR)/scripts/package/Makefile.dist                     \
                  $(SRCDIR)/scripts/package/Makefile
	test ! -f $(SRCDIR)/scripts/package/builddeb.dist ||                  \
           mv  -f $(SRCDIR)/scripts/package/builddeb.dist                     \
                  $(SRCDIR)/scripts/package/builddeb
	(cd       $(SRCDIR)/include; rm -f asm; ln -s asm-$(KERNEL_ARCH) asm)
	find . -path './scripts/*' -prune -o -path './Documentation/*' -prune -o  \
               -path './debian/*'  -prune -o -type f                              \
               \( -name Makefile -o -name 'Kconfig*' \) -print |                  \
                  cpio -pd --preserve-modification-time $(SRCDIR);
	test ! -d arch/$(KERNEL_ARCH)/include || find arch/$(KERNEL_ARCH)/include \
               -print | cpio -pd --preserve-modification-time $(SRCDIR);
	test ! -d arch/$(KERNEL_ARCH)/scripts || find arch/$(KERNEL_ARCH)/scripts \
               -print | cpio -pd --preserve-modification-time $(SRCDIR);
endif
ifeq ($(strip $(KERNEL_ARCH)),um)
	test ! -e arch/$(KERNEL_ARCH)/Makefile.cpu ||                              \
         $(install_file) arch/$(KERNEL_ARCH)/Makefile.cpu                          \
               $(SRCDIR)/arch/$(KERNEL_ARCH)/
	test ! -s $(SRCDIR)/arch/um || $(make_directory) $(SRCDIR)/arch/um
	$(install_file) arch/um/Makefile* $(SRCDIR)/arch/um/
	test ! -e arch/um/Kconfig.arch ||                                           \
         $(install_file) arch/um/Kconfig.arch $(SRCDIR)/arch/um/
endif
	test ! -e arch/$(KERNEL_ARCH)/kernel/asm-offsets.s ||                     \
           $(install_file)               arch/$(KERNEL_ARCH)/kernel/asm-offsets.s \
                           $(SRCDIR)/arch/$(KERNEL_ARCH)/kernel/asm-offsets.s
	for file in $(localversion_files) dummy; do                               \
          test ! -e $$file || $(install_file) $$file $(SRCDIR);                   \
        done
	$(install_file) .config  	        $(SRCDIR)/.config
	echo $(debian)                    > $(SRCDIR)/$(INT_STEM)-headers.revision
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'   \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g'  -e 's@=B@$(KERNEL_ARCH)@g'    \
            $(DEBDIR)/pkg/headers/create_link  > $(DOCDIR)/examples/create_link
	test -d $(SRCDIR)/debian || mkdir $(SRCDIR)/debian
	for file in $(DEBIAN_FILES) control changelog; do                    \
            cp -f  $(DEBDIR)/$$file $(SRCDIR)/debian/;                       \
        done
	for dir  in $(DEBIAN_DIRS);  do                                      \
          cp -af $(DEBDIR)/$$dir  $(SRCDIR)/debian/;                         \
        done
	(cd $(SRCDIR); find . -type d -name .arch-ids -print0 | xargs -0r rm -rf  )
#         $(DEBDIR)/pkg/headers/create_link  >                        \
#                $(TMPTOP)/etc/kernel/postinst.d/create_link-$(version)
ifeq (,$(findstring nostrip,$(DEB_BUILD_OPTIONS)))
	test ! -d $(SRCDIR)/scripts || find $(SRCDIR)/scripts -type f | while read i; do  \
           if file -b $$i | egrep -q "^ELF.*executable"; then                             \
             strip --strip-all --remove-section=.comment --remove-section=.note $$i;      \
           fi;                                                                            \
         done
	test ! -d $(SRCDIR)/scripts || find $(SRCDIR)/scripts -type f | while read i; do  \
           if file -b $$i | egrep -q "^ELF.*shared object"; then                          \
             strip --strip-unneeded --remove-section=.comment --remove-section=.note $$i; \
           fi;                                                                            \
         done
endif

debian/$(h_package): testroot
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(make_directory) $(TMPTOP)/DEBIAN
	$(eval $(deb_rule))
	sed -e 's/=V/$(version)/g'    -e 's/=IB/$(link_in_boot)/g'   \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g' -e 's/=P/$(package)/g'         \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g'  -e 's@=B@$(KERNEL_ARCH)@g'    \
		$(DEBDIR)/pkg/headers/postinst >        $(TMPTOP)/DEBIAN/postinst
	chmod 755                                       $(TMPTOP)/DEBIAN/postinst
#	echo "/etc/kernel/postinst.d/create_link-$(version)" > $(TMPTOP)/DEBIAN/conffiles
	cp -pf debian/control debian/control.dist
	k=`find $(TMPTOP) -type f | ( while read i; do                    \
          if file -b $$i | egrep -q "^ELF.*executable.*dynamically linked" ; then \
            j="$$j $$i";                                                  \
           fi;                                                            \
        done; echo $$j; )`; test -z "$$k" || dpkg-shlibdeps $$k;          \
        test -n "$$k" || perl -pli~ -e 's/\$$\{shlibs:Depends\}\,?//g' debian/control
	test ! -e debian/control~ || rm -f debian/control~
ifneq ($(strip $(header_clean_hook)),)
	(cd $(SRCDIR); test -x $(header_clean_hook) && $(header_clean_hook))
endif
	dpkg-gencontrol -isp -DArchitecture=$(DEB_HOST_ARCH) -p$(package) \
                                          -P$(TMPTOP)/
	$(create_md5sums)                   $(TMPTOP)
	chown -R root:root                  $(TMPTOP)
	chmod -R og=rX                      $(TMPTOP)
	dpkg --build                        $(TMPTOP) $(DEB_DEST)
	cp -pf debian/control.dist          debian/control

binary/$(h_package): 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(require_root)
	$(eval $(deb_rule))
	$(root_run_command) debian/$(package)

#Local variables:
#mode: makefile
#End:
