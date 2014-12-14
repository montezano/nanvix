/*
 * Copyright(C) 2011-2014 Pedro H. Penna <pedrohenriquepenna@gmail.com>
 * 
 * This file is part of Nanvix.
 * 
 * Nanvix is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Nanvix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with Nanvix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>

/**
 * @brief Prints program usage and exits.
 */
static usage(void)
{
	printf("usage: mkfs.minix <input file> <ninodes> <nblocks>\n");
	exit(EXIT_SUCCESS);
}

/**
 * @brief Creates a Minix file system.
 */
int main(int argc, char **argv)
{
	unsigned ninodes;     /* # inodes in the file system.      */
	unsigned nblocks; 	  /* # data blocks in the file system. */
	const char *diskfile; /* Disk file name.                   */
	
	/* Missing arguments. */
	if (argc < 4)
		usage();
	
	/* Extract arguments. */
	diskfile = argv[1];
	sscanf(argv[2], "%u", &ninodes);
	sscanf(argv[2], "%u", &nblocks);
	
	minix_mkfs(diskfile, ninodes, nblocks);
	
	return (EXIT_SUCCESS);
}