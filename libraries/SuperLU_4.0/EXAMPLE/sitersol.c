
#include "slu_sdefs.h"
/*
 * -- SuperLU routine (version 4.0) --
 * Lawrence Berkeley National Laboratory
 * June 30, 2009
 */

int *GLOBAL_PERM_C, *GLOBAL_PERM_R;
SuperMatrix *GLOBAL_A, *GLOBAL_L, *GLOBAL_U;
SuperLUStat_t *GLOBAL_STAT;

void smatvec_mult(float alpha, float x[], float beta, float y[])
{
    SuperMatrix *A = GLOBAL_A;

    sp_sgemv("N", alpha, A, x, 1, beta, y, 1);
}

void spsolve(int n, float x[], float y[])
{
    extern void scopy_(int *, float [], int *, float [], int *);

    int i_1 = 1;
    SuperMatrix *L = GLOBAL_L, *U = GLOBAL_U;
    SuperLUStat_t *stat = GLOBAL_STAT;
    int *perm_c = GLOBAL_PERM_C, *perm_r = GLOBAL_PERM_R;
    int info;
    static DNformat X;
    static SuperMatrix XX = {SLU_DN, SLU_S, SLU_GE, 1, 1, &X};

    scopy_(&n, y, &i_1, x, &i_1);
    XX.nrow = n;
    X.lda = n;
    X.nzval = x;
    sgstrs(NOTRANS, L, U, perm_c, perm_r, &XX, stat, &info);
}

int main(int argc, char *argv[])
{
    void smatvec_mult(float alpha, float x[], float beta, float y[]);
    void spsolve(int n, float x[], float y[]);
    extern int sfgmr( int n,
	void (*matvec_mult)(float, float [], float, float []),
	void (*psolve)(int n, float [], float[]),
	float *rhs, float *sol, double tol, int restrt, int *itmax,
	FILE *fits);
    extern int sfill_diag(int n, NCformat *Astore);

    char     equed[1] = {'B'};
    yes_no_t equil;
    trans_t  trans;
    SuperMatrix A, L, U;
    SuperMatrix B, X;
    NCformat *Astore;
    NCformat *Ustore;
    SCformat *Lstore;
    float   *a;
    int      *asub, *xa;
    int      *etree;
    int      *perm_c; /* column permutation vector */
    int      *perm_r; /* row permutations from partial pivoting */
    int      nrhs, ldx, lwork, info, m, n, nnz;
    float   *rhsb, *rhsx, *xact;
    float   *work = NULL;
    float   *R, *C;
    float   u, rpg, rcond;
    float zero = 0.0;
    float one = 1.0;
    mem_usage_t   mem_usage;
    superlu_options_t options;
    SuperLUStat_t stat;

    int restrt, iter, maxit, i;
    double resid;
    float *x, *b;

#ifdef DEBUG
    extern int num_drop_L, num_drop_U;
#endif

#if ( DEBUGlevel>=1 )
    CHECK_MALLOC("Enter main()");
#endif

    /* Defaults */
    lwork = 0;
    nrhs  = 1;
    equil = YES;
    u	  = 0.1; /* u=1.0 for complete factorization */
    trans = NOTRANS;

    /* Set the default input options:
	options.Fact = DOFACT;
	options.Equil = YES;
	options.ColPerm = COLAMD;
	options.DiagPivotThresh = 0.1; //different from complete LU
	options.Trans = NOTRANS;
	options.IterRefine = NOREFINE;
	options.SymmetricMode = NO;
	options.PivotGrowth = NO;
	options.ConditionNumber = NO;
	options.PrintStat = YES;
	options.RowPerm = LargeDiag;
	options.ILU_DropTol = 1e-4;
	options.ILU_FillTol = 1e-2;
	options.ILU_FillFactor = 10.0;
	options.ILU_DropRule = DROP_BASIC | DROP_AREA;
	options.ILU_Norm = INF_NORM;
	options.ILU_MILU = SMILU_2;
     */
    ilu_set_default_options(&options);

    /* Modify the defaults. */
    options.PivotGrowth = YES;	  /* Compute reciprocal pivot growth */
    options.ConditionNumber = YES;/* Compute reciprocal condition number */

    if ( lwork > 0 ) {
	work = SUPERLU_MALLOC(lwork);
	if ( !work ) ABORT("Malloc fails for work[].");
    }

    /* Read matrix A from a file in Harwell-Boeing format.*/
    if (argc < 2)
    {
	printf("Usage:\n%s [OPTION] < [INPUT] > [OUTPUT]\nOPTION:\n"
		"-h -hb:\n\t[INPUT] is a Harwell-Boeing format matrix.\n"
		"-r -rb:\n\t[INPUT] is a Rutherford-Boeing format matrix.\n"
		"-t -triplet:\n\t[INPUT] is a triplet format matrix.\n",
		argv[0]);
	return 0;
    }
    else
    {
	switch (argv[1][1])
	{
	    case 'H':
	    case 'h':
		printf("Input a Harwell-Boeing format matrix:\n");
		sreadhb(&m, &n, &nnz, &a, &asub, &xa);
		break;
	    case 'R':
	    case 'r':
		printf("Input a Rutherford-Boeing format matrix:\n");
		sreadrb(&m, &n, &nnz, &a, &asub, &xa);
		break;
	    case 'T':
	    case 't':
		printf("Input a triplet format matrix:\n");
		sreadtriple(&m, &n, &nnz, &a, &asub, &xa);
		break;
	    default:
		printf("Unrecognized format.\n");
		return 0;
	}
    }

    sCreate_CompCol_Matrix(&A, m, n, nnz, a, asub, xa, SLU_NC, SLU_S, SLU_GE);
    Astore = A.Store;
    sfill_diag(n, Astore);
    printf("Dimension %dx%d; # nonzeros %d\n", A.nrow, A.ncol, Astore->nnz);
    fflush(stdout);

    if ( !(rhsb = floatMalloc(m * nrhs)) ) ABORT("Malloc fails for rhsb[].");
    if ( !(rhsx = floatMalloc(m * nrhs)) ) ABORT("Malloc fails for rhsx[].");
    sCreate_Dense_Matrix(&B, m, nrhs, rhsb, m, SLU_DN, SLU_S, SLU_GE);
    sCreate_Dense_Matrix(&X, m, nrhs, rhsx, m, SLU_DN, SLU_S, SLU_GE);
    xact = floatMalloc(n * nrhs);
    ldx = n;
    sGenXtrue(n, nrhs, xact, ldx);
    sFillRHS(trans, nrhs, xact, ldx, &A, &B);

    if ( !(etree = intMalloc(n)) ) ABORT("Malloc fails for etree[].");
    if ( !(perm_r = intMalloc(m)) ) ABORT("Malloc fails for perm_r[].");
    if ( !(perm_c = intMalloc(n)) ) ABORT("Malloc fails for perm_c[].");
    if ( !(R = (float *) SUPERLU_MALLOC(A.nrow * sizeof(float))) )
	ABORT("SUPERLU_MALLOC fails for R[].");
    if ( !(C = (float *) SUPERLU_MALLOC(A.ncol * sizeof(float))) )
	ABORT("SUPERLU_MALLOC fails for C[].");

    info = 0;
#ifdef DEBUG
    num_drop_L = 0;
    num_drop_U = 0;
#endif

    /* Initialize the statistics variables. */
    StatInit(&stat);

    /* Compute the incomplete factorization and compute the condition number
       and pivot growth using dgsisx. */
    sgsisx(&options, &A, perm_c, perm_r, etree, equed, R, C, &L, &U, work,
	   lwork, &B, &X, &rpg, &rcond, &mem_usage, &stat, &info);

    Lstore = (SCformat *) L.Store;
    Ustore = (NCformat *) U.Store;
    printf("sgsisx(): info %d\n", info);
    if (info > 0 || rcond < 1e-8 || rpg > 1e8)
	printf("WARNING: This preconditioner might be unstable.\n");

    if ( info == 0 || info == n+1 ) {

	if ( options.PivotGrowth == YES )
	    printf("Recip. pivot growth = %e\n", rpg);
	if ( options.ConditionNumber == YES )
	    printf("Recip. condition number = %e\n", rcond);

    } else if ( info > 0 && lwork == -1 ) {
	printf("** Estimated memory: %d bytes\n", info - n);
    }
    printf("n(A) = %d, nnz(A) = %d\n", n, Astore->nnz);
    printf("No of nonzeros in factor L = %d\n", Lstore->nnz);
    printf("No of nonzeros in factor U = %d\n", Ustore->nnz);
    printf("No of nonzeros in L+U = %d\n", Lstore->nnz + Ustore->nnz - n);
    printf("Fill ratio: nnz(F)/nnz(A) = %.3f\n",
	    ((double)(Lstore->nnz) + (double)(Ustore->nnz) - (double)n)
	    / (double)Astore->nnz);
    printf("L\\U MB %.3f\ttotal MB needed %.3f\n",
	   mem_usage.for_lu/1e6, mem_usage.total_needed/1e6);
    fflush(stdout);

    /* Set the global variables. */
    GLOBAL_A = &A;
    GLOBAL_L = &L;
    GLOBAL_U = &U;
    GLOBAL_STAT = &stat;
    GLOBAL_PERM_C = perm_c;
    GLOBAL_PERM_R = perm_r;

    /* Set the variables used by GMRES. */
    restrt = SUPERLU_MIN(n / 3 + 1, 50);
    maxit = 1000;
    iter = maxit;
    resid = 1e-8;
    if (!(b = floatMalloc(m))) ABORT("Malloc fails for b[].");
    if (!(x = floatMalloc(n))) ABORT("Malloc fails for x[].");
    sp_sgemv("N", one, &A, xact, 1, zero, b, 1);

    if (info <= n + 1)
    {
	int i_1 = 1;
	double maxferr = 0.0, nrmA, nrmB, res, t;
        float temp;
	extern float snrm2_(int *, float [], int *);
	extern void saxpy_(int *, float *, float [], int *, float [], int *);

	/* Call GMRES. */
	/*double *sol = (double*) ((DNformat*) X.Store)->nzval;
	for (i = 0; i < n; i++) x[i] = sol[i];*/
	for (i = 0; i < n; i++) x[i] = zero;

	t = SuperLU_timer_();

	sfgmr(n, smatvec_mult, spsolve, b, x, resid, restrt, &iter, stdout);

	t = SuperLU_timer_() - t;

	/* Output the result. */
	nrmA = snrm2_(&(Astore->nnz), (float *)((DNformat *)A.Store)->nzval,
		&i_1);
	nrmB = snrm2_(&m, b, &i_1);
	sp_sgemv("N", -1.0, &A, x, 1, 1.0, b, 1);
	res = snrm2_(&m, b, &i_1);
	resid = res / nrmB;
	printf("||A||_F = %.1e, ||B||_2 = %.1e, ||B-A*X||_2 = %.1e, "
		"relres = %.1e\n", nrmA, nrmB, res, resid);

	if (iter >= maxit)
	{
	    if (resid >= 1.0) iter = -180;
	    else if (resid > 1e-8) iter = -111;
	}
	printf("iteration: %d\nresidual: %.1e\nGMRES time: %.2f seconds.\n",
		iter, resid, t);

	for (i = 0; i < m; i++)
	    maxferr = SUPERLU_MAX(maxferr, fabs(x[i] - xact[i]));
	printf("||X-X_true||_oo = %.1e\n", maxferr);
    }
#ifdef DEBUG
    printf("%d entries in L and %d entries in U dropped.\n",
	    num_drop_L, num_drop_U);
#endif
    fflush(stdout);

    if ( options.PrintStat ) StatPrint(&stat);
    StatFree(&stat);

    SUPERLU_FREE (rhsb);
    SUPERLU_FREE (rhsx);
    SUPERLU_FREE (xact);
    SUPERLU_FREE (etree);
    SUPERLU_FREE (perm_r);
    SUPERLU_FREE (perm_c);
    SUPERLU_FREE (R);
    SUPERLU_FREE (C);
    Destroy_CompCol_Matrix(&A);
    Destroy_SuperMatrix_Store(&B);
    Destroy_SuperMatrix_Store(&X);
    if ( lwork >= 0 ) {
	Destroy_SuperNode_Matrix(&L);
	Destroy_CompCol_Matrix(&U);
    }
    SUPERLU_FREE(b);
    SUPERLU_FREE(x);

#if ( DEBUGlevel>=1 )
    CHECK_MALLOC("Exit main()");
#endif

    return 0;
}
