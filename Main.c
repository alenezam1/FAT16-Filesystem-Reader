#include "Main.h"

int readData(int fd, off_t offset, void *buffer, size_t count) {
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking in file");
        return -1;
    }

    if (read(fd, buffer, count) != (ssize_t)count) {
        perror("Error reading from file");
        return -1;
    }

    return 0; // Success
}
//Intalizes my Volume Struct
int initializeVolume(const char* imgPath, Volume **volume)
{
    if ((imgPath == NULL) || (volume == NULL))
    {
        return EXIT_FAILURE;
    }

    int fileDs = open(imgPath, O_RDONLY);
    
    if (fileDs == -1) 
    {
        perror("Error opening disk image file");
        exit(EXIT_FAILURE);
    }

    *volume = (Volume*)malloc(sizeof(Volume));

    Volume *v = *volume;
    // Assuming standard sector size of 512 bytes for boot sector
    off_t offset = 0; // Boot sector is at the start of the file
    size_t count = sizeof(BootSector); // Count is the size of BootSector

    v->bootSector = (BootSector*)malloc(count);
    if (readData(fileDs, offset, v->bootSector, count) != 0) {
        close(fileDs);
        exit(EXIT_FAILURE);
    }
   
    // close the image file.
    close(fileDs);
    // Set firstFAT to NULL. It will be populated when FAT is initialized
    v->firstFAT = NULL;

    // Set the image path
    v->imgPath = imgPath;
    
    // Set file data to NULL;
    v->rootDirEntries = 0;

    // Return success
    return EXIT_SUCCESS;
}
// Prints Out the BootSector Info
void displayVolumeInfo(const Volume *volume)
{
    if ((volume != NULL) && (volume->bootSector != NULL))
    {
        BootSector* bs = volume->bootSector;
        char str[12];
        
        // extract volume label
        for (int j = 0; j < 11; j++)
        {
            if (bs->BS_VolLab[j] != ' ')
            {
                str[j] = bs->BS_VolLab[j];
                str[j+1] = 0;
            }
        }
  
        // Output some of the highlighted configuration parameters
        printf("--------------------------------------------\n");
        printf("  Boot Sector Contents: \n");
        printf("    Volume Label:\t\t%s\n", str);
        printf("    Bytes per Sector:\t\t%u\n", bs->BPB_BytsPerSec);
        printf("    Sectors per Cluster:\t%u\n", bs->BPB_SecPerClus);
        printf("    Reserved Sector Count:\t%u\n", bs->BPB_RsvdSecCnt);
        printf("    Number of FATs:\t\t%u\n", bs->BPB_NumFATs);
        printf("    Size of Root Directory:\t%u\n", bs->BPB_RootEntCnt);
        printf("    Total Sectors (16-bit):\t%u\n", bs->BPB_TotSec16);
        printf("    Total Sectors (32-bit):\t%u\n", bs->BPB_TotSec32);
        printf("    Sectors in FAT\t\t%u\n", bs->BPB_FATSz16);      
        printf("--------------------------------------------\n");
        
    }
}
//Intalizes The FAT entries
int initializeFAT(Volume *volume)
{
    if (!(((volume != NULL) && (volume->bootSector != NULL)) || (volume->firstFAT == NULL)))
    {
        perror("Volume is not initialized.");
        return EXIT_FAILURE;
    }

    int fileDs = open(volume->imgPath, O_RDONLY);
    
    if (fileDs == -1) 
    {
        perror("Error opening disk image file");
        return EXIT_FAILURE;
    }
    
    off_t fat_start = (off_t)(volume->bootSector->BPB_RsvdSecCnt * volume->bootSector->BPB_BytsPerSec);
    size_t fat_size = (size_t)(volume->bootSector->BPB_FATSz16 * volume->bootSector->BPB_BytsPerSec);
    
    uint16_t *buffer = (uint16_t*)malloc(fat_size);

    if (readData(fileDs, fat_start, buffer, fat_size) != 0) 
    {
        perror("Error reading FAT");
        
        // close the image file.
        close(fileDs);
    
        return EXIT_FAILURE;
    }

    // close the image file.
    close(fileDs);

    volume->firstFAT = buffer;

    // Set the image data pointer
    volume->rootDirEntries = 0;

    // return success
    return EXIT_SUCCESS;
}
//Traces the Cluster Chain 
void displatFATClusterChain(const Volume *volume, int clusterNumber)
{
    if ((volume != NULL) && (volume->bootSector != NULL) && (volume->firstFAT != NULL) && clusterNumber >= 0)
    {
        uint16_t currentCluster = clusterNumber;
        printf("  Displaying cluster chain starting cluster %u:\n", clusterNumber);
        while (currentCluster < 0xfff8) 
        {
            printf("    [%u] %u\n", currentCluster, volume->firstFAT[currentCluster]);
            currentCluster = volume->firstFAT[currentCluster];
        }
        printf("  End of cluster chain\n--------------------------------------------\n");
    }
}
// Helper Function in the rootDIR
void displayAttributes(uint8_t attr) 
{
    // Attributes order: A, D, V, S, H, R
    printf("    ");
    printf("%c", (attr & 0x20) ? 'A' : '-'); // Archive
    printf("%c", (attr & 0x10) ? 'D' : '-'); // Directory
    printf("%c", (attr & 0x08) ? 'V' : '-'); // Volume label
    printf("%c", (attr & 0x04) ? 'S' : '-'); // System
    printf("%c", (attr & 0x02) ? 'H' : '-'); // Hidden
    printf("%c", (attr & 0x01) ? 'R' : '-'); // Read-only
}

