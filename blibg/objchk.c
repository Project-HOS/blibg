/* ------------------------------------------------------------------------ */
/*  OBJect CHecKer   version 0.1a (2002/09/20)                              */
/*           for     H8/300H C COMPILER(Evaluation software)                */
/*                   SH SERIES C/C++ Compiler Ver. 5.0 Evaluation software  */
/*                                                                          */
/*                                     Copyright (C) 2002 by Project HOS    */
/* ------------------------------------------------------------------------ */
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

/* never change */
#define BLOCKSIZE 256

int strchk( char *str, int len)
{
  int i;

  for ( i=0; i<len; i++)
    if (!isprint( str[i])) break;

  return  i == len ? len: 0;
}

int main( int agc, char *agv[])
{
  int		i,j,k,l,s;
  char		tmp[17];
  unsigned char buf[BLOCKSIZE];
  FILE		*fp;
  struct stat	sb;

  fputs( "OBJect CHecKer Ver. 0.1a\n", stdout);
  fputs( "\tfor\tH8/300H C COMPILER(Evaluation software) Ver.1.0\n"
	 "\t\tSH SERIES C/C++ Compiler Ver. 5.0 Evaluation software\n\n"
	 "WARNING!! There is NO WARRANTIES on this software.\n"
	 "Please use at YOUR OWN RISK.\n\n", stderr);

  if ( agc != 2 ) {
    fputs( "usage:\nobjchk filename\n", stderr);
    return 1;
  }

  if ( ( fp = fopen( agv[1], "rb")) == NULL) {
    fprintf( stderr, "can't open %s \n", agv[1]);
    return 1;
  }

  fprintf( stdout, "\nOBJ file -> %s\n", agv[1]);

#ifdef __BORLANDC__
  fstat( fp->fd, &sb);		/* Borland C */
#else
  fstat( fp->_file, &sb);	/* FreeBSD */
#endif

  fprintf( stdout, "filesize  = %d bytes\n", (int)sb.st_size);

  /* read 2byte: 1st = ID, 2nd = length */
  for ( l=0; fread( buf, 1, 2, fp) == 2 ; l++) {
    fprintf( stdout, 
	     "\n\nBLOCK #%d\n"
	     "\tID = 0x%02x   blocksize = 0x%02x(%d)\n",
	     l, buf[0], buf[1], buf[1]);

    if ( (buf[1]-2) != fread( &buf[2], 1, buf[1]-2, fp)) {
      fprintf( stdout,
	       "\n\nHmm... %s maybe truncated or crashed."
	       " Otherwise, unknown type.\n", agv[1]);
      return 1;
    }
  
    for ( i=s=0; i<buf[1]; i++) s += buf[i];

    fprintf( stdout, "\tcheck sum ... %s \n\n", 
	     (s & 0xff) == 0xff ? "OK": "NG" );

    for ( i=0; i<buf[1]; i++) {
      tmp[16] = 0;
      if ((i%16) == 0) fprintf( stdout, "%04x ", i);
      fprintf( stdout, "%02x ", buf[i]);
      tmp[i%16] = isprint(buf[i]) ? buf[i]: '.';
      if ((i%16) == 15) fprintf( stdout, " |%16s\n", tmp);
    }
    if ( i%16 ) {
      tmp[i%16] = 0;
      for ( j=0; j< 16-(i%16); j++) fputs( "   ", stdout);
      fprintf( stdout, " |%s\n", tmp);
    }

    switch (buf[0]) {
      /* timestamp,PROGRAM,(ENVIRONMENT?|ARCHITECTURE?) block */
    case 0x83:
    case 0x84:
      fprintf( stdout, "\n\tTimestamp1       = %12s\n", &buf[3]);
      /* timestamp1 string check */
      if (!strchk( &buf[3], 12))
	fputs("\t\tHmm... timestamp includes unprintable char.\n"
	      "\t\tUnknown type.\n", stdout);


      fputs(           "\tPROGRAM name     = ", stdout);
      switch ( buf[2] ) {
      case 0xc0:
	i = 0x1e; break;
      case 0xb0:
	i = 0x1e; break;
      case 0xa0:
	i = 0x22; break;
      case 0x20:
	i = 0x26; break;
      default:
	i = -1;
      }

      if ( i>0 ) {
	for ( j=buf[i++],k=0; k<j; k++) fputc( buf[i++], stdout);
	fputc( '\n', stdout);
	/* PROGRAM name check */
	if (!strchk( &buf[i+1], buf[i]))
	  fputs("\t\tHmm... PROGRAM name includes unprintable char.\n"
		"\t\tUnknown type.\n", stdout);

	fputs(           "\tENV? | ARC?      = ", stdout);
	for ( j=buf[i],k=0; k<j; k++) fputc( buf[i+k+1], stdout);
	fputc( '\n', stdout);
	/* ( ENV? | ARC?) string check */
	if (!strchk( &buf[i+1], buf[i]))
	  fputs("\t\tHmm... ENV? | ARC? string includes unprintable char.\n"
		"\t\tUnknown type.\n", stdout);
      } else {
	fputs("\t\tHmm... Unknown type.\n", stdout);
      }
      break;

      /* source filename,toolname block */
    case 0x86:
      i = 9;
      fputs(         "\n\tSource filename  = ", stdout);
      for ( j=buf[i],k=0; k<j; k++) fputc( buf[i+1+k], stdout);
      fputc( '\n', stdout);
      /* source filename check */
      if (!strchk( &buf[i+1], buf[i]))
	fputs("\t\tHmm... source filename includes unprintable char.\n"
	      "\t\tUnknown type.\n", stdout);

      i += k + 1;
      fputs(         "\tToolname         = ", stdout);
      for ( j=buf[i],k=0; k<j; k++) fputc( buf[i+1+k], stdout);
      fputc( '\n', stdout);
      /* tool name check */
      if (!strchk( &buf[i+1], buf[i]))
	fputs("\t\tHmm... tool name includes unprintable char.\n"
	      "\t\tUnknown type.\n", stdout);

      i += k ;
      fputs(         "\tTimestamp2       = ", stdout);
      for ( j=12,k=0; k<j; k++) fputc( buf[i+1+k], stdout);
      fputc( '\n', stdout);
      /* tool name check */
      if (!strchk( &buf[i+1], 12))
	fputs("\t\tHmm... timestamp2 includes unprintable char.\n"
	      "\t\tUnknown type.\n", stdout);


	break;

      /* EXPORT symbol block */
    case 0x94:
    case 0x14:
      for ( i=2,s=0; i<buf[1]-1; ) {
	switch ( buf[i]) {
	case 0x00:
	case 0x20:
	  i += 7;
	  break;
	case 0x40:
	  i += 8;
	  break;
	}

	fprintf( stdout, "\tEXPORT #%d\t", s++);
	for ( j=buf[i],k=0; k<j; k++) fputc( buf[i+k+1], stdout);
	fputc( '\n', stdout);
	/* EXPORT symbol string check */
	if (!strchk( &buf[i+1], buf[i]))
	  fputs("\t\tHmm... export symbol string includes unprintable char.\n"
	      "\t\tUnknown type.\n", stdout);

	i += k+1;
      }
      break;

      }

  }

  fclose( fp);

  return 0;
}

/* ------------------------------------------------------------------------ */
/*  Copyright (C) 2002 by Project HOS                                       */
/* ------------------------------------------------------------------------ */
