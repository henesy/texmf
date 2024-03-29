#include <stdio.h>
#include <kpathsea/c-std.h>
/* regfix -- change locals to be register declarations.  This
   produces code which runs 10% faster on some systems (e.g., Vax-11/750,
   Sequent Balance).  Don't try to use this program with other than TeX
   and Metafont in C.

   Tim Morgan   February 25, 1988.  */

#include "config.h"

#ifdef REGFIX		/* REST OF FILE (almost) */

#define BUFFER_SIZE 10240
char line[BUFFER_SIZE];



/* Replace the last (should be only) newline in S with a null.  */

void
remove_newline P1C(string, s)
{
  char *temp = strrchr (s, '\n');
  if (temp == NULL)
    {
      fprintf (stderr, "regfix: Lost newline somehow.\n");
      exit (EXIT_FAILURE);
    }

  *temp = 0;
}



#define	Puts(s)	fputs(s, stdout)

char *types[] = {"ASCIIcode ", "schar ", "eightbits ", "scaled ",
	"glueord ", "halfword ", "hyphpointer ", "internalfontnumber ",
	"nonnegativeinteger ", "poolpointer ", "quarterword ", "smallnumber ",
	"strnumber ", "triepointer ", "integer ", "short "};
#define NUMTYPES (sizeof(types)/sizeof(types[0]))
int lens[NUMTYPES];


char *
matchestype P1H(void)
{
  register int i;

  if (line[0] != ' ' || line[1] != ' ')
      return 0;

  for (i = 0; i < NUMTYPES; i++)
    {
      if (strncmp (&line[2], types[i], lens[i]) == 0
          && strchr (line, '[') == 0)
        return line + 2 + lens[i];
  }

  return 0;
}


int
main P1H(void)
{
    register int i;
#ifdef	vax
    register char *cp;
#endif

    for (i=0; i<NUMTYPES; i++)
	lens[i] = strlen(types[i]);

    /* Copy the declarations.  */
    while (fgets (line, BUFFER_SIZE, stdin)
           && strncmp (&line[10], "coerce", 6) != 0)
      {
        remove_newline (line);
	puts (line);
      }

    puts (line);

    while (fgets (line, BUFFER_SIZE, stdin))
      {
        remove_newline (line);

#ifdef	vax
	if (cp = matchestype() ) {
	    Puts("  register long ");
	    puts(cp);
#else
	if ( matchestype() ) {
	    Puts("  register");
	    puts(line+1);
#endif
	} else
	    puts(line);
    }

    fclose (stdout);
    uexit (0);
}

#else /* not REGFIX */

/* If we don't want to use register variables, we just copy stdin to
   stdout.  If writing or reading fail, exit with bad status.  */

int
main ()
{
  int c;

  while ((c = getchar ()) != EOF)
    {
      if (putchar (c) == EOF)
        {
          perror ("regfix");
          exit (EXIT_FAILURE);
        }
    }

  if (!feof (stdin))
    {
      perror ("regfix");
      exit (EXIT_FAILURE);
    }

  return EXIT_SUCCESS;
}

#endif /* not REGFIX */
