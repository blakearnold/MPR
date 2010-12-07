############################ -*- Mode: Makefile -*- ###########################
## targets.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.green-gryphon.com ) 
## Created On       : Sat Nov 15 01:10:05 2003
## Created On Node  : glaurung.green-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Fri Sep 15 12:57:41 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 65
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : The top level targets mandated by policy, as well as
##                    their dependencies.
## 
## arch-tag: a81086a7-00f7-4355-ac56-8f38396935f4
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

#######################################################################
#######################################################################
###############             Miscellaneous               ###############
#######################################################################
#######################################################################
source diff:
	@echo >&2 'source and diff are obsolete - use dpkg-source -b'; false

testroot:
	@test $$(id -u) = 0 || (echo need root priviledges; exit 1)

checkpo:
	$(CHECKPO)

# arch-buildpackage likes to call this
prebuild: 

# OK. We have two sets of rules here, one for arch dependent packages,
# and one for arch independent packages. We have already calculated a
# list of each of these packages.

# In each set, we may need to do things in five steps: configure,
# build, install, package, and clean. Now, there can be a common
# actions to be taken for all the packages, all arch dependent
# packages, all all independent packages, and each package
# individually at each stage.

#######################################################################
#######################################################################
###############             Configuration               ###############
#######################################################################
#######################################################################

# Work here
CONFIG-common:: testdir
	$(REASON)
	$(checkdir)

stamp-arch-conf:  CONFIG-common
	$(REASON)
	$(checkdir)
	@echo done > $@
stamp-indep-conf: CONFIG-common
	$(REASON)
	$(checkdir)
	@echo done > $@

# Work here
CONFIG-arch::  stamp-arch-conf
	$(REASON)
CONFIG-indep:: stamp-indep-conf
	$(REASON)

STAMPS_TO_CLEAN += stamp-arch-conf stamp-indep-conf
# Work here
$(patsubst %,CONFIG/%,$(DEB_ARCH_PACKAGES))  :: CONFIG/% : CONFIG-arch  
	$(REASON)
	$(checkdir)
$(patsubst %,CONFIG/%,$(DEB_INDEP_PACKAGES)) :: CONFIG/% : CONFIG-indep 
	$(REASON)
	$(checkdir)

