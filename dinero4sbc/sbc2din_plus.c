/* 	sbc2din_plus.c
*
*	decompresses SBC.I and SBC.D files,
*	and creates dinero+ binary and ASCII files,
*	and ASCII stream table
*
*	Revision 1.1 Initial
*
*	Revision 1.2 optional ascii files, plus separate instruction and data
*	1.3 pipes
*	1.4 don't generate combined binary dinero
*	1.5 only combined dinero again
*	Authors: Milena & Aleksandar MIlenkovic, March 2003
*
*	Note: This software is released under GNU GPL.
*
*
************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#define MAX_SIZE 30000
#define MAX_FIFO 20000

typedef unsigned int long long address_t;
typedef unsigned int instr_t;
typedef int long long offset_t;
typedef char unsigned byte1_t;
typedef short unsigned byte2_t;
typedef short unsigned stream_t;


typedef struct node_ref
{
  offset_t current_addr;
  offset_t data_stride;
  int long long rep_count;
  struct node_ref *next;
} node_ref;

typedef struct node_word
{ instr_t instr_word;
  char unsigned instr_type;
  struct node_word *next;
} node_word;


 char 	*bname; 	 /*benchmark name, e.g., mcf_lgred*/
 char 	sys_command[80]; /* sys command buffer */
 char 	fname[64];	 /* file name buffer */

 int 	i,j;
 int  	FileNumber1, FileNumber2;	/* start, stop file numbers */
 FILE   *fsbci,				/* sbc instruction file */
 	*ftable_b, 			/* binary sbc table */
	*fsbcd; 			/* sbc data file */
 FILE 	*fdin_a,			/* dinero+ ascii file */
 	*fdin_b,			/* dinero+ binary file */
	*ftable_a;			/* sbc ascii table */

 byte2_t 	MaxStreamIndex;
 stream_t  	StreamIndex, StreamLen[MAX_SIZE], found;
 int unsigned	MaxInstIndex, InstrWord, CurrentMax, temp;
 address_t  	StreamStart[MAX_SIZE], CurrentInstAddr, CurrentDataAddr;
 instr_t 	InstrWord;
 byte1_t 	InstrType;


node_word* WordList[MAX_SIZE];
node_ref*  RefList[MAX_SIZE];

node_word* NewWord;
node_word* PrevWord;

node_ref* NewRef;
node_ref* PrevRef;

 byte1_t	StartRef, StartStream;	/* flags for memory reference and stream start */
 byte1_t	TempType;		/* temporary instr type */

 int		data_num;	/* number of currently processed SNC data trace */

 int 		ascii;		/* want ascii format */

/* functions */
void read_table(void);
void my_alloc(void);
void write_table(void);
void read_trace(void);
void write_data(void);

int main (int argc, char **argv)
{

    if (argc != 5)
    {
	printf("Usage: %s benchmark_name file_num1 file_num2 0/1\n",argv[0]);
	printf("E.g.: %s vortex 1 10 0\n", argv[0]);
	exit(1);
    }

    bname = argv[1];
    FileNumber1 = atoi(argv[2]);
    FileNumber2 = atoi(argv[3]);
    ascii = atoi(argv[4]);

    printf("decompress sbc: You entered %s %d %d %d\n", bname,FileNumber1,FileNumber2,ascii);

    /* read sbc table */
    read_table();

    /* write sbc ascii table */
    if (ascii) write_table();

    /*read sbc traces, write ascii/binary dinero+*/
    read_trace();

    return(0);
}


/*************************************************************/
/* read sbc table */

