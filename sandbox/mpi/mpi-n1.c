#include "mpi.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#define SIZE (47004)

/* Each process writes to separate files and reads them back. 
   The file name is taken as a command-line argument, and the process rank 
   is appended to it. */ 

int main(int argc, char **argv)
{
    int *buf, i, rank, nints, len;
    char *filename;
    MPI_File fh;
    MPI_Status status;

    MPI_Init(&argc,&argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	/* process 0 takes the file name as a command-line argument and 
	   broadcasts it to other processes */
    if (rank == 0) {
		i = 1;
		while ((i < argc) && strcmp("-fname", *argv)) {
			i++;
			argv++;
		}
		if (i >= argc) {
			fprintf(stderr, "\n*#  Usage: simple -fname filename\n\n");
			MPI_Abort(MPI_COMM_WORLD, 1);
		}
		argv++;
		len = strlen(*argv);
		filename = (char *) malloc(len+10);
		strcpy(filename, *argv);
		MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
		MPI_Bcast(filename, len+10, MPI_CHAR, 0, MPI_COMM_WORLD);
	} else {
		MPI_Bcast(&len, 1, MPI_INT, 0, MPI_COMM_WORLD);
		filename = (char *) malloc(len+10);
		MPI_Bcast(filename, len+10, MPI_CHAR, 0, MPI_COMM_WORLD);
    }
    

    buf = (int *) malloc(SIZE);
    nints = SIZE/sizeof(int);
    for (i=0; i<nints; i++) buf[i] = rank*100000 + i;

    /* each process opens that file. */ 
    printf("rank %d opening %s\n", rank, filename);
    int ret = MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR,
	   MPI_INFO_NULL, &fh);
	if (ret != 0) {
		printf("problem? %d\n", ret);
		char * estr = malloc(1000);
		int l;
		MPI_Error_string(ret, estr, &l);
		printf("%s\n", estr);
		exit(ret);
	}
    printf("rank %d writing %d at %d\n", 
            rank, (int)(nints*sizeof(MPI_INT)), (int)(rank*nints*sizeof(MPI_INT)));
    MPI_File_write_at(fh, rank*(nints*sizeof(MPI_INT)), 
            buf, nints, MPI_INT, &status);
    MPI_File_close(&fh);

    /* reopen the file and read the data back */

    for (i=0; i<nints; i++) buf[i] = 0;
    MPI_File_open(MPI_COMM_SELF, filename, MPI_MODE_CREATE | MPI_MODE_RDWR, 
                  MPI_INFO_NULL, &fh);
    printf("rank %d reading %d at %d\n", 
            rank, (int)(nints*sizeof(MPI_INT)), (int)(rank*nints*sizeof(MPI_INT)));
    MPI_File_read_at(fh, rank*(nints*sizeof(MPI_INT)), buf, nints, MPI_INT, &status);
    MPI_File_close(&fh);

    /* check if the data read is correct */
    for (i=0; i<nints; i++) {
		if (buf[i] != (rank*100000 + i)) {
    		fprintf(stderr, "Process %d: error, read %d, should be %d\n", rank, buf[i], rank*100000+i);
		}
	}

    if (!rank) {
		fprintf(stderr, "Done\n");
	}

    free(buf);
    free(filename);

    MPI_Finalize();
    return 0; 
}

