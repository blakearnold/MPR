#!/usr/bin/perl -w

%configs = ();
%common = ();

print "Reading config's ...\n";

opendir(DIR, ".");

while (defined($config = readdir(DIR))) {
	# Only config.*
	next if $config !~ /^config\..*/;
	# Nothing that is disabled, or remnant
	next if $config =~ /.*\.(default|disabled|stub)$/;
	# Server config's are standalone
	#next if $config =~ /config.server-.*/;

	%{$configs{$config}} = ();

	print "  processing $config ... ";

	open(CONFIG, "< $config");

	while (<CONFIG>) {
		/^#*\s*CONFIG_(\w+)[\s=](.*)$/ or next;

		${$configs{$config}}{$1} = $2;

		$common{$1} = $2;
	}

	close(CONFIG);

	print "done.\n";
}

closedir(DIR);

print "\n";

print "Merging lists ... \n";

for $config (keys(%configs)) {
	my %options = %{$configs{$config}};

	print "   processing $config ... ";

	for $key (keys(%common)) {
		next if not defined $common{$key};

		# If we don't have the common option, then it isn't
		# common. If we do have that option, it must have the same
		# value (this is where the old split.py was broken). It
		# also did the common check while it was parsing files, so
		# that there were cases where a non-common option was in
		# common anyway (ordering).
		if (not defined($options{$key})) {
			undef $common{$key};
		} elsif ($common{$key} ne $options{$key}) {
			undef $common{$key};
		}
	}

	print "done.\n";
}

print "\n";

print "Creating common config ... ";

open(COMMON, "> config");
print COMMON "#\n# Common config options automatically generated by splitconfig.pl\n#\n";

for $key (sort(keys(%common))) {
	next if not defined $common{$key};

	if ($common{$key} eq "is not set") {
		print COMMON "# CONFIG_$key is not set\n";
	} else {
		print COMMON "CONFIG_$key=$common{$key}\n";
	}
}
close(COMMON);

print "done.\n\n";

print "Creating stub configs ...\n";

for $config (keys(%configs)) {
	my %options = %{$configs{$config}};

	print "  processing $config ... ";

	open(STUB, "> $config");
	print STUB "#\n# Config options for $config automatically generated by splitconfig.pl\n#\n";

	for $key (sort(keys(%options))) {
		next if defined $common{$key};

		if ($options{$key} eq "is not set") {
			print STUB "# CONFIG_$key is not set\n";
		} else {
			print STUB "CONFIG_$key=$options{$key}\n";
		}
	}

	close(STUB);

	print "done.\n";
}