//TASK6 
void readLongFileName(LongDirectoryEntry *lfnEntries, char *lfnBuffer, int *lfnCount) 
{
    int lfnBufferIndex = 0;
    memset(lfnBuffer, 0, LFN_BUFFER_SIZE);
    for (int i = *lfnCount - 1; i >= 0; i--) 
    {
        for (int j = 0; j < 10; j += 2) 
        {
            lfnBuffer[lfnBufferIndex++] = lfnEntries[i].LDIR_Name1[j];
        }
        for (int j = 0; j < 12; j += 2) 
        {
            lfnBuffer[lfnBufferIndex++] = lfnEntries[i].LDIR_Name2[j];
        }
        for (int j = 0; j < 4; j += 2) 
        {
            lfnBuffer[lfnBufferIndex++] = lfnEntries[i].LDIR_Name3[j];
        }
    }
    *lfnCount = 0; // Reset the LFN count
}


//Displays the rootDir entries
void displayDirContents(Volume *volume)
{

    if (!(((volume != NULL) && (volume->bootSector != NULL)) || (volume->firstFAT == NULL)))
    {
        perror("Volume is not initialized.");
        return;
    }


    int fileDs = open(volume->imgPath, O_RDONLY);
    
    if (fileDs == -1) 
    {
        perror("Error opening disk image file");
        
        // close the image file.
        close(fileDs);
        return;
    }

    off_t rootDirStart = (off_t)(volume->bootSector->BPB_RsvdSecCnt + (volume->bootSector->BPB_NumFATs * volume->bootSector->BPB_FATSz16)) * volume->bootSector->BPB_BytsPerSec;
    
    printf("  Directory Contents:\n    %-10s%16s%16s%8s%8s\n", 
            "Attributes", 
            "Last Modified",
            "Cluster",
            "Size",
            "Name");
    
    DirectoryEntry entry;
    LongDirectoryEntry lfnEntries[MAX_LFN_ENTRIES];
    int entryIndex = 0;
    char lfnBuffer[LFN_BUFFER_SIZE];
    int lfnCount = 0;

   
        while (1) 
        {
            uint32_t effecivetAddress = rootDirStart + sizeof(DirectoryEntry) * entryIndex;
            readData(fileDs, effecivetAddress, &entry, sizeof(entry));
    
            if (entry.DIR_Name[0] == 0x00) { // No more entries
                volume->rootDirEntries = entryIndex;
                break;
            }

            if (entry.DIR_Attr == 0x0F) { // LFN entry
                memcpy(&lfnEntries[lfnCount++], &entry, sizeof(LongDirectoryEntry));
                entryIndex++;
                continue;
            }

            displayAttributes(entry.DIR_Attr);
            char dateStr[100];
            int year = ((entry.DIR_WrtDate >> 9) & 0x7F) + 1980;
            int month = (entry.DIR_WrtDate >> 5) & 0x0F;
            int day = entry.DIR_WrtDate & 0x1F;
            int hour = (entry.DIR_WrtTime >> 11) & 0x1F;
            int minute = (entry.DIR_WrtTime >> 5) & 0x3F;
            int second = (entry.DIR_WrtTime & 0x1F) * 2;
            sprintf(dateStr, "%02d-%02d-%04d %02d:%02d:%02d  ", day, month, year, hour, minute, second);
            printf("       %-20s", dateStr);
            uint32_t firstCluster = ((uint32_t)entry.DIR_FstClusHI << 16) | entry.DIR_FstClusLO;
            printf("%8u", firstCluster);

            printf("%8u", entry.DIR_FileSize);

            if (lfnCount > 0) 
            {
                readLongFileName(lfnEntries, lfnBuffer, &lfnCount);
                printf("    %-30s\n", lfnBuffer);
                
            } 
            else 
            {
                int dotPrinted = 0;
                printf("    "); // Additional spacing for alignment
                for (int i = 0; i < 11; ++i) 
                {
                    if (i == 8 && !dotPrinted) 
                    {
                        for (int j = 8; j < 11; ++j) 
                        {
                            if (entry.DIR_Name[j] != ' ') 
                            {
                                printf(".");
                                dotPrinted = 1;
                                break;
                            }
                        }
                    }
                    if (entry.DIR_Name[i] != ' ') 
                    {
                        printf("%c", entry.DIR_Name[i]);
                    }
                }
                printf("\n");
            }
            entryIndex++;
        }

    printf("\n--------------------------------------------\n");
    // close the image file.
    close(fileDs);
    }

