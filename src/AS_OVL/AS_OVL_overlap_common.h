
/**************************************************************************
 * This file is part of Celera Assembler, a software program that
 * assembles whole-genome shotgun reads into contigs and scaffolds.
 * Copyright (C) 1999-2004, Applera Corporation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received (LICENSE.txt) a copy of the GNU General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *************************************************************************/

const char *mainid = "$Id: AS_OVL_overlap_common.h,v 1.61 2010/04/02 06:55:32 brianwalenz Exp $";

/*************************************************
* Module:  AS_OVL_overlap.c
* Description:
*   Find matches between pairs of DNA strings and output overlaps
*   (matches that go to an end of both strings involved) and branch points
*   (matches that do not go to the end of either string).
*   Use hashing to select candidates and simple edit distance
*   to confirm matches.
*
*    Programmer:  A. Delcher
*       Written:  29 May 98
*  Last Revised:  30 Nov 99
*
*
* Assumptions:
*  argv[1] is <filename>[.<ext>]
*  Input is from file  argv [1]
*  Output is to file  <filename> . OUTPUT_FILENAME_EXTENSION
*
* Notes:
*   - Removed  ceil  function everywhere.  Overlaps are all <= ERROR_RATE
*     except where indels change length of olap region.
*   - Use hash table for index of strings that match
*   - Do memory allocation by self.
*   - "Band" edit distance calculations
*   - Avoid "hopeless" edit distance calculations
*   - Find branch points in overlaps.
*
*************************************************/

/* RCS info
 * $Id: AS_OVL_overlap_common.h,v 1.61 2010/04/02 06:55:32 brianwalenz Exp $
 * $Revision: 1.61 $
*/


/*************************************************************************/
/* System include files */
/*************************************************************************/

#include  <stdlib.h>
#include  <stdio.h>
#include  <assert.h>
#include  <fcntl.h>
#include  <string.h>
#include  <unistd.h>
#include  <float.h>
#include  <fstream>

#include  <jellyfish/jellyfish.hpp>
#include  <jellyfish/file_header.hpp>
#include  <jellyfish/mapped_file.hpp>

/*************************************************************************/
/* Local include files */
/*************************************************************************/

#include  "AS_OVL_delcher.h"
#include  "AS_PER_gkpStore.h"
#include  "AS_MSG_pmesg.h"
#include  "AS_OVL_overlap.h"
#include  "AS_UTL_fileIO.h"
#include  "AS_UTL_reverseComplement.h"

/*************************************************************************/
/* Type definitions */
/*************************************************************************/

typedef  uint32  Check_Vector_t;
    // Bit vector to see if hash bucket could possibly contain a match

typedef  enum Overlap_Type
  {NONE, LEFT_BRANCH_PT, RIGHT_BRANCH_PT, DOVETAIL}  Overlap_t;

typedef  struct Olap_Info
  {
   int  s_lo, s_hi, t_lo, t_hi;
   double  quality;
   int  delta [AS_READ_MAX_NORMAL_LEN+1];  //  needs only MAX_ERRORS
   int  delta_ct;
   int  s_left_boundary, s_right_boundary;
   int  t_left_boundary, t_right_boundary;
   int  min_diag, max_diag;
  }  Olap_Info_t;


#if 0

#undef HUGE_TABLE_VERSION
#undef TINY_FRAG_VERSION
    //  The HUGE_TABLE_VERSION essentially unlimits the amount of sequence
    //  that can be stored in the table.  It also gets around the problem
    //  of not having enough space to load the kmers to ignore.  It also
    //  requires nearly infinite memory; don't expect it to work in small
    //  spaces!

#if AS_READ_MAX_NORMAL_LEN_BITS > 11
#define HUGE_TABLE_VERSION
#endif

#ifdef  HUGE_TABLE_VERSION
#define STRING_NUM_BITS          24
#else
#define STRING_NUM_BITS          19
#endif

#ifdef  CONTIG_OVERLAPPER_VERSION
#undef  STRING_NUM_BITS
#define STRING_NUM_BITS          11
#endif

#ifdef  TINY_FRAG_VERSION
#undef  STRING_NUM_BITS
#define STRING_NUM_BITS          23
#endif
    //  Number of bits used to store the string number in the
    //  hash table

#define  MAX_STRING_NUM          ((1 << STRING_NUM_BITS) - 1)
   //   Largest string number that can fit in the hash table

#if STRING_NUM_BITS <= 19
#define  OFFSET_BITS             (30 - STRING_NUM_BITS)
#else
#warning OFFSET_BITS is 64-bit
#define  OFFSET_BITS             (60 - STRING_NUM_BITS)
#endif
    //  Number of bits used to store lengths of strings stored
    //  in the hash table


#define  STRING_NUM_MASK         (((uint64)1 << STRING_NUM_BITS) - 1)
#define  OFFSET_MASK             (((uint64)1 << OFFSET_BITS) - 1)
    //  Mask used to extract bits to put in  Offset  field


typedef  struct String_Ref
  {
#if STRING_NUM_BITS + OFFSET_BITS <= 30
   uint32  private_String_Num : STRING_NUM_BITS;
   uint32  private_Offset : OFFSET_BITS;
   uint32  private_Empty : 1;
   uint32  private_Last : 1;
#else
   uint64  private_String_Num : STRING_NUM_BITS;
   uint64  private_Offset : OFFSET_BITS;
   uint64  private_Empty : 1;
   uint64  private_Last : 1;
#endif
  }  String_Ref_t;

#define getStringRefStringNum(X)      ((X).private_String_Num)
#define getStringRefOffset(X)         ((X).private_Offset)
#define getStringRefEmpty(X)          ((X).private_Empty)
#define getStringRefLast(X)           ((X).private_Last)

#define setStringRefStringNum(X, Y)   ((X).private_String_Num = (Y))
#define setStringRefOffset(X, Y)      ((X).private_Offset = (Y))
#define setStringRefEmpty(X, Y)       ((X).private_Empty = (Y))
#define setStringRefLast(X, Y)        ((X).private_Last = (Y))

#else

typedef  uint32   String_Ref_t;

uint32 STRING_NUM_BITS       = (uint32)19;  //  21
uint32 OFFSET_BITS           = (uint32)(30 - STRING_NUM_BITS);

uint32 STRING_NUM_MASK       = ((uint32)1 << STRING_NUM_BITS) - 1;
uint32 OFFSET_MASK           = ((uint32)1 << OFFSET_BITS) - 1;

uint32 MAX_STRING_NUM        = STRING_NUM_MASK;

//
//  [ Last (1) ][ Empty (1) ][ Offset (11) ][ StringNum (19) ]
//

#define getStringRefStringNum(X)      (((X)                   ) & STRING_NUM_MASK)
#define getStringRefOffset(X)         (((X) >> STRING_NUM_BITS) & OFFSET_MASK)
#define getStringRefEmpty(X)          (((X) >> 30             ) & 0x0001)
#define getStringRefLast(X)           (((X) >> 31             ) & 0x0001)

#define setStringRefStringNum(X, Y)   ((X) = (((X) & ~(STRING_NUM_MASK                   )) | ((Y))))
#define setStringRefOffset(X, Y)      ((X) = (((X) & ~(OFFSET_MASK     << STRING_NUM_BITS)) | ((Y) << STRING_NUM_BITS)))
#define setStringRefEmpty(X, Y)       ((X) = (((X) & ~(1               << 30             )) | ((Y) << 30)))
#define setStringRefLast(X, Y)        ((X) = (((X) & ~(1               << 31             )) | ((Y) << 31)))

#endif


typedef  struct Hash_Bucket
  {
   String_Ref_t  Entry [ENTRIES_PER_BUCKET];
   unsigned char  Check [ENTRIES_PER_BUCKET];
   unsigned char  Hits [ENTRIES_PER_BUCKET];
   int16  Entry_Ct;
  }  Hash_Bucket_t;

typedef  struct Hash_Frag_Info
  {
   unsigned int  length : 30;
   unsigned int  left_end_screened : 1;
   unsigned int  right_end_screened : 1;
  }  Hash_Frag_Info_t;


/*************************************************************************/
/* Static Globals */
/*************************************************************************/

#if  SHOW_STATS
FILE  * Stat_File = NULL;

#include  "AS_OVL_olapstats.h"

static Distrib_t  Olap_Len_Dist, Num_Olaps_Dist, Kmer_Freq_Dist, Kmer_Hits_Dist;
static Distrib_t  String_Olap_Dist, Hits_Per_Olap_Dist, Diag_Dist, Gap_Dist;
static Distrib_t  Exacts_Per_Olap_Dist, Edit_Depth_Dist, Hash_Hits_Dist;
#endif

static int64  Bad_Short_Window_Ct = 0;
    //  The number of overlaps rejected because of too many errors in
    //  a small window
static int64  Bad_Long_Window_Ct = 0;
    //  The number of overlaps rejected because of too many errors in
    //  a long window
static double  Branch_Match_Value = 0.0;
static double  Branch_Error_Value = 0.0;
    //  Scores of matches and mismatches in alignments.  Alignment
    //  ends at maximum score.
    //  THESE VALUES CAN BE SET ONLY AT RUN TIME (search for them below)
static char  * Data = NULL;
    //  Stores sequence data of fragments in hash table
static char  * Quality_Data = NULL;
    //  Stores quality data of fragments in hash table
static size_t  Data_Len = 0;
static int  Doing_Partial_Overlaps = FALSE;
    //  If set true by the G option (G for Granger)
    //  then allow overlaps that do not extend to the end
    //  of either read.
static size_t  Extra_Data_Len;
    //  Total length available for hash table string data,
    //  including both regular strings and extra strings
    //  added from kmer screening
static uint64  Extra_Ref_Ct = 0;
static String_Ref_t  * Extra_Ref_Space = NULL;
static int  Extra_String_Ct = 0;
    //  Number of extra strings of screen kmers added to hash table
static int  Extra_String_Subcount = 0;
    //  Number of kmers already added to last extra string in hash table
static int  Frag_Olap_Limit = FRAG_OLAP_LIMIT;
    //  Maximum number of overlaps for end of an old fragment against
    //  a single hash table of frags, in each orientation
static Check_Vector_t  * Hash_Check_Array = NULL;
    //  Bit vector to eliminate impossible hash matches
static int  Hash_String_Num_Offset = 1;
static Hash_Bucket_t  * Hash_Table;
static int  Ignore_Clear_Range = FALSE;
    //  If true will use entire read sequence, ignoring the
    //  clear range values
static int  Ignore_Screen_Info = FALSE;
    //  If true will ignore screen messages with fragments
static int64  Kmer_Hits_With_Olap_Ct = 0;
static int64  Kmer_Hits_Without_Olap_Ct = 0;
static uint64  * Loc_ID = NULL;
    //  Locale ID field of each frag in hash table if in  Contig_Mode .
static int  Min_Olap_Len = DEFAULT_MIN_OLAP_LEN;
static int64  Multi_Overlap_Ct = 0;
static String_Ref_t  * Next_Ref = NULL;
static int  Full_ProtoIO_Output = FALSE;
    //  If set true by -q option, output a single ASCII line
    //  for each overlap in the same format as used by
    //  partial overlaps
static int  String_Ct;
    //  Number of fragments in the hash table
static Hash_Frag_Info_t  * String_Info = NULL;
static int64  * String_Start = NULL;
static int  String_Start_Size = 0;
    //  Number of available positions in  String_Start
static int  Unique_Olap_Per_Pair = TRUE;
    //  If true will allow at most
    //  one overlap output message per oriented fragment pair
    //  Set true by  -u  command-line option; set false by  -m
static size_t  Used_Data_Len = 0;
    //  Number of bytes of Data currently occupied, including
    //  regular strings and extra kmer screen strings

static int  Use_Hopeless_Check = TRUE;
    //  Determines whether check for absence of kmer matches
    //  at the end of a read is used to abort the overlap before
    //  the extension from a single kmer match is attempted.

static int  Use_Window_Filter = FALSE;
    //  Determines whether check for a window containing too many
    //  errors is used to disqualify overlaps.

static int  Read_Edit_Match_Limit [AS_READ_MAX_NORMAL_LEN] = {0};
    //  This array [e] is the minimum value of  Edit_Array [e] [d]
    //  to be worth pursuing in edit-distance computations between reads
    //  (only MAX_ERRORS needed)
static int  Guide_Edit_Match_Limit [AS_READ_MAX_NORMAL_LEN] = {0};
    //  This array [e] is the minimum value of  Edit_Array [e] [d]
    //  to be worth pursuing in edit-distance computations between guides
    //  (only MAX_ERRORS needed)
static int  Read_Error_Bound [AS_READ_MAX_NORMAL_LEN + 1];
    //  This array [i]  is the maximum number of errors allowed
    //  in a match between reads of length  i , which is
    //  i * AS_READ_ERROR_RATE .
static int  Guide_Error_Bound [AS_READ_MAX_NORMAL_LEN + 1];
    //  This array [i]  is the maximum number of errors allowed
    //  in a match between guides of length  i , which is
    //  i * AS_GUIDE_ERROR_RATE .
static double  Branch_Cost [AS_READ_MAX_NORMAL_LEN + 1];
    //  Branch_Cost [i]  is the "goodness" of matching i characters
    //  after a single error in determining branch points.
static int  Bit_Equivalent [256] = {0};
    //  Table to convert characters to 2-bit integer code
static int  Char_Is_Bad [256] = {0};
    //  Table to check if character is not a, c, g or t.
static FragType  * Kind_Of_Frag = NULL;
    //  Type of fragment in hash table, read or guide

static int64  Hash_Entries = 0;
#if  SHOW_STATS
static int64  Collision_Ct = 0, Match_Ct = 0;
static int64  Hash_Find_Ct = 0, Edit_Dist_Ct = 0;
static int32  Overlap_Ct;
#endif

static int64  Total_Overlaps = 0;
static int64  Contained_Overlap_Ct = 0;
static int64  Dovetail_Overlap_Ct = 0;

#if  SHOW_SNPS
static int  Global_SNP_A_Id, Global_SNP_B_Id;
static int  Global_Match_SNP_Bin [20] = {0};
static int  Global_Indel_SNP_Bin [20] = {0};
static int  Global_All_Match_Bin [20] = {0};
static int  Global_Hi_Qual_Bin [20] = {0};
static int  Global_Olap_Ct = 0;
static int  Global_Unscreened_Ct = 0;
#endif

/*************************************************************************/
/* External Global Definitions */
/*************************************************************************/

int64  Kmer_Len = 0;
int64  HSF1     = 666;
int64  HSF2     = 666;
int64  SV1      = 666;
int64  SV2      = 666;
int64  SV3      = 666;

int  Hash_Mask_Bits            = 666;
double  Max_Hash_Load          = MAX_HASH_LOAD;
int  Max_Hash_Strings          = 0;
int  Max_Hash_Data_Len         = 0;
int  Max_Frags_In_Memory_Store = 0;
  // The number of fragments to read in a batch when streaming
  // the old reads against the hash table.

int  Contig_Mode = FALSE;
uint32  Last_Hash_Frag_Read;
int  Lo_Hash_Frag = 0;
int  Hi_Hash_Frag = INT_MAX;
int  Lo_Old_Frag = 0;
int  Hi_Old_Frag = INT_MAX;
int  Num_PThreads = DEFAULT_NUM_PTHREADS;
int64  Olap_Ct = 0;
int  Table_Ct = 0;
clock_t  Start_Time = 0, Stop_Time = 0;


Screen_Range_t  * Screen_Space;

int  Screen_Space_Size;
int  * Screen_Sub = NULL;

char  Sequence_Buffer [2 * AS_READ_MAX_NORMAL_LEN];
char  Quality_Buffer [2 * AS_READ_MAX_NORMAL_LEN];

FILE  * BOL_File = NULL;
// FILE  * Kmer_Skip_File = NULL;
const char*             Kmer_Skip_Path = NULL;
jellyfish::mapped_file* skip_mers_file = NULL;
binary_query*           skip_mers      = NULL;
// jellyfish::mer_dna_bloom_counter* skip_mers      = NULL;
    // File and filter of kmers to be ignored in the hash table
    // Specified by the  -k  option
Output_Stream  Out_Stream = NULL;
BinaryOverlapFile  *Out_BOF = NULL;
gkStore  *OldFragStore;
gkStore  *BACtigStore;
char  * Frag_Store_Path;
char  * BACtig_Store_Path = NULL;
uint32  * IID_List = NULL;
int  IID_List_Len = 0;

pthread_mutex_t  FragStore_Mutex;
pthread_mutex_t  Write_Proto_Mutex;


/*************************************************************************/
/* Function prototypes for internal static functions */
/*************************************************************************/

static String_Ref_t  Add_Extra_Hash_String
    (const char * s);
static void  Add_Match
    (String_Ref_t, int *, int, int *, Work_Area_t *);
static void  Add_Overlap
    (int, int, int, int, double, Olap_Info_t *, int *, Work_Area_t *);
static void  Add_Ref
    (String_Ref_t, int, Work_Area_t *);
static int  Binomial_Bound
    (int, double, int, double);
static int  Binomial_Hit_Limit
    (double p, double trials, double prob_bound);
static int  By_Diag_Sum
    (const void * a, const void * b);
static int  By_uint32
    (const void * a, const void * b);
static void  Capped_Increment
    (unsigned char *, unsigned char, unsigned char);
static char  char_Min
    (char a, char b);
static void  Choose_Best_Partial
    (Olap_Info_t * olap, int ct, int deleted []);
static void  Combine_Into_One_Olap
    (Olap_Info_t * olap, int ct, int deleted []);
static void  Dump_Screen_Info
    (int frag_id, Screen_Info_t * screen, char dir);
static Overlap_t  Extend_Alignment
    (Match_Node_t *, char *, int, char *, int, int *, int *,
     int *, int *, int *, Work_Area_t *);
static void  Find_Overlaps
    (char [], int, char [], Int_Frag_ID_t, Direction_t, Work_Area_t *);
static void  Flip_Screen_Range
    (Screen_Info_t * screen, int len);
static int  Get_Range
    (char * value, char * flag, int * lo, int * hi);
static int  Has_Bad_Window
    (char a [], int n, int window_len, int threshold);
static String_Ref_t  Hash_Find
    (uint64 Key, int64 Sub, char * S, int64 * Where, int * hi_hits);
static String_Ref_t  Hash_Find__
    (uint64 Key, int64 Sub, char * S, int64 * Where, int * hi_hits);
static void  Hash_Insert
    (String_Ref_t, uint64, char *);
static void  Hash_Mark_Empty
    (uint64 key, char * s);
static void  Initialize_Globals
    (void);
static int  Interval_Intersection
    (int, int, int, int);
static int  Lies_On_Alignment
    (int, int, int, int, Work_Area_t *);
static void  Mark_Screened_Ends_Chain
    (String_Ref_t ref);
static void  Mark_Screened_Ends_Single
    (String_Ref_t ref);
static void  Mark_Skip_Kmers
    (void);
static void  Merge_Intersecting_Olaps
    (Olap_Info_t * p, int ct, int deleted []);
static int64  Next_Odd_Prime
    (int64);
static void  Output_Overlap
    (Int_Frag_ID_t, int, Direction_t, Int_Frag_ID_t,
     int, Olap_Info_t *, Work_Area_t *);
static void  Output_Partial_Overlap
    (Int_Frag_ID_t s_id, Int_Frag_ID_t t_id, Direction_t dir,
     const Olap_Info_t * p, int s_len, int t_len,
     Work_Area_t *WA);
static int  Passes_Screen
    (int, int, int);
static int  Prefix_Edit_Dist
    (char *, int, char *, int, int, int *, int *, int *, Work_Area_t *);
static void  Process_Matches
    (int *, char *, int, char * S_quality, Int_Frag_ID_t, Direction_t,
     char *, Hash_Frag_Info_t t_info, char * T_quality, Int_Frag_ID_t,
     Work_Area_t *, int);
static int  Process_String_Olaps
    (char *, int, char * quality, Int_Frag_ID_t, Direction_t,
     Work_Area_t *);
static void  Put_String_In_Hash
    (int i);
static int  Read_Next_Frag
    (char frag [AS_READ_MAX_NORMAL_LEN + 1], char quality [AS_READ_MAX_NORMAL_LEN + 1],
     gkStream *stream, gkFragment *, Screen_Info_t *,
     uint32 * last_frag_read);
static void  Read_uint32_List
    (char * file_name, uint32 * * list, int * n);
static int  Rev_Prefix_Edit_Dist
    (char *, int, char *, int, int, int *, int *, int *, int *,
     Work_Area_t *);
static void  Reverse_String
    (char *, int);
static void  Set_Left_Delta
    (int e, int d, int * leftover, int * t_end, int t_len,
     Work_Area_t * WA);
static void  Set_Right_Delta
    (int e, int d, Work_Area_t * WA);
static void  Show_Alignment
    (char *, char *, Olap_Info_t *);
#if  SHOW_SNPS
static void  Show_SNPs
    (char * a, int a_len, char * a_quality,
     char * b, int b_len, char * b_quality, Olap_Info_t * p,
     Work_Area_t * WA, int * mismatch_ct, int * indel_ct,
     int * olap_bases, int * unscreened_olap_bases,
     int * all_match_ct, int * hi_qual_ct,
     int local_msnp_bin [], int local_isnp_bin [],
     int local_match_bin [], int local_other_bin []);
#endif



static
int
Get_Range(char *value, char *flag, int * lo, int * hi) {
  char *s = value;

  if (*s == '-')
    *lo = 0;
  else
    *lo = strtol(s, &s, 10);

  if (*s != '-') {
    fprintf(stderr, "ERROR:  No hyphen in %s range '%s'\n", flag, value);
    return(1);
  }

  s++;

  if (*s == 0)
    *hi = INT_MAX;
  else
    *hi = strtol(s, &s, 10);

  if (*lo > *hi) {
    fprintf (stderr, "ERROR:  Numbers reversed in %s range '%s'\n", flag, value);
    return(1);
  }

  return(0);
}


int
main(int argc, char **argv) {
  char  bolfile_name[FILENAME_MAX] = {0};
  char  Outfile_Name[FILENAME_MAX] = {0};
  char  iidlist_file_name[FILENAME_MAX] = {0};
  int  illegal;
  char  * p;

  argc = AS_configure(argc, argv);

  {
    time_t  now = time (NULL);
#ifdef CONTIG_OVERLAPPER_VERSION
    fprintf (stderr, "Running Contig version, AS_READ_MAX_NORMAL_LEN = %d\n", AS_READ_MAX_NORMAL_LEN);
#else
    fprintf (stderr, "Running Fragment version, AS_READ_MAX_NORMAL_LEN = %d\n", AS_READ_MAX_NORMAL_LEN);
#endif
    fprintf (stderr, "### Starting at  %s\n", ctime (& now));
  }

  fprintf (stderr, "### Bucket size = " F_SIZE_T " bytes\n", sizeof (Hash_Bucket_t));
  fprintf (stderr, "### Read error rate = %.2f%%\n", 100.0 * AS_READ_ERROR_RATE);
  fprintf (stderr, "### Guide error rate = %.2f%%\n", 100.0 * AS_GUIDE_ERROR_RATE);

  {
    int err=0;
    int arg=1;
    while (arg < argc) {

      if        (strcmp(argv[arg], "-b") == 0) {
        strcpy(bolfile_name, argv[++arg]);
      } else if (strcmp(argv[arg], "-c") == 0) {
        Contig_Mode = TRUE;
      } else if (strcmp(argv[arg], "-G") == 0) {
        Doing_Partial_Overlaps = TRUE;
      } else if (strcmp(argv[arg], "-h") == 0) {
        err += Get_Range (argv[arg+1], argv[arg], & Lo_Hash_Frag, & Hi_Hash_Frag);
        arg++;
      } else if (strcmp(argv[arg], "-I") == 0) {
        strcpy(iidlist_file_name, argv[++arg]);
      } else if (strcmp(argv[arg], "-k") == 0) {
        arg++;
        if ((isdigit(argv[arg][0]) && (argv[arg][1] == 0)) ||
            (isdigit(argv[arg][0]) && isdigit(argv[arg][1]) && (argv[arg][2] == 0))) {
          Kmer_Len = atoi(argv[arg]);
        } else {
          //          Kmer_Skip_File = File_Open (argv[arg], "r");
          Kmer_Skip_Path = argv[arg];
        }
      } else if (strcmp(argv[arg], "-l") == 0) {
        Frag_Olap_Limit = strtol(argv[++arg], NULL, 10);
        if  (Frag_Olap_Limit < 1)
          Frag_Olap_Limit = INT_MAX;
      } else if (strcmp(argv[arg], "-m") == 0) {
        Unique_Olap_Per_Pair = FALSE;
      } else if (strcmp(argv[arg], "-M") == 0) {
        arg++;
#ifdef CONTIG_OVERLAPPER_VERSION
        if        (strcmp (argv[arg], "8GB") == 0) {
          Hash_Mask_Bits    = 24;
          Max_Hash_Data_Len = 400000000;
        } else if (strcmp (argv[arg], "4GB") == 0) {
          Hash_Mask_Bits    = 23;
          Max_Hash_Data_Len = 180000000;
        } else if (strcmp (argv[arg], "2GB") == 0) {
          Hash_Mask_Bits    = 22;
          Max_Hash_Data_Len = 110000000;  //  Was 75000000
        } else if (strcmp (argv[arg], "1GB") == 0) {
          Hash_Mask_Bits    = 21;
          Max_Hash_Data_Len = 30000000;
        } else if (strcmp (argv[arg], "256MB") == 0) {
          Hash_Mask_Bits    = 19;
          Max_Hash_Data_Len = 6000000;
        } else {
          fprintf(stderr, "ERROR:  Unknown memory size '%s'\n", argv[arg]);
          fprintf(stderr, "Valid values are '8GB', '4GB', '2GB', '1GB', '256MB'\n");
          err++;
        }
        Max_Hash_Strings  = 3 * Max_Hash_Data_Len / AS_READ_MAX_NORMAL_LEN;
        if  (Max_Hash_Strings > MAX_STRING_NUM)
          Max_Hash_Strings = MAX_STRING_NUM;
        Max_Frags_In_Memory_Store = OVL_Min_int (2 * Max_Hash_Data_Len / AS_READ_MAX_NORMAL_LEN, MAX_OLD_BATCH_SIZE);
#else
        if        (strcmp (argv[arg], "16GB") == 0) {
          if (STRING_NUM_BITS <= 19) {
            fprintf(stderr, "16GB run module requires STRING_NUM_BITS > 19.\n");
            exit(1);
          } else {
            fprintf(stderr, "\nWARNING:\n");
            fprintf(stderr, "WARNING:  '-M 16GB' not tuned to use less than 16GB!\n");
            fprintf(stderr, "WARNING:  It will use more!\n");
            fprintf(stderr, "WARNING:\n\n");
          }

          //  Read this if you're considering using -M 16GB.  You
          //  can up the Hash_Mask_Bits to 25.  The data set BPW
          //  tested with was highly repetitive, and so 24 was
          //  actually better, letting 1.5M reads (barely) fit
          //  into 24GB memory.
          //
          //  You'll certainly want to adjust Max_Hash_Strings,
          //  and Max_Hash_Data_Len; run it once, watch the table
          //  building results, and then increase/decrease to get
          //  the table loaded around 70%.

          Hash_Mask_Bits    = 24;
          Max_Hash_Strings  = 1500000;
          Max_Hash_Data_Len = 1200000000;

        } else if (strcmp (argv[arg], "8GB") == 0) {
          Hash_Mask_Bits    = 24;
          Max_Hash_Strings  = 512000;
          Max_Hash_Data_Len = 400000000;
        } else if (strcmp (argv[arg], "4GB") == 0) {
          Hash_Mask_Bits    = 23;
          Max_Hash_Strings  = 250000; // Was 250000
          Max_Hash_Data_Len = 220000000; //Was 180000000
        } else if (strcmp (argv[arg], "2GB") == 0) {
          Hash_Mask_Bits    = 22;
          Max_Hash_Strings  = 150000;     //  Was 100000
          Max_Hash_Data_Len = 110000000;  //  Was 75000000
        } else if (strcmp (argv[arg], "1GB") == 0) {
          Hash_Mask_Bits    = 21;
          Max_Hash_Strings  = 50000;      //  Was 40000
          Max_Hash_Data_Len = 75000000;   //  Was 30000000
        } else if (strcmp (argv[arg], "256MB") == 0) {
          Hash_Mask_Bits    = 19;
          Max_Hash_Strings  = 10000;
          Max_Hash_Data_Len = 6000000;
        } else {
          fprintf(stderr, "ERROR:  Unknown memory size '%s'\n", argv[arg]);
          fprintf(stderr, "Valid values are '8GB', '4GB', '2GB', '1GB', '256MB'\n");
          err++;
        }
        Max_Frags_In_Memory_Store = OVL_Min_int (Max_Hash_Strings, MAX_OLD_BATCH_SIZE);
#endif

      } else if (strcmp(argv[arg], "--hashbits") == 0) {
        Hash_Mask_Bits = atoi(argv[++arg]);
      } else if (strcmp(argv[arg], "--hashstrings") == 0) {
        Max_Hash_Strings = atoi(argv[++arg]);
        Max_Frags_In_Memory_Store = OVL_Min_int (Max_Hash_Strings, MAX_OLD_BATCH_SIZE);
      } else if (strcmp(argv[arg], "--hashdatalen") == 0) {
        Max_Hash_Data_Len = atoi(argv[++arg]);
      } else if (strcmp(argv[arg], "--hashload") == 0) {
        Max_Hash_Load = atof(argv[++arg]);
      } else if (strcmp(argv[arg], "--maxreadlen") == 0) {
        //  Quite the gross way to do this, but simple.
        arg++;
        OFFSET_BITS = 1;
        while ((1 << OFFSET_BITS) < atoi(argv[arg]))
          OFFSET_BITS++;

        STRING_NUM_BITS       = 30 - OFFSET_BITS;

        STRING_NUM_MASK       = (1 << STRING_NUM_BITS) - 1;
        OFFSET_MASK           = (1 << OFFSET_BITS) - 1;

        MAX_STRING_NUM        = STRING_NUM_MASK;

      } else if (strcmp(argv[arg], "-o") == 0) {
        strcpy(Outfile_Name, argv[++arg]);
      } else if (strcmp(argv[arg], "-p") == 0) {
        Full_ProtoIO_Output = TRUE;
      } else if (strcmp(argv[arg], "-r") == 0) {
        err += Get_Range (argv[arg+1], argv[arg], & Lo_Old_Frag, & Hi_Old_Frag);
        arg++;
      } else if (strcmp(argv[arg], "-s") == 0) {
        Ignore_Screen_Info = TRUE;
      } else if (strcmp(argv[arg], "-t") == 0) {
        Num_PThreads = (int) strtol (argv[++arg], & p, 10);
        if  (Num_PThreads < 1)
          Num_PThreads = 1;
      } else if (strcmp(argv[arg], "-u") == 0) {
        Unique_Olap_Per_Pair = TRUE;
      } else if (strcmp(argv[arg], "-v") == 0) {
        Min_Olap_Len = (int) strtol (argv[++arg], & p, 10);
      } else if (strcmp(argv[arg], "-w") == 0) {
        Use_Window_Filter = TRUE;
      } else if (strcmp(argv[arg], "-x") == 0) {
        Ignore_Clear_Range = TRUE;
      } else if (strcmp(argv[arg], "-z") == 0) {
        Use_Hopeless_Check = FALSE;
      } else {
        if (Frag_Store_Path == NULL) {
          Frag_Store_Path = argv[arg];
        } else if (Contig_Mode && (BACtig_Store_Path == NULL)) {
          if  (bolfile_name[0] == 0)
            fprintf (stderr, "ERROR:  No .BOL file name specified\n"), exit(1);
          if  (Outfile_Name[0] != 0)
            fprintf (stderr, "WARNING:  Output file name '%s' ignored - output is in -b file '%s'\n",
                     Outfile_Name, bolfile_name);
          BOL_File = File_Open (bolfile_name, "w");
          BACtig_Store_Path = argv[arg];
        } else {
          fprintf(stderr, "Unknown option '%s'\n", argv[arg]);
          err++;
        }
      }
      arg++;
    }

    //  Fix up some flags if we're allowing high error rates.
    //
    if (AS_OVL_ERROR_RATE > 0.06) {
      if (Use_Window_Filter)
        fprintf(stderr, "High error rates requested -- window-filter turned off despite -w flag!\n");
      Use_Window_Filter  = FALSE;
      Use_Hopeless_Check = FALSE;
    }

    if (Max_Hash_Strings == 0)
      fprintf(stderr, "* No memory model supplied; -M needed!\n"), err++;

    if (Kmer_Len == 0)
      fprintf(stderr, "* No kmer length supplied; -k needed!\n"), err++;

    if (Max_Hash_Strings > MAX_STRING_NUM)
      fprintf(stderr, "Too many strings (--hashstrings), must be less than %d\n", MAX_STRING_NUM), err++;

    if ((!Contig_Mode) && (Outfile_Name[0] == 0))
      fprintf (stderr, "ERROR:  No output file name specified\n"), err++;

    if ((err) || (Frag_Store_Path == NULL)) {
      fprintf(stderr, "USAGE:  %s [options] <gkpStorePath>\n", argv[0]);
      fprintf(stderr, "\n");
      fprintf(stderr, "-b <fn>     in contig mode, specify the output file\n");
      fprintf(stderr, "-c          contig mode.  Use 2 frag stores.  First is\n");
      fprintf(stderr, "            for reads; second is for contigs\n");
      fprintf(stderr, "-G          do partial overlaps\n");
      fprintf(stderr, "-h <range>  to specify fragments to put in hash table\n");
      fprintf(stderr, "            Implies LSF mode (no changes to frag store)\n");
      fprintf(stderr, "-I          designate a file of frag iids to limit olaps to\n");
      fprintf(stderr, "            (Contig mode only)\n");
      fprintf(stderr, "-k          if one or two digits, the length of a kmer, otherwise\n");
      fprintf(stderr, "            the filename containing a list of kmers to ignore in\n");
      fprintf(stderr, "            the hash table\n");
      fprintf(stderr, "-l          specify the maximum number of overlaps per\n");
      fprintf(stderr, "            fragment-end per batch of fragments.\n");
      fprintf(stderr, "-m          allow multiple overlaps per oriented fragment pair\n");
      fprintf(stderr, "-M          specify memory size.  Valid values are '8GB', '4GB',\n");
      fprintf(stderr, "            '2GB', '1GB', '256MB'.  (Not for Contig mode)\n");
      fprintf(stderr, "-o          specify output file name\n");
      fprintf(stderr, "-P          write protoIO output (if not -G)\n");
      fprintf(stderr, "-r <range>  specify old fragments to overlap\n");
      fprintf(stderr, "-s          ignore screen information with fragments\n");
      fprintf(stderr, "-t <n>      use <n> parallel threads\n");
      fprintf(stderr, "-u          allow only 1 overlap per oriented fragment pair\n");
      fprintf(stderr, "-v <n>      only output overlaps of <n> or more bases\n");
      fprintf(stderr, "-w          filter out overlaps with too many errors in a window\n");
      fprintf(stderr, "-x          ignore the clear ranges on reads and use the \n");
      fprintf(stderr, "            full sequence\n");
      fprintf(stderr, "-z          skip the hopeless check\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "--hashbits n       Use n bits for the hash mask.  This directly sets\n");
      fprintf(stderr, "                   the amount of memory used:\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "--hashstrings n    Load at most n strings into the hash table at one time.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "--hashdatalen n    Use at most n bytes for the hash table.\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "--hashload f       Load to at most 0.0 < f < 1.0 capacity (default 0.7).\n");
      fprintf(stderr, "\n");
      fprintf(stderr, "--maxreadlen n     Use m=log2(n) bits for storing read positions; read length limited\n");
      fprintf(stderr, "                   to n, and --hashstrings limited to 2^(30-m).  Common values:\n");
      fprintf(stderr, "                     maxreadlen 2048 -> hashstrings  524288 (default)\n");
      fprintf(stderr, "                     maxreadlen  512 -> hashstrings 2097152\n");
      fprintf(stderr, "                     maxreadlen  128 -> hashstrings 8388608\n");
      fprintf(stderr, "\n");
      exit(1);
    }

    assert(NULL == Out_Stream);
    assert(NULL == Out_BOF);

    if (Full_ProtoIO_Output)
      Out_Stream = File_Open (Outfile_Name, "w");
    else
      Out_BOF    = AS_OVS_createBinaryOverlapFile(Outfile_Name, FALSE);
  }

#if  SHOW_STATS
 assert(NULL == Stat_File);
 Stat_File = File_Open (STAT_FILE_NAME, "w");
{
   int  i;
   fprintf (Stat_File, ">");
   for  (i = 0;  i < argc;  i ++)
     fprintf (Stat_File, " %s", argv [i]);
   fprintf (Stat_File, "\n");
 }
#endif

   //  We know enough now to set the hash function variables, and some
   //  other random variables.
   //
   HSF1 = Kmer_Len - (Hash_Mask_Bits / 2);
   HSF2 = 2 * Kmer_Len - Hash_Mask_Bits;
   SV1  = HSF1 + 2;
   SV2  = (HSF1 + HSF2) / 2;
   SV3  = HSF2 - 2;

   Branch_Match_Value = (Doing_Partial_Overlaps) ? PARTIAL_BRANCH_MATCH_VAL : DEFAULT_BRANCH_MATCH_VAL;
   Branch_Error_Value = Branch_Match_Value - 1.0;

   fprintf(stderr, "\n");
   fprintf(stderr, "STRING_NUM_BITS     %d\n", STRING_NUM_BITS);
   fprintf(stderr, "OFFSET_BITS         %d\n", OFFSET_BITS);
   fprintf(stderr, "STRING_NUM_MASK     %d\n", STRING_NUM_MASK);
   fprintf(stderr, "OFFSET_MASK         %d\n", OFFSET_MASK);
   fprintf(stderr, "MAX_STRING_NUM      %d\n", MAX_STRING_NUM);
   fprintf(stderr, "\n");
   fprintf(stderr, "Hash_Mask_Bits      %d\n", Hash_Mask_Bits);
   fprintf(stderr, "Max_Hash_Strings    %d\n", Max_Hash_Strings);
   fprintf(stderr, "Max_Hash_Data_Len   %d\n", Max_Hash_Data_Len);
   fprintf(stderr, "Max_Hash_Load       %f\n", Max_Hash_Load);
   fprintf(stderr, "Kmer Length         %d\n", (int)Kmer_Len);
   fprintf(stderr, "Min Overlap Length  %d\n", Min_Olap_Len);
   fprintf(stderr, "MAX_ERRORS          %d\n", MAX_ERRORS);
   fprintf(stderr, "ERRORS_FOR_FREE     %d\n", ERRORS_FOR_FREE);

   assert (8 * sizeof (uint64) > 2 * Kmer_Len);

   Initialize_Globals ();

   if  (Contig_Mode)
       {
        if  (BACtig_Store_Path == NULL)
            {
             fprintf (stderr, "ERROR:  No BACtig store specified\n");
             exit (1);
            }
        BACtigStore = new gkStore(BACtig_Store_Path, FALSE, FALSE);

        if  (iidlist_file_name[0] != 0)
            {
             fprintf (stderr, "### Using IID list = %s\n",
                      iidlist_file_name);
             Read_uint32_List (iidlist_file_name, & IID_List, & IID_List_Len);
             if  (IID_List_Len <= 0)
                 {
                  fprintf (stderr, "ERROR:  Empty IID_List\n");
                  exit (1);
                 }
             if  (Lo_Old_Frag != 0 || Hi_Old_Frag != INT_MAX)
                 {
                  fprintf (stderr,
                           "WARNING:  IID list specified, range from -r ignored\n");
                  Lo_Old_Frag = 0;
                  Hi_Old_Frag = INT_MAX;
                 }
            }
       }

   fprintf (stderr, "* Using fragstore %s\n", Frag_Store_Path);

   OldFragStore = new gkStore(Frag_Store_Path, FALSE, FALSE);

   /****************************************/
   OverlapDriver(argc, argv);
   /****************************************/

   if (Out_Stream)
      fclose (Out_Stream);
   Out_Stream = NULL;

   if (Out_BOF)
     AS_OVS_closeBinaryOverlapFile(Out_BOF);

   if  (BOL_File)
     fclose (BOL_File);
   BOL_File = NULL;



#if  SHOW_STATS
fprintf (Stat_File, "Regular Overlaps:\n %10llu\n",
           Regular_Olap_Ct);
fprintf (Stat_File, "Duplicate Olaps:\n %10llu\n",
           Duplicate_Olap_Ct);
fprintf (Stat_File, "Too Short:\n %10llu\n",
           Too_Short_Ct);
Print_Distrib (Olap_Len_Dist, "Overlap Lengths");
Print_Distrib (Num_Olaps_Dist, "Numbers of Overlaps:");
Print_Distrib (Kmer_Freq_Dist, "Kmer Frequency in Hash Table:");
Print_Distrib (Kmer_Hits_Dist, "Kmer Hits per Fragment:");
Print_Distrib (String_Olap_Dist, "String Olaps per Fragment (each direction):");
Print_Distrib (Hits_Per_Olap_Dist, "Kmer Hits\n  per String Olap:");
Print_Distrib (Diag_Dist, "Diagonals per String Olap:");
Print_Distrib (Gap_Dist, "Num Gaps in\n  Nice Olaps:");
Print_Distrib (Exacts_Per_Olap_Dist, "Exact Matches per\n  String Olap:");
Print_Distrib (Edit_Depth_Dist, "Edit Distance Depth:");
fprintf (Stat_File, "Edit_Dist_Ct = " F_S64"\n", Edit_Dist_Ct);
fprintf (Stat_File, "Matches = " F_S64"\n", Match_Ct);
fprintf (Stat_File, "" F_S64" collisions in " F_S64" finds\n", Collision_Ct, Hash_Find_Ct);
// fprintf (Stat_File, "String_Olap_Size = %ld\n", WA -> String_Olap_Size);
fprintf (stderr, "Stats written to file '%s'\n", STAT_FILE_NAME);
fclose (Stat_File);
#endif

   fprintf (stderr, " Kmer hits without olaps = " F_S64 "\n", Kmer_Hits_Without_Olap_Ct);
   fprintf (stderr, "    Kmer hits with olaps = " F_S64 "\n", Kmer_Hits_With_Olap_Ct);
   fprintf (stderr, "  Multiple overlaps/pair = " F_S64 "\n", Multi_Overlap_Ct);
   fprintf (stderr, " Total overlaps produced = " F_S64 "\n", Total_Overlaps);
   fprintf (stderr, "      Contained overlaps = " F_S64 "\n", Contained_Overlap_Ct);
   fprintf (stderr, "       Dovetail overlaps = " F_S64 "\n", Dovetail_Overlap_Ct);
   fprintf (stderr, "Rejected by short window = " F_S64 "\n", Bad_Short_Window_Ct);
   fprintf (stderr, " Rejected by long window = " F_S64 "\n", Bad_Long_Window_Ct);

   delete OldFragStore;

#if  SHOW_SNPS
{
 int  total [20] = {0};
 int  a, b, c, d, e;
 int  i;

 fprintf (stderr, "%5s  %9s  %9s  %9s  %9s  %9s\n",
          "Qual", "MatchSNPs", "IndelSNPs", "AllMatch", "Other", "Total");
 for  (i = 3;  i < 13;  i ++)
   {
    total [i] = Global_Match_SNP_Bin [i]
                  + Global_Indel_SNP_Bin [i]
                  + Global_All_Match_Bin [i]
                  + Global_Hi_Qual_Bin [i];
    fprintf (stderr, "%2d-%2d  %9d  %9d  %9d  %9d  %9d\n",
             5 * i, 5 * i + 4,
             Global_Match_SNP_Bin [i],
             Global_Indel_SNP_Bin [i],
             Global_All_Match_Bin [i],
             Global_Hi_Qual_Bin [i],
             total [i]);
   }

 a = b = c = d = e = 0;
 fprintf (stderr, "\n%5s  %9s  %9s  %9s  %9s  %9s\n",
          "Qual", "MatchSNPs", "IndelSNPs", "AllMatch", "Other", "Total");
 for  (i = 12;  i >= 3;  i --)
   {
    a += Global_Match_SNP_Bin [i];
    b += Global_Indel_SNP_Bin [i];
    c += Global_All_Match_Bin [i];
    d += Global_Hi_Qual_Bin [i];
    e += total [i];
    fprintf (stderr, " >=%2d  %9d  %9d  %9d  %9d  %9d\n",
             5 * i, a, b, c, d, e);
   }

 fprintf (stderr, "\nTotal overlap bases = %d\n", Global_Olap_Ct);
 fprintf (stderr, "Total unscreened bases = %d\n", Global_Unscreened_Ct);
}
#endif

{
 time_t  now = time (NULL);
 fprintf (stderr, "### Finished at  %s\n", ctime (& now));
}


fprintf (stderr, "### Return from main\n");
   return  0;
  }



static String_Ref_t  Add_Extra_Hash_String
    (const char * s)

//  Add string  s  as an extra hash table string and return
//  a single reference to the beginning of it.

  {
   String_Ref_t  ref = { 0 };
   size_t  new_len;
   int  len, sub;

   new_len = Used_Data_Len + Kmer_Len;
   if  (Extra_String_Subcount < MAX_EXTRA_SUBCOUNT)
       sub = String_Ct + Extra_String_Ct - 1;
     else
       {
        sub = String_Ct + Extra_String_Ct;
        if  (sub >= String_Start_Size)
            {
             String_Start_Size *= MEMORY_EXPANSION_FACTOR;
             if  (sub >= String_Start_Size)
                 String_Start_Size = sub;
             String_Start = (int64 *) safe_realloc (String_Start,
                  String_Start_Size * sizeof (int64));
            }
        String_Start [sub] = Used_Data_Len;
        Extra_String_Ct ++;
        Extra_String_Subcount = 0;
        new_len ++;
       }

   if  (new_len >= Extra_Data_Len)
       {
        Extra_Data_Len = (size_t) (Extra_Data_Len * MEMORY_EXPANSION_FACTOR);
        if  (new_len > Extra_Data_Len)
            Extra_Data_Len = new_len;
        Data = (char *) safe_realloc (Data, Extra_Data_Len);
       }
   strncpy (Data + String_Start [sub] + Kmer_Len * Extra_String_Subcount,
        s, Kmer_Len + 1);
   Used_Data_Len = new_len;

   setStringRefStringNum(ref, sub);
   if  (sub > MAX_STRING_NUM)
       {
        fprintf (stderr, "Too many skip kmer strings for hash table.\n"
             "Try skipping hopeless check (-z option)\n"
             "Exiting\n");
        exit (1);
       }
   setStringRefOffset(ref, Extra_String_Subcount * Kmer_Len);
   assert(Extra_String_Subcount * Kmer_Len < OFFSET_MASK);
   setStringRefLast(ref, TRUE);
   setStringRefEmpty(ref, TRUE);
   Extra_String_Subcount ++;

   return  ref;
  }



static void  Add_Match
    (String_Ref_t ref, int * start, int offset, int * consistent,
     Work_Area_t * wa)

//  Add information for the match in  ref  to the list
//  starting at subscript  (* start) .  The matching window begins
//  offset  bytes from the beginning of this string.

  {
   int  * p, save;
   int  diag = 0, new_diag, expected_start = 0, num_checked = 0;
   int  move_to_front = FALSE;

#if  SHOW_STATS
Kmer_Hits_Ct ++;
#endif

   new_diag = getStringRefOffset(ref) - offset;

   for  (p = start;  (* p) != 0;  p = & (wa -> Match_Node_Space [(* p)] . Next))
     {
      expected_start = wa -> Match_Node_Space [(* p)] . Start
                         + wa -> Match_Node_Space [(* p)] . Len
                         - Kmer_Len + 1 + HASH_KMER_SKIP;
//  Added  HASH_KMER_SKIP here --------------^^^^^^^^^^^^^^
      diag = wa -> Match_Node_Space [(* p)] . Offset
               - wa -> Match_Node_Space [(* p)] . Start;

      if  (expected_start < offset)
          break;
      if  (expected_start == offset)
          {
           if  (new_diag == diag)
               {
                wa -> Match_Node_Space [(* p)] . Len += 1 + HASH_KMER_SKIP;
                if  (move_to_front)
                    {
                     save = (* p);
                     (* p) = wa -> Match_Node_Space [(* p)] . Next;
                     wa -> Match_Node_Space [save] . Next = (* start);
                     (* start) = save;
                    }
                return;
               }
             else
               move_to_front = TRUE;
          }
      num_checked ++;
     }

   if  (wa -> Next_Avail_Match_Node == wa -> Match_Node_Size)
       {
        wa -> Match_Node_Size = (int32) (wa -> Match_Node_Size *
               MEMORY_EXPANSION_FACTOR);
        //fprintf (stderr, "### reallocing  Match_Node_Space  Size = %d\n",
        //         wa -> Match_Node_Size);
        wa -> Match_Node_Space = (Match_Node_t *) safe_realloc
               (wa -> Match_Node_Space,
                wa -> Match_Node_Size * sizeof (Match_Node_t));
       }

   if  ((* start) != 0
          && (num_checked > 0
                || abs (diag - new_diag) > 3
                || offset < expected_start + Kmer_Len - 2))
       (* consistent) = FALSE;

   save = (* start);
   (* start) = wa -> Next_Avail_Match_Node;
   wa -> Next_Avail_Match_Node ++;

   wa -> Match_Node_Space [(* start)] . Offset = getStringRefOffset(ref);
   wa -> Match_Node_Space [(* start)] . Len = Kmer_Len;
   wa -> Match_Node_Space [(* start)] . Start = offset;
   wa -> Match_Node_Space [(* start)] . Next = save;

   return;
  }



static void  Add_Overlap
    (int s_lo, int s_hi, int t_lo, int t_hi, double qual,
     Olap_Info_t * olap, int * ct, Work_Area_t * WA)

//  Add information for the overlap between strings  S  and  T
//  at positions  s_lo .. s_hi  and  t_lo .. t_hi , resp., and
//  with quality  qual  to the array  olap []  which
//  currently has  (* ct)  entries.  Increment  (* ct)  if this
//  is a new, distinct overlap; otherwise, modify an existing
//  entry if this is just a "slide" of an existing overlap.

  {
   int  i, new_diag;

   if  (Doing_Partial_Overlaps)
     olap += (* ct);
   // Don't combine overlaps when doing partials
   else
     {

       new_diag = t_lo - s_lo;
       for  (i = 0;  i < (* ct);  i ++)
         {
           int  old_diag = olap -> t_lo - olap -> s_lo;

           if  ((new_diag > 0
                 && old_diag > 0
                 && olap -> t_right_boundary - new_diag - olap -> s_left_boundary
                 >= MIN_INTERSECTION)
                || (new_diag <= 0
                    && old_diag <= 0
                    && olap -> s_right_boundary + new_diag - olap -> t_left_boundary
                    >= MIN_INTERSECTION))
             {
               if  (new_diag < olap -> min_diag)
                 olap -> min_diag = new_diag;
               if  (new_diag > olap -> max_diag)
                 olap -> max_diag = new_diag;
               if  (s_lo < olap -> s_left_boundary)
                 olap -> s_left_boundary = s_lo;
               if  (s_hi > olap -> s_right_boundary)
                 olap -> s_right_boundary = s_hi;
               if  (t_lo < olap -> t_left_boundary)
                 olap -> t_left_boundary = t_lo;
               if  (t_hi > olap -> t_right_boundary)
                 olap -> t_right_boundary = t_hi;
               if  (qual < olap -> quality)      // lower value is better
                 {
                   olap -> s_lo = s_lo;
                   olap -> s_hi = s_hi;
                   olap -> t_lo = t_lo;
                   olap -> t_hi = t_hi;
                   olap -> quality = qual;
                   memcpy (& (olap ->  delta), WA -> Left_Delta,
                           WA -> Left_Delta_Len * sizeof (int));
                   olap -> delta_ct = WA -> Left_Delta_Len;
                 }

               //  check for intersections before outputting
               return;
             }

           olap ++;
         }
     }

   if  ((* ct) >= MAX_DISTINCT_OLAPS)
       return;   // no room for a new entry; this shouldn't happen

   olap -> s_lo = olap -> s_left_boundary = s_lo;
   olap -> s_hi = olap -> s_right_boundary = s_hi;
   olap -> t_lo = olap -> t_left_boundary = t_lo;
   olap -> t_hi = olap -> t_right_boundary = t_hi;
   olap -> quality = qual;
   memcpy (& (olap ->  delta), WA -> Left_Delta,
               WA -> Left_Delta_Len * sizeof (int));
   olap -> delta_ct = WA -> Left_Delta_Len;
   olap -> min_diag = olap -> max_diag = t_lo - s_lo;
   (* ct) ++;

   return;
  }


static void  Add_Ref
    (String_Ref_t Ref, int Offset, Work_Area_t * WA)

//  Add information for  Ref  and all its matches to the global
//  hash table in  String_Olap_Space .  Grow the space if necessary
//  by  MEMORY_EXPANSION_FACTOR .  The matching window begins
//  Offset  bytes from the beginning of this string.

  {
   int32  Prev, StrNum, Sub;
   int  consistent;

   StrNum = getStringRefStringNum(Ref);
   Sub = (StrNum ^ (StrNum >> STRING_OLAP_SHIFT)) & STRING_OLAP_MASK;

   while  (WA -> String_Olap_Space [Sub] . Full
              && WA -> String_Olap_Space [Sub] . String_Num != StrNum)
     {
      Prev = Sub;
      Sub = WA -> String_Olap_Space [Sub] . Next;
      if  (Sub == 0)
          {
           if  (WA -> Next_Avail_String_Olap == WA -> String_Olap_Size)
               {
                WA -> String_Olap_Size = (int32) (WA -> String_Olap_Size *
                       MEMORY_EXPANSION_FACTOR);
                //fprintf (stderr, "### reallocing  String_Olap_Space  Size = %d\n",
                //         WA -> String_Olap_Size);
                WA -> String_Olap_Space = (String_Olap_t *) safe_realloc
                       (WA -> String_Olap_Space,
                        WA -> String_Olap_Size * sizeof (String_Olap_t));
               }
           Sub = WA -> Next_Avail_String_Olap ++;
           WA -> String_Olap_Space [Prev] . Next = Sub;
           WA -> String_Olap_Space [Sub] . Full = FALSE;
           break;
          }
     }

   if  (! WA -> String_Olap_Space [Sub] . Full)
       {
        WA -> String_Olap_Space [Sub] . String_Num = StrNum;
        WA -> String_Olap_Space [Sub] . Match_List = 0;
        WA -> String_Olap_Space [Sub] . diag_sum = 0.0;
        WA -> String_Olap_Space [Sub] . diag_ct = 0;
        WA -> String_Olap_Space [Sub] . Next = 0;
        WA -> String_Olap_Space [Sub] . Full = TRUE;
        WA -> String_Olap_Space [Sub] . consistent = TRUE;
#if  SHOW_STATS
WA -> String_Olap_Space [Sub] . Kmer_Hits = 0;
#endif
       }

#if  SHOW_STATS
WA -> String_Olap_Space [Sub] . Kmer_Hits ++;
#endif

   consistent = WA -> String_Olap_Space [Sub] . consistent;

   WA -> String_Olap_Space [Sub] . diag_sum += getStringRefOffset(Ref) - Offset;
   WA -> String_Olap_Space [Sub] . diag_ct ++;
   Add_Match (Ref, & (WA -> String_Olap_Space [Sub] . Match_List),
              Offset, & consistent, WA);

   WA -> String_Olap_Space [Sub] . consistent = consistent;

   return;
  }



int  Build_Hash_Index
    (gkStream *stream, int32 first_frag_id, gkFragment *myRead)

/* Read the next batch of strings from  stream  and create a hash
*  table index of their  Kmer_Len -mers.  Return  1  if successful;
*  0 otherwise.  The batch ends when either end-of-file is encountered
*  or  Max_Hash_Strings  have been read in.   first_frag_id  is the
*  internal ID of the first fragment in the hash table. */

  {
   String_Ref_t  ref;
   Screen_Info_t  screen;
   int64  total_len;
   static int64  max_extra_ref_ct = 0;
   static int64  old_ref_len, new_ref_len;
   int  frag_status;
   int64  i;
   int  screen_blocks_used = 0;
   int  hash_entry_limit;
   int  j;

   Hash_String_Num_Offset = first_frag_id;
   String_Ct = Extra_String_Ct = 0;
   Extra_String_Subcount = MAX_EXTRA_SUBCOUNT;
   total_len = 0;
   if  (Data == NULL)
       {
        Extra_Data_Len = Data_Len = Max_Hash_Data_Len + AS_READ_MAX_NORMAL_LEN;
        Data = (char *) safe_realloc (Data, Data_Len);
        Quality_Data = (char *) safe_realloc (Quality_Data, Data_Len);
        old_ref_len = Data_Len / (HASH_KMER_SKIP + 1);
        Next_Ref = (String_Ref_t *) safe_realloc
             (Next_Ref, old_ref_len * sizeof (String_Ref_t));
       }

   memset (Next_Ref, '\377', old_ref_len * sizeof (String_Ref_t));
   memset (Hash_Table, 0, HASH_TABLE_SIZE * sizeof (Hash_Bucket_t));
   memset (Hash_Check_Array, 0, HASH_TABLE_SIZE * sizeof (Check_Vector_t));

   screen . range = (Screen_Range_t *) safe_malloc
                         (INIT_SCREEN_MATCHES * sizeof (Screen_Range_t));
   screen . match_len = INIT_SCREEN_MATCHES;
   screen . num_matches = 0;

   fprintf (stderr, "### Build_Hash:  first_frag_id = %d  Max_Hash_Strings = %d\n",
        first_frag_id, Max_Hash_Strings);

   screen_blocks_used = 1;

   Extra_Ref_Ct = 0;
   Hash_Entries = 0;
   hash_entry_limit = Max_Hash_Load * HASH_TABLE_SIZE * ENTRIES_PER_BUCKET;

   while  (String_Ct < Max_Hash_Strings
             && total_len < Max_Hash_Data_Len
             && Hash_Entries < hash_entry_limit
             && (frag_status
                     = Read_Next_Frag (Sequence_Buffer, Quality_Buffer, stream,
                                       myRead, & screen, & Last_Hash_Frag_Read)))
     {
      int  extra, len;
      size_t  new_len;

      if  (frag_status == DELETED_FRAG)
          {
           Sequence_Buffer [0] = '\0';
           Quality_Buffer [0] = '\0';
          }
        else
          {
                if  (screen . num_matches == 0)
                    Screen_Sub [String_Ct] = 0;
                  else
                    {
                     int  new_size = screen_blocks_used + screen . num_matches;

                     while  (new_size >= Screen_Space_Size)
                         {
                          Screen_Space_Size *= MEMORY_EXPANSION_FACTOR;
                          //fprintf (stderr, "### reallocing  Screen_Space  Size = %d\n",
                          //         Screen_Space_Size);
                          Screen_Space
                              = (Screen_Range_t *) safe_realloc
                                    (Screen_Space,
                                     Screen_Space_Size * sizeof (Screen_Range_t));
                         }

                     memmove (Screen_Space + screen_blocks_used, screen . range,
                              screen . num_matches * sizeof (Screen_Range_t));

                     Screen_Sub [String_Ct] = screen_blocks_used;
                     screen_blocks_used = new_size;
                    }

                //getReadType_ReadStruct (myRead, Kind_Of_Frag + String_Ct);
                Kind_Of_Frag[String_Ct] = AS_READ;
          }

      String_Start [String_Ct] = total_len;
      len = strlen (Sequence_Buffer);
      String_Info [String_Ct] . length = len;
      String_Info [String_Ct] . left_end_screened  = screen . left_end_screened;
      String_Info [String_Ct] . right_end_screened = screen . right_end_screened;
      new_len = total_len + len + 1;
      extra = new_len % (HASH_KMER_SKIP + 1);
      if  (extra > 0)
          new_len += 1 + HASH_KMER_SKIP - extra;

      if  (new_len > Data_Len)
          {
           Data_Len = (size_t) (Data_Len * MEMORY_EXPANSION_FACTOR);
           if  (new_len > Data_Len)
               Data_Len = new_len;
           //fprintf (stderr, "### reallocing  Data and Quality_Data  Data_Len = " F_SIZE_T "\n",
           //         Data_Len);
           if  (Data_Len > Extra_Data_Len)
               {
                Data = (char *) safe_realloc (Data, Data_Len);
                Extra_Data_Len = Data_Len;
               }
           Quality_Data = (char *) safe_realloc (Quality_Data, Data_Len);
           new_ref_len = Data_Len / (HASH_KMER_SKIP + 1);
           Next_Ref = (String_Ref_t *) safe_realloc
                (Next_Ref, new_ref_len * sizeof (String_Ref_t));
           memset (Next_Ref + old_ref_len, '\377',
                (new_ref_len - old_ref_len) * sizeof (String_Ref_t));
           old_ref_len = new_ref_len;
          }

      strcpy (Data + total_len, Sequence_Buffer);
      memcpy (Quality_Data + total_len, Quality_Buffer, len + 1);
      total_len = new_len;

      Put_String_In_Hash (String_Ct);

      if ((String_Ct % 100000) == 0)
        fprintf (stderr, "String_Ct:%d  totalLen:" F_S64 "  Hash_Entries:" F_S64 "  Load:%.1f%%\n",
                 String_Ct,
                 total_len,
                 Hash_Entries,
                 (100.0 * Hash_Entries) / (HASH_TABLE_SIZE * ENTRIES_PER_BUCKET));

      String_Ct ++;
     }

   if  (String_Ct == 0)
       {
         safe_free (screen . range);
        return  0;
       }

   fprintf (stderr, "strings read = %d  total_len = " F_S64 "\n",
            String_Ct, total_len);
   Used_Data_Len = total_len;

#if  SHOW_STATS
fprintf (Stat_File, "Num_Strings:\n %10ld\n", String_Ct);
fprintf (Stat_File, "total_len:\n %10ld\n", total_len);
#endif

   fprintf (stderr, "Hash_Entries = " F_S64 "  Load = %.1f%%\n",
        Hash_Entries, (100.0 * Hash_Entries) / (HASH_TABLE_SIZE * ENTRIES_PER_BUCKET));

   if  (Extra_Ref_Ct > max_extra_ref_ct)
       {
        max_extra_ref_ct *= MEMORY_EXPANSION_FACTOR;
        if  (Extra_Ref_Ct > max_extra_ref_ct)
            max_extra_ref_ct = Extra_Ref_Ct;
        fprintf (stderr,
                 "### realloc  Extra_Ref_Space  max_extra_ref_ct = " F_S64 "\n",
                 max_extra_ref_ct);
        Extra_Ref_Space = (String_Ref_t *) safe_realloc (Extra_Ref_Space,
                               max_extra_ref_ct * sizeof (String_Ref_t));
       }

#if  SHOW_STATS
fprintf (Stat_File, "Hash Table Size:\n %10ld\n",
            HASH_TABLE_SIZE * ENTRIES_PER_BUCKET);
fprintf (Stat_File, "Collisions:\n %10ld\n", Collision_Ct);
fprintf (Stat_File, "Hash entries:\n %10ld\n", Hash_Entries);
Collision_Ct = 0;
#endif

#if  SHOW_STATS
{
 int64  Ct, sub;

 Hash_Entries = 0;
 for  (i = 0;  i < HASH_TABLE_SIZE;  i ++)
   for  (j = 0;  j < Hash_Table [i] . Entry_Ct;  j ++)
     {
      Ct = 0;
      for  (ref = Hash_Table [i] . Entry [j];  ! ref . Empty;
                        ref = Next_Ref [sub])
        {
         Ct ++;
         sub = (String_Start [ref . String_Num] + ref . Offset)
                 / (HASH_KMER_SKIP + 1);
        }
      if  (Ct > 0)
          Incr_Distrib (& Kmer_Freq_Dist, Ct);
     }
}
#endif

 if  (Kmer_Skip_Path != NULL)
       Mark_Skip_Kmers ();

   // Coalesce reference chain into adjacent entries in  Extra_Ref_Space
   Extra_Ref_Ct = 0;
   for  (i = 0;  i < HASH_TABLE_SIZE;  i ++)
     for  (j = 0;  j < Hash_Table [i] . Entry_Ct;  j ++)
       {
        ref = Hash_Table [i] . Entry [j];
        if  (! getStringRefLast(ref) && ! getStringRefEmpty(ref))
            {
             Extra_Ref_Space [Extra_Ref_Ct] = ref;
             setStringRefStringNum(Hash_Table [i] . Entry [j], (Extra_Ref_Ct >> OFFSET_BITS));
             setStringRefOffset   (Hash_Table [i] . Entry [j], (Extra_Ref_Ct & OFFSET_MASK));
             Extra_Ref_Ct ++;
             do
               {
                ref = Next_Ref [(String_Start [getStringRefStringNum(ref)] + getStringRefOffset(ref)) / (HASH_KMER_SKIP + 1)];
                Extra_Ref_Space [Extra_Ref_Ct ++] = ref;
               }  while  (! getStringRefLast(ref));
            }
       }

   safe_free (screen . range);

   return  1;
  }



static int  Binomial_Bound
    (int e, double p, int Start, double Limit)

//  Return the smallest  n >= Start  s.t.
//    prob [>= e  errors in  n  binomial trials (p = error prob)]
//          > Limit

  {
   double  Normal_Z, Mu_Power, Factorial, Poisson_Coeff;
   double  q, Sum, P_Power, Q_Power, X;
   int  k, n, Bin_Coeff, Ct;

   q = 1.0 - p;
   if  (Start < e)
       Start = e;

   for  (n = Start;  n < AS_READ_MAX_NORMAL_LEN;  n ++)
     {
      if  (n <= 35)
          {
           Sum = 0.0;
           Bin_Coeff = 1;
           Ct = 0;
           P_Power = 1.0;
           Q_Power = pow (q, n);

           for  (k = 0;  k < e && 1.0 - Sum > Limit;  k ++)
             {
              X = Bin_Coeff * P_Power * Q_Power;
              Sum += X;
              Bin_Coeff *= n - Ct;
              Bin_Coeff /= ++ Ct;
              P_Power *= p;
              Q_Power /= q;
             }
           if  (1.0 - Sum > Limit)
               return  n;
          }
        else
          {
           Normal_Z = (e - 0.5 - n * p) / sqrt (n * p * q);
           if  (Normal_Z <= NORMAL_DISTRIB_THOLD)
               return  n;
#undef COMPUTE_IN_LOG_SPACE
#ifndef COMPUTE_IN_LOG_SPACE
           Sum = 0.0;
           Mu_Power = 1.0;
           Factorial = 1.0;
           Poisson_Coeff = exp (- n * p);
           for  (k = 0;  k < e;  k ++)
             {
              Sum += Mu_Power * Poisson_Coeff / Factorial;
              Mu_Power *= n * p;
              Factorial *= k + 1;
             }
#else
           Sum = 0.0;
           Mu_Power = 0.0;
           Factorial = 0.0;
           Poisson_Coeff = - n * p;
           for  (k = 0;  k < e;  k ++)
             {
	      Sum += exp(Mu_Power + Poisson_Coeff - Factorial);
              Mu_Power += log(n * p);
              Factorial = lgamma(k + 1);
             }
#endif
           if  (1.0 - Sum > Limit)
               return  n;
          }
     }

   return  AS_READ_MAX_NORMAL_LEN;
  }



static int  Binomial_Hit_Limit
    (double p, double n, double prob_bound)

//  Return  k  such that the probability of  >= k  successes in
//   n  trials is  < prob_bound , where  p  is the probability
//  of success of each trial.

  {
   double  lambda, target, sum, term, q;
   int  i;

   lambda = n * p;
   q = 1.0 - p;

   if  (lambda <= 5.0 && n >= 50.0)
       {  // use Poisson approximation
        target = (1.0 - prob_bound) * exp (lambda);
        sum = term = 1.0;
        for  (i = 1;  sum <= target && i < 50;  i ++)
          {
           term = term * lambda / (double) i;
           sum += term;
          }
        if  (sum > target)
            return  i;
       }
   if  (n >= 30.0)
       {  // use Normal approximation
        double  t, z;
        double  c [3] = {2.515517, 0.802853, 0.010328};
        double  d [4] = {1.0, 1.432788, 0.189269, 0.001308};

        if  (prob_bound <= 0.5)
            t = sqrt (-2.0 * log (prob_bound));
          else
            t = sqrt (-2.0 * log (1.0 - prob_bound));

        z = t - ((c [2] * t + c [1]) * t + c [0])
                  / (((d [3] * t + d [2]) * t + d [1]) * t + d [0]);

        if  (prob_bound <= 0.5)
            target = z;
          else
            target = -z;

        return  (int) ceil (lambda + sqrt (lambda * q) * target);
       }
     else
       {  // brute force
        target = 1.0 - prob_bound;
        sum = term = pow (q, n);
        for  (i = 1;  sum <= target && i < n;  i ++)
          {
           term *= (n + 1 - i) / i;
           term *= p / q;
           sum += term;
          }
        return  i;
       }
  }



static int  By_Diag_Sum
    (const void * a, const void * b)

//  Compare the  diag_sum  fields  in  a  and  b  as  (String_Olap_t *) 's and
//  return  -1  if  a < b ,  0  if  a == b , and  1  if  a > b .
//  Used for  qsort .

  {
   String_Olap_t  * x, * y;

   x = (String_Olap_t *) a;
   y = (String_Olap_t *) b;

   if  (x -> diag_sum < y -> diag_sum)
       return  -1;
   else if  (x -> diag_sum > y -> diag_sum)
       return  1;

   return  0;
  }



static int  By_uint32
    (const void * a, const void * b)

//  Compare the values in  a  and  b  as  (uint32 *) 's and
//  return  -1  if  a < b ,  0  if  a == b , and  1  if  a > b .
//  Used for  qsort .

  {
   uint32  * x, * y;

   x = (uint32 *) a;
   y = (uint32 *) b;

   if  ((* x) < (* y))
       return  -1;
   else if  ((* x) > (* y))
       return  1;

   return  0;
  }



static void  Capped_Increment
    (unsigned char * a, unsigned char b, unsigned char cap)

//  Set  (* a)  to the min of  a + b  and  cap .

  {
   int  sum;

   sum = (* a) + b;
   if  (sum < cap)
       (* a) = sum;
     else
       (* a) = cap;

   return;
  }



static char  char_Min
    (char a, char b)

//  Return the smaller of  a  and  b .

  {
   if  (a < b)
       return  a;

   return  b;
  }



static void  Choose_Best_Partial
    (Olap_Info_t * olap, int ct, int deleted [])

//  Choose the best partial overlap in  olap [0 .. (ct - 1)] .
//  Set the corresponding  deleted  entry to false for the others.
//  Best is the greatest number of matching bases.

  {
   double  matching_bases;
   int  i, best;

   best = 0;
   matching_bases = (1.0 - olap [0] . quality) * (2 + olap [0] . s_hi - olap [0] . s_lo
        + olap [0] . t_hi - olap [0] . t_lo);
     // actually twice the number of matching bases but the max will be
     // the same overlap

   for  (i = 1;  i < ct;  i ++)
     {
      double  mb;

      mb = (1.0 - olap [i] . quality) * (2 + olap [i] . s_hi - olap [i] . s_lo
           + olap [i] . t_hi - olap [i] . t_lo);
      if  (matching_bases < mb || (matching_bases == mb
             && olap [i] . quality < olap [best] . quality))
          best = i;
     }

   for  (i = 0;  i < ct;  i ++)
     deleted [i] = (i != best);

   return;
  }


static void  Combine_Into_One_Olap
    (Olap_Info_t olap [], int ct, int deleted [])

//  Choose the best overlap in  olap [0 .. (ct - 1)] .
//  Mark all others as deleted (by setting  deleted []  true for them)
//  and combine their information in the min/max entries in the
//  best one.

  {
   int  min_diag, max_diag;
   int  s_left_boundary, s_right_boundary;
   int  t_left_boundary, t_right_boundary;
   int  i, best;

   best = 0;
   min_diag = olap [0] . min_diag;
   max_diag = olap [0] . max_diag;
   s_left_boundary = olap [0] . s_left_boundary;
   s_right_boundary = olap [0] . s_right_boundary;
   t_left_boundary = olap [0] . t_left_boundary;
   t_right_boundary = olap [0] . t_right_boundary;

   for  (i = 1;  i < ct;  i ++)
     {
      if  (olap [i] . quality < olap [best] . quality)
          best = i;
      if  (olap [i] . min_diag < min_diag)
          min_diag = olap [i] . min_diag;
      if  (olap [i] . max_diag > max_diag)
          max_diag = olap [i] . max_diag;
      if  (olap [i] . s_left_boundary < s_left_boundary)
          s_left_boundary = olap [i] . s_left_boundary;
      if  (olap [i] . s_right_boundary > s_right_boundary)
          s_right_boundary = olap [i] . s_right_boundary;
      if  (olap [i] . t_left_boundary < t_left_boundary)
          t_left_boundary = olap [i] . t_left_boundary;
      if  (olap [i] . t_right_boundary > t_right_boundary)
          t_right_boundary = olap [i] . t_right_boundary;
     }

   olap [best] . min_diag = min_diag;
   olap [best] . max_diag = max_diag;
   olap [best] . s_left_boundary = s_left_boundary;
   olap [best] . s_right_boundary = s_right_boundary;
   olap [best] . t_left_boundary = t_left_boundary;
   olap [best] . t_right_boundary = t_right_boundary;

   for  (i = 0;  i < ct;  i ++)
     deleted [i] = (i != best);

   return;
  }





static Overlap_t  Extend_Alignment
    (Match_Node_t * Match, char * S, int S_Len, char * T, int T_Len,
     int * S_Lo, int * S_Hi, int * T_Lo, int * T_Hi, int * Errors,
     Work_Area_t * WA)

//  See how far the exact match in  Match  extends.  The match
//  refers to strings  S  and  T  with lengths  S_Len  and  T_Len ,
//  respectively.  Set  S_Lo ,  S_Hi ,  T_Lo  and  T_Hi  to the
//  regions within  S  and  T  to which the match extends.
//  Return the type of overlap:  NONE if doesn't extend to
//  the end of either fragment;  LEFT_BRANCH_PT
//  or  RIGHT_BRANCH_PT  if match extends to the end of just one fragment;
//  or  DOVETAIL  if it extends to the end of both fragments, i.e.,
//  it is a complete overlap.
//  Set  Errors  to the number of errors in the alignment if it is
//  a  DOVETAIL  overlap.

  {
   Overlap_t  return_type;
   int  S_Left_Begin, S_Right_Begin, S_Right_Len;
   int  T_Left_Begin, T_Right_Begin, T_Right_Len;
   int  Error_Limit, Left_Errors, Right_Errors, Total_Olap;
   int  i, Leftover, Right_Match_To_End, Left_Match_To_End;

   S_Left_Begin = Match -> Start - 1;
   S_Right_Begin = Match -> Start + Match -> Len;
   S_Right_Len = S_Len - S_Right_Begin;
   T_Left_Begin = Match -> Offset - 1;
   T_Right_Begin = Match -> Offset + Match -> Len;
   T_Right_Len = T_Len - T_Right_Begin;
   Total_Olap = OVL_Min_int (Match -> Start, Match -> Offset)
                     + OVL_Min_int (S_Right_Len, T_Right_Len)
                     + Match -> Len;
   Error_Limit = WA -> Error_Bound [Total_Olap];

   if  (S_Right_Len == 0 || T_Right_Len == 0)
       {
        Right_Errors = 0;
        WA -> Right_Delta_Len = 0;
        (* S_Hi) = (* T_Hi) = 0;
        Right_Match_To_End = TRUE;
       }
   else if  (S_Right_Len <= T_Right_Len)
       Right_Errors = Prefix_Edit_Dist (S + S_Right_Begin, S_Right_Len,
                          T + T_Right_Begin, T_Right_Len, Error_Limit,
                          S_Hi, T_Hi, & Right_Match_To_End, WA);
     else
       {
        Right_Errors = Prefix_Edit_Dist (T + T_Right_Begin, T_Right_Len,
                          S + S_Right_Begin, S_Right_Len, Error_Limit,
                          T_Hi, S_Hi, & Right_Match_To_End, WA);
        for  (i = 0;  i < WA -> Right_Delta_Len;  i ++)
          WA -> Right_Delta [i] *= -1;
       }

   (* S_Hi) += S_Right_Begin - 1;
   (* T_Hi) += T_Right_Begin - 1;

   assert (Right_Errors <= Error_Limit);

   if  (S_Left_Begin < 0 || T_Left_Begin < 0)
       {
        Left_Errors = 0;
        WA -> Left_Delta_Len = 0;
        (* S_Lo) = (* T_Lo) = 0;
        Leftover = 0;
        Left_Match_To_End = TRUE;
       }
   else if  (S_Right_Begin <= T_Right_Begin)
       Left_Errors = Rev_Prefix_Edit_Dist (S + S_Left_Begin,
                        S_Left_Begin + 1, T + T_Left_Begin,
                        T_Left_Begin + 1,
                        Error_Limit - Right_Errors,
                        S_Lo, T_Lo, & Leftover, & Left_Match_To_End,
                        WA);
     else
       {
        Left_Errors = Rev_Prefix_Edit_Dist (T + T_Left_Begin,
                        T_Left_Begin + 1, S + S_Left_Begin,
                        S_Left_Begin + 1,
                        Error_Limit - Right_Errors,
                        T_Lo, S_Lo, & Leftover, & Left_Match_To_End,
                        WA);
        for  (i = 0;  i < WA -> Left_Delta_Len;  i ++)
          WA -> Left_Delta [i] *= -1;
       }

   (* S_Lo) += S_Left_Begin + 1;        // Check later for branch points
   (* T_Lo) += T_Left_Begin + 1;        // Check later for branch points

   if  (! Right_Match_To_End)
       {
        if  (! Doing_Partial_Overlaps)
            WA -> Left_Delta_Len = 0;
        if  (! Left_Match_To_End)
            return_type = NONE;
          else
            return_type = RIGHT_BRANCH_PT;
       }
     else
       {
        if  (! Left_Match_To_End)
            return_type = LEFT_BRANCH_PT;
          else
            return_type = DOVETAIL;
       }

   if  (return_type == DOVETAIL || Doing_Partial_Overlaps)
       {
        (* Errors) = Left_Errors + Right_Errors;
        assert ((* Errors) <= Error_Limit);

        if  (WA -> Right_Delta_Len > 0)
            {
             if  (WA -> Right_Delta [0] > 0)
                 WA -> Left_Delta [WA -> Left_Delta_Len ++]
                      = WA -> Right_Delta [0] + Leftover + Match -> Len;
               else
                 WA -> Left_Delta [WA -> Left_Delta_Len ++]
                      = WA -> Right_Delta [0] - Leftover - Match -> Len;
            }
        for  (i = 1;  i < WA -> Right_Delta_Len;  i ++)
          WA -> Left_Delta [WA -> Left_Delta_Len ++] = WA -> Right_Delta [i];
       }

   return  return_type;
  }






static void  Find_Overlaps
    (char Frag [], int Frag_Len, char quality [],
     Int_Frag_ID_t Frag_Num, Direction_t Dir,
     Work_Area_t * WA)

//  Find and output all overlaps and branch points between string
//   Frag  and any fragment currently in the global hash table.
//   Frag_Len  is the length of  Frag  and  Frag_Num  is its ID number.
//   Dir  is the orientation of  Frag .

  {
   String_Ref_t  Ref;
   char  * P, * Window;
   uint64  Key, Next_Key;
   int64  Sub, Next_Sub, Where;
   Check_Vector_t  This_Check, Next_Check;
   int  Offset, Shift, Next_Shift;
   int  hi_hits;
   int  screen_sub, screen_lo, screen_hi;
   int  j;

   memset (WA -> String_Olap_Space, 0, STRING_OLAP_MODULUS * sizeof (String_Olap_t));
   WA -> Next_Avail_String_Olap = STRING_OLAP_MODULUS;
   WA -> Next_Avail_Match_Node = 1;

   assert (Frag_Len >= Kmer_Len);

   Offset = 0;
   P = Window = Frag;
   screen_sub = 0;
   if  (WA -> screen_info . num_matches == 0)
       screen_lo = screen_hi = INT_MAX;
     else
       {
        screen_lo = WA -> screen_info . range [0] . bgn;
        screen_hi = WA -> screen_info . range [0] . end;
       }

   Key = 0;
   for  (j = 0;  j < Kmer_Len;  j ++)
     Key |= (uint64) (Bit_Equivalent [(int) * (P ++)]) << (2 * j);

   Sub = HASH_FUNCTION (Key);
   Shift = HASH_CHECK_FUNCTION (Key);
   Next_Key = (Key >> 2);
   Next_Key |= ((uint64)
                 (Bit_Equivalent [(int) * P])) << (2 * (Kmer_Len - 1));
   Next_Sub = HASH_FUNCTION (Next_Key);
   Next_Shift = HASH_CHECK_FUNCTION (Next_Key);
   Next_Check = Hash_Check_Array [Next_Sub];

   if  ((Hash_Check_Array [Sub] & (((Check_Vector_t) 1) << Shift)) != 0
          && Offset <= screen_lo - Kmer_Len + WINDOW_SCREEN_OLAP)
       {
        Ref = Hash_Find (Key, Sub, Window, & Where, & hi_hits);
        if  (hi_hits)
            WA -> screen_info . left_end_screened = TRUE;
        if  (! getStringRefEmpty(Ref))
          {
           while  (TRUE)
             {
#if  SHOW_STATS
Match_Ct ++;
#endif
              if  (Contig_Mode
                   || Frag_Num < (int) getStringRefStringNum(Ref) + Hash_String_Num_Offset)
                  Add_Ref  (Ref, Offset, WA);

              if  (getStringRefLast(Ref))
                  break;
                else
                  {
                   Ref = Extra_Ref_Space [++ Where];
                   assert (! getStringRefEmpty(Ref));
                  }
             }
          }
       }

   while  ((* P) != '\0')
     {
      if  (Offset == screen_hi - 1 - WINDOW_SCREEN_OLAP)
          {
           if  (WA -> screen_info . range [screen_sub] . last)
               screen_lo = screen_hi = INT_MAX;
             else
               {
                screen_sub ++;
                screen_lo = WA -> screen_info . range [screen_sub] . bgn;
                screen_hi = WA -> screen_info . range [screen_sub] . end;
               }
          }

      Window ++;
      Offset ++;

      Key = Next_Key;
      Shift = Next_Shift;
      Sub = Next_Sub;
      This_Check = Next_Check;
      P ++;
      Next_Key = (Key >> 2);
      Next_Key |= ((uint64)
                    (Bit_Equivalent [(int) * P])) << (2 * (Kmer_Len - 1));
      Next_Sub = HASH_FUNCTION (Next_Key);
      Next_Shift = HASH_CHECK_FUNCTION (Next_Key);
      Next_Check = Hash_Check_Array [Next_Sub];

      if  ((This_Check & (((Check_Vector_t) 1) << Shift)) != 0
             && Offset <= screen_lo - Kmer_Len + WINDOW_SCREEN_OLAP)
          {
           Ref = Hash_Find (Key, Sub, Window, & Where, & hi_hits);
           if  (hi_hits)
               {
                if  (Offset < HOPELESS_MATCH)
                    WA -> screen_info . left_end_screened = TRUE;
                if  (Frag_Len - Offset - Kmer_Len + 1 < HOPELESS_MATCH)
                    WA -> screen_info . right_end_screened = TRUE;
               }
           if  (! getStringRefEmpty(Ref))
             {
              while  (TRUE)
                {
#if  SHOW_STATS
Match_Ct ++;
#endif
                 if  (Contig_Mode
                      || Frag_Num < (int) getStringRefStringNum(Ref) + Hash_String_Num_Offset)
                     Add_Ref  (Ref, Offset, WA);

                 if  (getStringRefLast(Ref))
                     break;
                   else
                     {
                      Ref = Extra_Ref_Space [++ Where];
                      assert (! getStringRefEmpty(Ref));
                     }
                }
             }
          }
     }

#if  SHOW_STATS
{
 int32  i, Ct = 0;

 for  (i = 0;  i < WA -> Next_Avail_String_Olap;  i ++)
   if  (WA -> String_Olap_Space [i] . Full)
       Ct ++;

 Incr_Distrib (& String_Olap_Dist, Ct);
}
#endif

#if  0
fprintf (stderr, "Frag %6d %c has %6d matches\n",
         Frag_Num, Dir == FORWARD ? 'f' : 'r', WA -> Next_Avail_Match_Node - 1);
#endif

   Process_String_Olaps  (Frag, Frag_Len, quality, Frag_Num, Dir, WA);

   return;
  }



static void  Flip_Screen_Range
    (Screen_Info_t * screen, int len)

//  Change the  range  entries in  (* screen)  to reflect that
//  the current fragment has been reverse-complemented.
//  len  is the length of the fragment.  Also reverse the
//  order of the entries.

  {
   int  i, j, k;
   Screen_Range_t  save;

   screen -> left_end_screened = FALSE;
   screen -> right_end_screened = FALSE;

   if  (screen -> num_matches < 1)
       return;

   for  (i = 0, j = screen -> num_matches - 1;  i < j;  i ++, j --)
     {
      save = screen -> range [j];
      screen -> range [j] . bgn = len - screen -> range [i] . end;
      screen -> range [j] . end = len - screen -> range [i] . bgn;
      screen -> range [i] . bgn = len - save . end;
      screen -> range [i] . end = len - save . bgn;
     }

   if  (i == j)
       {
        k = screen -> range [i] . end;
        screen -> range [i] . end = len - screen -> range [i] . bgn;
        screen -> range [i] . bgn = len - k;
       }

   screen -> left_end_screened
       = screen -> range [0] . bgn < HOPELESS_MATCH;
   screen -> right_end_screened
       = screen -> range [screen -> num_matches - 1] . end
                > len - HOPELESS_MATCH;

   return;
  }

// Checkered mask. cmask<uint16_t, 1> is every other bit on
// (0x55). cmask<uint16_t,2> is two bits one, two bits off (0x33). Etc.
template<typename U, int len, int l = sizeof(U) * 8 / (2 * len)>
struct cmask {
  static const U v =
    (cmask<U, len, l - 1>::v << (2 * len)) | (((U)1 << len) - 1);
};
template<typename U, int len>
struct cmask<U, len, 0> {
  static const U v = 0;
};

static uint64 word_reverse(uint64 w) {
  typedef uint64 U;
  w = ((w >> 2)  & cmask<U, 2 >::v) | ((w & cmask<U, 2 >::v) << 2);
  w = ((w >> 4)  & cmask<U, 4 >::v) | ((w & cmask<U, 4 >::v) << 4);
  w = ((w >> 8)  & cmask<U, 8 >::v) | ((w & cmask<U, 8 >::v) << 8);
  w = ((w >> 16) & cmask<U, 16>::v) | ((w & cmask<U, 16>::v) << 16);
  w = ( w >> 32                   ) | ( w                    << 32);
  return w;
}

// Wrapper around Hash_Find__. First look up in the bloom filter for a
// skip mer. If found, return an empty ref and set hi_hits to true. If
// not found, delegate to Hash_Find__.
static String_Ref_t Hash_Find(uint64 Key, int64 Sub, char * S, int64 * Where, int * hi_hits)
{
  static jellyfish::mer_dna m;
  String_Ref_t H_Ref = { 0 };
  uint64 rKey = word_reverse(Key) >> (2 * (32 - Kmer_Len));
  uint64 rcKey = ~Key;
  if(Kmer_Len < 32)
    rcKey &= ((uint64_t)1 << (2 * Kmer_Len)) - 1;

  // uint64 rKey = ((uint64_t)-1 - word_reverse(Key)) >> (2 * (32 - Kmer_Len));

  //  m.word__(0) = rKey;
  //  printf("%s\n", m.to_str().c_str());

  if(skip_mers) {
    if(rKey < rcKey) {
      m.word__(0) = rKey;
    } else {
      m.word__(0) = rcKey;
    }

    //    fprintf(stderr, "Check mer %s\n", m.to_str().c_str());
    if(skip_mers->check(m)) {
      //      fprintf(stderr, "Skipping mer %s\n", m.to_str().c_str());
      setStringRefEmpty(H_Ref, TRUE);
      *hi_hits = TRUE;
      return H_Ref;
    }
  }
  H_Ref = Hash_Find__(Key, Sub, S, Where, hi_hits);
  return H_Ref;
}

// Original Hash_Find, looking up in the actual hash
static String_Ref_t  Hash_Find__
    (uint64 Key, int64 Sub, char * S, int64 * Where, int * hi_hits)

//  Search for string  S  with hash key  Key  in the global
//  Hash_Table  starting at subscript  Sub .  Return the matching
//  reference in the hash table if there is one, or else a reference
//  with the  Empty bit set true.  Set  (* Where)  to the subscript in
//  Extra_Ref_Space  where the reference was found if it was found there.
//  Set  (* hi_hits)  to  TRUE  if hash table entry is found but is empty
//  because it was screened out, otherwise set to FALSE.

  {
    String_Ref_t  H_Ref = { 0 };
   char  * T;
   unsigned char  Key_Check;
   int64  Ct, Probe;
   int  i;

#if  SHOW_STATS
Hash_Find_Ct ++;
#endif
   Key_Check = KEY_CHECK_FUNCTION (Key);
   Probe = PROBE_FUNCTION (Key);

   (* hi_hits) = FALSE;
   Ct = 0;
   do
     {
      for  (i = 0;  i < Hash_Table [Sub] . Entry_Ct;  i ++)
        if  (Hash_Table [Sub] . Check [i] == Key_Check)
            {
             int  is_empty;

             H_Ref = Hash_Table [Sub] . Entry [i];
             is_empty = getStringRefEmpty(H_Ref);
             if  (! getStringRefLast(H_Ref) && ! is_empty)
                 {
                   (* Where) = ((uint64)getStringRefStringNum(H_Ref) << OFFSET_BITS) + getStringRefOffset(H_Ref);
                  H_Ref = Extra_Ref_Space [(* Where)];
                 }
             T = Data + String_Start [getStringRefStringNum(H_Ref)] + getStringRefOffset(H_Ref);
             if  (strncmp (S, T, Kmer_Len) == 0)
                 {
                  if  (is_empty)
                      {
                       setStringRefEmpty(H_Ref, TRUE);
                       (* hi_hits) = TRUE;
                      }
                  return  H_Ref;
                 }
            }
      if  (Hash_Table [Sub] . Entry_Ct < ENTRIES_PER_BUCKET)
          {
           setStringRefEmpty(H_Ref, TRUE);
           return  H_Ref;
          }
#if  SHOW_STATS
Collision_Ct ++;
#endif
      Sub = (Sub + Probe) % HASH_TABLE_SIZE;
     }  while  (++ Ct < HASH_TABLE_SIZE);

   setStringRefEmpty(H_Ref, TRUE);
   return  H_Ref;
  }




static int  Has_Bad_Window
    (char a [], int n, int window_len, int threshold)

//  Return  TRUE  iff there is any length-( window_len ) subarray
//  of  a [0 .. (n-1)]  that sums to  threshold  or higher.

  {
   int  i, j, sum;

   if  (n < window_len)
       return  FALSE;

   sum = 0;
   for  (i = 0;  i < window_len;  i ++)
     sum += a [i];

   j = 0;
   if  (sum >= threshold)
       {
        return  TRUE;
       }

   while  (i < n)
     {
      sum -= a [j ++];
      sum += a [i ++];
      if  (sum >= threshold)
          {
           return  TRUE;
          }
     }

   return  FALSE;
  }



static void  Hash_Insert
    (String_Ref_t Ref, uint64 Key, char * S)

//  Insert  Ref  with hash key  Key  into global  Hash_Table .
//  Ref  represents string  S .

  {
   String_Ref_t  H_Ref;
   char  * T;
   int  Shift;
   unsigned char  Key_Check;
   int64  Ct, Probe, Sub;
   int  i;

   Sub = HASH_FUNCTION (Key);
   Shift = HASH_CHECK_FUNCTION (Key);
   Hash_Check_Array [Sub] |= (((Check_Vector_t) 1) << Shift);
   Key_Check = KEY_CHECK_FUNCTION (Key);
   Probe = PROBE_FUNCTION (Key);

   Ct = 0;
   do
     {
      for  (i = 0;  i < Hash_Table [Sub] . Entry_Ct;  i ++)
        if  (Hash_Table [Sub] . Check [i] == Key_Check)
            {
             H_Ref = Hash_Table [Sub] . Entry [i];
             T = Data + String_Start [getStringRefStringNum(H_Ref)] + getStringRefOffset(H_Ref);
             if  (strncmp (S, T, Kmer_Len) == 0)
                 {
                  if  (getStringRefLast(H_Ref)) {
                      Extra_Ref_Ct ++;
                  }
                  Next_Ref [(String_Start [getStringRefStringNum(Ref)] + getStringRefOffset(Ref))
                              / (HASH_KMER_SKIP + 1)]
                                = H_Ref;
                  Extra_Ref_Ct ++;
                  setStringRefLast(Ref, FALSE);
                  Hash_Table [Sub] . Entry [i] = Ref;

                  if  (Hash_Table [Sub] . Hits [i] < HIGHEST_KMER_LIMIT)
                      Hash_Table [Sub] . Hits [i] ++;

                  return;
                 }
            }
if  (i != Hash_Table [Sub] . Entry_Ct)
    {
     fprintf (stderr, "i = %d  Sub = " F_S64 "  Entry_Ct = %d\n",
                  i, Sub, Hash_Table [Sub] . Entry_Ct);
    }
      assert (i == Hash_Table [Sub] . Entry_Ct);
      if  (Hash_Table [Sub] . Entry_Ct < ENTRIES_PER_BUCKET)
          {
           setStringRefLast(Ref, TRUE);
           Hash_Table [Sub] . Entry [i] = Ref;
           Hash_Table [Sub] . Check [i] = Key_Check;
           Hash_Table [Sub] . Entry_Ct ++;
           Hash_Entries ++;
           Hash_Table [Sub] . Hits [i] = 1;
           return;
          }
#if  SHOW_STATS
Collision_Ct ++;
#endif
      Sub = (Sub + Probe) % HASH_TABLE_SIZE;
     }  while  (++ Ct < HASH_TABLE_SIZE);

   fprintf (stderr, "ERROR:  Hash table full\n");
   assert (FALSE);

   return;
  }



static void  Hash_Mark_Empty
    (uint64 key, char * s)

//  Set the  empty  bit to true for the hash table entry
//  corresponding to string  s  whose hash key is  key .
//  Also set global  String_Info . left/right_end_screened
//  true if the entry occurs near the left/right end, resp.,
//  of the string in the hash table.  If not found, add an
//  entry to the hash table and mark it empty.

  {
   String_Ref_t  h_ref;
   char  * t;
   unsigned char  key_check;
   int64  ct, probe;
   int64  sub;
   int  i, shift;

   sub = HASH_FUNCTION (key);
   key_check = KEY_CHECK_FUNCTION (key);
   probe = PROBE_FUNCTION (key);

   ct = 0;
   do
     {
      for  (i = 0;  i < Hash_Table [sub] . Entry_Ct;  i ++)
        if  (Hash_Table [sub] . Check [i] == key_check)
            {
             h_ref = Hash_Table [sub] . Entry [i];
             t = Data + String_Start [getStringRefStringNum(h_ref)] + getStringRefOffset(h_ref);
             if  (strncmp (s, t, Kmer_Len) == 0)
                 {
                   if  (! getStringRefEmpty(Hash_Table [sub] . Entry [i]))
                      Mark_Screened_Ends_Chain (Hash_Table [sub] . Entry [i]);
                  setStringRefEmpty(Hash_Table [sub] . Entry [i], TRUE);
                  return;
                 }
            }
      assert (i == Hash_Table [sub] . Entry_Ct);
      if  (Hash_Table [sub] . Entry_Ct < ENTRIES_PER_BUCKET)
          {  // Not found
           if  (Use_Hopeless_Check)
               {
                Hash_Table [sub] . Entry [i] = Add_Extra_Hash_String (s);
                setStringRefEmpty(Hash_Table [sub] . Entry [i], TRUE);
                Hash_Table [sub] . Check [i] = key_check;
                Hash_Table [sub] . Entry_Ct ++;
                Hash_Table [sub] . Hits [i] = 0;
                Hash_Entries ++;
                shift = HASH_CHECK_FUNCTION (key);
                Hash_Check_Array [sub] |= (((Check_Vector_t) 1) << shift);
               }
           return;
          }
      sub = (sub + probe) % HASH_TABLE_SIZE;
     }  while  (++ ct < HASH_TABLE_SIZE);

   fprintf (stderr, "ERROR:  Hash table full\n");
   assert (FALSE);

   return;
  }



static void  Initialize_Globals
    (void)

//  Set initial values of global variables.

  {
   int  e, i, Start;

   for  (i = 0;  i <= ERRORS_FOR_FREE;  i ++)
     Read_Edit_Match_Limit [i] = 0;

   Start = 1;
   for  (e = ERRORS_FOR_FREE + 1;  e < MAX_ERRORS;  e ++)
     {
      Start = Binomial_Bound (e - ERRORS_FOR_FREE, AS_READ_ERROR_RATE,
                  Start, EDIT_DIST_PROB_BOUND);
      Read_Edit_Match_Limit [e] = Start - 1;
      assert (Read_Edit_Match_Limit [e] >= Read_Edit_Match_Limit [e - 1]);
     }

   for  (i = 0;  i <= ERRORS_FOR_FREE;  i ++)
     Guide_Edit_Match_Limit [i] = 0;

   Start = 1;
   for  (e = ERRORS_FOR_FREE + 1;  e < MAX_ERRORS;  e ++)
     {
      Start = Binomial_Bound (e - ERRORS_FOR_FREE, AS_GUIDE_ERROR_RATE,
                  Start, EDIT_DIST_PROB_BOUND);
      Guide_Edit_Match_Limit [e] = Start - 1;
      assert (Guide_Edit_Match_Limit [e] >= Guide_Edit_Match_Limit [e - 1]);
     }

   for  (i = 0;  i <= AS_READ_MAX_NORMAL_LEN;  i ++)
     Read_Error_Bound [i] = (int) (i * AS_READ_ERROR_RATE + 0.0000000000001);

   for  (i = 0;  i <= AS_READ_MAX_NORMAL_LEN;  i ++)
     Guide_Error_Bound [i] = (int) (i * AS_GUIDE_ERROR_RATE + 0.0000000000001);

   for  (i = 0;  i <= AS_READ_MAX_NORMAL_LEN;  i ++)
     Branch_Cost [i] = i * Branch_Match_Value + Branch_Error_Value;

   Bit_Equivalent ['a'] = Bit_Equivalent ['A'] = 0;
   Bit_Equivalent ['c'] = Bit_Equivalent ['C'] = 1;
   Bit_Equivalent ['g'] = Bit_Equivalent ['G'] = 2;
   Bit_Equivalent ['t'] = Bit_Equivalent ['T'] = 3;

   for  (i = 0;  i < 256;  i ++)
     {
      char  ch = tolower ((char) i);

      if  (ch == 'a' || ch == 'c' || ch == 'g' || ch == 't')
          Char_Is_Bad [i] = 0;
        else
          Char_Is_Bad [i] = 1;
     }

   Screen_Space_Size = Max_Hash_Strings;
   Screen_Space
       = (Screen_Range_t *) safe_malloc
             (Screen_Space_Size * sizeof (Screen_Range_t));

   Hash_Table = (Hash_Bucket_t *) safe_malloc
                    (HASH_TABLE_SIZE * sizeof (Hash_Bucket_t));
   fprintf (stderr, "### Bytes in hash table = " F_SIZE_T "\n",
            HASH_TABLE_SIZE * sizeof (Hash_Bucket_t));

   Hash_Check_Array = (Check_Vector_t *) safe_malloc
                          (HASH_TABLE_SIZE * sizeof (Check_Vector_t));
   Loc_ID = (uint64 *) safe_calloc (Max_Hash_Strings, sizeof (uint64));
   String_Info = (Hash_Frag_Info_t *) safe_calloc (Max_Hash_Strings,
                     sizeof (Hash_Frag_Info_t));
   String_Start = (int64 *) safe_calloc (Max_Hash_Strings, sizeof (int64));
   String_Start_Size = Max_Hash_Strings;
   Kind_Of_Frag = (FragType *) safe_calloc (Max_Hash_Strings, sizeof (FragType));
   Screen_Sub = (int *) safe_calloc (Max_Hash_Strings, sizeof (int));

#if  SHOW_STATS
 Init_Distrib (& Olap_Len_Dist, 14);
 {
   int  i;
   for  (i = 0;  i < 14;  i ++)
     Olap_Len_Dist . Thold [i] = 50.0 * (i + 1);
 }
 Init_Distrib (& Num_Olaps_Dist, 25);
 {
   int  i;
   for  (i = 0;  i <= 10;  i ++)
     Num_Olaps_Dist . Thold [i] = 1.0 * i;
   for  (i = 11;  i <= 18;  i ++)
     Num_Olaps_Dist . Thold [i] = 5.0 * (i - 8);
   for  (i = 19;  i <= 23;  i ++)
     Num_Olaps_Dist . Thold [i] = 100.0 * (i - 18);
   Num_Olaps_Dist . Thold [24] = 1000.0;
 }
 Init_Distrib (& Kmer_Freq_Dist, 25);
 {
   int  i;
   for  (i = 0;  i <= 10;  i ++)
     Kmer_Freq_Dist . Thold [i] = 1.0 * i;
   for  (i = 11;  i <= 18;  i ++)
     Kmer_Freq_Dist . Thold [i] = 5.0 * (i - 8);
   for  (i = 19;  i <= 23;  i ++)
     Kmer_Freq_Dist . Thold [i] = 100.0 * (i - 18);
   Kmer_Freq_Dist . Thold [24] = 1000.0;
 }
 Init_Distrib (& Kmer_Hits_Dist, 21);
 {
   int  i;
   for  (i = 0;  i <= 20;  i ++)
     Kmer_Hits_Dist . Thold [i] = 100.0 * i;
 }
 Init_Distrib (& String_Olap_Dist, 28);
 {
   int  i;
   for  (i = 0;  i <= 10;  i ++)
     String_Olap_Dist . Thold [i] = 1.0 * i;
   for  (i = 11;  i <= 18;  i ++)
     String_Olap_Dist . Thold [i] = 5.0 * (i - 8);
   for  (i = 19;  i <= 27;  i ++)
     String_Olap_Dist . Thold [i] = 50.0 * (i - 17);
 }
 Init_Distrib (& Hits_Per_Olap_Dist, 28);
 {
   int  i;
   for  (i = 0;  i <= 10;  i ++)
     Hits_Per_Olap_Dist . Thold [i] = 1.0 * i;
   for  (i = 11;  i <= 18;  i ++)
     Hits_Per_Olap_Dist . Thold [i] = 5.0 * (i - 8);
   for  (i = 19;  i <= 27;  i ++)
     Hits_Per_Olap_Dist . Thold [i] = 50.0 * (i - 17);
 }
Init_Distrib (& Diag_Dist, 21);
{
 int  i;
 for  (i = 0;  i <= 20;  i ++)
   Diag_Dist . Thold [i] = 1.0 * i;
}
Init_Distrib (& Gap_Dist, 21);
{
 int  i;
 for  (i = 0;  i <= 20;  i ++)
   Gap_Dist . Thold [i] = 1.0 * i;
}
Init_Distrib (& Exacts_Per_Olap_Dist, 21);
{
 int  i;
 for  (i = 0;  i <= 20;  i ++)
   Exacts_Per_Olap_Dist . Thold [i] = 1.0 * i;
}
Init_Distrib (& Edit_Depth_Dist, 26);
{
 int  i;
 for  (i = 0;  i <= 25;  i ++)
   Edit_Depth_Dist . Thold [i] = 1.0 * i;
}
#endif

   return;
  }



void  Initialize_Work_Area
    (Work_Area_t * WA, int id)

//  Allocate memory for  (* WA)  and set initial values.
//  Set  thread_id  field to  id .

  {
   int  i, Offset, Del;

   WA -> Left_Delta  = (int *)safe_malloc (MAX_ERRORS * sizeof (int));
   WA -> Right_Delta = (int *)safe_malloc (MAX_ERRORS * sizeof (int));

   WA -> Edit_Space = (int *)safe_malloc ((MAX_ERRORS + 4) * MAX_ERRORS * sizeof (int));
   WA -> Edit_Array = (int **)safe_malloc (MAX_ERRORS * sizeof (int *));

   WA -> String_Olap_Size = INIT_STRING_OLAP_SIZE;
   WA -> String_Olap_Space = (String_Olap_t *) safe_malloc (WA -> String_Olap_Size * sizeof (String_Olap_t));
   WA -> Match_Node_Size = INIT_MATCH_NODE_SIZE;
   WA -> Match_Node_Space = (Match_Node_t *) safe_malloc (WA -> Match_Node_Size * sizeof (Match_Node_t));

   fprintf(stderr, "Initialize_Work_Area:  MAX_ERRORS = %d\n", MAX_ERRORS);

   Offset = 2;
   Del = 6;
   for  (i = 0;  i < MAX_ERRORS;  i ++)
     {
       WA -> Edit_Array [i] = WA -> Edit_Space + Offset;
       Offset += Del;
       Del += 2;
     }

   WA -> status = 0;

   WA -> screen_info . range = (Screen_Range_t *) safe_malloc (INIT_SCREEN_MATCHES * sizeof (Screen_Range_t));
   WA -> screen_info . match_len = INIT_SCREEN_MATCHES;
   WA -> screen_info . num_matches = 0;

   WA -> thread_id = id;

   //  OVSoverlap is 16 bytes, so 1MB of data would store 65536
   //  overlaps.
   //
   WA->overlapsLen = 0;
   WA->overlapsMax = 1024 * 1024 / sizeof(OVSoverlap);
   WA->overlaps    = (OVSoverlap *)safe_malloc(sizeof(OVSoverlap) * WA->overlapsMax);

   return;
  }



static int  Interval_Intersection
    (int a, int b, int c, int d)

//  Return the number of bases by which the closed interval  [a, b]
//  intersects the closed interval  [c, d] .

  {
   if  (d < a || b < c)
       return  0;

   return  1 + OVL_Min_int (b, d) - OVL_Max_int (a, c);
  }



static int  Lies_On_Alignment
    (int start, int offset, int s_lo, int t_lo, Work_Area_t * WA)

//  Return  TRUE  iff the exact match region beginning at
//  position  start  in the first string and  offset  in
//  the second string lies along the alignment from
//   lo .. hi  on the first string where the delta-encoding
//  of the alignment is given by
//   WA -> Left_Delta [0 .. (WA -> Left_Delta_Len-1)] .

  {
   int  i, diag, new_diag;

   diag = t_lo - s_lo;
   new_diag = offset - start;

   for  (i = 0;  i < WA -> Left_Delta_Len;  i ++)
     {
      s_lo += abs (WA -> Left_Delta [i]);
      if  (start < s_lo)
          return  (abs (new_diag - diag) <= SHIFT_SLACK);
      if  (WA -> Left_Delta [i] < 0)
          diag ++;
        else
          {
           s_lo ++;
           diag --;
          }
     }

   return  (abs (new_diag - diag) <= SHIFT_SLACK);
  }


static void  Mark_Screened_Ends_Chain
    (String_Ref_t ref)

//  Mark  left/right_end_screened in global  String_Info  for
//   ref  and everything in its list, if they occur near
//  enough to the end of the string.

  {
   Mark_Screened_Ends_Single (ref);

   while  (! getStringRefLast(ref))
       {
         ref = Next_Ref [(String_Start [getStringRefStringNum(ref)] + getStringRefOffset(ref))
                          / (HASH_KMER_SKIP + 1)];
        Mark_Screened_Ends_Single (ref);
       }

   return;
  }



static void  Mark_Screened_Ends_Single
    (String_Ref_t ref)

//  Mark  left/right_end_screened in global  String_Info  for
//  single reference  ref , if it occurs near
//  enough to the end of its string.

  {
   int  s_num, len;

   s_num = getStringRefStringNum(ref);
   len = String_Info [s_num] . length;

   if  (getStringRefOffset(ref) < HOPELESS_MATCH)
       String_Info [s_num] . left_end_screened = TRUE;
   if  (len - getStringRefOffset(ref) - Kmer_Len + 1 < HOPELESS_MATCH)
       String_Info [s_num] . right_end_screened = TRUE;

   return;
  }

static void Mark_Skip_Kmers(void) {
  // Load the bloom filter containing skip k-mers
  std::ifstream in(Kmer_Skip_Path);
  jellyfish::file_header header(in);
  if(!in.good()) {
    fprintf(stderr, "Failed to open and parse skip k-mer file '%s'.\n",
            Kmer_Skip_Path);
    exit(1);
  }
  /* if(header.format() != "bloomcounter") { */
  /*   fprintf(stderr, "Unexpected skip k-mer file format '%s'. Should be 'bloomcounter'.\n", */
  /*           header.format().c_str()); */
  /*   exit(1); */
  /* } */
  if(header.format() != binary_dumper::format) {
    fprintf(stderr, "Unexpected skip k-mer file format '%s'. Should be '%s'.\n",
            header.format().c_str(), binary_dumper::format);
    exit(1);
  }
  if(Kmer_Len != header.key_len() / 2) {
    fprintf(stderr, "k-mer length in skip k-mer file (%d) different than -k parameter (%d).\n",
            (int)header.key_len() / 2, (int)Kmer_Len);
    exit(1);
  }
  jellyfish::mer_dna::k(Kmer_Len);
  /* jellyfish::hash_pair<jellyfish::mer_dna> fns(header.matrix(1), header.matrix(2)); */
  /* skip_mers = new jellyfish::mer_dna_bloom_counter(header.size(), header.nb_hashes(), in, fns); */
  skip_mers_file = new jellyfish::mapped_file(Kmer_Skip_Path);
  skip_mers      = new binary_query(skip_mers_file->base() + header.offset(), header.key_len(), header.counter_len(),
                                    header.matrix(), header.size() - 1, skip_mers_file->length() - header.offset());
}

/* static void  Mark_Skip_Kmers */
/*     (void) */

/* //  Set  Empty  bit true for all entries in global  Hash_Table */
/* //  that match a kmer in file  Kmer_Skip_File . */
/* //  Add the entry (and then mark it empty) if it's not in  Hash_Table. */

/*   { */
/*    uint64  key; */
/*    char  line [MAX_LINE_LEN]; */
/*    int  ct = 0; */

/*    rewind (Kmer_Skip_File); */

/*    while  (fgets (line, MAX_LINE_LEN, Kmer_Skip_File) != NULL) */
/*      { */
/*       int  i, len; */

/*       ct ++; */
/*       len = strlen (line) - 1; */
/*       if  (line [0] != '>' || line [len] != '\n') */
/*           { */
/*            fprintf (stderr, "ERROR:  Bad line %d in kmer skip file\n", ct); */
/*            fputs (line, stderr); */
/*            //AZ */
/* 	   continue; */
/* 	   //exit (1); */
/*           } */

/*       if  (fgets (line, MAX_LINE_LEN, Kmer_Skip_File) == NULL) */
/*           { */
/*            fprintf (stderr, "ERROR:  Bad line after %d in kmer skip file\n", ct); */
/* 	   //AZ */
/* 	continue; */
/*            //exit (1); */
/*           } */
/*       ct ++; */
/*       len = strlen (line) - 1; */
/*       if  (len != Kmer_Len || line [len] != '\n') */
/*           { */
/*            fprintf (stderr, "ERROR:  Bad line %d in kmer skip file\n", ct); */
/*            fputs (line, stderr); */
/*            //AZ */
/* 	   continue; */
/* 	   //exit (1); */
/*           } */
/*       line [len] = '\0'; */

/*       key = 0; */
/*       for  (i = 0;  i < len;  i ++) */
/*         { */
/*          line [i] = tolower (line [i]); */
/*          key |= (uint64) (Bit_Equivalent [(int) line [i]]) << (2 * i); */
/*         } */
/*       Hash_Mark_Empty (key, line); */

/*       reverseComplementSequence (line, len); */
/*       key = 0; */
/*       for  (i = 0;  i < len;  i ++) */
/*         key |= (uint64) (Bit_Equivalent [(int) line [i]]) << (2 * i); */
/*       Hash_Mark_Empty (key, line); */
/*      } */

/*    fprintf (stderr, "String_Ct = %d  Extra_String_Ct = %d  Extra_String_Subcount = %d\n", */
/*         String_Ct, Extra_String_Ct, Extra_String_Subcount); */
/*    fprintf (stderr, "Read %d kmers to mark to skip\n", ct / 2); */

/*    return; */
/*   } */



static void  Merge_Intersecting_Olaps
    (Olap_Info_t p [], int ct, int deleted [])

//  Combine overlaps whose overlap regions intersect sufficiently
//  in  p [0 .. (ct - 1)]  by marking the poorer quality one
//  deleted (by setting  deleted []  true for it) and combining
//  its min/max info in the other.  Assume all entries in
//  deleted are 0 initially.

  {
   int  i, j, lo_diag, hi_diag;

   for  (i = 0;  i < ct - 1;  i ++)
     for  (j = i + 1;  j < ct;  j ++)
       {
        if  (deleted [i] || deleted [j])
            continue;
        lo_diag = p [i] . min_diag;
        hi_diag = p [i] . max_diag;
        if  ((lo_diag <= 0 && p [j] . min_diag > 0)
                 || (lo_diag > 0 && p [j] . min_diag <= 0))
            continue;
        if  ((lo_diag >= 0
               && p [j] .  t_right_boundary - lo_diag
                      - p [j] . s_left_boundary
                          >= MIN_INTERSECTION)
             || (lo_diag <= 0
                   && p [j] .  s_right_boundary + lo_diag
                          - p [j] .  t_left_boundary
                              >= MIN_INTERSECTION)
             || (hi_diag >= 0
                   && p [j] .  t_right_boundary - hi_diag
                          - p [j] . s_left_boundary
                              >= MIN_INTERSECTION)
             || (hi_diag <= 0
                   && p [j] .  s_right_boundary + hi_diag
                          - p [j] .  t_left_boundary
                              >= MIN_INTERSECTION))
            {
             Olap_Info_t  * discard, * keep;

             if  (p [i] . quality < p [j] . quality)
                 {
                  keep = p + i;
                  discard = p + j;
                  deleted [j] = TRUE;
                 }
               else
                 {
                  keep = p + j;
                  discard = p + i;
                  deleted [i] = TRUE;
                 }
             if  (discard -> min_diag < keep -> min_diag)
                 keep -> min_diag = discard -> min_diag;
             if  (discard -> max_diag > keep -> max_diag)
                 keep -> max_diag = discard -> max_diag;
             if  (discard -> s_left_boundary < keep -> s_left_boundary)
                 keep -> s_left_boundary = discard -> s_left_boundary;
             if  (discard -> s_right_boundary > keep -> s_right_boundary)
                 keep -> s_right_boundary = discard -> s_right_boundary;
             if  (discard -> t_left_boundary < keep -> t_left_boundary)
                 keep -> t_left_boundary = discard -> t_left_boundary;
             if  (discard -> t_right_boundary > keep -> t_right_boundary)
                 keep -> t_right_boundary = discard -> t_right_boundary;
            }
       }

   return;
  }



static int64  Next_Odd_Prime
    (int64 N)

//  Return the first odd prime  >= N .

  {
   int64  Div, Last;

   if  (N % 2 == 0)
       N ++;
   while  (TRUE)
     {
      Last = (int64) (sqrt ((double) N));
      for  (Div = 3;  Div <= Last;  Div += 2)
        if  (N % Div == 0)
            break;
      if  (Div > Last)
          return  N;
      N += 2;
     }

   return  0;     // Just to make the compiler happy
  }






static void  Output_Overlap
    (Int_Frag_ID_t S_ID, int S_Len, Direction_t S_Dir,
     Int_Frag_ID_t T_ID, int T_Len, Olap_Info_t * olap,
     Work_Area_t *WA)

//  Output the overlap between strings  S_ID  and  T_ID  which
//  have lengths  S_Len  and  T_Len , respectively.
//  The overlap information is in  (* olap) .
//  S_Dir  indicates the orientation of  S .

  {
   int  S_Right_Hang, T_Right_Hang;
   int  this_diag;
   OverlapMesg ovMesg;
   GenericMesg outputMesg;
   signed char deltas[2 * AS_READ_MAX_NORMAL_LEN];
   signed char *deltaCursor = deltas;
   outputMesg.m = &ovMesg;
   outputMesg.t = MESG_OVL;

#if  SHOW_STATS
Overlap_Ct ++;
#endif

if  (Contig_Mode)
    {
     int  start, stop;

     if  (olap -> s_lo > 0)
         {
          olap -> t_lo -= olap -> s_lo;
          Dovetail_Overlap_Ct ++;
         }
     else if  (olap -> s_hi < S_Len - 1)
         {
          olap -> t_hi += S_Len - 1 - olap -> s_hi;
          Dovetail_Overlap_Ct ++;
         }
       else
         Contained_Overlap_Ct ++;

     if  (S_Dir == FORWARD)
         {
          start = olap -> t_lo;
          stop = olap -> t_hi + 1;
         }
       else
         {
          start = olap -> t_hi + 1;
          stop = olap -> t_lo;
         }

#if  0
     fprintf (BOL_File, "%11" F_U64P " %8d %8d %7d %7d\n",
             Loc_ID [T_ID - Hash_String_Num_Offset], T_ID, S_ID, start, stop);
#else
//  Show lengths of fragments, too.  Used to determine which overlaps
//  are A-end dovetails, B-end dovetails, contained or spanning.
     fprintf (BOL_File, "%11" F_U64P " %8d %8d  %7d %7d %7d  %7d %7d %7d\n",
             Loc_ID [T_ID - Hash_String_Num_Offset], T_ID, S_ID,
             start, stop, T_Len, olap -> s_lo, olap -> s_hi + 1, S_Len);
#endif

     Total_Overlaps ++;
     return;
    }

#if  OUTPUT_OVERLAP_DELTAS
   ovMesg.alignment_delta = deltas;
   *deltas = '\0';
#endif
   assert (S_ID < T_ID);

   S_Right_Hang = S_Len - olap -> s_hi - 1;
   T_Right_Hang = T_Len - olap -> t_hi - 1;
   this_diag = olap -> t_lo - olap -> s_lo;

   if  (olap -> s_lo > olap -> t_lo
            || (olap -> s_lo == olap -> t_lo
                   && S_Right_Hang > T_Right_Hang))
       {       // S is on the left
        ovMesg.aifrag = (Int_Frag_ID_t) S_ID;
        ovMesg.bifrag = (Int_Frag_ID_t) T_ID;

        if  (S_Dir == FORWARD)
          ovMesg.orientation.setIsNormal();
        else
          ovMesg.orientation.setIsOuttie();

        if  (S_Right_Hang >= T_Right_Hang)
            ovMesg.overlap_type = AS_CONTAINMENT;
        else
            ovMesg.overlap_type =  AS_DOVETAIL;

        ovMesg.ahg = olap -> s_lo;
        ovMesg.bhg = T_Right_Hang - S_Right_Hang;
        ovMesg.quality = olap -> quality;
        ovMesg.min_offset = olap -> s_lo - (olap -> max_diag - this_diag);
        ovMesg.max_offset = olap -> s_lo + (this_diag - olap -> min_diag);
       }
     else
       {
#if  OUTPUT_OVERLAP_DELTAS
        for  (i = 0;  i < olap -> delta_ct;  i ++)
          olap -> delta [i] *= -1;
#endif

         ovMesg.bifrag = (Int_Frag_ID_t) S_ID;
         ovMesg.aifrag = (Int_Frag_ID_t) T_ID;
        if  (S_Dir == FORWARD)
          ovMesg.orientation.setIsNormal();
          else
            ovMesg.orientation.setIsInnie();

        if  (T_Right_Hang >= S_Right_Hang)
            ovMesg.overlap_type = AS_CONTAINMENT;
        else
            ovMesg.overlap_type =  AS_DOVETAIL;
        ovMesg.ahg = olap -> t_lo;
        ovMesg.bhg = S_Right_Hang - T_Right_Hang;
        ovMesg.quality =  olap -> quality;
        ovMesg.min_offset = olap -> t_lo - (this_diag - olap -> min_diag);
        ovMesg.max_offset = olap -> t_lo + (olap -> max_diag - this_diag);
       }

   assert (ovMesg.min_offset <= ovMesg.ahg && ovMesg.ahg <= ovMesg.max_offset);
   ovMesg.polymorph_ct = 0;

#if  OUTPUT_OVERLAP_DELTAS
   for  (i = 0;  i < olap -> delta_ct;  i ++)
     {
      int  j;

      for  (j = abs (olap -> delta [i]);  j > 0;  j -= AS_LONGEST_DELTA)
        {
         if  (j > AS_LONGEST_DELTA)
           *deltaCursor++ = AS_LONG_DELTA_CODE;
         else{
           *deltaCursor++ = j * Sign (olap -> delta [i]);
         }
        }
     }
#endif
   *deltaCursor = AS_ENDOF_DELTA_CODE;


   if((ovMesg.overlap_type == AS_CONTAINMENT) &&
      (ovMesg.orientation.isOuttie())) {
     // CMM: Regularize the reverse orientated containment overlaps to
     // a common orientation.
     const int ahg = ovMesg.ahg;
     const int bhg = ovMesg.bhg;
     const int min_delta = ovMesg.min_offset - ahg;
     const int max_delta = ovMesg.max_offset - ahg;

     ovMesg.orientation.setIsInnie();
     ovMesg.ahg = -bhg;
     ovMesg.bhg = -ahg;
     ovMesg.min_offset = ovMesg.ahg - max_delta;
     ovMesg.max_offset = ovMesg.ahg - min_delta;
   }

   WA->Total_Overlaps ++;
   if  (ovMesg . bhg <= 0)
       WA->Contained_Overlap_Ct ++;
     else
       WA->Dovetail_Overlap_Ct ++;

   //  Output either the full protoIO format message, or
   //  the overlap store binary format.
   //
   if (Full_ProtoIO_Output) {
     if  (Num_PThreads > 1)
       pthread_mutex_lock (& Write_Proto_Mutex);

     if (Out_Stream)
       WriteProtoMesg_AS (Out_Stream, & outputMesg);

     if  (Num_PThreads > 1)
       pthread_mutex_unlock (& Write_Proto_Mutex);

   } else {

     AS_OVS_convertOverlapMesgToOVSoverlap(&ovMesg, WA->overlaps + WA->overlapsLen++);

     //  We also flush the file at the end of a thread

     if (WA->overlapsLen >= WA->overlapsMax) {
       int zz;

       if  (Num_PThreads > 1)
         pthread_mutex_lock (& Write_Proto_Mutex);

       for (zz=0; zz<WA->overlapsLen; zz++)
         AS_OVS_writeOverlap(Out_BOF, WA->overlaps + zz);
       WA->overlapsLen = 0;

       if  (Num_PThreads > 1)
         pthread_mutex_unlock (& Write_Proto_Mutex);
     }
   }

   return;
  }



static void  Output_Partial_Overlap
    (Int_Frag_ID_t s_id, Int_Frag_ID_t t_id, Direction_t dir,
     const Olap_Info_t * p, int s_len, int t_len,
     Work_Area_t  *WA)

//  Output the overlap between strings  s_id  and  t_id  which
//  have lengths  s_len  and  t_len , respectively.
//  The overlap information is in  p .
//  dir  indicates the orientation of the S string.

  {
   int  a, b, c, d;
   char  dir_ch;

   Total_Overlaps ++;

   // Convert to canonical form with s forward and use space-based
   // coordinates
   if  (dir == FORWARD)
     {
       a = p -> s_lo;
       b = p -> s_hi + 1;
       c = p -> t_lo;
       d = p -> t_hi + 1;
       dir_ch = 'f';
     }
   else
     {
       a = s_len - p -> s_hi - 1;
       b = s_len - p -> s_lo;
       c = p -> t_hi + 1;
       d = p -> t_lo;
       dir_ch = 'r';
     }

   if  (Contig_Mode) {
     if  (Num_PThreads > 1)
       pthread_mutex_lock (& Write_Proto_Mutex);

     fprintf (BOL_File, "%7d %7d  %c %4d %4d %4d  %6d %6d %6d  %5.2f\n",
              s_id, t_id, dir_ch, a, b, s_len, c, d, t_len,
              100.0 * p -> quality);

     if  (Num_PThreads > 1)
       pthread_mutex_unlock (& Write_Proto_Mutex);
   } else {
     OVSoverlap  *ovl = WA->overlaps + WA->overlapsLen++;
     ovl->a_iid            = s_id;
     ovl->b_iid            = t_id;

     ovl->dat.dat[0]       = 0;
     ovl->dat.dat[1]       = 0;
#if AS_OVS_NWORDS > 2
     ovl->dat.dat[2]       = 0;
#endif

     ovl->dat.obt.fwd      = (dir == FORWARD);
     ovl->dat.obt.a_beg    = a;
     ovl->dat.obt.a_end    = b;
     ovl->dat.obt.b_beg    = c;
     ovl->dat.obt.b_end_hi = d >> 9;
     ovl->dat.obt.b_end_lo = d & 0x1ff;
     ovl->dat.obt.erate    = AS_OVS_encodeQuality(p->quality);
     ovl->dat.obt.type     = AS_OVS_TYPE_OBT;

     //  We also flush the file at the end of a thread

     if (WA->overlapsLen >= WA->overlapsMax) {
       int zz;

       if  (Num_PThreads > 1)
         pthread_mutex_lock (& Write_Proto_Mutex);

       for (zz=0; zz<WA->overlapsLen; zz++)
         AS_OVS_writeOverlap(Out_BOF, WA->overlaps + zz);
       WA->overlapsLen = 0;

       if  (Num_PThreads > 1)
         pthread_mutex_unlock (& Write_Proto_Mutex);
     }
   }

   return;
  }



static int  Passes_Screen
    (int lo, int hi, int sub)

//  Return TRUE iff the range  lo .. hi  contains enough bases
//  outside of the screen ranges in  Screen_Space [sub .. ]
//  to be a valid overlap.

  {
   int  ct;

   ct = Interval_Intersection (lo, hi, Screen_Space [sub] . bgn,
                               Screen_Space [sub] . end - 1);

   while  (! Screen_Space [sub] . last)
     {
      sub ++;
      ct += Interval_Intersection
                (lo, hi, Screen_Space [sub] . bgn,
                 Screen_Space [sub] . end - 1);
     }

   return (1 + hi - lo - ct >= MIN_OLAP_OUTSIDE_SCREEN);
  }



static int  Prefix_Edit_Dist
    (char A [], int m, char T [], int n, int Error_Limit,
     int * A_End, int * T_End, int * Match_To_End, Work_Area_t * WA)

//  Return the minimum number of changes (inserts, deletes, replacements)
//  needed to match string  A [0 .. (m-1)]  with a prefix of string
//   T [0 .. (n-1)]  if it's not more than  Error_Limit .
//  If no match, return the number of errors for the best match
//  up to a branch point.
//  Put delta description of alignment in  WA -> Right_Delta  and set
//  WA -> Right_Delta_Len  to the number of entries there if it's a complete
//  match.
//  Set  A_End  and  T_End  to the rightmost positions where the
//  alignment ended in  A  and  T , respectively.
//  Set  Match_To_End  true if the match extended to the end
//  of at least one string; otherwise, set it false to indicate
//  a branch point.

  {
   int  Delta_Stack [MAX_ERRORS];
#if 0
   double  Cost_Sum, Min_Cost_Sum;
   int Adjustment, Cost_Sub;
#endif
   double  Score, Max_Score;
   int  Max_Score_Len = 0, Max_Score_Best_d = 0, Max_Score_Best_e = 0;
   int  Tail_Len;
   int  Best_d, Best_e, From, Last, Longest, Max, Row;
   int  Left, Right;
   int  d, e, i, j, k;

#if  SHOW_STATS
Edit_Dist_Ct ++;
#endif
   assert (m <= n);
   Best_d = Best_e = Longest = 0;
   WA -> Right_Delta_Len = 0;

   for  (Row = 0;  Row < m
                     && (A [Row] == T [Row]
                           || A [Row] == DONT_KNOW_CHAR
                           || T [Row] == DONT_KNOW_CHAR);  Row ++)
     ;

   WA -> Edit_Array [0] [0] = Row;

   if  (Row == m)                              // Exact match
       {
        (* A_End) = (* T_End) = m;
#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, 0);
#endif
        (* Match_To_End) = TRUE;
        return  0;
       }

   Left = Right = 0;
   Max_Score = 0.0;
   for  (e = 1;  e <= Error_Limit;  e ++)
     {
      Left = OVL_Max_int (Left - 1, -e);
      Right = OVL_Min_int (Right + 1, e);
      WA -> Edit_Array [e - 1] [Left] = -2;
      WA -> Edit_Array [e - 1] [Left - 1] = -2;
      WA -> Edit_Array [e - 1] [Right] = -2;
      WA -> Edit_Array [e - 1] [Right + 1] = -2;

      for  (d = Left;  d <= Right;  d ++)
        {
         Row = 1 + WA -> Edit_Array [e - 1] [d];
         if  ((j = WA -> Edit_Array [e - 1] [d - 1]) > Row)
             Row = j;
         if  ((j = 1 + WA -> Edit_Array [e - 1] [d + 1]) > Row)
             Row = j;
         while  (Row < m && Row + d < n
                  && (A [Row] == T [Row + d]
                        || A [Row] == DONT_KNOW_CHAR
                        || T [Row + d] == DONT_KNOW_CHAR))
           Row ++;

         WA -> Edit_Array [e] [d] = Row;

         if  (Row == m || Row + d == n)
             {
#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, e);
#endif
              //  Check for branch point here caused by uneven
              //  distribution of errors
              Score = Row * Branch_Match_Value - e;
                        // Assumes  Branch_Match_Value
                        //             - Branch_Error_Value == 1.0
              Tail_Len = Row - Max_Score_Len;
              if  ((Doing_Partial_Overlaps && Score < Max_Score)
                     ||  (e > MIN_BRANCH_END_DIST / 2
                           && Tail_Len >= MIN_BRANCH_END_DIST
                           && (Max_Score - Score) / Tail_Len >= MIN_BRANCH_TAIL_SLOPE))
                  {
                   (* A_End) = Max_Score_Len;
                   (* T_End) = Max_Score_Len + Max_Score_Best_d;
                   Set_Right_Delta (Max_Score_Best_e, Max_Score_Best_d, WA);
                   (* Match_To_End) = FALSE;
                   return  Max_Score_Best_e;
                  }

              // Force last error to be mismatch rather than insertion
              if  (Row == m
                     && 1 + WA -> Edit_Array [e - 1] [d + 1]
                          == WA -> Edit_Array [e] [d]
                     && d < Right)
                  {
                   d ++;
                   WA -> Edit_Array [e] [d] = WA -> Edit_Array [e] [d - 1];
                  }

              (* A_End) = Row;           // One past last align position
              (* T_End) = Row + d;
              Set_Right_Delta (e, d, WA);
              (* Match_To_End) = TRUE;
              return  e;
             }
        }

      while  (Left <= Right && Left < 0
                  && WA -> Edit_Array [e] [Left] < WA -> Edit_Match_Limit [e])
        Left ++;
      if  (Left >= 0)
          while  (Left <= Right
                    && WA -> Edit_Array [e] [Left] + Left < WA -> Edit_Match_Limit [e])
            Left ++;
      if  (Left > Right)
          break;
      while  (Right > 0
                  && WA -> Edit_Array [e] [Right] + Right < WA -> Edit_Match_Limit [e])
        Right --;
      if  (Right <= 0)
          while  (WA -> Edit_Array [e] [Right] < WA -> Edit_Match_Limit [e])
            Right --;
      assert (Left <= Right);

      for  (d = Left;  d <= Right;  d ++)
        if  (WA -> Edit_Array [e] [d] > Longest)
            {
             Best_d = d;
             Best_e = e;
             Longest = WA -> Edit_Array [e] [d];
            }

      Score = Longest * Branch_Match_Value - e;
               // Assumes  Branch_Match_Value - Branch_Error_Value == 1.0
      if  (Score > Max_Score)
          {
           Max_Score = Score;
           Max_Score_Len = Longest;
           Max_Score_Best_d = Best_d;
           Max_Score_Best_e = Best_e;
          }
     }

#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, 1 + Error_Limit);
#endif

   (* A_End) = Max_Score_Len;
   (* T_End) = Max_Score_Len + Max_Score_Best_d;
   Set_Right_Delta (Max_Score_Best_e, Max_Score_Best_d, WA);
   (* Match_To_End) = FALSE;
   return  Max_Score_Best_e;
  }



static void  Process_Matches
(int * Start, char * S, int S_Len, char * S_quality,
 Int_Frag_ID_t S_ID, Direction_t Dir, char * T, Hash_Frag_Info_t t_info,
 char * T_quality, Int_Frag_ID_t T_ID,
 Work_Area_t * WA, int consistent)

//  Find and report all overlaps and branch points between string  S
//  (with length  S_Len  and id  S_ID ) and string  T  (with
//  length & screen info in  t_info  and id  T_ID ) using the exact
//  matches in the list beginning at subscript  (* Start) .   Dir  is
//  the orientation of  S .

{
  int  P, * Ref;
  Olap_Info_t  distinct_olap [MAX_DISTINCT_OLAPS];
  Match_Node_t  * Longest_Match, * Ptr;
  Overlap_t  Kind_Of_Olap = NONE;
  double  Quality;
  int  Olap_Len;
  int  overlaps_output = 0;
  int  distinct_olap_ct;
  int  Max_Len, S_Lo, S_Hi, T_Lo, T_Hi;
  int  t_len;
  int  Done_S_Left, Done_S_Right;
  int  Errors, rejected;

  Done_S_Left = Done_S_Right = FALSE;
  t_len = t_info . length;

#if  SHOW_STATS
  Is_Duplicate_Olap = FALSE;
  {
    int  Space [2 * AS_READ_MAX_NORMAL_LEN + 3] = {0};
    int  * Ct = Space + AS_READ_MAX_NORMAL_LEN + 1;
    int  i, Diag_Ct, Gap_Ct, In_Order, Prev;

    In_Order = TRUE;
    Prev = INT_MAX;
    Gap_Ct = 0;
    for  (i = (* Start);  i != 0;  i = WA -> Match_Node_Space [i] . Next)
      {
        Gap_Ct ++;
        if  (WA -> Match_Node_Space [i] . Start >= Prev)
          In_Order = FALSE;
        else
          Prev = WA -> Match_Node_Space [i] . Start;
      }
    Incr_Distrib (& Exacts_Per_Olap_Dist, Gap_Ct);
    if  (In_Order)
      Incr_Distrib (& Gap_Dist, Gap_Ct - 1);
    Diag_Ct = 0;
    for  (i = - AS_READ_MAX_NORMAL_LEN;  i <= AS_READ_MAX_NORMAL_LEN;  i ++)
      if  (Ct [i] > 0)
        Diag_Ct ++;
    Incr_Distrib (& Diag_Dist, Diag_Ct);
  }
#endif

  assert ((* Start) != 0);

  // If a singleton match is hopeless on either side
  // it needn't be processed

  if  (Use_Hopeless_Check
       && WA -> Match_Node_Space [(* Start)] . Next == 0
       && ! Doing_Partial_Overlaps)
    {
      int  s_head, t_head, s_tail, t_tail;
      int  is_hopeless = FALSE;

      s_head = WA -> Match_Node_Space [(* Start)] . Start;
      t_head = WA -> Match_Node_Space [(* Start)] . Offset;
      if  (s_head <= t_head)
        {
          if  (s_head > HOPELESS_MATCH
               && ! WA -> screen_info . left_end_screened)
            is_hopeless = TRUE;
        }
      else
        {
          if  (t_head > HOPELESS_MATCH
               && ! t_info . left_end_screened)
            is_hopeless = TRUE;
        }

      s_tail = S_Len - s_head - WA -> Match_Node_Space [(* Start)] . Len + 1;
      t_tail = t_len - t_head - WA -> Match_Node_Space [(* Start)] . Len + 1;
      if  (s_tail <= t_tail)
        {
          if  (s_tail > HOPELESS_MATCH
               && ! WA -> screen_info . right_end_screened)
            is_hopeless = TRUE;
        }
      else
        {
          if  (t_tail > HOPELESS_MATCH
               && ! t_info . right_end_screened)
            is_hopeless = TRUE;
        }

      if  (is_hopeless)
        {
          (* Start) = 0;
          WA->Kmer_Hits_Without_Olap_Ct ++;
          return;
        }
    }

  distinct_olap_ct = 0;

  while  ((* Start) != 0)
    {
      int  a_hang, b_hang;
      int  hit_limit = FALSE;

      Max_Len = WA -> Match_Node_Space [(* Start)] . Len;
      Longest_Match = WA -> Match_Node_Space + (* Start);
      for  (P = WA -> Match_Node_Space [(* Start)] . Next;  P != 0;
            P = WA -> Match_Node_Space [P] . Next)
        if  (WA -> Match_Node_Space [P] . Len > Max_Len)
          {
            Max_Len = WA -> Match_Node_Space [P] . Len;
            Longest_Match = WA -> Match_Node_Space + P;
          }

      a_hang = Longest_Match -> Start - Longest_Match -> Offset;
      b_hang = a_hang + S_Len - t_len;
      hit_limit =  ((WA -> A_Olaps_For_Frag >= Frag_Olap_Limit
                     && a_hang <= 0)
                    ||
                    (WA -> B_Olaps_For_Frag >= Frag_Olap_Limit
                     && b_hang <= 0));

      if  (! hit_limit)
        {
          Kind_Of_Olap = Extend_Alignment (Longest_Match, S, S_Len, T, t_len,
                                           & S_Lo, & S_Hi, & T_Lo, & T_Hi, & Errors, WA);

          if  (Kind_Of_Olap == DOVETAIL || Doing_Partial_Overlaps)
            {
              if  (1 + S_Hi - S_Lo >= Min_Olap_Len
                   && 1 + T_Hi - T_Lo >= Min_Olap_Len)
                {
                  int  scr_sub;

                  Olap_Len = 1 + OVL_Min_int (S_Hi - S_Lo, T_Hi - T_Lo);
                  Quality = (double) Errors / Olap_Len;
                  scr_sub = Screen_Sub [T_ID - Hash_String_Num_Offset];
                  if  (Errors <= WA -> Error_Bound [Olap_Len]
                       && (scr_sub == 0
                           || Passes_Screen (T_Lo, T_Hi, scr_sub)))
                    {
                      Add_Overlap (S_Lo, S_Hi, T_Lo, T_Hi, Quality,
                                   distinct_olap, & distinct_olap_ct, WA);
#if  SHOW_STATS
                      Incr_Distrib (& Olap_Len_Dist, 1 + S_Hi - S_Lo);
                      if  (Is_Duplicate_Olap)
                        Duplicate_Olap_Ct ++;
                      else
                        {
                          Regular_Olap_Ct ++;
                          Is_Duplicate_Olap = TRUE;
                        }
#endif
                    }
                }
#if  SHOW_STATS
              else
                Too_Short_Ct ++;
#endif
            }
        }

#if  1
      if  (consistent)
        (* Start) = 0;
#endif

      for  (Ref = Start;  (* Ref) != 0;  )
        {
          Ptr = WA -> Match_Node_Space + (* Ref);
          if  (Ptr == Longest_Match
               || ((Kind_Of_Olap == DOVETAIL || Doing_Partial_Overlaps)
                   && S_Lo - SHIFT_SLACK <= Ptr -> Start
                   && Ptr -> Start + Ptr -> Len
                   <= (S_Hi + 1) + SHIFT_SLACK - 1
                   && Lies_On_Alignment
                   (Ptr -> Start, Ptr -> Offset,
                    S_Lo, T_Lo, WA)
                   ))
            (* Ref) = Ptr -> Next;         // Remove this node, it matches
          //   the alignment
          else
            Ref = & (Ptr -> Next);
        }
    }

  if  (distinct_olap_ct > 0)
    {
      int  deleted [MAX_DISTINCT_OLAPS] = {0};
      Olap_Info_t  * p;
      int  i;

      //  Check if any previously distinct overlaps should be merged because
      //  of other merges.

      if  (Doing_Partial_Overlaps)
        {
          if  (Unique_Olap_Per_Pair)
            Choose_Best_Partial (distinct_olap, distinct_olap_ct,
                                 deleted);
          else
            ;  // Do nothing, output them all
        }
      else
        {
          if  (Unique_Olap_Per_Pair)
            Combine_Into_One_Olap (distinct_olap, distinct_olap_ct,
                                   deleted);
          else
            Merge_Intersecting_Olaps (distinct_olap, distinct_olap_ct,
                                      deleted);
        }

      p = distinct_olap;
      for  (i = 0;  i < distinct_olap_ct;  i ++)
        {
          if  (! deleted [i])
            {

#define  SHOW_V_ALIGN  0
              rejected = FALSE;
              if  (Use_Window_Filter)
                {
                  int  d, i, j, k, q_len;
                  char  q_diff [AS_READ_MAX_NORMAL_LEN];

                  i = p -> s_lo;
                  j = p -> t_lo;
                  q_len = 0;

                  for  (k = 0;  k < p -> delta_ct;  k ++)
                    {
                      int  n, len;

                      len = abs (p -> delta [k]);
                      for  (n = 1;  n < len;  n ++)
                        {
                          if  (S [i] == T [j]
                               || S [i] == DONT_KNOW_CHAR
                               || T [j] == DONT_KNOW_CHAR)
                            d = 0;
                          else
                            {
                              d = char_Min (S_quality [i], T_quality [j]);
                              d = char_Min (d, QUALITY_CUTOFF);
                            }
#if  SHOW_V_ALIGN
                          fprintf (stderr, "%3d: %c %2d  %3d: %c %2d  %3d\n",
                                   i, S [i], S_quality [i],
                                   j, T [j], T_quality [j], d);
#endif
                          q_diff [q_len ++] = d;
                          i ++;
                          j ++;
                        }
                      if  (p -> delta [k] > 0)
                        {
                          d = S_quality [i];
#if  SHOW_V_ALIGN
                          fprintf (stderr, "%3d: %c %2d  %3s: %c %2c  %3d\n",
                                   i, S [i], S_quality [i],
                                   " . ", '-', '-', d);
#endif
                          i ++;
                        }
                      else
                        {
                          d = T_quality [j];
#if  SHOW_V_ALIGN
                          fprintf (stderr, "%3s: %c %2c  %3d: %c %2d  %3d\n",
                                   " . ", '-', '-',
                                   j, T [j], T_quality [j], d);
#endif
                          j ++;
                        }
                      q_diff [q_len ++] = char_Min (d, QUALITY_CUTOFF);
                    }
                  while  (i <= p -> s_hi)
                    {
                      if  (S [i] == T [j]
                           || S [i] == DONT_KNOW_CHAR
                           || T [j] == DONT_KNOW_CHAR)
                        d = 0;
                      else
                        {
                          d = char_Min (S_quality [i], T_quality [j]);
                          d = char_Min (d, QUALITY_CUTOFF);
                        }
#if  SHOW_V_ALIGN
                      fprintf (stderr, "%3d: %c %2d  %3d: %c %2d  %3d\n",
                               i, S [i], S_quality [i],
                               j, T [j], T_quality [j], d);
#endif
                      q_diff [q_len ++] = d;
                      i ++;
                      j ++;
                    }

                  if  (Has_Bad_Window (q_diff, q_len,
                                       BAD_WINDOW_LEN, BAD_WINDOW_VALUE))
                    {
                      rejected = TRUE;
                      Bad_Short_Window_Ct ++;
                    }
                  else if  (Has_Bad_Window (q_diff, q_len, 100, 240))
                    {
                      rejected = TRUE;
                      Bad_Long_Window_Ct ++;
                    }
                }

#if  SHOW_SNPS
              {
                int  mismatch_ct, indel_ct;
                int  olap_bases, unscreened_olap_bases, all_match_ct, hi_qual_ct;
                int  local_msnp_bin [20] = {0};
                int  local_isnp_bin [20] = {0};
                int  local_match_bin [20] = {0};
                int  local_other_bin [20] = {0};
                int  i;

                Show_SNPs (S, S_Len, S_quality, T, t_len, T_quality, p, WA,
                           & mismatch_ct, & indel_ct, & olap_bases,
                           & unscreened_olap_bases, & all_match_ct, & hi_qual_ct,
                           local_msnp_bin, local_isnp_bin,
                           local_match_bin, local_other_bin);
                fprintf (Out_Stream, "%8d %c %8d %3d %3d %4d %4d %4d %4d",
                         S_ID, Dir == FORWARD ? 'f' : 'r', T_ID,
                         mismatch_ct, indel_ct, olap_bases, unscreened_olap_bases,
                         all_match_ct, hi_qual_ct);
                fprintf (Out_Stream, "  ");
                for  (i = 3;  i <= 9;  i ++)
                  fprintf (Out_Stream, " %3d", local_msnp_bin [i]);
                fprintf (Out_Stream, "  ");
                for  (i = 3;  i <= 9;  i ++)
                  fprintf (Out_Stream, " %3d", local_isnp_bin [i]);
                fprintf (Out_Stream, "  ");
                for  (i = 3;  i <= 9;  i ++)
                  fprintf (Out_Stream, " %3d", local_match_bin [i]);
                fprintf (Out_Stream, "  ");
                for  (i = 3;  i <= 9;  i ++)
                  fprintf (Out_Stream, " %3d", local_other_bin [i]);
                fprintf (Out_Stream, "\n");

                rejected = TRUE;
              }
#endif

              if  (! rejected)
                {
                  if  (Doing_Partial_Overlaps)
                    Output_Partial_Overlap (S_ID, T_ID, Dir, p,
                                            S_Len, t_len,
                                            WA);
                  else
                    Output_Overlap (S_ID, S_Len, Dir, T_ID, t_len, p, WA);
                  overlaps_output ++;
                  if  (p -> s_lo == 0)
                    WA -> A_Olaps_For_Frag ++;
                  if  (p -> s_hi >= S_Len - 1)
                    WA -> B_Olaps_For_Frag ++;
                }
            }
          p ++;
        }
    }


  if  (overlaps_output == 0)
    WA->Kmer_Hits_Without_Olap_Ct ++;
  else
    {
      WA->Kmer_Hits_With_Olap_Ct ++;
      if  (overlaps_output > 1)
        WA->Multi_Overlap_Ct ++;
    }


  return;
}



void  Process_Overlaps
    (gkStream *stream, Work_Area_t * WA)

//  Find and output all overlaps between strings in  stream
//  and those in the global hash table.   (* WA)  has the
//  data structures that used to be global.

  {
   char  Frag [AS_READ_MAX_NORMAL_LEN + 1];
   char  quality [AS_READ_MAX_NORMAL_LEN + 1];
   Int_Frag_ID_t  Curr_String_Num;
   uint32  last_old_frag_read;
   int  frag_status;
   int  Len;

   WA->overlapsLen                = 0;

   WA->Total_Overlaps             = 0;
   WA->Contained_Overlap_Ct       = 0;
   WA->Dovetail_Overlap_Ct        = 0;

   WA->Kmer_Hits_Without_Olap_Ct  = 0;
   WA->Kmer_Hits_With_Olap_Ct     = 0;
   WA->Multi_Overlap_Ct           = 0;


   while  ((frag_status
              = Read_Next_Frag (Frag, quality, stream, &WA -> myRead,
                                & (WA -> screen_info), & last_old_frag_read)))
     {

      if  (frag_status == DELETED_FRAG)
           continue;

      Curr_String_Num = WA->myRead.gkFragment_getReadIID ();

      //getReadType_ReadStruct (WA -> myRead, & (WA -> curr_frag_type));
      WA -> curr_frag_type = AS_READ;
      Len = strlen (Frag);
      if  (Len < Min_Olap_Len)
          fprintf (stderr, "Frag %d is too short.  Len = %d\n",
                      Curr_String_Num, Len);
        else
          {

#if  SHOW_STATS
Overlap_Ct = 0;
Kmer_Hits_Ct = 0;
#endif

           WA -> A_Olaps_For_Frag = WA -> B_Olaps_For_Frag = 0;

           Find_Overlaps (Frag, Len, quality, Curr_String_Num, FORWARD, WA);


           reverseComplementSequence (Frag, Len);
           Reverse_String (quality, Len);

           Flip_Screen_Range (& (WA -> screen_info), Len);

           WA -> A_Olaps_For_Frag = WA -> B_Olaps_For_Frag = 0;

           Find_Overlaps (Frag, Len, quality, Curr_String_Num, REVERSE, WA);

#if  SHOW_STATS
Incr_Distrib (& Num_Olaps_Dist, Overlap_Ct);
Incr_Distrib (& Kmer_Hits_Dist, Kmer_Hits_Ct);
#endif

          }
     }


 if  (Num_PThreads > 1)
   pthread_mutex_lock (& Write_Proto_Mutex);

  //  Flush!  Also flushed in Output_Partial_Overlap and
  //  Output_Overlap.
  {
    int zz;
    for (zz=0; zz<WA->overlapsLen; zz++)
       AS_OVS_writeOverlap(Out_BOF, WA->overlaps + zz);
  }
  WA->overlapsLen = 0;

  Total_Overlaps            += WA->Total_Overlaps;
  Contained_Overlap_Ct      += WA->Contained_Overlap_Ct;
  Dovetail_Overlap_Ct       += WA->Dovetail_Overlap_Ct;

  Kmer_Hits_Without_Olap_Ct += WA->Kmer_Hits_Without_Olap_Ct;
  Kmer_Hits_With_Olap_Ct    += WA->Kmer_Hits_With_Olap_Ct;
  Multi_Overlap_Ct          += WA->Multi_Overlap_Ct;

  if  (Num_PThreads > 1)
    pthread_mutex_unlock (& Write_Proto_Mutex);

   return;
  }



static int  Process_String_Olaps
    (char * S, int Len, char * S_quality, Int_Frag_ID_t ID,
     Direction_t Dir, Work_Area_t * WA)

//  Find and report all overlaps and branch points between string  S
//  and all strings in global  String_Olap_Space .
//  Return the number of entries processed.
//  Len  is the length of  S ,  ID  is its fragment ID  and
//  Dir  indicates if  S  is forward, or reverse-complemented.

  {
   int32  i, ct, root_num, start, processed_ct;

//  Move all full entries to front of String_Olap_Space and set
//  diag_sum to average diagonal.  if enough entries to bother,
//  sort by average diagonal.  Then process positive & negative diagonals
//  separately in order from longest to shortest overlap.  Stop
//  processing when output limit has been reached.

   for  (i = ct = 0;  i < WA -> Next_Avail_String_Olap;  i ++)
     if  (WA -> String_Olap_Space [i] . Full)
         {
          root_num = WA -> String_Olap_Space [i] . String_Num;
          if  (Contig_Mode || root_num + Hash_String_Num_Offset > ID)
              {
#if  SHOW_STATS
Incr_Distrib (& Hits_Per_Olap_Dist, WA -> String_Olap_Space [i] . Kmer_Hits);
#endif
if  (WA -> String_Olap_Space [i] . Match_List == 0)
    {
     fprintf (stderr, " Curr_String_Num = %d  root_num  %d have no matches\n",
              ID, root_num);
     exit (-2);
    }
               if  (i != ct)
                   WA -> String_Olap_Space [ct] = WA -> String_Olap_Space [i];
               assert (WA -> String_Olap_Space [ct] . diag_ct > 0);
               WA -> String_Olap_Space [ct] . diag_sum
                   /= WA -> String_Olap_Space [ct] . diag_ct;
               ct ++;
              }
         }

   if  (ct == 0)
       return  ct;

   if  (ct <= Frag_Olap_Limit)
       {
        for  (i = 0;  i < ct;  i ++)
          {
           root_num = WA -> String_Olap_Space [i] . String_Num;
           if  ((WA -> curr_frag_type == AS_READ
                       || WA -> curr_frag_type == AS_EXTR
                       || WA -> curr_frag_type == AS_TRNR)
                    && (Kind_Of_Frag [root_num] == AS_READ
                            || Kind_Of_Frag [root_num] == AS_EXTR
                            || Kind_Of_Frag [root_num] == AS_TRNR))
               {
                WA -> Edit_Match_Limit = Read_Edit_Match_Limit;
                WA -> Error_Bound = Read_Error_Bound;
               }
             else
               {
                WA -> Edit_Match_Limit = Guide_Edit_Match_Limit;
                WA -> Error_Bound = Guide_Error_Bound;
               }
           Process_Matches
                  (& (WA -> String_Olap_Space [i] . Match_List),
                   S, Len, S_quality, ID, Dir,
                   Data + String_Start [root_num],
                   String_Info [root_num],
                   Quality_Data + String_Start [root_num],
                   root_num + Hash_String_Num_Offset, WA,
                   WA -> String_Olap_Space [i] . consistent);
           assert (WA -> String_Olap_Space [i] . Match_List == 0);
          }

        return  ct;
       }

#if  1
   qsort (WA -> String_Olap_Space, ct, sizeof (String_Olap_t), By_Diag_Sum);
#else
   for  (i = 0;  i < ct - 1;  i ++)
     for  (j = i + 1;  j < ct;  j ++)
       if  (WA -> String_Olap_Space [i] . diag_sum
              > WA -> String_Olap_Space [j] . diag_sum)
           {
            String_Olap_t  save = WA -> String_Olap_Space [i];

            WA -> String_Olap_Space [i] = WA -> String_Olap_Space [j];
            WA -> String_Olap_Space [j] = save;
           }
#endif

   for  (start = 0;
           start < ct && WA -> String_Olap_Space [start] . diag_sum < 0;
           start ++)
     ;

   processed_ct = 0;

   for  (i = start;  i < ct && WA -> A_Olaps_For_Frag < Frag_Olap_Limit ;  i ++)
     {
      root_num = WA -> String_Olap_Space [i] . String_Num;
      if  ((WA -> curr_frag_type == AS_READ
                  || WA -> curr_frag_type == AS_EXTR
                  || WA -> curr_frag_type == AS_TRNR)
               && (Kind_Of_Frag [root_num] == AS_READ
                       || Kind_Of_Frag [root_num] == AS_EXTR
                       || Kind_Of_Frag [root_num] == AS_TRNR))
          {
           WA -> Edit_Match_Limit = Read_Edit_Match_Limit;
           WA -> Error_Bound = Read_Error_Bound;
          }
        else
          {
           WA -> Edit_Match_Limit = Guide_Edit_Match_Limit;
           WA -> Error_Bound = Guide_Error_Bound;
          }
      Process_Matches
             (& (WA -> String_Olap_Space [i] . Match_List),
              S, Len, S_quality, ID, Dir,
              Data + String_Start [root_num],
              String_Info [root_num],
              Quality_Data + String_Start [root_num],
              root_num + Hash_String_Num_Offset, WA,
              WA -> String_Olap_Space [i] . consistent);
      assert (WA -> String_Olap_Space [i] . Match_List == 0);

      processed_ct ++;
     }
   for  (i = start - 1;  i >= 0 && WA -> B_Olaps_For_Frag < Frag_Olap_Limit ;  i --)
     {
      root_num = WA -> String_Olap_Space [i] . String_Num;
      if  ((WA -> curr_frag_type == AS_READ
                  || WA -> curr_frag_type == AS_EXTR
                  || WA -> curr_frag_type == AS_TRNR)
               && (Kind_Of_Frag [root_num] == AS_READ
                       || Kind_Of_Frag [root_num] == AS_EXTR
                       || Kind_Of_Frag [root_num] == AS_TRNR))
          {
           WA -> Edit_Match_Limit = Read_Edit_Match_Limit;
           WA -> Error_Bound = Read_Error_Bound;
          }
        else
          {
           WA -> Edit_Match_Limit = Guide_Edit_Match_Limit;
           WA -> Error_Bound = Guide_Error_Bound;
          }
      Process_Matches
             (& (WA -> String_Olap_Space [i] . Match_List),
              S, Len, S_quality, ID, Dir,
              Data + String_Start [root_num],
              String_Info [root_num],
              Quality_Data + String_Start [root_num],
              root_num + Hash_String_Num_Offset, WA,
              WA -> String_Olap_Space [i] . consistent);
      assert (WA -> String_Olap_Space [i] . Match_List == 0);

      processed_ct ++;
     }

   return  processed_ct;
  }




static void  Put_String_In_Hash
    (int i)

//  Insert string subscript  i  into the global hash table.
//  Sequence and information about the string are in
//  global variables  Data, String_Start, String_Info, ....

  {
   String_Ref_t  ref = { 0 };
   char  * p, * window;
   int  kmers_inserted = 0;
   int  skip_ct;
   int  screen_sub, screen_lo, screen_hi;
   uint64  key, key_is_bad;
   int  j;

   if  (String_Info [i] . length < Kmer_Len)
       return;

   screen_sub = Screen_Sub [i];
   if  (screen_sub == 0)
       screen_lo = screen_hi = INT_MAX;
     else
       {
        screen_lo = Screen_Space [screen_sub] . bgn;
        screen_hi = Screen_Space [screen_sub] . end;
       }

   p = window = Data + String_Start [i];
   key = key_is_bad = 0;
   for  (j = 0;  j < Kmer_Len;  j ++)
     {
      key_is_bad |= (uint64) (Char_Is_Bad [(int) * p]) << j;
      key |= (uint64) (Bit_Equivalent [(int) * (p ++)]) << (2 * j);
     }

   setStringRefStringNum(ref, i);
   if  (i > MAX_STRING_NUM)
       {
        fprintf (stderr, "Too many strings for hash table--exiting\n");
        exit (1);
       }
   setStringRefOffset(ref, 0);
   skip_ct = 0;
   setStringRefEmpty(ref, FALSE);

   if  ((int) (getStringRefOffset(ref)) <= screen_lo - Kmer_Len + WINDOW_SCREEN_OLAP
            && ! key_is_bad)
       {
        Hash_Insert (ref, key, window);
        kmers_inserted ++;
       }

   while  ((* p) != '\0')
     {
       if  ((int) (getStringRefOffset(ref)) == screen_hi - 1 - WINDOW_SCREEN_OLAP)
          {
           if  (Screen_Space [screen_sub] . last)
               screen_lo = screen_hi = INT_MAX;
             else
               {
                screen_sub ++;
                screen_lo = Screen_Space [screen_sub] . bgn;
                screen_hi = Screen_Space [screen_sub] . end;
               }
          }

      window ++;

      {
        uint32 newoff = getStringRefOffset(ref) + 1;
        assert(newoff < OFFSET_MASK);
        setStringRefOffset(ref, newoff);
      }

      if  (++ skip_ct > HASH_KMER_SKIP)
          skip_ct = 0;

      key_is_bad >>= 1;
      key_is_bad |= (uint64) (Char_Is_Bad [(int) * p]) << (Kmer_Len - 1);
      key >>= 2;
      key |= (uint64)
                 (Bit_Equivalent [(int) * (p ++)]) << (2 * (Kmer_Len - 1));

      if  (skip_ct == 0
           && (int) (getStringRefOffset(ref))
                     <= screen_lo - Kmer_Len + WINDOW_SCREEN_OLAP
               && ! key_is_bad)
          {
           Hash_Insert (ref, key, window);
           kmers_inserted ++;
          }
     }

   return;
  }



static int  Read_Next_Frag
    (char frag [AS_READ_MAX_NORMAL_LEN + 1],
     char quality [AS_READ_MAX_NORMAL_LEN + 1],
     gkStream *stream,
     gkFragment *myRead,
     Screen_Info_t * screen,
     uint32 * last_frag_read)

/* Read the next fragment from fragment stream  stream  and store it in  frag ,
*  with its quality values in  quality .  Put the read itself in  myRead
*  and the screen information in  screen .  Set  (* last_frag_read)
*  to the iid of the fragment read.
*  If successful, return  TRUE ; if there is no fragment, return  FALSE . */

  {
   int  return_val, success, match_ct;
   size_t  i, frag_len;
   char   *seqptr;
   char   *qltptr;


   //  BPW says we don't need to mutex this
   success = stream->next (myRead);

   if  (! success)
       return(0);

   *last_frag_read = myRead->gkFragment_getReadIID ();

   if  (myRead->gkFragment_getIsDeleted ())
       return (DELETED_FRAG);


   //  We got a read!  Lowercase it, adjust the quality, and extract
   //  the clear region.

   seqptr   = myRead->gkFragment_getSequence();
   qltptr   = myRead->gkFragment_getQuality();
   frag_len = myRead->gkFragment_getSequenceLength();

   for  (i = 0;  i < frag_len;  i ++)
     {
       frag [i]    = tolower (seqptr [i]);
       quality [i] = qltptr[i] - QUALITY_BASE_CHAR;
     }

   if  (! Ignore_Clear_Range)
     {
       uint32 clear_start, clear_end;
       myRead->gkFragment_getClearRegion(clear_start, clear_end);

       frag_len = clear_end - clear_start;
       if  (clear_start > 0)
         {
           memmove (frag, frag + clear_start, frag_len);
           memmove (quality, quality + clear_start, frag_len);
         }
     }
   if  (OFFSET_MASK < frag_len)
     {
       fprintf (stderr, "ERROR:  Read " F_IID " is too long (%lu) for hash table\n",
                myRead->gkFragment_getReadIID(), frag_len);
       exit (-1);
     }
   frag [frag_len] = '\0';
   quality [frag_len] = '\0';

   screen -> num_matches        = 0;
   screen -> left_end_screened  = FALSE;
   screen -> right_end_screened = FALSE;

   return (VALID_FRAG);
  }



static void  Read_uint32_List
    (char * file_name, uint32 * * list, int * n)

//  Open  file_name  and use the list of integers in it to build
//  a sorted (ascending) list of integers in  (* list) .
//  Set  (* n)  to the number of entries in that list.

  {
   FILE  * fp = File_Open (file_name, "r");
   int  num;


   (* n) = 0;
   while  (fscanf (fp, "%u", & num) == 1)
     {
      (* n) ++;
      (* list) = (uint32 *) safe_realloc ((* list), (* n) * sizeof (uint32));
      (* list) [(* n) - 1] = num;
     }

   if  ((* n) > 0)
       qsort ((* list), (* n), sizeof (uint32), By_uint32);

   fclose (fp);

   return;
  }





static int  Rev_Prefix_Edit_Dist
    (char A [], int m, char T [], int n, int Error_Limit,
     int * A_End, int * T_End, int * Leftover, int * Match_To_End,
     Work_Area_t * WA)

//  Return the minimum number of changes (inserts, deletes, replacements)
//  needed to match string  A [0 .. (1-m)]  right-to-left with a prefix of string
//   T [0 .. (1-n)]  right-to-left if it's not more than  Error_Limit .
//  If no match, return the number of errors for the best match
//  up to a branch point.
//  Put delta description of alignment in  WA -> Left_Delta  and set
//  WA -> Left_Delta_Len  to the number of entries there.
//  Set  A_End  and  T_End  to the leftmost positions where the
//  alignment ended in  A  and  T , respectively.
//  If the alignment succeeds set  Leftover  to the number of
//  characters that match after the last  WA -> Left_Delta  entry;
//  otherwise, set  Leftover  to zero.
//  Set  Match_To_End  true if the match extended to the end
//  of at least one string; otherwise, set it false to indicate
//  a branch point.

  {
#if 0
   double  Cost_Sum, Min_Cost_Sum;
   int Adjustment, Cost_Sub;
#endif
   double  Score, Max_Score;
   int  Max_Score_Len = 0, Max_Score_Best_d = 0, Max_Score_Best_e = 0;
   int  Tail_Len;
   int  Best_d, Best_e, From, Last, Longest, Max, Row;
   int  Left, Right;
   int  d, e, j, k;


#if  SHOW_STATS
Edit_Dist_Ct ++;
#endif
   assert (m <= n);
   Best_d = Best_e = Longest = 0;
   WA -> Left_Delta_Len = 0;

   for  (Row = 0;  Row < m
                     && (A [- Row] == T [- Row]
                          || A [- Row] == DONT_KNOW_CHAR
                          || T [- Row] == DONT_KNOW_CHAR);  Row ++)
     ;

   WA -> Edit_Array [0] [0] = Row;

   if  (Row == m)
       {
        (* A_End) = (* T_End) = - m;
        (* Leftover) = m;
#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, 0);
#endif
        (* Match_To_End) = TRUE;
        return  0;
       }

   Left = Right = 0;
   Max_Score = 0.0;
   for  (e = 1;  e <= Error_Limit;  e ++)
     {
      Left = OVL_Max_int (Left - 1, -e);
      Right = OVL_Min_int (Right + 1, e);
      WA -> Edit_Array [e - 1] [Left] = -2;
      WA -> Edit_Array [e - 1] [Left - 1] = -2;
      WA -> Edit_Array [e - 1] [Right] = -2;
      WA -> Edit_Array [e - 1] [Right + 1] = -2;

      for  (d = Left;  d <= Right;  d ++)
        {
         Row = 1 + WA -> Edit_Array [e - 1] [d];
         if  ((j = WA -> Edit_Array [e - 1] [d - 1]) > Row)
             Row = j;
         if  ((j = 1 + WA -> Edit_Array [e - 1] [d + 1]) > Row)
             Row = j;
         while  (Row < m && Row + d < n
                  && (A [- Row] == T [- Row - d]
                        || A [- Row] == DONT_KNOW_CHAR
                        || T [- Row - d] == DONT_KNOW_CHAR))
           Row ++;

         WA -> Edit_Array [e] [d] = Row;

         if  (Row == m || Row + d == n)
             {
#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, e);
#endif

              //  Check for branch point here caused by uneven
              //  distribution of errors

              Score = Row * Branch_Match_Value - e;
                        // Assumes  Branch_Match_Value
                        //             - Branch_Error_Value == 1.0
              Tail_Len = Row - Max_Score_Len;
              if  ((Doing_Partial_Overlaps && Score < Max_Score)
                     || (e > MIN_BRANCH_END_DIST / 2
                           && Tail_Len >= MIN_BRANCH_END_DIST
                           && (Max_Score - Score) / Tail_Len >= MIN_BRANCH_TAIL_SLOPE))
                  {
                   (* A_End) = - Max_Score_Len;
                   (* T_End) = - Max_Score_Len - Max_Score_Best_d;
                   Set_Left_Delta (Max_Score_Best_e, Max_Score_Best_d,
                        Leftover, T_End, n, WA);
                   (* Match_To_End) = FALSE;
                   return  Max_Score_Best_e;
                  }

              (* A_End) = - Row;           // One past last align position
              (* T_End) = - Row - d;
              Set_Left_Delta (e, d, Leftover, T_End, n, WA);
              (* Match_To_End) = TRUE;
              return  e;
             }
        }

      while  (Left <= Right && Left < 0
                  && WA -> Edit_Array [e] [Left] < WA -> Edit_Match_Limit [e])
        Left ++;
      if  (Left >= 0)
          while  (Left <= Right
                    && WA -> Edit_Array [e] [Left] + Left < WA -> Edit_Match_Limit [e])
            Left ++;
      if  (Left > Right)
          break;
      while  (Right > 0
                  && WA -> Edit_Array [e] [Right] + Right < WA -> Edit_Match_Limit [e])
        Right --;
      if  (Right <= 0)
          while  (WA -> Edit_Array [e] [Right] < WA -> Edit_Match_Limit [e])
            Right --;
      assert (Left <= Right);

      for  (d = Left;  d <= Right;  d ++)
        if  (WA -> Edit_Array [e] [d] > Longest)
            {
             Best_d = d;
             Best_e = e;
             Longest = WA -> Edit_Array [e] [d];
            }

      Score = Longest * Branch_Match_Value - e;
               // Assumes  Branch_Match_Value - Branch_Error_Value == 1.0
      if  (Score > Max_Score)
          {
           Max_Score = Score;
           Max_Score_Len = Longest;
           Max_Score_Best_d = Best_d;
           Max_Score_Best_e = Best_e;
          }
     }

#if  SHOW_STATS
Incr_Distrib (& Edit_Depth_Dist, 1 + Error_Limit);
#endif

   (* A_End) = - Max_Score_Len;
   (* T_End) = - Max_Score_Len - Max_Score_Best_d;
   Set_Left_Delta (Max_Score_Best_e, Max_Score_Best_d,
        Leftover, T_End, n, WA);
   (* Match_To_End) = FALSE;
   return  Max_Score_Best_e;
  }



static void  Reverse_String
    (char * s, int len)

//  Reverse the order of characters in  s [0 .. len - 1] .

  {
   int  i, j;

   for  (i = 0, j = len - 1;  i < j;  i ++, j --)
     {
      char  ch;

      ch = s [i];
      s [i] = s [j];
      s [j] = ch;
     }

   return;
  }



static void  Set_Left_Delta
    (int e, int d, int * leftover, int * t_end, int t_len,
     Work_Area_t * WA)

//  Put the delta encoding of the alignment represented in  WA -> Edit_Array
//  starting at row  e  (which is the number of errors) and column  d
//  (which is the diagonal) and working back to the start, into
//  WA -> Left_Delta .  Set  WA -> Left_Delta_Len  to the number of
//  delta entries and set  (* leftover)  to the number of
//  characters that match after the last  WA -> Left_Delta  entry.
//  Don't allow the first delta to be an indel if it can be
//  converted to a substitution by adjusting  (* t_end)  which
//  is where the alignment ends in the T string, which has length
//   t_len .

  {
   int  from, last, max;
   int  j, k;

   last = WA -> Edit_Array [e] [d];
   WA -> Left_Delta_Len = 0;
   for  (k = e;  k > 0;  k --)
     {
      from = d;
      max = 1 + WA -> Edit_Array [k - 1] [d];
      if  ((j = WA -> Edit_Array [k - 1] [d - 1]) > max)
          {
           from = d - 1;
           max = j;
          }
      if  ((j = 1 + WA -> Edit_Array [k - 1] [d + 1]) > max)
          {
           from = d + 1;
           max = j;
          }
      if  (from == d - 1)
          {
           WA -> Left_Delta [WA -> Left_Delta_Len ++] = max - last - 1;
           d --;
           last = WA -> Edit_Array [k - 1] [from];
          }
      else if  (from == d + 1)
          {
           WA -> Left_Delta [WA -> Left_Delta_Len ++] = last - (max - 1);
           d ++;
           last = WA -> Edit_Array [k - 1] [from];
          }
     }
   (* leftover) = last;

   // Don't allow first delta to be +1 or -1
   assert (WA -> Left_Delta_Len == 0 || WA -> Left_Delta [0] != -1);
   if  (WA -> Left_Delta_Len > 0 && WA -> Left_Delta [0] == 1
          && (* t_end) + t_len > 0)
       {
        int  i;

        if  (WA -> Left_Delta [1] > 0)
            WA -> Left_Delta [0] = WA -> Left_Delta [1] + 1;
          else
            WA -> Left_Delta [0] = WA -> Left_Delta [1] - 1;
        for  (i = 2;  i < WA -> Left_Delta_Len;  i ++)
          WA -> Left_Delta [i - 1] = WA -> Left_Delta [i];
        WA -> Left_Delta_Len --;
        (* t_end) --;
        if  (WA -> Left_Delta_Len == 0)
            (* leftover) ++;
       }

   return;
  }



static void  Set_Right_Delta
    (int e, int d, Work_Area_t * WA)

//  Put the delta encoding of the alignment represented in  WA -> Edit_Array
//  starting at row  e  (which is the number of errors) and column  d
//  (which is the diagonal) and working back to the start, into
//  WA -> Right_Delta .  Set  WA -> Right_Delta_Len  to the number of
//  delta entries.

  {
   int  delta_stack [MAX_ERRORS];
   int  from, last, max;
   int  i, j, k;

   last = WA -> Edit_Array [e] [d];
   WA -> Right_Delta_Len = 0;
   for  (k = e;  k > 0;  k --)
     {
      from = d;
      max = 1 + WA -> Edit_Array [k - 1] [d];
      if  ((j = WA -> Edit_Array [k - 1] [d - 1]) > max)
          {
           from = d - 1;
           max = j;
          }
      if  ((j = 1 + WA -> Edit_Array [k - 1] [d + 1]) > max)
          {
           from = d + 1;
           max = j;
          }
      if  (from == d - 1)
          {
           delta_stack [WA -> Right_Delta_Len ++] = max - last - 1;
           d --;
           last = WA -> Edit_Array [k - 1] [from];
          }
      else if  (from == d + 1)
          {
           delta_stack [WA -> Right_Delta_Len ++] = last - (max - 1);
           d ++;
           last = WA -> Edit_Array [k - 1] [from];
          }
     }
   delta_stack [WA -> Right_Delta_Len ++] = last + 1;

   k = 0;
   for  (i = WA -> Right_Delta_Len - 1;  i > 0;  i --)
     WA -> Right_Delta [k ++]
         = abs (delta_stack [i]) * Sign (delta_stack [i - 1]);
   WA -> Right_Delta_Len --;

   return;
  }





void  Show_Alignment
    (char * S, char * T, Olap_Info_t * p)

//  Display the alignment between strings  S  and  T  indicated
//  in  (* p) .

  {
   char  S_Line [AS_READ_MAX_NORMAL_LEN + 1], T_Line [AS_READ_MAX_NORMAL_LEN + 1];
   char  X_Line [AS_READ_MAX_NORMAL_LEN + 1];
   int  i, j, ks, kt, ns, nt, ct;

   i = p -> s_lo;
   j = p -> t_lo;
   ks = kt = 0;
   ns = nt = 1;
   while  (i <= p -> s_hi)
     {
      ct = 0;
      while  (i <= p -> s_hi && ct < DISPLAY_WIDTH)
        {
         if  (ks < p -> delta_ct && ns == abs (p -> delta [ks]))
             {
              if  (p -> delta [ks] < 0)
                  S_Line [ct] = '-';
                else
                  S_Line [ct] = S [i ++];
              ks ++;
              ns = 1;
             }
           else
             {
              S_Line [ct] = S [i ++];
              ns ++;
             }
         ct ++;
        }
      S_Line [ct] = '\0';
      printf ("%s\n", S_Line);

      ct = 0;
      while  (j <= p -> t_hi && ct < DISPLAY_WIDTH)
        {
         if  (kt < p -> delta_ct && nt == abs (p -> delta [kt]))
             {
              if  (p -> delta [kt] > 0)
                  T_Line [ct] = '-';
                else
                  T_Line [ct] = T [j ++];
              kt ++;
              nt = 1;
             }
           else
             {
              T_Line [ct] = T [j ++];
              nt ++;
             }
         if  (S_Line [ct] == T_Line [ct])
             X_Line [ct] =  ' ';
         else if  (S_Line [ct] == DONT_KNOW_CHAR
                     || T_Line [ct] == DONT_KNOW_CHAR)
             X_Line [ct] =  '?';
           else
             X_Line [ct] =  '^';
         ct ++;
        }
      T_Line [ct] = X_Line [ct] = '\0';
      printf ("%s\n", T_Line);
      printf ("%s\n", X_Line);

      putchar ('\n');
     }
  }




#if  SHOW_SNPS
#define  SHOW_SNP_DETAILS     0
#define  SIDE_BASES           4
#define  MIN_SIDE_QUALITY    20
#define  INIT_MASK            (1 << (2 * SIDE_BASES)) - 1
#define  QUAL_MATCH_MASK      ((1 << (2 * SIDE_BASES + 1)) - 1)
#define  SNP_MATCH_MASK       (QUAL_MATCH_MASK ^ (1 << SIDE_BASES))
#define  END_AVOID_LEN       30

static void  Show_SNPs
    (char * a, int a_len, char * a_quality,
     char * b, int b_len, char * b_quality, Olap_Info_t * p,
     Work_Area_t * WA, int * mismatch_ct, int * indel_ct,
     int * olap_bases, int * unscreened_olap_bases,
     int * all_match_ct, int * hi_qual_ct,
     int local_msnp_bin [], int local_isnp_bin [],
     int local_match_bin [], int local_other_bin [])

//  Show the overlap recorded in  (* p)  between strings  A  and  B ,
//  with lengths  A_Len  and  B_Len, respectively.  WA has screen
//  information about  A .  Set  (* mismatch_ct)  to the number
//  of mismatch SNPs and  (* indel_ct)  to the number of insert/delete
//  SNPs.  Set  (* olap_bases)  to the number of bases in  A  involved
//  in the overlap.  Set  (* unscreened_olap_bases)  to the number
//  of bases in the overlap that were not in screened regions.

  {
   int  i, j, k, m, n;
   int  indel_ring [SIDE_BASES + 1] = {0};
   int  quality_ring [SIDE_BASES + 1] = {0};
   int  screen_sub, screen_lo, screen_hi;
   unsigned int  match_mask = 0, quality_mask = 0, bit;

   (* mismatch_ct) = 0;
   (* indel_ct) = 0;
   (* olap_bases) = 0;
   (* unscreened_olap_bases) = 0;
   (* all_match_ct) = 0;
   (* hi_qual_ct) = 0;

   for  (k = 0;  k <= SIDE_BASES;  k ++)
     indel_ring [k] = 3;

   screen_sub = 0;
   if  (WA -> screen_info . num_matches == 0)
       screen_lo = screen_hi = INT_MAX;
     else
       {
        screen_lo = WA -> screen_info . range [0] . bgn;
        screen_hi = WA -> screen_info . range [0] . end;
       }
#if  SHOW_SNP_DETAILS
   fprintf (Out_Stream, "Screen range: %10d %10d\n",
            screen_lo, screen_hi);
#endif

   i = p -> s_lo;
   j = p -> t_lo;

   while  (i > screen_hi)
     {
      if  (WA -> screen_info . range [screen_sub] . last)
          screen_lo = screen_hi = INT_MAX;
        else
          {
           screen_sub ++;
           screen_lo = WA -> screen_info . range [screen_sub] . bgn;
           screen_hi = WA -> screen_info . range [screen_sub] . end;
          }
#if  SHOW_SNP_DETAILS
      fprintf (Out_Stream, "Screen range: %10d %10d\n",
               screen_lo, screen_hi);
#endif
     }

   n = 0;
   for  (k = 0;  k < p -> delta_ct;  k ++)
     {
      for  (m = 1;  m < abs (p -> delta [k]);  m ++)
        {
         match_mask &= INIT_MASK;
         bit = ((a [i] == b [j]) ? 1 : 0);
         match_mask = (match_mask << 1) | bit;

         quality_mask &= INIT_MASK;
         bit = ((a_quality [i] >= MIN_SIDE_QUALITY
                   && b_quality [j] >= MIN_SIDE_QUALITY) ? 1 : 0);
         quality_mask = (quality_mask << 1) | bit;

         if  (i < END_AVOID_LEN
                || i >= a_len - END_AVOID_LEN
                || j < END_AVOID_LEN
                || j >= b_len - END_AVOID_LEN)
             indel_ring [n % (SIDE_BASES + 1)] = 3;  // too near end
         else if  (i < screen_lo)
             indel_ring [n % (SIDE_BASES + 1)] = 0;  // mismatch
           else
             indel_ring [n % (SIDE_BASES + 1)] = 2;  // screened
         quality_ring [n % (SIDE_BASES + 1)]
             = OVL_Min_int (a_quality [i], b_quality [j]);

         if  (quality_mask == QUAL_MATCH_MASK
                && indel_ring [(n + 1) % (SIDE_BASES + 1)] < 2)
             {
              if  (match_mask == SNP_MATCH_MASK)
                  {
                   switch  (indel_ring [(n + 1) % (SIDE_BASES + 1)])
                     {
                      case  0 :    // mismatch
#if  SHOW_SNP_DETAILS
                        fprintf (Out_Stream, "SNP  %4d  %4d  %.*s/%.*s\n",
                                 i - SIDE_BASES, j - SIDE_BASES,
                                 2 * SIDE_BASES + 1, a + i - 2 * SIDE_BASES,
                                 2 * SIDE_BASES + 1, b + j - 2 * SIDE_BASES);
                        fprintf (Out_Stream,
                                 "     i = %4d  j = %4d  qual_mask = %04o"
                                 "  match_mask = %04o\n",
                                 i, j, quality_mask, match_mask);
#endif
                        (* mismatch_ct) ++;
                        Global_Match_SNP_Bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        local_msnp_bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        break;
                      case  1 :    // insert
#if  SHOW_SNP_DETAILS
                        fprintf (Out_Stream, "SNP  %4d  %4d  %.*s-%.*s/%.*s\n",
                                 i - SIDE_BASES, j - SIDE_BASES,
                                 SIDE_BASES, a + i - 2 * SIDE_BASES + 1,
                                 SIDE_BASES, a + i - SIDE_BASES + 1,
                                 2 * SIDE_BASES + 1, b + j - 2 * SIDE_BASES);
                        fprintf (Out_Stream,
                                 "     i = %4d  j = %4d  qual_mask = %04o"
                                 "  match_mask = %04o\n",
                                 i, j, quality_mask, match_mask);
#endif
                        (* indel_ct) ++;
                        Global_Indel_SNP_Bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        local_isnp_bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        break;
                      case  -1 :    // delete
#if  SHOW_SNP_DETAILS
                        fprintf (Out_Stream, "SNP  %4d  %4d  %.*s/%.*s-%.*s\n",
                                 i - SIDE_BASES, j - SIDE_BASES,
                                 2 * SIDE_BASES + 1, a + i - 2 * SIDE_BASES,
                                 SIDE_BASES, b + j - 2 * SIDE_BASES + 1,
                                 SIDE_BASES, b + j - SIDE_BASES + 1);
                        fprintf (Out_Stream,
                                 "     i = %4d  j = %4d  qual_mask = %04o"
                                 "  match_mask = %04o\n",
                                 i, j, quality_mask, match_mask);
#endif
                        (* indel_ct) ++;
                        Global_Indel_SNP_Bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        local_isnp_bin
                            [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                        break;
                      case  2 :    // screened
#if  SHOW_SNP_DETAILS
                        fprintf (Out_Stream, "SNP  %4d  %4d  screened\n",
                                 i - SIDE_BASES, j - SIDE_BASES);
#endif
                        break;
                      case  3 :   // too near end
#if  SHOW_SNP_DETAILS
                        fprintf (Out_Stream, "SNP  %4d  %4d  too near end\n",
                                 i - SIDE_BASES, j - SIDE_BASES);
#endif
                        break;
                      default :
                        assert (FALSE);
                     }
                  }
              else if  (match_mask == QUAL_MATCH_MASK)    // perfect match
                  {
                   (* all_match_ct) ++;
                   Global_All_Match_Bin
                       [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                   local_match_bin
                       [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                  }
                else
                  {
                   (* hi_qual_ct) ++;
                   Global_Hi_Qual_Bin
                       [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                   local_other_bin
                       [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                  }
             }

         if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 3)
             {
              (* olap_bases) ++;
              if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 2)
                  (* unscreened_olap_bases) ++;
             }

         i ++;
         j ++;
         n ++;

         while  (i > screen_hi)
           {
            if  (WA -> screen_info . range [screen_sub] . last)
                screen_lo = screen_hi = INT_MAX;
              else
                {
                 screen_sub ++;
                 screen_lo = WA -> screen_info . range [screen_sub] . bgn;
                 screen_hi = WA -> screen_info . range [screen_sub] . end;
                }
#if  SHOW_SNP_DETAILS
            fprintf (Out_Stream, "Screen range: %10d %10d\n",
                     screen_lo, screen_hi);
#endif
           }
        }
      if  (p -> delta [k] > 0)
          {
           match_mask &= INIT_MASK;
           bit = 0;
           match_mask = (match_mask << 1) | bit;

           quality_mask &= INIT_MASK;
           bit = ((a_quality [i] >= MIN_SIDE_QUALITY) ? 1 : 0);
           quality_mask = (quality_mask << 1) | bit;

           if  (i < END_AVOID_LEN
                  || i >= a_len - END_AVOID_LEN
                  || j < END_AVOID_LEN
                  || j >= b_len - END_AVOID_LEN)
               indel_ring [n % (SIDE_BASES + 1)] = 3;  // too near end
           else if  (i < screen_lo)
               indel_ring [n % (SIDE_BASES + 1)] = -1;
             else
               indel_ring [n % (SIDE_BASES + 1)] = 2;  // screened

           quality_ring [n % (SIDE_BASES + 1)]
               = a_quality [i];
           if  (quality_mask == QUAL_MATCH_MASK
                  && indel_ring [(n + 1) % (SIDE_BASES + 1)] < 2)
               {
                (* hi_qual_ct) ++;
                Global_Hi_Qual_Bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                local_other_bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
               }

           if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 3)
               {
                (* olap_bases) ++;
                if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 2)
                    (* unscreened_olap_bases) ++;
               }

           i ++;
           n ++;

           while  (i > screen_hi)
             {
              if  (WA -> screen_info . range [screen_sub] . last)
                  screen_lo = screen_hi = INT_MAX;
                else
                  {
                   screen_sub ++;
                   screen_lo = WA -> screen_info . range [screen_sub] . bgn;
                   screen_hi = WA -> screen_info . range [screen_sub] . end;
                  }
#if  SHOW_SNP_DETAILS
              fprintf (Out_Stream, "Screen range: %10d %10d\n",
                       screen_lo, screen_hi);
#endif
             }
          }
        else
          {
           match_mask &= INIT_MASK;
           bit = 0;
           match_mask = (match_mask << 1) | bit;

           quality_mask &= INIT_MASK;
           bit = ((b_quality [j] >= MIN_SIDE_QUALITY) ? 1 : 0);
           quality_mask = (quality_mask << 1) | bit;

           if  (i < END_AVOID_LEN
                  || i >= a_len - END_AVOID_LEN
                  || j < END_AVOID_LEN
                  || j >= b_len - END_AVOID_LEN)
               indel_ring [n % (SIDE_BASES + 1)] = 3;  // too near end
           else if  (i < screen_lo)
               indel_ring [n % (SIDE_BASES + 1)] = 1;
             else
               indel_ring [n % (SIDE_BASES + 1)] = 2;  // screened

           quality_ring [n % (SIDE_BASES + 1)]
               = b_quality [j];

           j ++;
           n ++;
          }
     }
   while  (i <= a_len - END_AVOID_LEN && j <= b_len - END_AVOID_LEN)
     {
      match_mask &= INIT_MASK;
      bit = ((a [i] == b [j]) ? 1 : 0);
      match_mask = (match_mask << 1) | bit;

      quality_mask &= INIT_MASK;
      bit = ((a_quality [i] >= MIN_SIDE_QUALITY
                && b_quality [j] >= MIN_SIDE_QUALITY) ? 1 : 0);
      quality_mask = (quality_mask << 1) | bit;

      if  (i < END_AVOID_LEN
             || i >= a_len - END_AVOID_LEN
             || j < END_AVOID_LEN
             || j >= b_len - END_AVOID_LEN)
          indel_ring [n % (SIDE_BASES + 1)] = 3;  // too near end
      else if  (i < screen_lo)
          indel_ring [n % (SIDE_BASES + 1)] = 0;
        else
          indel_ring [n % (SIDE_BASES + 1)] = 2;  // screened
      quality_ring [n % (SIDE_BASES + 1)]
          = OVL_Min_int (a_quality [i], b_quality [j]);

      if  (quality_mask == QUAL_MATCH_MASK
             && indel_ring [(n + 1) % (SIDE_BASES + 1)] < 2)
          {
           if  (match_mask == SNP_MATCH_MASK)
               {
                switch  (indel_ring [(n + 1) % (SIDE_BASES + 1)])
                  {
                   case  0 :    // mismatch
#if  SHOW_SNP_DETAILS
                     fprintf (Out_Stream, "SNP  %4d  %4d  %.*s/%.*s\n",
                              i - SIDE_BASES, j - SIDE_BASES,
                              2 * SIDE_BASES + 1, a + i - 2 * SIDE_BASES,
                              2 * SIDE_BASES + 1, b + j - 2 * SIDE_BASES);
                     fprintf (Out_Stream,
                              "     i = %4d  j = %4d  qual_mask = %04o"
                              "  match_mask = %04o\n",
                              i, j, quality_mask, match_mask);
#endif
                     (* mismatch_ct) ++;
                     Global_Match_SNP_Bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     local_msnp_bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     break;
                   case  1 :    // insert
#if  SHOW_SNP_DETAILS
                     fprintf (Out_Stream, "SNP  %4d  %4d  %.*s-%.*s/%.*s\n",
                              i - SIDE_BASES, j - SIDE_BASES,
                              SIDE_BASES, a + i - 2 * SIDE_BASES + 1,
                              SIDE_BASES, a + i - SIDE_BASES + 1,
                              2 * SIDE_BASES + 1, b + j - 2 * SIDE_BASES);
                     fprintf (Out_Stream,
                              "     i = %4d  j = %4d  qual_mask = %04o"
                              "  match_mask = %04o\n",
                              i, j, quality_mask, match_mask);
#endif
                     (* indel_ct) ++;
                     Global_Indel_SNP_Bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     local_isnp_bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     break;
                   case  -1 :    // delete
#if  SHOW_SNP_DETAILS
                     fprintf (Out_Stream, "SNP  %4d  %4d  %.*s/%.*s-%.*s\n",
                              i - SIDE_BASES, j - SIDE_BASES,
                              2 * SIDE_BASES + 1, a + i - 2 * SIDE_BASES,
                              SIDE_BASES, b + j - 2 * SIDE_BASES + 1,
                              SIDE_BASES, b + j - SIDE_BASES + 1);
                     fprintf (Out_Stream,
                              "     i = %4d  j = %4d  qual_mask = %04o"
                              "  match_mask = %04o\n",
                              i, j, quality_mask, match_mask);
#endif
                     (* indel_ct) ++;
                     Global_Indel_SNP_Bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     local_isnp_bin
                         [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                     break;
                   case  2 :    // screened
#if  SHOW_SNP_DETAILS
                     fprintf (Out_Stream, "SNP  %4d  %4d  screened\n",
                              i - SIDE_BASES, j - SIDE_BASES);
#endif
                     break;
                   case  3 :   // too near end
#if  SHOW_SNP_DETAILS
                     fprintf (Out_Stream, "SNP  %4d  %4d  too near end\n",
                              i - SIDE_BASES, j - SIDE_BASES);
#endif
                     break;
                   default :
                     assert (FALSE);
                  }
               }
           else if  (match_mask == QUAL_MATCH_MASK)    // perfect match
               {
                (* all_match_ct) ++;
                Global_All_Match_Bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                local_match_bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
               }
             else
               {
                (* hi_qual_ct) ++;
                Global_Hi_Qual_Bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
                local_other_bin
                    [quality_ring [(n + 1) % (SIDE_BASES + 1)] / 5] ++;
               }
          }

      if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 3)
          {
           (* olap_bases) ++;
           if  (indel_ring [(n + 1) % (SIDE_BASES + 1)] != 2)
               (* unscreened_olap_bases) ++;
          }

      i ++;
      j ++;
      n ++;

      while  (i > screen_hi)
        {
         if  (WA -> screen_info . range [screen_sub] . last)
             screen_lo = screen_hi = INT_MAX;
           else
             {
              screen_sub ++;
              screen_lo = WA -> screen_info . range [screen_sub] . bgn;
              screen_hi = WA -> screen_info . range [screen_sub] . end;
             }
#if  SHOW_SNP_DETAILS
         fprintf (Out_Stream, "Screen range: %10d %10d\n",
                  screen_lo, screen_hi);
#endif
        }
     }

   Global_Olap_Ct += (* olap_bases);
   Global_Unscreened_Ct += (* unscreened_olap_bases);

   return;
  }
#endif



int  Sign
    (int a)

//  Return the algebraic sign of  a .

  {
   if  (a > 0)
       return  1;
   else if  (a < 0)
       return  -1;

   return  0;
  }

