/*! \file  testgmg.c
 *  \brief The test function for FASP GMG ssolvers
 */

#include <time.h>
#include <math.h>

#include "fasp.h"
#include "fasp_functs.h"

const REAL pi = 3.14159265;

/**
 * \fn static REAL f1d(INT i, INT nx)
 * \fn static REAL f2d(INT i, INT nx, INT ny)
 * \fn static REAL f3d(INT i, INT nx, INT ny, INT nz)
 *
 * \brief Setting f in Poisson equation, where
 *        f = sin(pi x) in 1D
 *        f = sin(pi x)*sin(pi y) in 2D
 *        f = sin(pi x)*sin(pi y)*sin(pi z) in 3D
 *
 * \param nx     Number of grids in x direction
 * \param ny     Number of grids in y direction
 * \param nz     Number of grids in z direction
 *
 * \author Ziteng Wang
 * \date   06/07/2013
 */
static REAL f1d(INT i,
                INT nx)
{
    return sin(pi *(((REAL) i)/((REAL) nx)));
}

static REAL f2d(INT i,
                INT j,
                INT nx,
                INT ny)
{
    return sin(pi *(((REAL) j)/((REAL) nx)))
          *sin(pi *(((REAL) i)/((REAL) nx)));
}

static REAL f3d(INT i,
                INT j,
                INT k,
                INT nx,
                INT ny,
                INT nz)
{
    return sin(pi *(((REAL) i)/((REAL) nx)))
          *sin(pi *(((REAL) k)/((REAL) nx)))
          *sin(pi *(((REAL) j)/((REAL) nz)));
}

/**
 * \fn static REAL L2NormError1d(REAL *u, INT nx)
 * \fn static REAL L2NormError2d(REAL *u, INT nx, INT ny)
 * \fn static REAL L2NormError3d(REAL *u, INT nx, INT ny, INT nz)
 *
 * \brief Computing Discretization Error, where exact solution
 *        u = sin(pi x)/(pi*pi)                         1D
 *        u = sin(pi x)*sin(pi y)/(2*pi*pi)             2D
 *        u = sin(pi x)*sin(pi y)*sin(pi z)/(3*pi*pi)   3D
 *
 * \param u      Vector of DOFs
 * \param nx     Number of grids in x direction
 * \param ny     Number of grids in y direction
 * \param nz     Number of grids in z direction
 *
 * \author Ziteng Wang
 * \date   06/07/2013
 *
 * Modified by Chensong Zhang on 06/07/2013: bug fixed and revise the structure
 */
static REAL L2NormError1d (REAL *u,
                           INT nx)
{
    const REAL h = 1.0/nx;
    REAL l2norm  = 0.0, uexact;
    
    INT i;
    for ( i = 1; i < nx; i++ ) {
        uexact  = sin(pi*i*h)/(pi*pi);
        l2norm += pow((u[i] - uexact), 2);
    }
    l2norm = sqrt(l2norm*h);
    
    return l2norm;
}

static REAL L2NormError2d (REAL *u,
                           INT nx,
                           INT ny)
{
    const REAL h = 1.0/nx;
    REAL l2norm  = 0.0, uexact;
    
    INT i, j;
    for ( i = 1; i < ny; i++ ) {
        for ( j = 1; j < nx; j++ ) {
            uexact  = sin(pi*i*h)*sin(pi*j*h)/(pi*pi*2.0);
            l2norm += pow((u[i*(nx+1)+j] - uexact), 2);
        }
    }
    l2norm = sqrt(l2norm*h*h);
    
    return l2norm;
}

static REAL L2NormError3d (REAL *u,
                           INT nx,
                           INT ny,
                           INT nz)
{
    const REAL h = 1.0/nx;
    REAL l2norm  = 0.0, uexact;
    
    INT i, j, k;
    for ( i = 1; i < nz; i++ ) {
        for ( j = 1; j < ny; j++ ) {
            for ( k = 1; k < nx; k++ ) {
                uexact  = sin(pi*i*h)*sin(pi*j*h)*sin(pi*k*h)/(pi*pi*3.0);
				l2norm += pow((u[i*(nx+1)*(ny+1)+j*(nx+1)+k] - uexact), 2);
            }
        }
    }
    l2norm = sqrt(l2norm*h*h*h);
    
    return l2norm;
}

/**
 * \brief An example of GMG method: V-cycle = 1, FMG = 2, PCG = 3
 *
 * \author Ziteng Wang
 * \date   06/07/2013
 *
 * \note   Number of grids of nx, ny, ny should be all equal to 2^maxlevel.
 *
 * Modified by Chensong Zhang on 06/07/2013: bug fixed and revise the structure
 */
