build_arch	= i386
header_arch	= i386
defconfig	= defconfig
flavours	= lpia
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub
