#define NRZONE 16
#define SZZONE 8
#define NRBLK (NRZONE*SZZONE)
#define SZBLK 512//2*1024*1024 //2MB

#define NRFILE 40
#define SZFILE 512
#define SZFS (NRFILE*SZFILE)

#define GC_ENABLE true
#define GC_INTERVAL 10
#define GC_START_LEVEL 20
#define GC_SLOPE 3

#define SHOW_ZONE false // Block Allocation
#define SHOW_RESET true // Zone Reset
#define SHOW_ZONEFILE false // Write data to ZoneFile and Push Extent
#define SHOW_SIMPLEFS true // Garbage Collection and Reset Before WP
#define SHOW_ERR true
#define SHOW_INFO false

#define YAK false
