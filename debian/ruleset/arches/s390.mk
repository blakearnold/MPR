######################### -*- Mode: Makefile-Gmake -*- ########################
## s390.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:31:05 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Mon Oct 31 18:31:05 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : handle the architecture specific variables.
## 
## arch-tag: c30bdf32-18da-4916-af32-8169d1bedd49
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

# make it possible to build s390x kernels on s390 for 2.4 kernels only
# because 2.6 always use s390 as architecture.
ifeq (4,$(PATCHLEVEL))
  ifeq (,$(findstring $(KPKG_SUBARCH),s390 s390x))
    KPKG_SUBARCH = s390
  endif
  KERNEL_ARCH = $(KPKG_SUBARCH)
  ifneq ($(shell uname -m),$(KPKG_SUBARCH))
    UNAME_MACHINE = $(KPKG_SUBARCH)
    export UNAME_MACHINE
  endif
endif
kimage := zimage
loaderdep=zipl
loader=zipl
loaderdoc=
target = image
NEED_DIRECT_GZIP_IMAGE=NO
kimagesrc = $(strip arch/$(KERNEL_ARCH)/boot/$(target))
kimagedest = $(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
DEBCONFIG= $(CONFDIR)/config.$(KPKG_SUBARCH)
kelfimagesrc = vmlinux
kelfimagedest = $(INT_IMAGE_DESTDIR)/vmlinux-$(version)

#Local variables:
#mode: makefile
#End:
