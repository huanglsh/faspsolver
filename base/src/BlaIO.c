/*! \file BlaIO.c
 *
 *  \brief Matrix/vector input/output subroutines
 *
 *  \note Read, write or print a matrix or a vector in various formats
 *
 *  \note This file contains Level-1 (Bla) functions. It requires
 *        AuxArray.c, AuxConvert.c, AuxMemory.c, AuxMessage.c, AuxVector.c,
 *        and BlaFormat.c
 */

#include "fasp.h"
#include "fasp_functs.h"
#include "hb_io.h"

// Flags which indicates lengths of INT and REAL numbers
INT   ilength; /**< Length of INT in byte */
INT   dlength; /**< Length of REAL in byte */

/*---------------------------------*/
/*--  Declare Private Functions  --*/
/*---------------------------------*/

// Decoration of private functions
static void fasp_dcsr_read_s(FILE *fp, dCSRmat *A);
static void fasp_dcsr_read_b(FILE *fp, dCSRmat *A, const SHORT EndianFlag);
static void fasp_dcoo_read_s(FILE *fp, dCSRmat *A);
static void fasp_dcoo_read_b(FILE *fp, dCSRmat *A, const SHORT EndianFlag);
static void fasp_dbsr_read_s(FILE *fp, dBSRmat *A);
static void fasp_dbsr_read_b(FILE *fp, dBSRmat *A, const SHORT EndianFlag);
static void fasp_dstr_read_s(FILE *fp, dSTRmat *A);
static void fasp_dstr_read_b(FILE *fp, dSTRmat *A, const SHORT EndianFlag);
static void fasp_dmtx_read_s(FILE *fp, dCSRmat *A);
static void fasp_dmtx_read_b(FILE *fp, dCSRmat *A, const SHORT EndianFlag);
static void fasp_dmtxsym_read_s(FILE *fp, dCSRmat *A);
static void fasp_dmtxsym_read_b(FILE *fp, dCSRmat *A, const SHORT EndianFlag);
static void fasp_dcsr_write_s(FILE *fp, dCSRmat *A);
static void fasp_dcsr_write_b(FILE *fp, dCSRmat *A);
static void fasp_dbsr_write_s(FILE *fp, dBSRmat *A);
static void fasp_dbsr_write_b(FILE *fp, dBSRmat *A);
static void fasp_dstr_write_s(FILE *fp, dSTRmat *A);
static void fasp_dstr_write_b(FILE *fp, dSTRmat *A);
static void fasp_dvec_read_s(FILE *fp, dvector *b);
static void fasp_dvec_read_b(FILE *fp, dvector *b, const SHORT EndianFlag);
static void fasp_ivec_read_s(FILE *fp, ivector *b);
static void fasp_ivec_read_b(FILE *fp, ivector *b, const SHORT EndianFlag);
static void fasp_dvecind_read_s(FILE *fp, dvector *b);
static void fasp_dvecind_read_b(FILE *fp, dvector *b, const SHORT EndianFlag);
static void fasp_ivecind_read_s(FILE *fp, ivector *b);
static void fasp_ivecind_read_b(FILE *fp, ivector *b, const SHORT EndianFlag);
static void fasp_dvec_write_s(FILE *fp, dvector *b);
static void fasp_dvec_write_b(FILE *fp, dvector *b);
static void fasp_ivec_write_s(FILE *fp, ivector *b);
static void fasp_ivec_write_b(FILE *fp, ivector *b);
static void fasp_dvecind_write_s(FILE *fp, dvector *b);
static void fasp_dvecind_write_b(FILE *fp, dvector *b);
static void fasp_ivecind_write_s(FILE *fp, ivector *b);
static void fasp_ivecind_write_b(FILE *fp, ivector *A);
static INT  endian_convert_int (const INT,  const INT, const INT);
static REAL endian_convert_real(const REAL, const INT, const INT);

/*---------------------------------*/
/*--      Public Functions       --*/
/*---------------------------------*/

/**
 * \fn void fasp_dcsrvec1_read (const char *filename, dCSRmat *A, dvector *b)
 *
 * \brief Read A and b from a SINGLE disk file
 *
 * \param filename  File name
 * \param A         Pointer to the CSR matrix
 * \param b         Pointer to the dvector
 *
 * \note
 *      This routine reads a dCSRmat matrix and a dvector vector from a single disk file.
 *
 * \note
 *
 *      The difference between this and fasp_dcoovec_read is that this
 *      routine support non-square matrices.
 *
 * \note File format:
 *   - nrow ncol         % number of rows and number of columns
 *   - ia(j), j=0:nrow   % row index
 *   - ja(j), j=0:nnz-1  % column index
 *   - a(j), j=0:nnz-1   % entry value
 *   - n                 % number of entries
 *   - b(j), j=0:n-1     % entry value
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 *
 * Modified by Chensong Zhang on 03/14/2012
 */
void fasp_dcsrvec1_read (const char  *filename,
                         dCSRmat     *A,
                         dvector     *b)
{
    int  i,m,n,idata;
    REAL ddata;
    
    // Open input disk file
    FILE *fp = fopen(filename, "r");
    
    if (fasp_mem_check((void *)fp,NULL,ERROR_OPEN_FILE) < 0) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    // Read CSR matrix
    fscanf(fp, "%d %d", &m, &n);
    A->row=m; A->col=n;
    
    A->IA=(INT *)fasp_mem_calloc(m+1, sizeof(INT));
    for ( i = 0; i <= m; ++i ) {
        fscanf(fp, "%d", &idata);
        A->IA[i] = idata;
    }
    
    INT nnz = A->IA[m]-A->IA[0];
    
    A->nnz = nnz;
    A->JA  = (INT *)fasp_mem_calloc(nnz, sizeof(INT));
    A->val = (REAL*)fasp_mem_calloc(nnz, sizeof(REAL));
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%d", &idata);
        A->JA[i] = idata;
    }
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%lf", &ddata);
        A->val[i] = ddata;
    }
    
    // Read RHS vector
    fscanf(fp, "%d", &m);
    b->row = m;
    
    b->val = (REAL*)fasp_mem_calloc(m, sizeof(REAL));
    
    for ( i = 0; i < m; ++i ) {
        fscanf(fp, "%lf", &ddata);
        b->val[i] = ddata;
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dcsrvec2_read (const char *filemat, const char *filerhs,
 *                              dCSRmat *A, dvector *b)
 *
 * \brief Read A and b from two disk files
 *
 * \param filemat  File name for matrix
 * \param filerhs  File name for right-hand side
 * \param A        Pointer to the dCSR matrix
 * \param b        Pointer to the dvector
 *
 * \note
 *
 *      This routine reads a dCSRmat matrix and a dvector vector from a disk file.
 *
 * \note
 * CSR matrix file format:
 *   - nrow              % number of columns (rows)
 *   - ia(j), j=0:nrow   % row index
 *   - ja(j), j=0:nnz-1  % column index
 *   - a(j), j=0:nnz-1   % entry value
 *
 * \note
 * RHS file format:
 *   - n                 % number of entries
 *   - b(j), j=0:nrow-1  % entry value
 *
 * \note Indices start from 1, NOT 0!!!
 *
 * \author Zhiyang Zhou
 * \date   2010/08/06
 *
 * Modified by Chensong Zhang on 2011/03/01
 * Modified by Chensong Zhang on 2012/01/05
 */
void fasp_dcsrvec2_read (const char  *filemat,
                         const char  *filerhs,
                         dCSRmat     *A,
                         dvector     *b)
{
    int i, n, tempi;
    
    /* read the matrix from file */
    FILE *fp = fopen(filemat,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n",filemat);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filemat);
    
    fscanf(fp,"%d\n",&n);
    A->row = n;
    A->col = n;
    A->IA  = (INT *)fasp_mem_calloc(n+1, sizeof(INT));
    
    for ( i = 0; i <= n; ++i ) {
        fscanf(fp,"%d\n",&tempi);
        A->IA[i] = tempi-1;
    }
    
    INT nz = A->IA[n];
    A->nnz = nz;
    A->JA  = (INT *) fasp_mem_calloc(nz, sizeof(INT));
    A->val = (REAL *)fasp_mem_calloc(nz, sizeof(REAL));
    
    for ( i = 0; i < nz; ++i ) {
        fscanf(fp,"%d\n",&tempi);
        A->JA[i] = tempi-1;
    }
    
    for ( i = 0; i < nz; ++i ) fscanf(fp,"%le\n",&(A->val[i]));
    
    fclose(fp);
    
    /* Read the rhs from file */
    b->row = n;
    b->val = (REAL *)fasp_mem_calloc(n, sizeof(REAL));
    
    fp = fopen(filerhs,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n",filerhs);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filerhs);
    
    fscanf(fp,"%d\n",&n);
    
    if ( n != b->row ) {
        printf("### ERROR: rhs size %d does not match matrix size %d!\n",n,b->row);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp,"%le\n", &(b->val[i]));
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dcsr_read (const char *filename, dCSRmat *A)
 *
 * \brief Read A from matrix disk file in IJ format
 *
 * \param *filename  char for matrix file name
 * \param *A         pointer to the CSR matrix
 *
 * \author Ziteng Wang
 * \date   12/25/2012
 */
void fasp_dcsr_read (const char  *filename,
                     dCSRmat     *A)
{
    int  i,m,idata;
    REAL ddata;
    
    // Open input disk file
    FILE *fp = fopen(filename, "r");
    
    if (fasp_mem_check((void *)fp,NULL,ERROR_OPEN_FILE) < 0) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    // Read CSR matrix
    fscanf(fp, "%d", &m);
    A->row = A->col = m;
    
    A->IA = (INT *)fasp_mem_calloc(m+1, sizeof(INT));
    for ( i = 0; i <= m; ++i ) {
        fscanf(fp, "%d", &idata);
        A->IA[i] = idata;
    }
    
    INT nnz = A->IA[m]-A->IA[0];
    
    A->nnz = nnz;
    A->JA  = (INT *)fasp_mem_calloc(nnz, sizeof(INT));
    A->val = (REAL*)fasp_mem_calloc(nnz, sizeof(REAL));
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%d", &idata);
        A->JA[i] = idata;
    }
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%lf", &ddata);
        A->val[i]= ddata;
    }
    fclose(fp);
}

/**
 * \fn void fasp_dcoo_read (const char *filename, dCSRmat *A)
 *
 * \brief Read A from matrix disk file in IJ format -- indices starting from 0
 *
 * \param filename  File name for matrix
 * \param A         Pointer to the CSR matrix
 *
 * \note File format:
 *   - nrow ncol nnz     % number of rows, number of columns, and nnz
 *   - i  j  a_ij        % i, j a_ij in each line
 *
 * \note After reading, it converts the matrix to dCSRmat format.
 *
 * \author Xuehai Huang, Chensong Zhang
 * \date   03/29/2009
 */
void fasp_dcoo_read (const char  *filename,
                     dCSRmat     *A)
{
    int  i,j,k,m,n,nnz;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    for ( k = 0; k < nnz; k++ ) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            Atmp.rowind[k]=i; Atmp.colind[k]=j; Atmp.val[k] = value;
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dcoo_read");
        }
    }
    
    fclose(fp);
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

/**
 * \fn void fasp_dcoo1_read (const char *filename, dCOOmat *A)
 *
 * \brief Read A from matrix disk file in IJ format -- indices starting from 1
 *
 * \param filename  File name for matrix
 * \param A         Pointer to the COO matrix
 *
 * \note File format:
 *   - nrow ncol nnz     % number of rows, number of columns, and nnz
 *   - i  j  a_ij        % i, j a_ij in each line
 *
 * \note difference between fasp_dcoo_read and this funciton is this function do not change to CSR format
 *
 * \author Xiaozhe Hu
 * \date   03/24/2013
 */
void fasp_dcoo1_read (const char  *filename,
                      dCOOmat     *A)
{
    int  i,j,k,m,n,nnz;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    fasp_dcoo_alloc(m, n, nnz, A);
    
    for ( k = 0; k < nnz; k++ ) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            A->rowind[k]=i-1; A->colind[k]=j-1; A->val[k] = value;
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dcoo1_read");
        }
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dcoo_shift_read (const char *filename, dCSRmat *A)
 *
 * \brief Read A from matrix disk file in IJ format -- indices starting from 0
 *
 * \param filename  File name for matrix
 * \param A         Pointer to the CSR matrix
 *
 * \note File format:
 *   - nrow ncol nnz     % number of rows, number of columns, and nnz
 *   - i  j  a_ij        % i, j a_ij in each line
 *
 * \note i and j suppose to start with index 1!!!
 *
 * \note After read in, it shifts the index to C fashin and converts the matrix to dCSRmat format.
 *
 * \author Xiaozhe Hu
 * \date   04/01/2014
 */
void fasp_dcoo_shift_read (const char  *filename,
                           dCSRmat     *A)
{
    int  i,j,k,m,n,nnz;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    for ( k = 0; k < nnz; k++ ) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            Atmp.rowind[k]=i-1; Atmp.colind[k]=j-1; Atmp.val[k] = value;
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, __FUNCTION__);
        }
    }
    
    fclose(fp);
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

/**
 * \fn void fasp_dmtx_read (const char *filename, dCSRmat *A)
 *
 * \brief Read A from matrix disk file in MatrixMarket general format
 *
 * \param filename  File name for matrix
 * \param A         Pointer to the CSR matrix
 *
 * \note File format:
 *   This routine reads a MatrixMarket general matrix from a mtx file.
 *   And it converts the matrix to dCSRmat format. For details of mtx format,
 *   please refer to http://math.nist.gov/MatrixMarket/.
 *
 * \note Indices start from 1, NOT 0!!!
 *
 * \author Chensong Zhang
 * \date   09/05/2011
 */
void fasp_dmtx_read (const char  *filename,
                     dCSRmat     *A)
{
    int  i,j,m,n,nnz;
    INT  innz; // index of nonzeros
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    innz = 0;
    
    while (innz < nnz) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            
            Atmp.rowind[innz]=i-1;
            Atmp.colind[innz]=j-1;
            Atmp.val[innz] = value;
            innz = innz + 1;
            
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dmtx_read");
        }
    }
    
    fclose(fp);
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

/**
 * \fn void fasp_dmtxsym_read (const char *filename, dCSRmat *A)
 *
 * \brief Read A from matrix disk file in MatrixMarket sym format
 *
 * \param filename  File name for matrix
 * \param A         Pointer to the CSR matrix
 *
 * \note File format:
 *   This routine reads a MatrixMarket symmetric matrix from a mtx file.
 *   And it converts the matrix to dCSRmat format. For details of mtx format,
 *   please refer to http://math.nist.gov/MatrixMarket/.
 *
 * \note
 *
 *      Indices start from 1, NOT 0!!!
 *
 * \author Chensong Zhang
 * \date   09/02/2011
 */
void fasp_dmtxsym_read (const char  *filename,
                        dCSRmat     *A)
{
    int  i,j,m,n,nnz;
    int  innz; // index of nonzeros
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    nnz = 2*(nnz-m) + m; // adjust for sym problem
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    innz = 0;
    
    while (innz < nnz) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            
            if ( i == j ) {
                Atmp.rowind[innz] = i-1;
                Atmp.colind[innz] = j-1;
                Atmp.val[innz] = value;
                innz = innz + 1;
            }
            else {
                Atmp.rowind[innz] = i-1; Atmp.rowind[innz+1] = j-1;
                Atmp.colind[innz] = j-1; Atmp.colind[innz+1] = i-1;
                Atmp.val[innz] = value;
                Atmp.val[innz+1] = value;
                innz = innz + 2;
            }
            
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dmtxsym_read");
        }
    }
    
    fclose(fp);
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

/**
 * \fn void fasp_dstr_read (const char *filename, dSTRmat *A)
 *
 * \brief Read A from a disk file in dSTRmat format
 *
 * \param filename  File name for the matrix
 * \param A         Pointer to the dSTRmat
 *
 * \note
 *      This routine reads a dSTRmat matrix from a disk file. After done, it converts
 *      the matrix to dCSRmat format.
 *
 * \note File format:
 *   - nx, ny, nz
 *   - nc: number of components
 *   - nband: number of bands
 *   - n: size of diagonal, you must have diagonal
 *   - diag(j), j=0:n-1
 *   - offset, length: offset and length of off-diag1
 *   - offdiag(j), j=0:length-1
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_dstr_read (const char  *filename,
                     dSTRmat     *A)
{
    int  nx, ny, nz, nxy, ngrid, nband, nc, offset;
    int  i, k, n;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d %d %d",&nx,&ny,&nz); // read dimension of the problem
    A->nx = nx; A->ny = ny; A->nz = nz;
    
    nxy = nx*ny; ngrid = nxy*nz;
    A->nxy = nxy; A->ngrid = ngrid;
    
    fscanf(fp,"%d",&nc); // read number of components
    A->nc = nc;
    
    fscanf(fp,"%d",&nband); // read number of bands
    A->nband = nband;
    
    A->offsets = (INT *)fasp_mem_calloc(nband, sizeof(INT));
    
    // read diagonal
    fscanf(fp, "%d", &n);
    A->diag = (REAL *)fasp_mem_calloc(n, sizeof(REAL));
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%le", &value);
        A->diag[i] = value;
    }
    
    // read offdiags
    k = nband;
    A->offdiag = (REAL **)fasp_mem_calloc(nband, sizeof(REAL *));
    while ( k-- ) {
        fscanf(fp,"%d %d",&offset,&n); // read number band k
        A->offsets[nband-k-1] = offset;
        
        A->offdiag[nband-k-1] = (REAL *)fasp_mem_calloc(n, sizeof(REAL));
        for ( i = 0; i < n; ++i ) {
            fscanf(fp, "%le", &value);
            A->offdiag[nband-k-1][i] = value;
        }
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dbsr_read (const char *filename, dBSRmat *A)
 *
 * \brief Read A from a disk file in dBSRmat format
 *
 * \param filename   File name for matrix A
 * \param A          Pointer to the dBSRmat A
 *
 * \note
 *   This routine reads a dBSRmat matrix from a disk file in the following format:
 *
 * \note File format:
 *   - ROW, COL, NNZ
 *   - nb: size of each block
 *   - storage_manner: storage manner of each block
 *   - ROW+1: length of IA
 *   - IA(i), i=0:ROW
 *   - NNZ: length of JA
 *   - JA(i), i=0:NNZ-1
 *   - NNZ*nb*nb: length of val
 *   - val(i), i=0:NNZ*nb*nb-1
 *
 * \author Xiaozhe Hu
 * \date   10/29/2010
 */
