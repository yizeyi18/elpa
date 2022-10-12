/*   This file is part of ELPA.

     The ELPA library was originally created by the ELPA consortium,
     consisting of the following organizations:

     - Max Planck Computing and Data Facility (MPCDF), formerly known as
       Rechenzentrum Garching der Max-Planck-Gesellschaft (RZG),
     - Bergische Universität Wuppertal, Lehrstuhl für angewandte
       Informatik,
     - Technische Universität München, Lehrstuhl für Informatik mit
       Schwerpunkt Wissenschaftliches Rechnen ,
     - Fritz-Haber-Institut, Berlin, Abt. Theorie,
     - Max-Plack-Institut für Mathematik in den Naturwissenschaften,
       Leipzig, Abt. Komplexe Strukutren in Biologie und Kognition,
       and
     - IBM Deutschland GmbH


     More information can be found here:
     http://elpa.mpcdf.mpg.de/

     ELPA is free software: you can redistribute it and/or modify
     it under the terms of the version 3 of the license of the
     GNU Lesser General Public License as published by the Free
     Software Foundation.

     ELPA is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU Lesser General Public License for more details.

     You should have received a copy of the GNU Lesser General Public License
     along with ELPA.  If not, see <http://www.gnu.org/licenses/>

     ELPA reflects a substantial effort on the part of the original
     ELPA consortium, and we ask you to respect the spirit of the
     license that we chose: i.e., please contribute any changes you
     may have back to the original ELPA library distribution, and keep
     any derivatives of ELPA under the same license that we chose for
     the original distribution, the GNU Lesser General Public License.
*/

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef WITH_MPI
#include <mpi.h>
#endif
#include <math.h>

#include <elpa/elpa.h>
#include <assert.h>

#if (TEST_NVIDIA_GPU == 1)
#include <cuda.h>
#include <cuda_runtime_api.h>
#endif


#if !(defined(TEST_REAL) ^ defined(TEST_COMPLEX))
#error "define exactly one of TEST_REAL or TEST_COMPLEX"
#endif

#if !(defined(TEST_SINGLE) ^ defined(TEST_DOUBLE))
#error "define exactly one of TEST_SINGLE or TEST_DOUBLE"
#endif

#if !(defined(TEST_SOLVER_1STAGE) ^ defined(TEST_SOLVER_2STAGE))
#error "define exactly one of TEST_SOLVER_1STAGE or TEST_SOLVER_2STAGE"
#endif

#ifdef TEST_GENERALIZED_DECOMP_EIGENPROBLEM
#define TEST_GENERALIZED_EIGENPROBLEM
#endif

#ifdef TEST_SINGLE
#  define EV_TYPE float
#  ifdef TEST_REAL
#    define MATRIX_TYPE float
#    define PREPARE_MATRIX_RANDOM prepare_matrix_random_real_single_f
#    define PREPARE_MATRIX_RANDOM_SPD prepare_matrix_random_spd_real_single_f
#    define PREPARE_MATRIX_ANALYTIC prepare_matrix_analytic_real_single_f
#    define CHECK_CORRECTNESS_EVP_NUMERIC_RESIDUALS check_correctness_evp_numeric_residuals_real_single_f
#    define CHECK_CORRECTNESS_EVP_GEN_NUMERIC_RESIDUALS check_correctness_evp_gen_numeric_residuals_real_single_f
#    define CHECK_CORRECTNESS_CHOLESKY check_correctness_cholesky_real_single_f
#    define CHECK_CORRECTNESS_HERMITIAN_MULTIPLY check_correctness_hermitian_multiply_real_single_f
#    define CHECK_CORRECTNESS_ANALYTIC check_correctness_analytic_real_single_f
#  else
#    define MATRIX_TYPE complex float
#    define PREPARE_MATRIX_RANDOM prepare_matrix_random_complex_single_f
#    define PREPARE_MATRIX_RANDOM_SPD prepare_matrix_random_spd_complex_single_f
#    define PREPARE_MATRIX_ANALYTIC prepare_matrix_analytic_complex_single_f
#    define CHECK_CORRECTNESS_EVP_NUMERIC_RESIDUALS check_correctness_evp_numeric_residuals_complex_single_f
#    define CHECK_CORRECTNESS_EVP_GEN_NUMERIC_RESIDUALS check_correctness_evp_gen_numeric_residuals_complex_single_f
#    define CHECK_CORRECTNESS_CHOLESKY check_correctness_cholesky_complex_single_f
#    define CHECK_CORRECTNESS_HERMITIAN_MULTIPLY check_correctness_hermitian_multiply_complex_single_f
#    define CHECK_CORRECTNESS_ANALYTIC check_correctness_analytic_complex_single_f
#  endif
#else
#  define EV_TYPE double
#  ifdef TEST_REAL
#    define MATRIX_TYPE double
#    define PREPARE_MATRIX_RANDOM prepare_matrix_random_real_double_f
#    define PREPARE_MATRIX_RANDOM_SPD prepare_matrix_random_spd_real_double_f
#    define PREPARE_MATRIX_ANALYTIC prepare_matrix_analytic_real_double_f
#    define CHECK_CORRECTNESS_EVP_NUMERIC_RESIDUALS check_correctness_evp_numeric_residuals_real_double_f
#    define CHECK_CORRECTNESS_EVP_GEN_NUMERIC_RESIDUALS check_correctness_evp_gen_numeric_residuals_real_double_f
#    define CHECK_CORRECTNESS_CHOLESKY check_correctness_cholesky_real_double_f
#    define CHECK_CORRECTNESS_HERMITIAN_MULTIPLY check_correctness_hermitian_multiply_real_double_f
#    define CHECK_CORRECTNESS_ANALYTIC check_correctness_analytic_real_double_f
#  else
#    define MATRIX_TYPE complex double
#    define PREPARE_MATRIX_RANDOM prepare_matrix_random_complex_double_f
#    define PREPARE_MATRIX_RANDOM_SPD prepare_matrix_random_spd_complex_double_f
#    define PREPARE_MATRIX_ANALYTIC prepare_matrix_analytic_complex_double_f
#    define CHECK_CORRECTNESS_EVP_NUMERIC_RESIDUALS check_correctness_evp_numeric_residuals_complex_double_f
#    define CHECK_CORRECTNESS_EVP_GEN_NUMERIC_RESIDUALS check_correctness_evp_gen_numeric_residuals_complex_double_f
#    define CHECK_CORRECTNESS_CHOLESKY check_correctness_cholesky_complex_double_f
#    define CHECK_CORRECTNESS_HERMITIAN_MULTIPLY check_correctness_hermitian_multiply_complex_double_f
#    define CHECK_CORRECTNESS_ANALYTIC check_correctness_analytic_complex_double_f
#  endif
#endif

#define assert_elpa_ok(x) assert(x == ELPA_OK)


#ifdef HAVE_64BIT_INTEGER_MATH_SUPPORT
#define TEST_C_INT_TYPE_PTR long int*
#define C_INT_TYPE_PTR long int*
#define TEST_C_INT_TYPE long int
#define C_INT_TYPE long int
#else
#define TEST_C_INT_TYPE_PTR int*
#define C_INT_TYPE_PTR int*
#define TEST_C_INT_TYPE int
#define C_INT_TYPE int
#endif

#ifdef HAVE_64BIT_INTEGER_MPI_SUPPORT
#define TEST_C_INT_MPI_TYPE_PTR long int*
#define C_INT_MPI_TYPE_PTR long int*
#define TEST_C_INT_MPI_TYPE long int
#define C_INT_MPI_TYPE long int
#else
#define TEST_C_INT_MPI_TYPE_PTR int*
#define C_INT_MPI_TYPE_PTR int*
#define TEST_C_INT_MPI_TYPE int
#define C_INT_MPI_TYPE int
#endif

#define TEST_GPU  0
#if (TEST_NVIDIA_GPU == 1) || (TEST_AMD_GPU == 1) || (TEST_INTEL_GPU == 1)
#undef TEST_GPU
#define TEST_GPU  1
#endif


#include "test/shared/generated.h"


int main(int argc, char** argv) {
   /* matrix dimensions */
   C_INT_TYPE na, nev, nblk;

   /* mpi */
   C_INT_TYPE myid, nprocs;
   C_INT_MPI_TYPE myidMPI, nprocsMPI;
   C_INT_TYPE na_cols, na_rows; // local matrix size
   C_INT_TYPE np_cols, np_rows; // number of MPI processes per column/row
   C_INT_TYPE my_prow, my_pcol; // local MPI task position (my_prow, my_pcol) in the grid (0..np_cols -1, 0..np_rows -1)
   C_INT_TYPE mpi_comm;
   C_INT_MPI_TYPE provided_mpi_thread_level;

   /* blacs */
   C_INT_TYPE my_blacs_ctxt, sc_desc[9], info, blacs_ok;

   /* gpu */
#if TEST_GPU_SET_ID == 1
   C_INT_TYPE  gpuID = 0;
#endif
      
   /* The Matrix */
   MATRIX_TYPE *a, *as;
#if defined(TEST_HERMITIAN_MULTIPLY)
   MATRIX_TYPE *b, *c;
#endif	   
#if defined(TEST_GENERALIZED_EIGENPROBLEM)
   MATRIX_TYPE *b, *bs;
#endif	   
   /* eigenvectors */
   MATRIX_TYPE *z;
   /* eigenvalues */
   EV_TYPE *ev;

#if TEST_GPU_DEVICE_POINTER_API == 1
   MATRIX_TYPE *a_dev, *z_dev, *b_dev, *c_dev;
   EV_TYPE *ev_dev;
#endif
   
   C_INT_TYPE error, status;
   int error_elpa;

   elpa_t handle;

   int  value;

#ifdef WITH_MPI
#ifndef WITH_OPENMP_TRADITIONAL
   MPI_Init(&argc, &argv);
#else
   MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided_mpi_thread_level);

   if (provided_mpi_thread_level != MPI_THREAD_MULTIPLE) {
     fprintf(stderr, "MPI ERROR: MPI_THREAD_MULTIPLE is not provided on this system\n");
     MPI_Finalize();
     exit(77);
   }
#endif

   MPI_Comm_size(MPI_COMM_WORLD, &nprocsMPI);
   nprocs = (C_INT_TYPE) nprocsMPI;
   MPI_Comm_rank(MPI_COMM_WORLD, &myidMPI);
   myid = (C_INT_TYPE) myidMPI;

#else
   nprocs = 1;
   myid = 0;
#endif

#if defined(HAVE_64BIT_INTEGER_MPI_SUPPORT) || defined(HAVE_64BIT_INTEGER_MATH_SUPPORT) || defined(HAVE_64BIT_INTEGER_SUPPORT)
#ifdef WITH_MPI
   MPI_Finalize();
#endif
   return 77;
#endif

#ifdef WITH_CUDA_AWARE_MPI
#if TEST_NVIDIA_GPU != 1
#ifdef WITH_MPI
   MPI_Finalize();
#endif
   return 77;
#endif
#ifdef TEST_COMPLEX
#ifdef WITH_MPI
   MPI_Finalize();
#endif
   return 77;
#endif
#endif

// pointer API is tested only for CUDA
#if TEST_GPU_DEVICE_POINTER_API == 1 && TEST_NVIDIA_GPU == 0
#ifdef WITH_MPI
   MPI_Finalize();
#endif
   return 77;
#endif


   if (argc == 4) {
     na = atoi(argv[1]);
     nev = atoi(argv[2]);
     nblk = atoi(argv[3]);
   } else {
     na = 500;
     nev = 250;
     nblk = 16;
   }

   for (np_cols = (C_INT_TYPE) sqrt((double) nprocs); np_cols > 1; np_cols--) {
     if (nprocs % np_cols == 0) {
       break;
     }
   }

   np_rows = nprocs/np_cols;

   if (myid == 0){
       printf("Matrix size: %i\n", na);
       printf("Num eigenvectors: %i\n", nev);
       printf("Blocksize: %i\n", nblk);
#ifdef WITH_MPI
       printf("Num MPI proc: %i\n", nprocs);
       printf("Number of processor rows=%i, cols=%i, total=%i\n", np_rows, np_cols, nprocs);
#endif
   }
         
   /* set up blacs */
   /* convert communicators before */
#ifdef WITH_MPI
   mpi_comm = MPI_Comm_c2f(MPI_COMM_WORLD);
#else
   mpi_comm = 0;
#endif
   set_up_blacsgrid_f(mpi_comm, np_rows, np_cols, 'C', &my_blacs_ctxt, &my_prow, &my_pcol);
   set_up_blacs_descriptor_f(na, nblk, my_prow, my_pcol, np_rows, np_cols, &na_rows, &na_cols, sc_desc, my_blacs_ctxt, &info, &blacs_ok);

   if (blacs_ok == 0) {
     if (myid == 0) {
       printf("Setting up the blacsgrid failed. Aborting...");
     }
#ifdef WITH_MPI
     MPI_Finalize();
#endif
     abort();
   }


   /* Allocate the matrices needed for elpa */
	
   a  = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
   z  = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
   as = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
   ev = calloc(na, sizeof(EV_TYPE));

   PREPARE_MATRIX_RANDOM(na, myid, na_rows, na_cols, sc_desc, a, z, as);

#ifdef TEST_HERMITIAN_MULTIPLY
	b  = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
	c  = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
	PREPARE_MATRIX_RANDOM(na, myid, na_rows, na_cols, sc_desc, b, z, c); // b=c
#endif
	   
#if defined(TEST_GENERALIZED_EIGENPROBLEM)
	b  = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
	bs = calloc(na_rows*na_cols, sizeof(MATRIX_TYPE));
	PREPARE_MATRIX_RANDOM_SPD(na, myid, na_rows, na_cols, sc_desc, b, z, bs, nblk, np_rows, np_cols, my_prow, my_pcol);
#endif

#if defined(TEST_CHOLESKY)
	PREPARE_MATRIX_RANDOM_SPD(na, myid, na_rows, na_cols, sc_desc, a, z, as, nblk, np_rows, np_cols, my_prow, my_pcol);
#endif
	
#if defined(TEST_EIGENVALUES)
   PREPARE_MATRIX_ANALYTIC(na, a, na_rows, na_cols, nblk, myid, np_rows, np_cols, my_prow, my_pcol);
   memcpy(as, a, na_rows * na_cols * sizeof(MATRIX_TYPE));
#endif  
      
   
   if (elpa_init(CURRENT_API_VERSION) != ELPA_OK) {
     fprintf(stderr, "Error: ELPA API version not supported");
     exit(1);
   }

   handle = elpa_allocate(&error_elpa);
   //assert_elpa_ok(error_elpa);

   /* Set parameters */
	
   elpa_set(handle, "na", (int) na, &error_elpa);
   assert_elpa_ok(error_elpa);

   elpa_set(handle, "nev", (int) nev, &error_elpa);
   assert_elpa_ok(error_elpa);

   if (myid == 0) {
     printf("Setting the matrix parameters na=%d, nev=%d \n",na,nev);
   }
   elpa_set(handle, "local_nrows", (int) na_rows, &error_elpa);
   assert_elpa_ok(error_elpa);

   elpa_set(handle, "local_ncols", (int) na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);

   elpa_set(handle, "nblk", (int) nblk, &error_elpa);
   assert_elpa_ok(error_elpa);

#ifdef WITH_MPI
   elpa_set(handle, "mpi_comm_parent", (int) (MPI_Comm_c2f(MPI_COMM_WORLD)), &error_elpa);
   assert_elpa_ok(error_elpa);

   elpa_set(handle, "process_row", (int) my_prow, &error_elpa);
   assert_elpa_ok(error_elpa);

   elpa_set(handle, "process_col", (int) my_pcol, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#ifdef TEST_GENERALIZED_EIGENPROBLEM
   elpa_set(handle, "blacs_context", (int) my_blacs_ctxt, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
   
   /* Setup */
   assert_elpa_ok(elpa_setup(handle));

   /* Set kernel */
	
#ifdef TEST_SOLVER_1STAGE
   elpa_set(handle, "solver", ELPA_SOLVER_1STAGE, &error_elpa);
#else
   elpa_set(handle, "solver", ELPA_SOLVER_2STAGE, &error_elpa);
#endif
   assert_elpa_ok(error_elpa);

#if TEST_NVIDIA_GPU == 1
   elpa_set(handle, "nvidia-gpu", TEST_GPU, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif

#if TEST_AMD_GPU == 1
   elpa_set(handle, "amd-gpu", TEST_GPU, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif

#if TEST_INTEL_GPU == 1
   elpa_set(handle, "intel-gpu", TEST_GPU, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif

#if defined(TEST_SOLVE_2STAGE) && defined(TEST_KERNEL)
   kernel = TEST_KERNEL
#ifdef TEST_COMPLEX
   elpa_set(handle, "complex_kernel", kernel, &error_elpa);
#else
   elpa_set(handle, "real_kernel", kernel, &error_elpa);
#endif

#ifdef TEST_REAL
#if (TEST_NVIDIA_GPU == 1)
#if WITH_NVIDIA_SM80_GPU_KERNEL == 1
     kernel = ELPA_2STAGE_REAL_NVIDIA_SM80_GPU
#else
     kernel = ELPA_2STAGE_REAL_NVIDIA_GPU
#endif
#endif /* TEST_NVIDIA_GPU */

#if (TEST_AMD_GPU == 1)
     kernel = ELPA_2STAGE_REAL_AMD_GPU
#endif

#if (TEST_INTEL_GPU == 1) || (TEST_INTEL_GPU_OPENMP == 1) || (TEST_INTEL_GPU_SYCL == 1)
     kernel = ELPA_2STAGE_REAL_INTEL_GPU_SYCL
#endif
#endif /* TEST_REAL */
#ifdef TEST_COMPLEX
#if (TEST_NVIDIA_GPU == 1)
     kernel = ELPA_2STAGE_COMPLEX_NVIDIA_GPU
#endif
#if (TEST_AMD_GPU == 1)
     kernel = ELPA_2STAGE_COMPLEX_AMD_GPU
#endif
#if (TEST_INTEL_GPU == 1) || (TEST_INTEL_GPU_OPENMP == 1) || (TEST_INTEL_GPU_SYCL == 1)
     kernel = ELPA_2STAGE_COMPLEX_INTEL_GPU_SYCL
#endif
#endif /* TEST_COMPLEX */

#ifdef TEST_COMPLEX
   elpa_set(handle, "complex_kernel", kernel, &error_elpa);
#else
   elpa_set(handle, "real_kernel", kernel, &error_elpa);
#endif

   assert_elpa_ok(error_elpa);
#endif /* defined(TEST_SOLVE_2STAGE) && defined(TEST_KERNEL) */

#if (TEST_GPU_SET_ID == 1) && (TEST_INTEL_GPU == 0) && (TEST_INTEL_GPU_OPENMP == 0) && (TEST_INTEL_GPU_SYCL == 0)
   int nDevices;
   cudaError_t err = cudaGetDeviceCount(&nDevices);
   if (err != cudaSuccess) printf("%s\n", cudaGetErrorString(err));
   printf("Number of Devices found: %d\n\n", nDevices);
   // simple test
   // Can (and should) fail often
   gpuID = myid%2;
   if(nDevices==1) gpuID=0; 
   printf("gpuID: %i\n", gpuID);
   elpa_set(handle, "use_gpu_id", gpuID, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
   
#if TEST_GPU_DEVICE_POINTER_API == 1 && TEST_NVIDIA_GPU == 1
   
   // Set device
   cudaError_t cuerr = cudaSetDevice(gpuID);
   if (cuerr != cudaSuccess){    
      printf("Error in cudaSetDevice: %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
#if defined(TEST_EIGENVECTORS)
   // malloc
   cuerr = cudaMalloc((void **) &a_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMalloc((void **) &z_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMalloc(z_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMalloc((void **) &ev_dev, na*sizeof(EV_TYPE));   
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMalloc(ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }


   cuerr = cudaMemcpy(a_dev, a, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a_dev, a): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* defined(TEST_EIGENVECTORS) */

#if defined(TEST_EIGENVALUES)
   // malloc
   cuerr = cudaMalloc((void **) &a_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMalloc((void **) &ev_dev, na*sizeof(EV_TYPE));   
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMalloc(ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMemcpy(a_dev, a, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a_dev, a): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* TEST_EIGENVALUES */

#if defined(TEST_CHOLESKY)
   elpa_set(handle, "gpu_cholesky", 1, &error_elpa);
   assert_elpa_ok(error_elpa);
   
   cuerr = cudaMalloc((void **) &a_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMemcpy(a_dev, a, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a_dev, a): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* TEST_CHOLESKY */

#if defined(TEST_HERMITIAN_MULTIPLY)
   elpa_set(handle, "gpu_hermitian_multiply", 1, &error_elpa);
   assert_elpa_ok(error_elpa);
   
   cuerr = cudaMalloc((void **) &a_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
	
   cuerr = cudaMalloc((void **) &b_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(b_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      } 	
	  
   cuerr = cudaMalloc((void **) &c_dev, na_rows*na_cols*sizeof(MATRIX_TYPE));   
   if (cuerr != cudaSuccess){    
      printf("Error in cudaMalloc(c_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMemcpy(a_dev, a, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a_dev, a): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   cuerr = cudaMemcpy(b_dev, b, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(b_dev, b): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   cuerr = cudaMemcpy(c_dev, c, na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyHostToDevice);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(c_dev, c): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

#endif /* TEST_HERMITIAN_MULTIPLY */

#endif /* TEST_GPU_DEVICE_POINTER_API == 1 && TEST_NVIDIA_GPU == 1 */     
   
   elpa_get(handle, "solver", &value, &error_elpa);
   if (myid == 0) {
     printf("Solver is set to %d \n", value);
   }
      
   //----------------------------------------------------------------------------------------------------------------------------     
	/* The actual solve step */

#if defined(TEST_EIGENVECTORS)
#if defined(TEST_EXPLICIT_NAME)
   
#if defined(TEST_REAL)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvectors_double(handle, a_dev, ev_dev, z_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvectors_double(handle, a    , ev    , z    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvectors_float (handle, a_dev, ev_dev, z_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvectors_float (handle, a    , ev    , z    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_REAL */

#if defined(TEST_COMPLEX)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvectors_double_complex(handle, a_dev, ev_dev, z_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvectors_double_complex(handle, a    , ev    , z    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvectors_float_complex (handle, a_dev, ev_dev, z_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvectors_float_complex (handle, a    , ev    , z    , &error_elpa); 
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_COMPLEX */
   
#else /* TEST_EXPLICIT_NAME */
   
	elpa_eigenvectors(handle, a, ev, z, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif /* TEST_EXPLICIT_NAME */
#endif /* TEST_EIGENVECTORS */

   
#if defined(TEST_EIGENVALUES)
#if defined(TEST_EXPLICIT_NAME)
   
#if defined(TEST_REAL)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
   elpa_eigenvalues_double(handle, a_dev, ev_dev, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
   elpa_eigenvalues_double(handle, a    , ev    , &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvalues_float (handle, a_dev, ev_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvalues_float (handle, a    , ev    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_REAL */

#if defined(TEST_COMPLEX)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvalues_double_complex(handle, a_dev, ev_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvalues_double_complex(handle, a    , ev    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_eigenvalues_float_complex (handle, a_dev, ev_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_eigenvalues_float_complex (handle, a    , ev    , &error_elpa); 
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_COMPLEX */
   
#else /* TEST_EXPLICIT_NAME */   
	elpa_eigenvalues(handle, a, ev, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif /* TEST_EXPLICIT_NAME */
#endif /* TEST_EIGENVALUES */

#if defined(TEST_CHOLESKY)
#if defined(TEST_EXPLICIT_NAME)
   
#if defined(TEST_REAL)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
   elpa_cholesky_double(handle, a_dev, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
   elpa_cholesky_double(handle, a    , &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_cholesky_float (handle, a_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_cholesky_float (handle, a    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_REAL */

#if defined(TEST_COMPLEX)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_cholesky_double_complex(handle, a_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_cholesky_double_complex(handle, a    , &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
     elpa_cholesky_float_complex (handle, a_dev, &error_elpa);
     assert_elpa_ok(error_elpa);
#else
     elpa_cholesky_float_complex (handle, a    , &error_elpa); 
     assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_COMPLEX */

#else /* TEST_EXPLICIT_NAME */
   elpa_cholesky(handle, a, &error_elpa);
   assert_elpa_ok(error_elpa);
   
#endif /* TEST_EXPLICIT_NAME */
#endif /* TEST_CHOLESKY */

#if defined(TEST_HERMITIAN_MULTIPLY)
#if defined(TEST_EXPLICIT_NAME)

#if defined(TEST_REAL)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
	elpa_hermitian_multiply_double(handle, 'F', 'F', na, a_dev, b_dev, na_rows, na_cols, c_dev, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
	elpa_hermitian_multiply_double(handle, 'F', 'F', na, a, b, na_rows, na_cols, c, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
	elpa_hermitian_multiply_float(handle, 'F', 'F', na, a_dev, b_dev, na_rows, na_cols, c_dev, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
	elpa_hermitian_multiply_float(handle, 'F', 'F', na, a, b, na_rows, na_cols, c, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_REAL */

#if defined(TEST_COMPLEX)
#if defined(TEST_DOUBLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
	elpa_hermitian_multiply_double_complex(handle, 'F', 'F', na, a_dev, b_dev, na_rows, na_cols, c_dev, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
	elpa_hermitian_multiply_double_complex(handle, 'F', 'F', na, a, b, na_rows, na_cols, c, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_DOUBLE */
#if defined(TEST_SINGLE)
#if TEST_GPU_DEVICE_POINTER_API == 1
	elpa_hermitian_multiply_float_complex(handle, 'F', 'F', na, a_dev, b_dev, na_rows, na_cols, c_dev, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#else
	elpa_hermitian_multiply_float_complex(handle, 'F', 'F', na, a, b, na_rows, na_cols, c, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif
#endif /* TEST_SINGLE */
#endif /* TEST_COMPLEX */

#else /* TEST_EXPLICIT_NAME */
	elpa_hermitian_multiply(handle, 'F', 'F', na, a, b, na_rows, na_cols, c, na_rows, na_cols, &error_elpa);
   assert_elpa_ok(error_elpa);
#endif /* TEST_EXPLICIT_NAME */
#endif /* TEST_HERMITIAN_MULTIPLY */
	
#if defined(TEST_GENERALIZED_EIGENPROBLEM)
     elpa_generalized_eigenvectors(handle, a, b, ev, z, 0, &error_elpa);
     assert_elpa_ok(error_elpa);
#if defined(TEST_GENERALIZED_DECOMP_EIGENPROBLEM)
     //a = as, so that the problem can be solved again
     memcpy(a, as, na_rows * na_cols * sizeof(MATRIX_TYPE));
     elpa_generalized_eigenvectors(handle, a, b, ev, z, 1, &error_elpa);
     assert_elpa_ok(error_elpa);
#endif
#endif
	

//----------------------------------------------------------------------------------------------------------------------------     
   /* TEST_GPU_DEVICE_POINTER_API case: copy for testing from device to host, deallocate device pointers */

#if TEST_GPU_DEVICE_POINTER_API == 1

#if defined(TEST_EIGENVECTORS)
   // copy for testing
   cuerr = cudaMemcpy(z , z_dev , na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(z , z_dev ): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMemcpy(ev, ev_dev, na*sizeof(EV_TYPE)    , cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(ev, ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   // and deallocate device pointer
   cuerr = cudaFree(a_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaFree(z_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(z_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaFree(ev_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* defined(TEST_EIGENVECTORS) */   

#if defined(TEST_EIGENVALUES)
   // copy for testing
   cuerr = cudaMemcpy(ev, ev_dev, na*sizeof(EV_TYPE)    , cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(ev, ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   // and deallocate device pointer
   cuerr = cudaFree(a_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   
   cuerr = cudaFree(ev_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(ev_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* TEST_EIGENVALUES */  

#if defined(TEST_CHOLESKY)
   // copy for testing
   cuerr = cudaMemcpy(a , a_dev , na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a, a_dev ): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   // and deallocate device pointer
   cuerr = cudaFree(a_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
#endif /* TEST_CHOLESKY */

#if defined(TEST_HERMITIAN_MULTIPLY)
   // copy for testing
   cuerr = cudaMemcpy(a , a_dev , na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(a, a_dev ): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaMemcpy(b , b_dev , na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(b, b_dev ): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
      
   cuerr = cudaMemcpy(c , c_dev , na_rows*na_cols*sizeof(MATRIX_TYPE), cudaMemcpyDeviceToHost);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaMemcpy(c, c_dev ): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
      
   // and deallocate device pointers
   cuerr = cudaFree(a_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(a_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }
   
   cuerr = cudaFree(b_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(b_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }

   cuerr = cudaFree(c_dev);
   if (cuerr != cudaSuccess){    
      fprintf(stderr, "Error in cudaFree(c_dev): %s\n", cudaGetErrorString(cuerr));
      exit(1);
      }      
#endif /* TEST_HERMITIAN_MULTIPLY */
   
#endif /* TEST_GPU_DEVICE_POINTER_API == 1 */
   
   //----------------------------------------------------------------------------------------------------------------------------
   /* Check the results */

#if defined(TEST_CHOLESKY)
	status = CHECK_CORRECTNESS_CHOLESKY(na, a, as, na_rows, na_cols, sc_desc, myid);
#endif

#if defined(TEST_HERMITIAN_MULTIPLY)
   status = CHECK_CORRECTNESS_HERMITIAN_MULTIPLY('H', na, a, b, c, na_rows, na_cols, sc_desc, myid);
#endif

#if defined(TEST_EIGENVALUES)
   status = CHECK_CORRECTNESS_ANALYTIC (na, nev, ev, z, na_rows, na_cols, nblk, myid, np_rows, np_cols, 
                                           my_prow, my_pcol, 1, 0);
#endif
   
#if defined(TEST_EIGENVECTORS)
   status = CHECK_CORRECTNESS_EVP_NUMERIC_RESIDUALS(na, nev, na_rows, na_cols, as, z, ev,
                                sc_desc, nblk, myid, np_rows, np_cols, my_prow, my_pcol);
#endif

#if defined(TEST_GENERALIZED_EIGENPROBLEM)
   status = CHECK_CORRECTNESS_EVP_GEN_NUMERIC_RESIDUALS(na, nev, na_rows, na_cols, as, z, ev,
                                sc_desc, nblk, myid, np_rows, np_cols, my_prow, my_pcol, bs);
#endif

   if (status !=0){
     printf("Test produced an error!\n");
   }
   if (status ==0){
     printf("All ok!\n");
   }

   //----------------------------------------------------------------------------------------------------------------------------
   /* Deallocate */
   
   elpa_deallocate(handle, &error_elpa);
   assert_elpa_ok(error_elpa);
   elpa_uninit(&error_elpa);
   assert_elpa_ok(error_elpa);
   
   free(a);
   free(z);
   free(as);
   free(ev);
	
#ifdef TEST_HERMITIAN_MULTIPLY
	free(b);
	free(c);
#endif
	
#if defined(TEST_GENERALIZED_EIGENPROBLEM)
   free(b);
   free(bs);
#endif
   
#ifdef WITH_MPI
   MPI_Finalize();
#endif
   
   return status;
}
