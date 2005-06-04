#!/usr/bin/perl

open(NEWHEAD, ">radical-convtable.h") || die "Can't open\n";
select(NEWHEAD);
print "#ifndef _RADICAL_CONVTABLE_H_\n";
print "#define _RADICAL_CONVTABLE_H_\n";
open(TABLE, "radical-convtable.txt") || die "Can't open\n";


print <<__STRUCT__;
typedef struct _radpair {
    gchar *jis;
    gchar *uni;
} radpair;
__STRUCT__

    
print "radpair radicaltable[] = {\n";
my %escapes;
for (0..255) {
    $escapes{chr($_)} = sprintf("\\x%02X", $_);
}

while(<TABLE>) {
  if( /^$/ ) {
    print "\n";
    next;
  }

  if( /^#(.*)$/ ) {
    print "/* $1 */\n";
    next;
  }
  @pair = split(/\s/);
  print "  { \"" . getHex($pair[0]) . "\", \"" . getHex($pair[1]) . "\" },\n";
}
print "};\n";
close(TABLE);
print "\n#endif /* _CONVTABLE_H_ */\n";
close(NEWHEAD);


sub getHex {
    my ($strg) = @_;
    my $result = '';
    foreach $chr (split('', $strg)) {
	$result .= $escapes{$chr};
    }
    return $result;
}