void read_table(void)
{
    int     k;

    sprintf(sys_command, "gzip -cd tblID.%s.I.bin.gz", bname);
    if ( (ftable_b = popen(sys_command, "r")) == NULL)
    {
	   fprintf(stderr, "error popen ftable_b\n");
	   exit(1);
    }

    MaxStreamIndex = 1;

    while ( fread(&StreamLen[MaxStreamIndex],sizeof(stream_t),1,ftable_b) == 1 )
    {
        fread(&StreamStart[MaxStreamIndex],sizeof(address_t),1,ftable_b);
        StartRef = 1;
	StartStream = 1;

        for (k=1;k<=StreamLen[MaxStreamIndex]/4;++k)
	{
	    fread(&InstrType,sizeof(byte1_t),1,ftable_b);
	    for (i=3;i>=0;--i)
	    {
	        TempType=InstrType>>(2*i);
	        TempType = TempType & 0x03;

		my_alloc();
	    }
	}

	if (StreamLen[MaxStreamIndex]%4 != 0)
	   fread(&InstrType,sizeof(byte1_t),1,ftable_b);

	for (k=StreamLen[MaxStreamIndex]%4;k>=1;--k)
	{
	    TempType=InstrType>>(2*(k-1));
	    TempType = TempType & 0x03;

	    my_alloc();
	}

	/*read instruction words*/
	NewWord = WordList[MaxStreamIndex];

	for(i=1;i<=StreamLen[MaxStreamIndex];++i)
	{
	    fread(&InstrWord,sizeof(instr_t),1,ftable_b);
	    NewWord->instr_word = InstrWord;
	    NewWord = NewWord -> next;
	}
	++MaxStreamIndex;
    } /*end read table*/


    if ( pclose(ftable_b) == -1 )
    {   fprintf(stderr,"error pclose ftable_b\n");
	exit(1);
    }

    --MaxStreamIndex;
    fprintf(stderr,"%d\n",MaxStreamIndex);
}


/************************************************************************/
/* my_alloc */

void my_alloc(void)
{
    /* alloc new word */
    NewWord = malloc(sizeof(struct node_word));
    NewWord->next = NULL;
    NewWord->instr_type = TempType;

    if (StartStream)
    {   WordList[MaxStreamIndex] = NewWord;
        StartStream = 0;
    }
    else
    {   PrevWord->next=NewWord;
    }
    PrevWord = NewWord;

    /* alloc new mem ref */
    if (TempType !=2)
    {
        NewRef = malloc(sizeof(struct node_ref));
	NewRef->next = NULL;
	NewRef->current_addr = 0;
	NewRef->data_stride = 0;
	NewRef->rep_count = 0;

	if (StartRef)
	{   RefList[MaxStreamIndex] = NewRef;
	    StartRef = 0;
	}
	else
	{     PrevRef->next=NewRef;
	}
	PrevRef = NewRef;
    }
}

/***********************************************************************/
/* read sbc traces and regenerate ascii/bin dinero+ */

