// STORAGE
// 32GB
// FILESYSTEM
// 16GB

#define TRACE true // 1 ssdtrace 0 systor

#define FAR false
#define WAZONE false

#define EC_LIMIT 300
#define ALPHA 0.1

#define NRZONE 256
#define SZZONE 256 // blocks
#define NRBLK (NRZONE*SZZONE)
#define SZBLK 1024 // sector //(512 * 1024) //512KB // 128pages
#define SZPAGE 8 // Secotr //(4 * 1024) //4KB

#define LATENCY_WRITE 200 // micro second 10^-6
#define LATENCY_READ  40 // micro second
#define LATENCY_ERASE 2000//0.015 // 15000 //50000 // micro second
#define US 1000000

#define SZBUF 1024 // sector //(1024 * 1024 * 2) // io_zenfs line 814
#define NRFILE (int64_t) 512 //4 // 512
#define SZFILE (64 * 1024) //sector //(16 * 2 * 1024 * 1024) //(8 * 1024) //(2 * 1024 * 1024)
#define SZFS ((int64_t)(NRFILE*SZFILE))

#define GC_ENABLE true
#define GC_INTERVAL 1000
#define GC_START_LEVEL 20
#define GC_SLOPE 3

#define ENABLE_DYNAMIC_MAPPING		true	
#define ENABLE_GC_WL 		   	true
#define ENABLE_FBL_REFRESH 	   	true

// Block Allocation
#define SHOW_ZONE 		false
// Zone Reset
#define SHOW_RESET		false
// Write data to ZoneFile and Push Extent
#define SHOW_ZONEFILE		false
// Garbage Collection and Reset Before WP
#define SHOW_SIMPLEFS		false
#define SHOW_ERR 		true
#define SHOW_CMD 		false

//typedef long long int int64_t; 
typedef long long int type_latency; 

