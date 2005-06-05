#!/usr/bin/perl

#
# Perl script to download EDICT dictionaries for gjiten.
#
# It will fetch and unpack dicfiles then configure the gconf settings.
# Dicfiles will be placed under $HOME/.gjiten
#

use strict;


my $TMPDIR = "/tmp/gjiten-dics";
my $DLDIR = "$TMPDIR/dl";

my $UTFDICDIR = "$ENV{HOME}/gjiten";


# binaries
my $WGET = "/usr/bin/wget";
my $GUNZIP = "/bin/gunzip";
my $UNZIP = "/usr/bin/unzip";
my $ICONV = "/usr/bin/iconv";
my $GCONFTOOL = "/usr/bin/gconftool-2";

#my $DELETETMP = 0;  # clean up after finishing?

die "couldn't get home directory!\n" if "$ENV{HOME}" eq "";

my @dics = (
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/edict.gz",
		"package"     => "edict.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "edict",
		"targetfile"  => "edict",
		"name"        => "English-main",
		"section"     => "basic",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/kanjidic.gz",
		"package"     => "kanjidic.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "kanjidic",
		"targetfile"  => "kanjidic",
		"name"        => "KanjiDic",
		"section"     => "kanjidic",
	    },

# EXTRA
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/enamdict.gz",
		"package"     => "enamdict.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "enamdict",
		"targetfile"  => "enamdict",
		"name"        => "Japanese Names",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/j_places.gz",
		"package"     => "j_places.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "j_places",
		"targetfile"  => "j_places",
		"name"        => "Japanese Places",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/lifscdic.Z",
		"package"     => "lifscdic.Z",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "lifscdic",
		"targetfile"  => "lifescidic",
		"name"        => "Science",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/engscidic.gz",
		"package"     => "engscidic.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "engscidic",
		"targetfile"  => "engscidic",
		"name"        => "Science2",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/compdic.gz",
		"package"     => "compdic.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "compdic",
		"targetfile"  => "compdic",
		"name"        => "Computers&IT",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/geodic.gz",
		"package"     => "geodic.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "geodic",
		"targetfile"  => "geodic",
		"name"        => "Geology",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/compverb.gz",
		"package"     => "compverb.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "compverb",
		"targetfile"  => "compverb",
		"name"        => "Compound Verbs",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/riverwater.zip",
		"package"     => "riverwater.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "riverwater",
		"targetfile"  => "riverwater",
		"name"        => "RiverWater",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/4jword2.gz",
		"package"     => "4jword2.gz",
		"format"      => "gzip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "4jword2",
		"targetfile"  => "4jword2",
		"name"        => "4-kanji expressions",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/classical.zip",
		"package"     => "classical.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "classical",
		"targetfile"  => "classical",
		"name"        => "Classical Japanese",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/lingdic.zip",
		"package"     => "lingdic.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "lingdic",
		"targetfile"  => "lingdic",
		"name"        => "Linguistics",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/forsdic.zip",
		"package"     => "forsdic.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "forsdic_e",
		"targetfile"  => "forsdic",
		"name"        => "Forestry",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/aviation.zip",
		"package"     => "aviation.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "aviation",
		"targetfile"  => "aviation",
		"name"        => "Aviation",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/concrete.zip",
		"package"     => "concrete.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "concrete",
		"targetfile"  => "concrete",
		"name"        => "Concrete",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/findic.zip",
		"package"     => "findic.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "findic",
		"targetfile"  => "findic",
		"name"        => "Financial",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/mktdic.zip",
		"package"     => "mktdic.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "mktdic",
		"targetfile"  => "mktdic",
		"name"        => "Marketing",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/stardict.zip",
		"package"     => "stardict.zip",
		"format"      => "zip",
		"encoding"    => "EUC-JP",
		"sourcefile"  => "stardict",
		"targetfile"  => "stardict",
		"name"        => "Stars&Constellations",
		"section"     => "extra",
	    },
	    { 
		"url"         => "http://ftp.cc.monash.edu.au/pub/nihongo/buddhdic.zip",
		"package"     => "buddhdic.zip",
		"format"      => "zip",
		"encoding"    => "UTF-8",
		"sourcefile"  => "buddhdicu",
		"targetfile"  => "buddhdic",
		"name"        => "Buddhism",
		"section"     => "extra",
	    },
# GERMAN
	    { 
		"url"         => "http://bibiko.de/WdJTUTF.zip",
		"package"     => "WdJTUTF.zip",
		"format"      => "zip",
		"encoding"    => "UTF-8",
		"sourcefile"  => "WdJTUTF.txt",
		"targetfile"  => "wadoku",
		"name"        => "Japanese-German",
		"section"     => "extra",
	    },
# CHINESE
	    { 
		"url"         => "http://www.mandarintools.com/download/cedictu8.zip",
		"package"     => "cedictu8.zip",
		"format"      => "zip",
		"encoding"    => "UTF-8",
		"sourcefile"  => "cedict_ts.u8",
		"targetfile"  => "cedict",
		"name"        => "Chinese-English",
		"section"     => "extra",
	    },

	    );



if (! -d $TMPDIR) {
    mkdir $TMPDIR or die "couldn't create $TMPDIR";
}
if (! -d $DLDIR) {
    mkdir $DLDIR or die "couldn't create $DLDIR";
}
if (! -d $UTFDICDIR) {
    mkdir $UTFDICDIR or die "couldn't create $UTFDICDIR";
}

foreach my $dic (@dics) {

# DOWNLOAD    
#wget -c http://ftp.cc.monash.edu.au/pub/nihongo/edict.gz -O dl/edict.gz
    if (! -f "$DLDIR/$dic->{package}") {
	print "Downloading \"$dic->{name}\" from $dic->{url}\n";
	my @wget = ("$WGET -q -c $dic->{url} -O $DLDIR/$dic->{package}");
	if (system(@wget) != 0) {
	    $dic->{status} = "download failed";
	    next;
	}
    }
    $dic->{status} = "downloaded";


# UNPACK
#gunzip   <dl/edict.gz >tmp/edict
#unzip -o  dl/classical.zip -d tmp
    if (! -f "$DLDIR/$dic->{sourcefile}") {
	my @unpack;
	if ($dic->{format} eq "gzip") {
	    @unpack = ("$GUNZIP <$DLDIR/$dic->{package} >$DLDIR/$dic->{sourcefile}");
	    #print join(" ", @unpack) . "\n";
	    if (system(@unpack) != 0) {
		$dic->{status} = "unpack failed";
		next;
	    }
	}
	elsif ($dic->{format} eq "zip") {
	    @unpack = ("$UNZIP -o $DLDIR/$dic->{package} -d $DLDIR");
	    if (system(@unpack) != 0) {
		$dic->{status} = "unpack failed";
		next;
	    }
	}
	elsif ($dic->{format} ne "uncompressed") {
	    $dic->{status} = "unpack failed";
	    next;
	}
    }
    $dic->{status} = "unpacked";

# CONVERT
#iconv -c -f EUC-JP -t UTF-8 tmp/edict -o dics/edict

    if (! -f "$UTFDICDIR/$dic->{targetfile}") {
	my @iconv;
	if ($dic->{encoding} ne "UTF-8") {
	    #print "$ICONV -c -f $dic->{encoding} -t UTF-8 $DLDIR/$dic->{sourcefile} -o $DLDIR/$dic->{targetfile}.utf8\n";
	    @iconv = ("$ICONV -c -f $dic->{encoding} -t UTF-8 $DLDIR/$dic->{sourcefile} -o $DLDIR/$dic->{targetfile}.utf8");
	    if (system(@iconv) != 0) {
		$dic->{status} = "iconv failed";
		next;
	    }
	    else {
		$dic->{sourcefile} = "$dic->{targetfile}.utf8";
	    }
	}
    }
    $dic->{status} = "converted";
    

# MOVE

    if (! -f "$UTFDICDIR/$dic->{targetfile}") {
	if (system("/bin/mv $DLDIR/$dic->{sourcefile} $UTFDICDIR/$dic->{targetfile}") != 0) {
	    $dic->{status} = "move failed";
	    next;
	}
    }
    $dic->{status} = "OK";


}

my $gconfstrg = "[";
my $kanjidicgconf = "";

foreach my $dic (@dics) {
    print "[$dic->{name}] $dic->{status}\n";
    if ($dic->{status} eq "OK") {
	if ($dic->{section} eq "kanjidic") {
	    $kanjidicgconf = "$UTFDICDIR/$dic->{targetfile}\n$dic->{name}";
	}
	else {
	    $gconfstrg .= "$UTFDICDIR/$dic->{targetfile}\n$dic->{name},";
	}
    }
}
chop($gconfstrg);
    $gconfstrg .= "]";

if (system("$GCONFTOOL --type list --list-type=string --set /apps/gjiten/general/dictionary_list \"$gconfstrg\"") != 0) {
    print "Setting up dicfiles for gjiten failed.\n";
}

if ($kanjidicgconf ne "") {
#    print "$GCONFTOOL --type string --set /apps/gjiten/kanjidic/kanjidicfile \"$kanjidicgconf\"\n";
    if (system("$GCONFTOOL --type string --set /apps/gjiten/kanjidic/kanjidicfile \"$kanjidicgconf\"") != 0) {
	print "Setting up kanjidic for gjiten failed.\n";
    }
}

print "Please run \"rm -rf $TMPDIR\" if everything is set up correctly!\n";

