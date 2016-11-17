#ifndef _STARS_H_
#define _STARS_H_

typedef struct Array Array;
typedef struct List List;
typedef struct STARS_Problem STARS_Problem;
typedef struct STARS_Cluster STARS_Cluster;
typedef struct STARS_BLRF STARS_BLRF;
typedef struct STARS_BLRM STARS_BLRM;
typedef int (*block_kernel)(int, int, int *, int *, void *, void *, void *);

typedef enum {STARS_Dense, STARS_LowRank, STARS_Unknown} STARS_BlockStatus;
// Enum type to show lowrankness of admissible blocks

typedef enum {STARS_BLRF_Tiled, STARS_BLRF_H, STARS_BLRF_HODLR}
    STARS_BLRF_Type;
// Enum type to show format type (tiled, hierarchical HOLDR or hierarchical H).

typedef enum {STARS_ClusterTiled, STARS_ClusterHierarchical}
    STARS_ClusterType;
// Enum type to show type of clusterization.

struct Array
// N-dimensional array
{
    int ndim;
    // Number of dimensions of array.
    int *shape;
    // Shape of array.
    int *stride;
    // Size of step to increase value of corresponding axis by 1
    char order;
    // C or Fortran order. C-order means stride is descending, Fortran-order
    // means stride is ascending.
    int size;
    // Number of elements of an array.
    char dtype;
    // Data type of each element of array. Possile value is 's', 'd', 'c' or
    // 'z', much like in names of LAPACK routines.
    size_t dtype_size;
    // Size in bytes of each element of a matrix
    size_t nbytes;
    // Size of buffer in bytes.
    void *buffer;
    // Buffer, containing array. Stored in Fortran style.
};

// Routines to work with N-dimensional arrays
Array *Array_from_buffer(int ndim, int *shape, char dtype, char order,
        void *buffer);
// Init array from given buffer. Check if all parameters are good and
// proceed.
Array *Array_new(int ndim, int *shape, char dtype, char order);
// Allocation of memory for array
Array *Array_new_like(Array *array);
// Initialize new array with exactly the same shape, dtype and so on, but
// with a different memory buffer
Array *Array_copy(Array *array, char order);
// Create copy of array with given data layout or keeping layout if order ==
// 'N'
void Array_free(Array *array);
// Free memory, consumed by array structure and buffer
void Array_info(Array *array);
// Print all the data from Array structure
void Array_print(Array *array);
// Print elements of array, different rows of array are printed on different
// rows of output
void Array_init(Array *array, char *kind);
// Init buffer in a special manner: randn, rand, ones or zeros
void Array_init_randn(Array *array);
// Init buffer of array with random numbers of normal (0,1) distribution
void Array_init_rand(Array *array);
// Init buffer with random numbers of uniform [0,1] distribution
void Array_init_zeros(Array *array);
// Set all elements to 0.0
void Array_init_ones(Array *array);
// Set all elements to 1.0
void Array_tomatrix(Array *array, char kind);
// Convert N-dimensional array to 2-dimensional array (matrix) by
// collapsing dimensions. This collapse can be assumed as attempt to look
// at array as at a matrix with long rows (kind == 'R') or long columns
// (kind == 'C'). If kind is 'R', dimensions from 1 to the last are
// collapsed into columns. If kind is 'C', dimensions from 0 to the last
// minus one are collapsed into rows. Example: array of shape (2,3,4,5)
// will be collapsed to array of shape (2,60) if kind is 'R' or to array of
// shape (24,5) if kind is 'C'.
void Array_trans(Array *array);
// Transposition of array. No real transposition is performed, only changes
// shape, stride and order.
Array *Array_dot(Array* A, Array *B);
// GEMM for two arrays. Multiplication is performed by last dimension of
// array A and first dimension of array B. These dimensions, data types and
// ordering of both arrays should be equal.
int Array_SVD(Array *array, Array **U, Array **S, Array **V);
// Compute short SVD of 2-dimensional array
int SVD_get_rank(Array *S, double tol, char type);
// Find rank by singular values and given accuracy tolerance
// (Frobenius or Spectral norm)
void Array_scale(Array *array, char kind, Array *factor);
// Apply row or column scaling to array
double Array_diff(Array *array, Array *array2);
// Measure Frobenius error of approximation of array by array2
double Array_norm(Array *array);
// Measure Frobenius norm of array
Array *Array_convert(Array *array, char dtype);
// Copy array and convert data type
int SVD_get_rank(Array *S, double tol, char type);
// Returns rank by given array of singular values, tolerance and type of norm
// ('2' for spectral norm, 'F' for Frobenius norm)
int Array_Cholesky(Array *array, char uplo);
// Cholesky factoriation for an array


struct STARS_Problem
// Structure, storing all the necessary data for reconstruction of matrix,
// generated by given kernel. This matrix may be not 2-dimensional (e.g. for
// Astrophysics problem, where each matrix entry is a vector of 3 elements.
// Matrix elements are not stored in memory, but computed on demand.
// Rows correspond to first dimension of the array and columns correspond to
// last dimension of the array.
{
    int ndim;
    // Real dimensionality of corresponding array. ndim=2 for problems with
    // scalar kernel. ndim=3 for Astrophysics problem. May be greater than 2.
    int *shape;
    // Real shape of corresponding array.
    char symm;
    // 'S' if problem is symmetric, and 'N' otherwise.
    char dtype;
    // Possible values are 's', 'd', 'c' or 'z', just as in LAPACK routines
    // names.
    size_t dtype_size;
    // Size of data type in bytes (size of scalar element of array).
    size_t entry_size;
    // Size of matrix entry in bytes (size of subarray, corresponding to
    // matrix entry on a given row on a given column). Equal to dtype_size,
    // multiplied by total number of elements and divided by number of rows
    // and by number of columns.
    void *row_data, *col_data;
    // Pointers to physical data, corresponding to rows and columns.
    block_kernel kernel;
    // Pointer to a function, returning submatrix on intersection of
    // given rows and columns. Rows stand for first dimension, columns stand
    // for last dimension.
    char *name;
    // Name of problem, useful for printing additional info. It is up to user
    // to set it as desired.
};

STARS_Problem *STARS_Problem_init(int ndim, int *shape, char symm, char dtype,
        void *row_data, void *col_data, block_kernel kernel, char *name);
// Init for STARS_Problem instance
// Parameters:
//   ndim: dimensionality of corresponding array. Equal 2+dimensionality of
//     kernel
//   shape: shape of corresponding array. shape[1:ndim-2] is equal to shape of
//     kernel
//   symm: 'S' for summetric problem, 'N' for nonsymmetric problem. Symmetric
//     problem require symmetric kernel and equality of row_data and col_data
//   dtype: data type of the problem. Equal to 's', 'd', 'c' or 'z' as in
//     LAPACK routines. Stands for data type of an element of a kernel.
//   row_data: pointer to some structure of physical data for rows
//   col_data: pointer to some srructure of physical data for columns
//   kernel: pointer to a function of interaction. More on this is written
//     somewhere else.
//   name: string, containgin name of the problem. Used only to print
//     information about structure problem.
// Returns:
//    STARS_Problem *: pointer to structure problem with proper filling of all
//    the fields of structure.
void STARS_Problem_free(STARS_Problem *problem);
// Free memory, consumed by data buffers of data
void STARS_Problem_info(STARS_Problem *problem);
// Print some info about Problem
Array *STARS_Problem_get_block(STARS_Problem *problem, int nrows, int ncols,
        int *irow, int *icol);
// Get submatrix on given rows and columns (rows=first dimension, columns=last
// dimension)
STARS_Problem *STARS_Problem_from_array(Array *array, char symm);
// Generate STARS_Problem with a given array and flag if it is symmetric
Array *STARS_Problem_to_array(STARS_Problem *problem);
// Compute matrix/array, corresponding to the problem


struct STARS_Cluster
// Information on clusterization (hierarchical or plain) of physical data.
{
    void *data;
    // Pointer to structure, holding physical data.
    int ndata;
    // Number of discrete elements, corresponding to physical data (particles,
    // grid nodes or mesh elements).
    int *pivot;
    // Pivoting for clusterization. After applying this pivoting, discrete
    // elements, corresponding to a given cluster, have indexes in a row. pivot
    // has ndata elements.
    int nblocks;
    // Total number of subclusters/blocks of discrete elements.
    int nlevels;
    // Number of levels of hierarchy. 0 in case of tiled cluster.
    int *level;
    // Compressed format to store start points of each level of hierarchy.
    // indexes of subclusters from level[i] to level[i+1]-1 correspond to i-th
    // level of hierarchy. level has nlevels+1 elements in hierarchical case
    // and is NULL in tiled case.
    int *start, *size;
    // Start points in array pivot and corresponding sizes of each subcluster
    // of discrete elements. Since subclusters overlap in hierarchical
    // case, value start[i+1]-start[i] does not represent actual size of
    // subcluster. start and size have nblocks elements.
    int *parent;
    // Array of parents, each node has only one parent. parent[0] = -1 since 0
    // is assumed to be root node. In case of tiled cluster, parent is NULL.
    int *child_start, *child;
    // Arrays of children and start points in array of children of each
    // subcluster. child_start has nblocks+1 elements, child has nblocks
    // elements in case of hierarchical cluster. In case of tiled cluster
    // child and child_start are NULL.
    STARS_ClusterType type;
    // Type of cluster (tiled or hierarchical).
};

STARS_Cluster *STARS_Cluster_init(void *data, int ndata, int *pivot,
        int nblocks, int nlevels, int *level, int *start, int *size,
        int *parent, int *child_start, int *child, STARS_ClusterType type);
// Init for STARS_Cluster instance
// Parameters:
//   data: pointer structure, holding to physical data.
//   ndata: number of discrete elements (particles or mesh elements),
//     corresponding to physical data.
//   pivot: pivoting of clusterization. After applying this pivoting, rows (or
//     columns), corresponding to one block are placed in a row.
//   nblocks: number of blocks/block rows/block columns/subclusters.
//   nlevels: number of levels of hierarchy.
//   level: array of size nlevels+1, indexes of blocks from level[i] to
//     level[i+1]-1 inclusively belong to i-th level of hierarchy.
//   start: start point of of indexes of discrete elements of each
//     block/subcluster in array pivot.
//   size: size of each block/subcluster/block row/block column.
//   parent: array of parents, size is nblocks.
//   child_start: array of start points in array child of each subcluster.
//   child: array of children of each subcluster.
//   type: type of cluster. Tiled with STARS_ClusterTiled or hierarchical with
//     STARS_ClusterHierarchical.
void STARS_Cluster_free(STARS_Cluster *cluster);
// Free data buffers, consumed by clusterization information.
void STARS_Cluster_info(STARS_Cluster *cluster);
// Print some info about clusterization
STARS_Cluster *STARS_Cluster_init_tiled(void *data, int ndata, int block_size);
// Plain (non-hierarchical) division of data into blocks of discrete elements.


struct STARS_BLRF
// STARS Block Low-Rank Format, means non-nested division of a matrix/array
// into admissible blocks. Some of admissible blocks are low-rank, some are
// dense.
{
    STARS_Problem *problem;
    // Pointer to a problem.
    char symm;
    // 'S' if format and problem are symmetric, and 'N' otherwise.
    STARS_Cluster *row_cluster, *col_cluster;
    // Clusterization of rows and columns into blocks/subclusters of discrete
    // elements.
    int nbrows, nbcols;
    // Number of block rows/row subclusters and block columns/column
    // subclusters.
    int nblocks_far, nblocks_near;
    // Number of admissible far-field blocks and admissible near-field blocks.
    // Far-field blocks can be approximated with low rank, whereas near-field
    // blocks can not.
    int *block_far, *block_near;
    // Indexes of far-field and near-field admissible blocks. block[2*i] and
    // block[2*i+1] are indexes of block row and block column correspondingly.
    int *brow_far_start, *brow_far;
    // Compressed sparse format to store indexes of admissible far-field blocks
    // for each block row.
    int *bcol_far_start, *bcol_far;
    // Compressed sparse format to store indexes of admissible far-field blocks
    // for each block column.
    int *brow_near_start, *brow_near;
    // Compressed sparse format to store indexes of admissible near-field
    // blocks for each block row.
    int *bcol_near_start, *bcol_near;
    // Compressed sparse format to store indexes of admissible near-field
    // blocks for each block column.
    STARS_BLRF_Type type;
    // Type of format. Possible value is STARS_Tiled, STARS_H or STARS_HODLR.
};

STARS_BLRF *STARS_BLRF_init(STARS_Problem *problem, char symm,
        STARS_Cluster *row_cluster, STARS_Cluster *col_cluster,
        int nblocks_far, int *far_bindex, int nblocks_near, int *near_bindex,
        STARS_BLRF_Type type);
// Initialization of structure STARS_BLRF
// Parameters:
//   problem: pointer to a structure, holding all the information about problem
//   symm: 'S' if problem and division into blocks are both symmetric, 'N'
//     otherwise.
//   row_cluster: clusterization of rows into block rows.
//   col_cluster: clusterization of columns into block columns.
//   nblocks_far: number of admissible far-field blocks.
//   bindex_far: array of pairs of admissible far-filed block rows and block
//     columns. far_bindex[2*i] is an index of block row and far_bindex[2*i+1]
//     is an index of block column.
//   nblocks_near: number of admissible far-field blocks.
//   bindex_near: array of pairs of admissible near-filed block rows and block
//     columns. near_bindex[2*i] is an index of block row and near_bindex[2*i+1]
//     is an index of block column.
//   type: type of block low-rank format. Tiled with STARS_BLRF_Tiled or
//     hierarchical with STARS_BLRF_H or STARS_BLRF_HOLDR.
void STARS_BLRF_free(STARS_BLRF *blrf);
// Free memory, used by block low rank format (partitioning of array into
// blocks)
void STARS_BLRF_info(STARS_BLRF *blrf);
// Print short info on block partitioning
void STARS_BLRF_print(STARS_BLRF *blrf);
// Print full info on block partitioning
STARS_BLRF *STARS_BLRF_init_tiled(STARS_Problem *problem, STARS_Cluster
        *row_cluster, STARS_Cluster *col_cluster, char symm);
// Create plain division into tiles/blocks using plain cluster trees for rows
// and columns without actual pivoting
void STARS_BLRF_getblock(STARS_BLRF *blrf, int i, int j, int *shape, void **D);
// PLEASE CLEAN MEMORY POINTER *D AFTER USE

struct STARS_BLRM
// STARS Block Low-Rank Matrix, which is used as an approximation in non-nested
// block low-rank format.
{
    STARS_BLRF *blrf;
    // Pointer to block low-rank format.
    int *far_rank;
    // Rank of each far-field block.
    Array **far_U, **far_V, **far_D;
    // Arrays of pointers to factors U and V of each low-rank far-field block
    // and dense array of each dense far-field block.
    int onfly;
    // 1 to store dense blocks, 0 not to store them and compute on demand.
    Array **near_D;
    // Array of pointers to dense array of each near-field block.
    void *U_alloc, *V_alloc, *D_alloc;
    // Pointer to memory buffer, holding buffers of low-rank factors of
    // low-rank blocks and dense buffers of dense blocks
    char alloc_type;
    // Type of memory allocation: '1' for allocating 3 big buffers U_alloc,
    // V_alloc and D_alloc, '2' for allocating many small buffers U, V and D.
};


STARS_BLRM *STARS_BLRM_init(STARS_BLRF *blrf, int *far_rank, Array **far_U,
        Array **far_V, Array **far_D, int onfly, Array **near_D, void *U_alloc,
        void *V_alloc, void *D_alloc, char alloc_type);
// Init procedure for a non-nested block low-rank matrix
void STARS_BLRM_free(STARS_BLRM *blrm);
// Free memory of a non-nested block low-rank matrix
void STARS_BLRM_info(STARS_BLRM *blrm);
// Print short info on non-nested block low-rank matrix
void STARS_BLRM_error(STARS_BLRM *blrm);
// Measure error of approximation by non-nested block low-rank matrix
void STARS_BLRM_getblock(STARS_BLRM *mat, int i, int j, int *shape, int *rank,
        void **U, void **V, void **D);
// Returns shape of block, its rank and low-rank factors or dense
// representation of a block
STARS_BLRM *STARS_blrf_tiled_compress_algebraic_svd(STARS_BLRF *blrf,
        int maxrank, double tol, int onfly);
// Private function of STARS-H
// Uses SVD to acquire rank of each block, compresses given matrix (given
// by block kernel, which returns submatrices) with relative accuracy tol
// or with given maximum rank (if maxrank <= 0, then tolerance is used)


#endif // _STARS_H_
