#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "vfs.h"

const char* filesystem_name = "file_system";

/***************************************************/
/* Helpers */									
unsigned int calculate_required_inodes (size_t);						
unsigned int calculate_block_address (unsigned int, unsigned int);
unsigned int convert_bytes_to_inodes (size_t);
void close_and_save ();			
void close_unsaved ();				
size_t calculate_file_size(int);
/***************************************************/

/* Create virtual disk of given size (in bytes) */
void create_vfs(size_t system_size) {
	unsigned int i;
	vfs  = malloc(sizeof(VFS));
	if (sizeof(SUPERBLOCK) > system_size) {
		free(vfs);
		printf("ERROR. Za malo pamieci.\n");
		exit(1);
	}

	vfs->vfs_ptr = fopen(filesystem_name, "wb");
	
	if (!vfs->vfs_ptr)
	{
		free(vfs);
		printf("ERROR. Blad przy probie utworzenia pliku.\n");
		exit(1);
	}
	
	/* End file with '0' */
	fseek(vfs->vfs_ptr, system_size-1, SEEK_SET);
	fwrite("0", sizeof(char), 1, vfs->vfs_ptr);
	fseek(vfs->vfs_ptr,0, SEEK_SET);
	
	/*Initialize data in superblock*/
	vfs->super_block.file_count = 0;
	vfs->super_block.size = system_size;
	
	/*Initialize virtual disk data*/
	vfs->inode_count = convert_bytes_to_inodes(system_size);
	vfs->super_block.free_space = vfs->inode_count * BLOCK_SIZE;
	vfs->inode_list = malloc(sizeof(INODE) * (vfs->inode_count));

	printf("Free space: %ld\n",vfs->super_block.free_space);
	printf("Inodes: %d\n",vfs->inode_count);
	/* Set all inodes as free */
	for (i = 0; i < vfs->inode_count; ++i)
	{
		vfs->inode_list[i].type = FREE;
	}
	
	printf("Utworzono dysk wirtualny o nazwie %s oraz rozmiarze %ld\n", filesystem_name, system_size);
	close_and_save(vfs);
	
}

/* Close virtual disk without saving */
void close_unsaved(VFS* vfsg) {
	if (vfs->vfs_ptr) {
		fclose(vfs->vfs_ptr);
		free(vfs->inode_list);
	}
	
	free(vfs);
	vfs = NULL;
}

/* Close virtual disk and save all data */
void close_and_save() {
	fseek(vfs->vfs_ptr, 0, SEEK_SET);
	/* write to file one superblock */
	fwrite(&vfs->super_block, sizeof(SUPERBLOCK), 1, vfs->vfs_ptr);
	/* write to file all inodes */
	fwrite(vfs->inode_list, sizeof(INODE), vfs->inode_count, vfs->vfs_ptr);

	/* cleanup */
	fclose(vfs->vfs_ptr);
	free(vfs->inode_list);
	free(vfs);
	vfs = NULL;

}

/* Boot vfs and load all data */
void load_vfs() {
	size_t system_size;
	int success = 1;
	vfs = malloc(sizeof(VFS));
	
	/* Set pointer on file representing filesystem */
	vfs->vfs_ptr = fopen(filesystem_name, "r+b");
	
	if (!vfs->vfs_ptr) {
		close_unsaved(vfs);
		printf("ERROR. Nie udalo sie otworzyc pliku.\n");
		exit(1);
	}
	
	fseek(vfs->vfs_ptr, 0, SEEK_END);
	system_size = ftell(vfs->vfs_ptr);
	
	/* Read data from superblock */
	fseek(vfs->vfs_ptr, 0, SEEK_SET);
	success = fread(&vfs->super_block, sizeof(SUPERBLOCK), 1, vfs->vfs_ptr);
	if (success <= 0) {
		printf("ERROR. Nie udalo sie wczytac danych z superbloku.\n");
		close_unsaved(vfs);
		exit(1);
	}
	
	/* Count needed inodes and initialize inode list */
	vfs->inode_count = convert_bytes_to_inodes(vfs->super_block.size);
	vfs->inode_list = malloc(sizeof(INODE) * (vfs->inode_count));
	
	/* Read data from inodes */
	success = fread(vfs->inode_list, sizeof(struct inode), vfs->inode_count, vfs->vfs_ptr);
	if ( success <= 0) {
		printf("ERROR. Nie udalo sie wczytac danych z inode.\n");
		close_unsaved(vfs);
		exit(1);
	}
	
}

