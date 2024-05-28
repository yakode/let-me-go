// STORAGE
// 1TB
// FILESYSTEM
// 1GB

#define EC_LIMIT 500

#define NRZONE 1024
#define SZZONE 512 // blocks
#define NRBLK (NRZONE*SZZONE)
#define SZBLK (2*1024*1024) //2MB // 128pages
#define SZPAGE (16*1024) //16KB

#define LATENCY_WRITE 1500 //400 // micro second 10^-6
#define LATENCY_READ 100 //80 // micro second
#define LATENCY_ERASE 15000 //50000 // micro second
#define US 1000000

#define SZBUF 2048
#define NRFILE 4 // 512
#define SZFILE (8 * 1024) //(2 * 1024 * 1024)
#define SZFS (NRFILE*SZFILE)

#define GC_ENABLE true
#define GC_INTERVAL 1000
#define GC_START_LEVEL 20
#define GC_SLOPE 3

#define ENABLE_DYNAMIC_MAPPING false	
#define ENABLE_GC_WL 		   false 
#define ENABLE_FBL_REFRESH 	   false 

#define SHOW_ZONE 		false // Block Allocation
#define SHOW_RESET		false // Zone Reset
#define SHOW_ZONEFILE	false // Write data to ZoneFile and Push Extent
#define SHOW_SIMPLEFS	false // Garbage Collection and Reset Before WP
#define SHOW_ERR 		false
#define SHOW_CMD 		false

typedef long long int64_t; 
