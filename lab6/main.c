#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "vfs.h"

/* Program usage */
void info() {

	printf("List of commands: \n");
	printf("c <size in bytes> - Create virtual disk\n");
	printf("v <file_name>     - Copy file from physical disk to virtual disk\n");
	printf("p <file_name>     - copy file from virtual disk to physical disk\n");
	printf("l                 - list all files in catalogue\n");
	printf("i                 - show disk statistics\n");
	printf("d                 - remove virtual disk\n");
	printf("r <file_name>     - remove file\n");

}

int main(int argc, char * argv[]) {
	if (argc<2) 
		info();
	else {
		int command = argv[1][0];
		switch (command) {
			case 'c': {
					create_vfs(atoi(argv[2]));
					break;
			}
			case 'v': {
					if (argc<3)
						printf("Brak nazwy pliku.\n");
					else {
						load_vfs();
						copy_from_physical_disk(argv[2]);
					}					
					break;
			}
			case 'p': {
					if (argc<3)
						printf("Brak nazwy pliku.\n");
					else {
						load_vfs();
						copy_from_virtual_disk(argv[2]);
					}
					break;
			}
			case 'l': {
					load_vfs();
					show_folder();
					break;
			}
			case 'i': {
					load_vfs();
					show_vfs_map();
					break;
			}
			case 'd': {
					delete_vfs();
					break;
			}
			case 'r': {
					load_vfs();
					delete_from_virtual_disk(argv[2]);
					break;
			}
			default: {
				info(argv[0]);
				break;
			}

		}
	}
	return 0;
}

