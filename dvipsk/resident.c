/*   For use with emTeX set FONTPATH to "TEXTFM"
 */
#ifndef FONTPATH
#define FONTPATH "TEXFONTS"
#endif

/*
 *   This code reads in and handles the defaults for the program from the
 *   file config.sw.  This entire file is a bit kludgy, sorry.
 */
#include "dvips.h" /* The copyright notice in that file is included too! */
#include "paths.h"
#ifdef KPATHSEA
#include <kpathsea/config.h>
#include <kpathsea/c-ctype.h>
#include <kpathsea/c-pathch.h>
#include <kpathsea/pathsearch.h>
#include <kpathsea/tex-file.h>
#include <kpathsea/variable.h>
#endif
/*
 *   This is the structure definition for resident fonts.  We use
 *   a small and simple hash table to handle these.  We don't need
 *   a big hash table.
 */
struct resfont *reshash[RESHASHPRIME] ;
/*
 *   These are the external routines we use.
 */
#include "protos.h"
/*
 *   These are the external variables we use.
 */
extern char *realnameoffile ;
#ifdef DEBUG
extern integer debug_flag;
#endif  /* DEBUG */
extern integer pagecopies ;
extern int overridemag ;
extern long bytesleft ;
extern quarterword *raster ;
extern FILE *pkfile ;
extern char *oname ;
extern integer swmem, fontmem ;
#ifndef KPATHSEA
extern char *tfmpath, *pictpath ;
extern char *pkpath ;
extern char *vfpath ;
extern char *figpath ;
extern char *configpath ;
extern char *headerpath ;
#ifdef SEARCH_SUBDIRECTORIES
extern char *fontsubdirpath ;
#endif
#endif
extern Boolean noenv ;
extern Boolean downloadpspk ;
#ifdef FONTLIB
extern char *flipath, *fliname ;
#endif
extern char *paperfmt ; 
extern char *nextstring ;
extern char *maxstring ;
extern char *warningmsg ;
extern Boolean disablecomments ;
extern Boolean compressed ;
extern Boolean partialdownload ;
extern int quiet ;
extern int filter ;
extern Boolean reverse ;
extern Boolean usesPSfonts ;
extern Boolean nosmallchars ;
extern Boolean removecomments ;
extern Boolean safetyenclose ;
extern Boolean dopprescan ;
extern integer maxsecsize ;
extern double mag ;
extern Boolean sepfiles ;
extern int actualdpi ;
extern int vactualdpi ;
extern int maxdrift ;
extern int vmaxdrift ;
extern char *printer ;
extern char *mfmode, *mflandmode ;
extern int mfmode_option;
extern int oname_option;
extern Boolean sendcontrolD ;
#ifdef SHIFTLOWCHARS
extern Boolean shiftlowchars ;
#endif
extern unsigned lastresortsizes[] ;
extern integer hoff, voff ;
extern struct papsiz *papsizes ;
extern Boolean secure ;
extern integer hpapersize, vpapersize ;
extern int landscape ;
/*
 *   To maintain a list of document fonts, we use the following
 *   pointer.
 */
struct header_list *ps_fonts_used ;
/*
 *   Our hash routine.
 */
int
hash P1C(char *, s)
{
   int h = 12 ;

   while (*s != 0)
      h = (h + h + *s++) % RESHASHPRIME ;
   return(h) ;
}
/*
 *   Reverse the hash chains.
 */
void
revpslists P1H(void) {
   register int i ;
   for (i=0; i<RESHASHPRIME; i++)
      reshash[i] = (struct resfont *)revlist(reshash[i]) ;
}
/*
 *   cleanres() marks all resident fonts as not being yet sent.
 */
void
cleanres P1H(void) {
   register int i ;
   register struct resfont *p ;
   for (i=0; i<RESHASHPRIME; i++)
      for (p=reshash[i]; p; p=p->next)
         p->sent = 0 ;
}
/*
 *   The routine that looks up a font name.
 */
struct resfont *
lookup P1C(char *, name)
{
   struct resfont *p ;

   for (p=reshash[hash(name)]; p!=NULL; p=p->next)
      if (strcmp(p->Keyname, name)==0)
         return(p) ;
   return(NULL) ;
}

struct resfont *
findPSname P1C(char *, name)
{
   register int i ;
   register struct resfont *p ;
   for (i=0; i<RESHASHPRIME; i++)
      for (p=reshash[i]; p; p=p->next) {
         if (strcmp(p->PSname, name)==0)
            return p;
      }
   return NULL;
}

/*
 *   This routine adds an entry.
 */
void
add_entry P6C(char *, TeXname, char *, PSname, char *, Fontfile,
	      char *, Vectfile, char *, specinfo, char *, downloadinfo)
{
   struct resfont *p ;
   int h ;
   if (PSname == NULL)
      PSname = TeXname ;
   p = (struct resfont *)mymalloc((integer)sizeof(struct resfont)) ;
   p->Keyname = TeXname ;
   p->PSname = PSname ;
   p->Fontfile = Fontfile;
   p->Vectfile = Vectfile;
   p->TeXname = TeXname ;
   p->specialinstructions = specinfo ;
   if (downloadinfo && *downloadinfo)
      p->downloadheader = downloadinfo ;
   else
      p->downloadheader = 0 ;
   h = hash(TeXname) ;
   p->next = reshash[h] ;
   p->sent = 0 ;
   reshash[h] = p ;
}
/*
 *   Now our residentfont routine.  Returns the number of characters in
 *   this font, based on the TFM file.
 */
extern char *infont ;
int
residentfont P1C(register fontdesctype *, curfnt)
{
   register shalfword i ;
   struct resfont *p ;

/*
 *   First we determine if we can find this font in the resident list.
 */
   if (*curfnt->area)
      return 0 ; /* resident fonts never have a nonstandard font area */
   if ((p=lookup(curfnt->name))==NULL)
      return 0 ;
/*
 *   This is not yet the correct way to do things, but it is useful as it
 *   is so we leave it in.  The problem:  if resident Times-Roman is
 *   re-encoded, then it will be downloaded as bitmaps; this is not
 *   right.  The solution will be to introduce two types of `<'
 *   directives, one that downloads fonts and one that downloads
 *   short headers that are innocuous.
 */
   if (p->Fontfile && downloadpspk) {
#ifdef DEBUG
      if (dd(D_FONTS))
         (void)fprintf(stderr,"Using PK font %s for <%s>.\n",
                                     curfnt->name, p->PSname) ;
#endif  /* DEBUG */
      return 0 ;
   }
/*
 *   We clear out some pointers:
 */
#ifdef DEBUG
   if (dd(D_FONTS))
        (void)fprintf(stderr,"Font %s <%s> is resident.\n",
                                     curfnt->name, p->PSname) ;
#endif  /* DEBUG */
   curfnt->resfont = p ;
   curfnt->name = p->TeXname ;
   for (i=0; i<256; i++) {
      curfnt->chardesc[i].TFMwidth = 0 ;
      curfnt->chardesc[i].packptr = NULL ;
      curfnt->chardesc[i].pixelwidth = 0 ;
      curfnt->chardesc[i].flags = 0 ;
   }
   add_name(p->PSname, &ps_fonts_used) ;
/*
 *   We include the font here.  But we only should need to include the
 *   font if we have a stupid spooler; smart spoolers should be able
 *   to supply it automatically.
 */
   if (p->downloadheader) {
      char *cp = p->downloadheader ;
      char *q ;

      infont = p->PSname ;
      while (1) {
         q = cp ;
         while (*cp && *cp != ' ')
            cp++ ;
         if (*cp) {
            *cp = 0 ;
            add_header(q) ;
            *cp++ = ' ' ;
         } else {
            if (strstr(q,".pfa")||strstr(q,".pfb")||
                strstr(q,".PFA")||strstr(q,".PFB"))
               break ;       /* We don't add PS font file here */
            else {
              add_header(q) ;
              break;
            }
         }
         infont = 0 ;
      }
      infont = 0 ;
   }
   i = tfmload(curfnt) ;
   if (i < 0)
      i = 1 ;
   usesPSfonts = 1 ;
   return(i) ;
}
#define INLINE_SIZE (2000)
static char was_inline[INLINE_SIZE] ;
static unsigned c_lineno;
void
bad_config P1C(char *, err)
{
   fprintf (stderr, "%s:%d:", realnameoffile, c_lineno);
   error (err);
   fprintf(stderr, " (%s)\n", was_inline) ;
}

#ifndef KPATHSEA
/*
 *   Get environment variables! These override entries in ./config.h.
 *   We substitute everything of the form ::, ^: or :$ with default,
 *   so a user can easily build on to the existing paths.
 */
static char *getpath P2C(char *, who, char *, what)
{
   if (who) {
      register char *pp, *qq ;
      int lastsep = 1 ;

      for (pp=nextstring, qq=who; *qq;) {
         if (*qq == PATHSEP) {
            if (lastsep) {
               strcpy(pp, what) ;
               pp = pp + strlen(pp) ;
            }
            lastsep = 1 ;
         } else
            lastsep = 0 ;
         *pp++ = *qq++ ;
      }
      if (lastsep) {
         strcpy(pp, what) ;
         pp = pp + strlen(pp) ;
      }
      *pp = 0 ;
      qq = nextstring ;
      nextstring = pp + 1 ;
      return qq ;
   } else
      return what ;
}
#endif
/*
 *   We use this function so we can support strings delimited by
 *   double quotes with spaces in them.  We also accept strings
 *   with spaces in them, but kill off any spaces at the end.
 */
char *configstring P2C(char *, s, int, nullok)
{
   char tstr[INLINE_SIZE] ;
   char *p = tstr ;

   while (*s && *s <= ' ')
      s++ ;
   if (*s == '"') {
      s++ ;
      while (*s != 10 && *s != 0 && *s != '"' && p < tstr+290)
         *p++ = *s++ ;
   } else {
      while (*s && p < tstr+290)
         *p++ = *s++ ;
      while (*(p-1) <= ' ' && p > tstr)
         p-- ;
   }
   *p = 0 ;
   if (p == tstr && ! nullok)
      bad_config("bad string") ;
   return newstring(tstr) ;
}
#ifdef KPATHSEA
/* We use this in `getdefaults' to modify the kpathsea structure for the
   paths we read.  See kpathsea/tex-file.[ch].  */
#define SET_CLIENT_PATH(filefmt, val) \
  kpse_format_info[filefmt].client_path = xstrdup (val)
#endif
/*
 *   Now we have the getdefaults routine.
 */
char *psmapfile = PSMAPFILE ;
Boolean
getdefaults P1C(char *, s)
{
   FILE *deffile ;
   char PSname[INLINE_SIZE] ;
   register char *p ;
   int i, j ;
   integer hsiz, vsiz ;
#ifndef KPATHSEA
   char *d = configpath ;
#endif
   int canaddtopaper = 0 ;

   if (printer == NULL) {
      if (s) {
         strcpy(PSname, s) ;
      } else {
#ifdef KPATHSEA
         char *dvipsrc = kpse_var_value ("DVIPSRC");
#ifdef WIN32
	 if (dvipsrc && *dvipsrc) {
	   /* $DVIPSRC was set by user */
	   strcpy(PSname, dvipsrc);
	 }
	 else 
	   /* No env var, looking into some kind of standard path. */
	   if (SearchPath(".;%HOME%;c:\\", ".dvipsrc", NULL,
			  INLINE_SIZE, PSname, 
			  &dvipsrc) == 0) {
	     /* search failed, we must put something into PSname. */
	     dvipsrc = kpse_var_expand(DVIPSRC);
	     if (dvipsrc) {
	       strcpy(PSname, dvipsrc);
	       free(dvipsrc);
	     }
	     /* Else SearchPath has filled PSname with something */
	   }
	 /* remove any redundant path separators. Many configurations
	    can show  up: c:\/.dvipsrc and so on ... */	       
	 { 
	   char *p, *q;
	   for (p = q = PSname; *p && (p - PSname < INLINE_SIZE) ; 
		p++, q++) {
	     if (IS_DIR_SEP(*p)) {
	       *q = DIR_SEP; p++; q++;
	       while (*p && IS_DIR_SEP(*p)) p++;
	     }
	     *q = *p;
	   }
	   *q = '\0';
	 }
#else
         strcpy(PSname, dvipsrc ? dvipsrc : DVIPSRC) ;
#endif /* WIN32 */
	 
#else /* ! KPATHSEA */
#ifndef VMCMS  /* IBM: VM/CMS - don't have home directory on VMCMS */
#ifndef MVSXA
         d = "~" ;
#endif
#endif  /* IBM: VM/CMS */
         strcpy(PSname, DVIPSRC) ;
#endif /* KPATHSEA */
      }
   } else {
#if defined(MSDOS) || defined(OS2)
      strcpy(PSname, printer) ;
      strcat(PSname, ".cfg") ;
#else
      strcpy(PSname, "config.") ;
      strcat(PSname, printer) ;
#endif
   }
#ifdef KPATHSEA
   if ((deffile=search(configpath,PSname,READ))!=NULL) {
#else
   if ((deffile=search(d,PSname,READ))!=NULL) {
#endif
#ifdef DEBUG
     if (dd (D_CONFIG)) {
       fprintf (stderr, "Reading dvips config file `%s':\n", realnameoffile);
     }
#endif
     c_lineno = 0;
     while (fgets(was_inline, INLINE_SIZE, deffile)!=NULL) {
       c_lineno++;
#ifdef DEBUG
       if (dd (D_CONFIG)) {
         fprintf (stderr, "%s:%d:%s", realnameoffile, c_lineno, was_inline);
       }
#endif
/*
 *   We need to get rid of the newline.
 */
       for (p=was_inline; *p; p++) ;
       if (p > was_inline) *(p-1) = 0 ;
       if (was_inline[0] != '@')
          canaddtopaper = 0 ;
       switch (was_inline[0]) {
/*
 *   Handling paper size information:
 *
 *      If line is empty, then we clear out the paper size information
 *      we have so far.
 *
 *      If it is `@+', then we add to the current paper size info.
 *
 *      If it is `name hsize vsize', then we start a new definition.
 */
case '@' :
         p = was_inline + 1 ;
         while (*p && *p <= ' ') p++ ;
         if (*p == 0) {
            papsizes = 0 ; /* throw away memory */
         } else if (*p == '+') {
            if (canaddtopaper == 0)
               error(
      " @+ in config files must immediately following a @ lines") ;
            else {
               *(nextstring-1) = '\n' ;/* IBM: VM/CMS - changed 10 to "\n" */
               p++ ;
               while (*p && *p == ' ') p++ ;
               strcpy(nextstring, p) ;
               nextstring += strlen(p) + 1 ;
            }
         } else {
            struct papsiz *ps ;
            
            ps = (struct papsiz *)mymalloc((integer)sizeof(struct papsiz)) ;
            ps->next = papsizes ;
            papsizes = ps ;
            ps->name = p ;
            while (*p && *p > ' ')
               p++ ;
            *p++ = 0 ;
            ps->name = newstring(ps->name) ;
            while (*p && *p <= ' ') p++ ;
            handlepapersize(p, &hsiz, &vsiz) ;
            ps->xsize = hsiz ;
            ps->ysize = vsiz ;
            ps->specdat = nextstring++ ;
            canaddtopaper = 1 ;
         }
         break ;
case 'a' :
         dopprescan = (was_inline[1] != '0') ;
         break ;
case 'b':
#ifdef SHORTINT
         if (sscanf(was_inline+1, "%ld", &pagecopies) != 1) 
	   bad_config("missing pagecopies to b") ;
#else
         if (sscanf(was_inline+1, "%d", &pagecopies) != 1) 
	   bad_config("missing pagecopies to b") ;
#endif
         if (pagecopies < 1 || pagecopies > 1000)
            bad_config("pagecopies not between 1 and 1000") ;
         break ;
case 'm' :
#ifdef SHORTINT
         if (sscanf(was_inline+1, "%ld", &swmem) != 1) 
	   bad_config("missing swmem to m") ;
#else   /* ~SHORTINT */
         if (sscanf(was_inline+1, "%d", &swmem) != 1)
	   bad_config("missing swmem to m") ;
#endif  /* ~SHORTINT */
         swmem += fontmem ; /* grab headers we've seen already */
         break ;
case 'M' :
         /* If the user specified a -mode, don't replace it.  */
         if (!mfmode_option)
	   mfmode = configstring(was_inline+1, 0) ;
         mflandmode = 0 ;
         {
            char *pp ;
            for (pp=mfmode; pp && *pp>' '; pp++) ;
            if (pp && *pp == ' ') {
               *pp++ = 0 ;
               while (*pp && *pp <= ' ')
                  pp++ ;
               if (*pp)
                  mflandmode = pp ;
            }
         }
         break ;
case 'o' :
	 if (!oname_option) {
           oname = configstring(was_inline+1, 1) ;
           if (*oname && oname[strlen(oname)-1] == ':')
              sendcontrolD = 1 ; /* if we send to a device, *we* are spooler */
	 }
         break ;
case 'O' :
         p = was_inline + 1 ;
         handlepapersize(p, &hoff, &voff) ;
         break ;
#ifdef FONTLIB
case 'L' : 
         {
            char tempname[INLINE_SIZE] ;
            extern char *fliparse() ;
            if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config("missing arg to L") ;
            else {
               flipath = getpath(fliparse(PSname,tempname), flipath);
               fliname = newstring(tempname) ;
            }
	 }
         break ;
#endif
case 'T' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1)
	   bad_config("missing arg to T") ;
         else
#ifdef KPATHSEA
	   SET_CLIENT_PATH (kpse_tfm_format, PSname);
#else
	   tfmpath = getpath(PSname, tfmpath) ;
#endif
         break ;
case 'P' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) bad_config("missing arg to P") ;
         else 
#ifdef KPATHSEA
	   SET_CLIENT_PATH (kpse_pk_format, PSname);
#else
	   pkpath = getpath(PSname, pkpath) ;
#endif
         break ;
case 'p' :
         p = was_inline + 1 ;
         while (*p && *p <= ' ')
            p++ ;
         if (*p == '+') {
            if (sscanf(p+1, "%s", PSname) != 1)
              bad_config("missing arg to p") ;
            getpsinfo(PSname) ;
         } else {
            psmapfile = configstring(was_inline+1, 0) ;
         }
         break ;
case 'v' : case 'V' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) 
	   bad_config("missing arg to V") ;
         else 
#ifdef KPATHSEA
	   SET_CLIENT_PATH (kpse_vf_format, PSname);
#else
	   vfpath = getpath(PSname, vfpath) ;
#endif
         break ;
case 'S' :
         if (sscanf(was_inline+1, "%s", PSname) != 1) 
	   bad_config("missing arg to S") ;
         else
#ifdef KPATHSEA
	   SET_CLIENT_PATH (kpse_pict_format, PSname);
#else
	   figpath = getpath(PSname, figpath) ;
#endif
         break ;
case 's':
         safetyenclose = 1 ;
         break ;
case 'H' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1) 
	   bad_config("missing arg to H") ;
         else 
#ifdef KPATHSEA
	   SET_CLIENT_PATH (headerpath, PSname);
#else
	   headerpath = getpath(PSname, headerpath) ;
#endif
         break ;
case '%': case ' ' : case '*' : case '#' : case ';' :
case '=' : case 0 : case '\n' :
         break ;
case 'r' :
         reverse = (was_inline[1] != '0') ;
         break ;
/*
 *   This case is for last resort font scaling; I hate this, but enough
 *   people have in no uncertain terms demanded it that I'll go ahead and
 *   add it.
 *
 *   This line must have numbers on it, resolutions, to search for the
 *   font as a last resort, and then the font will be scaled.  These
 *   resolutions should be in increasing order.
 *
 *   For most machines, just `300' is sufficient here; on the NeXT,
 *   `300 400' may be more appropriate.
 */
case 'R':
#ifndef KPATHSEA
         i = 0 ;
         p = was_inline + 1 ;
         while (*p) {
            while (*p && *p <= ' ')
               p++ ;
            if ('0' <= *p && *p <= '9') {
               j = 0 ;
               while ('0' <= *p && *p <= '9')
                  j = 10 * j + (*p++ - '0') ;
               if (i > 0)
                  if (lastresortsizes[i-1] > j) {
                     bad_config("last resort sizes (R) must be sorted") ;
                  }
               lastresortsizes[i++] = j ;
            } else {
               if (*p == 0)
                  break ;
               bad_config("only numbers expected on `R' line") ;
            }
         }
         lastresortsizes[i] = 32000 ;
#else /* KPATHSEA */
	 for (p = was_inline + 1; *p; p++) {
	   if (isblank (*p)) {
	     *p = ':';
	   }
	 }
         kpse_fallback_resolutions_string = xstrdup (was_inline + 1);
#endif
         break ;
case 'D' :
         if (sscanf(was_inline+1, "%d", &actualdpi) != 1)
           bad_config("missing arg to D") ;
         if (actualdpi < 10 || actualdpi > 10000)
           bad_config("dpi must be between 10 and 10000") ;
	 vactualdpi = actualdpi;
         break ;
/*
 *   Execute a command.  This can be dangerous, but can also be very useful.
 */
case 'E' :
#ifdef SECURE
         error("dvips was compiled with SECURE, which disables E in config") ;
#else
         if (secure) {
            error("dvips -R option used, which disables E in config") ;
            break ;
         }
         (void)system(was_inline+1) ;
#endif
         break ;
case 'K':
         removecomments = (was_inline[1] != '0') ;
         break ;
case 'U':
         nosmallchars = (was_inline[1] != '0') ;
         break ;
case 'W':
         for (p=was_inline+1; *p && *p <= ' '; p++) ;
         if (*p)
            warningmsg = newstring(p) ;
         else
            warningmsg = 0 ;
         break ;
case 'X' :
         if (sscanf(was_inline+1, "%d", &actualdpi) != 1)
           bad_config("missing numeric arg to X") ;
         if (actualdpi < 10 || actualdpi > 10000)
           bad_config("X arg must be between 10 and 10000") ;
         break ;
case 'Y' :
         if (sscanf(was_inline+1, "%d", &vactualdpi) != 1)
           bad_config("missing numeric arg to Y") ;
         if (vactualdpi < 10 || vactualdpi > 10000)
           bad_config("Y arg must be between 10 and 10000") ;
         break ;
case 'x': case 'y':
         if (sscanf(was_inline+1, "%lg", &mag) != 1) 
	   bad_config("missing arg to x or y") ;
         overridemag = (was_inline[0] == 'x') ? 1 : -1 ;
         break ;
case 'e' :
         if (sscanf(was_inline+1, "%d", &maxdrift) != 1)
           bad_config("missing arg to e") ;
         if (maxdrift < 0) bad_config("bad argument to e") ;
	 vmaxdrift = maxdrift;
         break ;
case 'q' : case 'Q' :
         quiet = (was_inline[1] != '0') ;
         break ;
case 'f' : case 'F' :
         filter = (was_inline[1] != '0') ;
	 if (filter)
	   oname = "";
	 /* noenv has already been tested, so no point in setting.  */
	 sendcontrolD = 0;
         break ;
#ifdef SHIFTLOWCHARS
case 'G':
         shiftlowchars = (was_inline[1] != '0') ;
         break ;
#endif
case 'h' : 
         if (sscanf(was_inline+1, "%s", PSname) != 1)
           bad_config("missing arg to h") ;
         else (void)add_header(PSname) ;
         break ;
case 'i' :
         if (sscanf(was_inline+1, "%d", &maxsecsize) != 1)
            maxsecsize = 0 ;
         sepfiles = 1 ;
         break ;
case 'I':
         noenv = (was_inline[1] != '0') ;
         break ;
case 'N' :
         disablecomments = (was_inline[1] != '0') ;
         break ;
case 'Z' :
         compressed = (was_inline[1] != '0') ;
         break ;
case 'j':
         partialdownload = (was_inline[1] != '0') ;
         break ;
case 't' :
         if (sscanf(was_inline+1, "%s", PSname) != 1)
           bad_config("missing arg to t") ;
         else {
           if (strcmp(PSname, "landscape") == 0) {
               if (hpapersize || vpapersize)
                  error(
            "both landscape and papersize specified; ignoring landscape") ;
               else
                  landscape = 1 ;
            } else
               paperfmt = newstring(PSname) ;
         }
         break ;
default:
         bad_config("strange line") ;
      }
     }
     (void)fclose(deffile) ;
   } else {
      if (printer)
        {
          char msg[1000];
          sprintf (msg, "warning: no config file for `%s'", printer);
          error(msg);
          return 0;
        }
   }
  
  return 1;
}

/*
*   If a character pointer is passed in, use that name; else, use the
*   default (possibly set) name, psfonts.map.
*/
void getpsinfo P1C(char *, name)
{
    FILE *deffile ;
    register char *p ;
    char *specinfo, *downloadinfo ;
   char downbuf[500] ;
   char specbuf[500] ;
   int slen ;

   if (name == 0)
      name = psmapfile ;
   if ((deffile=search(configpath, name, READ))!=NULL) {
      while (fgets(was_inline, INLINE_SIZE, deffile)!=NULL) {
         p = was_inline ;
         if (*p > ' ' && *p != '*' && *p != '#' && *p != ';' && *p != '%') {
            char *TeXname = NULL ;
            char *PSname = NULL ;
            char *Fontfile = NULL;
            char *Vectfile = NULL;
            char *hdr_name = NULL;
            boolean nopartial_p = false;
            boolean encoding_p = false;
            specinfo = NULL ;
            downloadinfo = NULL ;
            downbuf[0] = 0 ;
            specbuf[0] = 0 ;
            while (*p) {
               while (*p && *p <= ' ')
                  p++ ;
               if (*p) {
                  if (*p == '"') {             /* PostScript instructions? */
                     if (specinfo) {
                        strcat(specbuf, specinfo) ;
                        strcat(specbuf, " ") ;
                     }
                     specinfo = p + 1 ;

                  } else if (*p == '<') {    /* Header to download? */
                     /* If had previous downloadinfo, save it.  */
                     if (downloadinfo) {
                        strcat(downbuf, downloadinfo) ;
                        strcat(downbuf, " ") ;
                        downloadinfo = NULL;
                     }
                     if (p[1] == '<') {     /* << means always full download */
                       p++;
                       nopartial_p = true;
                     } else if (p[1] == '[') { /* <[ means an encoding */
                       p++;
                       encoding_p = true;
                     }
                     
                     /* skip whitespace after < */
                     while (p[1] == ' ' || p[1] == '\t')
                       p++;
                     
                     /* save start of header name */
                     hdr_name = p + 1 ;

                  } else if (TeXname) /* second regular word on line? */
                     PSname = p ;

                  else                /* first regular word? */
                     TeXname = p ;

                  if (*p == '"') {
                     p++ ;            /* find end of "..." word */
                     while (*p != '"' && *p)
                        p++ ;
                  } else
                     while (*p > ' ') /* find end of anything else */
                        p++ ;
                  if (*p)
                     *p++ = 0 ;

                  /* If we had a header we were downloading, figure
                     out what to do; couldn't do this above since we
                     want to check the suffix.  */
                  if (hdr_name) {
                     char *suffix = find_suffix (hdr_name);
                     if (encoding_p || STREQ (suffix, "enc")) {
                        /* (SPQR) if it is a reencoding, pass on to
                           FontPart, and download as usual */
                        Vectfile = downloadinfo = hdr_name;
		     } else if 	(!STREQ (suffix, "pfa")
				 && !STREQ (suffix, "pfb")
				 && !STREQ (suffix, "PFA")
				 && !STREQ (suffix, "PFB")) {
                       /* No special suffix. check if tagged <<*/
                       if (nopartial_p) /* its a font after all */
			 {  Fontfile = hdr_name;}
                       else 
                         {  downloadinfo = hdr_name; }
		     }
                     else {
                        /* its a font suffix. */
                        Fontfile = hdr_name;
                     }
                  }
               }
            }
            if (specinfo)
               strcat(specbuf, specinfo) ;
            if (downloadinfo)
               strcat(downbuf, downloadinfo) ;
            slen = strlen(downbuf) - 1;
            if (downbuf[slen] == ' ') {
              downbuf[slen] = 0;
            }
            if (TeXname) {
               TeXname = newstring(TeXname) ;
               PSname = newstring(PSname) ;
               Fontfile = newstring(Fontfile);
               Vectfile = newstring(Vectfile);
               specinfo = newstring(specbuf) ;
               downloadinfo = newstring(downbuf) ;
               add_entry(TeXname, PSname, Fontfile, Vectfile,
                         specinfo, downloadinfo) ;
            }
   	 }
      }
      (void)fclose(deffile) ;
   }
   checkstrings() ;
}
#ifndef KPATHSEA
/*
 *   Get environment variables! These override entries in ./config.h.
 *   We substitute everything of the form ::, ^: or :$ with default,
 *   so a user can easily build on to the existing paths.
 */
static char *getenvup P2C(char *, who, char *, what)
{
   return getpath(getenv(who), what) ;
}
#endif
#if !defined(KPATHSEA) && defined(SEARCH_SUBDIRECTORIES)
static char *concat3();
#endif
void checkenv P1C(int, which)
{
#ifndef KPATHSEA
   if (which) {
      tfmpath = getenvup(FONTPATH, tfmpath) ;
      vfpath = getenvup("VFFONTS", vfpath) ;
      pictpath = getenvup("TEXPICTS", pictpath) ;
      figpath = getenvup("TEXINPUTS", figpath) ;
      headerpath = getenvup("DVIPSHEADERS", headerpath) ;
      if (getenv("TEXPKS"))
         pkpath = getenvup("TEXPKS", pkpath) ;
      else if (getenv("TEXPACKED"))
         pkpath = getenvup("TEXPACKED", pkpath) ;
      else if (getenv("PKFONTS"))
         pkpath = getenvup("PKFONTS", pkpath) ;
#ifdef SEARCH_SUBDIRECTORIES
      else if (getenv(FONTPATH))
         pkpath = getenvup(FONTPATH, pkpath) ;
      if (getenv ("TEXFONTS_SUBDIR"))
         fontsubdirpath = getenvup ("TEXFONTS_SUBDIR", fontsubdirpath);
      {
         char pathsep[2] ;
         char *do_subdir_path();
         char *dirs = do_subdir_path (fontsubdirpath);
         /* If the paths were in dynamic storage before, that memory is
            wasted now.  */
         pathsep[0] = PATHSEP ;
         pathsep[1] = '\0' ;
         tfmpath = concat3 (tfmpath, pathsep, dirs);
         pkpath = concat3 (pkpath, pathsep, dirs);
      }
#endif
   } else
      configpath = getenvup("TEXCONFIG", configpath) ;
#endif
}

