////////////////////////////////////////
//
//  Library of reads.
//
//  You can add boolean flags and small-value types to the 64-bit-wide
//  bit-vectors immediately below.
//
//  IMPORTANT!  The default value of these should be 0.
//
//  This can make for some twisted logic, but consider adding an
//  on/off flag like doNotQVTrim and goodBadQVThreshold do -- if
//  doNotQVTrim is set, the value of goodBadQVThreshold is ignored,
//  otherwise, it is used.  Further, goodBadQVThreshold == 0 is
//  interpreted as the 'default' value (set in the client code),
//
//  IMPORTANT!  Try not to distrub the order.  It should be OK to add
//  new fields by stealing bits from the 'spare' field, and creating
//  the new field just after the spare.  The existing named fields
//  should be left in the same order and location.  (Though, adding
//  fields and remaining compatible with an existing store has not
//  actually been tested.)
//
//  When adding new fields, please update:
//    AS_GKP/AS_GKP_edit.C      -- to add edit interfaces
//    AS_PER/AS_PER_gkLibrary.C -- to add encoding/decoding of features
//    AS_PER/gkLibrary.H        -- this file
//
//  You need to also update clients so they know about the new
//  feature.  In general, only clients that act on a feature need to
//  know about it.
//
class gkLibrary {
public:
  gkLibrary() {
    libraryUID = AS_UID_undefined();
    mean       = 0.0;
    stddev     = 0.0;

    gkLibrary_clearFeatures();
  };
  ~gkLibrary() {
  };

  //  AS_MSG reads protoIO, turns a library into a LibraryMesg, which
  //  has the features and values strings.
  //
  //  AS_GKP receives the LibraryMesg, checks sanity, and converts it
  //  into gkLibrary.  It uses the functions below to populate the
  //  GKLR.
  //
  void gkLibrary_decodeFeatures(LibraryMesg *lmesg);
  void gkLibrary_encodeFeatures(LibraryMesg *lmesg);
  void gkLibrary_encodeFeaturesCleanup(LibraryMesg *lmesg);

  void gkLibrary_clearFeatures(void) {
    spareUN3                  = 0;
    spareUN2                  = 0;
    spareUN1                  = 0;
    spareUTG                  = 0;
    forceBOGunitigger         = 0;
    isNotRandom               = 0;
    spareALN                  = 0;
    doNotTrustHomopolymerRuns = 0;
    spareOBT                  = 0;
    doRemoveDuplicateReads    = 0;
    doNotQVTrim               = 0;
    goodBadQVThreshold        = 0;
    doNotOverlapTrim          = 0;
    spareGKP                  = 0;
    usePackedFragments        = 0;
    spareLIB                  = 0;
    orientation               = 0;
  };

private:
public:
  AS_UID         libraryUID;

  double         mean;
  double         stddev;

  //  Spare bits for new modules, one word for each
  uint64         spareUN3;
  uint64         spareUN2;
  uint64         spareUN1;

  //  Unitigger options
  uint64         spareUTG:62;                  //  Spare for unitigger changes
  uint64         forceBOGunitigger:1;          //  Default 0 -> allow any unitigger
  uint64         isNotRandom:1;                //  Default 0 -> fragments in here are random

  //  Alignment options (aka, consensus, but also overlapper)
  uint64         spareALN:63;                  //  Spare for alignment changes
  uint64         doNotTrustHomopolymerRuns:1;  //  Default 0 -> trust 'em

  //  OBT options
  uint64         spareOBT:55;                  //  Spare for OBT changes
  uint64         doRemoveDuplicateReads:1;     //  Default 0 -> do not search for and remove duplicates
  uint64         doNotQVTrim:1;                //  Default 0 -> do initial qv trimming in OBT
  uint64         goodBadQVThreshold:6;         //  Default 0 -> threshold is 12 in initialTrim.C
  uint64         doNotOverlapTrim:1;           //  Default 0 -> do trimming

  //  Gatekeeper options
  uint64         spareGKP:63;                  //  Spare for gatekeeper changes
  uint64         usePackedFragments:1;         //  Default 0 -> don't store gkPackedFragment types

  //  Library options
  uint64         spareLIB:61;                  //  Spare for library changes
  uint64         orientation:3;                //  Default 0 -> AS_READ_ORIENT_UNKNOWN
};