int main (int argc, const char *argv[])
{
    const REAL rtol = 1.0e-6;
    
    INT        maxlevel = 8, dim = 3, method = 2;
    INT        i, j, k, nx, ny, nz;
    REAL       GMG_start, GMG_end;
    REAL      *u, *b, h, error0;
    
    printf("Enter spatial dimenstion (1, 2 or 3):   ");
    scanf("%d", &dim);
    
    if ( dim > 3 || dim < 1) {
        printf("### ERROR: Wrong dimension number !!!\n");
        return 0;
    }
    
    printf("Choosing solver (V-cycle=1, FMG=2, PCG=3):   ");
    scanf("%d", &method);
    
    if ( method > 3 || method < 1) {
        printf("### ERROR: Wrong solver type !!!\n");
        return 0;
    }
    
    printf("Enter the desired number of levels:   ");
    scanf("%d", &maxlevel);
    
    nx = pow(2, maxlevel);
    if ( dim > 1 ) ny = pow(2, maxlevel);
    if ( dim > 2 ) nz = pow(2, maxlevel);
    h = 1.0/((REAL) nx);
    
    fasp_gettime(&GMG_start);
    
    switch (dim) {
            
        case 1: // 1 dimesion
            
            u = (REAL *)malloc((nx+1)*sizeof(REAL));
            fasp_array_set(nx+1, u, 0.0);

            b = (REAL *)malloc((nx+1)*sizeof(REAL));
            for (i = 0; i <= nx; i++) {
                b[i] = h*h*f1d(i, nx);
            }
            
            switch (method) {
                    
                case 1: // V-cycle
                    fasp_poisson_gmg_1D(u, b, nx, maxlevel, rtol); break;
                    
                case 2: // FMG
                    fasp_poisson_fgmg_1D(u, b, nx, maxlevel, rtol); break;
                    
                case 3: // PCG
                    fasp_poisson_pcg_gmg_1D(u, b, nx, maxlevel, rtol); break;
                    
            }
            
            break;
            
        case 2: // 2 dimension
            
            u = (REAL *)malloc((nx+1)*(ny+1)*sizeof(REAL));
            fasp_array_set((nx+1)*(ny+1), u, 0.0);

            b = (REAL *)malloc((nx+1)*(ny+1)*sizeof(REAL));
            for (i = 0; i <= nx; i++) {
                for (j = 0; j <= ny; j++) {
                    b[j*(nx+1)+i] = h*h*f2d(i, j, nx, ny);
                }
            }
            
            switch (method) {
                    
                case 1: // V-cycle
                    fasp_poisson_gmg_2D(u, b, nx, ny, maxlevel, rtol); break;
                    
                case 2: // FMG
                    fasp_poisson_fgmg_2D(u, b, nx, ny, maxlevel, rtol); break;
                    
                case 3: // PCG
                    fasp_poisson_pcg_gmg_2D(u, b, nx, ny, maxlevel, rtol); break;
                    
            }
            
            break;
            
        case 3: // 3 dimension
            
            u = (REAL *)malloc((nx+1)*(ny+1)*(nz+1)*sizeof(REAL));
            fasp_array_set((nx+1)*(ny+1)*(nz+1), u, 0.0);

            b = (REAL *)malloc((nx+1)*(ny+1)*(nz+1)*sizeof(REAL));
            for (i = 0; i <= nx; i++) {
                for (j = 0; j <= ny; j++) {
                    for (k = 0; k <= nz; k++) {
                        b[i+j*(nx+1)+k*(nx+1)*(ny+1)] = h*h*f3d(i,j,k,nx,ny,nz);
                    }
                }
            }
            
            switch (method) {
                    
                case 1: // V-cycle
                    fasp_poisson_gmg_3D(u, b, nx, ny, nz, maxlevel, rtol); break;
                    
                case 2: // FMG
                    fasp_poisson_fgmg_3D(u, b, nx, ny, nz, maxlevel, rtol); break;
                    
                case 3: // PCG
                    fasp_poisson_pcg_gmg_3D(u, b, nx, ny, nz, maxlevel, rtol); break;
                    
            }
            
            break;
            
    }
    
    fasp_gettime(&GMG_end);
    print_cputime("GMG totally", GMG_end - GMG_start);
    
    switch (dim) {
            
        case 1: // 1 dimesion
            error0 = L2NormError1d(u, nx); break;
            
        case 2: // 2 dimension
            error0 = L2NormError2d(u, nx, ny); break;
            
        case 3: // 3 dimension
            error0 = L2NormError3d(u, nx, ny, nz); break;
            
    }
    
    printf("L2-norm of the discretization error: %e\n", error0);
    
    free(u);
    free(b);
    
    return SUCCESS;
}

/*---------------------------------*/
/*--        End of File          --*/
/*---------------------------------*/