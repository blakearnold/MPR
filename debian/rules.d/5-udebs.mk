binary-udebs: binary-debs binary-custom debian/control
	dh_testdir
	dh_testroot

	# unpack the kernels into a temporary directory
	mkdir -p debian/d-i-${arch}

	imagelist=$$(cat kernel-versions | grep ^${arch} | awk '{print $$4}') && \
	for i in $$imagelist; do \
	  dpkg -x $$(ls ../linux-image-$$i\_*${arch}.deb) \
		debian/d-i-${arch}; \
	done

	export SOURCEDIR=debian/d-i-${arch} && \
	  kernel-wedge install-files && \
	  kernel-wedge check

        # Build just the udebs
	dilist=$$(dh_listpackages -s | grep "\-di$$") && \
	[ -z "$dilist" ] || \
	for i in $$dilist; do \
	  dh_fixperms -p$$i; \
	  dh_gencontrol -p$$i; \
	  dh_builddeb -p$$i; \
	done
