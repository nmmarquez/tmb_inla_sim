// Space time 
#include <TMB.hpp>
// Function for detecting NAs
template<class Type>
bool isNA(Type x){
  return R_IsNA(asDouble(x));
}

template<class Type>
Type objective_function<Type>::operator() ()
{
  // Indices
  DATA_INTEGER( n_i );    
  DATA_INTEGER( n_x );    
  DATA_INTEGER( n_t );    
  DATA_INTEGER( n_p );    
  
  // Data
  DATA_IVECTOR( x_s );	   
  DATA_VECTOR( c_i );   
  DATA_VECTOR( Exp_i );
  DATA_IVECTOR( s_i );    
  DATA_IVECTOR( t_i );    
  DATA_MATRIX( X_xp );		
  
  // SPDE objects
  DATA_SPARSE_MATRIX(G0);
  DATA_SPARSE_MATRIX(G1);
  DATA_SPARSE_MATRIX(G2);
  
  // Fixed effects
  PARAMETER_VECTOR(alpha);
  PARAMETER(log_tau_E);  
  PARAMETER(log_kappa);   
  PARAMETER(rho);         
  
  // Random effects
  PARAMETER_ARRAY(epsilon);  
  PARAMETER_ARRAY(sp);  
  
  
  // objective function -- joint negative log-likelihood 
  using namespace density;
  Type jnll = 0;
  vector<Type> jnll_comp(3);
  jnll_comp.setZero();
  
  
  // Spatial parameters
  Type kappa2 = exp(2.0*log_kappa);
  Type kappa4 = kappa2*kappa2;
  Type pi = 3.141592;
  Type Range = sqrt(8) / exp( log_kappa );
  Type SigmaE = 1 / sqrt(4*pi*exp(2*log_tau_E)*exp(2*log_kappa));
  Eigen::SparseMatrix<Type> Q = kappa4*G0 + Type(2.0)*kappa2*G1 + G2;
  
  
  // Objects for derived values
  vector<Type> linear_x(n_x);
  matrix<Type> Epsilon_xt(n_x, n_t);
  
  
  // Probability of Gaussian-Markov random fields (GMRFs)
  jnll_comp(0) += GMRF(Q)(sp);
  jnll_comp(1) += SEPARABLE(AR1(rho),GMRF(Q))(epsilon);
  
  
  // Transform GMRFs
  for(int x=0; x<n_x; x++){
    for( int t=0; t<n_t; t++){
      Epsilon_xt(x,t) = epsilon(x,t) / exp(log_tau_E);
    }
  }
  
  
  // Likelihood contribution from observations
  linear_x = X_xp * alpha.matrix();
  vector<Type> mrprob(n_i);
  for (int i=0; i<n_i; i++){
    mrprob(i) = linear_x(x_s(s_i(i))) + Epsilon_xt(x_s(s_i(i)),t_i(i));
    if( !isNA(c_i(i)) ){
      jnll_comp(2) -= dbinom( c_i(i), Exp_i(i), invlogit(mrprob(i)), true );
    }
  }
  jnll = jnll_comp.sum();
  
  
  // Diagnostics
  // REPORT( jnll_comp );
  // REPORT( jnll );
  REPORT( SigmaE );
  // REPORT( rho );
  ADREPORT( SigmaE );
  // REPORT( Epsilon_xt );
  
  
  return jnll;
}