stamp-configure-arch:  $(patsubst %,CONFIG/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
	@echo done > $@
stamp-configure-indep: $(patsubst %,CONFIG/%,$(DEB_INDEP_PACKAGES))
	$(REASON)
	@echo done > $@

configure-arch:  stamp-configure-arch
	$(REASON)
configure-indep: stamp-configure-indep
	$(REASON)

stamp-configure: configure-arch configure-indep
	$(REASON)
	@echo done > $@

configure: stamp-configure
	$(REASON)

STAMPS_TO_CLEAN += stamp-configure-arch stamp-configure-indep stamp-configure
#######################################################################
#######################################################################
###############                 Build                   ###############
#######################################################################
#######################################################################

# Work here
BUILD-common:: testdir
	$(REASON)
	$(checkdir)

stamp-arch-build:  BUILD-common $(patsubst %,CONFIG/%,$(DEB_ARCH_PACKAGES))  
	$(REASON)
	$(checkdir)
	@echo done > $@
stamp-indep-build: BUILD-common $(patsubst %,CONFIG/%,$(DEB_INDEP_PACKAGES)) 
	$(REASON)
	$(checkdir)
	@echo done > $@

STAMPS_TO_CLEAN += stamp-arch-build stamp-indep-build
# sync. Work here
BUILD-arch::  stamp-arch-build
	$(REASON)
	$(checkdir)
BUILD-indep:: stamp-indep-build
	$(REASON)
	$(checkdir)

# Work here
$(patsubst %,BUILD/%,$(DEB_ARCH_PACKAGES))  :: BUILD/% : BUILD-arch  
	$(REASON)
	$(checkdir)
$(patsubst %,BUILD/%,$(DEB_INDEP_PACKAGES)) :: BUILD/% : BUILD-indep 
	$(REASON)
	$(checkdir)

stamp-build-arch:  $(patsubst %,BUILD/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
	@echo done > $@
stamp-build-indep: $(patsubst %,BUILD/%,$(DEB_INDEP_PACKAGES))
	$(REASON)
	@echo done > $@

build-arch:  stamp-build-arch
	$(REASON)
build-indep: stamp-build-indep
	$(REASON)

stamp-build: build-arch build-indep 
	$(REASON)
	@echo done > $@

build: stamp-build
	$(REASON)

# Work here
POST-BUILD-arch-stamp::
	$(REASON)
POST-BUILD-indep-stamp::
	$(REASON)

STAMPS_TO_CLEAN += stamp-build-arch stamp-build-indep stamp-build
#######################################################################
#######################################################################
###############                Install                  ###############
#######################################################################
#######################################################################
# Work here
INST-common:: testdir
	$(checkdir)
	$(REASON)

stamp-arch-inst: POST-BUILD-arch-stamp INST-common $(patsubst %,BUILD/%,$(DEB_ARCH_PACKAGES))    
	$(REASON)
	$(checkdir)
	@echo done > $@
stamp-indep-inst: POST-BUILD-indep-stamp INST-common $(patsubst %,BUILD/%,$(DEB_INDEP_PACKAGES)) 
	$(REASON)
	$(checkdir)
	@echo done > $@

STAMPS_TO_CLEAN += stamp-arch-inst stamp-indep-inst
# sync. Work here
INST-arch::  stamp-arch-inst
	$(REASON)
	$(checkdir)
INST-indep:: stamp-indep-inst
	$(REASON)
	$(checkdir)

# Work here
$(patsubst %,INST/%,$(DEB_ARCH_PACKAGES))  :: INST/% : INST-arch  
	$(REASON)
	$(checkdir)
$(patsubst %,INST/%,$(DEB_INDEP_PACKAGES)) :: INST/% : INST-indep 
	$(REASON)
	$(checkdir)

stamp-install-arch:  $(patsubst %,INST/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
	@echo done > $@
stamp-install-indep: $(patsubst %,INST/%,$(DEB_INDEP_PACKAGES))
	$(REASON)
	@echo done > $@

install-arch:  stamp-install-arch
	$(REASON)
install-indep: stamp-install-indep
	$(REASON)

stamp-install: install-indep install-arch
	$(REASON)
	@echo done > $@

install: stamp-install
	$(REASON)

STAMPS_TO_CLEAN += stamp-install stamp-install-arch stamp-install-indep
#######################################################################
#######################################################################
###############                Package                  ###############
#######################################################################
#######################################################################
# Work here
BIN-common:: testdir
	$(REASON)
	$(checkdir)

stamp-arch-bin:  BIN-common  $(patsubst %,INST/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
	$(checkdir)
	@echo done > $@
stamp-indep-bin: BIN-common  $(patsubst %,INST/%,$(DEB_INDEP_PACKAGES))
	$(REASON)
	$(checkdir)
	@echo done > $@

STAMPS_TO_CLEAN += stamp-arch-bin stamp-indep-bin
# sync Work here
BIN-arch::  stamp-arch-bin
	$(REASON)
	$(checkdir)
BIN-indep:: stamp-indep-bin
	$(REASON)
	$(checkdir)

# Work here
$(patsubst %,BIN/%,$(DEB_ARCH_PACKAGES))  :: BIN/% : BIN-arch  
	$(REASON)
	$(checkdir)
$(patsubst %,BIN/%,$(DEB_INDEP_PACKAGES)) :: BIN/% : BIN-indep 
	$(REASON)
	$(checkdir)


stamp-binary-arch:  $(patsubst %,BIN/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
	@echo done > $@
stamp-binary-indep: $(patsubst %,BIN/%,$(DEB_INDEP_PACKAGES))
	$(REASON)
	@echo done > $@
# required
binary-arch:  stamp-binary-arch
	$(REASON)
binary-indep: stamp-binary-indep
	$(REASON)

stamp-binary: binary-indep binary-arch
	$(REASON)
	@echo done > $@

# required
binary: stamp-binary
	$(REASON)
	@echo arch package   = $(DEB_ARCH_PACKAGES)
	@echo indep packages = $(DEB_INDEP_PACKAGES)

STAMPS_TO_CLEAN += stamp-binary stamp-binary-arch stamp-binary-indep
#######################################################################
#######################################################################
###############                 Clean                   ###############
#######################################################################
#######################################################################
# Work here
CLN-common:: testdir 
	$(REASON)
	$(checkdir)
# sync Work here
CLN-arch::  CLN-common
	$(REASON)
	$(checkdir)
CLN-indep:: CLN-common
	$(REASON)
	$(checkdir)
# Work here
$(patsubst %,CLEAN/%,$(DEB_ARCH_PACKAGES))  :: CLEAN/% : CLN-arch
	$(REASON)
	$(checkdir)
$(patsubst %,CLEAN/%,$(DEB_INDEP_PACKAGES)) :: CLEAN/% : CLN-indep
	$(REASON)
	$(checkdir)

clean-arch:  $(patsubst %,CLEAN/%,$(DEB_ARCH_PACKAGES))
	$(REASON)
clean-indep: $(patsubst %,CLEAN/%,$(DEB_INDEP_PACKAGES))
	$(REASON)

stamp-clean: clean-indep clean-arch
	$(REASON)
	$(checkdir)
	-rm -f  $(FILES_TO_CLEAN) $(STAMPS_TO_CLEAN)
	-rm -rf $(DIRS_TO_CLEAN)
	-rm -f core TAGS                                                     \
               `find . ! -regex '.*/\.git/.*' ! -regex '.*/\{arch\}/.*'      \
                       ! -regex '.*/CVS/.*'   ! -regex '.*/\.arch-ids/.*'    \
                       ! -regex '.*/\.svn/.*'                                \
                   \( -name '*.orig' -o -name '*.rej' -o -name '*~'       -o \
                      -name '*.bak'  -o -name '#*#'   -o -name '.*.orig'  -o \
		      -name '.*.rej' -o -name '.SUMS' -o -size 0 \)          \
                -print`

clean: stamp-clean
	$(REASON)


#######################################################################
#######################################################################
###############                                         ###############
#######################################################################
#######################################################################

.PHONY: CONFIG-common  CONFIG-indep  CONFIG-arch  configure-arch  configure-indep  configure     \
        BUILD-common   BUILD-indep   BUILD-arch   build-arch      build-indep      build         \
        INST-common    INST-indep    INST-arch    install-arch    install-indep    install       \
        BIN-common     BIN-indep     BIN-arch     binary-arch     binary-indep     binary        \
        CLN-common     CLN-indep     CLN-arch     clean-arch      clean-indep      clean         \
        $(patsubst %,CONFIG/%,$(DEB_INDEP_PACKAGES)) $(patsubst %,CONFIG/%,$(DEB_ARCH_PACKAGES)) \
        $(patsubst %,BUILD/%, $(DEB_INDEP_PACKAGES)) $(patsubst %,BUILD/%, $(DEB_ARCH_PACKAGES)) \
        $(patsubst %,INST/%,  $(DEB_INDEP_PACKAGES)) $(patsubst %,INST/%,  $(DEB_ARCH_PACKAGES)) \
        $(patsubst %,BIN/%,   $(DEB_INDEP_PACKAGES)) $(patsubst %,BIN/%,   $(DEB_ARCH_PACKAGES)) \
        $(patsubst %,CLEAN/%, $(DEB_INDEP_PACKAGES)) $(patsubst %,CLEAN/%, $(DEB_ARCH_PACKAGES)) \
        implode explode prebuild checkpo

#Local variables:
#mode: makefile
#End:
