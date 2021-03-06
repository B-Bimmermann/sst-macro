/**
Copyright 2009-2017 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2017, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of Sandia Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#include <sstmac/replacements/mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include "mpitestconf.h"
#ifdef HAVE_STRING_H
#include <string.h>
#endif
namespace hindexed_block {

#if !defined(USE_STRICT_MPI) && defined(MPICH2)
#define TEST_HINDEXED_BLOCK 1
#endif

static int verbose = 0;

/** tests */
int hindexed_block_contig_test(void);
int hindexed_block_vector_test(void);

/** helper functions */
int parse_args(int argc, char **argv);
static int pack_and_unpack(char *typebuf, int count, MPI_Datatype datatype, int typebufsz);

int hindexed_block(int argc, char **argv)
{
    int err, errs = 0;
    int rank;

    MPI_Init(&argc, &argv);     /** MPI-1.2 doesn't allow for MPI_Init(0,0) */
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
#if defined(TEST_HINDEXED_BLOCK)
    parse_args(argc, argv);

    /** To improve reporting of problems about operations, we
     * change the error handler to errors return */
    MPI_Comm_set_errhandler(MPI_COMM_WORLD, MPI_ERRORS_RETURN);

    /** perform some tests */
    err = hindexed_block_contig_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in hindexed_block test.\n", err);
    errs += err;

    err = hindexed_block_vector_test();
    if (err && verbose)
        fprintf(stderr, "%d errors in hindexed_block vector test.\n", err);
    errs += err;
#endif /**defined(TEST_HINDEXED_BLOCK)*/

    /** print message and exit */
    if (rank == 0) {
        if (errs) {
            fprintf(stderr, "Found %d errors\n", errs);
        }
        else {
            printf(" No Errors\n");
        }
    }
    MPI_Finalize();
    return 0;
}

#if defined(TEST_HINDEXED_BLOCK)

/** hindexed_block_contig_test()
 *
 * Tests behavior with a hindexed_block that can be converted to a
 * contig easily.  This is specifically for coverage.
 *
 * Returns the number of errors encountered.
 */
int hindexed_block_contig_test(void)
{
    int buf[4] = { 7, -1, -2, -3 };
    int err, errs = 0;

    int i, count = 1;
    MPI_Aint disp = 0;
    MPI_Datatype newtype;

    int size, int_size;
    MPI_Aint extent;

    err = MPIX_Type_create_hindexed_block(count, 1, &disp, MPI_INT, &newtype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error creating struct type in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_size(MPI_INT, &int_size);

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type size in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    if (size != int_size) {
        if (verbose) {
            fprintf(stderr, "error: size != int_size in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    err = MPI_Type_extent(newtype, &extent);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type extent in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    if (extent != int_size) {
        if (verbose) {
            fprintf(stderr, "error: extent != int_size in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_commit(&newtype);

    err = pack_and_unpack((char *) buf, 1, newtype, 4 * sizeof(int));
    if (err != 0) {
        if (verbose) {
            fprintf(stderr, "error packing/unpacking in hindexed_block_contig_test()\n");
        }
        errs += err;
    }

    for (i = 0; i < 4; i++) {
        int goodval;

        switch (i) {
        case 0:
            goodval = 7;
            break;
        default:
            goodval = 0;        /** pack_and_unpack() zeros before unpack */
            break;
        }
        if (buf[i] != goodval) {
            errs++;
            if (verbose)
                fprintf(stderr, "buf[%d] = %d; should be %d\n", i, buf[i], goodval);
        }
    }

    MPI_Type_free(&newtype);

    return errs;
}

/** hindexed_block_vector_test()
 *
 * Tests behavior with a hindexed_block of some vector types;
 * this shouldn't be easily convertable into anything else.
 *
 * Returns the number of errors encountered.
 */
int hindexed_block_vector_test(void)
{
#define NELT (18)
    int buf[NELT] = {
        -1, -1, -1,
         1, -2,  2,
        -3, -3, -3,
        -4, -4, -4,
         3, -5,  4,
         5, -6,  6
    };
    int expected[NELT] = {
         0,  0,  0,
         1,  0,  2,
         0,  0,  0,
         0,  0,  0,
         3,  0,  4,
         5,  0,  6
    };
    int err, errs = 0;

    int i, count = 3;
    MPI_Aint disp[] = { 1, 4, 5 };
    MPI_Datatype vectype, newtype;

    int size, int_size;
    MPI_Aint extent;

    /** create a vector type of 2 ints, skipping one in between */
    err = MPI_Type_vector(2, 1, 2, MPI_INT, &vectype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error creating vector type in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_commit(&vectype);

    MPI_Type_extent(vectype, &extent);
    for (i = 0; i < count; i++)
        disp[i] *= extent;

    err = MPIX_Type_create_hindexed_block(count, 1, disp, vectype, &newtype);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr,
                    "error creating hindexed_block type in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_commit(&newtype);

    err = MPI_Type_size(newtype, &size);
    if (err != MPI_SUCCESS) {
        if (verbose) {
            fprintf(stderr, "error obtaining type size in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_size(MPI_INT, &int_size);

    if (size != 6 * int_size) {
        if (verbose) {
            fprintf(stderr, "error: size != 6 * int_size in hindexed_block_contig_test()\n");
        }
        errs++;
    }

    MPI_Type_extent(newtype, &extent);

    err = pack_and_unpack((char *) buf, 1, newtype, NELT * sizeof(int));
    if (err != 0) {
        if (verbose) {
            fprintf(stderr, "error packing/unpacking in hindexed_block_vector_test()\n");
        }
        errs += err;
    }

    for (i = 0; i < NELT; i++) {
        if (buf[i] != expected[i]) {
            errs++;
            if (verbose)
                fprintf(stderr, "buf[%d] = %d; should be %d\n", i, buf[i], expected[i]);
        }
    }

    MPI_Type_free(&vectype);
    MPI_Type_free(&newtype);
    return errs;
}


/** pack_and_unpack()
 *
 * Perform packing and unpacking of a buffer for the purposes of checking
 * to see if we are processing a type correctly.  Zeros the buffer between
 * these two operations, so the data described by the type should be in
 * place upon return but all other regions of the buffer should be zero.
 *
 * Parameters:
 * typebuf - pointer to buffer described by datatype and count that
 *           will be packed and then unpacked into
 * count, datatype - description of typebuf
 * typebufsz - size of typebuf; used specifically to zero the buffer
 *             between the pack and unpack steps
 *
 */
static int pack_and_unpack(char *typebuf, int count, MPI_Datatype datatype, int typebufsz)
{
    char *packbuf;
    int err, errs = 0, pack_size, type_size, position;

    err = MPI_Type_size(datatype, &type_size);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Type_size call; aborting after %d errors\n", errs);
        }
        return errs;
    }

    type_size *= count;

    err = MPI_Pack_size(count, datatype, MPI_COMM_SELF, &pack_size);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Pack_size call; aborting after %d errors\n", errs);
        }
        return errs;
    }
    packbuf = (char *) malloc(pack_size);
    if (packbuf == NULL) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in malloc call; aborting after %d errors\n", errs);
        }
        return errs;
    }

    position = 0;
    err = MPI_Pack(typebuf, count, datatype, packbuf, type_size, &position, MPI_COMM_SELF);

    if (position != type_size) {
        errs++;
        if (verbose)
            fprintf(stderr, "position = %d; should be %d (pack)\n", position, type_size);
    }

    memset(typebuf, 0, typebufsz);
    position = 0;
    err = MPI_Unpack(packbuf, type_size, &position, typebuf, count, datatype, MPI_COMM_SELF);
    if (err != MPI_SUCCESS) {
        errs++;
        if (verbose) {
            fprintf(stderr, "error in MPI_Unpack call; aborting after %d errors\n", errs);
        }
        return errs;
    }
    free(packbuf);

    if (position != type_size) {
        errs++;
        if (verbose)
            fprintf(stderr, "position = %d; should be %d (unpack)\n", position, type_size);
    }

    return errs;
}

int parse_args(int argc, char **argv)
{
    if (argc > 1 && strcmp(argv[1], "-v") == 0)
        verbose = 1;
    return 0;
}
#endif /**defined(TEST_HINDEXED_BLOCK)*/

}