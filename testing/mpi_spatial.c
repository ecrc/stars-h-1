/*! @copyright (c) 2017 King Abdullah University of Science and
 *                      Technology (KAUST). All rights reserved.
 *
 * STARS-H is a software package, provided by King Abdullah
 *             University of Science and Technology (KAUST)
 *
 * @file testing/mpi_spatial.c
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2017-08-22
 * */

#ifdef MKL
    #include <mkl.h>
#else
    #include <cblas.h>
    #include <lapacke.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <string.h>
#include "starsh.h"
#include "starsh-spatial.h"

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);
    int mpi_size, mpi_rank;
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    if(argc != 12)
    {
        if(mpi_rank == 0)
        {
            printf("%d arguments provided, but 11 are needed\n",
                    argc-1);
            printf("mpi_spatial ndim placement kernel beta nu N block_size "
                    "maxrank tol check_matvec check_cg_solve\n");
        }
        MPI_Finalize();
        exit(0);
    }
    int problem_ndim = atoi(argv[1]);
    int place = atoi(argv[2]);
    int kernel_type = atoi(argv[3]);
    double beta = atof(argv[4]);
    double nu = atof(argv[5]);
    int N = atoi(argv[6]);
    int block_size = atoi(argv[7]);
    int maxrank = atoi(argv[8]);
    double tol = atof(argv[9]);
    double noise = 0;
    int check_matvec = atoi(argv[10]);
    int check_cg_solve = atoi(argv[11]);
    int onfly = 0;
    char symm = 'N', dtype = 'd';
    int ndim = 2;
    STARSH_int shape[2] = {N, N};
    // Possible values can be found in documentation for enum
    // STARSH_PARTICLES_PLACEMENT
    int nrhs = 1;
    int info;
    srand(0);
    // Init STARS-H
    starsh_init();
    // Generate data for spatial statistics problem
    STARSH_ssdata *data;
    STARSH_kernel *kernel;
    //starsh_gen_ssdata(&data, &kernel, n, beta);
    info = starsh_application((void **)&data, &kernel, N, dtype,
            STARSH_SPATIAL, kernel_type, STARSH_SPATIAL_NDIM, problem_ndim,
            STARSH_SPATIAL_BETA, beta, STARSH_SPATIAL_NU, nu,
            STARSH_SPATIAL_NOISE, noise, STARSH_SPATIAL_PLACE, place, 0);
    //starsh_particles_write_to_file_pointer_ascii(&data->particles, stdout);
    if(info != 0)
    {
        if(mpi_rank == 0)
            printf("Problem was NOT generated (wrong parameters)\n");
        MPI_Finalize();
        exit(0);
    }
    // Init problem with given data and kernel and print short info
    STARSH_problem *P;
    starsh_problem_new(&P, ndim, shape, symm, dtype, data, data,
            kernel, "Spatial Statistics example");
    if(mpi_rank == 0)
        starsh_problem_info(P); 
    // Init plain clusterization and print info
    STARSH_cluster *C;
    starsh_cluster_new_plain(&C, data, N, block_size);
    if(mpi_rank == 0)
        starsh_cluster_info(C);
    // Init tlr division into admissible blocks and print short info
    STARSH_blrf *F;
    STARSH_blrm *M;
    starsh_blrf_new_tlr_mpi(&F, P, symm, C, C);
    if(mpi_rank == 0)
        starsh_blrf_info(F);
    // Approximate each admissible block
    MPI_Barrier(MPI_COMM_WORLD);
    double time1 = MPI_Wtime();
    info = starsh_blrm_approximate(&M, F, maxrank, tol, onfly);
    if(info != 0)
    {
        if(mpi_rank == 0)
            printf("Approximation was NOT computed due to error\n");
        MPI_Finalize();
        exit(0);
    }
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime()-time1;
    if(mpi_rank == 0)
    {
        starsh_blrf_info(F);
        starsh_blrm_info(M);
    }
    if(mpi_rank == 0)
        printf("TIME TO APPROXIMATE: %e secs\n", time1);
    // Measure approximation error
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime();
    double rel_err = starsh_blrm__dfe_mpi(M);
    MPI_Barrier(MPI_COMM_WORLD);
    time1 = MPI_Wtime()-time1;
    if(mpi_rank == 0)
    {
        printf("TIME TO MEASURE ERROR: %e secs\nRELATIVE ERROR: %e\n",
                time1, rel_err);
        if(rel_err/tol > 10.)
        {
            printf("Resulting relative error is too big\n");
            MPI_Finalize();
            exit(1);
        }
    }
    /*
    if(rel_err/tol > 10.)
        exit(1);
    // Flush STDOUT, since next step is very time consuming
    fflush(stdout);
    // Measure time for 10 BLRM matvecs and for 10 BLRM TLR matvecs
    if(check_matvec == 1)
    {
        double *x, *y, *y_tiled;
        int nrhs = 1;
        x = malloc(N*nrhs*sizeof(*y));
        y = malloc(N*nrhs*sizeof(*y));
        y_tiled = malloc(N*nrhs*sizeof(*y_tiled));
        if(mpi_rank == 0)
        {
            int iseed[4] = {0, 0, 0, 1};
            LAPACKE_dlarnv_work(3, iseed, N*nrhs, x);
            cblas_dscal(N*nrhs, 0.0, y, 1);
            cblas_dscal(N*nrhs, 0.0, y_tiled, 1);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime();
        for(int i = 0; i < 10; i++)
            starsh_blrm__dmml_mpi(M, nrhs, 1.0, x, N, 0.0, y, N);
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime()-time1;
        if(mpi_rank == 0)
        {
            printf("TIME FOR 10 BLRM MATVECS: %e secs\n", time1);
        }
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime();
        for(int i = 0; i < 10; i++)
            starsh_blrm__dmml_mpi_tiled(M, nrhs, 1.0, x, N, 0.0, y_tiled, N);
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime()-time1;
        if(mpi_rank == 0)
        {
            cblas_daxpy(N, -1.0, y, 1, y_tiled, 1);
            printf("TIME FOR 10 TLR MATVECS: %e secs\n", time1);
            printf("MATVEC DIFF: %e\n", cblas_dnrm2(N, y_tiled, 1)
                    /cblas_dnrm2(N, y, 1));
        }
    }
    // Measure time for 10 BLRM and TLR matvecs and then solve with CG, initial
    // solution x=0, b is RHS and r is residual
    if(check_cg_solve == 1)
    {
        double *b, *x, *r, *CG_work;
        b = malloc(N*nrhs*sizeof(*b));
        x = malloc(N*nrhs*sizeof(*x));
        r = malloc(N*nrhs*sizeof(*r));
        CG_work = malloc(3*(N+1)*nrhs*sizeof(*CG_work));
        if(mpi_rank == 0)
        {
            int iseed[4] = {0, 0, 0, 1};
            LAPACKE_dlarnv_work(3, iseed, N*nrhs, b);
        }
        // Solve with CG
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime();
        int info = starsh_itersolvers__dcg_mpi(M, nrhs, b, N, x, N, tol,
                CG_work);
        MPI_Barrier(MPI_COMM_WORLD);
        time1 = MPI_Wtime()-time1;
        starsh_blrm__dmml_mpi_tiled(M, nrhs, -1.0, x, N, 0.0, r, N);
        if(mpi_rank == 0)
        {
            cblas_daxpy(N, 1.0, b, 1, r, 1);
            double norm_rhs = cblas_dnrm2(N, b, 1);
            double norm_res = cblas_dnrm2(N, r, 1);
            printf("CG INFO=%d\nCG TIME=%f secs\nCG RELATIVE ERROR IN "
                    "RHS=%e\n", info, time1, norm_res/norm_rhs);
        }
    }
    */
    MPI_Finalize();
    return 0;
}
