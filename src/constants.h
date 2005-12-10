#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#define DEFWORDFONTNAME "-*-fixed-medium-r-normal-*-14-*-*-*-*-*-jisx0208.1983-0"
#define DEFBIGFONTNAME "-*-fixed-medium-r-normal-*-24-*-*-*-*-*-jisx0208.1983-0";
#define DEFMAXWORDMATCHES 100
#define MAXDICFILES 100

#define VINFL_FILENAME GJITEN_DATADIR"/vconj.utf8"
#define RADKFILE_NAME GJITEN_DATADIR"/radkfile.utf8"
#define GJITEN_DICDIR GJITEN_DATADIR"/dics"

#define GCONF_ROOT "/apps/gjiten"
#define GCONF_PATH_GENERAL GCONF_ROOT"/general"
#define GCONF_PATH_KANJIDIC  GCONF_ROOT"/kanjidic"


#define EXACT_MATCH 1 		//jp en
#define START_WITH_MATCH 2 	//jp
#define END_WITH_MATCH 3 	//jp
#define ANY_MATCH 4 		//jp en
#define WORD_MATCH 5 		//en


#define SRCH_OK		0
#define SRCH_FAIL	1
#define SRCH_START	2
#define SRCH_CONT	3


#endif
