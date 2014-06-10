// -*- mode: c++ -*-
//
//  Copyright(C) 2014 Taro Watanabe <taro.watanabe@nict.go.jp>
//

#ifndef __RNNP__OPTIMIZE_ADADELTA__HPP__
#define __RNNP__OPTIMIZE_ADADELTA__HPP__ 1

//
// ADADELTA: AN ADAPTIVE LEARNING RATE METHOD
//

#include <rnnp/optimize.hpp>
#include <rnnp/model_traits.hpp>

#include <utils/mathop.hpp>

namespace rnnp
{
  namespace optimize
  {

    template <typename Theta>
    struct AdaDelta : public Optimize
    {
      typedef typename model_traits<Theta>::model_type    model_impl_type;
      typedef typename model_traits<Theta>::gradient_type gradient_impl_type;
      
      AdaDelta(const Theta& theta,
	       const double& lambda,
	       const double& eta0)
	: G_(theta), X_(theta), lambda_(lambda), eta0_(eta0) { G_.clear(); X_.clear(); }

      double decay()
      {
	eta0_ *= 0.5;
	return eta0_;
      }
      
      void operator()(model_impl_type& theta,
		      const gradient_impl_type& gradient,
		      const option_type& option) const;
      
      static inline
      double learning_rate(const double& eta0, const double& x, const double& g)
      {
	return std::min(1.0, std::sqrt(eta0 + x) / std::sqrt(eta0 + g));
      }

      struct update_visitor_regularize
      {
	update_visitor_regularize(tensor_type& theta,
				  tensor_type& G,
				  tensor_type& X,
				  const tensor_type& g,
				  const double& scale,
				  const double& lambda,
				  const double& eta0)
	  : theta_(theta), G_(G), X_(X), g_(g), scale_(scale), lambda_(lambda), eta0_(eta0) {}
      
	void init(const tensor_type::Scalar& value, tensor_type::Index i, tensor_type::Index j)
	{
	  operator()(value, i, j);
	}
      
	void operator()(const tensor_type::Scalar& value, tensor_type::Index i, tensor_type::Index j)
	{
	  if (g_(i, j) == 0) return;
	
	  G_(i, j) = G_(i, j) * 0.95 + (g_(i, j) * scale_) * (g_(i, j) * scale_);
	
	  const double rate = learning_rate(eta0_, X_(i, j), G_(i, j));
	  const double x1 = theta_(i, j) - rate * scale_ * g_(i, j);
	  const double x2 = utils::mathop::sgn(x1) * std::max(0.0, std::fabs(x1) - rate * lambda_);
	
	  X_(i, j) = X_(i, j) * 0.95 + (x2 - theta_(i, j)) * (x2 - theta_(i, j));
	
	  theta_(i, j) = x2;
	}
      
	tensor_type&       theta_;
	tensor_type&       G_;
	tensor_type&       X_;
	const tensor_type& g_;
      
	const double scale_;
	const double lambda_;
	const double eta0_;
      };

      struct update_visitor
      {
	update_visitor(tensor_type& theta,
		       tensor_type& G,
		       tensor_type& X,
		       const tensor_type& g,
		       const double& scale,
		       const double& eta0)
	  : theta_(theta), G_(G), X_(X), g_(g), scale_(scale), eta0_(eta0) {}
      
	void init(const tensor_type::Scalar& value, tensor_type::Index i, tensor_type::Index j)
	{
	  operator()(value, i, j);
	}
      
	void operator()(const tensor_type::Scalar& value, tensor_type::Index i, tensor_type::Index j)
	{
	  if (g_(i, j) == 0) return;
	
	  G_(i, j) = G_(i, j) * 0.95 + (g_(i, j) * scale_) * (g_(i, j) * scale_);
	  
	  const double rate = learning_rate(eta0_, X_(i, j), G_(i, j));
	  const double x1 = theta_(i, j) - rate * scale_ * g_(i, j);
	  
	  X_(i, j) = X_(i, j) * 0.95 + (x1 - theta_(i, j)) * (x1 - theta_(i, j));
	  
	  theta_(i, j) = x1;
	}
      
	tensor_type&       theta_;
	tensor_type&       G_;
	tensor_type&       X_;
	const tensor_type& g_;
      
	const double scale_;
	const double eta0_;
      };

