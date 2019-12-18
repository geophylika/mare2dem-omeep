/** @file slu_util.h
 * \brief Utility header file 
 *
 * -- SuperLU routine (version 3.1) --
 * Univ. of California Berkeley, Xerox Palo Alto Research Center,
 * and Lawrence Berkeley National Lab.
 * August 1, 2008
 *
 */

#ifndef __SUPERLU_UTIL /* allow multiple inclusions */
#define __SUPERLU_UTIL

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/*
#ifndef __STDC__
#include <malloc.h>
#endif
*/
#include <assert.h>

/***********************************************************************
 * Macros
 ***********************************************************************/
#define FIRSTCOL_OF_SNODE(i)	(xsup[i])
/* No of marker arrays used in the symbolic factorization,
   each of size n */
#define NO_MARKER     3
#define NUM_TEMPV(m,w,t,b)  ( SUPERLU_MAX(m, (t + b)*w) )

#ifndef USER_ABORT
#define USER_ABORT(msg) superlu_abort_and_exit(msg)
#endif

#define ABORT(err_msg) \
 { char msg[256];\
   sprintf(msg,"%s at line %d in file %s\n",err_msg,__LINE__, __FILE__);\
   USER_ABORT(msg); }


#ifndef USER_MALLOC
#if 1
#define USER_MALLOC(size) superlu_malloc(size)
#else
/* The following may check out some uninitialized data */
#define USER_MALLOC(size) memset (superlu_malloc(size), '\x0F', size)
#endif
#endif

#define SUPERLU_MALLOC(size) USER_MALLOC(size)

#ifndef USER_FREE
#define USER_FREE(addr) superlu_free(addr)
#endif

#define SUPERLU_FREE(addr) USER_FREE(addr)

#define CHECK_MALLOC(where) {                 \
    extern int superlu_malloc_total;        \
    printf("%s: malloc_total %d Bytes\n",     \
	   where, superlu_malloc_total); \
}

#define SUPERLU_MAX(x, y) 	( (x) > (y) ? (x) : (y) )
#define SUPERLU_MIN(x, y) 	( (x) < (y) ? (x) : (y) )

/*********************************************************
 * Macros used for easy access of sparse matrix entries. *
 *********************************************************/
#define L_SUB_START(col)     ( Lstore->rowind_colptr[col] )
#define L_SUB(ptr)           ( Lstore->rowind[ptr] )
#define L_NZ_START(col)      ( Lstore->nzval_colptr[col] )
#define L_FST_SUPC(superno)  ( Lstore->sup_to_col[superno] )
#define U_NZ_START(col)      ( Ustore->colptr[col] )
#define U_SUB(ptr)           ( Ustore->rowind[ptr] )


/***********************************************************************
 * Constants 
 ***********************************************************************/
#define EMPTY	(-1)
/*#define NO	(-1)*/
#define FALSE	0
#define TRUE	1

#define NO_MEMTYPE  4      /* 0: lusup;
			      1: ucol;
			      2: lsub;
			      3: usub */

#define GluIntArray(n)   (5 * (n) + 5)

/* Dropping rules */
#define  NODROP	        ( 0x0000 )
#define	 DROP_BASIC	( 0x0001 )  /* ILU(tau) */
#define  DROP_PROWS	( 0x0002 )  /* ILUTP: keep p maximum rows */
#define  DROP_COLUMN	( 0x0004 )  /* ILUTP: for j-th column, 
				              p = gamma * nnz(A(:,j)) */
#define  DROP_AREA 	( 0x0008 )  /* ILUTP: for j-th column, use
 		 			      nnz(F(:,1:j)) / nnz(A(:,1:j))
					      to limit memory growth  */
#define  DROP_SECONDARY	( 0x000E )  /* PROWS | COLUMN | AREA */
#define  DROP_DYNAMIC	( 0x0010 )  /* adaptive tau */
#define  DROP_INTERP	( 0x0100 )  /* use interpolation */