int find_free_nodes(unsigned int needed_nodes, unsigned int* free_nodes_list) {
	int i=0, j=0;

	/* While there are still inodes in vfs and we didn't find enough nodes */
	while(i < vfs->inode_count && j < needed_nodes) {
		/* Look for free nodes and add them to list */
		if(vfs->inode_list[i].type == FREE) {
			free_nodes_list[j] = i;
			j++;
		}
		if(j==needed_nodes)
			break;
		i++;
	}
	
	if(j != needed_nodes) {
		return -1;
	}
	return 0;
}


/* Copy file of given name from physical disk to virtual disk */
void copy_from_physical_disk(char * file_name) {
	FILE * file;
	size_t file_size;
	char buffer[BLOCK_SIZE];
	unsigned int needed_nodes;
	unsigned int i=0, j=0;
	unsigned int* free_nodes_list;
	int first_inode_index;
	int error;

	file = fopen(file_name, "rb");
	if(!file) {
		printf("ERROR. Nie udalo sie otworzyc pliku\n");
		close_unsaved(vfs);
		exit(1);
	}
	
	fseek(file, 0, SEEK_END);
	file_size = ftell(file);
	if(vfs->super_block.free_space < file_size) {
		printf("ERROR. Za malo pamieci na dysku");
		close_unsaved(vfs);
	    exit(1);
	}
	
	needed_nodes = calculate_required_inodes(file_size);
	free_nodes_list = malloc(needed_nodes * sizeof(unsigned int));
	
	/* Find free nodes that can be used for the file */
	error = find_free_nodes(needed_nodes, free_nodes_list);

	/* If not enough nodes were found */
	if(error == -1) {
		printf("ERROR. Nie udalo sie znalezc wystarczajacej liczby wolnych blokow.\n");
		free(free_nodes_list);
		close_unsaved(vfs);
		fclose(file);
		exit(3);
	}

    	
	fseek(file, 0, SEEK_SET);
	for(i = 0; i < needed_nodes; i++) {

		/* Define if its first inode connected to the file */
		if(i == 0) {
			vfs->inode_list[free_nodes_list[i]].first_inode = -1;
			first_inode_index = free_nodes_list[i];
		}
		else {
			/* Remember the address for first inode */
			vfs->inode_list[free_nodes_list[i]].first_inode = first_inode_index;
		}

		vfs->inode_list[free_nodes_list[i]].type = TAKEN;


		/* Set next inode (if it's last inode, set to -1) */
		if (i < needed_nodes - 1)
			vfs->inode_list[free_nodes_list[i]].next_inode = free_nodes_list[i+1];
		else
			vfs->inode_list[free_nodes_list[i]].next_inode = -1;

		
		/* Set size located in each inode */
		if(file_size >= BLOCK_SIZE) {
			vfs->inode_list[free_nodes_list[i]].size = BLOCK_SIZE;
			file_size -= BLOCK_SIZE;
		}
		else 
			vfs->inode_list[free_nodes_list[i]].size = file_size;
		

		/* Set file name */
		if(strlen(file_name) <= MAX_FILE_NAME)
			strcpy(vfs->inode_list[free_nodes_list[i]].file_name, file_name);
		else {
			printf("ERROR. Przekroczono maksymalna dlugosc nazwy pliku: %d.\n", MAX_FILE_NAME);
			free(free_nodes_list);
			close_unsaved(vfs);
			fclose(file);
			exit(3);
		}		


		/*Copy content*/
		fseek(vfs->vfs_ptr, calculate_block_address(free_nodes_list[i], vfs->inode_count), SEEK_SET);
		fread(buffer, 1, sizeof(buffer), file);
		fwrite(buffer, 1, vfs->inode_list[free_nodes_list[i]].size, vfs->vfs_ptr);
	}


	/* Adjust left space and file count */
	vfs->super_block.free_space -= needed_nodes * BLOCK_SIZE;
	vfs->super_block.file_count +=1;

	/* cleanup */
	fclose(file);
	free(free_nodes_list);
	close_and_save(vfs);
	
	printf("Sukces. Przekopiowano plik %s na dysk wirtualny.\n", file_name);
}

unsigned int check_file_exists(char* file_name) {
	int found = 0, i=0, first_inode;
	for (i = 0; i < vfs->inode_count; i++) {
		if (vfs->inode_list[i].type == TAKEN && strncmp(vfs->inode_list[i].file_name, file_name, MAX_FILE_NAME) == 0) {
			first_inode = i;
			found = 1;
			break;
		}
	}
	if(found == 0) {
		printf("ERROR. Plik %s nie istnieje na dysku.\n", file_name);
		close_unsaved(vfs);
		exit(1);
	}
	return first_inode;
}

/* Copy file of given name from virtual disk to physical disk */
void copy_from_virtual_disk(char* file_name) {
	FILE * file;
	char buffer[BLOCK_SIZE];
	unsigned int first_inode, current_inode;

	/* Check if the file exists */
	first_inode = check_file_exists(file_name);
	
	file = fopen(file_name, "wb");
	if (!file) {
		printf("ERROR. Nie udalo sie otworzyc pliku do zapisu.\n");
		close_unsaved(vfs);
		exit(1);
	}
	
	current_inode = first_inode;
	/* Write data to buffer from vfs to new file*/	
	do
	{
		fseek(vfs->vfs_ptr, calculate_block_address(current_inode, vfs->inode_count), SEEK_SET);
		fread(buffer, 1, vfs->inode_list[current_inode].size, vfs->vfs_ptr);
		fwrite(buffer, 1, vfs->inode_list[current_inode].size, file);
		current_inode = vfs->inode_list[current_inode].next_inode;
	} while (current_inode != -1);

	fclose(file);
	close_unsaved(vfs);
	printf("Sukces. Przekopiowano plik %s z dysku wirtualnego\n.", file_name);
}

/* Delete file of given nam efrom virtual disk */
void delete_from_virtual_disk(char* file_name) {
	int first_inode, current_inode, blocks=0;
	
	/* Check if file exists */
	first_inode = check_file_exists(file_name);
	
	/* Free space connected to the file */
	current_inode = first_inode;
	
	do
	{	
		vfs->inode_list[current_inode].type = FREE;
		vfs->inode_list[current_inode].first_inode = -1;
		blocks++;
		current_inode = vfs->inode_list[current_inode].next_inode;
	} while (current_inode != -1);
	
	vfs->super_block.file_count -= 1;
	vfs->super_block.free_space += blocks * BLOCK_SIZE;
	
	close_and_save(vfs);
	printf("Sukces. Usunieto plik %s z dysku wirtualnego\n",file_name);
}

void delete_vfs() {
	unlink(filesystem_name);
}

/* List files in catalogue */
void show_folder() {
	int i = 0, j=0, n_node=-1;
	size_t size;

	printf("%-30s%-10s\n","Nazwa","Rozmiar");
	printf("----------------------------------------\n");
	for (i = 0; i < vfs->inode_count; i++) {
		/* Look for first inode connected to file */
		if (vfs->inode_list[i].first_inode == -1 && vfs->inode_list[i].type == TAKEN) {
			printf("%-32s", vfs->inode_list[i].file_name);
			size = calculate_file_size(i);
			printf("%-12lu", (unsigned long)size);
			printf("\n");
		}
	}
}

/* Show disk statistics */
void show_vfs_map() {
	int i = 0, type;

	printf("System plikow - info:\n");
	printf("Rozmiar systemu          %lu \n", (unsigned long)vfs->super_block.size);
	printf("Wolne miejsce            %lu \n", (unsigned long)vfs->super_block.free_space);
	printf("Liczba plikow            %d \n", vfs->super_block.file_count);

	printf("\n");
	printf("Mapa blokow:\n");
	printf("W - Wolny\tZ - Zajety\n");
	printf("--------------------------------------------------\n");
	for (i = 0; i < vfs->inode_count; i++)
	{
		printf("[ ");
		printf("%3d:", i);
		type = vfs->inode_list[i].type;
		switch(type) {
			case TAKEN:
				printf("Z  ]  ");
				break;
			case FREE:
				printf("W  ]  ");
				break;
			default:
				printf("?  ]  ");
				break;
		}
		if ((i+1)%10==0 && i<vfs->inode_count-1) 
			printf("\n");
	}
	printf("\n");

}

unsigned int convert_bytes_to_inodes(size_t size) {
	unsigned int ret1 = (size-sizeof(SUPERBLOCK));
	unsigned int ret2 = (sizeof(INODE)+BLOCK_SIZE);
	return ret1/ret2;
}

unsigned int calculate_required_inodes(size_t size) {
	if(size % BLOCK_SIZE)
		return size/BLOCK_SIZE + 1;
	else
		return size/BLOCK_SIZE;
}

unsigned int calculate_block_address(unsigned int inode_index, unsigned int inode_count) {
	return sizeof(SUPERBLOCK) + sizeof(INODE) * inode_count + BLOCK_SIZE* inode_index;
}

/* Calculate the file size depending on index of first indoe */
size_t calculate_file_size(int i) {
	int n_node = -1, j=0;
	size_t size=0;
	n_node = vfs->inode_list[i].next_inode;
	size = size + vfs->inode_list[i].size;
	while (n_node != -1) {
		j++;
		size = size + vfs->inode_list[n_node].size;
		n_node = vfs->inode_list[n_node].next_inode;
	}
	return size;
}


