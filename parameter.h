// STORAGE
// 64K
// FILESYSTEM
// 32K

#define EC_LIMIT 100

#define NRZONE 1024 //16
#define SZZONE 512 //8
#define NRBLK (NRZONE*SZZONE)
#define SZBLK (2*1024*1024) //2MB //512
#define SZPAGE -1

#define LATENCY_WRITE -1
#define LATENCY_READ -1
#define LATENCY_ERASE -1

#define SZBUF 2048
#define NRFILE 4
#define SZFILE (1024 * 8)
#define SZFS (NRFILE*SZFILE)

#define GC_ENABLE true
#define GC_INTERVAL 10
#define GC_START_LEVEL 20
#define GC_SLOPE 3

#define DYNAMIC_MAPPING false
#define ENABLE_GC_WL false
#define ENABLE_FBL_REFRESH false

#define SHOW_ZONE false // Block Allocation
#define SHOW_RESET false // Zone Reset
#define SHOW_ZONEFILE false // Write data to ZoneFile and Push Extent
#define SHOW_SIMPLEFS false // Garbage Collection and Reset Before WP
#define SHOW_ERR true
#define SHOW_CMD false

typedef long long int64_t; 