      void update(tensor_type& theta,
		  tensor_type& G,
		  tensor_type& X,
		  const matrix_embedding_type& grad,
		  const double scale,
		  const bool regularize) const
      {
	if (lambda_ != 0.0) {
	  matrix_embedding_type::const_iterator eiter_end = grad.end();
	  for (matrix_embedding_type::const_iterator eiter = grad.begin(); eiter != eiter_end; ++ eiter) {
	    const size_type col = eiter->first.id();
	    const tensor_type& g = eiter->second;
	    
	    for (tensor_type::Index row = 0; row != eiter->second.rows(); ++ row) 
	      if (g(row, 0) != 0.0) {
		G(row, col) = G(row, col) * 0.95 + (g(row, 0) * scale) * (g(row, 0) * scale);
		
		const double rate = learning_rate(eta0_, X(row, col), G(row, col));
		const double x1 = theta(row, col) - rate * scale * g(row, 0);
		const double x2 = utils::mathop::sgn(x1) * std::max(0.0, std::fabs(x1) - rate * lambda_);
		
		X(row, col) = X(row, col) * 0.95 + (x2 - theta(row, col)) * (x2 - theta(row, col));
		
		theta(row, col) = x2;
	      }
	  }
	} else {
	  matrix_embedding_type::const_iterator eiter_end = grad.end();
	  for (matrix_embedding_type::const_iterator eiter = grad.begin(); eiter != eiter_end; ++ eiter) {
	    const size_type col = eiter->first.id();
	    const tensor_type& g = eiter->second;
	    
	    for (tensor_type::Index row = 0; row != eiter->second.rows(); ++ row) 
	      if (g(row, 0) != 0.0) {
		G(row, col) = G(row, col) * 0.95 + (g(row, 0) * scale) * (g(row, 0) * scale);
		
		const double rate = learning_rate(eta0_,  X(row, col), G(row, col));
		const double x1 = theta(row, col) - rate * scale * g(row, 0);
		
		X(row, col) = X(row, col) * 0.95 + (x1 - theta(row, col)) * (x1 - theta(row, col));
		
		theta(row, col) = x1;
	      }
	  }
	}
      }
      
      void update(tensor_type& theta,
		  tensor_type& G,
		  tensor_type& X,
		  const matrix_category_type& grad,
		  const double scale,
		  const bool regularize) const
      {
	if (regularize && lambda_ != 0.0) {
	  matrix_category_type::const_iterator giter_end = grad.end();
	  for (matrix_category_type::const_iterator giter = grad.begin(); giter != giter_end; ++ giter) {
	    const size_type rows = giter->second.rows();
	    const size_type cols = giter->second.cols();
	    const size_type offset = rows * giter->first.non_terminal_id();
	    
	    const tensor_type& g = giter->second;

	    for (tensor_type::Index col = 0; col != g.cols(); ++ col) 
	      for (tensor_type::Index row = 0; row != g.rows(); ++ row) 
		if (g(row, col) != 0) {
		  G.block(offset, 0, rows, cols)(row, col) =
		    G.block(offset, 0, rows, cols)(row, col) * 0.95 + (g(row, col) * scale) * (g(row, col) * scale);

		  tensor_type::Scalar& x = theta.block(offset, 0, rows, cols)(row, col);
		  
		  const double rate = learning_rate(eta0_,
						    X.block(offset, 0, rows, cols)(row, col),
						    G.block(offset, 0, rows, cols)(row, col));
		  
		  const double x1 = x - rate * scale * g(row, col);
		  const double x2 = utils::mathop::sgn(x1) * std::max(0.0, std::fabs(x1) - rate * lambda_);
		  
		  X.block(offset, 0, rows, cols)(row, col)
		    = X.block(offset, 0, rows, cols)(row, col) * 0.95 + (x2 - x) * (x2 - x);
		  
		  x = x2;
		}
	  }
	} else {
	  matrix_category_type::const_iterator giter_end = grad.end();
	  for (matrix_category_type::const_iterator giter = grad.begin(); giter != giter_end; ++ giter) {
	    const size_type rows = giter->second.rows();
	    const size_type cols = giter->second.cols();
	    const size_type offset = rows * giter->first.non_terminal_id();
	    
	    const tensor_type& g = giter->second;

	    for (tensor_type::Index col = 0; col != g.cols(); ++ col) 
	      for (tensor_type::Index row = 0; row != g.rows(); ++ row) 
		if (g(row, col) != 0) {
		  G.block(offset, 0, rows, cols)(row, col) =
		    G.block(offset, 0, rows, cols)(row, col) * 0.95 + (g(row, col) * scale) * (g(row, col) * scale);

		  tensor_type::Scalar& x = theta.block(offset, 0, rows, cols)(row, col);
		  
		  const double rate = learning_rate(eta0_,
						    X.block(offset, 0, rows, cols)(row, col),
						    G.block(offset, 0, rows, cols)(row, col));
		  const double x1 = x - rate * scale * g(row, col);
		  
		  X.block(offset, 0, rows, cols)(row, col)
		    = X.block(offset, 0, rows, cols)(row, col) * 0.95 + (x1 - x) * (x1 - x);
		  
		  x = x1;
		}
	  }
	}
      }
      
      void update(tensor_type& theta,
		  tensor_type& G,
		  tensor_type& X,
		  const tensor_type& g,
		  const double scale,
		  const bool regularize) const
      {
	if (regularize && lambda_ != 0.0) {
	  update_visitor_regularize visitor(theta, G, X, g, scale, lambda_, eta0_);
	  
	  theta.visit(visitor);
	} else {
	  update_visitor visitor(theta, G, X, g, scale, eta0_);
	  
	  theta.visit(visitor);
	}
      }
    
    private:
      Theta G_;
      Theta X_;
    
      double lambda_;
      double eta0_;
    };

    template <>
    inline
    void AdaDelta<model::Model1>::operator()(model::Model1& theta,
					     const gradient::Model1& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wre_, G.Wre_, X.Wre_, gradient.Wre_, scale, true);
	update(theta.Bre_, G.Bre_, X.Bre_, gradient.Bre_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model2>::operator()(model::Model2& theta,
					     const gradient::Model2& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wre_, G.Wre_, X.Wre_, gradient.Wre_, scale, true);
	update(theta.Bre_, G.Bre_, X.Bre_, gradient.Bre_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model3>::operator()(model::Model3& theta,
					     const gradient::Model3& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wre_, G.Wre_, X.Wre_, gradient.Wre_, scale, true);
	update(theta.Bre_, G.Bre_, X.Bre_, gradient.Bre_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Wqu_, G.Wqu_, X.Wqu_, gradient.Wqu_, scale, true);
	update(theta.Bqu_, G.Bqu_, X.Bqu_, gradient.Bqu_, scale, false);
	update(theta.Bqe_, G.Bqe_, X.Bqe_, gradient.Bqe_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model4>::operator()(model::Model4& theta,
					     const gradient::Model4& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wre_, G.Wre_, X.Wre_, gradient.Wre_, scale, true);
	update(theta.Bre_, G.Bre_, X.Bre_, gradient.Bre_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model5>::operator()(model::Model5& theta,
					     const gradient::Model5& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wre_, G.Wre_, X.Wre_, gradient.Wre_, scale, true);
	update(theta.Bre_, G.Bre_, X.Bre_, gradient.Bre_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Wqu_, G.Wqu_, X.Wqu_, gradient.Wqu_, scale, true);
	update(theta.Bqu_, G.Bqu_, X.Bqu_, gradient.Bqu_, scale, false);
	update(theta.Bqe_, G.Bqe_, X.Bqe_, gradient.Bqe_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model6>::operator()(model::Model6& theta,
					     const gradient::Model6& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);

      if (option.learn_head())
	update(theta.head_, G.head_, X.head_, gradient.head_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wrel_, G.Wrel_, X.Wrel_, gradient.Wrel_, scale, true);
	update(theta.Brel_, G.Brel_, X.Brel_, gradient.Brel_, scale, false);

	update(theta.Wrer_, G.Wrer_, X.Wrer_, gradient.Wrer_, scale, true);
	update(theta.Brer_, G.Brer_, X.Brer_, gradient.Brer_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }

    template <>
    inline
    void AdaDelta<model::Model7>::operator()(model::Model7& theta,
					     const gradient::Model7& gradient,
					     const option_type& option) const
    {
      if (! gradient.count_) return;

      const double scale = 1.0 / gradient.count_;
      
      model_impl_type& G = const_cast<model_impl_type&>(G_);
      model_impl_type& X = const_cast<model_impl_type&>(X_);
      
      if (option.learn_embedding())
	update(theta.terminal_, G.terminal_, X.terminal_, gradient.terminal_, scale, false);
	
      if (option.learn_classification())
	update(theta.Wc_, G.Wc_, X.Wc_, gradient.Wc_, scale, true);
	
      if (option.learn_hidden()) {
	update(theta.Wsh_, G.Wsh_, X.Wsh_, gradient.Wsh_, scale, true);
	update(theta.Bsh_, G.Bsh_, X.Bsh_, gradient.Bsh_, scale, false);
	
	update(theta.Wrel_, G.Wrel_, X.Wrel_, gradient.Wrel_, scale, true);
	update(theta.Brel_, G.Brel_, X.Brel_, gradient.Brel_, scale, false);

	update(theta.Wrer_, G.Wrer_, X.Wrer_, gradient.Wrer_, scale, true);
	update(theta.Brer_, G.Brer_, X.Brer_, gradient.Brer_, scale, false);
	
	update(theta.Wu_, G.Wu_, X.Wu_, gradient.Wu_, scale, true);
	update(theta.Bu_, G.Bu_, X.Bu_, gradient.Bu_, scale, false);

	update(theta.Wf_, G.Wf_, X.Wf_, gradient.Wf_, scale, true);
	update(theta.Bf_, G.Bf_, X.Bf_, gradient.Bf_, scale, false);
	  
	update(theta.Wi_, G.Wi_, X.Wi_, gradient.Wi_, scale, true);
	update(theta.Bi_, G.Bi_, X.Bi_, gradient.Bi_, scale, false);
	
	update(theta.Wqu_, G.Wqu_, X.Wqu_, gradient.Wqu_, scale, true);
	update(theta.Bqu_, G.Bqu_, X.Bqu_, gradient.Bqu_, scale, false);
	update(theta.Bqe_, G.Bqe_, X.Bqe_, gradient.Bqe_, scale, false);
	
	update(theta.Ba_, G.Ba_, X.Ba_, gradient.Ba_, scale, false);
      }
    }
  };
};

#endif
