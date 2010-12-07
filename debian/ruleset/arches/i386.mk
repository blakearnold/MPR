######################### -*- Mode: Makefile-Gmake -*- ########################
## i386.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:31:10 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Apr 12 15:17:16 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 14
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : handle the architecture specific variables.
## 
## arch-tag: 81e94c69-cffd-4647-b6d2-0cd943160d0d
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

# sub archs can be i386 i486 i586 i686
GUESS_SUBARCH:=$(shell if test -f .config; then \
                      perl -nle '/^CONFIG_M(.86)=y/ && print "$$1"' .config;\
                     else \
                       uname -m;\
                     fi)

ifeq (,$(findstring $(KPKG_SUBARCH), xen i386 i486 i586 i686))
  KPKG_SUBARCH:=$(GUESS_SUBARCH)
endif
DEBCONFIG= $(CONFDIR)/config.$(KPKG_SUBARCH)
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
  ifeq ($(strip $(CONFIG_X86_XEN)),)
    kimagesrc = $(strip arch/$(KERNEL_ARCH)/boot/$(kimage))
    kimagedest = $(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
    kelfimagesrc = vmlinux
    kelfimagedest = $(INT_IMAGE_DESTDIR)/vmlinux-$(version)
    loaderdep=lilo (>= 19.1) | grub
    loader=lilo
    loaderdoc=LiloDefault
    kimage := bzImage
    target = $(kimage)
  else
    kimagesrc = vmlinux
    ifeq ($(strip $(CONFIG_XEN_PRIVILEGED_GUEST)),)
      kimagedest = $(INT_IMAGE_DESTDIR)/xenu-linux-$(version)
    else
      kimagedest = $(INT_IMAGE_DESTDIR)/xen0-linux-$(version)
    endif
    loaderdep=grub,xen-vm
    loader=grub
    loaderdoc=
    kimage := vmlinux
    target = $(kimage)
  endif
else
  loaderdep=grub | grub2
  loader=grub
  ifeq ($(DEB_HOST_GNU_SYSTEM), kfreebsd-gnu)
    kimagesrc = $(strip $(KERNEL_ARCH)/compile/GENERIC/kernel)
    kimagedest = $(INT_IMAGE_DESTDIR)/kfreebsd-$(version)
  endif
endif

#Local variables:
#mode: makefile
#End:
