build_arch	= parisc
header_arch	= $(build_arch)
defconfig	= defconfig
flavours	= hppa32 hppa64
build_image	= vmlinux
kernel_file	= $(build_image)
install_file	= $(build_image)

no_image_strip	= true
