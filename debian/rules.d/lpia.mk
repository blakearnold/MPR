build_arch	= i386
header_arch	= i386
asm_link	= x86
defconfig	= defconfig
flavours	= 
build_image	= bzImage
kernel_file	= arch/$(build_arch)/boot/bzImage
install_file	= vmlinuz

do_debug_image	= true

loader		= grub

#custom_flavours = lpiacompat lpia
custom_flavours = lpia

#
# Set this variable to 'true' in order to
# avoid building udebs for the debian installer.
#
disable_d_i    = true