int FindEntry(const char *dirName, ShortDirEntry* dirEntry, const Volume *volume)
{
    
    int fileDs = open(volume->imgPath, O_RDONLY);
    
    if (fileDs == -1) 
    {
        perror("Error opening disk image file");
        
        // close the image file.
        close(fileDs);
        return EXIT_FAILURE;
    }
 
    int found = 0;

    off_t rootDirStart = (off_t)(volume->bootSector->BPB_RsvdSecCnt + (volume->bootSector->BPB_NumFATs * volume->bootSector->BPB_FATSz16)) * volume->bootSector->BPB_BytsPerSec;
    
    DirectoryEntry entry;
    LongDirectoryEntry lfnEntries[MAX_LFN_ENTRIES];
    int entryIndex = 0;
    char lfnBuffer[LFN_BUFFER_SIZE];
    int lfnCount = 0;

    while (!found) 
    {
        readData(fileDs, rootDirStart + sizeof(DirectoryEntry) * entryIndex, &entry, sizeof(entry));
  
        if (entry.DIR_Name[0] == 0x00) 
        { // No more entries
            break;
        }

        if (entry.DIR_Attr == 0x0F) 
        { // LFN entry
            memcpy(&lfnEntries[lfnCount++], &entry, sizeof(LongDirectoryEntry));
            entryIndex++;
            continue;
        }

        if (lfnCount > 0) 
        {
            readLongFileName(lfnEntries, lfnBuffer, &lfnCount);
            if (strcmp(dirName, lfnBuffer) == 0)
            {
                found = 1;
            } 
            
        } 
        else 
        {
            int dotPrinted = 0;
            char fname[12];
            int i;
            for (i = 0; i < 11; ++i) 
            {
                if (i == 8 && !dotPrinted) 
                {
                    for (int j = 8; j < 11; ++j) 
                    {
                        if (entry.DIR_Name[j] != ' ') 
                        {
                            fname[i] = '.';
                            dotPrinted = 1;
                            break;
                        }
                    }
                }
                if (entry.DIR_Name[i] != ' ') 
                {
                    fname[i] = entry.DIR_Name[i];
                }
            }
            
            fname[i] = 0;
            
            if (strncmp(dirName, fname, strlen(dirName)) == 0)
            {
                found = 1;
            }
        }
        entryIndex++;
    }

    close(fileDs);

    if (found){
        memcpy(dirEntry, &entry, sizeof(ShortDirEntry));
        return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

File* openFile(Volume *volume, ShortDirEntry *entry)
{
    if (!(((volume != NULL) && (volume->bootSector != NULL)) || (volume->firstFAT == NULL)))
    {
        perror("Volume is not initialized.");
        return NULL;
    }
    File *file = (File *)malloc(sizeof(File));
    if (!file) {
        perror("Error allocating File structure");
        return NULL;
    }

    file->fileSize = entry->DIR_FileSize;
    file->startCluster = ((uint32_t)entry->DIR_FstClusHI << 16) | entry->DIR_FstClusLO;
    file->currentPosition = 0;
    file->volume = volume;
    file->fatTable = volume->firstFAT;
    file->data = NULL; // Data will be loaded in readFile

    return file;
}

off_t seekFile(File *file, off_t offset, int whence) 
{
    if (!file) {
        perror("File not open.");
        return -1;
    }

    off_t new_pos;

    switch (whence) {
        case SEEK_SET:
            new_pos = offset;
            break;
        case SEEK_CUR:
            new_pos = file->currentPosition + offset;
            break;
        case SEEK_END:
            new_pos = file->fileSize + offset;
            break;
        default:
            perror("Incorrect whence.");
            return -1;
    }

    if (new_pos < 0 || new_pos > file->fileSize) {
        perror("New position out of bound.");
        return -1;
    }

    file->currentPosition = new_pos;
  
    return new_pos;
}

size_t readFile(File *file, void *buffer, size_t length) 
{

    int fileDs = open(file->volume->imgPath, O_RDONLY);
    
    if (fileDs == -1) 
    {
        perror("Error opening disk image file");
        
        // close the image file.
        close(fileDs);
        return 0;
    }

    // Load file data if not already loaded
    if (file->data == NULL) 
    {
        file->data = (uint8_t *)malloc(file->fileSize);
        if (file->data == NULL) 
        {
            perror("Error allocating memory for file data");
            return 0;
        }
        
        uint16_t currentCluster = file->startCluster;
        uint32_t reservedSectors = file->volume->bootSector->BPB_RsvdSecCnt;
        uint32_t fatSectors = (file->volume->bootSector->BPB_NumFATs * file->volume->bootSector->BPB_FATSz16);
        uint32_t reservedSectorsOffset = reservedSectors * file->volume->bootSector->BPB_BytsPerSec;
        uint32_t fatSectorsOffset = fatSectors * file->volume->bootSector->BPB_BytsPerSec;
        uint32_t rootDirBytes = (file->volume->bootSector->BPB_RootEntCnt * sizeof(DirectoryEntry));
        
        while (currentCluster < 0xfff8)
        {

            uint32_t dataOffset = ((currentCluster - 2) * file->volume->bootSector->BPB_SecPerClus) * file->volume->bootSector->BPB_BytsPerSec;
            uint32_t dataStart = reservedSectorsOffset + fatSectorsOffset + rootDirBytes;
            uint32_t effectiveAddress = dataStart + dataOffset;

            // Read the file data into the buffer
            if (lseek(fileDs, effectiveAddress, SEEK_SET) == -1) 
            {
                perror("Error seeking to image data position");
                free(file->data);
                file->data = NULL;
                return 0;
            }

            if (read(fileDs, file->data, file->fileSize) != file->fileSize) {
                perror("Error reading file data");
                free(file->data);
                file->data = NULL;
                return 0;
            }

            currentCluster = file->volume->firstFAT[currentCluster];
        }
    
    }

    // Adjust read length if it exceeds the remaining part of the file
    size_t remaining = file->fileSize - file->currentPosition;
    if (length > remaining) {
        length = remaining;
    }

    // Perform the read operation
    memcpy(buffer, file->data + file->currentPosition, length);

    // Update the current file position
    file->currentPosition += length;
    
    // close the image file.
    close(fileDs);
        
    // Return the number of bytes read
    return length;
}

void closeFile(File *file)
{
    if (file != NULL)
    {
        if (file->data != NULL)
        {
            free(file->data);
        }

        free(file);
    }
    
}

int main() 
{    
    char *imgPath = "fat16.img"; // Can change file name here (MAKE SURE .IMG FILE IS IN THE SAME FOLDER)
    uint16_t clusterNumber = 0;
    Volume *volume;

    /*  Task 1 and Task 2a: 
     *    Reading image path and populating Volume and BootSector data structures. 
     */
    if (initializeVolume(imgPath, &volume) == EXIT_FAILURE)
    {
        perror("Volume Initialization Failed");
        exit(EXIT_FAILURE);
    }

    /*  Task 2b: 
     *    Displaying BootSector information from read file 
     */ 
    displayVolumeInfo(volume);

    /*  Task 3a: 
     *    Load first FAT into memory   
     */
    if (initializeFAT(volume) == EXIT_FAILURE)
    {
        perror("FAT Initialization Failed");
        exit(EXIT_FAILURE);
    }

    /*  Task 3b: 
     *    Display FAT Cluster Chain starting from clusterNumber   
     */
    printf("Enter the starting cluster number:\n");
    scanf("%hu",&clusterNumber);
    displatFATClusterChain(volume, clusterNumber);

    /*  Task 4 and 6: 
     *    Display Root Directory Contents alongwith long file names supported   
     */
    displayDirContents(volume); 
    
    /*  Task 5: 
     *    Display contents of a file. We will display 
     *    contents of file called sessions.txt    
     */
    const char* filename = "sessions.txt";
    ShortDirEntry entry;

    if (FindEntry(filename, &entry, volume) == EXIT_SUCCESS)
    {
        printf("  File: %s found. Displaying contents:\n\n", filename);
        
        File* flptr = NULL;
        flptr = openFile(volume, &entry);

        if (flptr)
        {
            uint8_t* buffer = (uint8_t*)malloc(flptr->fileSize);

            if (seekFile(flptr, 0, SEEK_SET) >= 0)
            {
                if (readFile(flptr, buffer, flptr->fileSize) > 0)
                {
                    printf("%s", buffer);
                }

            }
            else
            {
                printf("Unable to seek to required position\n");
            }

            closeFile(flptr);
        }
        else
        {
            printf("Unable to open file: %s\n", filename);
        }
        
    }
    else
    {
        printf("File: %s not found\n", filename);
        
    }

   
    return EXIT_SUCCESS;
} 