######################### -*- Mode: Makefile-Gmake -*- ########################
## local.mk<ruleset> --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Fri Oct 28 00:37:46 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Tue Sep  5 22:28:47 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 11
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : 
## 
## arch-tag: d047cfca-c918-4f47-b6e2-8c7df9778b26
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
## Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
##
###############################################################################
testdir:
	$(checkdir)

$(eval $(which_debdir))
include $(DEBDIR)/ruleset/targets/target.mk


CONFIG-common:: debian/stamp-conf 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
CONFIG-arch:: .config conf.vars 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
CONFIG-indep:: conf.vars debian/stamp-kernel-conf
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."


BUILD-common:: sanity_check 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
BUILD-arch:: 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

BUILD/$(i_package):: debian/stamp-build-kernel
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

BIN/$(s_package):: binary/$(s_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
BIN/$(i_package):: binary/$(i_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
BIN/$(d_package):: binary/$(d_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
BIN/$(m_package):: binary/$(m_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
BIN/$(h_package):: binary/$(h_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

INST/$(s_package):: install/$(s_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
INST/$(i_package):: install/$(i_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
INST/$(d_package):: install/$(d_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
INST/$(m_package):: install/$(m_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
INST/$(h_package):: install/$(h_package)
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

CLN-common::
	$(REASON)
	$(warn_root)
	$(eval $(deb_rule))
	$(root_run_command) real_stamp_clean

CLEAN/$(s_package)::
	-rm -rf $(TMPTOP)
CLEAN/$(i_package)::
	-rm -rf $(TMPTOP)
ifneq ($(strip $(KERNEL_ARCH)),um)
  ifeq  ($(strip $(CONFIG_XEN)),)
	test ! -d ./debian || test ! -e stamp-building ||            \
	sed -e 's/=V/$(version)/g'    -e 's/=B/$(link_in_boot)/g'    \
            -e 's/=ST/$(INT_STEM)/g'  -e 's/=R/$(reverse_symlink)/g' \
            -e 's/=K/$(kimage)/g'     -e 's/=L/$(loader)/g'          \
            -e 's@=MK@$(initrdcmd)@g' -e 's@=A@$(DEB_HOST_ARCH)@g'   \
            -e 's/=I/$(INITRD)/g'     -e 's,=D,$(IMAGEDIR),g'        \
            -e 's/=MD/$(initrddep)/g'                                \
            -e 's@=M@$(MKIMAGE)@g'    -e 's/=OF/$(AM_OFFICIAL)/g'    \
            -e 's/=S/$(no_symlink)/g' -e 's@=B@$(KERNEL_ARCH)@g'     \
          $(DEBDIR)/templates.in   > ./debian/templates.master
	test ! -d ./debian || test ! -e stamp-building || $(INSTALL_TEMPLATE)
  endif
endif

CLEAN/$(d_package)::
	-rm -rf $(TMPTOP)
CLEAN/$(m_package)::
	-rm -rf $(TMPTOP)
CLEAN/$(h_package)::
	-rm -rf $(TMPTOP)

buildpackage: CONFIG-common stamp-buildpackage
stamp-buildpackage: 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
ifneq ($(strip $(HAVE_VERSION_MISMATCH)),)
	@echo "The changelog says we are creating $(saved_version)"
	@echo "However, I thought the version is $(version)"
	exit 1
endif
	echo 'Building Package' > stamp-building
	dpkg-buildpackage -nc $(strip $(int_root_cmd)) $(strip $(int_us))  \
               $(strip $(int_uc)) -m"$(maintainer) <$(email)>" -k"$(pgp)"
	rm -f stamp-building
	echo done >  $@
STAMPS_TO_CLEAN += stamp-buildpackage

# All of these are targets that insert themselves into the normal flow
# of policy specified targets, so they must hook themselves into the
# stream.            
debian:  stamp-indep-conf
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

# For the following, that means that we must make sure that the configure and 
# corresponding build targets are all done before the packages are built.
linux-source  linux_source  kernel-source  kernel_source:  stamp-configure stamp-build-indep stamp-kernel-source
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

stamp-kernel-source: install/$(s_package) binary/$(s_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	echo done > $@
STAMPS_TO_CLEAN += stamp-kernel-source

kernel-manual  kernel_manual:  stamp-configure stamp-build-indep stamp-kernel-doc stamp-kernel-manual
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
stamp-kernel-manual: install/$(d_package) install/$(m_package) binary/$(d_package) binary/$(m_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	echo done > $@
STAMPS_TO_CLEAN += stamp-kernel-manual

linux-doc   linux_doc   kernel-doc     kernel_doc:     stamp-configure stamp-build-indep stamp-kernel-doc
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
stamp-kernel-doc: install/$(d_package) binary/$(d_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	echo done > $@
STAMPS_TO_CLEAN += stamp-kernel-doc

linux-headers linux_headers kernel-headers kernel_headers: stamp-configure debian/stamp-prepare stamp-kernel-headers
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
stamp-kernel-headers: install/$(h_package) binary/$(h_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	echo done > $@
STAMPS_TO_CLEAN += stamp-kernel-headers

linux-image   linux_image   kernel-image   kernel_image:   stamp-configure debian/stamp-build-kernel stamp-kernel-image
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."

kernel-image-deb stamp-kernel-image: install/$(i_package) binary/$(i_package) 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	echo done > $@
STAMPS_TO_CLEAN += stamp-kernel-image

libc-kheaders libc_kheaders: 
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
	@echo This target is now obsolete.


$(eval $(which_debdir))
include $(DEBDIR)/ruleset/modules.mk

#Local variables:
#mode: makefile
#End:
