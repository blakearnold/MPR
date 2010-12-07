######################### -*- Mode: Makefile-Gmake -*- ########################
## source.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 13:55:32 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Sep  6 11:49:38 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 4
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This file is responsible forcreating the kernel-source packages 
## 
## arch-tag: 1a7fd804-128f-4f9d-9e3d-ce6bdb731823
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


install/$(s_package): 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	rm -rf $(TMPTOP)
	$(make_directory) $(SRCDIR)
	$(make_directory) $(DOCDIR)
	$(eval $(which_debdir))
	$(install_file) README                         $(DOCDIR)/README
	$(install_file) debian/changelog               $(DOCDIR)/changelog.Debian
	$(install_file) $(DEBDIR)/docs/README          $(DOCDIR)/debian.README
	$(install_file) $(DEBDIR)/docs/README.grub     $(DOCDIR)/
	$(install_file) $(DEBDIR)/docs/README.tecra    $(DOCDIR)/
	$(install_file) $(DEBDIR)/docs/README.modules  $(DOCDIR)/
	$(install_file) $(DEBDIR)/docs/Rationale       $(DOCDIR)/
	$(install_file) $(DEBDIR)/examples/sample.module.control               \
                                                       $(DOCDIR)/
	gzip -9qfr                                     $(DOCDIR)/
	$(install_file) $(DEBDIR)/pkg/source/copyright $(DOCDIR)/copyright
	echo "This was produced by kernel-package version $(kpkg_version)." >  \
	                                               $(DOCDIR)/Buildinfo
ifneq ($(strip $(int_follow_symlinks_in_src)),)
	-tar cfh - $$(echo * | sed -e 's/ debian//g' -e 's/\.deb//g' ) |       \
	(cd $(SRCDIR); umask 000; tar xpsf -)
	(cd $(SRCDIR)/include; rm -rf asm ; )
else
	-tar cf - $$(echo * | sed -e 's/ debian//g' -e 's/\.deb//g' ) |         \
	(cd $(SRCDIR); umask 000; tar xspf -)
	(cd $(SRCDIR)/include; rm -f asm ; )
endif
	$(install_file) debian/changelog      $(SRCDIR)/Debian.src.changelog
	(cd $(SRCDIR);                                                          \
            $(MAKE) $(EXTRAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) distclean)
	(cd $(SRCDIR);         rm -f stamp-building $(STAMPS_TO_CLEAN))
	(cd $(SRCDIR);                                                          \
         [ ! -d scripts/cramfs ]   || make -C scripts/cramfs distclean ; )
	if test -f debian/official && test -f debian/README.Debian ; then       \
           $(install_file) debian/README.Debian $(SRCDIR)/README.Debian ;       \
           $(install_file) debian/README.Debian $(DOCDIR)/README.Debian ;       \
	   gzip -9qf $(DOCDIR)/README.Debian;                                   \
	else                                                                    \
	    sed -e 's/=V/$(version)/g' -e 's/=A/$(DEB_HOST_ARCH)/g'             \
             -e 's/=ST/$(INT_STEM)/g'  -e 's/=B/$(KERNEL_ARCH)/g'               \
                 $(DEBDIR)/pkg/source/README >  $(SRCDIR)/README.Debian ;       \
	fi
	if test -f README.Debian ; then                                         \
           $(install_file) README.Debian        $(DOCDIR)/README.Debian.1st;    \
	   gzip -9qf                            $(DOCDIR)/README.Debian.1st;    \
	fi
	test -d $(SRCDIR)/debian || mkdir $(SRCDIR)/debian
	for file in $(DEBIAN_FILES) control changelog; do                    \
            cp -f  $(DEBDIR)/$$file $(SRCDIR)/debian/;                       \
        done
	for dir  in $(DEBIAN_DIRS);  do                                      \
          cp -af $(DEBDIR)/$$dir  $(SRCDIR)/debian/;                         \
        done
	(cd $(SRCDIR); find . -type d -name .arch-ids -print0 | xargs -0r rm -rf {} \; )
ifneq ($(strip $(source_clean_hook)),)
	(cd $(SRCDIR); test -x $(source_clean_hook) && $(source_clean_hook))
endif
	(cd $(SRCDIR) && cd .. &&                                            \
           tar $(TAR_COMPRESSION) -cf $(package).tar.$(TAR_SUFFIX) $(package) && \
             rm -rf $(package);)

debian/$(s_package): testroot
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(eval $(which_debdir))
	$(make_directory) $(TMPTOP)/DEBIAN
	sed -e 's/=P/$(package)/g' -e 's/=V/$(version)/g'                       \
	    $(DEBDIR)/pkg/source/postinst >          $(TMPTOP)/DEBIAN/postinst
	chmod 755                                    $(TMPTOP)/DEBIAN/postinst
	chmod -R og=rX                               $(TMPTOP)
	chown -R root:root                           $(TMPTOP)
	dpkg-gencontrol -isp -p$(package)          -P$(TMPTOP)/
	$(create_md5sums)                            $(TMPTOP)
	chmod -R og=rX                               $(TMPTOP)
	chown -R root:root                           $(TMPTOP)
	dpkg --build                                 $(TMPTOP) $(DEB_DEST)

binary/$(s_package):
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(checkdir)
	$(require_root)
	$(eval $(deb_rule))
	$(root_run_command) debian/$(package)


#Local variables:
#mode: makefile
#End:
