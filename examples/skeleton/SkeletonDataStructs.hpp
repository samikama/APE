#ifndef __SKELETON_DATA_STRUCTS__
#define __SKELETON_DATA_STRUCTS__

typedef GlobalData int;// use your own global data structure
typedef MultiEventData double;// structure to keep multi-event data
typedef EventSpecificData float;//Structure to keep scratch spaces
typedef EventSpecificDataWork2 float;//Structure to keep scratch spaces
enum WORKTYPES{ALGORITHM1=1,
	       ALGORITHM2=2,
	       UPDATECONDITIONS=3,
	       UPLOADGLOBALDATA=4};
struct CONDITIONSDATA{
  int LB;
  MultiEventData REALCONDITIONS;
};
#endif