void fasp_dbsr_read (const char  *filename,
                     dBSRmat     *A)
{
    int  ROW, COL, NNZ, nb, storage_manner;
    int  i, n;
    int  index;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp, "%d %d %d", &ROW,&COL,&NNZ); // read dimension of the problem
    A->ROW = ROW; A->COL = COL; A->NNZ = NNZ;
    
    fscanf(fp, "%d", &nb); // read the size of each block
    A->nb = nb;
    
    fscanf(fp, "%d", &storage_manner); // read the storage_manner of each block
    A->storage_manner = storage_manner;
    
    // allocate memory space
    fasp_dbsr_alloc(ROW, COL, NNZ, nb, storage_manner, A);
    
    // read IA
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d", &index);
        A->IA[i] = index;
    }
    
    // read JA
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ){
        fscanf(fp, "%d", &index);
        A->JA[i] = index;
    }
    
    // read val
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%le", &value);
        A->val[i] = value;
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dvecind_read (const char *filename, dvector *b)
 *
 * \brief Read b from matrix disk file
 *
 * \param filename  File name for vector b
 * \param b         Pointer to the dvector b (output)
 *
 * \note File Format:
 *     - nrow
 *     - ind_j, val_j, j=0:nrow-1
 *
 * \note Because the index is given, order is not important!
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_dvecind_read (const char  *filename,
                        dvector     *b)
{
    INT  i, n;
    INT  index;
    REAL value;
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d",&n);
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        
        fscanf(fp, "%d %le", &index, &value);
        
        if ( value > BIGREAL || index >= n ) {
            printf("### ERROR: Wrong index = %d or value = %lf\n", index, value);
            fasp_dvec_free(b);
            fclose(fp);
            exit(ERROR_INPUT_PAR);
        }
        
        b->val[index] = value;
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dvec_read (const char *filename, dvector *b)
 *
 * \brief Read b from a disk file in array format
 *
 * \param filename  File name for vector b
 * \param b         Pointer to the dvector b (output)
 *
 * \note File Format:
 *   - nrow
 *   - val_j, j=0:nrow-1
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_dvec_read (const char  *filename,
                     dvector     *b)
{
    int  i, n;
    REAL value;
    
    FILE *fp = fopen(filename,"r");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d",&n);
    
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        
        fscanf(fp, "%le", &value);
        b->val[i] = value;
        
        if ( value > BIGREAL ) {
            printf("### ERROR: Wrong value = %lf\n", value);
            fasp_dvec_free(b);
            fclose(fp);
            exit(ERROR_INPUT_PAR);
        }
        
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_ivecind_read (const char *filename, ivector *b)
 *
 * \brief Read b from matrix disk file
 *
 * \param filename  File name for vector b
 * \param b         Pointer to the dvector b (output)
 *
 * \note File Format:
 *   - nrow
 *   - ind_j, val_j ... j=0:nrow-1
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_ivecind_read (const char  *filename,
                        ivector     *b)
{
    int i, n, index, value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d",&n);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d %d", &index, &value);
        b->val[index] = value;
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_ivec_read (const char *filename, ivector *b)
 *
 * \brief Read b from a disk file in array format
 *
 * \param filename  File name for vector b
 * \param b         Pointer to the dvector b (output)
 *
 * \note File Format:
 *   - nrow
 *   - val_j, j=0:nrow-1
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_ivec_read (const char  *filename,
                     ivector     *b)
{
    int i, n, value;
    
    FILE *fp = fopen(filename,"r");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fscanf(fp,"%d",&n);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d", &value);
        b->val[i] = value;
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dcsrvec1_write (const char *filename, dCSRmat *A, dvector *b)
 *
 * \brief Write A and b to a SINGLE disk file
 *
 * \param filename  File name
 * \param A         Pointer to the CSR matrix
 * \param b         Pointer to the dvector
 *
 * \note
 *      This routine writes a dCSRmat matrix and a dvector vector to a single disk file.
 *
 * \note File format:
 *   - nrow ncol         % number of rows and number of columns
 *   - ia(j), j=0:nrow   % row index
 *   - ja(j), j=0:nnz-1  % column index
 *   - a(j), j=0:nnz-1   % entry value
 *   - n                 % number of entries
 *   - b(j), j=0:n-1     % entry value
 *
 * \author Feiteng Huang
 * \date   05/19/2012
 *
 * Modified by Chensong on 12/26/2012
 */
void fasp_dcsrvec1_write (const char  *filename,
                          dCSRmat     *A,
                          dvector     *b)
{
    INT m = A->row, n = A->col, nnz = A->nnz;
    INT i;
    
    FILE *fp = fopen(filename, "w");
    
    /* write the matrix to file */
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d %d\n",m,n);
    for ( i = 0; i < m+1; ++i ) {
        fprintf(fp, "%d\n", A->IA[i]);
    }
    for (i = 0; i < nnz; ++i) {
        fprintf(fp, "%d\n", A->JA[i]);
    }
    for (i = 0; i < nnz; ++i) {
        fprintf(fp, "%le\n", A->val[i]);
    }
    
    m = b->row;
    
    /* write the rhs to file */
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%le\n",b->val[i]);
    
    fclose(fp);
}

/**
 * \fn void fasp_dcsrvec2_write (const char *filemat, const char *filerhs,
 *                               dCSRmat *A, dvector *b)
 *
 * \brief Write A and b to two disk files
 *
 * \param filemat  File name for matrix
 * \param filerhs  File name for right-hand side
 * \param A        Pointer to the dCSR matrix
 * \param b        Pointer to the dvector
 *
 * \note
 *
 *      This routine writes a dCSRmat matrix and a dvector vector to two disk files.
 *
 * \note
 * CSR matrix file format:
 *   - nrow              % number of columns (rows)
 *   - ia(j), j=0:nrow   % row index
 *   - ja(j), j=0:nnz-1  % column index
 *   - a(j),  j=0:nnz-1  % entry value
 *
 * \note
 * RHS file format:
 *   - n                 % number of entries
 *   - b(j), j=0:nrow-1  % entry value
 *
 * \note Indices start from 1, NOT 0!!!
 *
 * \author Feiteng Huang
 * \date   05/19/2012
 *
 */
void fasp_dcsrvec2_write (const char  *filemat,
                          const char  *filerhs,
                          dCSRmat     *A,
                          dvector     *b)
{
    INT m=A->row, nnz=A->nnz;
    INT i;
    
    FILE *fp = fopen(filemat, "w");
    
    /* write the matrix to file */
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filemat);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filemat);
    
    fprintf(fp,"%d\n",m);
    for ( i = 0; i < m+1; ++i ) {
        fprintf(fp, "%d\n", A->IA[i]+1);
    }
    for (i = 0; i < nnz; ++i) {
        fprintf(fp, "%d\n", A->JA[i]+1);
    }
    for (i = 0; i < nnz; ++i) {
        fprintf(fp, "%le\n", A->val[i]);
    }
    
    fclose(fp);
    
    m = b->row;
    
    fp = fopen(filerhs,"w");
    
    /* write the rhs to file */
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filerhs);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filerhs);
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%le\n",b->val[i]);
    
    fclose(fp);
}

/**
 * \fn void fasp_dcoo_write (const char *filename, dCSRmat *A)
 *
 * \brief Write a matrix to disk file in IJ format (coordinate format)
 *
 * \param A         pointer to the dCSRmat matrix
 * \param filename  char for vector file name
 *
 * \note
 *
 *      The routine writes the specified REAL vector in COO format.
 *      Refer to the reading subroutine \ref fasp_dcoo_read.
 *
 * \note File format:
 *   - The first line of the file gives the number of rows, the
 *   number of columns, and the number of nonzeros.
 *   - Then gives nonzero values in i j a(i,j) format.
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_dcoo_write (const char  *filename,
                      dCSRmat     *A)
{
    const INT m = A->row, n = A->col;
    INT i, j;
    
    FILE *fp = fopen(filename, "w");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d  %d  %d\n",m,n,A->nnz);
    for ( i = 0; i < m; ++i ) {
        for ( j = A->IA[i]; j < A->IA[i+1]; j++ )
            fprintf(fp,"%d  %d  %0.15e\n",i,A->JA[j],A->val[j]);
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dstr_write (const char *filename, dSTRmat *A)
 *
 * \brief Write a dSTRmat to a disk file
 *
 * \param filename  File name for A
 * \param A         Pointer to the dSTRmat matrix A
 *
 * \note
 *
 *      The routine writes the specified REAL vector in STR format.
 *      Refer to the reading subroutine \ref fasp_dstr_read.
 *
 * \author Shiquan Zhang
 * \date   03/29/2010
 */
void fasp_dstr_write (const char  *filename,
                      dSTRmat     *A)
{
    const INT nx = A->nx, ny = A->ny, nz = A->nz;
    const INT ngrid = A->ngrid, nband = A->nband, nc = A->nc;
    
    INT *offsets=A->offsets;
    
    unsigned INT i, k, n;
    
    FILE *fp = fopen(filename,"w");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d  %d  %d\n",nx,ny,nz); // write dimension of the problem
    
    fprintf(fp,"%d\n",nc); // read number of components
    
    fprintf(fp,"%d\n",nband); // write number of bands
    
    // write diagonal
    n=ngrid*nc*nc; // number of nonzeros in each band
    fprintf(fp,"%d\n",n); // number of diagonal entries
    for ( i = 0; i < n; ++i ) fprintf(fp, "%le\n", A->diag[i]);
    
    // write offdiags
    k = nband;
    while ( k-- ) {
        INT offset=offsets[nband-k-1];
        n=(ngrid-ABS(offset))*nc*nc; // number of nonzeros in each band
        fprintf(fp,"%d  %d\n",offset,n); // read number band k
        for ( i = 0; i < n; ++i ) {
            fprintf(fp, "%le\n", A->offdiag[nband-k-1][i]);
        }
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dbsr_write (const char *filename, dBSRmat *A)
 *
 * \brief Write a dBSRmat to a disk file
 *
 * \param filename  File name for A
 * \param A         Pointer to the dBSRmat matrix A
 *
 * \note
 *
 *      The routine writes the specified REAL vector in BSR format.
 *      Refer to the reading subroutine \ref fasp_dbsr_read.
 *
 * \author Shiquan Zhang
 * \date   10/29/2010
 */
void fasp_dbsr_write (const char *filename,
                      dBSRmat *A)
{
    const INT ROW = A->ROW, COL = A->COL, NNZ = A->NNZ;
    const INT nb = A->nb, storage_manner = A->storage_manner;
    
    INT  *ia  = A->IA;
    INT  *ja  = A->JA;
    REAL *val = A->val;
    
    unsigned INT i, n;
    
    FILE *fp = fopen(filename,"w");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d  %d  %d\n",ROW,COL,NNZ); // write dimension of the block matrix
    
    fprintf(fp,"%d\n",nb); // write the size of each block
    
    fprintf(fp,"%d\n",storage_manner); // write storage manner of each block
    
    // write A->IA
    n = ROW+1; // length of A->IA
    fprintf(fp,"%d\n",n); // length of A->IA
    for ( i = 0; i < n; ++i ) fprintf(fp, "%d\n", ia[i]);
    
    // write A->JA
    n = NNZ; // length of A->JA
    fprintf(fp, "%d\n", n); // length of A->JA
    for ( i = 0; i < n; ++i ) fprintf(fp, "%d\n", ja[i]);
    
    // write A->val
    n = NNZ*nb*nb; // length of A->val
    fprintf(fp, "%d\n", n); // length of A->val
    for ( i = 0; i < n; ++i ) fprintf(fp, "%le\n", val[i]);
    
    fclose(fp);
}

/**
 * \fn void fasp_dvec_write (const char *filename, dvector *vec)
 *
 * \brief Write a dvector to disk file
 *
 * \param vec       Pointer to the dvector
 * \param filename  File name
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_dvec_write (const char  *filename,
                      dvector     *vec)
{
    INT m = vec->row, i;
    
    FILE *fp = fopen(filename,"w");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%0.15e\n",vec->val[i]);
    
    fclose(fp);
}

/**
 * \fn void fasp_dvecind_write (const char *filename, dvector *vec)
 *
 * \brief Write a dvector to disk file in coordinate format
 *
 * \param vec       Pointer to the dvector
 * \param filename  File name
 *
 * \note The routine writes the specified REAL vector in IJ format.
 *   - The first line of the file is the length of the vector;
 *   - After that, each line gives index and value of the entries.
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_dvecind_write (const char  *filename,
                         dvector     *vec)
{
    INT m = vec->row, i;
    
    FILE *fp = fopen(filename,"w");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%d %le\n",i,vec->val[i]);
    
    fclose(fp);
}

/**
 * \fn void fasp_ivec_write (const char *filename, ivector *vec)
 *
 * \brief Write a ivector to disk file in coordinate format
 *
 * \param vec       Pointer to the dvector
 * \param filename  File name
 *
 * \note The routine writes the specified INT vector in IJ format.
 *   - The first line of the file is the length of the vector;
 *   - After that, each line gives index and value of the entries.
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_ivec_write (const char  *filename,
                      ivector     *vec)
{
    INT m = vec->row, i;
    
    FILE *fp = fopen(filename,"w");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    // write number of nonzeros
    fprintf(fp,"%d\n",m);
    
    // write index and value each line
    for ( i = 0; i < m; ++i ) fprintf(fp,"%d %d\n",i,vec->val[i]+1);
    
    fclose(fp);
}

/**
 * \fn void fasp_dvec_print (const INT n, dvector *u)
 *
 * \brief Print first n entries of a vector of REAL type
 *
 * \param n   An interger (if n=0, then print all entries)
 * \param u   Pointer to a dvector
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_dvec_print (const INT  n,
                      dvector   *u)
{
    unsigned INT i;
    unsigned INT NumPrint = n;
    
    if ( n <= 0 ) NumPrint = u->row; // print all
    
    for ( i = 0; i < NumPrint; ++i ) printf("vec_%d = %15.10E\n",i,u->val[i]);
}

/**
 * \fn void fasp_ivec_print (const INT n, ivector *u)
 *
 * \brief Print first n entries of a vector of INT type
 *
 * \param n   An interger (if n=0, then print all entries)
 * \param u   Pointer to an ivector
 *
 * \author Chensong Zhang
 * \date   03/29/2009
 */
void fasp_ivec_print (const INT  n,
                      ivector   *u)
{
    unsigned INT i;
    unsigned INT NumPrint = n;
    
    if ( n <= 0 ) NumPrint = u->row; // print all
    
    for ( i = 0; i < NumPrint; ++i ) printf("vec_%d = %d\n",i,u->val[i]);
}

/**
 * \fn void fasp_dcsr_print (const dCSRmat *A)
 *
 * \brief Print out a dCSRmat matrix in coordinate format
 *
 * \param A   Pointer to the dCSRmat matrix A
 *
 * \author Xuehai Huang
 * \date   03/29/2009
 */
void fasp_dcsr_print (const dCSRmat *A)
{
    const INT m=A->row, n=A->col;
    INT i, j;
    
    printf("nrow = %d, ncol = %d, nnz = %d\n",m,n,A->nnz);
    for ( i = 0; i < m; ++i ) {
        for (j=A->IA[i]; j<A->IA[i+1]; j++)
            printf("A_(%d,%d) = %+.10E\n",i,A->JA[j],A->val[j]);
    }
}

/**
 * \fn void fasp_dcoo_print (const dCOOmat *A)
 *
 * \brief Print out a dCOOmat matrix in coordinate format
 *
 * \param A   Pointer to the dCOOmat matrix A
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 */
void fasp_dcoo_print (const dCOOmat *A)
{
    INT k;
    
    printf("nrow = %d, ncol = %d, nnz = %d\n",A->row,A->col,A->nnz);
    for ( k = 0; k < A->nnz; k++ ) {
        printf("A_(%d,%d) = %+.10E\n",A->rowind[k],A->colind[k],A->val[k]);
    }
}

/**
 * \fn void fasp_dbsr_print (const dBSRmat *A)
 *
 * \brief Print out a dBSRmat matrix in coordinate format
 *
 * \param A   Pointer to the dBSRmat matrix A
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 *
 * Modified by Chunsheng Feng on 11/16/2013
 */
void fasp_dbsr_print (const dBSRmat *A)
{
    INT i, j, k, l;
    INT nb,nb2;
    nb  = A->nb;
    nb2 = nb*nb;
    
    printf("nrow = %d, ncol = %d, nnz = %d, nb = %d, storage_manner = %d\n",
           A->ROW, A->COL, A->NNZ, A->nb, A->storage_manner);
    
    for ( i = 0; i < A->ROW; i++ ) {
        for ( j = A->IA[i]; j < A->IA[i+1]; j++ ) {
            for ( k = 0; k < A->nb; k++ ) {
                for ( l = 0; l < A->nb; l++ ) {
                    printf("A_(%d,%d) = %+.10E\n",
                           i*nb+k+1, A->JA[j]*nb+l+1,  A->val[ A->JA[j]*nb2+k*nb+l]);
                }
            }
        }
    }
    
}

/**
 * \fn void fasp_dbsr_write_coo (const char *filename, const dBSRmat *A)
 *
 * \brief Print out a dBSRmat matrix in coordinate format for matlab spy
 *
 * \param filename   Name of file to write to
 * \param A          Pointer to the dBSRmat matrix A
 *
 * \author Chunsheng Feng
 * \date   11/14/2013
 *
 * Modified by Chensong Zhang on 06/14/2014: Fix index problem.
 */
void fasp_dbsr_write_coo (const char    *filename,
                          const dBSRmat *A)
{
    
    INT i, j, k, l;
    INT nb,nb2;
    nb = A->nb;
    nb2 =nb*nb;
    
    FILE *fp = fopen(filename,"w");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
#if DEBUG_MODE > 1
    printf("nrow = %d, ncol = %d, nnz = %d, nb = %d, storage_manner = %d\n",
           A->ROW, A->COL, A->NNZ, A->nb, A->storage_manner);
#endif
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    fprintf(fp,"%% dimension of the block matrix and nonzeros %d  %d  %d\n",A->ROW,A->COL,A->NNZ); // write dimension of the block matrix
    fprintf(fp,"%% the size of each block %d\n",A->nb); // write the size of each block
    fprintf(fp,"%% storage manner of each block %d\n",A->storage_manner); // write storage manner of each block
    
    for ( i = 0; i < A->ROW; i++ ) {
        for ( j = A->IA[i]; j < A->IA[i+1]; j++ ) {
            for ( k = 0; k < A->nb; k++ ) {
                for ( l = 0; l < A->nb; l++ ) {
                    fprintf(fp, "%d %d %+.10E\n",
                            i*nb+k+1, A->JA[j]*nb+l+1, A->val[ j*nb2+k*nb+l]);
                }
            }
        }
    }
    fclose(fp);
}

/**
 * \fn void fasp_dcsr_write_coo (const char *filename, const dCSRmat *A)
 *
 * \brief Print out a dCSRmat matrix in coordinate format for matlab spy
 *
 * \param filename   Name of file to write to
 * \param A          Pointer to the dCSRmat matrix A
 *
 * \author Chunsheng Feng
 * \date   11/14/2013
 */
void fasp_dcsr_write_coo (const char    *filename,
                          const dCSRmat *A)
{
    
    INT i, j;
    printf("nrow = %d, ncol = %d, nnz = %d\n",
           A->row, A->col, A->nnz);
    
    FILE *fp = fopen(filename,"w");
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    // write dimension of the block matrix
    fprintf(fp,"%% dimension of the block matrix and nonzeros %d  %d  %d\n",
            A->row,A->col,A->nnz);
    
    for ( i = 0; i < A->row; i++ ) {
        for ( j = A->IA[i]; j < A->IA[i+1]; j++ ) {
            fprintf(fp, "%d %d %+.10E\n", i+1, A->JA[j]+1, A->val[j]);
        }
    }
    
    fclose(fp);
}

/**
 * \fn void fasp_dstr_print (const dSTRmat *A)
 *
 * \brief Print out a dSTRmat matrix in coordinate format
 *
 * \param A		Pointer to the dSTRmat matrix A
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 */
void fasp_dstr_print (const dSTRmat *A)
{
    // To be added later! --Chensong
}

/**
 * \fn fasp_matrix_read (const char *filemat, void *A)
 *
 * \brief Read matrix from different kinds of formats from both ASCII and binary files
 *
 * \param filemat File name of matrix file
 * \param A Pointer to the matrix
 *
 * \note Flags for matrix file format:
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0000: ASCII
 *	 - formatflag		 % a 3-digit number for internal use, see below
 *   - matrix			 % different types of matrix
 *
 * \note Meaning of formatflag:
 *   - matrixflag        % first digit of formatflag
 *		 + matrixflag = 1: CSR format
 *		 + matrixflag = 2: BSR format
 *		 + matrixflag = 3: STR format
 *		 + matrixflag = 4: COO format
 *		 + matrixflag = 5: MTX format
 *		 + matrixflag = 6: MTX symmetrical format
 *	 - ilength			 % third digit of formatflag, length of INT
 *	 - dlength			 % fourth digit of formatflag, length of REAL
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 *
 * Modified by Chensong Zhang on 05/01/2013
 */
void fasp_matrix_read (const char  *filename,
                       void        *A)
{
    
    INT index,flag;
    
    FILE *fp = fopen(filename,"rb");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fread(&index, sizeof(INT), 1, fp);
    
    // matrix stored in ASCII format
    if ( index == 808464432 ) {
        
        fclose(fp);
        fp = fopen(filename,"r"); // reopen file of reading file in ASCII
        fscanf(fp,"%d\n",&flag); // jump over the first line
        fscanf(fp,"%d\n",&flag); // reading the format information
        flag = (INT) flag/100;
        
        switch (flag) {
            case 0:
                fasp_dcsr_read_s(fp, (dCSRmat *)A); break;
            case 1:
                fasp_dcoo_read_s(fp, (dCSRmat *)A); break;
            case 2:
                fasp_dbsr_read_s(fp, (dBSRmat *)A); break;
            case 3:
                fasp_dstr_read_s(fp, (dSTRmat *)A); break;
            case 4:
                fasp_dcoo_read_s(fp, (dCSRmat *)A); break;
            case 5:
                fasp_dmtx_read_s(fp, (dCSRmat *)A); break;
            case 6:
                fasp_dmtxsym_read_s(fp, (dCSRmat *)A); break;
            default:
                printf("### ERROR: Unknown file flag %d", flag);
        }
        
        fclose(fp);
        return;
        
    }
    
    // matrix stored in binary format
    
    // test Endian consistence of machine and file
    SHORT EndianFlag = index;
    
    fread(&index, sizeof(INT), 1, fp);
    index = endian_convert_int(index, sizeof(INT), EndianFlag);
    flag = (INT) index/100;
    ilength = (INT) (index - flag*100)/10;
    dlength = index%10;
    
    switch (flag) {
        case 1:
            fasp_dcsr_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 2:
            fasp_dbsr_read_b(fp, (dBSRmat *)A, EndianFlag);
            break;
        case 3:
            fasp_dstr_read_b(fp, (dSTRmat *)A, EndianFlag);
            break;
        case 4:
            fasp_dcoo_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 5:
            fasp_dmtx_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 6:
            fasp_dmtxsym_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        default:
            printf("### ERROR: Unknown file flag %d", flag);
    }
    
    fclose(fp);
    
}

/**
 * \fn void fasp_matrix_read_bin (const char *filemat, void *A)
 *
 * \brief Read matrix in binary format
 *
 * \param filemat File name of matrix file
 * \param A Pointer to the matrix
 *
 * \author Xiaozhe Hu
 * \date   04/14/2013
 *
 * Modified by Chensong Zhang on 05/01/2013: Use it to read binary files!!!
 */
void fasp_matrix_read_bin (const char *filename,
                           void       *A)
{
    INT index, flag;
    FILE *fp = fopen(filename, "rb");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filename);
    
    fread(&index, sizeof(INT), 1, fp);
    
    SHORT EndianFlag = 1;
    
    index = endian_convert_int(index, sizeof(INT), EndianFlag);
    
    flag = (INT) index/100;
    ilength = (int) (index - flag*100)/10;
    dlength = index%10;
    
    switch (flag) {
        case 1:
            fasp_dcoo_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 2:
            fasp_dbsr_read_b(fp, (dBSRmat *)A, EndianFlag);
            break;
        case 3:
            fasp_dstr_read_b(fp, (dSTRmat *)A, EndianFlag);
            break;
        case 4:
            fasp_dcsr_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 5:
            fasp_dmtx_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        case 6:
            fasp_dmtxsym_read_b(fp, (dCSRmat *)A, EndianFlag);
            break;
        default:
            printf("### ERROR: Unknown file flag %d", flag);
    }
    
    fclose(fp);
    
}

/**
 * \fn fasp_matrix_write (const char *filemat, void *A, const INT flag)
 *
 * \brief write matrix from different kinds of formats from both ASCII and binary files
 *
 * \param filemat   File name of matrix file
 * \param A         Pointer to the matrix
 * \param flag      Type of file and matrix, a 3-digit number
 *
 * \note Meaning of flag:
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0: ASCII
 *	 - matrixflag
 *		+ matrixflag = 1: CSR format
 *		+ matrixflag = 2: BSR format
 *		+ matrixflag = 3: STR format
 *
 * \note Matrix file format:
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0000: ASCII
 *	 - formatflag		 % a 3-digit number
 *   - matrixflag		 % different kinds of matrix judged by formatflag
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 */
void fasp_matrix_write (const char *filename,
                        void       *A,
                        const INT   flag)
{
    
    INT fileflag, matrixflag;
    FILE *fp;
    
    matrixflag = flag%100;
    fileflag = (INT) flag/100;
    
    // write matrix in ASCII file
    if( !fileflag ) {
        
        fp = fopen(filename,"w");
        if ( fp == NULL ) {
            printf("### ERROR: Cannot open %s!\n", filename);
            fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
        }
        
        printf("%s: writing to file %s...\n", __FUNCTION__, filename);
        
        fprintf(fp,"%d%d%d%d\n",fileflag,fileflag,fileflag,fileflag);
        
        fprintf(fp,"%d%d%d\n",matrixflag,(int)sizeof(INT),(int)sizeof(REAL));
        
        switch (matrixflag) {
            case 1:
                fasp_dcsr_write_s(fp, (dCSRmat *)A);
                break;
            case 2:
                fasp_dbsr_write_s(fp, (dBSRmat *)A);
                break;
            case 3:
                fasp_dstr_write_s(fp, (dSTRmat *)A);
                break;
        }
        fclose(fp);
        return;
    }
    
    // write matrix in binary file
    fp = fopen(filename,"wb");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filename);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filename);
    
    INT putflag = fileflag*100 + sizeof(INT)*10 + sizeof(REAL);
    fwrite(&putflag,sizeof(INT),1,fp);
    
    switch (matrixflag) {
        case 1:
            fasp_dcsr_write_b(fp, (dCSRmat *)A);
            break;
        case 2:
            fasp_dbsr_write_b(fp,(dBSRmat *)A);
            break;
        case 3:
            fasp_dstr_write_b(fp, (dSTRmat *)A);
            break;
    }
    
    fclose(fp);
}

/**
 * \fn fasp_vector_read (const char *filerhs, void *b)
 *
 * \brief Read RHS vector from different kinds of formats from both ASCII and binary files
 *
 * \param filerhs File name of vector file
 * \param b Pointer to the vector
 *
 * \note Matrix file format:
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0000: ASCII
 *	 - formatflag		 % a 3-digit number
 *   - vector			 % different kinds of vector judged by formatflag
 *
 * \note Meaning of formatflag:
 *   - vectorflag        % first digit of formatflag
 *		 + vectorflag = 1: dvec format
 *		 + vectorflag = 2: ivec format
 *		 + vectorflag = 3: dvecind format
 *		 + vectorflag = 4: ivecind format
 *	 - ilength			 % second digit of formatflag, length of INT
 *	 - dlength			 % third digit of formatflag, length of REAL
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 */
void fasp_vector_read (const char *filerhs,
                       void       *b)
{
    
    INT index,flag;
    
    FILE *fp = fopen(filerhs,"rb");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filerhs);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: reading file %s...\n", __FUNCTION__, filerhs);
    
    fread(&index, sizeof(INT), 1, fp);
    
    // vector stored in ASCII
    if (index==808464432) {
        fclose(fp);
        fp = fopen(filerhs,"r");
        fscanf(fp,"%d\n",&flag);
        fscanf(fp,"%d\n",&flag);
        flag = (int) flag/100;
        
        switch (flag) {
            case 1:
                fasp_dvec_read_s(fp, (dvector *)b);
                break;
            case 2:
                fasp_ivec_read_s(fp, (ivector *)b);
                break;
            case 3:
                fasp_dvecind_read_s(fp, (dvector *)b);
                break;
            case 4:
                fasp_ivecind_read_s(fp, (ivector *)b);
                break;
        }
        fclose(fp);
        return;
    }
    
    // vector stored in binary
    SHORT EndianFlag = index;
    fread(&index, sizeof(INT), 1, fp);
    index = endian_convert_int(index, sizeof(INT), EndianFlag);
    flag = (int) index/100;
    ilength = (int) (index-100*flag)/10;
    dlength = index%10;
    
    switch (flag) {
        case 1:
            fasp_dvec_read_b(fp, (dvector *)b, EndianFlag);
            break;
        case 2:
            fasp_ivec_read_b(fp, (ivector *)b, EndianFlag);
            break;
        case 3:
            fasp_dvecind_read_b(fp, (dvector *)b, EndianFlag);
            break;
        case 4:
            fasp_ivecind_read_b(fp, (ivector *)b, EndianFlag);
            break;
    }
    
    fclose(fp);
}

/**
 * \fn fasp_vector_write (const char *filerhs, void *b, const INT flag)
 *
 * \brief write RHS vector from different kinds of formats in both ASCII and binary files
 *
 * \param filerhs File name of vector file
 *
 * \param b Pointer to the vector
 *
 * \param flag Type of file and vector, a 2-digit number
 *
 * \note Meaning of the flags
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0: ASCII
 *	 - vectorflag
 *		 + vectorflag = 1: dvec format
 *		 + vectorflag = 2: ivec format
 *		 + vectorflag = 3: dvecind format
 *		 + vectorflag = 4: ivecind format
 *
 * \note Matrix file format:
 *   - fileflag			 % fileflag = 1: binary, fileflag = 0000: ASCII
 *	 - formatflag		 % a 2-digit number
 *   - vectorflag		 % different kinds of vector judged by formatflag
 *
 * \author Ziteng Wang
 * \date   12/24/2012
 *
 * Modified by Chensong Zhang on 05/02/2013: fix a bug when writing in binary format
 */
void fasp_vector_write (const char *filerhs,
                        void       *b,
                        const INT   flag)
{
    
    INT fileflag, vectorflag;
    FILE *fp;
    
    fileflag = (int) flag/10;
    vectorflag = (int) flag%10;
    
    // write vector in ASCII
    if (!fileflag) {
        fp = fopen(filerhs,"w");
        
        if ( fp == NULL ) {
            printf("### ERROR: Cannot open %s!\n", filerhs);
            fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
        }
        
        printf("%s: writing to file %s...\n", __FUNCTION__, filerhs);
        
        fprintf(fp,"%d%d%d%d\n",fileflag,fileflag,fileflag,fileflag);
        
        fprintf(fp,"%d%d%d\n",vectorflag,(int)sizeof(INT),(int)sizeof(REAL));
        
        switch (vectorflag) {
            case 1:
                fasp_dvec_write_s(fp, (dvector *)b);
                break;
            case 2:
                fasp_ivec_write_s(fp, (ivector *)b);
                break;
            case 3:
                fasp_dvecind_write_s(fp, (dvector *)b);
                break;
            case 4:
                fasp_ivecind_write_s(fp, (ivector *)b);
                break;
        }
        fclose(fp);
        return;
    }
    
    // write vector in binary
    fp = fopen(filerhs,"wb");
    
    if ( fp == NULL ) {
        printf("### ERROR: Cannot open %s!\n", filerhs);
        fasp_chkerr(ERROR_OPEN_FILE, __FUNCTION__);
    }
    
    printf("%s: writing to file %s...\n", __FUNCTION__, filerhs);
    
    INT putflag = vectorflag*100 + sizeof(INT)*10 + sizeof(REAL);
    fwrite(&putflag,sizeof(INT),1,fp);
    
    switch (vectorflag) {
        case 1:
            fasp_dvec_write_b(fp, (dvector *)b);
            break;
        case 2:
            fasp_ivec_write_b(fp, (ivector *)b);
            break;
        case 3:
            fasp_dvecind_write_b(fp, (dvector *)b);
            break;
        case 4:
            fasp_ivecind_write_b(fp, (ivector *)b);
            break;
    }
    
    fclose(fp);
}

/**
 * \fn fasp_hb_read (const char *input_file, dCSRmat *A, dvector *b)
 *
 * \brief Read matrix and right-hans side from a HB format file
 *
 * \param input_file   File name of vector file
 * \param A            Pointer to the matrix
 * \param b            Pointer to the vector
 *
 * \note Modified from the c code hb_io_prb.c by John Burkardt
 *
 * \author Xiaoehe Hu
 * \date   05/30/2014
 */
void fasp_hb_read (const char *input_file,
                   dCSRmat    *A,
                   dvector    *b)
{
    //-------------------------
    // Setup local variables
    //-------------------------
    // variables for FASP
    dCSRmat tempA;
    
    // variables for hb_io
    
    int *colptr = NULL;
    double *exact = NULL;
    double *guess = NULL;
    int i;
    int indcrd;
    char *indfmt = NULL;
    FILE *input;
    int j;
    char *key = NULL;
    char *mxtype = NULL;
    int ncol;
    int neltvl;
    int nnzero;
    int nrhs;
    int nrhsix;
    int nrow;
    int ptrcrd;
    char *ptrfmt = NULL;
    int rhscrd;
    char *rhsfmt = NULL;
    int *rhsind = NULL;
    int *rhsptr = NULL;
    char *rhstyp = NULL;
    double *rhsval = NULL;
    double *rhsvec = NULL;
    int *rowind = NULL;
    char *title = NULL;
    int totcrd;
    int valcrd;
    char *valfmt = NULL;
    double *values = NULL;
    
    printf ( "\n" );
    printf ( "HB_FILE_READ reads all the data in an HB file.\n" );
    
    printf ( "\n" );
    printf ( "Reading the file '%s'\n", input_file );
    
    input = fopen ( input_file, "rt" );
    
    if ( !input )
    {
        printf ( "\n" );
        printf ( "### ERROR: Fail to open the file.\n" );
        return;
    }
    
    //-------------------------
    // Reading...
    //-------------------------
    hb_file_read ( input, &title, &key, &totcrd, &ptrcrd, &indcrd,
                  &valcrd, &rhscrd, &mxtype, &nrow, &ncol, &nnzero, &neltvl,
                  &ptrfmt, &indfmt, &valfmt, &rhsfmt, &rhstyp, &nrhs, &nrhsix,
                  &colptr, &rowind, &values, &rhsval, &rhsptr, &rhsind, &rhsvec,
                  &guess, &exact );
    
    //-------------------------
    // Printing if needed
    //-------------------------
#if DEBUG_MODE > 1
    /*
     Print out the header information.
     */
    hb_header_print ( title, key, totcrd, ptrcrd, indcrd, valcrd,
                     rhscrd, mxtype, nrow, ncol, nnzero, neltvl, ptrfmt, indfmt, valfmt,
                     rhsfmt, rhstyp, nrhs, nrhsix );
    /*
     Print the structure information.
     */
    hb_structure_print ( ncol, mxtype, nnzero, neltvl, colptr, rowind );
    
    /*
     Print the values.
     */
    hb_values_print ( ncol, colptr, mxtype, nnzero, neltvl, values );
    
    if ( 0 < rhscrd )
    {
        /*
         Print a bit of the right hand sides.
         */
        if ( rhstyp[0] == 'F' )
        {
            r8mat_print_some ( nrow, nrhs, rhsval, 1, 1, 5, 5, "  Part of RHS" );
        }
        else if ( rhstyp[0] == 'M' && mxtype[2] == 'A' )
        {
            i4vec_print_part ( nrhs+1, rhsptr, 10, "  Part of RHSPTR" );
            i4vec_print_part ( nrhsix, rhsind, 10, "  Part of RHSIND" );
            r8vec_print_part ( nrhsix, rhsvec, 10, "  Part of RHSVEC" );
        }
        else if ( rhstyp[0] == 'M' && mxtype[2] == 'E' )
        {
            r8mat_print_some ( nnzero, nrhs, rhsval, 1, 1, 5, 5, "  Part of RHS" );
        }
        /*
         Print a bit of the starting guesses.
         */
        if ( rhstyp[1] == 'G' )
        {
            r8mat_print_some ( nrow, nrhs, guess, 1, 1, 5, 5, "  Part of GUESS" );
        }
        /*
         Print a bit of the exact solutions.
         */
        if ( rhstyp[2] == 'X' )
        {
            r8mat_print_some ( nrow, nrhs, exact, 1, 1, 5, 5, "  Part of EXACT" );
        }
        
    }
#endif
    
    //-------------------------
    // Closing
    //-------------------------
    fclose ( input );
    
    //-------------------------
    // Convert to FASP format
    //-------------------------
    
    // convert matrix
    if (ncol != nrow) {
        printf ( "### ERROR: The matrix is not square!\n" );
        goto FINISHED;
    }
    
    tempA = fasp_dcsr_create(nrow, ncol, nnzero);
    
    for (i=0; i<=ncol;  i++) tempA.IA[i] = colptr[i]-1;
    for (i=0; i<nnzero; i++) tempA.JA[i] = rowind[i]-1;
    fasp_array_cp (nnzero, values, tempA.val);
    
    // if the matrix is symmeric
    if (mxtype[1] == 'S'){
        
        // A = A'+ A
        dCSRmat tempA_tran;
        fasp_dcsr_trans(&tempA, &tempA_tran);
        fasp_blas_dcsr_add(&tempA, 1.0, &tempA_tran, 1.0, A);
        fasp_dcsr_free(&tempA);
        fasp_dcsr_free(&tempA_tran);
        
        // modify diagonal entries
        for (i=0; i<A->row; i++) {
            
            for(j=A->IA[i]; j<A->IA[i+1]; j++){
                
                if (A->JA[j] == i) {
                    A->val[j] = A->val[j]/2;
                    break;
                }
                
            }
            
        }
        
    }
    // if the matrix is not symmetric
    else
    {
        fasp_dcsr_trans(&tempA, A);
        fasp_dcsr_free(&tempA);
        
    }
    
    // convert right hand side
    
    if ( nrhs == 0 ){
        
        printf ( "### ERROR: There is not right hand side!\n" );
        goto FINISHED;
        
    }
    else if (nrhs > 1){
        
        printf ( "### ERROR: There is more than one right hand side!\n" );
        goto FINISHED;
        
    }
    else {
        
        fasp_dvec_alloc(nrow, b);
        fasp_array_cp(nrow, rhsval, b->val);
    }
    
    //-------------------------
    // Cleanning
    //-------------------------
FINISHED:
    if ( colptr ) free ( colptr );
    if ( exact )  free ( exact );
    if ( guess )  free ( guess );
    if ( rhsind ) free ( rhsind );
    if ( rhsptr ) free ( rhsptr );
    if ( rhsval ) free ( rhsval );
    if ( rhsvec ) free ( rhsvec );
    if ( rowind ) free ( rowind );
    if ( values ) free ( values );
    
    return;
}

/*---------------------------------*/
/*--      Private Functions      --*/
/*---------------------------------*/

static void fasp_dcsr_read_s (FILE        *fp,
                              dCSRmat     *A)
{
    INT  i,m,nnz,idata;
    REAL ddata;
    
    // Read CSR matrix
    fscanf(fp, "%d", &m);
    A->row=m;
    
    A->IA=(INT *)fasp_mem_calloc(m+1, sizeof(INT));
    for ( i = 0; i <= m; ++i ) {
        fscanf(fp, "%d", &idata);
        A->IA[i] = idata;
    }
    
    nnz=A->IA[m]-A->IA[0]; A->nnz=nnz;
    
    A->JA=(INT *)fasp_mem_calloc(nnz, sizeof(INT));
    A->val=(REAL*)fasp_mem_calloc(nnz, sizeof(REAL));
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%d", &idata);
        A->JA[i] = idata;
    }
    
    for ( i = 0; i < nnz; ++i ) {
        fscanf(fp, "%lf", &ddata);
        A->val[i]= ddata;
    }
}

static void fasp_dcsr_read_b (FILE        *fp,
                              dCSRmat     *A,
                              const SHORT  EndianFlag)
{
    INT  i,m,nnz,idata;
    REAL ddata;
    
    // Read CSR matrix
    fread(&idata, ilength, 1, fp);
    A->row = endian_convert_int(idata, ilength, EndianFlag);
    m = A->row;
    
    A->IA=(INT *)fasp_mem_calloc(m+1, sizeof(INT));
    for ( i = 0; i <= m; ++i ) {
        fread(&idata, ilength, 1, fp);
        A->IA[i] = endian_convert_int(idata, ilength, EndianFlag);
    }
    
    nnz=A->IA[m]-A->IA[0]; A->nnz=nnz;
    
    A->JA=(INT *)fasp_mem_calloc(nnz, sizeof(INT));
    A->val=(REAL*)fasp_mem_calloc(nnz, sizeof(REAL));
    
    for ( i = 0; i < nnz; ++i ) {
        fread(&idata, ilength, 1, fp);
        A->JA[i] = endian_convert_int(idata, ilength, EndianFlag);
    }
    
    for ( i = 0; i < nnz; ++i ) {
        fread(&ddata, dlength, 1, fp);
        A->val[i] = endian_convert_real(ddata, dlength, EndianFlag);
    }
}

static void fasp_dcoo_read_s (FILE        *fp,
                              dCSRmat     *A)
{
    INT  i,j,k,m,n,nnz;
    REAL value;
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    dCOOmat Atmp = fasp_dcoo_create(m,n,nnz);
    
    for ( k = 0; k < nnz; k++ ) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            Atmp.rowind[k]=i; Atmp.colind[k]=j; Atmp.val[k] = value;
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dcoo_read");
        }
    }
    
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dcoo_read_b (FILE        *fp,
                              dCSRmat     *A,
                              const SHORT  EndianFlag)
{
    INT  k,m,n,nnz,index;
    REAL value;
    
    fread(&m, ilength, 1, fp);
    m = endian_convert_int(m, ilength, EndianFlag);
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fread(&nnz, ilength, 1, fp);
    nnz = endian_convert_int(nnz, ilength, EndianFlag);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    for (k = 0; k < nnz; k++) {
        if ( fread(&index, ilength, 1, fp) !=EOF ) {
            Atmp.rowind[k] = endian_convert_int(index, ilength, EndianFlag);
            fread(&index, ilength, 1, fp);
            Atmp.colind[k] = endian_convert_int(index, ilength, EndianFlag);
            fread(&value, sizeof(REAL), 1, fp);
            Atmp.val[k] = endian_convert_real(value, sizeof(REAL), EndianFlag);
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, "fasp_dcoo_read");
        }
    }
    
    fasp_format_dcoo_dcsr(&Atmp, A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dbsr_read_s (FILE        *fp,
                              dBSRmat     *A)
{
    INT  ROW, COL, NNZ, nb, storage_manner;
    INT  i, n;
    INT  index;
    REAL value;
    
    fscanf(fp, "%d %d %d", &ROW,&COL,&NNZ); // read dimension of the problem
    A->ROW = ROW; A->COL = COL; A->NNZ = NNZ;
    
    fscanf(fp, "%d", &nb); // read the size of each block
    A->nb = nb;
    
    fscanf(fp, "%d", &storage_manner); // read the storage_manner of each block
    A->storage_manner = storage_manner;
    
    // allocate memory space
    fasp_dbsr_alloc(ROW, COL, NNZ, nb, storage_manner, A);
    
    // read IA
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d", &index);
        A->IA[i] = index;
    }
    
    // read JA
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ){
        fscanf(fp, "%d", &index);
        A->JA[i] = index;
    }
    
    // read val
    fscanf(fp, "%d", &n);
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%le", &value);
        A->val[i] = value;
    }
    
}

static void fasp_dbsr_read_b (FILE        *fp,
                              dBSRmat     *A,
                              const SHORT  EndianFlag)
{
    INT    ROW, COL, NNZ, nb, storage_manner;
    INT    i, n, index;
    REAL   value;
    
    // read dimension of the problem
    fread(&ROW, ilength, 1, fp);
    A->ROW = endian_convert_int(ROW, ilength, EndianFlag);
    fread(&COL, ilength, 1, fp);
    A->COL = endian_convert_int(COL, ilength, EndianFlag);
    fread(&NNZ, ilength, 1, fp);
    A->NNZ = endian_convert_int(NNZ, ilength, EndianFlag);
    
    fread(&nb, ilength, 1, fp); // read the size of each block
    A->nb = endian_convert_int(nb, ilength, EndianFlag);
    
    fread(&storage_manner, 1, ilength, fp); // read the storage manner of each block
    A->storage_manner = endian_convert_int(storage_manner, ilength, EndianFlag);
    
    // allocate memory space
    fasp_dbsr_alloc(ROW, COL, NNZ, nb, storage_manner, A);
    
    // read IA
    fread(&n, ilength, 1, fp);
    for ( i = 0; i < n; i++ ) {
        fread(&index, 1, ilength, fp);
        A->IA[i] = endian_convert_int(index, ilength, EndianFlag);
    }
    
    // read JA
    fread(&n, ilength, 1, fp);
    for ( i = 0; i < n; i++ ) {
        fread(&index, ilength, 1, fp);
        A->JA[i] = endian_convert_int(index, ilength, EndianFlag);
    }
    
    // read val
    fread(&n, ilength, 1, fp);
    for ( i = 0; i < n; i++ ) {
        fread(&value, sizeof(REAL), 1, fp);
        A->val[i] = endian_convert_real(value, sizeof(REAL), EndianFlag);
    }
}

static void fasp_dstr_read_s (FILE        *fp,
                              dSTRmat     *A)
{
    INT  nx, ny, nz, nxy, ngrid, nband, nc, offset;
    INT  i, k, n;
    REAL value;
    
    fscanf(fp,"%d %d %d",&nx,&ny,&nz); // read dimension of the problem
    A->nx = nx; A->ny = ny; A->nz = nz;
    
    nxy = nx*ny; ngrid = nxy*nz;
    A->nxy = nxy; A->ngrid = ngrid;
    
    fscanf(fp,"%d",&nc); // read number of components
    A->nc = nc;
    
    fscanf(fp,"%d",&nband); // read number of bands
    A->nband = nband;
    
    A->offsets=(INT*)fasp_mem_calloc(nband, ilength);
    
    // read diagonal
    fscanf(fp, "%d", &n);
    A->diag=(REAL *)fasp_mem_calloc(n, sizeof(REAL));
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%le", &value);
        A->diag[i] = value;
    }
    
    // read offdiags
    k = nband;
    A->offdiag=(REAL **)fasp_mem_calloc(nband, sizeof(REAL *));
    while ( k-- ) {
        fscanf(fp,"%d %d",&offset,&n); // read number band k
        A->offsets[nband-k-1]=offset;
        
        A->offdiag[nband-k-1]=(REAL *)fasp_mem_calloc(n, sizeof(REAL));
        for ( i = 0; i < n; ++i ) {
            fscanf(fp, "%le", &value);
            A->offdiag[nband-k-1][i] = value;
        }
    }
    
}

static void fasp_dstr_read_b (FILE        *fp,
                              dSTRmat     *A,
                              const SHORT  EndianFlag)
{
    INT  nx, ny, nz, nxy, ngrid, nband, nc, offset;
    INT  i, k, n;
    REAL value;
    
    // read dimension of the problem
    fread(&nx, ilength, 1, fp);
    A->nx = endian_convert_int(nx, ilength, EndianFlag);
    fread(&ny, ilength, 1, fp);
    A->ny = endian_convert_int(ny, ilength, EndianFlag);
    fread(&nz, ilength, 1, fp);
    A->nz = endian_convert_int(nz, ilength, EndianFlag);
    
    nxy = nx*ny; ngrid = nxy*nz;
    A->nxy = nxy; A->ngrid = ngrid;
    
    // read number of components
    fread(&nc, ilength, 1, fp);
    A->nc = nc;
    
    // read number of bands
    fread(&nband, ilength, 1, fp);
    A->nband = nband;
    
    A->offsets=(INT*)fasp_mem_calloc(nband, ilength);
    
    // read diagonal
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    A->diag=(REAL *)fasp_mem_calloc(n, sizeof(REAL));
    for ( i = 0; i < n; i++ ) {
        fread(&value, sizeof(REAL), 1, fp);
        A->diag[i]=endian_convert_real(value, sizeof(REAL), EndianFlag);
    }
    
    // read offdiags
    k = nband;
    A->offdiag=(REAL **)fasp_mem_calloc(nband, sizeof(REAL *));
    while ( k-- ) {
        fread(&offset, ilength, 1, fp);
        A->offsets[nband-k-1]=endian_convert_int(offset, ilength, EndianFlag);;
        
        fread(&n, ilength, 1, fp);
        n = endian_convert_int(n, ilength, EndianFlag);
        A->offdiag[nband-k-1]=(REAL *)fasp_mem_calloc(n, sizeof(REAL));
        for ( i = 0; i < n; i++ ) {
            fread(&value, sizeof(REAL), 1, fp);
            A->offdiag[nband-k-1][i]=endian_convert_real(value, sizeof(REAL), EndianFlag);
        }
    }
}

static void fasp_dmtx_read_s (FILE        *fp,
                              dCSRmat     *A)
{
    INT  i,j,m,n,nnz;
    INT  innz; // index of nonzeros
    REAL value;
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    innz = 0;
    
    while (innz < nnz) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            
            Atmp.rowind[innz]=i-1;
            Atmp.colind[innz]=j-1;
            Atmp.val[innz] = value;
            innz = innz + 1;
            
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, __FUNCTION__);
        }
    }
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dmtx_read_b (FILE        *fp,
                              dCSRmat     *A,
                              const SHORT  EndianFlag)
{
    INT   m,n,k,nnz;
    INT   index;
    REAL  value;
    
    fread(&m, ilength, 1, fp);
    m = endian_convert_int(m, ilength, EndianFlag);
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fread(&nnz, ilength, 1, fp);
    nnz = endian_convert_int(nnz, ilength, EndianFlag);
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    for (k = 0; k < nnz; k++) {
        if ( fread(&index, ilength, 1, fp) !=EOF ) {
            Atmp.rowind[k] = endian_convert_int(index, ilength, EndianFlag)-1;
            fread(&index, ilength, 1, fp);
            Atmp.colind[k] = endian_convert_int(index, ilength, EndianFlag)-1;
            fread(&value, sizeof(REAL), 1, fp);
            Atmp.val[k] = endian_convert_real(value, sizeof(REAL), EndianFlag);
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, __FUNCTION__);
        }
    }
    
    fasp_format_dcoo_dcsr(&Atmp, A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dmtxsym_read_s (FILE        *fp,
                                 dCSRmat     *A)
{
    INT  i,j,m,n,nnz;
    INT  innz; // index of nonzeros
    REAL value;
    
    fscanf(fp,"%d %d %d",&m,&n,&nnz);
    
    nnz = 2*(nnz-m) + m; // adjust for sym problem
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    innz = 0;
    
    while (innz < nnz) {
        if ( fscanf(fp, "%d %d %le", &i, &j, &value) != EOF ) {
            
            if (i==j) {
                Atmp.rowind[innz]=i-1;
                Atmp.colind[innz]=j-1;
                Atmp.val[innz] = value;
                innz = innz + 1;
            }
            else {
                Atmp.rowind[innz]=i-1; Atmp.rowind[innz+1]=j-1;
                Atmp.colind[innz]=j-1; Atmp.colind[innz+1]=i-1;
                Atmp.val[innz] = value; Atmp.val[innz+1] = value;
                innz = innz + 2;
            }
            
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, __FUNCTION__);
        }
    }
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dmtxsym_read_b (FILE        *fp,
                                 dCSRmat     *A,
                                 const SHORT  EndianFlag)
{
    INT  m,n,nnz;
    INT  innz;
    INT  index[2];
    REAL value;
    
    fread(&m, ilength, 1, fp);
    m = endian_convert_int(m, ilength, EndianFlag);
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fread(&nnz, ilength, 1, fp);
    nnz = endian_convert_int(nnz, ilength, EndianFlag);
    
    nnz = 2*(nnz-m) + m; // adjust for sym problem
    
    dCOOmat Atmp=fasp_dcoo_create(m,n,nnz);
    
    innz = 0;
    
    while (innz < nnz) {
        if ( fread(index, ilength, 2, fp) !=EOF ) {
            
            if (index[0]==index[1]) {
                INT indextemp = index[0];
                Atmp.rowind[innz] = endian_convert_int(indextemp, ilength, EndianFlag)-1;
                indextemp = index[1];
                Atmp.colind[innz] = endian_convert_int(indextemp, ilength, EndianFlag)-1;
                fread(&value, sizeof(REAL), 1, fp);
                Atmp.val[innz] = endian_convert_real(value, sizeof(REAL), EndianFlag);
                innz = innz + 1;
            }
            else {
                INT indextemp = index[0];
                Atmp.rowind[innz] = endian_convert_int(indextemp, ilength, EndianFlag)-1;
                Atmp.rowind[innz+1] = Atmp.rowind[innz];
                indextemp = index[1];
                Atmp.colind[innz] = endian_convert_int(indextemp, ilength, EndianFlag)-1;
                Atmp.colind[innz+1] = Atmp.colind[innz];
                fread(&value, sizeof(REAL), 1, fp);
                Atmp.val[innz] = endian_convert_real(value, sizeof(REAL), EndianFlag);
                Atmp.val[innz+1] = Atmp.val[innz];
                innz = innz + 2;
            }
            
        }
        else {
            fasp_chkerr(ERROR_WRONG_FILE, __FUNCTION__);
        }
    }
    
    fasp_format_dcoo_dcsr(&Atmp,A);
    fasp_dcoo_free(&Atmp);
}

static void fasp_dcsr_write_s (FILE        *fp,
                               dCSRmat     *A)
{
    const INT m=A->row, n=A->col;
    INT i;
    
    fprintf(fp,"%d  %d  %d\n",m,n,A->nnz);
    
    for ( i = 0; i < m+1; ++i ) fprintf(fp,"%d\n", A->IA[i]);
    
    for ( i = 0; i < A->nnz; ++i ) fprintf(fp,"%d\n", A->JA[i]);
    
    for ( i = 0; i < A->nnz; ++i ) fprintf(fp,"%le\n", A->val[i]);
}

static void fasp_dcsr_write_b (FILE        *fp,
                               dCSRmat     *A)
{
    const INT m=A->row, n=A->col;
    INT i, j, nnz, index;
    REAL value;
    
    nnz = A->nnz;
    fwrite(&m, sizeof(INT), 1, fp);
    fwrite(&n, sizeof(INT), 1, fp);
    fwrite(&nnz, sizeof(INT), 1, fp);
    for ( i = 0; i < m; i++ ) {
        for (j = A->IA[i]; j < A->IA[i+1]; j++) {
            fwrite(&i, sizeof(INT), 1, fp);
            index = A->JA[j];
            value = A->val[j];
            fwrite(&index, sizeof(INT), 1, fp);
            fwrite(&value, sizeof(REAL), 1, fp);
        }
    }
    
    fclose(fp);
}

static void fasp_dbsr_write_s (FILE        *fp,
                               dBSRmat     *A)
{
    const INT ROW = A->ROW, COL = A->COL, NNZ = A->NNZ;
    const INT nb = A->nb, storage_manner = A->storage_manner;
    
    INT  *ia  = A->IA;
    INT  *ja  = A->JA;
    REAL *val = A->val;
    
    unsigned INT i, n;
    
    fprintf(fp,"%d  %d  %d\n",ROW,COL,NNZ); // write dimension of the block matrix
    
    fprintf(fp,"%d\n",nb); // write the size of each block
    
    fprintf(fp,"%d\n",storage_manner); // write storage manner of each block
    
    // write A->IA
    n = ROW+1; // length of A->IA
    fprintf(fp,"%d\n",n); // length of A->IA
    for ( i = 0; i < n; ++i ) fprintf(fp, "%d\n", ia[i]);
    
    // write A->JA
    n = NNZ; // length of A->JA
    fprintf(fp, "%d\n", n); // length of A->JA
    for ( i = 0; i < n; ++i ) fprintf(fp, "%d\n", ja[i]);
    
    // write A->val
    n = NNZ*nb*nb; // length of A->val
    fprintf(fp, "%d\n", n); // length of A->val
    for ( i = 0; i < n; ++i ) fprintf(fp, "%le\n", val[i]);
}

static void fasp_dbsr_write_b (FILE        *fp,
                               dBSRmat     *A)
{
    const INT ROW = A->ROW, COL = A->COL, NNZ = A->NNZ;
    const INT nb = A->nb, storage_manner = A->storage_manner;
    
    INT  *ia  = A->IA;
    INT  *ja  = A->JA;
    REAL *val = A->val;
    
    unsigned INT i, n, index;
    REAL value;
    
    // write dimension of the block matrix
    fwrite(&ROW, sizeof(INT), 1, fp);
    fwrite(&COL, sizeof(INT), 1, fp);
    fwrite(&NNZ, sizeof(INT), 1, fp);
    
    fwrite(&nb, sizeof(INT), 1, fp); // write the size of each block
    
    fwrite(&storage_manner, sizeof(INT), 1, fp);
    
    // write A.IA
    n = ROW+1;
    fwrite(&n, sizeof(INT), 1, fp);
    for ( i = 0; i < n; i++ ) {
        index = ia[i];
        fwrite(&index, sizeof(INT), 1, fp);
    }
    
    // write A.JA
    n = NNZ;
    fwrite(&n, sizeof(INT), 1, fp);
    for ( i = 0; i < n; i++ ) {
        index = ja[i];
        fwrite(&index, sizeof(INT), 1, fp);
    }
    
    // write A.val
    n = NNZ*nb*nb;
    fwrite(&n,sizeof(INT), 1, fp);
    for ( i = 0; i < n; i++ ) {
        value = val[i];
        fwrite(&value, sizeof(REAL), 1, fp);
    }
}

static void fasp_dstr_write_s (FILE        *fp,
                               dSTRmat     *A)
{
    const INT nx=A->nx, ny=A->ny, nz=A->nz;
    const INT ngrid=A->ngrid, nband=A->nband, nc=A->nc;
    
    INT *offsets=A->offsets;
    
    unsigned INT i, k, n;
    
    fprintf(fp,"%d  %d  %d\n",nx,ny,nz); // write dimension of the problem
    
    fprintf(fp,"%d\n",nc); // read number of components
    
    fprintf(fp,"%d\n",nband); // write number of bands
    
    // write diagonal
    n=ngrid*nc*nc; // number of nonzeros in each band
    fprintf(fp,"%d\n",n); // number of diagonal entries
    for ( i = 0; i < n; ++i ) fprintf(fp, "%le\n", A->diag[i]);
    
    // write offdiags
    k = nband;
    while ( k-- ) {
        INT offset=offsets[nband-k-1];
        n=(ngrid-ABS(offset))*nc*nc; // number of nonzeros in each band
        fprintf(fp,"%d  %d\n",offset,n); // read number band k
        for ( i = 0; i < n; ++i ) {
            fprintf(fp, "%le\n", A->offdiag[nband-k-1][i]);
        }
    }
    
}

static void fasp_dstr_write_b (FILE        *fp,
                               dSTRmat     *A)
{
    const INT nx=A->nx, ny=A->ny, nz=A->nz;
    const INT ngrid=A->ngrid, nband=A->nband, nc=A->nc;
    
    INT *offsets=A->offsets;
    
    unsigned INT i, k, n;
    REAL value;
    
    // write dimension of the problem
    fwrite(&nx, sizeof(INT), 1, fp);
    fwrite(&ny, sizeof(INT), 1, fp);
    fwrite(&nz, sizeof(INT), 1, fp);
    
    fwrite(&nc, sizeof(INT), 1, fp);  // read number of components
    
    fwrite(&nband, sizeof(INT), 1, fp); // write number of bands
    
    // write diagonal
    n=ngrid*nc*nc; // number of nonzeros in each band
    fwrite(&n, sizeof(INT), 1, fp); // number of diagonal entries
    for ( i = 0; i < n; i++ ) {
        value = A->diag[i];
        fwrite(&value, sizeof(REAL), 1, fp);
    }
    
    // write offdiags
    k = nband;
    while ( k-- ) {
        INT offset=offsets[nband-k-1];
        n=(ngrid-ABS(offset))*nc*nc; // number of nonzeros in each band
        fwrite(&offset, sizeof(INT), 1, fp);
        fwrite(&n, sizeof(INT), 1, fp);
        for ( i = 0; i < n; i++ ) {
            value = A->offdiag[nband-k-1][i];
            fwrite(&value, sizeof(REAL), 1, fp);
        }
    }
    
}

static void fasp_dvec_read_s (FILE        *fp,
                              dvector     *b)
{
    
    INT  i, n;
    REAL value;
    
    fscanf(fp,"%d",&n);
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%le", &value);
        b->val[i] = value;
    }
}

static void fasp_dvec_read_b (FILE        *fp,
                              dvector     *b,
                              const SHORT  EndianFlag)
{
    
    INT  i, n;
    REAL value;
    
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; i++ ) {
        fread(&value, dlength, 1, fp);
        b->val[i]=endian_convert_real(value, dlength, EndianFlag);
    }
}

static void fasp_ivec_read_s (FILE        *fp,
                              ivector     *b)
{
    INT i, n, value;
    
    fscanf(fp,"%d",&n);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d", &value);
        b->val[i] = value;
    }
}

static void fasp_ivec_read_b (FILE        *fp,
                              ivector     *b,
                              const SHORT  EndianFlag)
{
    INT i, n, value;
    
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; i++ ) {
        fread(&value, dlength, 1, fp);
        b->val[i]=endian_convert_real(value, dlength, EndianFlag);
    }
    
    fclose(fp);
}

static void fasp_dvecind_read_s (FILE        *fp,
                                 dvector     *b)
{
    INT  i, n, index;
    REAL value;
    
    fscanf(fp,"%d",&n);
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d %le", &index, &value);
        b->val[index] = value;
    }
}

static void fasp_dvecind_read_b (FILE        *fp,
                                 dvector     *b,
                                 const SHORT  EndianFlag)
{
    INT  i, n, index;
    REAL value;
    
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fasp_dvec_alloc(n,b);
    
    for ( i = 0; i < n; i++ ) {
        fread(&index, ilength, 1, fp);
        fread(&value, dlength, 1, fp);
        index = endian_convert_int(index, ilength, EndianFlag);
        value = endian_convert_real(value, ilength, EndianFlag);
        b->val[index] = value;
    }
}

static void fasp_ivecind_read_s (FILE        *fp,
                                 ivector     *b)
{
    INT i, n, index, value;
    
    fscanf(fp,"%d",&n);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; ++i ) {
        fscanf(fp, "%d %d", &index, &value);
        b->val[index] = value;
    }
}

static void fasp_ivecind_read_b (FILE        *fp,
                                 ivector     *b,
                                 const SHORT  EndianFlag)
{
    INT i, n, index, value;
    
    fread(&n, ilength, 1, fp);
    n = endian_convert_int(n, ilength, EndianFlag);
    fasp_ivec_alloc(n,b);
    
    for ( i = 0; i < n; i++ ) {
        fread(&index, ilength, 1, fp);
        fread(&value, dlength, 1, fp);
        index = endian_convert_int(index, ilength, EndianFlag);
        value = endian_convert_real(value, dlength, EndianFlag);
        b->val[index] = value;
    }
}

static void fasp_dvec_write_s (FILE        *fp,
                               dvector     *vec)
{
    INT m = vec->row, i;
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%le\n",vec->val[i]);
    
}

static void fasp_dvec_write_b (FILE        *fp,
                               dvector     *vec)
{
    INT m = vec->row, i;
    REAL value;
    
    fwrite(&m, sizeof(INT), 1, fp);
    
    for ( i = 0; i < m; i++ ) {
        value = vec->val[i];
        fwrite(&value, sizeof(REAL), 1, fp);
    }
}

static void fasp_ivec_write_s (FILE        *fp,
                               ivector     *vec)
{
    INT m = vec->row, i;
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%d %d\n",i,vec->val[i]);
    
}

static void fasp_ivec_write_b (FILE        *fp,
                               ivector     *vec)
{
    INT m = vec->row, i, value;
    
    fwrite(&m, sizeof(INT), 1, fp);
    
    for ( i = 0; i < m; i++ ) {
        value = vec->val[i];
        fwrite(&i, sizeof(INT), 1, fp);
        fwrite(&value, sizeof(INT), 1, fp);
    }
    
}

static void fasp_dvecind_write_s (FILE        *fp,
                                  dvector     *vec)
{
    INT m = vec->row, i;
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp,"%d %le\n",i,vec->val[i]);
    
}

static void fasp_dvecind_write_b (FILE        *fp,
                                  dvector     *vec)
{
    INT m = vec->row, i;
    REAL value;
    
    fwrite(&m, sizeof(INT), 1, fp);
    
    for ( i = 0; i < m; i++ ) {
        value = vec->val[i];
        fwrite(&i, sizeof(INT), 1, fp);
        fwrite(&value, sizeof(REAL), 1, fp);
    }
    
}

static void fasp_ivecind_write_b (FILE        *fp,
                                  ivector     *vec)
{
    INT m = vec->row, i;
    INT value;
    
    fwrite(&m, sizeof(INT), 1, fp);
    
    for ( i = 0; i < m; i++ ) {
        value = vec->val[i];
        fwrite(&i, sizeof(INT), 1, fp);
        fwrite(&value, sizeof(INT), 1, fp);
    }
    
}

static void fasp_ivecind_write_s (FILE        *fp,
                                  ivector     *vec)
{
    INT m = vec->row, i;
    
    fprintf(fp,"%d\n",m);
    
    for ( i = 0; i < m; ++i ) fprintf(fp, "%d %d\n", i, vec->val[i]);
    
}

/**
 * \fn static INT endian_convert_int (const INT inum, const INT ilength,
 *                                    const SHORT EndianFlag)
 *
 * \brief Swap order of an INT number
 *
 * \param inum        An INT value
 * \param ilength     Length of INT: 2 for short, 4 for int, 8 for long
 * \param EndianFlag  If EndianFlag = 1, it returns inum itself
 *                    If EndianFlag = 2, it returns the swapped inum
 *
 * \return Value of inum or swapped inum
 *
 * \author Ziteng Wang
 * \date   2012-12-24
 */
static INT endian_convert_int (const INT   inum,
                               const INT   ilength,
                               const INT   EndianFlag)
{
    INT iretVal,i;
    char *intToConvert = ( char* ) & inum;
    char *returnInt = ( char* ) & iretVal;
    
    if (EndianFlag==1) return inum;
    else {
        for (i = 0; i < ilength; i++) {
            returnInt[i] = intToConvert[ilength-i-1];
        }
        return iretVal;
    }
}

/**
 * \fn static REAL endian_convert_real (const REAL rnum, const INT ilength,
 *                                      const SHORT EndianFlag)
 *
 * \brief Swap order of a REAL number
 *
 * \param rnum        An REAL value
 * \param ilength     Length of INT: 2 for short, 4 for int, 8 for long
 * \param EndianFlag  If EndianFlag = 1, it returns rnum itself
 *                    If EndianFlag = 2, it returns the swapped rnum
 *
 * \return Value of rnum or swapped rnum
 *
 * \author Ziteng Wang
 * \date   2012-12-24
 */
static REAL endian_convert_real (const REAL  rnum,
                                 const INT   vlength,
                                 const INT   EndianFlag)
{
    REAL dretVal;
    char *realToConvert = (char *) & rnum;
    char *returnReal    = (char *) & dretVal;
    INT  i;
    
    if (EndianFlag==1) return rnum;
    else {
        for (i = 0; i < vlength; i++) {
            returnReal[i] = realToConvert[vlength-i-1];
        }
        return dretVal;
    }
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/
