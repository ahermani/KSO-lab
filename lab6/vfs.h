#include <stdio.h>

#define MAX_FILE_NAME 32
#define BLOCK_SIZE 1024

/* Flag for defining block usage */
#define TAKEN 1
#define FREE 2


/* Inode structure */
typedef struct inode {
	char file_name[MAX_FILE_NAME];
	size_t size;
	unsigned int type;	
	int next_inode;
	short first_inode;
} INODE;

/* Superblock structure */
typedef struct superblock {
	size_t size;
	size_t free_space;
	unsigned int file_count;
} SUPERBLOCK;

/* VFS primary data */
typedef struct vfs {
	SUPERBLOCK super_block;
	FILE* vfs_ptr;
	INODE* inode_list;
	int inode_count;
} VFS;

VFS* vfs;

/* Virtudal disk functionality */
void create_vfs (size_t);							
void copy_from_physical_disk (char*);	
void copy_from_virtual_disk (char*);	
void delete_from_virtual_disk ( char*);	
void delete_vfs();										
void show_folder();								
void show_vfs_map();					

void load_vfs();
