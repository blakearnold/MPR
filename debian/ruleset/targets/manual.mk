######################### -*- Mode: Makefile-Gmake -*- ########################
## manual.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 15:52:16 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Sep  6 11:40:58 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 2
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This file is responsible for creating the kernel-manual packages 
## 
## arch-tag: a34656e9-483a-4339-a26b-f1b5c5a2d964
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


# Only dependencies that have not been registered into the ladder
# created in rulesets/common/targets.mk
install/$(m_package): install/$(d_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	rm -rf            $(TMPTOP)
	$(make_directory) $(DOCDIR)
	$(make_directory) $(MANDIR)/man9
	$(install_file)   debian/changelog    $(DOCDIR)/changelog.Debian
	test ! -d $(TMP_MAN) || find $(TMP_MAN) -type f -exec mv {} $(MAN9DIR) \;
	-gunzip -qfr $(MANDIR)
	find $(MANDIR) -type f -size 0 -exec rm {} \;
	-gzip -9qfr $(MANDIR)
	-gzip -9qfr $(DOCDIR)
	$(install_file) $(DEBDIR)/pkg/doc/copyright $(DOCDIR)/copyright

debian/$(m_package): testroot
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(make_directory) $(TMPTOP)/DEBIAN
	dpkg-gencontrol -isp -p$(package)       -P$(TMPTOP)/
	$(create_md5sums)                         $(TMPTOP)
	chmod -R og=rX                            $(TMPTOP)
	chown -R root:root                        $(TMPTOP)
	dpkg --build                              $(TMPTOP) $(DEB_DEST)

binary/$(m_package):
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	$(checkdir)
	$(require_root)
	$(eval $(deb_rule))
	$(root_run_command) debian/$(package)

#Local variables:
#mode: makefile
#End:
