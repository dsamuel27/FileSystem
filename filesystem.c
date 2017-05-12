//
//  filesystem.c
//  
//
//  Created by David Samuel on 2017-04-10.
//
//


#include <stdio.h>
#include <time.h> // for time functions
#include <string.h>
#include <stdlib.h>

/******************************************************************************************************************/
                                    /*  OBJECTIVE   */
/******************************************************************************************************************/

/*
Design a file system that has a directory structure, which can store subdirectories and files with the following requirements:

-	Directory and filenames have to be a maximum of 8 characters, and filenames can have a 3 characters extension.
-	Maximum file size is 32768 bytes
-	Block size (allocation units) for the file system must be 512 bytes
-	All the directories must be linked in some way.
-	Every entry in a directory must have a time stamp indicating the last time the entry was modified, and the time it was created.
-	A logical directory that describes files and attributes/meta data
-	A physical directory that organizes blocks allocated to each file.

The file system must also have basic input/output control system functions. The input/output control system functions include:

-	Create file
-	Open file
-	Delete file
-	Close file
-	Read file
-	Write file

Lastly, an application must be made that can demonstrate the functionality of the created file system.

 */

/******************************************************************************************************************/
                                /*  NOTES FOR SELF   */
/******************************************************************************************************************/

// THE filesystem is split into 4 regions/

// The ROOT DIR ADDR WILL BE ROOT_START_INDEX * 512


/******************************************************************************************************************/
                              /*  MACROS, GLOBAL VARIABLES & FUNCTION DECLARATIONS   */
/******************************************************************************************************************/

# define BLOCK_SIZE 512 // This macro will define the block size. Will be useful when navigating the file system.

# define TOTAL_BLOCKS 5859 // This macro will define the total number of blocks in the file system.

# define FAT_REGION_START 1 // this macro defines the index where the fat region starts. The value is one because index 0 is reserved for the info block region.

# define FAT_REGION_END 22 // This macro will define where the fat region ends. It has the value of 23, since 23 – 1 = 22. 22 is the number of blocks allocated to the FAT region.

# define ROOT_START_INDEX 23 // This macro determines where the Root Directory region starts

# define ROOT_END_INDEX 227 // This macro determines where the root directory region ends

# define DATA_REGION_START 228 // This macro determines where the data region starts.

#define MAX_FILE_SIZE  32768 // max file size


// Root Region Offsets

/*
# define ROOT_FLAG_OFFSET 0
# define ROOT_FAT_ADDR_OFFSET 4
# define ROOT_NAME_OFFSET 8
# define ROOT_EXT_OFFSET 12
# define ROOT_CREATION_TIME_OFFSET 16
# define ROOT_MOD_TIME_OFFSET 20
# define ROOT_PARENT_OFFSET 24
# define ROOT_OPEN_OFFSET 32
*/

FILE *virtual_disk; // This File variable represents the virtual disk


//The commented out struct below will be used as reference for directory entries
/*
struct root_directory_entry { // This struct represents an entry in the root directory

    char flag; //(size: 1 bytes)  this flag will determine if the entry is a file or directory d = directory, f = file, null character = empty entry.
    short open; // (size: 2 bytes) this flag will tell you if a file is open or closes. //Directories will always be open.
    short FAT_address; //(size: 2 bytes) this variable will point to starting address in the FAT
    char name[8]; //(size: 8 bytes)  this array will hold the name of a file/directory
    char ext[3]; //(size: 3 bytes)   this array will hold the extension of a file, if there is one.
    int creation_year; //(size: 4 bytes)   this variable will hold the value of the creation year.
    int creation_month; //(size: 4 bytes)  this variable will hold the value of the creation month.
    int creation_day; //(size: 4 bytes)  this variable will hold the value of the creation day.
    int mod_year; //(size: 4 bytes)   this variable will hold the value of the mod year.
    int mod_month; //(size: 4 bytes)   this variable will hold the value of the mod month.
    int mod_day; //(size: 4 bytes)   this variable will hold the value of the mod day.
    int parent; //(size: 4 bytes)   Holds the index of the parent directory
    int size; // size: 4 bytes  Represents number of blocks allocated to a file.
    
};  // total size of this structure is 48 bytes :) */

// explanations regarding these function are below.
void Mount();
void UnMount();
void Format();
int search_Root_Dir(char *name, char *ext);
int search_Root_Dir(char *name, char *ext);
int check_path( char *path);
int search_empty_Root_dir();
int search_empty_FAT_entry();
void create_root_dir_entry(int root_offset, int parent_value, char *name, char *ext, short Fat_addr);
int create_file(char *path, char *name);
int open_file(char *path,char *name);



/******************************************************************************************************************/
                                    /*  FUNCTION IMPLEMENTATION  */
/******************************************************************************************************************/


// Opens the virtual disk
void Mount(){
    virtual_disk = fopen("Drive3MB","r+");
    
}

// closes the virtual disk
void UnMount(){
    
    fclose(virtual_disk);
}



// this function will set format our virtual disk into a empty filesystem.
void Format() {
    
    char zeros[512]; // an array of all 0's, To be used to set regions with default values 0
    signed short fat_empty_entry = -2;
    memset(zeros,0,BLOCK_SIZE);
    int i; int j = 0;
    struct tm *t_m; // this struct stores variables related to time. Such as year, month, date.
    time_t time_of_format;
    short year, month, day;
    
    // first set info region to default values
     fwrite(zeros,1,sizeof(zeros),virtual_disk); // Fwrite will also move the file pointer 512 bytes towards the end of the file.
    
    // Due to the fwrite, we are now in the FAT region. Must set FAT region values to -2, to indicate unallocated entries. Total of 5632 entries.
    
    while ( j < 22) {
        
        for(i = 0; i<(BLOCK_SIZE/2);i++) { // each entry is 2 bytes. Therefore 512/2 entries in one block for FAT.
        fwrite(&fat_empty_entry,1,sizeof(fat_empty_entry),virtual_disk);
        // fwrite advances the File pointer like f seek.
            
    }
        j++;
    }
    
    // Now the File Pointer is in the root directory region. The root directory is made up of 204 blocks. Set all these blocks equal to the null character.
    
    j = 0;
    
    while ( j<204) {
        
         fwrite(zeros,1,sizeof(zeros),virtual_disk);
        j++;
    }
    
    // Now the File Pointer is in the data region. The root directory is made up of 5632 blocks. Set all these blocks equal to the null character.

    j = 0;
    while ( j<5632) {
        
        fwrite(zeros,1,sizeof(zeros),virtual_disk);
        j++;
    }
    
    

    fseek(virtual_disk,0,SEEK_SET);
    
    time_of_format = time(NULL);
    t_m = localtime(&time_of_format);
    
    // have to cast values in struct to short
    year = (short) (t_m->tm_year + 1900); // adding 1900 because date the year var in the stuct represents number of years from 1900.
    day = (short)t_m->tm_mday ;
    month = (short)(t_m->tm_mon+1); // adding one because month in struct are stored with index of 0.
    
    printf("The date the disk was formatted was %d/%d/%d\n",month,day,year);
    
    fwrite(&month,1,sizeof(month),virtual_disk);
    
    fwrite(&day,1,sizeof(day),virtual_disk);
    
    fwrite(&year,1,sizeof(day),virtual_disk);

    
    
}
           

// This function will determine if a given path is valid, and returns the address of the lowest subdirectory
// provided on success.
// Path is in the form /ROOT/subdirectoryA/subdirectoryB/.../
int check_path( char *path) {
    
    int flag = 0; // this flag will be 1 if we have a valid path. Otherwise 0.
    int lowest_subdir_addr; // this variable will store the address of the lowest subdirectory.
    
    // copy the path argument into the path_copy variable.
    char *path_copy = calloc(1, sizeof(char*)*50);
    strcpy(path_copy,path);
    
    // the backslash is the delimeter for the path provided by client.
    char *delimeter = "/";
    char slash = '/';
    int slash_count = 0;
    int j = 0;
    
    // count the number of slashes
    while (j < sizeof(path_copy)) {
        if (path_copy[j] == slash) slash_count++;
        j++;
    }
    if (slash_count <= 1) return -1; //invalid entry. At the very least there must be 2 backslashes.
    
    // There will be always be one more slash than the number of directories provided.
    int num_of_dir =slash_count - 1;
    
    // initialize an array that will hold all the directories provided in path.
    char *arrayOfDirs[num_of_dir];
    
    // will break up path using strtok.
    char *token = strtok(path_copy,delimeter);
    
    j = 0;
    while (token != NULL) {
        arrayOfDirs[j] = token;
        
        token = strtok(NULL,delimeter);
        j++;
    }
    
    j = j - 1; // subtracted one because j iterates once more through the loop. This is the number of elements in the array with index of 0.
    
   
    
    if ( j == 0) { //if only one entry then it's most likely the root directory. Return address that points to beginning of ROOT region.
        
        if (strcmp(arrayOfDirs[j],"ROOT") == 0) { // Making sure the one entry is the ROOT.
            return ROOT_START_INDEX * BLOCK_SIZE;
        }
    } else { // there is more than one directory in the given path
        // Have to check if the given path is correct. Does each subdirectory have the correct parent?
        
        int k = 0; // initialize this to keep track of lowest subdir
        while ( j > 0 ) {
            
            int does_dir_exist = search_Root_Dir(arrayOfDirs[j]," "); //check to see if given directory exists
            
            if (does_dir_exist == -1) return -1; // failed to find directory, therefore invalid path
            
            // If we reach here, than the directory does exist. is it given the correct parent?
            
            // lets get the address of the parent
            
            int does_parent_exist;
            
            if ( (j-1) == 0) { // this is the case where the parent is the ROOT.
                if (strcmp(arrayOfDirs[j-1],"ROOT") == 0) {
                    does_parent_exist = ROOT_START_INDEX * BLOCK_SIZE;
                }else {
                    return -1;
                }
        
            } else { // search for the parent directory with search_Root_Dir. search_Root_Dir will return address of parent directory/
             does_parent_exist = search_Root_Dir(arrayOfDirs[j-1]," ");
           
            if (does_parent_exist == -1) return -1; // the parent doesn't exist, therefore invalid
            }
            // If we get to this point then the parent is a valid directory, now we must check if the parent is linked with the subdirectory given.
            
            int buffer;
             fseek(virtual_disk,does_dir_exist+40,SEEK_SET); // seek to the subdirectory's parent section
            fread(&buffer,sizeof(buffer),1,virtual_disk);
            
            //printf("%d\n",does_parent_exist);
            
            if ( does_parent_exist == buffer) {flag = 1; if(k==0){lowest_subdir_addr = does_dir_exist;}} // Addresses match therefore we have the correct parent.
            else { flag = 0;}
            
            j--; k++;
            
        }
    }
            
    if (flag == 1) return lowest_subdir_addr;
    return -1; // Otherwise return -1
 
}

// This function will check to see if the parameter name is in the root directory.
int search_Root_Dir(char *name, char *ext) {
    
    
   
    int valid = 0; //flag that tells us if we found a match.
    int flag_for_directory = 0; // flag to tell us if we're working with a file or directory.
    
    // buffers to be used by fread.
    char valid_entry_buf;
    char valid_name_buf[9];
    memset(valid_name_buf,0,9);
    char valid_ext_buf[4];
    memset(valid_ext_buf,0,4);
    
    // checking to see if we're working with a directory
    if (strcmp(ext," ") == 0) flag_for_directory = 1; // In my implementation, directories don't have extensions.
    
    // initialize index starting at Root Region Start.
    int curr_index = ROOT_START_INDEX * BLOCK_SIZE;
    
  
    
    // searching for a directory
    if (flag_for_directory == 1) {
       
        while(curr_index <= ((DATA_REGION_START*BLOCK_SIZE)-48)) {
            
        fseek(virtual_disk,curr_index,SEEK_SET);
            
       fread(&valid_entry_buf,sizeof(valid_entry_buf),1,virtual_disk); // fread will move the file pointer one byte towards the end
          
        if(valid_entry_buf == 'd') {
            
            fseek(virtual_disk,curr_index+5,SEEK_SET); // go to the section holding the name of directory.
            fread(valid_name_buf,sizeof(valid_name_buf)-1,1,virtual_disk);
            
            // determine if name matches
            if (strcmp(valid_name_buf,name) == 0) {
                valid = 1;
                break; // if name matches change flag and break.
            
            }
        }
             curr_index = curr_index + 32;
        }
        
    } else { // looking for a file.
        
        
        while(curr_index <= ((DATA_REGION_START*BLOCK_SIZE)-32)) {
            fseek(virtual_disk,curr_index,SEEK_SET);
            
            fread(&valid_entry_buf,sizeof(valid_entry_buf),1,virtual_disk); // fread will move the file pointer one byte towards the end
            if(valid_entry_buf == 'f') {
                
                fseek(virtual_disk,curr_index+5,SEEK_SET); // go to the section holding the name of the file.
                fread(valid_name_buf,sizeof(valid_name_buf)-1,1,virtual_disk);
                
                // We also have to check extensions for file.
                if (strcmp(valid_name_buf,name) == 0) {
                    fseek(virtual_disk,curr_index+13,SEEK_SET);
                    fread(valid_ext_buf,sizeof(valid_ext_buf)-1,1,virtual_disk);
                    if(strcmp(valid_ext_buf,ext) == 0) {
                        valid = 1;
                        break;
                    }
                  
                    
                }
            }
            curr_index = curr_index + 48;
        }

        
        
        
        
    }
    
    if (valid == 1) return curr_index; //return index of location of valid file/directory
    
    
    return -1; // failed to find file.
}

// Like the function above but instead looking for an empty Root Directory Enry
int search_empty_Root_dir() {
    // check if we're looking for a directory or file.
    int valid = 0;
    char valid_entry_buf;
   
    int curr_index = ROOT_START_INDEX *BLOCK_SIZE;
        
        while(curr_index <= ((DATA_REGION_START*BLOCK_SIZE)-48) && (valid != 1)) {
            fseek(virtual_disk,curr_index,SEEK_SET);
            
            fread(&valid_entry_buf,sizeof(valid_entry_buf),1,virtual_disk); // fread will move the file pointer one byte towards the end
            if(valid_entry_buf == 0) {
                
                valid = 1;
                    break;
                    
                }
            curr_index = curr_index + 48;
            }
    
    


if (valid == 1) return curr_index; //return index of location of empty entry.


return -1; // failed to find empty.
}

// Like the above functions but searching for a file using an offset.
int search_Root_for_file( int offset) {
    int valid = 0;
    char valid_entry_buf;
    int curr_index;
    if (offset == 0) {
     curr_index = ROOT_START_INDEX * BLOCK_SIZE;
    } else {
        curr_index = offset;
    }
    
    while(curr_index <= ((DATA_REGION_START*BLOCK_SIZE)-48) && (valid != 1)) {
        fseek(virtual_disk,curr_index,SEEK_SET);
        
        fread(&valid_entry_buf,sizeof(valid_entry_buf),1,virtual_disk); // fread will move the file pointer one byte towards the end
        if(valid_entry_buf == 'f') {
            
            valid = 1;
            //return curr_index;
            break; //return curr_index;
            //curr_index = curr_index + 32;
            
        }
        curr_index = curr_index + 48;
    }
    
    
    
    
    if (valid == 1) return curr_index; //return index of location of valid file/directory
    
    
    return -1; // failed to find file.
}


// Searching for an empty FAT entry.
int search_empty_FAT_entry(){
    
    signed short FAT_value;
    int current = FAT_REGION_START * BLOCK_SIZE;
    fseek(virtual_disk, current, SEEK_SET);
    while(current <= ((ROOT_START_INDEX*512)-2)){
        fread(&FAT_value, sizeof(short), 1, virtual_disk);
        
        if(FAT_value == -2){
            return current;
        }
        current = current + 2;
    }
    
    return -1; // failed to find anything
}


// Creating an empty Root Directory entry. Size of a root directory entry is 48 bytes.
void create_root_dir_entry(int root_offset, int parent_value, char *name, char *ext, short Fat_addr) {
    
    
   time_t time_of_format = time(NULL);
   struct tm *t_m = localtime(&time_of_format);
    
    // have to cast values in struct to short
    int year =  t_m->tm_year + 1900; // adding 1900 because date the year var in the stuct represents number of years from 1900.
    int day = t_m->tm_mday ;
    int month = t_m->tm_mon+1; // adding one because month in struct are stored with index of 0.
    int size = 0;
   

    char flag;
   signed  short closed = -1;
    char filler[3]; // this will be used in case ext is null.
    
    
    if (strcmp(" ",ext)==0) flag = 'd'; else flag = 'f';
    
    fseek(virtual_disk,root_offset,SEEK_SET);
    
    fwrite(&flag,1,sizeof(flag),virtual_disk);
    
    fseek(virtual_disk,root_offset+1,SEEK_SET);
    fwrite(&closed,1,sizeof(closed),virtual_disk);
    
    fseek(virtual_disk,root_offset+3,SEEK_SET);
    fwrite(&Fat_addr,1,sizeof(Fat_addr),virtual_disk);
    
    fseek(virtual_disk,root_offset+5,SEEK_SET);
    
    fwrite(name,1,sizeof(name),virtual_disk);
    
    fseek(virtual_disk,root_offset+13,SEEK_SET);
    fwrite(ext,1,sizeof(ext),virtual_disk);
    
    fseek(virtual_disk,root_offset+16,SEEK_SET);
    fwrite(&month,1,sizeof(month),virtual_disk);
    
    fseek(virtual_disk,root_offset+20,SEEK_SET);
    fwrite(&day,1,sizeof(day),virtual_disk);
    
    fseek(virtual_disk,root_offset+24,SEEK_SET);
    fwrite(&year,1,sizeof(day),virtual_disk);
    
    
    fseek(virtual_disk,root_offset+28,SEEK_SET);
    fwrite(&month,1,sizeof(month),virtual_disk);
    
    fseek(virtual_disk,root_offset+32,SEEK_SET);
    fwrite(&day,1,sizeof(day),virtual_disk);
    
    fseek(virtual_disk,root_offset+36,SEEK_SET);
    fwrite(&year,1,sizeof(day),virtual_disk);
    
    fseek(virtual_disk,root_offset+40,SEEK_SET);
    fwrite(&parent_value,1,sizeof(parent_value),virtual_disk);
    
    fseek(virtual_disk,root_offset+44,SEEK_SET);
     fwrite(&size,1,sizeof(size),virtual_disk);
    
}

// This function will change the value of a fat entry/
int change_FAT_entry(int offset, short value) { //For value -2 = Free, -1 = In use but not pointing anywhere, any value 0 or above represents a link to another address.
    fseek(virtual_disk,offset,SEEK_SET);
    
    fwrite(&value,1,sizeof(value),virtual_disk);
    return 0;
}

// This function will create a file for the filesystem. Returns 1 on success, -1 on failure.
int create_file(char *path, char *name) {
    
// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    char file_name[9];
    memset(file_name,0,9);
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    
    token = strtok(name_copy, ".");
  
    
    if (strlen(token) > 8) {
        printf("INVALID FILE NAME >:(\n");
        return -1;
    }
   
       strcpy(file_name,token);
    
    //printf("%s\n",file_name);

    
        token = strtok(NULL, ".");
    
    
    char ext[4];
       memset(ext,0,4);
    
    if (token != NULL) {
        
    if (strlen(token) > 3) {
        printf("INVALID FILE EXTENSION >:(\n");
        return -1;
    }
        
        
        strcpy(ext,token);
      
    } else {
        
        strcpy(ext, " ");
     
    }
    
 // FINISHED PARSING
 // ***************************************

  
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist != -1){ printf("THIS FILE/DIRECTORY %s ALREADY EXISTS >:(\n",file_name); return -1;}
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("NOT A VALID PATH\n"); return -1; }
    
    // Looking for an empty root entry, so we can store our file's meta data.
    int open_Root_index = search_empty_Root_dir();
    if (open_Root_index== -1) {printf("EMPTY ROOT DIR SEARCH FAILED\n"); return -1; } // Root Directory is filled.
    
    // Checking for an empty Fat entry, to make sure we have enough space in filesystem.
    int open_Fat_entry = search_empty_FAT_entry();
    if(open_Fat_entry == -1) {printf("EMPTY FAT ENTRY SEARCH FAILED\n");return -1; }
    
    // FINISHED ALL THE CHECKS, NOW A NEW FILE OR DIRECTORY CAN BE MADE YAHOO!!
    
    // This function will write all the nessecary meta data.
    create_root_dir_entry(open_Root_index,path_check,file_name,ext,open_Fat_entry);
    
    // set Fat entry to -1.
    
    change_FAT_entry(open_Fat_entry,-1);
    
    
    return 1;
}

// This function will set a file's flag to open
int open_file(char *path, char *name) {
    
 
    
    
// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    
    char file_name[9];
    memset(file_name,0,9);
    
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    token = strtok(name_copy, ".");
    
    
    if (strlen(token) > 8) {
        printf("INVALID FILE NAME >:(\n");
        return -1;
    }
    
    strcpy(file_name,token);
    //printf("%s\n",file_name);
    
    token = strtok(NULL, ".");

    char ext[4];
    memset(ext,0,4);
    
    if (token != NULL) {
        
        if (strlen(token) > 3) {
            printf("INVALID FILE EXTENSION >:(\n");
            return -1;
        }
        strcpy(ext,token);
        
    } else {
        
            strcpy(ext, " ");
        
    }
    

// FINISHED PARSING
// ***************************************
    
    
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist == -1){ printf("FILE OPEN: THIS FILE/DIRECTORY %s DOESN'T EXIST! >:(\n",file_name); return -1;}
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("FILE OPEN: NOT A VALID PATH\n"); return -1; }
    
    // Get past checks, can now change file status to open.
    
    // 0 is the value that represents an open file. This flag will be written into the file's root directory entry.
    short open_sesame = 0;
    
    fseek(virtual_disk,does_file_exist+1,SEEK_SET);
    fwrite(&open_sesame,1,sizeof(open_sesame),virtual_disk);
    
    
    
    return 1;
    
    
}

// This function will set a file's flag to closed. Returns -1 on failure, and 1 on success.
int close_file(char *path, char *name) {

// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    
    char file_name[9];
    memset(file_name,0,9);
    
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    token = strtok(name_copy, ".");
    
    
    if (strlen(token) > 8) {
        printf("FROM CLOSE FILE: INVALID FILE NAME >:(\n");
        return -1;
    }
    
    strcpy(file_name,token);
    //printf("%s\n",file_name);
    
    token = strtok(NULL, ".");
    
    char ext[4];
    memset(ext,0,4);
    
    if (token != NULL) {
        
        if (strlen(token) > 3) {
            printf("FROM CLOSE FILE: INVALID FILE EXTENSION >:(\n");
            return -1;
        }
        strcpy(ext,token);
        
    } else {
        
        strcpy(ext, " ");
        
    }
   
// FINISHED PARSING
// ***************************************
    
    
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist == -1){ printf("FROM FILE CLOSE: THIS FILE/DIRECTORY %s DOESN'T EXIST! >:(\n",file_name); return -1;}
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("FROM FILE CLOSE: NOT A VALID PATH\n"); return -1; }
    
    // -1 is the value that represents an open file. This flag will be written into the file's root directory entry.
    signed short close_sesame = -1;
    
    fseek(virtual_disk,does_file_exist+1,SEEK_SET);
    fwrite(&close_sesame,1,sizeof(close_sesame),virtual_disk);
    
   
    
    return 0;
    
}

// This function will check if the file is open by looking at it's flag in the root directory entry.
int is_File_open(int root_offset) {
    fseek(virtual_disk,(root_offset + 1),SEEK_SET);
    short flag;
    fread(&flag,sizeof(flag),1,virtual_disk);
    
    if (flag == - 1) return -1;
    else if (flag == 0) return 0;
}

// This function will retrieve the first FAT address of a file from the root directory entry.
short get_starting_FAT_addr(int root_offset) {
    fseek(virtual_disk,(root_offset + 3),SEEK_SET);
    short flag;
    fread(&flag,1,sizeof(flag),virtual_disk);
   
    return flag;
}

// This function will retrieve the address of the parent directory from the root directory entry.
int get_parent(int root_offset) {
    fseek(virtual_disk,(root_offset + 40),SEEK_SET);
    int flag;
    fread(&flag,1,sizeof(flag),virtual_disk);
    return 0;

}