#if !defined(KPATHSEA) && defined(SEARCH_SUBDIRECTORIES)

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#ifdef SYSV
#include <dirent.h>
typedef struct dirent *directory_entry_type;
#else
#include <sys/dir.h>
typedef struct direct *directory_entry_type;
#endif

/* Declare the routine to get the current working directory.  */

#ifndef HAVE_GETCWD
extern char *getwd ();
#define getcwd(b, len)  ((b) ? getwd (b) : getwd (xmalloc (len)))
#else
#ifdef ANSI
extern char *getcwd (char *, int);
#else
extern char *getcwd ();
#endif /* not ANSI */
#endif /* not HAVE_GETWD */

#if defined(SYSV) || defined(VMS) || defined(MSDOS) || defined(OS2) || defined(ATARIST)
#define MAXPATHLEN (256)
#else   /* ~SYSV */
#include <sys/param.h>          /* for MAXPATHLEN */
#endif

extern void exit() ;
extern int chdir() ;

/* Memory operations: variants of malloc(3) and realloc(3) that just
   give up the ghost when they fail.  */

char *
xmalloc P1C(unsigned, size)
{
  char *mem = malloc (size);
  
  if (mem == NULL)
    {
      fprintf (stderr, "! Cannot allocate %u bytes.\n", size);
      exit (10);
    }
  
  return mem;
}


