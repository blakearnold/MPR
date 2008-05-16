build_arch	= i386
header_arch	= x86_64
asm_link	= x86
defconfig	= defconfig
#
# Only build -386 and -generic for PPA.
#
flavours = 386 generic
flavours	+= server virtual
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

#
# No custom binaries for the PPA build.
#
custom_flavours	= rt xen openvz
