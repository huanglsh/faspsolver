/*! \file KryPgcr.c
 *
 *  \brief Krylov subspace methods -- Preconditioned GCR
 *
 *  \note This file contains Level-3 (Kry) functions. It requires
 *        AuxArray.c, AuxMemory.c, AuxMessage.c, BlaArray.c, BlaSpmvCSR.c, 
 *        and BlaVector.c
 */

#include <math.h>

#include "fasp.h"
#include "fasp_functs.h"

/*---------------------------------*/
/*--  Declare Private Functions  --*/
/*---------------------------------*/

#include "KryUtil.inl"

static void dense_aAtxpby (INT, INT, REAL *, REAL, REAL *, REAL, REAL *);

/**
 * \fn INT fasp_solver_dcsr_pgcr (dCSRmat *A,  dvector *b, dvector *x,
 *                                precond *pc, const REAL tol, const INT MaxIt,
 *                                const SHORT restart, const SHORT stop_type,
 *                                const SHORT prtlvl)
 *
 * \brief A preconditioned GCR method for solving Au=b
 *
 * \param A         Pointer to coefficient matrix
 * \param b         Pointer to dvector of right hand side
 * \param x         Pointer to dvector of dofs
 * \param pc        Pointer to structure of precondition (precond)
 * \param tol       Tolerance for stopage
 * \param MaxIt     Maximal number of iterations
 * \param restart   Restart number for GCR
 * \param stop_type Stopping type
 * \param prtlvl    How much information to print out
 *
 * \return          Iteration number if converges; ERROR otherwise.
 * \note            Refer to YVAN NOTAY "AN AGGREGATION-BASED ALGEBRAIC MULTIGRID METHOD"
 * \author Zheng Li
 * \date   12/23/2014
 */
INT fasp_solver_dcsr_pgcr (dCSRmat     *A,
                           dvector     *b,
                           dvector     *x,
                           precond     *pc,
                           const REAL   tol,
                           const INT    MaxIt,
                           const SHORT  restart,
                           const SHORT  stop_type,
                           const SHORT  prtlvl)
{
    const INT   n = b->row;
    
    // local variables
    INT      iter = 0, rst = -1;
    INT      i, j, k;
    
    REAL     gamma, alpha, beta, checktol;
    REAL     absres0 = BIGREAL, absres = BIGREAL;
    REAL     relres  = BIGREAL;
    
    // allocate temp memory (need about (restart+4)*n REAL numbers)
    REAL    *c = NULL, *z = NULL, *alp = NULL, *tmpx = NULL;
    REAL    *norms = NULL, *r = NULL, *work = NULL;
    REAL    **h = NULL;
    
    INT      Restart = MIN(restart, MaxIt);
    LONG     worksize = n+2*Restart*n+Restart+Restart;
    
#if DEBUG_MODE > 0
    printf("### DEBUG: %s ...... [Start]\n", __FUNCTION__);
    printf("### DEBUG: maxit = %d, tol = %.4le\n", MaxIt, tol);
#endif
    
    work = (REAL *) fasp_mem_calloc(worksize, sizeof(REAL));
    
    /* check whether memory is enough for GMRES */
    while ( (work == NULL) && (Restart > 5) ) {
        Restart = Restart - 5;
        worksize = n+2*Restart*n+Restart+Restart;
        work = (REAL *) fasp_mem_calloc(worksize, sizeof(REAL));
    }
    
    if ( work == NULL ) {
        printf("### ERROR: No enough memory for GMRES %s : %s : %d!\n",
               __FILE__, __FUNCTION__, __LINE__ );
        exit(ERROR_ALLOC_MEM);
    }
    
    if ( prtlvl > PRINT_MIN && Restart < restart ) {
        printf("### WARNING: GMRES restart number set to %d!\n", Restart);
    }
    
    r = work, z = r+n, c = z + Restart*n, alp = c + Restart*n, tmpx = alp + Restart;
    
    h = (REAL **)fasp_mem_calloc(Restart, sizeof(REAL *));
    for (i = 0; i < Restart; i++) h[i] = (REAL*)fasp_mem_calloc(Restart, sizeof(REAL));
    
    norms = (REAL *) fasp_mem_calloc(MaxIt+1, sizeof(REAL));
    
    // r = b-A*x
    fasp_array_cp(n, b->val, r);
    fasp_blas_dcsr_aAxpy(-1.0, A, x->val, r);
    
    absres = fasp_blas_array_dotprod(n, r, r);
    
    absres0 = MAX(SMALLREAL,absres);
    
    relres  = absres/absres0;
    
    // output iteration information if needed
    print_itinfo(prtlvl,stop_type,0,relres,sqrt(absres0),0.0);
    
    // store initial residual
    norms[0] = relres;
    
    checktol = MAX(tol*tol*absres0, absres*1.0e-4);
    
    while ( iter < MaxIt && sqrt(relres) > tol ) {
        
        i = -1; rst ++;
        
        while ( i < Restart-1 && iter < MaxIt ) {
            
            i++; iter++;
            
            // z = B^-1r
            if ( pc == NULL )
                fasp_array_cp(n, r, &z[i*n]);
            else
                pc->fct(r, &z[i*n], pc->data);
            
            // c = Az
            fasp_blas_dcsr_mxv(A, &z[i*n], &c[i*n]);
            
            /* Modified Gram_Schmidt orthogonalization */
            for ( j = 0; j < i; j++ ) {
                gamma = fasp_blas_array_dotprod(n, &c[j*n], &c[i*n]);
                h[i][j] = gamma/h[j][j];
                fasp_blas_array_axpy(n, -h[i][j], &c[j*n], &c[i*n]);
            }
            // gamma = (c,c)
            gamma = fasp_blas_array_dotprod(n, &c[i*n], &c[i*n]);
            
            h[i][i] = gamma;
            
            // alpha = (c, r)
            alpha = fasp_blas_array_dotprod(n, &c[i*n], r);
            
            beta = alpha/gamma;
            
            alp[i] = beta;
            
            // r = r - beta*c
            fasp_blas_array_axpy(n, -beta, &c[i*n], r);
            
            // equivalent to ||r||_2
            absres = absres - alpha*alpha/gamma;
            
            if (absres < checktol) {
                absres = fasp_blas_array_dotprod(n, r, r);
                checktol = MAX(tol*tol*absres0, absres*1.0e-4);
            }
            
            relres = absres / absres0;
            
            norms[iter] = relres;
            
            print_itinfo(prtlvl, stop_type, iter, sqrt(relres), sqrt(absres),
                         sqrt(norms[iter]/norms[iter-1]));
            
            if (sqrt(relres) < tol)  break;
        }
        
        for ( k = i; k >=0; k-- ) {
            tmpx[k] = alp[k];
            for (j=0; j<k; ++j) {
                alp[j] -= h[k][j]*tmpx[k];
            }
        }
        
        if (rst==0) dense_aAtxpby(n, i+1, z, 1.0, tmpx, 0.0, x->val);
        else dense_aAtxpby(n, i+1, z, 1.0, tmpx, 1.0, x->val);
        
    }
    
    if ( prtlvl > PRINT_NONE ) ITS_FINAL(iter,MaxIt,sqrt(relres));
    
    // clean up memory
    for (i = 0; i < Restart; i++) fasp_mem_free(h[i]);
    fasp_mem_free(h);
    
    fasp_mem_free(work);
    fasp_mem_free(norms);
    
#if DEBUG_MODE > 0
    printf("### DEBUG: %s ...... [Finish]\n", __FUNCTION__);
#endif
    
    if ( iter >= MaxIt )
        return ERROR_SOLVER_MAXIT;
    else
        return iter;
}

/*---------------------------------*/
/*--    Private Functions        --*/
/*---------------------------------*/

/**
 * \fn static void dense_aAtxpby (INT n, INT m, REAL *A, REAL alpha,
 *                                REAL *x, REAL beta, REAL *y)
 *
 * \brief  y = alpha*A^T*x + beta*y
 *
 * \param n     Pointer to row
 * \param m     Pointer to col
 * \param A     Pointer to CSR matrix
 * \param alpha Real factor alpha
 * \param x     Pointer to dvector of right hand side
 * \param beta  Real factor beta
 * \param y     Maximal number of iterations
 *
 * \author zheng Li, Chensong Zhang
 * \date   12/23/2014
 *
 * \warning This is a special function. Move it to blas_smat.c if needed else where. 
 */
static void dense_aAtxpby (INT   n,
                           INT   m,
                           REAL *A,
                           REAL  alpha,
                           REAL *x,
                           REAL  beta,
                           REAL *y)
{
    INT i, j;
    
    for (i=0; i<m; i++) fasp_blas_array_ax(n, x[i], &A[i*n]);
    
    for (j=1; j<m; j++) {
        for (i=0; i<n; i++) {
            A[i] += A[i+j*n];
        }
    }
    
    fasp_blas_array_axpby(n, alpha, A, beta, y);
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/