char *
xrealloc P2C(char *, ptr, unsigned, size)
{
  char *mem = realloc (ptr, size);
  
  if (mem == NULL)
    {
      fprintf (stderr, "! Cannot reallocate %u bytes at %x.\n", size, (int)ptr);
      exit (10);
    }
    
  return mem;
}


/* Return, in NAME, the next component of PATH, i.e., the characters up
   to the next PATHSEP.  */
   
static void
next_component P2C(char *, name, char **, path)
{
  unsigned count = 0;
  
  while (**path != 0 && **path != PATHSEP)
    {
      name[count++] = **path;
      (*path)++; /* Move further along, even between calls.  */
    }
  
  name[count] = 0;
  if (**path == PATHSEP)
    (*path)++; /* Move past the delimiter.  */
}


#ifndef _POSIX_SOURCE
#ifndef S_ISDIR
#define S_ISDIR(m) ((m & S_IFMT) == S_IFDIR)
#endif
#endif

/* Return true if FN is a directory or a symlink to a directory,
   false if not. */

int
is_dir P1C(char *, fn)
{
  struct stat stats;

  return stat (fn, &stats) == 0 && S_ISDIR (stats.st_mode);
}


static char *
concat3 P3C(char *, s1, char *, s2, char *, s3)
{
  char *r = xmalloc (strlen (s1) + strlen (s2) + strlen (s3) + 1);
  strcpy (r, s1);
  strcat (r, s2);
  strcat (r, s3);
  return r;
}


