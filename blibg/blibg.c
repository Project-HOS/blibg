/* ------------------------------------------------------------------------ */
/*  Bogus LIBrary Generator                                                 */
/*           for H8/300H C COMPILER(Evaluation software) Ver. 1.0           */
/*               H8S,H8/300 SERIES C Compiler Ver. 2.0D Evaluation software */
/*               SH SERIES C/C++ Compiler Ver. 5.0 Evaluation software      */
/*                                                                          */
/*                                Copyright (C) 2002,2003 by Project HOS    */
/* ------------------------------------------------------------------------ */
#include <stdio.h>
#include <string.h>
#include <ctype.h>

//#define DEBUG

#define MAXEXPORT 200	/* max export symbol number per 1 obect file	*/
#define MAXOBJS   256	/* max object file number			*/

/* fake time stamp string */
const char *pseudo_stamp = "020916113234";
#define TSTAMPLEN  (strlen(pseudo_stamp))

/* Tool ID string "not official B-)" */
const char *tool_name = "BLIBG-01";
#define TOOLNAMELEN  (strlen(tool_name))

/* struct for Object data */
typedef struct objcell {
  char *fname;			/* filename */
  char *pname;			/* program name (normally = filename - ext) */
  int  exp_num;			/* number of export symbols */
  char *exp_str[MAXEXPORT];	/* pointer of export symbols */
  int  fsize;			/* file size (before padding) */
  int  offset;			/* location offset in library file */ 
} OBJCELL;

OBJCELL  Obj_table[MAXOBJS];
int      O_ed = 0;

/* list of objects (for sorting) */
int      Prog_list[MAXOBJS];

/* struct for EXPORT symbol */
typedef struct expcell {
  char *exp_str;		/* symbol string */
  int  prog_num;		/* # of program that this symbol belongs to */
} EXPCELL;

EXPCELL Exp_table[MAXOBJS*MAXEXPORT];
int     E_ed = 0;

/* list of export symbols (for sorting) */
int     Exp_list[MAXOBJS*MAXEXPORT];

/* strings(filename,program name,symbol) buffer size */
#define SBUFFSIZE 32*1024

/* strings(filename,program name,symbol) buffer */
char Sbuf[SBUFFSIZE];
int  S_ed = 0;

#ifdef DEBUG
#define  debc(c) putchar((c))

void print_Objtable( void)
{
  int i,k;

  putchar('\n');
  for ( i=0; i<O_ed; i++) {
    printf( "OBJ #%d\n\tfname = %s\n\tpname = %s\n"
	    "\texp_num = %d\n\tfsize = %d\n",
	    i, Obj_table[i].fname, Obj_table[i].pname,
	    Obj_table[i].exp_num, Obj_table[i].fsize );
    printf( "\tEXPORT ->\n");
    for ( k=0; k<Obj_table[i].exp_num; k++)
      printf( "\t\t%s\n", Obj_table[i].exp_str[k]);

    putchar( '\n');
  }

  return;
}
#else
#define  debc(c)
#define print_Objtable()
#endif

#define OBJ_OK         0
#define OBJ_OPEN_ERR   1
#define OBJ_FSTAT_ERR  2
#define OBJ_UNS_TYPE   3
#define OBJ_STR_OVR    4
#define OBJ_EXP_OVR    5

/* sort PROGRAM. key = program name, dictionary order */
void prog_sort( void) /* (T_T) poor */
{
  int i,j,k;

  /* init list */
  for ( i=0; i<O_ed; i++) Prog_list[i] = i;

  for ( i=0; i<O_ed-1; i++) {
    for ( j=i+1; j<O_ed; j++) {
      if ( strcmp( Obj_table[Prog_list[i]].pname, 
		   Obj_table[Prog_list[j]].pname ) > 0 ) {
	           k = Prog_list[i];
	Prog_list[i] = Prog_list[j];
	Prog_list[j] = k;
      }
    }
  }

#ifdef DEBUG
  printf("\n == PROGRAM TABLE ==\n\n");
  for ( i=0; i<O_ed; i++)
    printf("\tPROGRAM #%d\t%s\n", i, Obj_table[Prog_list[i]].pname);
  printf("\n ==       END     ==\n");
#endif
}


/* sort EXPORT. key = symbol string, dictionary order */
void export_sort( void) /* (T_T) poor */
{
  int i,j,k,l;

  /* init table */
  for ( i=j=0; i<O_ed; i++) {
    /* init list */
    for ( l=0; l<O_ed; l++)
	if ( Prog_list[l] == i) break;

    /* init Exp_table. set symbol string and program # from object data */
    for ( k=0; k<Obj_table[i].exp_num; j++,k++) {
      Exp_table[j].exp_str  = Obj_table[i].exp_str[k];
      Exp_table[j].prog_num = l;
      Exp_list[j] = j;
    }
  }

  E_ed = j;

  for ( i=0; i<E_ed-1; i++) {
    for ( j=i+1; j<E_ed; j++) {
      if ( strcmp( Exp_table[Exp_list[i]].exp_str,
		   Exp_table[Exp_list[j]].exp_str) > 0 ) {
	          k = Exp_list[i];
	Exp_list[i] = Exp_list[j];
	Exp_list[j] = k;
      }
    }
  }

#ifdef DEBUG
  printf("\n == EXPORT TABLE ==\n\n");
  for ( i=0; i<E_ed; i++)
    printf("\t PROGRAM #%d \t<- %s\n", 
	   Exp_table[Exp_list[i]].prog_num,
	   Exp_table[Exp_list[i]].exp_str );
  printf("\n ==      END     ==\n");
#endif
}

/* ! never change ! */
#define BLOCKSIZE 256

/* read object file. then make object table and return its size */ 
int make_objtbl( int num, char *fname)
{
  int i,j,ofsize;
  unsigned char rbuf[BLOCKSIZE];
  FILE *fp;

  /* open check */ 
  if ( ( fp = fopen( fname, "rb")) == NULL )
    return OBJ_OPEN_ERR;

  Obj_table[num].fname = fname;

  Obj_table[num].exp_num = 0;
  ofsize = 0;

  /* 1st BYTE = type, 2nd BYTE = BLOCK size */ 
  while ( fread( rbuf, 1, 2, fp) == 2 ) {
    int sum,type,size;

    sum = rbuf[0]+rbuf[1]; type = rbuf[0]; size = rbuf[1]-2;

    /* check read blocksize */
    if ( fread( rbuf, 1, size, fp) != size ) {
      /* trunacated or unsuported type object */
      fclose( fp);
      return OBJ_UNS_TYPE;
    }
    ofsize += size +2 ;

    /* check sum */
    for ( i=0; i<size; i++) sum += rbuf[i];
    if (( sum&0xff ) != 0xff ) {
      /* crashed or unsuported type object */
      fclose( fp);
      return OBJ_UNS_TYPE;
    }

    switch ( type) {
    case 0x83: /* timestamp/PROGRAM/environment block */
    case 0x84: /* timestamp/PROGRAM/environment block */
      /* offset to program name. rule is unknown. */
      switch ( rbuf[0]) {
      case 0xc0:
	i = 0x1c; break;
      case 0xb0:
	i = 0x1c; break;
      case 0xa0:
	i = 0x20; break;
      case 0x20:
	i = 0x24; break;
      default:
	/* program name (suposed) is't printable char. 
	   it means this is unknown(unseen) object type */  
	fclose( fp);
	return OBJ_UNS_TYPE;
      }

      /* check program name */
      for ( j=0; j<rbuf[i]; j++)
	if (!isprint(rbuf[i+1+j])) break;

      if ( j != rbuf[i]) {
	/* program name (suposed) is't printable char. 
	   it means this is unknown(unseen) object type */  
	fclose( fp);
	return OBJ_UNS_TYPE;
      }

      /* check buffer over flow */
      if ( S_ed + rbuf[i] + 1 > SBUFFSIZE) {
	fclose( fp);
	return OBJ_STR_OVR;
      }

      /* store program name */
      strncpy( &Sbuf[S_ed], &rbuf[i+1], rbuf[i]);
      Obj_table[num].pname = &Sbuf[S_ed];
      S_ed += rbuf[i]+1;

      break;

    case 0x14: /* EXPORT symbol block */
    case 0x94: /* EXPORT symbol block */
      for ( i=0; i<size-1; ) {
	/* i found 3 types */
	switch ( rbuf[i+2]) {
	case 0x00:
	case 0x20:
	  i += 7;
	  break;

	case 0x40:
	  i += 8;
	  break;

	default:
	  fclose( fp);
	  return OBJ_UNS_TYPE;
	}

	/* check buffer overflow */
	if ( S_ed + rbuf[i] + 1 >= SBUFFSIZE ) {
	  fclose( fp);
	  return OBJ_STR_OVR;
	}

	/* check export symbol number per 1 object file */
	if ( Obj_table[num].exp_num == MAXEXPORT - 1 ) {
	  fclose( fp);
	  return OBJ_EXP_OVR;
	}

	/* store export symbol string */
	Obj_table[num].exp_str[Obj_table[num].exp_num++] =
	  strncpy( &Sbuf[S_ed], &rbuf[i+1], rbuf[i]);
	S_ed += rbuf[i] + 1;

	i += rbuf[i] + 1;
      }
    }
  }
  fclose( fp);
  Obj_table[num].fsize = ofsize;

  return Obj_table[num].exp_num  ? OBJ_OK: OBJ_UNS_TYPE;
} 

/* joint object files with zero padding */
int joint_objfile( FILE *fpw)
{
  int  i,j,sz,offset;
  char jbuf[BLOCKSIZE];
  FILE *fpr;

  for ( i=offset=0; i<O_ed; i++) {
    fpr = fopen( Obj_table[Prog_list[i]].fname, "rb");
    Obj_table[Prog_list[i]].offset = offset;

    do {
      sz = fread( jbuf, 1, BLOCKSIZE, fpr);
      fwrite( jbuf, 1, sz, fpw);
      offset++;
    } while ( sz == BLOCKSIZE );

    if ( sz ) {
      /* zero padding */
      for ( j=0; j<BLOCKSIZE-sz; j++) jbuf[j] = 0;
      fwrite( jbuf, 1, 256-sz, fpw);
    }

    fclose( fpr);
  }

  return 0;
}

/* writes EXPORT TABLE temporally file, and return its size */
int write_exptmp( FILE *fpw)
{
  int  i,j,k,l,s,expblk_size;
  char ebbuf[BLOCKSIZE];

  for ( i=expblk_size=0,j=2; i<E_ed; i++) {
    /* 1 block size  < 256 byte */
    if ( j + 2 + 1 + ( l = strlen( Exp_table[Exp_list[i]].exp_str))
	 > BLOCKSIZE-2 ) {
      if ( i == 0) return 0;		/* excess */

      /* ebbuf[0] = type(0x64: normal block), ebbuf[1] = size */
      ebbuf[0] = 0x64; ebbuf[1] = j+1;

      /* last 1 BYTE is checksum */
      for ( k=s=0; k<j; k++) s += ebbuf[k];
      ebbuf[j] = 0xff - ( s & 0xff);

      fwrite( ebbuf, 1, j+1, fpw);
      expblk_size += j+1;
      j = 2;
    }

    /* # of program that this export symbol belongs to.
       BIG ENDIAN WORD */
    ebbuf[j++] = Exp_table[Exp_list[i]].prog_num >>   8;
    ebbuf[j++] = Exp_table[Exp_list[i]].prog_num & 0xff;

    /* BYTE(length),"symbol string" */
    ebbuf[j++] = l;
    strcpy( &ebbuf[j], Exp_table[Exp_list[i]].exp_str);
    j += l;
  }
  
  /* ebbuf[0] = type(0xe4: last block), ebbuf[1] = size */
  ebbuf[0] = 0xe4; ebbuf[1] = j+1;

  /* last 1 BYTE is checksum */
  for ( k=s=0; k<j; k++) s += ebbuf[k];
  ebbuf[j] = 255 - ( s & 0xff);

  fwrite( ebbuf, 1, j+1, fpw);
  expblk_size += j+1;

  /* zero padding */
  if ( expblk_size & 0xff ) {
    k = BLOCKSIZE - ( expblk_size & 0xff);
    for ( i=0; i<k; i++) ebbuf[i] = 0 ;
    fwrite( ebbuf, 1, k, fpw);
    expblk_size += k;
  }

  /* returns block size */
  return expblk_size;
}

