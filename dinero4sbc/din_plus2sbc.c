/**********************************************************************
*	din_plus2sbc.c
*
*	Converts gzipped traces in Dinero+ format
*	to gzipped traces in SBC format
*
*	Assumed Dinero+ format: 1B type (0/1/2), 8B address,
*	if type = 2 (instruction), 4B instruction word.
*	We assume each data reference in the trace 
*	immediataly follows the corresponding load/store instruction.
*
*	Input: traceName, fileNum1, fileNum2
*	(convert from fileNum1 to fileNum2),
*	Dinero+ traces: traceName.fileNum.bin.gz
*
*	Output: SBC traces:
*		T1.traceName.I.fileNum.bin.gz - SBIT
*		C3.traceName.D.fileNum.bin.gz - SBDT
*		tblID.traceName.I.bin.gz
*
*	Authors: Milena & Aleksandar MIlenkovic, March 2003
*
*	Note: This software is released under GNU GPL.
*
*
*	Revision 1.1: Initial
*
************************************************************************/

#include "mytrace.h"

int main (int argc, char **argv)
{
    if (argc != 5)
    {
	fprintf(stderr,"Usage: %s trace_name file_num1 file_num2 FIFO_size\n", argv[0]);
	fprintf(stderr,"E.g.: %s vortex 1 10 1000\n", argv[0]);
	exit(1);
    }
    traceName = argv[1];
    fileNum1 = atoi(argv[2]);
    fileNum2 = atoi(argv[3]);
    FIFOSize = atoi(argv[4]);

    fprintf(stderr, "You entered %s %d %d %d\n", traceName, fileNum1, fileNum2, FIFOSize);

    compressTrace();

    return 0;
}

/***********************************************************************/
/* 	converts dinero+ trace to sbc */

void compressTrace(void)
{
    char 	sysCommand[80]; /* sys command buffer */
    static char	Start = 1;	/* start flag */
    int		i;

    /* open sbc table */
    sprintf(sysCommand, "gzip -f - > tblID.%s.I.bin.gz", traceName);
    if ( (ftable = popen(sysCommand, "w")) == NULL)
	fatal("error popen table\n");

    /* read & compress dinero+ traces */
    for (i=fileNum1; i<=fileNum2; ++i)
    {
    	fprintf(stderr, "commpressing trace #%d\n", i);
	sprintf(sysCommand, "gzip -cd %s.%d.bin.gz", traceName, i);
 	if ( (fdin = popen(sysCommand, "r")) == NULL)
	    fatal("error popen din\n");

	if (!Start)
	{
	    if ( pclose(fsbci) == -1 )	fatal("pclose error\n");
	    if ( pclose(fsbcd) == -1 )	fatal("pclose error\n");
	}
	else Start = 0;

	sprintf(sysCommand, "gzip -f - > C3.%s.D.%d.bin.gz", traceName, i);
        if ( (fsbcd = popen(sysCommand, "w")) == NULL)
	    fatal("error popen sbcd\n");
	sprintf(sysCommand, "gzip -f - > T1.%s.I.%d.bin.gz", traceName, i);
 	if ( (fsbci = popen(sysCommand, "w")) == NULL)
	    fatal("error popen sbci\n");

	while (fread(&myType, sizeof(byte1_t), 1, fdin) == 1)
	{
	    if (fread(&myAddr, sizeof(address_t), 1, fdin) != 1)
	        fatal("error reading din\n");

	    if (myType == INSTR_TYPE) /* instruction reference */
	    {
	        if (fread(&myIWord, sizeof(instr_t), 1, fdin) != 1)
	            fatal("error reading din\n");
		doInstr();
	    }
	    else
	        doData();
	}

	if ( pclose(fdin) == -1 )   fatal("pclose error\n");
    }

    /* write the last stream */
    
    if (currentLen != 0) SearchUpdateStream();
    EmptyFIFO();

    /* wrap up*/
    if ( pclose(fsbci) == -1 )	fatal("pclose error\n");
    if ( pclose(fsbcd) == -1 )	fatal("pclose error\n");
    if ( pclose(ftable) == -1 )	fatal("pclose error\n");


}
/********************************************************************************/
/* process intruction references 						*/

void doInstr(void)
{
    int 		i;
    static char		Start = 1;	/* start flag */
    static address_t	prevIAddr;	/* previous instruction address */

    if (!Start)
    {
        if ( prevIAddr != myAddr - sizeof(instr_t))
        {
    	    /* if end of a stream, find a table entry, write ID */
    	    /* and update data FIFO buffer */

	    SearchUpdateStream();

            /* free previous data address list */

            CurrentTemp = HeadTemp;
            while (CurrentTemp != NULL)
            {
	    	PrevTemp = CurrentTemp;
            	CurrentTemp = CurrentTemp->next;
            	free(PrevTemp);
            }
            if (HeadTemp != NULL) HeadTemp = NULL; /* check this */

            MemRef = 0;
	    currentLen = 0;

	    currentStreamStart = myAddr;

    	} /* end if end of stream */
    }
    else /*start of the trace */
    {
        Start = 0;
	currentStreamStart = myAddr;
	MemRef = 0;
	currentLen = 0;
	tableSize = 0;
	CurrentTemp = NULL;
	HeadTemp = NULL;
	FIFOTop = 0;
	FIFOStart = 0;
    }

    ++currentLen;
    currentStream[currentLen] = myIWord;
    prevIAddr = myAddr;

    /*set type bits to default - 10 (instr not referencing memory */
    /*00 - inst not l/s, 01 load, 10 store, 11 unused */

    i = (currentLen-1)/4+1;
    currentTypeB[i] = currentTypeB[i]<<2;
    currentTypeB[i] +=  INSTR_TYPE;
}

/*********************************************************************************/
void doData(void)
{
    int 	i;

    /* set temp data node */
    ++MemRef;

    CurrentTemp = calloc(1, sizeof(struct temp_data));
    if (CurrentTemp == NULL) fatal("malloc failed \n");

    CurrentTemp->next = NULL;
    CurrentTemp->mem_ref = MemRef;
    CurrentTemp->data_address = myAddr;

    if (MemRef == 1)
        HeadTemp = CurrentTemp;
    else
	PrevTemp->next=CurrentTemp;

    PrevTemp = CurrentTemp;

    /* set the proper instruction type */
    i = (currentLen-1)/4+1;
    currentTypeB[i] = currentTypeB[i] & 0xfc;
    currentTypeB[i] += myType;

}

/********************************************************************************/
/* add new FIFO entries for a new stream (not in the stream table) 		*/

void NewFIFO(void)
{
    int	FIFOIndex;		/* current FIFO index */
    int	currRef = 0;	    	/* current number of mem refs */

    CurrentTemp = HeadTemp;

    while (CurrentTemp != NULL)
    {
        ++currRef;
	FIFOIndex=FIFOTop;
	++FIFOTop;
	if (FIFOTop ==FIFOSize) FIFOTop = 0;

	if (FIFOTop == FIFOStart)
	{   /*full FIFO - write first*/
	    WriteFIFO(FIFOStart);
	    ++FIFOStart;
	    if (FIFOStart == FIFOSize) FIFOStart = 0;
	}

	FIFOBuffer[FIFOIndex].stream_index = StreamIndex;
  	FIFOBuffer[FIFOIndex].mem_ref = currRef;
	FIFOBuffer[FIFOIndex].ready_flag = 0;
	FIFOBuffer[FIFOIndex].data_stride = 0;
	FIFOBuffer[FIFOIndex].rep_count = 0;
	FIFOBuffer[FIFOIndex].addr_offset = CurrentTemp -> data_address;

	CurrentTemp = CurrentTemp->next;
     }

     if (currRef != MemRef)
         fatal("currRef != MemRef");
}

/******************************************************************************/
/* search and update FIFO buffer w data address info */

void SearchUpdateFIFO(void)
{
    int 	i;
    char	found;		/* found flag */
    int		FIFOIndex;	/* current FIFOIndex */

    found = 0;
    FIFOIndex = FIFOStart;

    while (FIFOIndex != FIFOTop && found != 1)
    {
        if ((FIFOBuffer[FIFOIndex].stream_index == StreamIndex)
	    && (FIFOBuffer[FIFOIndex].mem_ref == CurrentTemp->mem_ref)
	    && (FIFOBuffer[FIFOIndex].ready_flag == 0))
        {
	    /*hit - check stride*/
            found = 1;

	    if (FIFOBuffer[FIFOIndex].data_stride == TestStride)
      	        ++FIFOBuffer[FIFOIndex].rep_count;
            else
            {
                if (FIFOBuffer[FIFOIndex].rep_count == 0) /*second time*/
	        {
		    ++FIFOBuffer[FIFOIndex].rep_count;
	            FIFOBuffer[FIFOIndex].data_stride = TestStride;
	        }
		else
        	{   /* new stride - commit and add new entry*/
	    	    FIFOBuffer[FIFOIndex].ready_flag = 1;
	  	    i = FIFOStart;

		    while ((FIFOBuffer[i].ready_flag == 1) && (i != FIFOTop))
	  	    {
	  		WriteFIFO(i);
	  		++i;
	  		if (i == FIFOSize) i = 0;
	  		FIFOStart = i;
	    	    }
	  	    FIFOIndex = FIFOTop;
	  	    ++FIFOTop;
	  	    if (FIFOTop == FIFOSize) FIFOTop = 0;

		    if (FIFOTop == FIFOStart)
	  	    {   /*full FIFO - write first*/
			WriteFIFO(FIFOStart);
			++FIFOStart;
			if (FIFOStart == FIFOSize) FIFOStart = 0;
	  	    } /*end full FIFO */

	  	    FIFOBuffer[FIFOIndex].stream_index = StreamIndex;
     	  	    FIFOBuffer[FIFOIndex].mem_ref = CurrentTemp->mem_ref;
	  	    FIFOBuffer[FIFOIndex].ready_flag = 0;
	  	    FIFOBuffer[FIFOIndex].data_stride = 0;
	  	    FIFOBuffer[FIFOIndex].rep_count = 0;
	   	    FIFOBuffer[FIFOIndex].addr_offset = TestStride;
		} /*end else - add new */
  	    } /*end else - not TestSTride */
        } /* end if ==*/
        else
        {
     	    ++FIFOIndex;
     	    if (FIFOIndex == FIFOSize) FIFOIndex = 0;
    	}
    } /*end while*/

    if (found ==0)
    { /*add new*/
	FIFOIndex = FIFOTop;
	++FIFOTop;
	if (FIFOTop == FIFOSize) FIFOTop = 0;
	
	if (FIFOTop == FIFOStart)
	{    /*full FIFO - write first*/
	    WriteFIFO(FIFOStart);
	    ++FIFOStart;
	    if (FIFOStart == FIFOSize) FIFOStart = 0;
	} /*end full FIFO */

	FIFOBuffer[FIFOIndex].stream_index = StreamIndex;
 	FIFOBuffer[FIFOIndex].mem_ref = CurrentTemp->mem_ref;
	FIFOBuffer[FIFOIndex].ready_flag = 0;
	FIFOBuffer[FIFOIndex].data_stride = 0;
	FIFOBuffer[FIFOIndex].rep_count = 0;
	FIFOBuffer[FIFOIndex].addr_offset = TestStride;
    }
}


/******************************************************************************/

char OffsetSize(offset_t Offset)
{
    if (Offset == 0) return (0);
    if((Offset >= CHAR_MIN) && ( Offset <= CHAR_MAX)) return (1);
    if((Offset >= SHRT_MIN) && ( Offset <= SHRT_MAX)) return (2);
    if((Offset >= INT_MIN) && (Offset <= INT_MAX)) return (3);
    return(4);
}


/******************************************************************************/

void WriteFIFO(int index)
{
    byte1_t 	HeaderByte;
    char 	temp1,temp2,temp3;
    char 	tchar;
    short 	tshort;
    int  	tint;

    temp1 = OffsetSize(FIFOBuffer[index].addr_offset);
    temp2 = OffsetSize(FIFOBuffer[index].data_stride);
    temp3 = OffsetSize(FIFOBuffer[index].rep_count);

    if (temp1 == 0)
 	temp1 = 1;

    switch(FIFOBuffer[index].data_stride)
    {
   	/*case 0:
     	    break;*/
   	case 1:
     	    temp2 = 5;
    	    break;
   	/*case 2:
     	    break; */
   	case 4:
     	    temp2 = 6;
     	    break;
   	case 8:
     	    temp2 = 7;
    	    break;
   	default:
     	     break;
    }

    if (FIFOBuffer[index].rep_count == 1)
    	temp3 = 5;

    HeaderByte = temp1-1 + (temp2<<2) + (temp3<<5);
    fwrite(&HeaderByte, sizeof(byte1_t), 1, fsbcd);

    if (temp2 > 4) temp2 = 0;
    if (temp3 == 5) temp3 = 0;

     switch(temp1)
     {
 	/* case 0:*/
  	case 1:
	    tchar = (char) FIFOBuffer[index].addr_offset;
	    fwrite(&tchar, sizeof(char), 1, fsbcd);
	    break;
  	case 2:
	    tshort = (short) FIFOBuffer[index].addr_offset;
	    fwrite(&tshort, sizeof(short), 1, fsbcd);
	    break;
  	case 3:
	    tint = (int) FIFOBuffer[index].addr_offset;
	    fwrite(&tint, sizeof(int), 1, fsbcd);
	    break;
        case 4:
	    fwrite(&FIFOBuffer[index].addr_offset, sizeof(int long long), 1, fsbcd);
	    break;
  	default:
       	    fatal("should not be here\n");
    }


    switch(temp2)
    {
  	case 0:
  	    break;
  	case 1:
	    tchar = (char) FIFOBuffer[index].data_stride;
	    fwrite(&tchar, sizeof(char), 1, fsbcd);
	    break;
  	case 2:
	    tshort = (short) FIFOBuffer[index].data_stride;
	    fwrite(&tshort, sizeof(short), 1, fsbcd);
	    break;
  	case 3:
	    tint = (int) FIFOBuffer[index].data_stride;
	    fwrite(&tint, sizeof(int), 1, fsbcd);
	    break;
  	case 4:
	    fwrite(&FIFOBuffer[index].data_stride, sizeof(int long long), 1, fsbcd);
	    break;
  	default:
       	    fatal("should not be here\n");
    }

    switch(temp3)
    {
  	case 0:
  	    break;
  	case 1:
	    tchar = (char) FIFOBuffer[index].rep_count;
	    fwrite(&tchar, sizeof(char), 1, fsbcd);
	    break;
        case 2:
	    tshort = (short) FIFOBuffer[index].rep_count;
	    fwrite(&tshort, sizeof(short), 1, fsbcd);
	    break;
  	case 3:
	    tint = (int) FIFOBuffer[index].rep_count;
	    fwrite(&tint, sizeof(int), 1, fsbcd);
	    break;
       /*case 4:
	fwrite(&FIFOBuffer[index].rep_count, sizeof(int long long), 1, fsbcd);
	break;
       */
  	default:
       	    fatal("should not be here\n");
   }
}


/******************************************************************************/

void EmptyFIFO(void)
{
    int i;
    i = FIFOStart;
    while (i !=FIFOTop)
    {
	WriteFIFO(i);
	++i;
	if (i == FIFOSize) i = 0;
    }
}

/******************************************************************************/

void SearchUpdateStream(void)
{
    int 	i;
    char 	found;	/* found flag */

    /* search the table */
    found = 0;
    StreamIndex = tableSize;

    while ( (found == 0) && (StreamIndex > 0) )
    {
  	if ( currentLen == ISLen[StreamIndex] &&
	    currentStreamStart == ISAddress[StreamIndex]) found = 1;
	else --StreamIndex;
    }

    if (!found) /* add new stream */
    {
	++tableSize;
	StreamIndex=tableSize;

	if (tableSize > MAX_TABLE)
	   fatal("table too small!!!!\n");

	ISLen[tableSize] = currentLen;
	ISAddress[tableSize] = currentStreamStart;

	fwrite(&currentLen,sizeof(short),1,ftable);
	fwrite(&currentStreamStart,sizeof(address_t),1,ftable);

	for (i=1;i<=(currentLen-1)/4+1;++i)
	    fwrite(&currentTypeB[i],sizeof(char),1,ftable);

	for (i=1;i<=currentLen;++i)
	    fwrite(&currentStream[i],sizeof(instr_t),1,ftable);


	/* also add mem ref info, if any mem refs in the stream */
	if (MemRef != 0)
	{
	    CurrentTemp = HeadTemp;
	    for (i=1; i<=MemRef;++i)
	    {
		CurrentRef = malloc(sizeof(struct node_ref));
	 	CurrentRef->next = NULL;
	  	CurrentRef->last_addr = CurrentTemp->data_address;
		CurrentTemp = CurrentTemp->next;

	 	if ( i == 1 )
		    RefList[StreamIndex] = CurrentRef;
	     	else
		    PrevRef->next=CurrentRef;
	          
		PrevRef = CurrentRef;
	    }

	      /* we know the entry can not be in FIFO buffer,
	         so just add a new one */
	    NewFIFO();
	}
    } /*end if new stream in table */
    else
    {
	/* update mem ref addr info */
	CurrentRef = RefList[StreamIndex];
	CurrentTemp = HeadTemp;

	while (CurrentTemp != NULL)
	{
	    TestStride = CurrentTemp->data_address - CurrentRef->last_addr;

	    SearchUpdateFIFO();

	    CurrentRef->last_addr = CurrentTemp->data_address;
	    CurrentRef = CurrentRef -> next;
	    CurrentTemp = CurrentTemp -> next;
	}
    }

    /* write stream ID to sbci */
    fwrite(&StreamIndex,sizeof(short),1,fsbci);

    /*reinit stream*/
    for (i=0;i<=currentLen;++i)
    currentTypeB[i] = 0;
}

/******************************************************************************/
/* print error message and exit */

void fatal(const char *msg)
{
    fflush (stdout);
    fprintf (stderr, msg);
    exit (1);
}

/*******************************************************************************/
