#ifndef _MAIN_H
#define  _MAIN_H

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define END_OF_FILE_CLUSTER 0xFFF8
#define MAX_LFN_ENTRIES 20
#define LFN_BUFFER_SIZE 260

typedef struct __attribute__((__packed__))
{
    uint8_t BS_jmpBoot[3];    // x86 jump instr. to boot code
    uint8_t BS_OEMName[8];    // What created the filesystem
    uint16_t BPB_BytsPerSec;  // Bytes per Sector
    uint8_t BPB_SecPerClus;   // Sectors per Cluster
    uint16_t BPB_RsvdSecCnt;  // Reserved Sector Count
    uint8_t BPB_NumFATs;      // Number of copies of FAT
    uint16_t BPB_RootEntCnt;  // FAT12/FAT16: size of root DIR
    uint16_t BPB_TotSec16;    // Sectors, may be 0, see below
    uint8_t BPB_Media;        // Media type, e.g. fixed
    uint16_t BPB_FATSz16;     // Sectors in FAT (FAT12 or FAT16)
    uint16_t BPB_SecPerTrk;   // Sectors per Track
    uint16_t BPB_NumHeads;    // Number of heads in disk
    uint32_t BPB_HiddSec;     // Hidden Sector count
    uint32_t BPB_TotSec32;    // Sectors if BPB_TotSec16 == 0
    uint8_t BS_DrvNum;        // 0 = floppy, 0x80 = hard disk
    uint8_t BS_Reserved1;   
    uint8_t BS_BootSig;       // Should = 0x29
    uint32_t BS_VolID;        // 'Unique' ID for volume
    uint8_t BS_VolLab[11];    // Non zero terminated string
    uint8_t BS_FilSysType[8]; // e.g. 'FAT16 ' (Not 0 term.)
} BootSector;

typedef struct __attribute__((__packed__)) {
    uint8_t DIR_Name[11];
    uint8_t DIR_Attr;
    uint8_t DIR_NTRes;
    uint8_t DIR_CrtTimeTenth;
    uint16_t DIR_CrtTime;
    uint16_t DIR_CrtDate;
    uint16_t DIR_LstAccDate;
    uint16_t DIR_FstClusHI;
    uint16_t DIR_WrtTime;
    uint16_t DIR_WrtDate;
    uint16_t DIR_FstClusLO;
    uint32_t DIR_FileSize;
} DirectoryEntry;


typedef struct __attribute__((__packed__)) {
    uint8_t LDIR_Ord;
    uint8_t LDIR_Name1[10];
    uint8_t LDIR_Attr;
    uint8_t LDIR_Type;
    uint8_t LDIR_Chksum;
    uint8_t LDIR_Name2[12];
    uint16_t LDIR_FstClusLO;
    uint8_t LDIR_Name3[4];
} LongDirectoryEntry;

typedef struct _volume {
    BootSector *bootSector;  // Pointer (largest member first)
    uint16_t *firstFAT;      // Pointer
    const char *imgPath;     // Pointer
    uint16_t rootDirEntries; // 16-bit integer (smallest member last) 
    char pad[6];   // Pointer
} Volume;

typedef struct _file{
    int fd;                    // File descriptor for the FAT16 image
    uint32_t startCluster;     // Starting cluster of the file in the FAT16 filesystem
    uint32_t currentCluster;   // Current cluster being accessed
    uint32_t fileSize;         // Size of the file
    uint32_t currentPosition;  // Current position in the file
    uint32_t dataStartSector;  // The starting sector of the file data
    uint8_t *data;             // Pointer to the file data buffer
    uint16_t *fatTable; 
    Volume *volume;        // Pointer to the FAT table
    // ... other fields as required
} File;


typedef struct {
    char DIR_Name[11];         // File name, 8 chars + 3 chars extension
    uint8_t DIR_Attr;          // File attributes
    uint8_t DIR_NTRes;         // Reserved for use by Windows NT
    uint8_t DIR_CrtTimeTenth;  // Millisecond stamp at file creation time
    uint16_t DIR_CrtTime;      // Time file was created
    uint16_t DIR_CrtDate;      // Date file was created
    uint16_t DIR_LstAccDate;   // Last access date
    uint16_t DIR_FstClusHI;    // High word of this entry's first cluster number (0 for FAT12/16)
    uint16_t DIR_WrtTime;      // Time of last write
    uint16_t DIR_WrtDate;      // Date of last write
    uint16_t DIR_FstClusLO;    // Low word of this entry's first cluster number
    uint32_t DIR_FileSize;     // Size of the file in bytes
} ShortDirEntry;

int initializeVolume(const char* imgPath, Volume **volume);
void displayVolumeInfo(const Volume *volume);
int initializeFAT(Volume *volume);
void displayFATClusterChain(const Volume *volume, int clusterNumber);
void displayDirContents(Volume *volume); 
int readData(int fd, off_t offset, void *buffer, size_t count);
void displayAttributes(uint8_t attr);
void readLongFileName(LongDirectoryEntry *lfnEntries, char *lfnBuffer, int *lfnCount);
int FindEntry(const char *dirName, ShortDirEntry* dirEntry, const Volume *volume);
File* openFile(Volume *volume, ShortDirEntry *entry);
off_t seekFile(File *file, off_t offset, int whence);
size_t readFile(File *file, void *buffer, size_t length);
void closeFile(File *file);
#endif 
