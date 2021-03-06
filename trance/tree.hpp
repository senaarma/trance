// -*- mode: c++ -*-
//
//  Copyright(C) 2014 Taro Watanabe <taro.watanabe@nict.go.jp>
//

#ifndef __TRANCE__TREE__HPP__
#define __TRANCE__TREE__HPP__ 1

#include <iostream>
#include <vector>
#include <string>
#include <iterator>

#include <trance/symbol.hpp>
#include <trance/sentence.hpp>

#include <utils/hashmurmur3.hpp>
#include <utils/piece.hpp>
#include <utils/simple_vector.hpp>

namespace trance
{
  class Tree
  {
  public:
    typedef size_t    size_type;
    typedef ptrdiff_t difference_type;
    
    typedef trance::Symbol   symbol_type;
    typedef trance::Sentence sentence_type;
    
    typedef Tree tree_type;
    typedef utils::simple_vector<tree_type, std::allocator<tree_type> > antecedent_type;

    typedef antecedent_type::const_iterator const_iterator;
    typedef antecedent_type::iterator       iterator;
    
  public:
    Tree() {}
    Tree(const symbol_type& label) : label_(label), antecedent_() {}
    Tree(const symbol_type& label, const antecedent_type& antecedent) : label_(label), antecedent_(antecedent) {}
    template <typename Iterator>
    Tree(const symbol_type& label, Iterator first, Iterator last) : label_(label), antecedent_(first, last) {}
    Tree(const utils::piece& x) { assign(x); }
    
  public:
    void assign(const utils::piece& x);
    bool assign(std::string::const_iterator& iter, std::string::const_iterator end);
    bool assign(utils::piece::const_iterator& iter, utils::piece::const_iterator end);
    
    void clear()
    {
      label_ = symbol_type();
      antecedent_.clear();
    }

    void swap(Tree& x)
    {
      label_.swap(x.label_);
      antecedent_.swap(x.antecedent_);
    }
    
    std::string string() const;

  public:
    bool terminal() const { return antecedent_.empty(); }
    bool empty() const { return label_.empty() && antecedent_.empty(); }

  public:
    template <typename Iterator>
    void leaf(Iterator iter) const
    {
      if (terminal()) {
	*iter = label_;
	++ iter;
      } else {
	for (const_iterator aiter = begin(); aiter != end(); ++ aiter)
	  aiter->leaf(iter);
      }
    }

    sentence_type leaf() const
    {
      sentence_type sent;
      leaf(std::back_inserter(sent));
      return sent;
    }
    
  public:
    inline const_iterator begin() const { return antecedent_.begin(); }
    inline       iterator begin()       { return antecedent_.begin(); }
    inline const_iterator end() const { return antecedent_.end(); }
    inline       iterator end()       { return antecedent_.end(); }
    
  public:
    friend
    std::ostream& operator<<(std::ostream& os, const Tree& tree);
    friend
    std::istream& operator>>(std::istream& is, Tree& tree);
    
  public:
    symbol_type     label_;
    antecedent_type antecedent_;
  };
  
  namespace impl
  {
    inline
    size_t hash_value(Tree const& x, size_t seed)
    {
      for (Tree::const_iterator aiter = x.begin(); aiter != x.end(); ++ aiter)
	seed = hash_value(*aiter, seed);
      
      return utils::hashmurmur3<size_t>()(x.label_.id(), seed);
    }
  };
  
  inline
  size_t hash_value(Tree const& x)
  {
    return impl::hash_value(x, 0);
  }

  inline
  bool operator==(const Tree& x, const Tree& y)
  {
    return x.label_ == y.label_ && x.antecedent_ == y.antecedent_;
  }

  inline
  bool operator!=(const Tree& x, const Tree& y)
  {
    return x.label_ != y.label_ || x.antecedent_ != y.antecedent_;
  }
  
  inline
  bool operator<(const Tree& x, const Tree& y)
  {
    return (x.label_ < y.label_ || (!(y.label_ < x.label_) && x.antecedent_ < y.antecedent_));
  }

  inline
  bool operator>(const Tree& x, const Tree& y)
  {
    return y < x;
  }

  inline
  bool operator<=(const Tree& x, const Tree& y)
  {
    return ! (y < x);
  }
  
  inline
  bool operator>=(const Tree& x, const Tree& y)
  {
    return ! (x < y);
  }
};

namespace std
{
  inline
  void swap(trance::Tree& x, trance::Tree& y)
  {
    x.swap(y);
  }
};

#endif