/* write head block (0x00-0xff) */
int write_head( int progblk_size, FILE *fpw)
{
  int  j,k;
  char hbbuf[BLOCKSIZE];

  /* 1st BYTE : library ID ?
     2nd BYTE : block length(before zero padding)
     3rd BYTE : ?? maybe zero */
  hbbuf[0] = 0xe0; hbbuf[1] = 35+TOOLNAMELEN; hbbuf[2] = 0x00;

  /* 2 time stamps */
  strcpy( &hbbuf[ 3], pseudo_stamp);
  strcpy( &hbbuf[15], pseudo_stamp);

  /* number of object files, BIG ENDIAN WORD */ 
  hbbuf[27] = O_ed >>   8;
  hbbuf[28] = O_ed & 0xff;

  /* number of export symbol strings, BIG ENDIAN WORD */ 
  hbbuf[29] = E_ed >>   8;
  hbbuf[30] = E_ed & 0xff;

  /* offset to object file block / 0x100, BIG ENDIAN WORD */ 
  j = (0x100 + progblk_size) >> 8;
  hbbuf[31] = j >>   8;
  hbbuf[32] = j & 0xff;

  /* librarian tool ID string */
  hbbuf[33] = TOOLNAMELEN;
  strcpy( &hbbuf[34], tool_name);

  /* last BYTE is checksum */
  for ( j=k=0; j<34+TOOLNAMELEN; j++) k += hbbuf[j];
  hbbuf[34+TOOLNAMELEN] = 255 - ( k & 0xff);

  /* zero padding */
  for ( j=35+TOOLNAMELEN; j<BLOCKSIZE; hbbuf[j++] = 0);

  fwrite( hbbuf, 1, BLOCKSIZE, fpw);

  return 0;
}

/* writes PROGRAM TABLE temporally file and returns its size */
int write_progtmp( int expblk_size, int progblk_size, FILE *fpw)
{
  int  i,j,k,l,s,new_size;
  char pbbuf[BLOCKSIZE];

  /* 0x2-0xd is time stamp string */
  strcpy( &pbbuf[2], pseudo_stamp);

  for ( i=new_size=0,j=2+TSTAMPLEN; i<O_ed; i++) {
    /* block size < 256 byte */
    if ( j + 2 + 2 + 1 + ( l = strlen(Obj_table[Prog_list[i]].pname))
         + TSTAMPLEN > BLOCKSIZE-2 ) {
      if ( i == 0) return 0;		/* excess 256 bytes */

      /* 1st BYTE: type (0x62: noraml block)
	 2nd BYTE: size */
      pbbuf[0] = 0x62; pbbuf[1] = j+1 - TSTAMPLEN;

      /* last BYTE is checksum */
      for ( k=s=0; k<j-TSTAMPLEN; k++) s += pbbuf[k];
      pbbuf[k] = 0xff - ( s & 0xff);

      /* determination of offset to object requires  size of 
         PROGRAM TABLE's own.
	 so, 1st call is dummy. no write. */
      if ( progblk_size != 0)
	fwrite( pbbuf, 1, j+1-TSTAMPLEN, fpw);

      new_size += j+1-TSTAMPLEN;
      j = 2 + TSTAMPLEN;
    }

    /* (offset of object)/0x100. BIGENDIAN WORD */ 
    s = Obj_table[Prog_list[i]].offset 
      + 1 + ((progblk_size + expblk_size)>>8);
    /* 1 = sizeof(HEADBLOCK)>>8 */

    pbbuf[j++] = s  >> 8 ;
    pbbuf[j++] = s & 0xff;

    /* (object size (zero padded))/0x100. BIGENDIAN WORD */
    s = ( Obj_table[Prog_list[i]].fsize >> 8 ) 
      + ( Obj_table[Prog_list[i]].fsize & 0xff ? 1: 0 );
    pbbuf[j++] = s  >> 8 ;
    pbbuf[j++] = s & 0xff;

    /* [program name length],"programname",time stamp string */
    pbbuf[j++] = l;
    strcpy( &pbbuf[j], Obj_table[Prog_list[i]].pname);
    j += l;
    strcpy( &pbbuf[j], pseudo_stamp);
    j += TSTAMPLEN;

  }
  
  /* 1st BYTE: type (0xe2: last block)
     2nd BYTE: size */
  pbbuf[0] = 0xe2; pbbuf[1] = j+1-TSTAMPLEN;

  /* last BYTE is checksum */
  for ( k=s=0; k<j-TSTAMPLEN; k++) s += pbbuf[k];
  pbbuf[k] = 0xff - ( s & 0xff);

  /* determination of offset to object requires size of PROGRAM TABLE's own.
     so, 1st call is dummy. no write. */
  if ( progblk_size != 0)
    fwrite( pbbuf, 1, j+1-TSTAMPLEN, fpw);
  new_size += j+1-TSTAMPLEN;

  if ( new_size & 0xff ) {
    k = BLOCKSIZE - ( new_size & 0xff);
    /* zero padding */
    if ( progblk_size != 0) {
      /* determination of offset to object requires size of 
         PROGRAM TABLE's own. 
	 so, 1st call is dummy. no write. */
      for ( i=0; i<k; i++) pbbuf[i] = 0;
      fwrite( pbbuf, 1, k, fpw);
    }
    new_size += k;
  }

  return new_size;
}

void cat_file( FILE *fp, FILE *add)
{
  int i;

  fseek( fp,  0, SEEK_END);
  fseek( add, 0, SEEK_SET);

  do {
    i = fread( Sbuf, 1, SBUFFSIZE, add);
    fwrite( Sbuf, 1, i, fp);
  } while ( i == SBUFFSIZE);
}

void usage( void)
{
    fputs( 
	  "usage:\n\tblibg libfile objfile1 [ objfile2 ... ]\n"
	  "\tblibg -sub=subcmdfile\n"
	  , stderr);
}

/* read subcommand file. 
   get input objet file list and output library filename */
int read_subcmd( char *fname, char *infiles[])
{
  int i,j,k;
  FILE *fp;
  char sbuf[256],str1[32],str2[32],dmy[2];

  if (( fp = fopen( fname, "r")) == NULL ) {
    fprintf( stderr, "can't open %s\n", fname);
    return 0;
  }

  /* output filename is unset. */
  infiles[1] = NULL;

  for ( i=2; fgets( sbuf, 256, fp) != NULL; ) {
    /* drop except 2 word line */ 
    if ( sscanf( sbuf, "%31s%31s%1s", str1, str2, dmy) != 2) continue;

    /* comannds are case insensitive */
    for ( j=0; str1[j]!='\0'; j++) str1[j] = tolower(str1[j]);

    k = strlen( str2);
    if ( strcmp( str1, "input") == 0 ) {
      if ( S_ed + k +1 > SBUFFSIZE ) {
	fputs( "string buffer overflow.\n", stderr);
	return 0;
      }
      /* store input object filename */
      strcpy( &Sbuf[S_ed], str2);
      infiles[i++] = &Sbuf[S_ed];
      S_ed += k + 1;
      continue;
    }

    if ( strcmp( str1, "output") == 0 ) {
      if ( S_ed + k +1 > SBUFFSIZE ) {
	fputs( "string buffer overflow.\n", stderr);
	return 0;
      }
      /* store output library filename */
      strcpy( &Sbuf[S_ed], str2);
      infiles[1] = &Sbuf[S_ed];
      S_ed += k + 1;
    }

  }

  /* returns number of object files + 2 */
  return i;
}


#define OPTSUBCMD "-sub="

int main( int agc, char *agv[])
{
  int i,j;
  char *infiles[MAXOBJS+2];
  FILE *fpl,*fpp,*fpe,*fpo;

  fputs( "Bogus LIBrary Generator Ver. 0.2a\n"
	 "\tfor\tH8/300H C COMPILER(Evaluation software) Ver.1.0\n"
	 "\t\tSH SERIES C/C++ Compiler Ver. 5.0 Evaluation software\n\n"
	 "WARNING!! There is NO WARRANTIES on this software.\n"
	 "Please use at YOUR OWN RISK.\n\n", stderr);

  if ( agc == 2  ) {
    /* allows -SUB=,-sub=,-Sub=, ... */ 
    strncpy( Sbuf, agv[1], SBUFFSIZE-1);
    for ( i=0; Sbuf[i] != '\0' && i<sizeof(OPTSUBCMD); i++)
      Sbuf[i] = tolower(Sbuf[i]);

    if ( strncmp( Sbuf, OPTSUBCMD, sizeof(OPTSUBCMD)-1) == 0) {
      /* setup agc and agv as "blibg libraryname object1 object2 ... */

      agc = read_subcmd( agv[1] + sizeof(OPTSUBCMD) - 1, infiles);
      if ( infiles[1] == NULL ) {
	fprintf( stderr,"no 'output' in %s.\n", agv[1]+sizeof(OPTSUBCMD)-1);
	return 1;
      }
      if ( agc == 2 ) {
	fprintf( stderr,"no 'input' in %s.\n", agv[1]+sizeof(OPTSUBCMD)-1);
	return 1;
      }
      agv = infiles;

    } else {
      usage();
      return 1;
    }
  }

  if ( agc < 3 ) {
    usage();
    return 1;
  }

  if ( agc > MAXOBJS + 2) {
    fputs( "too many object files\n", stderr);
    return 1;
  }

  for ( i=0; i<agc-2; i++) {
#ifdef DEBUG
    fprintf( stdout, "\tarchive -> %s\n", agv[i+2]);
#endif
    switch ( make_objtbl( i, agv[i+2])) {
    case OBJ_OPEN_ERR:
      fprintf( stderr, "\ncan't open %s.\n", agv[i+2]);
      return 1;

    case OBJ_UNS_TYPE:
      fprintf( stderr, "\n%s is unsupported file type.\n", agv[i+2]);
      return 1;

    case OBJ_STR_OVR:
      fprintf( stderr, "\nstrings buffer overflow in %s.\n", agv[i+2]);
      return 1;

    case OBJ_EXP_OVR:
      fprintf( stderr, "\ntoo much exports in %s.\n", agv[i+2]);
      return 1;
    }
  }

  O_ed = i;

  print_Objtable();

  /* sort programs and export symbols */
  prog_sort(); export_sort();

  if (( fpo = tmpfile()) == NULL) {
      fputs( "can't create temporaly file.\n", stderr);
      return 1;
  }
  joint_objfile( fpo);

  if (( fpe = tmpfile()) == NULL) {
      fputs( "can't create temporaly file.\n", stderr);
      return 1;
  }
  i = write_exptmp( fpe);

  if (( fpp = tmpfile()) == NULL) {
      fputs( "can't create temporaly file.\n", stderr);
      return 1;
  }
  /* determin of PROGRAM TABLE requires its own size.
     so, 1st call is dummy. no write. get its size only. */
  j = write_progtmp( i, 0, fpp);
  write_progtmp( i, j, fpp);

  if (( fpl = fopen( agv[1], "wb")) == NULL ) {
    fprintf( stderr, "can't create %s\n", agv[1]);
    return 1;
  }
  /* HEAD BLOCK requires PROGRAM TABLE size,too */
  write_head( j, fpl);

  fprintf( stdout, "\n\toutput library -> %s\n", agv[1]);
  cat_file( fpl, fpp);
  cat_file( fpl, fpe);
  cat_file( fpl, fpo);

  fclose( fpl);
  fclose( fpp);
  fclose( fpe);
  fclose( fpo);

  return 0;
}

/* ------------------------------------------------------------------------- */
/*  Copyright (C) 2002,2003 by Project HOS                                   */
/* ------------------------------------------------------------------------- */