void read_trace(void)
{
    int 	j;
    char 	trace_flag;	/* flag for type - 2 instr, 0 load data address, 1 store */

    data_num = FileNumber1;
    trace_flag = 2;

    /* open first sbc data trace */

    fprintf(stderr, "reading sbc data trace #%d\n", data_num);
    sprintf(sys_command, "gzip -cd C3.%s.D.%d.bin.gz", bname, data_num);
    if ( (fsbcd = popen(sys_command, "r")) == NULL)
    {
	   fprintf(stderr, "error popen sbcd\n");
	   exit(1);
    }

    for (j=FileNumber1;j<=FileNumber2;++j)
    {
        /* open sbc instr trace */

    	fprintf(stderr, "reading sbc instr trace #%d\n", j);
	sprintf(sys_command, "gzip -cd T1.%s.I.%d.bin.gz", bname, j);
        if ( (fsbci = popen(sys_command, "r")) == NULL)
    	{
	    fprintf(stderr, "error popen sbci\n");
	    exit(1);
    	}


	sprintf(sys_command, "gzip -f - > %s.%d.bin.gz", bname, j);
	if ( (fdin_b = popen(sys_command, "w")) == NULL)
    	{
	    fprintf(stderr, "error popen fdin_b\n");
	    exit(1);
    	}


	if (ascii)
	{

	    sprintf(sys_command, "gzip -f - > %s.%d.txt.gz", bname, j);
	    if ( (fdin_a = popen(sys_command, "w")) == NULL)
    	    {
	    	fprintf(stderr, "error popen fdin_a\n");
	    	exit(1);
    	    }

	}

	while (fread(&StreamIndex,sizeof(short),1,fsbci)==1)
	{
	    CurrentInstAddr=StreamStart[StreamIndex];
	    NewWord = WordList[StreamIndex];
	    NewRef = RefList[StreamIndex];

	    fwrite(&trace_flag, sizeof(char), 1, fdin_b);
	    fwrite(&CurrentInstAddr, sizeof(unsigned long long), 1, fdin_b);
	    fwrite(&(NewWord->instr_word), sizeof(int unsigned), 1, fdin_b);

	    if (ascii)
	    {
	    	fprintf(fdin_a,"%d %llx %x\n", trace_flag, CurrentInstAddr, NewWord->instr_word);

	    }

	    if ( NewWord->instr_type != 2) /* memory addressing instruction */
	    {
	        write_data();
	    }

	    CurrentInstAddr = CurrentInstAddr +4;
	    NewWord = NewWord->next;

	    while (NewWord != NULL)
	    {
		fwrite(&trace_flag, sizeof(char), 1, fdin_b);
	     	fwrite(&CurrentInstAddr, sizeof(unsigned long long), 1, fdin_b);
	     	fwrite(&(NewWord->instr_word), sizeof(int unsigned), 1, fdin_b);


		if (ascii)
		{
	            fprintf(fdin_a,"%d %llx %x\n", trace_flag, CurrentInstAddr, NewWord->instr_word);
		}

	        if ( NewWord->instr_type != 2) /* memory addressing instruction */
	        {   write_data();
	        }

		CurrentInstAddr = CurrentInstAddr +4;
		NewWord = NewWord->next;
	    }

	}

	if ( pclose(fsbci) == -1 )
    	{   fprintf(stderr,"error pclose sbcd\n");
	    exit(1);
    	}

	if (ascii)
	{
	    if ( pclose(fdin_a) == -1 )
    	    {   fprintf(stderr,"error pclose\n");
	     	exit(1);
    	    }
	}


	if ( pclose(fdin_b) == -1 )
        {   fprintf(stderr,"error pclose\n");
	    exit(1);
 	}
    }

    if ( pclose(fsbcd) == -1 )
    {   fprintf(stderr,"error pclose sbcd\n");
	exit(1);
    }
}

/***********************************************************/
/* write data */

void write_data(void)
{
    byte1_t 	HeaderByte;
    char 	tchar;
    short 	tshort;
    int 	tint;
    int long long tllint;
    char 	h_addr_offset, h_data_stride, h_rep_count;

    if (NewRef->rep_count == 0)
    {
          NewRef->current_addr = NewRef->current_addr - NewRef->data_stride;
	/* read from sbc data file */

	/* find file w data, read header */

	while ( fread(&HeaderByte, sizeof(byte1_t), 1, fsbcd) != 1 )
	{

	    if ( pclose(fsbcd) == -1 )
            {   fprintf(stderr,"error pclose sbcd\n");
	    	exit(1);
 	    }

	    ++data_num;

	    fprintf(stderr, "reading sbc data trace #%d\n", data_num);

	    sprintf(sys_command, "gzip -cd C3.%s.D.%d.bin.gz", bname, data_num);
    	    if ( (fsbcd = popen(sys_command, "r")) == NULL)
    	    {
	   	fprintf(stderr, "error popen sbcd\n");
	   	exit(1);
    	    }
	}

	/* read the rest */

	h_addr_offset = HeaderByte & 0x03;
	h_data_stride = (HeaderByte>>2) & 0x07;
	h_rep_count = (HeaderByte>>5) & 0x07;

	switch (h_addr_offset)
	{
	    case 0:
		fread(&tchar, sizeof(char), 1, fsbcd);
		NewRef->current_addr = NewRef->current_addr + tchar;
		break;
           case 1:
	   	fread(&tshort, sizeof(short), 1, fsbcd);
		NewRef->current_addr = NewRef->current_addr + tshort;
		break;
  	    case 2:
	        fread(&tint, sizeof(int), 1, fsbcd);
		NewRef->current_addr = NewRef->current_addr + tint;
		break;
            case 3:
	    	fread(&tllint, sizeof(int long long), 1, fsbcd);
		NewRef->current_addr = NewRef->current_addr + tllint;
		break;
  	    default:
       		fprintf(stderr,"should not be here\n");
		exit(1);
  		break;
 	}

	switch(h_data_stride)
 	{
  	    case 0:
		NewRef->data_stride = 0;
		break;
  	    case 1:
		fread(&tchar, sizeof(char), 1, fsbcd);
		NewRef->data_stride = tchar;
		break;
            case 2:
		fread(&tshort, sizeof(short), 1, fsbcd);
		NewRef->data_stride = tshort;
		break;
  	    case 3:
	        fread(&tint, sizeof(int), 1, fsbcd);
		NewRef->data_stride = tint;
		break;
  	    case 4:
	    	fread(&tllint, sizeof(int long long), 1, fsbcd);
		NewRef->data_stride = tllint;
		break;
	    case 5:
		NewRef->data_stride = 1;
		break;
	    case 6:
		NewRef->data_stride = 4;
		break;
	    case 7:
		NewRef->data_stride = 8;
		break;
            default:
       	        fprintf(stderr, "should not be here\n");
		exit(1);
  		break;
 	}

	switch(h_rep_count)
 	{
  	    case 0:
		NewRef->rep_count = 0;
		break;
  	    case 1:
		fread(&tchar, sizeof(char), 1, fsbcd);
		NewRef->rep_count = tchar;
		break;
            case 2:
		fread(&tshort, sizeof(short), 1, fsbcd);
		NewRef->rep_count = tshort;
		break;
  	    case 3:
	        fread(&tint, sizeof(int), 1, fsbcd);
		NewRef->rep_count = tint;
		break;
  	    case 4:
	    	fread(&tllint, sizeof(int long long), 1, fsbcd);
		NewRef->rep_count = tllint;
		break;
	    case 5:
		NewRef->rep_count = 1;
		break;
            default:
       	        fprintf(stderr, "should not be here\n");
		exit(1);
  		break;
 	}
	++NewRef->rep_count;
    }

    fwrite(&(NewWord->instr_type), sizeof(char), 1, fdin_b);
    fwrite(&(NewRef->current_addr), sizeof(address_t), 1, fdin_b);

    if (ascii)
    {
        fprintf(fdin_a, "%d %llx 0\n", NewWord->instr_type, NewRef->current_addr);
    }

    NewRef->current_addr = NewRef->current_addr + NewRef->data_stride;
    --NewRef->rep_count;

    NewRef = NewRef->next;
}

/************************************************************************/
/* write_table */
void write_table(void)
{
    int i, j;

    sprintf(fname, "tblID.%s.txt", bname);
    if ( (ftable_a=fopen(fname,"w")) == NULL)
    {
    	perror("open failed");
	exit(1);
    }

    for (i=1;i<=MaxStreamIndex;++i)
    {
        fprintf(ftable_a,"%x %llx\n",StreamLen[i], StreamStart[i]);

	NewWord = WordList[i];
	for (j=1;j<=StreamLen[i];++j)
	{
	    fprintf(ftable_a,"%d\n",NewWord->instr_type);
	    NewWord = NewWord->next;
        }

	NewWord = WordList[i];
	for (j=1;j<=StreamLen[i];++j)
	{
	    fprintf(ftable_a,"%x\n",NewWord->instr_word);
	    NewWord = NewWord->next;
        }
    }

    fclose(ftable_a);
}