// This function will retrieve the size of the file from the root directory entry.
int get_size_of_file(int root_offset) {
    fseek(virtual_disk,(root_offset + 44),SEEK_SET);
    int flag;
    fread(&flag,sizeof(flag),1,virtual_disk);
    
    return flag;
}

// This function gets the value of a FAT entry.
short get_FAT_value(short fat_offset) {
    fseek(virtual_disk,fat_offset,SEEK_SET);
    short flag;
    fread(&flag,sizeof(flag),1,virtual_disk);
    return flag;

}

// This function reads a file's contents, and returns a char holding the contents.
// Returns a NULL pointer on failure
char* read_file(char *path, char *name) { // returns a buffer
    
// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    char file_name[9];
    memset(file_name,0,9);
    
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    token = strtok(name_copy, ".");
    
    
    if (strlen(token) > 8) {
        printf("FROM READ FILE: INVALID FILE NAME >:(\n");
        return NULL;
    }
    
    strcpy(file_name,token);
   
    
    token = strtok(NULL, ".");
    
    char ext[4];
    memset(ext,0,4);
    
    if (token != NULL) {
        
        if (strlen(token) > 3) {
            printf("FROM READ FILE: INVALID FILE EXTENSION >:(\n");
            return NULL;
        }
        strcpy(ext,token);
        
    } else {
        
        strcpy(ext, " ");
        
    }
   
// FINISHED PARSING
// ***************************************
    
    
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist == -1){ printf("FROM READ FILE: THIS FILE/DIRECTORY %s DOESN'T EXIST! >:(\n",file_name); return NULL;}
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("FROM READ FILE: NOT A VALID PATH\n"); return NULL; }
    
    // Check to see if the file is open. If not return failure.
    if (is_File_open(does_file_exist)== -1){printf("FROM READ FILE: FILE NOT OPEN\n"); return NULL;}
    
    // store the FAT address of first FAT entry of file.
    short FAT_addr = get_starting_FAT_addr(does_file_exist);
    
    // this short holds the value stored at FAT address.
    signed short FAT_value;
    
    // calculate the data region's index
    int data_region_index = ((FAT_addr-BLOCK_SIZE)/2);
    
    // get the size of the file. If size = 0, return failure. Can't read an empty file.
    int size_of_file = get_size_of_file(does_file_exist);
    if (size_of_file == 0){printf("FROM READ FILE: FILE EMPTY\n"); return NULL;} //nothing to read
    
    // allocate buffer in which we read characters.
     char *buf = malloc(MAX_FILE_SIZE);
    memset(buf,0,MAX_FILE_SIZE);
    int index_buf = 0;
    
    
    do {
    
    fseek(virtual_disk,(DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index),SEEK_SET);
        
        // Read from the first block
        int index_for_block = 0;
        while (index_buf < size_of_file && index_for_block <= BLOCK_SIZE) {
            fread(&buf[index_buf],sizeof(char),1,virtual_disk);
            index_buf++; index_for_block++;
        }
        int ok = get_FAT_value(FAT_addr);
        FAT_addr = ok;
        //printf("%d\n",data_region_index);
        data_region_index = ((FAT_addr-BLOCK_SIZE)/2); // this value will be negative if there isn't another FAT entry.
        // It will be negative because invalid FAT values would be -2, or -1 for this function.
        
     
    
    //If we finished reading from the block, but there is more to read as indicated by size, then we continue to loop.
    } while (data_region_index >= 0 && index_buf < size_of_file);
    
    return buf;

}

// change the size of a file in the root directory entry.
int change_size(int root_offset,int new_size) {
    fseek(virtual_disk,(root_offset +44),SEEK_SET);
    
    fwrite(&new_size,1,sizeof(new_size),virtual_disk);
    
    return 1;
}

// change the modification time of a file in it's root directory entry.
int change_modtime(int root_offset) {
    
    time_t time_of_format = time(NULL);
    struct tm *t_m = localtime(&time_of_format);
    
    // have to cast values in struct to short
    int year =  t_m->tm_year + 1900; // adding 1900 because date the year var in the stuct represents number of years from 1900.
    int day = t_m->tm_mday ;
    int month = t_m->tm_mon+1; // adding one because month in struct are stored with index of 0.
    
    
    fseek(virtual_disk,root_offset+28,SEEK_SET);
    fwrite(&month,1,sizeof(month),virtual_disk);
    
    fseek(virtual_disk,root_offset+32,SEEK_SET);
    fwrite(&day,1,sizeof(day),virtual_disk);
    
    fseek(virtual_disk,root_offset+36,SEEK_SET);
    fwrite(&year,1,sizeof(day),virtual_disk);
    return 0;
}

// this function will write information from the argument buf into a file. This function appends to the file.
int write_file(char *path, char *name, char *buf, int buf_size) {
    
    
// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    char file_name[9];
    memset(file_name,0,9);
    
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    token = strtok(name_copy, ".");
    
    
    if (strlen(token) > 8) {
        printf("FROM CLOSE FILE: INVALID FILE NAME >:(\n");
        return -1;
    }
    
    strcpy(file_name,token);
    //printf("%s\n",file_name);
    
    token = strtok(NULL, ".");
    
    char ext[4];
    memset(ext,0,4);
    
    if (token != NULL) {
        
        if (strlen(token) > 3) {
            printf("FROM CLOSE FILE: INVALID FILE EXTENSION >:(\n");
            return -1;
        }
        strcpy(ext,token);
        
    } else {
        
        strcpy(ext, " ");
        
    }
    
// FINISHED PARSING
// ***************************************
    
    
    
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist == -1){ printf("FROM FILE CLOSE: THIS FILE/DIRECTORY %s DOESN'T EXIST! >:(\n",file_name); return -1;}
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("FROM FILE CLOSE: NOT A VALID PATH\n"); return -1; }
    
    // Check to see if the file is open.
    if (is_File_open(does_file_exist)== -1){printf("FILE NOT OPEN\n"); return -1; }
    
    // store the address of first FAT entry of file.
    short FAT_addr = get_starting_FAT_addr(does_file_exist);
   
    
    // get the size of the file. If file is at max capacity return -1.
    int size_of_file = get_size_of_file(does_file_exist);
    
        if (size_of_file == MAX_FILE_SIZE){printf("FILE IS AT MAX CAPACITY\n"); return -1;} // FILE IS AT MAX CAPACITY
    
    int num_blocks = (size_of_file / BLOCK_SIZE); // how many complete blocks does file already take up.
    
    int bytes = size_of_file % BLOCK_SIZE; // how many bytes it takes up in non-full block.
    
    // Check if we can fit the buffer we want to write into the file. This is what sets a max for the file.
    int remaining_space =  MAX_FILE_SIZE - size_of_file; if (remaining_space < buf_size) return -1;
    
  // Traverse through FAT to get the furthest most FAT entry.
    int index = 0;
    while (index < num_blocks) {
        FAT_addr = get_FAT_value(FAT_addr);
    }
    
    // get the data_region_index
    int data_region_index = (FAT_addr - BLOCK_SIZE)/2;
    
    // calculate the offset, which we'll use to seek to the data region.
    int data_offset = (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index) + bytes;
    
    
    int elements_written = 0; // keep track of how many elements we've written, and compare it to number of buf elements.
    while ( elements_written < buf_size) {
        fseek(virtual_disk,data_offset,SEEK_SET);
        int remaining_bytes_in_block = BLOCK_SIZE-bytes;
        int bytes_cp = bytes;
        
        while (bytes_cp <= BLOCK_SIZE && elements_written < buf_size) {
            fwrite(&buf[elements_written],1,1,virtual_disk); //write byte by byte
            bytes_cp++; elements_written++;
        }
        
            if ( elements_written < buf_size) { // If we go into this if statement, then we need to add a new FAT entry.
                // make new FAT entry
                printf("f %d\n",FAT_addr);
                short new_empty_FAT_entry = search_empty_FAT_entry();
                if (new_empty_FAT_entry == -1) {printf("FS has reached max capacity\n"); return -1;}
                change_FAT_entry(FAT_addr,new_empty_FAT_entry);
                
                // go to next link
                
                int ok = get_FAT_value(FAT_addr);
                FAT_addr = ok;
                
                change_FAT_entry(FAT_addr,-1);
                
                // change data region index & fseek to that region
                data_region_index = (FAT_addr - BLOCK_SIZE)/2;
                data_offset = (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index);
                
            }
        
        }
    // upate size of the file.
    change_size(does_file_exist,(buf_size+size_of_file));
        return 1;
    }

// This function will change the parent address in the root directory entry to the Root Directory Entry.
int change_parent_value_to_root(int offset){
    
    fseek(virtual_disk,offset+44,SEEK_SET);
    int root= ROOT_START_INDEX*BLOCK_SIZE;
    fwrite(&root,1,sizeof(root),virtual_disk);
    return 0;

}