/* DIR_LIST is the default list of directories (colon-separated) to
   search.  We want to add all the subdirectories directly below each of
   the directories in the path.
     
   We return the list of directories found.  */

char *
do_subdir_path P1C(char *, dir_list)
{
  char *cwd;
  unsigned len;
  char *result = xmalloc ((unsigned)1);
  char *temp = dir_list;
  char dirsep[2] ;

  dirsep[0] = DIRSEP ;
  dirsep[1] = '\0' ;

  /* Make a copy in writable memory.  */
  dir_list = xmalloc (strlen (temp) + 1);
  strcpy (dir_list, temp);
  
  *result = 0;

  /* Unfortunately, we can't look in the environment for the current
     directory, because if we are running under a program (let's say
     Emacs), the PWD variable might have been set by Emacs' parent
     to the current directory at the time Emacs was invoked.  This
     is not necessarily the same directory the user expects to be
     in.  So, we must always call getcwd(3) or getwd(3), even though
     they are slow and prone to hang in networked installations.  */
  cwd = getcwd (NULL, MAXPATHLEN + 2);
  if (cwd == NULL)
    {
      perror ("getcwd");
      exit (errno);
    }

  do
    {
      DIR *dir;
      directory_entry_type e;
      char dirname[MAXPATHLEN];

      next_component (dirname, &dir_list);

      /* All the `::'s should be gone by now, but we may as well make
         sure `chdir' doesn't crash.  */
      if (*dirname == 0) continue;

      /* By changing directories, we save a bunch of string
         concatenations (and make the pathnames the kernel looks up
         shorter).  */
      if (chdir (dirname) != 0) continue;

      dir = opendir (".");
      if (dir == NULL) continue;

      while ((e = readdir (dir)) != NULL)
        {
          if (is_dir (e->d_name) && strcmp (e->d_name, ".") != 0
              && strcmp (e->d_name, "..") != 0)
            {
              char *found = concat3 (dirname, dirsep, e->d_name);

              result = xrealloc (result, strlen (result) + strlen (found) + 2);

              len = strlen (result);
              if (len > 0)
                {
                  result[len] = PATHSEP;
                  result[len + 1] = 0;
                }
              strcat (result, found);
              free (found);
            }
        }
      closedir (dir);

      /* Change back to the current directory, in case the path
         contains relative directory names.  */
      if (chdir (cwd) != 0)
        {
          perror (cwd);
          exit (errno);
        }
    }
  while (*dir_list != 0);
  
  return result;
}
#endif /* SEARCH_SUBDIRECTORIES */
