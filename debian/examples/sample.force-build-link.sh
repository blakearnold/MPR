#!/bin/sh
#                               -*- Mode: Sh -*- 
# sample.force-build-link.sh --- 
# Author           : Manoj Srivastava ( srivasta@glaurung.internal.golden-gryphon.com ) 
# Created On       : Wed Mar 22 08:07:21 2006
# Created On Node  : glaurung.internal.golden-gryphon.com
# Last Modified By : Manoj Srivastava
# Last Modified On : Wed Mar 22 08:14:10 2006
# Last Machine Used: glaurung.internal.golden-gryphon.com
# Update Count     : 4
# Status           : Unknown, Use with caution!
# HISTORY          : 
# Description      : 
# 
#
# This is an example of a script that can be run as a postinst hook,
# and forces the build link to point to a headers directory, even if
# the source dir is present.
# 
# arch-tag: ca36fde4-759d-4f96-8d03-aeea4fd24f99

set -e


VERSION=$1
KERNELDIR=$2

MODULEDIR=/lib/modules/$VERSION
HEADERDIR=/usr/src/linux-headers-$VERSION

# exit silently if there is no module dir
test -d $MODULEDIR || exit 0
test -d $HEADERDIR || exit 0

# update build link -- by first deleting whatever is there
test ! -e $MODULEDIR/build || rm -f $MODULEDIR/build
# and then creating a new one
ln -sf $HEADERDIR $MODULEDIR/build
