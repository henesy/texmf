/*
 *   Loads a tfm file.  It marks the characters as undefined.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#ifdef KPATHSEA
#include <kpathsea/c-pathmx.h>
#endif
/*
 *   These are the external routines it calls:
 */
#include "protos.h"
/*
 *   Here are the external variables we use:
 */
extern real conv ;
extern real vconv ;
extern real alpha ;
#ifndef KPATHSEA
extern char *tfmpath ;
#endif
extern char errbuf[] ;
extern integer fsizetol ;
/*
 *   Our static variables:
 */
FILE *tfmfile ; 
static char name[50] ;

/*
 *   Tries to open a tfm file.  Uses cmr10.tfm if unsuccessful,
 *   and complains loudly about it.
 */
void
tfmopen P1C(register fontdesctype *, fd)
{
  register char *n;
#ifdef KPATHSEA
  kpse_file_format_type d = tfmpath;
#else
   register char *d;
   d = fd->area ;
   if (*d==0)
     d = tfmpath ;
#endif
   n = fd->name ;
#ifdef MVSXA   /* IBM: MVS/XA */
   (void)sprintf(name, "tfm(%s)", n) ;
#else
   (void)sprintf(name, "%s.tfm", n) ;
#endif
   if ((tfmfile=search(d, name, READBIN))==NULL) {
      (void)sprintf(errbuf, "Can't open font metric file %s%s",
             fd->area, name) ;
      error(errbuf) ;
      error("I will use cmr10.tfm instead, so expect bad output.") ;
#ifdef MVSXA   /* IBM: MVS/XA */
      if ((tfmfile=search(d, "tfm(cmr10)", READBIN))==NULL)
#else
      if ((tfmfile=search(d, "cmr10.tfm", READBIN))==NULL)
#endif
         error(
          "! I can't find cmr10.tfm; please reinstall me with proper paths") ;
   }
}

shalfword
tfmbyte P1H(void)
{
  return(getc(tfmfile)) ;
}

halfword
tfm16 P1H(void)
{
  register halfword a ; 
  a = tfmbyte () ; 
  return ( a * 256 + tfmbyte () ) ; 
} 

integer
tfm32 P1H(void)
{
  register integer a ; 
  a = tfm16 () ; 
  if (a > 32767) a -= 65536 ;
  return ( a * 65536 + tfm16 () ) ; 
} 

int
tfmload P1C(register fontdesctype *, curfnt)
{
   register shalfword i ;
   register integer li ;
   integer scaledsize ;
   shalfword nw, hd ;
   shalfword bc, ec ;
   integer scaled[256] ;
   halfword chardat[256] ;
   int charcount = 0 ;

   tfmopen(curfnt) ;
/*
 *   Next, we read the font data from the tfm file, and store it in
 *   our own arrays.
 */
   li = tfm16() ; hd = tfm16() ;
   bc = tfm16() ; ec = tfm16() ;
   nw = tfm16() ;
   li = tfm32() ; li = tfm32() ; li = tfm32() ; li = tfm16() ;
   li = tfm32() ;
   check_checksum (li, curfnt->checksum, curfnt->name);
   li = (integer)(alpha * (real)tfm32()) ;
   if (li > curfnt->designsize + fsizetol ||
       li < curfnt->designsize - fsizetol) {
      (void)sprintf(errbuf,"Design size mismatch in %s", name) ;
      error(errbuf) ;
   }
   for (i=2; i<hd; i++)
      li = tfm32() ;
   for (i=0; i<256; i++)
      chardat[i] = 256 ;
   for (i=bc; i<=ec; i++) {
      chardat[i] = tfmbyte() ;
      li = tfm16() ;
      li |= tfmbyte() ;
      if (li || chardat[i])
         charcount++ ;
   }
   scaledsize = curfnt->scaledsize ;
   for (i=0; i<nw; i++)
      scaled[i] = scalewidth(tfm32(), scaledsize) ;
   (void)fclose(tfmfile) ;
   for (i=0; i<256; i++)
      if (chardat[i]!= 256) {
         li = scaled[chardat[i]] ;
         curfnt->chardesc[i].TFMwidth = li ;
         if (li >= 0)
            curfnt->chardesc[i].pixelwidth = ((integer)(conv*li+0.5)) ;
         else
            curfnt->chardesc[i].pixelwidth = -((integer)(conv*-li+0.5)) ;
         curfnt->chardesc[i].flags = (curfnt->resfont ? EXISTS : 0) ;
      }
   curfnt->loaded = 1 ;
   return charcount ;
}
