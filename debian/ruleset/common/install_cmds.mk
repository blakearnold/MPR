######################### -*- Mode: Makefile-Gmake -*- ########################
## install_cmds.mk --- 
## Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
## Created On       : Fri Jun 16 14:40:20 2006
## Created On Node  : glaurung.internal.golden-gryphon.com
## Last Modified By : Manoj Srivastava
## Last Modified On : Wed Sep  6 12:23:42 2006
## Last Machine Used: glaurung.internal.golden-gryphon.com
## Update Count     : 3
## Status           : Unknown, Use with caution!
## HISTORY          : 
## Description      : helps install files, scripts, and programs as part of
##                    packaging 
## 
## arch-tag: a38b6a93-2539-4034-9060-ae94d5c8a071
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

# install commands
install_file   = install -p    -m 644
install_script = install -p    -m 755
install_program= install -p    -m 755
make_directory = install -p -d -m 755

define create_md5sums
    create_md5sums_fn () { \
        cd $$1 ; \
	find . -type f \
	       ! -regex '.*/DEBIAN/.*' \
               ! -regex './etc/.*'     $(EXTRA_MD5SUM_EXCLUDE)      \
	       ! -regex '.*lib/modules/[^/]*/modules\..*' \
	       -printf '%P\0' | xargs -r0 md5sum > DEBIAN/md5sums ; \
	if [ -z "DEBIAN/md5sums" ] ; then \
	    rm -f "DEBIAN/md5sums" ; \
	fi ; \
    } ; \
    create_md5sums_fn
endef

#Local variables:
#mode: makefile
#End:
