######################### -*- Mode: Makefile-Gmake -*- ########################
## target.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 10:41:41 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Sat Nov  4 22:26:02 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 17
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : This file provides the commands commaon to a number of
##                    packages built, and also includes the files providing
##                    commands to build each of the packages we create.
## 
## arch-tag: 254cf803-a899-4234-ba83-8d032e970c38
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

# Find out whether we need to have a pre-defined .config
NEED_CONFIG = $(shell if [ $(VERSION) -lt 2 ]; then                        \
                           echo "";                                        \
           elif [ $(VERSION) -eq 2 ] && [ $(PATCHLEVEL) -lt 6 ]; then      \
                           echo "";                                        \
           elif [ $(VERSION) -eq 2 ] && [ $(PATCHLEVEL) -eq 6 ] &&         \
                   [ $(SUBLEVEL) -lt 18 ]; then                            \
                           echo "";                                        \
           else                                                            \
                           echo "YES";                                     \
           fi)

.config:
	$(REASON)
	$(checkdir)
ifneq ($(strip $(use_saved_config)),NO)
	test -f .config || test ! -f .config.save || \
		            cp -pf .config.save .config
endif
	test -f .config || test ! -f $(CONFIG_FILE) || \
		            cp -pf $(CONFIG_FILE) .config
	$(eval $(which_debdir))
	test -f .config || test ! -f $(DEBDIR)/config || \
		            cp -pf $(DEBDIR)/config  .config
ifeq ($(strip $(have_new_config_target)),)
  ifneq ($(strip $(NEED_CONFIG)),)
	test -f .config || $(MAKE) defconfig
  endif
	test -f .config || (echo "*** Need a config file .config" && false)
endif
# if $(have_new_config_target) is set, then we need not have a .config
# file at this point


conf.vars: Makefile .config
	$(REASON)
	$(checkdir)
	@rm -f .mak
	@touch .mak
	@echo "This is kernel package version $(kpkg_version)." >> .mak
	@echo "VERSION          = $(VERSION)"       >> .mak
	@echo "PATCHLEVEL       = $(PATCHLEVEL)"    >> .mak
	@echo "SUBLEVEL 	= $(SUBLEVEL)"      >> .mak
	@echo "EXTRAVERSION     = $(EXTRAVERSION)"  >> .mak
ifneq ($(strip $(iatv)),)
	@echo "APPEND_TO_VERSION = $(iatv)"         >> .mak
endif
ifeq ($(strip $(patch_the_kernel)),YES)
	@echo "KPKG_SELECTED_PATCHES = $(KPKG_SELECTED_PATCHES)" >> .mak
endif
ifeq ($(strip $(MODULES_ENABLED)),YES)
	@echo "KPKG_SELECTED_MODULES = $(KPKG_SELECTED_MODULES)" >> .mak
endif
	@echo "Debian Revision  = $(debian)"        >> .mak
	@echo "KPKG_ARCH        = $(KPKG_ARCH)"        >> .mak
# Fetch the rest of the information from the kernel's Makefile
	$(eval $(which_debdir))
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	@$(MAKE) --no-print-directory -sf $(DEBDIR)/ruleset/kernel_version.mk  \
          ARCH=$(KERNEL_ARCH) $(CROSS_ARG) debian_conf_var              >> .mak
endif
	@echo "do_parallel      = $(do_parallel)"   >> .mak
	@echo "fast_dep         = $(fast_dep)"      >> .mak
#	@sed -e 's%$(TOPDIR)%$$(TOPDIR)%g' .mak     > conf.vars
# Use the kernel's Makefile to calculate the TOPDIR.
# TOPDIR is obsolete in 2.6 kernels, so the kernel_version.mk
# will get us the right answer
	@echo $(shell $(MAKE) --no-print-directory -sf $(DEBDIR)/ruleset/kernel_version.mk debian_TOPDIR 2>/dev/null | tail -n 1) >/dev/null
	@sed -e 's%$(shell $(MAKE) --no-print-directory -sf $(DEBDIR)/ruleset/kernel_version.mk debian_TOPDIR 2>/dev/null | tail -n 1)%$$(TOPDIR)%g' .mak     > conf.vars
	@rm -f .mak

debian/dummy_do_dep:
	$(REASON)
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	+$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) \
                                 ARCH=$(KERNEL_ARCH) $(fast_dep) dep
else
  ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
	$(PMAKE) -C $(architecture)/compile/GENERIC depend
  endif
endif


debian/stamp-kernel-conf: .config Makefile
	$(REASON)
	$(eval $(which_debdir))
	$(eval $(deb_rule))
ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
	mkdir -p bin
	ln -sf `which gcc-3.4` bin/cc
	cd $(architecture)/conf && freebsd-config GENERIC
endif
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) \
                $(config_target)
  ifeq ($(shell if   [ $(VERSION) -gt 2 ]; then                            \
                   echo new;                                               \
                elif [ $(VERSION) -ge 2 ] && [ $(PATCHLEVEL) -ge 5 ]; then \
                  echo new;                                                \
                fi),)
	+$(run_command)  debian/dummy_do_dep
	$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) \
                                 ARCH=$(KERNEL_ARCH) clean
  else
    ifeq ($(strip $(MAKING_VIRTUAL_IMAGE)),)
	$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) prepare
    endif
  endif
endif
	echo done > $@

STAMPS_TO_CLEAN += debian/stamp-kernel-conf

debian/stamp-prepare: .config Makefile
	$(REASON)
	$(eval $(which_debdir))
	$(eval $(deb_rule))
ifeq ($(shell if   [ $(VERSION) -gt 2 ]; then                            \
                 echo new;                                               \
              elif [ $(VERSION) -ge 2 ] && [ $(PATCHLEVEL) -ge 5 ]; then \
                echo new;                                                \
              fi),)
	$(run_command) stamp-build-arch
else
	$(MAKE) $(EXTRAV_ARG) $(FLAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) prepare
endif
	echo done > $@
STAMPS_TO_CLEAN += debian/stamp-prepare

debian/stamp-conf:
	$(REASON)
ifneq ($(strip $(HAVE_VERSION_MISMATCH)),)
	@echo "The changelog says we are creating $(saved_version)."
	@echo "However, I thought the version is $(version)"
	exit 3
endif
	# work around idiocy in recent kernel versions
	test ! -e scripts/package/builddeb || \
            mv -f scripts/package/builddeb scripts/package/builddeb.kpkg-dist
	test ! -e scripts/package/Makefile || \
            (mv -f scripts/package/Makefile scripts/package/Makefile.kpkg-dist && \
               (echo "# Dummy file "; echo "help:") >  scripts/package/Makefile)
	@test -f $(LIBLOC)/rules || \
            echo Error: Could not find $(LIBLOC)/rules
	test -d debian || mkdir ./debian
	test ! -e stamp-building || rm -f stamp-building
	test ! -f ./debian || test -f stamp-debian || test -f debian/official || \
               (rm -rf ./debian && mkdir ./debian)
ifeq ($(strip $(patch_the_kernel)),YES)
	test ! -f applied_patches || rm -f applied_patches
  ifneq ($(strip $(valid_patches)),)
	for patch in $(valid_patches) ; do            \
          if test -x  $$patch; then                    \
              if $$patch; then                         \
                  echo "Patch $$patch processed fine"; \
		  echo "$(notdir $$patch)" >> applied_patches;   \
              else                                     \
                   echo "Patch $(notdir $$patch)  failed.";      \
                   echo "Hit return to Continue";      \
		   read ans;                           \
              fi;                                      \
	  fi;                                          \
        done
	echo done >  stamp-patch
  endif
endif
	test -f stamp-debian  ||                                           \
          ( test -f debian/official && test -f debian/control) ||          \
	   sed -e 's/=V/$(version)/g'         -e 's/=D/$(debian)/g'        \
	       -e 's/=A/$(DEB_HOST_ARCH)/g'   -e 's/=SA/$(INT_SUBARCH)/g'  \
                -e 's/=L/$(int_loaderdep) /g' -e 's/=I/$(initrddep)/g'     \
                -e 's/=CV/$(VERSION).$(PATCHLEVEL)/g'                      \
                -e 's/=M/$(maintainer) <$(email)>/g'                       \
                -e 's/=ST/$(INT_STEM)/g'      -e 's/=B/$(KERNEL_ARCH)/g'   \
		         $(CONTROL)> debian/control
	test -f stamp-debian  ||    test -f debian/official ||                \
           sed -e 's/=V/$(version)/g' -e 's/=D/$(debian)/g'                   \
	    -e 's/=A/$(DEB_HOST_ARCH)/g' -e 's/=M/$(maintainer) <$(email)>/g' \
            -e 's/=ST/$(INT_STEM)/g'     -e 's/=B/$(KERNEL_ARCH)/g'           \
		$(LIBLOC)/changelog > debian/changelog
	test  -f debian/rules || install -p -m 755 $(LIBLOC)/rules debian/rules
	test  -f stamp-debian || test -f debian/official ||                     \
	   for file in $(DEBIAN_FILES); do                                      \
               cp -f  $(LIBLOC)/$$file ./debian/;                               \
           done
	test  -f stamp-debian || test -f debian/official ||                     \
	   for dir  in $(DEBIAN_DIRS);  do                                      \
             cp -af $(LIBLOC)/$$dir  ./debian/;                                 \
           done
	echo done >  stamp-debian
	echo done >  $@

STAMPS_TO_CLEAN += stamp-patch debian/stamp-conf
STAMPS_TO_CLEAN += stamp-debian

# Perhaps a list of patches should be dumped to a file on patching? so we
# only unpatch what we have applied? That would be changed, though saner,
# behaviour
unpatch_now:
	$(REASON)
ifneq ($(strip $(valid_unpatches)),)
	-for patch in $(valid_unpatches) ; do              \
          if test -x  $$patch; then                        \
              if $$patch; then                             \
                  echo "Removed Patch $$patch ";           \
              else                                         \
                   echo "Patch $$patch  failed.";          \
                   echo "Hit return to Continue";          \
		   read ans;                               \
              fi;                                          \
	  fi;                                              \
        done
	rm -f stamp-patch
endif

real_stamp_clean:
	$(REASON)
	@echo running clean
	test ! -f scripts/package/builddeb.kpkg-dist ||                     \
          mv -f scripts/package/builddeb.kpkg-dist scripts/package/builddeb
	test ! -f scripts/package/Makefile.kpkg-dist ||                     \
          mv -f scripts/package/Makefile.kpkg-dist scripts/package/Makefile
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	test ! -f .config  || cp -pf .config config.precious
	test ! -f Makefile || \
            $(MAKE) $(FLAV_ARG) $(EXTRAV_ARG) $(CROSS_ARG) ARCH=$(KERNEL_ARCH) distclean
	test ! -f config.precious || mv -f config.precious .config
else
	rm -f .config
  ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
	rm -rf bin
	if test -e $(architecture)/compile/GENERIC ; then     \
	  $(PMAKE) -C $(architecture)/compile/GENERIC clean ; \
	fi
  endif
endif
	$(eval $(deb_rule))
ifeq ($(strip $(patch_the_kernel)),YES)
	$(run_command) unpatch_now
endif
ifeq ($(strip $(NO_UNPATCH_BY_DEFAULT)),)
	test ! -f stamp-patch || $(run_command) unpatch_now
endif
	test -f stamp-building || test -f debian/official || rm -rf debian
	# work around idiocy in recent kernel versions
	test ! -e scripts/package/builddeb.dist || \
            mv -f scripts/package/builddeb.dist scripts/package/builddeb
	test ! -e scripts/package/Makefile.dist || \
            mv -f scripts/package/Makefile.dist scripts/package/Makefile
	rm -f $(FILES_TO_CLEAN) $(STAMPS_TO_CLEAN)
	rm -rf $(DIRS_TO_CLEAN)



debian/stamp-build-kernel: sanity_check debian/stamp-kernel-conf
	$(REASON)
	@echo "This is kernel package version $(kpkg_version)."
# Builds the binary package.
# debian.config contains the current idea of what the image should
# have.
ifneq ($(strip $(HAVE_VERSION_MISMATCH)),)
	@echo "The changelog says we are creating $(saved_version)"
	@echo "However, I thought the version is $(version)"
	exit 1
endif
	$(if $(subst $(strip $(UTS_RELEASE_VERSION)),,$(strip $(version))),               \
	  if [ -f $(UTS_RELEASE_HEADER) ]; then                                           \
             uts_ver=$$(grep 'define UTS_RELEASE' $(UTS_RELEASE_HEADER) |                 \
                perl -nle  'm/^\s*\#define\s+UTS_RELEASE\s+("?)(\S+)\1/g && print $$2;'); \
	    if [ "X$$uts_ver" != "X$(strip $(UTS_RELEASE_VERSION))" ]; then               \
                echo "The UTS Release version in $(UTS_RELEASE_HEADER)";                  \
	        echo "     \"$$uts_ver\" ";                                               \
                echo "does not match current version " ;                                  \
                echo "     \"$(strip $(version))\" " ;                                    \
                echo "Reconfiguring." ;                                                   \
                touch Makefile;                                                           \
             fi;                                                                          \
	  fi)
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
	$(MAKE) $(do_parallel) $(EXTRAV_ARG) $(FLAV_ARG) ARCH=$(KERNEL_ARCH) \
	                    $(CROSS_ARG) $(target)
  ifneq ($(strip $(shell grep -E ^[^\#]*CONFIG_MODULES $(CONFIG_FILE))),)
	$(MAKE) $(do_parallel) $(EXTRAV_ARG) $(FLAV_ARG) ARCH=$(KERNEL_ARCH) \
	                    $(CROSS_ARG) modules
  endif
else
  ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
	$(PMAKE) -C $(architecture)/compile/GENERIC
  endif
endif
	COLUMNS=150 dpkg -l 'gcc*' perl dpkg 'libc6*' binutils make dpkg-dev |\
         awk '$$1 ~ /[hi]i/ { printf("%s-%s\n", $$2, $$3) }'> debian/buildinfo
	@echo this was built on a machine with the kernel: >> debian/buildinfo
	uname -a >> debian/buildinfo
	echo using the compiler: >> debian/buildinfo
	grep LINUX_COMPILER include/linux/compile.h | \
           sed -e 's/.*LINUX_COMPILER "//' -e 's/"$$//' >> debian/buildinfo
ifneq ($(strip $(shell test -f version.Debian && cat version.Debian)),)
	echo kernel source package used: >> debian/buildinfo
	echo $(INT_STEM)-source-$(shell cat version.Debian) >> debian/buildinfo
endif
	echo applied kernel patches: >> debian/buildinfo
ifneq ($(strip $(valid_patches)),)
	COLUMNS=150 dpkg -l $(shell dpkg -S $(valid_patches) | cut -d: -f1) | \
	      awk '$$1 ~ /[hi]i/  { printf("%s-%s\n", $$2, $$3) }' >> debian/buildinfo
endif
	echo done > $@

STAMPS_TO_CLEAN += debian/stamp-build-kernel

$(eval $(which_debdir))
include $(DEBDIR)/ruleset/targets/sanity_check.mk
include $(DEBDIR)/ruleset/targets/source.mk
include $(DEBDIR)/ruleset/targets/headers.mk
include $(DEBDIR)/ruleset/targets/manual.mk
include $(DEBDIR)/ruleset/targets/doc.mk
include $(DEBDIR)/ruleset/targets/image.mk

#Local variables:
#mode: makefile
#End:
