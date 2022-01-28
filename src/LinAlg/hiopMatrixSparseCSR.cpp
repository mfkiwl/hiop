#include "hiopMatrixSparseCSR.hpp"
#include "hiopVectorPar.hpp"

#include "hiop_blasdefs.hpp"

#include <algorithm> //for std::min
#include <cmath> //for std::isfinite
#include <cstring>
#include <vector>
#include <numeric>
#include <cassert>
#include <sstream>
#include <iostream>     // std::cout, std::fixed
#include <iomanip>      // std::setprecision

#include "hiopCppStdUtils.hpp"

namespace hiop
{

hiopMatrixSparseCSR::hiopMatrixSparseCSR(size_type rows, size_type cols, size_type nnz)
  : hiopMatrixSparse(rows, cols, nnz),
    irowptr_(nullptr),
    jcolind_(nullptr),
    values_(nullptr),
    buf_col_(nullptr),
    row_starts_(nullptr)
{
  if(rows==0 || cols==0) {
    assert(nnz_==0 && "number of nonzeros must be zero when any of the dimensions are 0");
    nnz_ = 0;
  } else {
    alloc();
  }
}

hiopMatrixSparseCSR::hiopMatrixSparseCSR()
  : hiopMatrixSparse(0, 0, 0),
    irowptr_(nullptr),
    jcolind_(nullptr),
    values_(nullptr),
    buf_col_(nullptr),
    row_starts_(nullptr)
{
}

  
hiopMatrixSparseCSR::~hiopMatrixSparseCSR()
{
  dealloc();
}

void hiopMatrixSparseCSR::alloc()
{
  assert(irowptr_ == nullptr);
  assert(jcolind_ == nullptr);
  assert(values_ == nullptr);

  irowptr_ = new index_type[nrows_+1];
  jcolind_ = new index_type[nnz_];
  values_ = new double[nnz_];

  assert(buf_col_ == nullptr);
  //buf_col_ remains null since it is allocated on demand
  assert(row_starts_ == nullptr);
  //row_starts_ remains null since it is allocated on demand
}


void hiopMatrixSparseCSR::dealloc()
{
  delete[] row_starts_;
  delete[] buf_col_;
  delete[] irowptr_;
  delete[] jcolind_;
  delete[] values_;
  row_starts_ = nullptr;
  buf_col_ = nullptr;
  irowptr_ = nullptr;
  jcolind_ = nullptr;
  values_ = nullptr;
}
  
void hiopMatrixSparseCSR::setToZero()
{
  for(index_type i=0; i<nnz_; i++) {
    values_[i] = 0.;
  }
}
void hiopMatrixSparseCSR::setToConstant(double c)
{
  for(index_type i=0; i<nnz_; i++) {
    values_[i] = c;
  }
}

/** y = beta * y + alpha * this * x */
void hiopMatrixSparseCSR::timesVec(double beta,
                                   hiopVector& y,
                                   double alpha,
                                   const hiopVector& x) const
{
  assert(x.get_size() == ncols_);
  assert(y.get_size() == nrows_);

  hiopVectorPar& yy = dynamic_cast<hiopVectorPar&>(y);
  const hiopVectorPar& xx = dynamic_cast<const hiopVectorPar&>(x);

  double* y_data = yy.local_data();
  const double* x_data = xx.local_data_const();

  timesVec(beta, y_data, alpha, x_data);
}

/** y = beta * y + alpha * this * x */
void hiopMatrixSparseCSR::timesVec(double beta,
                                   double* y,
                                   double alpha,
                                   const double* x) const
{
  assert(false && "not yet implemented");
}

/** y = beta * y + alpha * this^T * x */
void hiopMatrixSparseCSR::transTimesVec(double beta,
                                            hiopVector& y,
                                            double alpha,
                                            const hiopVector& x) const
{
  assert(x.get_size() == nrows_);
  assert(y.get_size() == ncols_);

  hiopVectorPar& yy = dynamic_cast<hiopVectorPar&>(y);
  const hiopVectorPar& xx = dynamic_cast<const hiopVectorPar&>(x);

  double* y_data = yy.local_data();
  const double* x_data = xx.local_data_const();

  transTimesVec(beta, y_data, alpha, x_data);
}

/** y = beta * y + alpha * this^T * x */
void hiopMatrixSparseCSR::transTimesVec(double beta,
                                        double* y,
                                        double alpha,
                                        const double* x) const
{
  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::timesMat(double beta,
                                   hiopMatrix& W,
                                   double alpha,
                                   const hiopMatrix& X) const
{
  assert(false && "not needed");
}

void hiopMatrixSparseCSR::transTimesMat(double beta,
                                            hiopMatrix& W,
                                            double alpha,
                                            const hiopMatrix& X) const
{
  assert(false && "not needed");
}

void hiopMatrixSparseCSR::timesMatTrans(double beta,
                                        hiopMatrix& Wmat,
                                        double alpha,
                                        const hiopMatrix& M2mat) const
{
  assert(false && "not needed");
}

void hiopMatrixSparseCSR::addDiagonal(const double& alpha, const hiopVector& D)
{
  assert(irowptr_ && jcolind_ && values_);
  assert(D.get_size() == nrows_);
  assert(D.get_size() == ncols_);
  
  const double* Da = D.local_data_const();
  
  for(index_type i=0; i<nrows_; ++i) {
    for(index_type pt=irowptr_[i]; pt<irowptr_[i+1]; ++pt) {
      if(jcolind_[pt]==i) {
        values_[pt] += alpha*Da[i];
        break;
      }
    }
  }
}

void hiopMatrixSparseCSR::addDiagonal(const double& value)
{
  assert(false && "not needed");
}
void hiopMatrixSparseCSR::addSubDiagonal(const double& alpha, index_type start, const hiopVector& d_)
{
  assert(false && "not needed");
}

void hiopMatrixSparseCSR::copySubDiagonalFrom(const index_type& start_on_dest_diag,
                                              const size_type& num_elems,
                                              const hiopVector& d_,
                                              const index_type& start_on_nnz_idx,
                                              double scal)
{
  assert(false && "not implemented");
}

void hiopMatrixSparseCSR::setSubDiagonalTo(const index_type& start_on_dest_diag,
                                           const size_type& num_elems,
                                           const double& c,
                                           const index_type& start_on_nnz_idx)
{
  assert(start_on_dest_diag>=0 && start_on_dest_diag+num_elems<=this->nrows_);
  assert(false && "not implemented");
}

void hiopMatrixSparseCSR::addMatrix(double alpha, const hiopMatrix& X)
{
  assert(false && "not needed");
}

/* block of W += alpha*transpose(this)
 * Note W; contains only the upper triangular entries */
void hiopMatrixSparseCSR::
transAddToSymDenseMatrixUpperTriangle(index_type row_start,
                                      index_type col_start,
                                      double alpha,
                                      hiopMatrixDense& W) const
{
  assert(row_start>=0 && row_start+ncols_<=W.m());
  assert(col_start>=0 && col_start+nrows_<=W.n());
  assert(W.n()==W.m());

  assert(false && "not yet implemented");
}

double hiopMatrixSparseCSR::max_abs_value()
{
  char norm='M'; size_type one=1;
  double maxv = DLANGE(&norm, &one, &nnz_, values_, &one, nullptr);
  return maxv;
}

void hiopMatrixSparseCSR::row_max_abs_value(hiopVector &ret_vec)
{
  assert(ret_vec.get_local_size() == nrows_);

  hiopVectorPar& yy = dynamic_cast<hiopVectorPar&>(ret_vec);
  yy.setToZero();
  double* y_data = yy.local_data();

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::scale_row(hiopVector &vec_scal, const bool inv_scale)
{
  assert(vec_scal.get_local_size() == nrows_);

  hiopVectorPar& vscal = dynamic_cast<hiopVectorPar&>(vec_scal);  
  double* vd = vscal.local_data();
  assert(false && "not yet implemented");
}

bool hiopMatrixSparseCSR::isfinite() const
{
  for(index_type i=0; i<nnz_; i++)
    if(false==std::isfinite(values_[i])) return false;
  return true;
}

hiopMatrixSparse* hiopMatrixSparseCSR::alloc_clone() const
{
  return new hiopMatrixSparseCSR(nrows_, ncols_, nnz_);
}

hiopMatrixSparse* hiopMatrixSparseCSR::new_copy() const
{
  hiopMatrixSparseCSR* copy = new hiopMatrixSparseCSR(nrows_, ncols_, nnz_);
  memcpy(copy->irowptr_, irowptr_, (nrows_+1)*sizeof(index_type));
  memcpy(copy->jcolind_, jcolind_, nnz_*sizeof(index_type));
  memcpy(copy->values_, values_, nnz_*sizeof(double));
  return copy;
}
void hiopMatrixSparseCSR::copyFrom(const hiopMatrixSparse& dm)
{
  assert(false && "to be implemented - method def too vague for now");
}

/// @brief copy to 3 arrays.
/// @pre these 3 arrays are not nullptr
void hiopMatrixSparseCSR::copy_to(index_type* irow, index_type* jcol, double* val)
{
  assert(irow && jcol && val);
  memcpy(irow, irowptr_, (1+nrows_)*sizeof(index_type));
  memcpy(jcol, jcolind_, nnz_*sizeof(index_type));
  memcpy(val, values_, nnz_*sizeof(double));
}

void hiopMatrixSparseCSR::copy_to(hiopMatrixDense& W)
{
  assert(false && "not needed");
  assert(W.m() == nrows_);
  assert(W.n() == ncols_);
}

void hiopMatrixSparseCSR::
addMDinvMtransToDiagBlockOfSymDeMatUTri(index_type rowAndCol_dest_start,
                                        const double& alpha,
                                        const hiopVector& D, hiopMatrixDense& W) const
{
  assert(false && "not needed");
}

/*
 * block of W += alpha * M1 * D^{-1} * transpose(M2), where M1=this
 *  Sizes: M1 is (m1 x nx);  D is vector of len nx, M2 is  (m2, nx)
 */
void hiopMatrixSparseCSR::
addMDinvNtransToSymDeMatUTri(index_type row_dest_start,
                             index_type col_dest_start,
                             const double& alpha,
                             const hiopVector& D,
                             const hiopMatrixSparse& M2mat,
                             hiopMatrixDense& W) const
{
  assert(false && "not needed");
}

void hiopMatrixSparseCSR::copyRowsFrom(const hiopMatrix& src_gen,
                                       const index_type* rows_idxs,
                                       size_type n_rows)
{
  const hiopMatrixSparseCSR& src = dynamic_cast<const hiopMatrixSparseCSR&>(src_gen);
  assert(this->m() == n_rows);
  assert(this->numberOfNonzeros() <= src.numberOfNonzeros());
  assert(this->n() == src.n());
  assert(n_rows <= src.m());

  assert(false && "not yet implemented");
}

/**
 * @brief Copy 'n_rows' rows started from 'rows_src_idx_st' (array of size 'n_rows') from 'src' to the destination,
 * which starts from the 'rows_dest_idx_st'th row in 'this'
 *
 * @pre 'this' must have exactly, or more than 'n_rows' rows
 * @pre 'this' must have exactly, or more cols than 'src'
 */
void hiopMatrixSparseCSR::copyRowsBlockFrom(const hiopMatrix& src_gen,
                                         const index_type& rows_src_idx_st, const size_type& n_rows,
                                         const index_type& rows_dest_idx_st, const size_type& dest_nnz_st)
{
  const hiopMatrixSparseCSR& src = dynamic_cast<const hiopMatrixSparseCSR&>(src_gen);
  assert(this->numberOfNonzeros() >= src.numberOfNonzeros());
  assert(this->n() >= src.n());
  assert(n_rows + rows_src_idx_st <= src.m());
  assert(n_rows + rows_dest_idx_st <= this->m());

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::copySubmatrixFrom(const hiopMatrix& src_gen,
                                            const index_type& dest_row_st,
                                            const index_type& dest_col_st,
                                            const size_type& dest_nnz_st,
                                            const bool offdiag_only)
{
  const hiopMatrixSparseCSR& src = dynamic_cast<const hiopMatrixSparseCSR&>(src_gen);
  auto m_rows = src.m();
  auto n_cols = src.n();

  assert(this->numberOfNonzeros() >= src.numberOfNonzeros());
  assert(n_cols + dest_col_st <= this->n() );
  assert(m_rows + dest_row_st <= this->m());
  assert(dest_nnz_st <= this->numberOfNonzeros());

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::copySubmatrixFromTrans(const hiopMatrix& src_gen,
                                                 const index_type& dest_row_st,
                                                 const index_type& dest_col_st,
                                                 const size_type& dest_nnz_st,
                                                 const bool offdiag_only)
{
  const auto& src = dynamic_cast<const hiopMatrixSparseCSR&>(src_gen);
  auto m_rows = src.n();
  auto n_cols = src.m();

  assert(this->numberOfNonzeros() >= src.numberOfNonzeros());
  assert(n_cols + dest_col_st <= this->n() );
  assert(m_rows + dest_row_st <= this->m());
  assert(dest_nnz_st <= this->numberOfNonzeros());

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::
setSubmatrixToConstantDiag_w_colpattern(const double& scalar,
                                        const index_type& dest_row_st,
                                        const index_type& dest_col_st,
                                        const size_type& dest_nnz_st,
                                        const size_type& nnz_to_copy,
                                        const hiopVector& ix)
{
  assert(ix.get_local_size() + dest_row_st <= this->m());
  assert(nnz_to_copy + dest_col_st <= this->n() );
  assert(dest_nnz_st + nnz_to_copy <= this->numberOfNonzeros());
  
  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::
setSubmatrixToConstantDiag_w_rowpattern(const double& scalar,
                                        const index_type& dest_row_st,
                                        const index_type& dest_col_st,
                                        const size_type& dest_nnz_st,
                                        const size_type& nnz_to_copy,
                                        const hiopVector& ix)
{
  assert(nnz_to_copy + dest_row_st <= this->m());
  assert(ix.get_local_size() + dest_col_st <= this->n() );
  assert(dest_nnz_st + nnz_to_copy <= this->numberOfNonzeros());
  
  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::
copyDiagMatrixToSubblock(const double& src_val,
                         const index_type& dest_row_st,
                         const index_type& col_dest_st,
                         const size_type& dest_nnz_st,
                         const size_type &nnz_to_copy)
{
  assert(this->numberOfNonzeros() >= nnz_to_copy+dest_nnz_st);
  assert(this->n() >= nnz_to_copy);
  assert(nnz_to_copy + dest_row_st <= this->m());
  assert(nnz_to_copy + col_dest_st <= this->n());

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::
copyDiagMatrixToSubblock_w_pattern(const hiopVector& dx,
                                   const index_type& dest_row_st,
                                   const index_type& dest_col_st,
                                   const size_type& dest_nnz_st,
                                   const size_type &nnz_to_copy,
                                   const hiopVector& ix)
{
  assert(this->numberOfNonzeros() >= nnz_to_copy+dest_nnz_st);
  assert(this->n() >= nnz_to_copy);
  assert(nnz_to_copy + dest_row_st <= this->m());
  assert(nnz_to_copy + dest_col_st <= this->n());

  assert(false && "not yet implemented");
}

void hiopMatrixSparseCSR::print(FILE* file, const char* msg/*=nullptr*/,
                                    int maxRows/*=-1*/, int maxCols/*=-1*/,
                                    int rank/*=-1*/) const
{
  int myrank_=0, numranks=1; //this is a local object => always print

  if(file==nullptr) file = stdout;

  int max_elems = maxRows>=0 ? maxRows : nnz_;
  max_elems = std::min(max_elems, nnz_);
  
  if(myrank_==rank || rank==-1) {
    std::stringstream ss;
    if(nullptr==msg) {
      if(numranks>1) {
        ss << "CSR matrix of size " << m() << " " << n() << " and nonzeros " 
           << numberOfNonzeros() << ", printing " <<  max_elems << " elems (on rank="
           << myrank_ << ")" << std::endl;
      } else {
        ss << "CSR matrix of size " << m() << " " << n() << " and nonzeros " 
           << numberOfNonzeros() << ", printing " <<  max_elems << " elems" << std::endl;
      }
    } else {
      ss << msg << " ";
    }

    // using matlab indices (starting at 1)
    //fprintf(file, "iRow_=[");
    ss << "iRow_=[";

    for(index_type i=0; i<nrows_; i++) {
      const index_type ip1 = i+1;
      for(int p=irowptr_[i]; p<irowptr_[i+1] && p<max_elems; ++p) {
        ss << ip1 << "; ";
      }
    }
    ss << "];" << std::endl;

    ss << "jCol_=[";
    for(index_type it=0; it<max_elems; it++) {
      ss << (jcolind_[it]+1) << "; ";
    }
    ss << "];" << std::endl;
    
    ss << "v=[";
    ss << std::scientific << std::setprecision(16);
    for(index_type it=0; it<max_elems; it++) {
      ss << values_[it] << "; ";
    }
    //fprintf(file, "];\n");
    ss << "];" << std::endl;

    fprintf(file, "%s", ss.str().c_str());
  }
}


//M = X*D*Y -> computes nnz in M and allocates M 
//By convention, M is mxn, X is mxK and Y is Kxn
hiopMatrixSparseCSR* hiopMatrixSparseCSR::times_mat_alloc(const hiopMatrixSparseCSR& Y) const
{
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();

  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;

  const index_type m = this->m();
  const index_type n = Y.n();

  const index_type K = this->n();
  assert(Y.m() == K);
  
  index_type nnzM = 0;
    // count the number of entries in the result M
  char* flag = new char[n];
  
  for(int i=0; i<m; i++) {
    //reset flag 
    memset(flag, 0, n*sizeof(char));

    for(int pt=irowptrX[i]; pt<irowptrX[i+1]; pt++) {
      //X[i,k] is nonzero
      const index_type k = jcolindX[pt];
      assert(k<K);

      //add the nonzero pattern of row k of Y to M
      for(int p=irowptrY[k]; p<irowptrY[k+1]; p++) {
	const index_type j = jcolindY[p];
        assert(j<n);
        
        //Y[k,j] is non zero, hence M[i,j] is non zero
	if(flag[j]==0) {
          //only count once
	  nnzM++;
	  flag[j]=1;
	}
      }
    }
  }
  assert(nnzM>=0); //overflow?!?
  delete[] flag;

  //allocate result M
  return new hiopMatrixSparseCSR(m, n, nnzM);
} 

/**
 *  M = X*D*Y -> computes nnz in M and allocates M 
 * By convention, M is mxn, X is mxK, Y is Kxn, and D is size K.
 * 
 * The algorithm uses the fact that the sparsity pattern of the i-th row of M is
 *           K
 * M_{i*} = sum x_{ik} Y_{j*}   (see Tim Davis book p.17)
 *          k=1
 * Therefore, to get sparsity pattern of the i-th row of M:
 *  1. we iterate over nonzeros (i,k) in the i-th row of X
 *  2. for each such k we iterate over the nonzeros (k,j) in the k-th row of Y and 
 *  3. count (i,j) as nonzero of M 
 */
void hiopMatrixSparseCSR::times_mat_symbolic(hiopMatrixSparseCSR& M,
                                             const hiopMatrixSparseCSR& Y) const
{
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();
  const double* valuesY = Y.M();
  
  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;
  const double* valuesX = values_;

  index_type* irowptrM = M.i_row();
  index_type* jcolindM = M.j_col();
  double* valuesM = M.M();
  
  const index_type m = this->m();
  const index_type n = Y.n();
  assert(M.m()==m && M.n()==n);
  
  const index_type K = this->n();
  assert(Y.m() == K);
  
  if(nullptr == M.buf_col_) {
    M.buf_col_ = new double[n];
  }
  double* W = M.buf_col_;
  
  char* flag=new char[n];

  for(int it=0; it<n; it++) {
    W[it] = 0.0;
  }

  int nnzM=0;
  for(int i=0; i<m; i++) {
    memset(flag, 0, n);

    //start row i of M
    irowptrM[i]=nnzM;
    
    for(int px=irowptrX[i]; px<irowptrX[i+1]; px++) { 
      const auto k = jcolindX[px]; //X[i,k] is non-zero
      assert(k<K);
      
      //const double val = valuesX[px]*d[k];

      //iterate the row k of Y and scatter the values into W
      for(int py=irowptrY[k]; py<irowptrY[k+1]; py++) {
	const auto j = jcolindY[py];
        assert(j<n);
        
	//we have M[k,j] nonzero
	if(flag[j]==0) {
          assert(nnzM<M.numberOfNonzeros());
          
	  jcolindM[nnzM++]=j;
	  flag[j]=1;
	}
        //W[j] += (valuesY[py]*val);
      }
    }
  }
  irowptrM[m] = nnzM;
  delete[] flag;
}

void hiopMatrixSparseCSR::times_mat_numeric(double beta,
                                            hiopMatrixSparseCSR& M,
                                            double alpha,
                                            const hiopMatrixSparseCSR& Y)
{
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();
  const double* valuesY = Y.M();
  
  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;
  const double* valuesX = values_;

  index_type* irowptrM = M.i_row();
  index_type* jcolindM = M.j_col();
  double* valuesM = M.M();
  
  const index_type m = this->m();
  const index_type n = Y.n();
  assert(M.m()==m && M.n()==n);
  
  const index_type K = this->n();
  assert(Y.m() == K);

  if(beta!=1.0) {
    int NN = M.numberOfNonzeros();
    if(beta==0.0) {
      //just in case M comes uninitialized
      for(index_type i=0; i<NN; ++i) {
        valuesM[i] = 0.0;
      }
    } else {
      //since beta is nonzero, we assume M is initialized
      int inc = 1;
      DSCAL(&NN, &beta, valuesM, &inc);
    }
  }
  
  if(nullptr == M.buf_col_) {
    M.buf_col_ = new double[n];
  }
  double* W = M.buf_col_;

  for(int it=0; it<n; it++) {
    W[it] = 0.0;
  }

  for(int i=0; i<m; i++) {
    for(int px=irowptrX[i]; px<irowptrX[i+1]; px++) { 
      const auto k = jcolindX[px]; //X[i,k] is non-zero
      assert(k<K);
      
      const double val = valuesX[px];

      //iterate the row k of Y and scatter the values into W
      for(int py=irowptrY[k]; py<irowptrY[k+1]; py++) {
        assert(jcolindY[py]<n);        
	W[jcolindY[py]] += (valuesY[py]*val);
      }
    }
    //gather the values into the i-th row M
    for(int p=irowptrM[i]; p<irowptrM[i+1]; ++p) {
      const auto j = jcolindM[p];
      valuesM[p] += alpha*W[j];
      W[j] = 0.0;
    }
  }
}

void hiopMatrixSparseCSR::form_from_symbolic(const hiopMatrixSparseTriplet& M)
{
  if(M.m()!=nrows_ || M.n()!=ncols_ || M.numberOfNonzeros()!=nnz_) {
    dealloc();
    
    nrows_ = M.m();
    ncols_ = M.n();
    nnz_ = M.numberOfNonzeros();

    alloc();
  }

  assert(nnz_>=0);
  if(nnz_<=0) {
    return;
  }
  
  assert(irowptr_);
  assert(jcolind_);
  assert(values_);

  const index_type* Mirow = M.i_row();
  const index_type* Mjcol = M.j_col();

  //storage the row count
  std::vector<index_type> w(nrows_, 0);
  
  for(int it=0; it<nnz_; ++it) {
    const index_type row_idx = Mirow[it];

#ifndef NDEBUG
    if(it>0) {
      assert(Mirow[it] >= Mirow[it-1] && "row indexes of the triplet format are not ordered.");
      if(Mirow[it] == Mirow[it-1]) {
        assert(Mjcol[it] > Mjcol[it-1] && "col indexes of the triplet format are not ordered or unique.");
      }
    }
#endif
    assert(row_idx<nrows_ && row_idx>=0);
    assert(Mjcol[it]<ncols_ && Mjcol[it]>=0);

    w[row_idx]++;

    jcolind_[it] = Mjcol[it];
  }

  irowptr_[0] = 0;
  for(int i=0; i<nrows_; i++) {
    irowptr_[i+1] = irowptr_[i] + w[i];
  }
  assert(irowptr_[nrows_] == nnz_);
}

void hiopMatrixSparseCSR::form_from_numeric(const hiopMatrixSparseTriplet& M)
{
  assert(irowptr_ && jcolind_ && values_);
  assert(nrows_ == M.m());
  assert(ncols_ == M.n());
  assert(nnz_ == M.numberOfNonzeros());

  memcpy(values_, M.M(), nnz_*sizeof(double));
}

void hiopMatrixSparseCSR::form_transpose_from_symbolic(const hiopMatrixSparseTriplet& M)
{
  if(M.m()!=ncols_ || M.n()!=nrows_ || M.numberOfNonzeros()!=nnz_) {
    dealloc();
    
    nrows_ = M.n();
    ncols_ = M.m();
    nnz_ = M.numberOfNonzeros();

    alloc();
  }

  assert(nnz_>=0);
  if(nnz_<=0) {
    return;
  }
  
  assert(irowptr_);
  assert(jcolind_);
  assert(values_);

  const index_type* Mirow = M.i_row();
  const index_type* Mjcol = M.j_col();
  const double* Mvalues  = M.M();

  assert(nullptr == row_starts_);
  row_starts_ = new index_type[nrows_];

  //in this method we use the row_starts_ as working buffer to count nz on each row of `this`
  //at the end of this method row_starts_ keeps row starts, used by the numeric method to
  //speed up computations
  {
    index_type* w = row_starts_;
    
    // initialize nz per row to zero
    for(index_type i=0; i<nrows_; ++i) {
      w[i] = 0;
    }
    // count number of nonzeros in each row
    for(index_type it=0; it<nnz_; ++it) {
      assert(Mjcol[it]<nrows_);
      w[Mjcol[it]]++;
    }
  
    // cum sum in irowptr_ and set w to the row starts
    irowptr_[0] = 0;
    for(int i=1; i<=nrows_; ++i) {
      irowptr_[i] = irowptr_[i-1] + w[i-1];
      w[i-1] = irowptr_[i-1];
    }
    //here row_starts_(==w) contains the row starts
  }
  assert(irowptr_[nrows_] = nnz_);

  //populate jcolind_ and values_
  for(index_type it=0; it<nnz_; ++it) {
    const index_type row_idx = Mjcol[it];
    
    //index in nonzeros of this (transposed)
    const auto nz_idx = row_starts_[row_idx];
    assert(nz_idx<nnz_);
    
    //assign col
    jcolind_[nz_idx] = Mirow[it];
    //values_[nz_idx] = Mvalues[it];
    assert(Mirow[it] < ncols_);
    
    //increase start for row 'row_idx'
    row_starts_[row_idx]++;

    assert(row_starts_[row_idx] <= irowptr_[row_idx+1]);
  }

  //rollback row_starts_
  for(int i=nrows_-1; i>=1; --i) {
    row_starts_[i] = row_starts_[i-1];
  }
  row_starts_[0]=0;
#ifndef NDEBUG
  for(int i=0; i<nrows_; i++) {
    for(int itnz=irowptr_[i]+1; itnz<irowptr_[i+1]; ++itnz) {
      assert(jcolind_[itnz] > jcolind_[itnz-1] &&
             "something wrong: col indexes not sorted or not unique");
    }
  }
#endif
}

void hiopMatrixSparseCSR::form_transpose_from_numeric(const hiopMatrixSparseTriplet& M)
{
  assert(irowptr_ && jcolind_ && values_ && row_starts_);
  assert(nrows_ == M.n());
  assert(ncols_ == M.m());
  assert(nnz_ == M.numberOfNonzeros());
  
#ifndef NDEBUG
  for(int i=0; i<nrows_; i++) {
    for(int itnz=irowptr_[i]+1; itnz<irowptr_[i+1]; ++itnz) {
      assert(jcolind_[itnz] > jcolind_[itnz-1] &&
             "something wrong: col indexes not sorted or not unique");
    }
  }
#endif
  const index_type* Mirow = M.i_row();
  const index_type* Mjcol = M.j_col();
  const double* Mvalues  = M.M();

  //populate values_
  for(index_type it=0; it<nnz_; ++it) {
    const index_type row_idx = Mjcol[it];
    
    //index in nonzeros of this (transposed)
    const auto nz_idx = row_starts_[row_idx];
    assert(nz_idx<nnz_);
    
    //set value
    values_[nz_idx] = Mvalues[it];
    assert(Mirow[it] < ncols_);
    
    //increase start for row 'row_idx'
    row_starts_[row_idx]++;

    assert(row_starts_[row_idx] <= irowptr_[row_idx+1]);
  }

  for(int i=nrows_-1; i>=1; --i) {
    row_starts_[i] = row_starts_[i-1];
  }
  row_starts_[0]=0;
}

void hiopMatrixSparseCSR::form_diag_from_symbolic(const hiopVector& D)
{
  int m = D.get_size();
  if(m!=ncols_ || m!=nrows_ || m!=nnz_) {
    dealloc();
    
    nrows_ = m;
    ncols_ = m;
    nnz_ = m;

    alloc();
  }

  assert(irowptr_);
  assert(jcolind_);
  assert(values_);

  const double* da = D.local_data_const();

  for(index_type i=0; i<m; i++) {
    irowptr_[i] = i;
    jcolind_[i] = i;
  }
  irowptr_[m] = m;
}

void hiopMatrixSparseCSR::form_diag_from_numeric(const hiopVector& D)
{
  assert(D.get_size()==ncols_ && D.get_size()==nrows_ && D.get_size()==nnz_);
  memcpy(values_, D.local_data_const(), nrows_*sizeof(double));
}

///Column scaling or right multiplication by a diagonal: `this`=`this`*D
void hiopMatrixSparseCSR::scale_cols(const hiopVector& D)
{
  assert(ncols_ == D.get_size());
  const double* Da = D.local_data_const();  
  
  for(index_type i=0; i<nrows_; ++i) {
    for(index_type pt=irowptr_[i]; pt<irowptr_[i+1]; ++pt) {
      values_[pt] *= Da[jcolind_[pt]];
    }
  }

}

/// @brief Row scaling or left multiplication by a diagonal: `this`=D*`this`
void hiopMatrixSparseCSR::scale_rows(const hiopVector& D)
{
  assert(nrows_ == D.get_size());
  const double* Da = D.local_data_const();
  
  for(index_type i=0; i<nrows_; ++i) {
    for(index_type pt=irowptr_[i]; pt<irowptr_[i+1]; ++pt) {
      values_[pt] *= Da[i];
    }
  }
}

// sparsity pattern of M=X+Y, where X is `this`
hiopMatrixSparseCSR* hiopMatrixSparseCSR::add_matrix_alloc(const hiopMatrixSparseCSR& Y) const
{
  assert(nrows_ == Y.m());
  assert(ncols_ == Y.n());
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();

  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;

  // count the number of entries in the result M
  index_type nnzM = 0;
  
  for(int i=0; i<nrows_; i++) {

    // add nx pattern of rows i of X and Y ordered by col indexes

    index_type ptX = irowptrX[i];
    index_type ptY = irowptrY[i];

    while(ptX<irowptrX[i+1] && ptY<irowptrY[i+1]) {
      const index_type jX = jcolindX[ptX];
      const index_type jY = jcolindY[ptY];
      assert(jX<ncols_);
      assert(jY<ncols_);

      nnzM++;
      if(jX<jY) {
        ptX++;
      } else {
        if(jX==jY) {
          ptX++;
          ptY++;
        } else {
          // jX>jY
          ptY++;
        }
      }     
    } // end of while
    assert(ptX==irowptrX[i+1] || ptY==irowptrY[i+1]);
    for(; ptX<irowptrX[i+1]; ++ptX) {
      nnzM++;
    }
    for(; ptY<irowptrY[i+1]; ++ptY) {
      nnzM++;
    }
    
  } // end of for over rows
  assert(nnzM>=0); //overflow?!?

  //allocate result M
  return new hiopMatrixSparseCSR(nrows_, ncols_, nnzM);
}

/**
 * Computes sparsity pattern of M = X+Y (i.e., populates the row pointers and 
 * column indexes arrays) of `M`.
 *
 */
void hiopMatrixSparseCSR::add_matrix_symbolic(hiopMatrixSparseCSR& M, const hiopMatrixSparseCSR& Y) const
{
  assert(nrows_ == Y.m());
  assert(ncols_ == Y.n());
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();

  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;

  index_type* irowptrM = M.i_row();
  index_type* jcolindM = M.j_col();


  // counter for nz in M 
  index_type itnnzM = 0;
  
  for(int i=0; i<nrows_; i++) {

    irowptrM[i] = itnnzM;
    
    // add nx pattern of rows i of X and Y ordered by col indexes

    index_type ptX = irowptrX[i];
    index_type ptY = irowptrY[i];

    while(ptX<irowptrX[i+1] && ptY<irowptrY[i+1]) {
      const index_type jX = jcolindX[ptX];
      const index_type jY = jcolindY[ptY];
      assert(jX<ncols_);
      assert(jY<ncols_);
      assert(itnnzM<M.numberOfNonzeros());
      
      if(jX<jY) {
        jcolindM[itnnzM] = jX;
        ptX++;
      } else {
        if(jX==jY) {
          jcolindM[itnnzM] = jX;
          ptX++;
          ptY++;
        } else {
          // jX>jY
          jcolindM[itnnzM] = jY;
          ptY++;
        }
      }
      itnnzM++;
    } // end of while
    assert(ptX==irowptrX[i+1] || ptY==irowptrY[i+1]);
    for(; ptX<irowptrX[i+1]; ++ptX) {
      const index_type jX = jcolindX[ptX];
      assert(jX<ncols_);
      assert(itnnzM<M.numberOfNonzeros());
      
      jcolindM[itnnzM] = jX;
      itnnzM++;
    }
    for(; ptY<irowptrY[i+1]; ++ptY) {
      const index_type jY = jcolindY[ptY];
      assert(jY<ncols_);
      assert(itnnzM<M.numberOfNonzeros());
      
      jcolindM[itnnzM] = jY;
      itnnzM++;
    }
    assert(itnnzM<=M.numberOfNonzeros());
  } // end of for over rows
  assert(itnnzM<=M.numberOfNonzeros());
  irowptrM[nrows_] = itnnzM;
}

/**
 * Performs matrix addition M = gamma*M + alpha*X + beta*Y numerically
 */
void hiopMatrixSparseCSR::hiopMatrixSparseCSR::add_matrix_numeric(double gamma,
                                                                  hiopMatrixSparseCSR& M,
                                                                  double alpha,
                                                                  const hiopMatrixSparseCSR& Y,
                                                                  double beta) const
{
  assert(nrows_ == Y.m());
  assert(ncols_ == Y.n());
  const index_type* irowptrY = Y.i_row();
  const index_type* jcolindY = Y.j_col();
  const double* valuesY = Y.M();
  const index_type* irowptrX = irowptr_;
  const index_type* jcolindX = jcolind_;
  const double* valuesX = values_;
  
#ifdef HIOP_DEEP_CHECKING
  index_type* irowptrM = M.i_row();
  index_type* jcolindM = M.j_col();
#endif
  double* valuesM = M.M();

  
  int nnzM = M.numberOfNonzeros();
  if(gamma==0.0) {
    for(auto i=0; i<nnzM; i++) {
      valuesM[i] = 0.0;
    }
  } else if(gamma!=1.0) {
    int inc = 1;
    DSCAL(&nnzM, &gamma, valuesM, &inc);
  }
  
  // counter for nz in M 
  index_type itnnzM = 0;
  
  for(int i=0; i<nrows_; i++) {
#ifdef HIOP_DEEP_CHECKING
    assert(irowptrM[i] == itnnzM);
#endif    
    // iterate same order as in symbolic function
    // row i of M contains ordered merging of col indexes of row i of X and rowi of Y 

    index_type ptX = irowptrX[i];
    index_type ptY = irowptrY[i];

    // follow sorted merge of the col indexes of X and Y to update values of M
    while(ptX<irowptrX[i+1] && ptY<irowptrY[i+1]) {

      const index_type jX = jcolindX[ptX];
      const index_type jY = jcolindY[ptY];
      assert(jX<ncols_);
      assert(jY<ncols_);

      assert(itnnzM<M.numberOfNonzeros());
      
      if(jX<jY) {
        
#ifdef HIOP_DEEP_CHECKING
        assert(jX==jcolindM[itnnzM]);
#endif        
        valuesM[itnnzM] += alpha*valuesX[ptX];
        ptX++;
      } else {
        if(jX==jY) {
#ifdef HIOP_DEEP_CHECKING
          assert(jX==jcolindM[itnnzM]);
#endif
          valuesM[itnnzM] += alpha*valuesX[ptX] + beta*valuesY[ptY];
          ptX++;
          ptY++;
        } else {
          // jX>jY
#ifdef HIOP_DEEP_CHECKING          
          assert(jY==jcolindM[itnnzM]);
#endif
          valuesM[itnnzM] += beta*valuesY[ptY];
          ptY++;
        }
      }
      itnnzM++;
    } // end of while "sorted merge" iteration 
    assert(ptX==irowptrX[i+1] || ptY==irowptrY[i+1]);

    // iterate over remaining col indexes of (i row of) X
    for(; ptX<irowptrX[i+1]; ++ptX) {
      const index_type jX = jcolindX[ptX];
      assert(jX<ncols_);
#ifdef HIOP_DEEP_CHECKING            
      assert(jX==jcolindM[itnnzM]);
#endif      
      assert(itnnzM<M.numberOfNonzeros());

      valuesM[itnnzM] += alpha*valuesX[ptX];
      itnnzM++;
    }

    // iterate over remaining col indexes of (i row of) X
    for(; ptY<irowptrY[i+1]; ++ptY) {
      const index_type jY = jcolindY[ptY];
      assert(jY<ncols_);
      assert(itnnzM<M.numberOfNonzeros());
#ifdef HIOP_DEEP_CHECKING
      assert(jY==jcolindM[itnnzM]);
#endif
      valuesM[itnnzM] += beta*valuesY[ptY];
      itnnzM++;
    }
  } // end of for over rows
  assert(itnnzM == M.numberOfNonzeros());
}

void hiopMatrixSparseCSR::set_diagonal(const double& val)
{
  assert(irowptr_ && jcolind_ && values_);
  for(index_type i=0; i<nrows_; ++i) {
    for(index_type pt=irowptr_[i]; pt<irowptr_[i+1]; ++pt) {
      if(jcolind_[pt]==i) {
        values_[pt] = val;
        break;
      }
    }
  }
}
} //end of namespace