// This function will delete a file. Returns 1 on success, and -1 on failure.
int delete_file(char *path, char *name) {
   
    // a pointer to 512 zeroes.
    char *zero = malloc(sizeof(char)*BLOCK_SIZE);
    memset(zero,0,512);
    
// PARSING NAME PROVIDED IN PARAMETER
// ***************************************
    char file_name[9];
    memset(file_name,0,9);
    
    char *name_copy = calloc(1, sizeof(name));
    strcpy(name_copy,name);
    
    char *token;
    token = strtok(name_copy, ".");
    
    
    if (strlen(token) > 8) {
        printf("FROM CLOSE FILE: INVALID FILE NAME >:(\n");
        return -1;
    }
    
    strcpy(file_name,token);
   
    
    token = strtok(NULL, ".");
    
    char ext[4];
    memset(ext,0,4);
    
    if (token != NULL) {
        
        if (strlen(token) > 3) {
            printf("FROM CLOSE FILE: INVALID FILE EXTENSION >:(\n");
            return -1;
        }
        strcpy(ext,token);
        
    } else {
        
        strcpy(ext, " ");
        
    }
   
// FINISHED PARSING
// ***************************************
    
    
    
    // Checking if the file exists. If it does then the return value is the address
    // of the root directory entry of that file.
    int does_file_exist = search_Root_Dir(file_name, ext);
    if (does_file_exist == -1){ printf("FROM FILE CLOSE: THIS FILE/DIRECTORY %s DOESN'T EXIST! >:(\n",file_name); return -1;}
    
    
    // Checking to see if the path provided is valid. path_check will hold the root
    // directory address of lowest subdirectory provided in the path.
    int path_check = check_path(path);
    if (path_check == - 1){printf("FROM FILE CLOSE: NOT A VALID PATH\n"); return -1; }
    
    // Delete will be different depending on if a file is a directory or file
    
    // If the file is a directory
    if (strcmp(ext, " ") == 0) {
       
        int index = 0;
        
        // Change the parent directory of files linked to the directory about to be deleted to the root directory
        do {
           int index2 = search_Root_for_file(index);
            index = index2 + 48; // add 48 to go to the next root entry directory index
            printf("%d\n",index2);
            if (index2 != -1) {
                int parValue = get_parent(index2);
                
                if (parValue == does_file_exist) change_parent_value_to_root(index2);
            }
        }while (search_Root_for_file(index) != -1);
        
        
        // change the values in the FAT to -2
            short Fat_addr = get_starting_FAT_addr(does_file_exist);
            while (get_FAT_value(Fat_addr) != -1) {
                
                short temp = Fat_addr;
                short ok = get_FAT_value(Fat_addr);
                Fat_addr = ok;
                change_FAT_entry(temp,-2);
                
                // set data region to all null characters
                int data_region_index = (ok-BLOCK_SIZE)/2;
                fseek(virtual_disk, (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index), SEEK_SET);
                fwrite(zero,1,sizeof(zero),virtual_disk);
            }
        // Have to change one FAT entry outside loop, and it corresponding data region block.
            change_FAT_entry(Fat_addr,-2);
            int data_region_index = (Fat_addr-BLOCK_SIZE)/2;
            fseek(virtual_disk, (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index), SEEK_SET);
            fwrite(zero,1,sizeof(zero),virtual_disk);
        
        // Lastly indicate that the root entry directory is free by chaning it's flag to the null char.
        fseek(virtual_disk,does_file_exist,SEEK_SET);
        char zerochar = 0;
        fwrite(&zerochar,1,sizeof(zerochar),virtual_disk);
        
    } else { // If we're deleting a file
        
        // change the values in the FAT to -2
        short Fat_addr = get_starting_FAT_addr(does_file_exist);
        while (get_FAT_value(Fat_addr) != -1) {
            
            // change the values in the FAT to -2
            short temp = Fat_addr;
            short ok = get_FAT_value(Fat_addr);
            Fat_addr = ok;
            change_FAT_entry(temp,-2);
             // set data region to all null characters
            int data_region_index = (ok-BLOCK_SIZE)/2;
            fseek(virtual_disk, (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index), SEEK_SET);
            fwrite(zero,1,sizeof(zero),virtual_disk);
        }
        
         // Have to change one FAT entry outside loop, and it corresponding data region block.
        change_FAT_entry(Fat_addr,-2);
        int data_region_index = (Fat_addr-BLOCK_SIZE)/2;
        fseek(virtual_disk, (DATA_REGION_START*BLOCK_SIZE) + (BLOCK_SIZE*data_region_index), SEEK_SET);
        fwrite(zero,1,sizeof(zero),virtual_disk);
        fseek(virtual_disk,does_file_exist,SEEK_SET);
        
         // Lastly indicate that the root entry directory is free by chaning it's flag to the null char.
        char zerochar = 0;
        fwrite(&zerochar,1,sizeof(zerochar),virtual_disk);
        
    }
    return 1;
}
/******************************************************************************************************************/
                                        /*  MAIN FUNCTION  */
/******************************************************************************************************************/

int main() {
    
    
    Mount();
    
    // path has to be in the form /a/b/c/
    
    Format();
    create_file("/ROOT/","DIRONE");
    create_file("/ROOT/DIRONE/","Test.lol");
    
    char a[600];
    memset(a,'a',600);
    
    open_file("/ROOT/DIRONE/","Test.lol");
    write_file("/ROOT/DIRONE/","Test.lol",a,sizeof(a));
    close_file("/ROOT/DIRONE/","Test.lol");
    open_file("/ROOT/DIRONE/","Test.lol");
    char *b = read_file("/ROOT/DIRONE/","Test.lol");
    create_file("/ROOT/DIRONE/","David.txt");
    int i = 0;
    
    while (b[i] == 'a') {
        printf("%c",b[i]);
        i++;
    }
    printf("\nTOTAL a's: %d\n",i);
    
    delete_file("/ROOT/","DIRONE");
    
    UnMount();
    
    
    
}
