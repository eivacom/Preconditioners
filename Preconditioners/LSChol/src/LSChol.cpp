#include "LSChol/LSChol.h"

namespace Preconditioners {

void LSChol::init() {

  /// Set default SPQR options

  ordering_ = SPQR_ORDERING_DEFAULT;
  num_fact_tol_ = SPQR_DEFAULT_TOL;
  pivot_tol_ = std::numeric_limits<Scalar>::epsilon();
  useDefaultThreshold_ = true;

  // Initialize Cholmod environment
  cholmod_l_start(&chol_com_);
}

/// Constructors

LSChol::LSChol() { init(); }

LSChol::LSChol(const SparseMatrix &A) {
  init();
  compute(A);
}

void LSChol::compute(const SparseMatrix &A) {

  LSCholSparseMatrix LSA(A);

  /// Get a view of mat as a cholmod_sparse matrix.  NB: this does *NOT*
  /// allocate new memory, rather it just wrap's A's
  cholmod_sparse Achol = Eigen::viewAsCholmod(LSA);

  /// Set threshold for numerical pivoting

  /* Compute the default threshold as in MatLab, see:
   * Tim Davis, "Algorithm 915, SuiteSparseQR: Multifrontal
   * Multithreaded Rank-Revealing Sparse QR Factorization, ACM Trans. on
   * Math. Soft. 38(1), 2011, Page 8:3
   */
  RealScalar pivotThreshold = pivot_tol_;
  if (useDefaultThreshold_) {
    RealScalar max2Norm = 0.0;
    for (int j = 0; j < A.cols(); j++)
      max2Norm = std::max(max2Norm, A.col(j).norm());
    if (max2Norm == RealScalar(0))
      max2Norm = RealScalar(1);
    pivotThreshold = 20 * (A.rows() + A.cols()) * max2Norm *
                     std::numeric_limits<RealScalar>::epsilon();
  }

  // Cholmod output
  cholmod_sparse *cR; // The sparse triangular factor R in cholmod's format
  StorageIndex *E;    // The permutation applied to columns

  // Compute factorization!
  rank_ = SuiteSparseQR<Scalar>(ordering_, pivotThreshold,
                                static_cast<Index>(A.cols()), &Achol, &cR, &E,
                                &chol_com_);

  // Store upper-triangular factor R as a standard Eigen matrix
  R_ = Eigen::viewAsEigen<Scalar, Eigen::ColMajor, StorageIndex>(*cR);

  // Release cR and E
  cholmod_l_free_sparse(&cR, &chol_com_);

  if (E != NULL)
    std::free(E);

  initialized_ = true;
}

LSChol::~LSChol() { cholmod_l_finish(&chol_com_); }

} // namespace Preconditioners
