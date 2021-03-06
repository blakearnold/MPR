# Check ABI for package against last release (if not same abinum)
abi-%: $(abidir)/%
	@# Empty for make to be happy
$(abidir)/%: $(stampdir)/stamp-build-%
	install -d $(abidir)
	sed -e 's/^\(.\+\)[[:space:]]\+\(.\+\)[[:space:]]\(.\+\)$$/\3 \2 \1/'	\
		$(builddir)/build-$*/Module.symvers | sort > $@

abi-check-%: $(abidir)/%
	@$(SHELL) debian/scripts/abi-check "$*" "$(prev_abinum)" "$(abinum)" \
		"$(prev_abidir)" "$(abidir)" "$(skipabi)"

# Check the module list against the last release (always)
module-%: $(abidir)/%.modules
	@# Empty for make to be happy
$(abidir)/%.modules: $(stampdir)/stamp-build-%
	install -d $(abidir)
	find $(builddir)/build-$*/ -name \*.ko | \
		sed -e 's/.*\/\([^\/]*\)\.ko/\1/' | sort > $@

module-check-%: $(abidir)/%.modules
	@perl -f debian/scripts/module-check "$*" \
		"$(prev_abidir)" "$(abidir)" $(skipmodule)

checks-%: abi-check-% module-check-%
	@# Will be calling more stuff later
