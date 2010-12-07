######################### -*- Mode: Makefile-Gmake -*- ########################
## amd64.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Mon Oct 31 18:31:11 2005
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Mon Oct 31 18:31:11 2005
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 0
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : handle the architecture specific variables.
## 
## arch-tag: 0429f056-d3a2-43d3-a02b-78bf0f655633
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

KERNEL_ARCH=x86_64
ifeq ($(DEB_HOST_GNU_SYSTEM), linux-gnu)
  ifeq ($(strip $(CONFIG_X86_64_XEN)),)
    kimage := bzImage
    loaderdep=lilo (>= 19.1) | grub
    loader=lilo
    loaderdoc=LiloDefault
    target = $(kimage)
    kimagesrc = $(strip arch/$(KERNEL_ARCH)/boot/$(kimage))
    kimagedest = $(INT_IMAGE_DESTDIR)/vmlinuz-$(version)
    DEBCONFIG= $(CONFDIR)/config.$(KPKG_SUBARCH)
    kelfimagesrc = vmlinux
    kelfimagedest = $(INT_IMAGE_DESTDIR)/vmlinux-$(version)
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
endif

#Local variables:
#mode: makefile
#End:
