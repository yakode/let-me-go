// STORAGE
// 64K
// FILESYSTEM
// 32K

#define NRZONE 16 // 1024 //16
#define SZZONE 8 //(512/SMALL) // 512 //8
#define NRBLK (NRZONE*SZZONE)
#define SZBLK 512 //(2*1024*1024) //2MB //512

#define SZBUF 2048
#define NRFILE 4
#define SZFILE (1024 * 8)
#define SZFS (NRFILE*SZFILE)

#define GC_ENABLE true
#define GC_INTERVAL 10
#define GC_START_LEVEL 20
#define GC_SLOPE 5  //3

#define SHOW_ZONE true // Block Allocation
#define SHOW_RESET true // Zone Reset
#define SHOW_ZONEFILE true // Write data to ZoneFile and Push Extent
#define SHOW_SIMPLEFS true // Garbage Collection and Reset Before WP
#define SHOW_ERR true

#define YAK false