#if 1
#define MILU_ALPHA (1.0e-2) /* multiple of drop_sum to be added to diagonal */
#else
#define MILU_ALPHA  1.0 /* multiple of drop_sum to be added to diagonal */
#endif


/***********************************************************************
 * Enumerate types
 ***********************************************************************/
typedef enum {NO, YES}                                          yes_no_t;
typedef enum {DOFACT, SamePattern, SamePattern_SameRowPerm, FACTORED} fact_t;
typedef enum {NOROWPERM, LargeDiag, MY_PERMR}                   rowperm_t;
typedef enum {NATURAL, MMD_ATA, MMD_AT_PLUS_A, COLAMD, MY_PERMC}colperm_t;
typedef enum {NOTRANS, TRANS, CONJ}                             trans_t;
typedef enum {NOEQUIL, ROW, COL, BOTH}                          DiagScale_t;
typedef enum {NOREFINE, SINGLE=1, DOUBLE, EXTRA}                IterRefine_t;
typedef enum {LUSUP, UCOL, LSUB, USUB}                          MemType;
typedef enum {HEAD, TAIL}                                       stack_end_t;
typedef enum {SYSTEM, USER}                                     LU_space_t;
typedef enum {ONE_NORM, TWO_NORM, INF_NORM}			norm_t;
typedef enum {SILU, SMILU_1, SMILU_2, SMILU_3}			milu_t;
#if 0
typedef enum {NODROP		= 0x0000,
	      DROP_BASIC	= 0x0001, /* ILU(tau) */
	      DROP_PROWS	= 0x0002, /* ILUTP: keep p maximum rows */
	      DROP_COLUMN	= 0x0004, /* ILUTP: for j-th column, 
					     p = gamma * nnz(A(:,j)) */
	      DROP_AREA 	= 0x0008, /* ILUTP: for j-th column, use
					     nnz(F(:,1:j)) / nnz(A(:,1:j))
					     to limit memory growth  */
	      DROP_SECONDARY	= 0x000E, /* PROWS | COLUMN | AREA */
	      DROP_DYNAMIC	= 0x0010,
	      DROP_INTERP	= 0x0100}			rule_t;
#endif


/* 
 * The following enumerate type is used by the statistics variable 
 * to keep track of flop count and time spent at various stages.
 *
 * Note that not all of the fields are disjoint.
 */
typedef enum {
    COLPERM, /* find a column ordering that minimizes fills */
    RELAX,   /* find artificial supernodes */
    ETREE,   /* compute column etree */
    EQUIL,   /* equilibrate the original matrix */
    FACT,    /* perform LU factorization */
    RCOND,   /* estimate reciprocal condition number */
    SOLVE,   /* forward and back solves */
    REFINE,  /* perform iterative refinement */
    TRSV,    /* fraction of FACT spent in xTRSV */
    GEMV,    /* fraction of FACT spent in xGEMV */
    FERR,    /* estimate error bounds after iterative refinement */
    NPHASES  /* total number of phases */
} PhaseType;


/***********************************************************************
 * Type definitions
 ***********************************************************************/
typedef float    flops_t;
typedef unsigned char Logical;

/* 
 *-- This contains the options used to control the solve process.
 *
 * Fact   (fact_t)
 *        Specifies whether or not the factored form of the matrix
 *        A is supplied on entry, and if not, how the matrix A should
 *        be factorizaed.
 *        = DOFACT: The matrix A will be factorized from scratch, and the
 *             factors will be stored in L and U.
 *        = SamePattern: The matrix A will be factorized assuming
 *             that a factorization of a matrix with the same sparsity
 *             pattern was performed prior to this one. Therefore, this
 *             factorization will reuse column permutation vector 
 *             ScalePermstruct->perm_c and the column elimination tree
 *             LUstruct->etree.
 *        = SamePattern_SameRowPerm: The matrix A will be factorized
 *             assuming that a factorization of a matrix with the same
 *             sparsity	pattern and similar numerical values was performed
 *             prior to this one. Therefore, this factorization will reuse
 *             both row and column scaling factors R and C, both row and
 *             column permutation vectors perm_r and perm_c, and the
 *             data structure set up from the previous symbolic factorization.
 *        = FACTORED: On entry, L, U, perm_r and perm_c contain the 
 *              factored form of A. If DiagScale is not NOEQUIL, the matrix
 *              A has been equilibrated with scaling factors R and C.
 *
 * Equil  (yes_no_t)
 *        Specifies whether to equilibrate the system (scale A's row and
 *        columns to have unit norm).
 *
 * ColPerm (colperm_t)
 *        Specifies what type of column permutation to use to reduce fill.
 *        = NATURAL: use the natural ordering 
 *        = MMD_ATA: use minimum degree ordering on structure of A'*A
 *        = MMD_AT_PLUS_A: use minimum degree ordering on structure of A'+A
 *        = COLAMD: use approximate minimum degree column ordering
 *        = MY_PERMC: use the ordering specified in ScalePermstruct->perm_c[]
 *         
 * Trans  (trans_t)
 *        Specifies the form of the system of equations:
 *        = NOTRANS: A * X = B        (No transpose)
 *        = TRANS:   A**T * X = B     (Transpose)
 *        = CONJ:    A**H * X = B     (Transpose)
 *
 * IterRefine (IterRefine_t)
 *        Specifies whether to perform iterative refinement.
 *        = NO: no iterative refinement
 *        = WorkingPrec: perform iterative refinement in working precision
 *        = ExtraPrec: perform iterative refinement in extra precision
 *
 * DiagPivotThresh (double, in [0.0, 1.0]) (only for sequential SuperLU)
 *        Specifies the threshold used for a diagonal entry to be an
 *        acceptable pivot.
 *
 * PivotGrowth (yes_no_t)
 *        Specifies whether to compute the reciprocal pivot growth.
 *
 * ConditionNumber (ues_no_t)
 *        Specifies whether to compute the reciprocal condition number.
 *
 * RowPerm (rowperm_t) (only for SuperLU_DIST or ILU)
 *        Specifies whether to permute rows of the original matrix.
 *        = NO: not to permute the rows
 *        = LargeDiag: make the diagonal large relative to the off-diagonal
 *        = MY_PERMR: use the permutation given in ScalePermstruct->perm_r[]
 *           
 * SymmetricMode (yest_no_t)
 *        Specifies whether to use symmetric mode.
 *
 * PrintStat (yes_no_t)
 *        Specifies whether to print the solver's statistics.
 *
 * ReplaceTinyPivot (yes_no_t) (only for SuperLU_DIST)
 *        Specifies whether to replace the tiny diagonals by
 *        sqrt(epsilon)*||A|| during LU factorization.
 *
 * SolveInitialized (yes_no_t) (only for SuperLU_DIST)
 *        Specifies whether the initialization has been performed to the
 *        triangular solve.
 *
 * RefineInitialized (yes_no_t) (only for SuperLU_DIST)
 *        Specifies whether the initialization has been performed to the
 *        sparse matrix-vector multiplication routine needed in iterative
 *        refinement.
 */
typedef struct {
    fact_t        Fact;
    yes_no_t      Equil;
    colperm_t     ColPerm;
    trans_t       Trans;
    IterRefine_t  IterRefine;
    double        DiagPivotThresh;
    yes_no_t      PivotGrowth;
    yes_no_t      ConditionNumber;
    rowperm_t     RowPerm;
    yes_no_t      SymmetricMode;
    yes_no_t      PrintStat;
    yes_no_t      ReplaceTinyPivot;
    yes_no_t      SolveInitialized;
    yes_no_t      RefineInitialized;
    double	  ILU_DropTol;    /* threshold for dropping */
    double	  ILU_FillTol;    /* threshold for zero pivot perturbation */
    double	  ILU_FillFactor; /* gamma in the secondary dropping */
    int 	  ILU_DropRule;
    norm_t	  ILU_Norm;
    milu_t	  ILU_MILU;
} superlu_options_t;

/*! \brief Headers for 4 types of dynamatically managed memory */
typedef struct e_node {
    int size;      /* length of the memory that has been used */
    void *mem;     /* pointer to the new malloc'd store */
} ExpHeader;

typedef struct {
    int  size;
    int  used;
    int  top1;  /* grow upward, relative to &array[0] */
    int  top2;  /* grow downward */
    void *array;
} LU_stack_t;

typedef struct {
    int     *panel_histo; /* histogram of panel size distribution */
    double  *utime;       /* running time at various phases */
    flops_t *ops;         /* operation count at various phases */
    int     TinyPivots;   /* number of tiny pivots */
    int     RefineSteps;  /* number of iterative refinement steps */
    int     expansions;   /* number of memory expansions */
} SuperLUStat_t;

typedef struct {
    float for_lu;
    float total_needed;
} mem_usage_t;


/***********************************************************************
 * Prototypes
 ***********************************************************************/
#ifdef __cplusplus
extern "C" {
#endif

extern void    Destroy_SuperMatrix_Store(SuperMatrix *);
extern void    Destroy_CompCol_Matrix(SuperMatrix *);
extern void    Destroy_CompRow_Matrix(SuperMatrix *);
extern void    Destroy_SuperNode_Matrix(SuperMatrix *);
extern void    Destroy_CompCol_Permuted(SuperMatrix *);
extern void    Destroy_Dense_Matrix(SuperMatrix *);
extern void    get_perm_c(int, SuperMatrix *, int *);
extern void    set_default_options(superlu_options_t *options);
extern void    ilu_set_default_options(superlu_options_t *options);
extern void    sp_preorder (superlu_options_t *, SuperMatrix*, int*, int*,
			    SuperMatrix*);
extern void    superlu_abort_and_exit(char*);
extern void    *superlu_malloc (size_t);
extern int     *intMalloc (int);
extern int     *intCalloc (int);
extern void    superlu_free (void*);
extern void    SetIWork (int, int, int, int *, int **, int **, int **,
                         int **, int **, int **, int **);
extern int     sp_coletree (int *, int *, int *, int, int, int *);
extern void    relax_snode (const int, int *, const int, int *, int *);
extern void    heap_relax_snode (const int, int *, const int, int *, int *);
extern int     mark_relax(int, int *, int *, int *, int *, int *, int *);
extern void    ilu_relax_snode (const int, int *, const int, int *,
				int *, int *);
extern void    ilu_heap_relax_snode (const int, int *, const int, int *,
				     int *, int*);
extern void    resetrep_col (const int, const int *, int *);
extern int     spcoletree (int *, int *, int *, int, int, int *);
extern int     *TreePostorder (int, int *);
extern double  SuperLU_timer_ ();
extern int     sp_ienv (int);
extern int     lsame_ (char *, char *);
extern int     xerbla_ (char *, int *);
extern void    ifill (int *, int, int);
extern void    snode_profile (int, int *);
extern void    super_stats (int, int *);
extern void    check_repfnz(int, int, int, int *);
extern void    PrintSumm (char *, int, int, int);
extern void    StatInit(SuperLUStat_t *);
extern void    StatPrint (SuperLUStat_t *);
extern void    StatFree(SuperLUStat_t *);
extern void    print_panel_seg(int, int, int, int, int *, int *);
extern int     print_int_vec(char *,int, int *);
extern int     PrintInt10(char *, int, int *);

#ifdef __cplusplus
  }
#endif

#endif /* __SUPERLU_UTIL */
