build_arch	= powerpc
header_arch	= $(build_arch)
defconfig	= pmac32_defconfig
flavours	= powerpc powerpc-smp powerpc64-smp
build_image	= vmlinux
kernel_file	= $(build_image)
install_file	= $(build_image)

loader		= yaboot
