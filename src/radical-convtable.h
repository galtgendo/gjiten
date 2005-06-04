#ifndef _RADICAL_CONVTABLE_H_
#define _RADICAL_CONVTABLE_H_
typedef struct _radpair {
    gchar *jis;
    gchar *uni;
} radpair;
radpair radicaltable[] = {
/*  jis to unicode radical mapping */
/*  http://www.unicode.org/charts/PDF/U2E80.pdf */

  { "\xE5\x8C\x96", "\xE4\xBA\xBB" },
/* 		个	人 */
  { "\xE5\x88\x88", "\xE5\x88\x82" },
  { "\xE8\xBE\xBC", "\xE8\xBE\xB6" },
/* 		尚	小 */
  { "\xE5\xBF\x99", "\xE5\xBF\x84" },
  { "\xE6\x89\x8E", "\xE6\x89\x8C" },
  { "\xE6\xB1\x81", "\xE6\xB0\xB5" },
  { "\xE7\x8A\xAF", "\xE7\x8A\xAD" },
  { "\xE8\x89\xBE", "\xE8\x89\xB9" },
/* 		邦	 */
  { "\xE9\x98\xA1", "\xE9\x98\x9D" },
/* 		老	耂 */
  { "\xE6\x9D\xB0", "\xE7\x81\xAC" },
  { "\xE7\xA4\xBC", "\xE7\xA4\xBB" },
/* 		禹	?? */
  { "\xE5\x88\x9D", "\xE8\xA1\xA4" },
/* 		買	⺲ ⺫ */

  { "\x30", "\x30" },
};

#endif /* _CONVTABLE_H_ */
