#ifndef INCLUDE_OCTTREE_OCTTREE_HPP
#define INCLUDE_OCTTREE_OCTTREE_HPP
#define OCTTREE_DEFINE_OSTREAM_OPERATORS
#define OCTTREE_DEFINE_VTK_OUTPUT
//#define OCTTREE_DEFINE_TIMERS

#include <array>
#include <algorithm>
#include <functional>
#ifdef max
#undef max 
#endif
#include <limits.h>
#include <stack>

#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>
#include <vector>

#include "thread.hpp"
#include "box.hpp"
#include "functor.hpp"
#include "node.hpp"

namespace OCTree {

template <typename type> struct max {
		type operator() (const type& a,  const type& b) const {
			return std::max(a, b);
		} 
	}; 
	template <typename type> struct min {
		type operator() (const type& a,  const type& b) const {
			return std::min(a, b);
		} 
	};
	template <typename type> 
	std::atomic<type>& max_atomic (std::atomic<type>& a, type& b) {
		type value_a = static_cast<type>(a);
		do {} while (!a.compare_exchange_weak( value_a, std::max(value_a, b))); 
		return a;
	};
		
	//Main class 
	//template < size_t const __K, typename __Val, class __Sync >
	template < size_t const __K, typename __Val, class __Sync = empty_sync_object >
	//template < size_t const __K, typename __Val, class __Sync = spin_lock_sync_object >
		class OCTree {
			private:
				OCTree  (const OCTree&);
				OCTree& operator=(const OCTree&);
			public:
				typedef       size_t                           size_type;
				typedef const size_t                           size_const_type;
				typedef       typename __Val::value_type       value_type;
				typedef const typename __Val::value_type       value_const_type;
				typedef       __Val                            object_type;	  
				typedef const __Val                            object_const_type;	  
				typedef       __Val&                           object_reference;
				typedef const __Val&                           object_const_reference;
				typedef       _Box<__K, value_type>            box_type;
				typedef const _Box<__K, value_type>            box_const_type;
				typedef       _Node<__K, __Val, __Sync>        node_type;
				typedef const _Node<__K, __Val, __Sync>        node_const_type;
				typedef       _Node<__K, __Val, __Sync>*       link_type;
				typedef const _Node<__K, __Val, __Sync>*       link_const_type;
				typedef       std::array<value_type, __K>      query_type;
				typedef const std::array<value_type, __K>      query_const_type;
				typedef __Sync                                 sync_object_type;

				std::atomic<bool>   optimized;
#ifdef OCTTREE_DEFINE_TIMERS
				std::atomic<size_t> min_query_time_find_exact;
				std::atomic<size_t> max_query_time_find_exact;
				std::atomic<size_t> min_query_time_find_nearest;
				std::atomic<size_t> max_query_time_find_nearest;
				std::atomic<size_t> min_query_time_find_nearest_s;
				std::atomic<size_t> max_query_time_find_nearest_s;
				std::atomic<size_t> min_query_time_find_if;
				std::atomic<size_t> max_query_time_find_if;
				size_t optimize_time;
#endif
				OCTree( const box_type& box, const size_t height = 4 ) : optimized(false)
#ifdef OCTTREE_DEFINE_TIMERS
					,min_query_time_find_exact    (std::numeric_limits<size_type>::max()),  max_query_time_find_exact    (0)
					,min_query_time_find_nearest  (std::numeric_limits<size_type>::max()),  max_query_time_find_nearest  (0)
					,min_query_time_find_nearest_s(std::numeric_limits<size_type>::max()),  max_query_time_find_nearest_s(0) 
					,min_query_time_find_if       (std::numeric_limits<size_type>::max()),  max_query_time_find_if       (0) 
					,optimize_time(std::numeric_limits<size_type>::max())
#endif
					,pre_optimize_flag    (false)
					,optimize_flag        (false)
					,post_optimize_flag   (false)
			                ,pre_optimize_barrier ()
					,optimize_barrier     ()
					,post_optimize_barrier()
					,_M_root              (nullptr)
				{ _M_build_tree(box, height);  }

				~OCTree() { delete _M_root; }
				//Inserts __Object in OCTree structure 
				void insert(object_const_reference __Object) {
					optimized = false;
					return _M_insert(_M_get_root(), __Object);
				}
				//Traverses through OCTree structure 
				//Finds the closest leaf node to a query point
				//Returns all objects which are stored in the closest leaf node
				std::vector<object_type> find_exact(query_const_type& point) {
#ifdef OCTTREE_DEFINE_TIMERS
					const std::chrono::high_resolution_clock::time_point start_ = std::chrono::high_resolution_clock::now();
#endif
					link_const_type   _Node   = _M_find_exact(_M_root, point);
					if   (_Node == nullptr ) return std::vector<object_type>();
					else {
#ifdef OCTTREE_DEFINE_TIMERS
						const std::chrono::high_resolution_clock::time_point end_ = std::chrono::high_resolution_clock::now();
						const size_t query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count(); 
						max_query_time_find_exact.store( std::max( max_query_time_find_exact.load(), query_time ), std::memory_order_relaxed);
						min_query_time_find_exact.store( std::min( min_query_time_find_exact.load(), query_time ), std::memory_order_relaxed);
#endif
						return _Node->_M_data;
					}
				};
				//Traverses through OCTree structure 
				//Finds the closest leaf node to a query point
				//Returns all objects which are stored in the closest leaf node
				std::vector<object_type> find_nearest(query_const_type& point, value_const_type radius = std::numeric_limits<double>::max() ) {
					link_const_type   _Node = nullptr;
#ifdef OCTTREE_DEFINE_TIMERS
					const std::chrono::high_resolution_clock::time_point start_ = std::chrono::high_resolution_clock::now();
#endif
					if (radius == 0) _Node = _M_find_exact(_M_root, point);
					else _Node = _M_find_nearest(_M_root, point, radius);
					if   (_Node == nullptr ) return std::vector<object_type>();
					else {
#ifdef OCTTREE_DEFINE_TIMERS
						const std::chrono::high_resolution_clock::time_point end_ = std::chrono::high_resolution_clock::now();
						const size_t query_time =  std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count(); 
						max_query_time_find_nearest.store( std::max( max_query_time_find_nearest.load(), query_time), std::memory_order_relaxed);
						min_query_time_find_nearest.store( std::min( min_query_time_find_nearest.load(), query_time), std::memory_order_relaxed);
#endif
						return _Node->_M_data;  
					}
				};
				//Traverses through OCTree structure 
				//Finds the closest leaf nodes to a query point
				//Returns all objects which are stored in the closest leaf nodes
				std::vector<object_type> find_nearest_s(query_const_type& _M_query_point, value_const_type _M_query_radius = std::numeric_limits<double>::max() ) {
					link_const_type   _Node   = _M_root;
#ifdef OCTTREE_DEFINE_TIMERS
					const std::chrono::high_resolution_clock::time_point start_ = std::chrono::high_resolution_clock::now();
#endif
					std::vector< link_const_type > _Input; _Input.push_back(_Node);
					_Input = _M_find_nearest_s(_Input, _M_query_point, _M_query_radius );
					typename std::vector< link_const_type >::const_iterator it_result;
					typename std::vector< link_const_type >::const_iterator begin_result = _Input.begin();
					typename std::vector< link_const_type >::const_iterator end_result   = _Input.end();
					size_t size = 0;
					for (it_result = begin_result; it_result != end_result; ++it_result) 
						size += (*it_result)->_M_data.size();

					std::vector<object_type> output;
					std::vector<object_type> temp;
					output.reserve( size );
					temp.reserve( size );

					for (it_result = begin_result; it_result != end_result; ++it_result) {
						typename node_type::data_const_iterator it_data;
						typename node_type::data_const_iterator begin_data = (*it_result)->_M_data.begin();
						typename node_type::data_const_iterator end_data   = (*it_result)->_M_data.end();
						if(optimized) {
							std::merge(output.begin(), output.end(), begin_data, end_data, std::back_inserter(temp) );
							std::swap(output, temp);
							temp.clear();
						} else {
							std::copy(begin_data, end_data, std::back_inserter(output) );
						}
					}
#ifdef OCTTREE_DEFINE_TIMERS
					const std::chrono::high_resolution_clock::time_point end_ = std::chrono::high_resolution_clock::now();
					const size_t query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count(); 
					max_query_time_find_nearest_s.store( std::max( max_query_time_find_nearest_s.load(), query_time), std::memory_order_relaxed);
					min_query_time_find_nearest_s.store( std::min( min_query_time_find_nearest_s.load(), query_time), std::memory_order_relaxed);
#endif
					return output;
				};
				//Traverses through OCTree structure
				//Finds all leaf nodes which have intersection an query box
				//Returns all objects which are stored in these leaf nodes
				template <class _Functor>
					std::vector<object_type> find_if (const _Functor& _functor) {
						link_const_type   _Node   = _M_root;
#ifdef OCTTREE_DEFINE_TIMERS
						const std::chrono::high_resolution_clock::time_point start_ = std::chrono::high_resolution_clock::now();
#endif
						box_const_type _box = _Node->_M_box;

						std::vector< link_const_type > _Input; _Input.push_back(_Node);
						_Input = _M_find_if(_Input, _functor );

						typename std::vector< link_const_type >::const_iterator it_result;
						typename std::vector< link_const_type >::const_iterator begin_result = _Input.begin();
						typename std::vector< link_const_type >::const_iterator end_result   = _Input.end();
						std::vector<object_type> output;
						std::vector<object_type> temp;
						for (it_result = begin_result; it_result != end_result; ++it_result) {
							typename node_type::data_const_iterator it_data;
							typename node_type::data_const_iterator begin_data = (*it_result)->_M_data.begin();
							typename node_type::data_const_iterator end_data   = (*it_result)->_M_data.end();
							if (optimized) {
								std::merge(output.begin(), output.end(), begin_data, end_data, std::back_inserter(temp));
								std::swap(output, temp);
								temp.clear();
							} else {
								std::copy(begin_data, end_data, std::back_inserter(output) );
							}
						}
#ifdef OCTTREE_DEFINE_TIMERS
						const std::chrono::high_resolution_clock::time_point end_ = std::chrono::high_resolution_clock::now();
						const size_t query_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end_ - start_).count(); 
						max_query_time_find_if.store( std::max( max_query_time_find_if.load(), query_time));
						min_query_time_find_if.store( std::min( min_query_time_find_if.load(), query_time));
#endif
						return output;
					}
		
				std::atomic<bool>              pre_optimize_flag;
				std::atomic<bool>                  optimize_flag;
				std::atomic<bool>             post_optimize_flag;

				barrier<sync_object_type>   pre_optimize_barrier;
				barrier<sync_object_type>       optimize_barrier;
				barrier<sync_object_type>  post_optimize_barrier;

				//Optimize OCTree structure
				void optimize() {
#ifdef OCTTREE_DEFINE_TIMERS
					auto start = std::chrono::high_resolution_clock::now();
#endif
					pre_optimize_barrier .lock();
					optimize_barrier     .lock();
					post_optimize_barrier.lock();
					//Pre-optimize
					if( !pre_optimize_flag ) {
						_M_pre_optimize ( _M_get_root() );
						pre_optimize_flag = true;
					}
					pre_optimize_barrier.unlock();
					//Optimize
					if( !optimize_flag ) { 
						_M_optimize     ( _M_get_root() );
						optimize_flag = true;
					}
					optimize_barrier.unlock();
					//Post-optimize
					if( !post_optimize_flag ) {
						_M_post_optimize( _M_get_root() );
						post_optimize_flag = true;
					}
					post_optimize_barrier.unlock();
#ifdef OCTTREE_DEFINE_TIMERS
					auto end      = std::chrono::high_resolution_clock::now();
					auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
					optimize_time = std::min(optimize_time, static_cast<size_t>(duration));
#endif
					//Completed optimization
					optimized = true;
				};
				bool empty() const {
					bool flag =  _M_root == nullptr ||  _M_empty_branch( _M_root); 
					return flag;
				};
				size_type size() const {
					return _M_size( _M_get_root() );
				}

				static size_type all            (link_const_type _Node) { return true                     ? 1 : 0; }
				static size_type leaf_node      (link_const_type _Node) { return _Node->isLeafNode()      ? 1 : 0; }
				static size_type empty_leaf_node(link_const_type _Node) { return _Node->isEmptyLeafNode() ? 1 : 0; }
				static size_type internal_node  (link_const_type _Node) { return _Node->isInternalNode () ? 1 : 0; }

				static size_type size_of_data   (link_const_type _Node) { return                  _Node->_M_data.capacity()*sizeof(object_type);}
				static size_type size_of_tree   (link_const_type _Node) { return sizeof(*_Node);                                               }
				static size_type size_of_all    (link_const_type _Node) { return sizeof(*_Node) + _Node->_M_data.capacity()*sizeof(object_type);}

				static size_type min_data_size      (link_const_type _Node) { return _Node->isLeafNode() ? _Node->_M_data.size() : std::numeric_limits<size_type>::max(); }
				static size_type max_data_size      (link_const_type _Node) { return _Node->_M_data.size(); }

				template <class Operator = std::plus<size_type>, class Functor>
					size_type size_if(Functor func) const {
						return _M_size_if<Operator>(_M_get_root(), func);
					};

				size_type max_height() const {
					return _M_max_height( _M_get_root() );
				}
				size_type min_height() const {
					return _M_min_height( _M_get_root() );
				}
			private:
				link_const_type              _M_get_root() const { return const_cast<link_const_type>(_M_root); }
				link_type                    _M_get_root()       { return _M_root; }
				size_type                    _M_height(link_const_type _Input) const     { 
					size_type height = 1;
					if (_Input->_M_parent == nullptr) 
						return height;
					else 
						return height + _M_height( _Input->_M_parent ); 
				}
				size_type                    _M_max_height(link_const_type _Input) const     { 
					bool  _Input_isLeafNode = _Input->isLeafNode();
					size_type height = 1;
					if(!_Input_isLeafNode) {
						typename node_type::node_const_iterator it_node;
						typename node_type::node_const_iterator begin_node = _Input->_M_child.begin();
						typename node_type::node_const_iterator end_node   = _Input->_M_child.end();
						size_type temp = 0;
						for(it_node = begin_node; it_node != end_node; ++it_node) temp = std::max(_M_max_height(*it_node), temp);
						height += temp;
					}
					return height;		
				}
				size_type                    _M_min_height(link_const_type _Input) const     { 
					bool  _Input_isLeafNode = _Input->isLeafNode();
					size_type height = 1;
					if(!_Input_isLeafNode) {
						typename node_type::node_const_iterator it_node;
						typename node_type::node_const_iterator begin_node = _Input->_M_child.begin();
						typename node_type::node_const_iterator end_node   = _Input->_M_child.end();
						size_type temp = std::numeric_limits<size_type>::max();
						for(it_node = begin_node; it_node != end_node; ++it_node) temp = std::min(_M_min_height(*it_node), temp);
						height += temp;
					}
					return height;		
				}
				size_type                    _M_size(link_const_type _Input) const     { 
					bool  _Input_isLeafNode = _Input->isLeafNode();
					size_type size = 1;
					if(!_Input_isLeafNode) {
						typename node_type::node_const_iterator it_node;
						typename node_type::node_const_iterator begin_node = _Input->_M_child.begin();
						typename node_type::node_const_iterator end_node   = _Input->_M_child.end();
						for(it_node = begin_node; it_node != end_node; ++it_node) size += _M_size(*it_node);
					}
					return size;		
				}
				template <class Operator, class Functor>
					size_type                    _M_size_if(link_const_type _Input,  Functor func) const     { 
						Operator op;
						size_type size = func(_Input);
						bool  _Input_isLeafNode = _Input->isLeafNode();
						if(!_Input_isLeafNode) {
							typename node_type::node_const_iterator it_node;
							typename node_type::node_const_iterator begin_node = _Input->_M_child.begin();
							typename node_type::node_const_iterator end_node   = _Input->_M_child.end();
							//for(it_node = begin_node; it_node != end_node; ++it_node) size += _M_size_if(*it_node, func);
							for(it_node = begin_node; it_node != end_node; ++it_node) size = op(size, _M_size_if<Operator>(*it_node, func));
						}
						return size;		
					}

				std::vector<link_const_type> _M_find_nearest_s(const std::vector<link_const_type>& _Input, query_const_type& _M_query_point, value_const_type& _M_input_radius) {
					typename std::vector<link_const_type>::const_iterator it_input;
					typename std::vector<link_const_type>::const_iterator begin_input = _Input.begin();
					typename std::vector<link_const_type>::const_iterator end_input   = _Input.end();

					std::vector<link_const_type>    _Output;
					bool  allOutputNodesAreLeafNodes = true;
					value_type _M_output_radius = std::numeric_limits<double>::max();

					for( it_input = begin_input; it_input != end_input; it_input++ ) {
						const box_type& _Node_box = (*it_input)->_M_box;
						//bool                 isLeafNode = (*it_input)->isLeafNode();
						bool                isEmptyNode = _M_empty_branch(*it_input);
						value_type         max_distance = _Node_box.longest_distance(_M_query_point) ;  
						if (!isEmptyNode ) _M_output_radius = std::min(_M_output_radius, max_distance );			  
					}
					_M_output_radius = std::min(_M_output_radius, _M_input_radius);

					for( it_input = begin_input; it_input != end_input; it_input++ ) {
						const box_type& _Input_box = (*it_input)->_M_box;
						bool  _Input_isLeafNode          = (*it_input)->isLeafNode();
						bool  _Input_isEmptyNode         = _M_empty_branch(*it_input); 

						if ( !_Input_isEmptyNode  ) {
							_Sphere<__K, value_type> _box;
							_box._M_center  = _M_query_point;
							_box._M_radius2 = _M_output_radius;
							//bool allPredicatesTrue = _Input_box.intersects_with(_box);
							bool allPredicatesTrue = _box.intersects_with(_Input_box);
							if(allPredicatesTrue) {
								if ( _Input_isLeafNode ) {
									_Output.push_back(*it_input);
								} else {
									typename node_type::node_const_iterator it_node;
									typename node_type::node_const_iterator begin_node = (*it_input)->_M_child.begin();
									typename node_type::node_const_iterator end_node   = (*it_input)->_M_child.end();
									allOutputNodesAreLeafNodes = false;
									_Output.insert( _Output.end(), begin_node, end_node );
								}
							}
						} 
					}
					if (allOutputNodesAreLeafNodes) 
						return _Output;
					else 
						return _M_find_nearest_s(_Output, _M_query_point, _M_output_radius );
				}
				//Traverse through OCTree structure by recursion calls of itself
				//Checks that an OCTree node is intersected with all predicates
				//Returns leaf nodes
				template<class Functor> 
					std::vector<link_const_type> _M_find_if(const  std::vector<link_const_type>& _Input, const Functor& functor) {
						typename std::vector<link_const_type>::const_iterator it_input;
						typename std::vector<link_const_type>::const_iterator begin_input = _Input.begin();
						typename std::vector<link_const_type>::const_iterator end_input   = _Input.end();

						std::vector<link_const_type>    _Output;
						bool  allOutputNodesAreLeafNodes = true;

						for( it_input = begin_input; it_input != end_input; it_input++ ) {
							bool  _Input_isLeafNode    = (*it_input)->isLeafNode();
							bool  _Input_isEmptyNode   = _M_empty_branch(*it_input); 

							if ( !_Input_isEmptyNode  ) {
								//Check input predicates
								bool allFunctorsTrue = functor( *(*it_input) ); 
								//If input predicates and the box of the current node intersect we will store child nodes
								if(allFunctorsTrue) {
									if ( _Input_isLeafNode ) {
										_Output.push_back(*it_input);
									} else {
										typename node_type::node_const_iterator it_node;
										typename node_type::node_const_iterator begin_node = (*it_input)->_M_child.begin();
										typename node_type::node_const_iterator end_node   = (*it_input)->_M_child.end();
										allOutputNodesAreLeafNodes = false;
										_Output.insert( _Output.end(), begin_node, end_node );
									}
								}
							} 
						}
						if (allOutputNodesAreLeafNodes) 
							return _Output;
						else 
							return _M_find_if( _Output, functor);
					}
				//Traverse through OCTree structure by recursion calls of itself
				//Checks that an query point is inside of an OCTree node 
				//Returns leaf node
				link_const_type _M_find_nearest(link_const_type _Node, query_const_type& point, value_const_type& radius) {
					link_const_type _ClosestNode = nullptr;
					value_type   shortest_radius = std::numeric_limits<value_type>::max();
					typename node_const_type::node_const_iterator it_node;
					typename node_const_type::node_const_iterator begin_node = _Node->_M_child.begin();
					typename node_const_type::node_const_iterator end_node   = _Node->_M_child.end();
					for ( it_node = begin_node; it_node != end_node; it_node++ ) {
						if(!_M_empty_branch(*it_node)) {
							value_type temp = (*it_node)->_M_box.shortest_distance(point);
							if(temp < shortest_radius) {
								shortest_radius = temp;
								_ClosestNode = (*it_node);
							}
						}
					}
					if( shortest_radius < radius ) 
						if(_ClosestNode->isLeafNode() ) {
							_M_empty_branch(_ClosestNode);
							return _ClosestNode;
						}
						else return _M_find_nearest( _ClosestNode, point, radius);
					else return nullptr;
				}
				//Traverse through OCTree structure by recursion calls of itself
				//Checks that an query point is inside of an OCTree node 
				//Returns leaf node
				link_const_type _M_find_exact(link_const_type _Node, query_const_type& point) {
					typename node_const_type::node_const_iterator it_node;
					typename node_const_type::node_const_iterator begin_node = _Node->_M_child.begin();
					typename node_const_type::node_const_iterator end_node   = _Node->_M_child.end();
					for ( it_node = begin_node; it_node != end_node; it_node++ ) {
						if(!_M_empty_branch(*it_node)) {
							if( (*it_node)->_M_box.is_inside(point) ) {
								if( (*it_node)->isLeafNode() ) return                  (*it_node);
								else                           return _M_find_exact( (*it_node), point);
							}
						}
					}
					return nullptr;
				}
				//Optimizes OCTree structure
				void _M_pre_optimize ( link_type _Node ) {
						size_type        threshold      =  50;
						size_type        maximal_height =  10;
						auto             expected = node_type::STATE::M_DEFAULT;
						auto			 val      = node_type::STATE::M_NO_ACTION;

						if ( _Node->isLeafNode() ) {
							if ( _Node->_M_state.compare_exchange_strong( expected, val ) ) {
								size_type        currentSize = _Node->_M_data.size();
								if( currentSize > threshold && _M_height(_Node) < maximal_height ) 
									_Node->_M_state = node_type::STATE::M_SPLIT_NODE;
								if(currentSize == 0) { 
									_Node->_M_state = node_type::STATE::M_EMPTY_NODE;
								}
							}
						} else {
							for (auto it_node = _Node->_M_child.begin(); it_node != _Node->_M_child.end(); ++it_node ) {
								_M_pre_optimize(*it_node);
							}
							bool  clear_branch_flag = true;
							for (auto it_node = _Node->_M_child.begin(); it_node != _Node->_M_child.end(); ++it_node ) 
					  	 	 	clear_branch_flag &= 
									(*it_node)->_M_state == node_type::STATE::M_CLEAR_BRANCH
									||
									(*it_node)->_M_state == node_type::STATE::M_EMPTY_NODE;
							if( clear_branch_flag ) {
								_Node->_M_state = node_type::STATE::M_CLEAR_BRANCH;
							}
						}
					return;
				}
				void _M_optimize     ( link_type _Node  ) {
						if ( _Node->_M_state == node_type::STATE::M_DEFAULT ) {
							for (auto it_node = _Node->_M_child.begin(); it_node != _Node->_M_child.end(); ++it_node )
								_M_optimize(*it_node);
						} else {
							auto state = _Node->_M_state.exchange( node_type::STATE::M_NO_ACTION );
							//Clear branch
							if ( state == node_type::STATE::M_CLEAR_BRANCH ) {
								for (auto it_node = _Node->_M_child.begin(); it_node != _Node->_M_child.end(); ++it_node ) {
									_M_remove_node(*it_node);
									*it_node = nullptr;
								}
							}
						 	//Split leaf node	
							if ( state == node_type::STATE::M_SPLIT_NODE ) {
								size_type        threshold      =  50;
								size_type        maximal_height =  10;
								value_const_type factor = std::sqrt(static_cast<value_type>(power<__K>::result));
								size_type        currentSize = _Node->_M_data.size();
								//Create child nodes		  
								_Node->_M_child = _M_create_nodes( _Node );
								for (auto it_data = _Node->_M_data.begin(); it_data != _Node->_M_data.end(); ++it_data)
									_M_insert(_Node, (*it_data) );
								//Clear node
								_Node->clearData();
								for (auto it_child = _Node->_M_child.begin(); it_child != _Node->_M_child.end(); ++it_child) {
									const size_t newSize = (*it_child)->_M_data.size();
									if(currentSize > factor*newSize &&  newSize > threshold && _M_height(*it_child) < maximal_height ) 
							    		(*it_child)->_M_state = node_type::STATE::M_SPLIT_NODE;	
									else
										(*it_child)->_M_state = node_type::STATE::M_NO_ACTION;
									_M_optimize(*it_child);
								}
							}
						}
					return;
				}
				void _M_post_optimize( link_type _Node ) {
					if (_Node->isLeafNode()) {
						auto state = _Node->_M_state.exchange(node_type::STATE::M_DEFAULT);
						if (state == node_type::STATE::M_NO_ACTION)
							_Node->sortData();
					} else
						for (auto it_node = _Node->_M_child.begin(); it_node != _Node->_M_child.end(); ++it_node )
							_M_post_optimize(*it_node);
					return;
				}
				//Checks that a branch is empty or not
				bool _M_empty_branch( link_const_type _Node ) const {
					if(_Node->isLeafNode()) {
						return _Node->_M_data.empty();
					} else {
						if ( optimized ) {
							for (auto it = _Node->_M_child.begin(); it != _Node->_M_child.end(); ++it ) 
								if( (*it) != nullptr ) return false;
						}
						else {
							for (auto it = _Node->_M_child.begin(); it != _Node->_M_child.end(); ++it ) 
								if( !_M_empty_branch(*it) ) return false;
						}
						return true;	
					}
				}
				//Traverse through OCTree structure by recursion calls of itself
				//Inserts an object in a leaf OCTree node 
				void _M_insert(link_type __N, object_const_reference __Object) {
					if (__Object( __N->_M_box ) ) {
						if ( __N->isLeafNode() ) {
							__N->Insert(__Object);
						} else
							for ( auto it_node = __N->_M_child.begin(); it_node != __N->_M_child.end(); ++it_node ) _M_insert((*it_node), __Object);
					} 
					return;
				}
				//Builds OCTree structure	  
				void _M_build_tree(const box_type& box, size_t height){
					_M_root = new _Node< __K, __Val, __Sync>();
					_M_root->_M_box = box;
					if ( height > 0 ) _M_root->_M_child = _M_create_nodes(_M_root, height);
					return;
				}
				//Create additional nodes
				std::array< link_type, power<__K>::result>  _M_create_nodes(link_type parent, size_t height = 1) {
					std::array<link_type, power<__K>::result>            result;

					typename cartesian_product<__K>::const_iterator it;
					typename cartesian_product<__K>::const_iterator begin = cartesian_product<__K>::product.begin();
					typename cartesian_product<__K>::const_iterator end   = cartesian_product<__K>::product.end();
					size_t index;
					const box_type& box = parent->_M_box; 
					//Iteration over an nodes of the parent node
					for ( index = 0, it = begin; it != end; ++it, ++index ) {
						box_type _box;
						for ( size_t dim = 0; dim < __K; dim++ ) {
							_box._M_low_bounds[dim]  = std::min( ((1 - (*it)[dim])*box._M_low_bounds[dim] + (1 +(*it)[dim])*box._M_high_bounds[dim])/2,
																	(box._M_low_bounds[dim] + box._M_high_bounds[dim])/2 );
							_box._M_high_bounds[dim] = std::max( ((1 - (*it)[dim])*box._M_low_bounds[dim] + (1 +(*it)[dim])*box._M_high_bounds[dim])/2, 
																	(box._M_low_bounds[dim] + box._M_high_bounds[dim])/2 );
						}
						result[index] = new _Node< __K, __Val,  __Sync>();
						result[index]->_M_parent = parent;
						result[index]->_M_box = _box;
					}
					if ( --height > 0 ) {
						typename std::array<link_type, power<__K>::result>::iterator it;
						typename std::array<link_type, power<__K>::result>::iterator begin = result.begin();
						typename std::array<link_type, power<__K>::result>::iterator end   = result.end();
						for ( it = begin; it != end; ++it ) 
							(*it)->_M_child = _M_create_nodes( (*it),height);
					}
					return result;
				}
				//Removes a node
				std::vector<__Val> _M_remove_node(link_type _M_node) {
					std::vector<__Val> result;
					if(_M_node != nullptr) result = _M_node->_M_data;
					delete _M_node;
					return result;
				}
				link_type   _M_root;
#ifdef OCTTREE_DEFINE_OSTREAM_OPERATORS
				friend std::ostream& operator<<(std::ostream& o, OCTree<__K, __Val, __Sync> const& tree) {
					typedef OCTree<__K, __Val, __Sync> _Tree;
						if (tree.empty()) return o << "[empty " << "OCTree " << &tree << "]";
						o << "dimensions                  : " << __K                                                  << std::endl;
						o << "minimum height              : " << tree.min_height()                                    << std::endl;
						o << "maximum height              : " << tree.max_height()                                    << std::endl;
						o << "total number of nodes       : " << tree.size()                                          << std::endl;
						o << "number of leaf nodes        : " << tree.size_if<>              (_Tree::leaf_node      ) << std::endl;
						o << "number of internal nodes    : " << tree.size_if<>              (_Tree::internal_node  ) << std::endl;
						o << "number of empty leaf nodes  : " << tree.size_if<>              (_Tree::empty_leaf_node) << std::endl;
						o << "maximum number of objects   : " << tree.size_if<max<size_type>>(_Tree::max_data_size  ) << std::endl;
						o << "minimum number of objects   : " << tree.size_if<min<size_type>>(_Tree::min_data_size  ) << std::endl;
						o << "memory usage in bytes       : " << tree.size_if<>              (_Tree::size_of_all    ) << std::endl;
#ifdef OCTTREE_DEFINE_TIMERS
						o << "query_time (find_exact    ) : " << tree.min_query_time_find_exact    << " " << tree.max_query_time_find_exact     << " nsec" << std::endl;
						o << "query_time (find_nearest  ) : " << tree.min_query_time_find_nearest  << " " << tree.max_query_time_find_nearest   << " nsec" << std::endl;
						o << "query_time (find_nearest_s) : " << tree.min_query_time_find_nearest_s<< " " << tree.max_query_time_find_nearest_s << " nsec" << std::endl;
						o << "query_time (find_if       ) : " << tree.min_query_time_find_if       << " " << tree.max_query_time_find_if        << " nsec" << std::endl;
						o << "optimize_time               : " << tree.optimize_time                << " " << tree.max_query_time_find_if        << " nsec" << std::endl;
#endif 
					return o;
				}
#endif
#ifdef OCTTREE_DEFINE_VTK_OUTPUT
			public:
				template<class __Functor = _TrueFunctor>
				bool dump(const std::string& filename, const __Functor& functor = _TrueFunctor() ) const {
						typedef OCTree<__K, __Val, __Sync> _Tree;
						typedef _Tree::link_const_type   link_const_type;
						typedef _Tree::box_const_type box_const_type;

						std::vector<link_const_type> parent_;
						std::vector<link_const_type> child_;
						parent_.push_back(_M_get_root());

						std::stringstream vtm_filename; vtm_filename << filename << ".vtm";
						std::ofstream vtm_file( vtm_filename.str() );
						vtm_file << "<VTKFile type=\"vtkMultiBlockDataSet\" version=\"1.0\" byte_order=\"LittleEndian\">" << std::endl;
						vtm_file << "<vtkMultiBlockDataSet>" << std::endl;
						vtm_file << "<Block index=\"" << 0 << "\" name=\"meshes\">" << std::endl;

						size_t level = 0;
						while (!parent_.empty()) {
							size_t number_of_cells = parent_.size();
							size_t number_of_nodes = power<__K>::result * number_of_cells;

							std::vector<double>  coords; 
							std::vector<int>      nodes; 
							std::vector<int>    offsets; 
							std::vector<int>      types;
							std::vector<int>  cell_data;

							int index_node   = 0;
							int index_offset = 0;
							int type         = __K == 3 ? 12 : __K == 2 ? 9 : 3; 

							for( auto it_node = parent_.begin();  it_node != parent_.end(); it_node++ ) {
								if(functor( *(*it_node)) ) {
									link_const_type node     = (*it_node);
									box_const_type box = node->_M_box;
									cell_data.push_back(_M_size_if<std::plus<size_type> >(*it_node, size_of_data));
									//cell_data.push_back( (*it_node)->_M_data.size());
									
									offsets.push_back( power<__K>::result*(++index_offset) );
									types  .push_back(type);
									for(auto it = cartesian_product<__K>::product.begin(); it != cartesian_product<__K>::product.end(); it++) {
										nodes  .push_back( index_node++ );
										for ( size_t dim = 0; dim < __K; dim++ ) {
											const double value = (1 - (*it)[dim])/2*box._M_low_bounds[dim] + (1 + (*it)[dim])/2*box._M_high_bounds[dim];
											coords.push_back(value);
										}
									}
								}
							}

							double double_min_, double_max_;
							int       int_min_,    int_max_;

							std::stringstream vtu_filename; vtu_filename << "level-" << level << ".vtu";
							std::ofstream vtu_file( vtu_filename.str().c_str() );

							vtm_file << "<DataSet index=\"" << level << "\" name=\"" << vtu_filename.str() << "\" file=\"" << vtu_filename.str() <<"\">"<< std::endl;

							vtu_file << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">" << std::endl;
							vtu_file << "<UnstructuredGrid>" << std::endl;
							vtu_file << "<Piece NumberOfPoints=\"" <<  number_of_nodes << "\" NumberOfCells=\"" << number_of_cells << "\">" << std::endl;
							vtu_file << "<PointData>"  << std::endl;
							vtu_file << "</PointData>" << std::endl;
							vtu_file << "<CellData>"   << std::endl;

							int_min_ = 0;
							int_max_ = 0; 
							vtu_file << "<DataArray type=\"Float64\" Name=\"size\" format=\"ascii\"";
							vtu_file << " RangeMin=\"" << int_min_ << "\" RangeMax=\"" << int_max_ << "\">" << std::endl;
							for( auto it_node = parent_.begin();  it_node != parent_.end(); it_node++ ) {
								vtu_file << (*it_node)->_M_data.size() << " ";
							}
							vtu_file << std::endl << "</DataArray>" << std::endl;

							vtu_file << "</CellData>"  << std::endl;
							vtu_file << "<Points>"     << std::endl;

							vtu_file << "<DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"" << __K <<"\" format=\"ascii\"" ;
							double_min_ = (*std::min_element(coords.begin(), coords.end()));
							double_max_ = (*std::max_element(coords.begin(), coords.end()));
							vtu_file << " RangeMin=\"" << double_min_ << "\" RangeMax=\"" << double_max_ << "\">" << std::endl;
							for(auto it = coords.begin(); it != coords.end(); it++ ) vtu_file << (*it) << " ";
							vtu_file << "</DataArray>" << std::endl;

							vtu_file << "</Points>"    << std::endl;
							vtu_file << "<Cells>"      << std::endl;

							int_min_ = (*std::min_element(nodes.begin(), nodes.end()));
							int_max_ = (*std::max_element(nodes.begin(), nodes.end()));
							vtu_file << "<DataArray type=\"Int64\" Name=\"connectivity\" format=\"ascii\"";
							vtu_file << " RangeMin=\"" << int_min_ << "\" RangeMax=\"" << int_max_ << "\">" << std::endl;
							for(auto it = nodes.begin(); it != nodes.end(); it++ ) vtu_file << (*it) << " ";
							vtu_file << "</DataArray>" << std::endl;

							int_min_ = (*std::min_element(offsets.begin(), offsets.end()));
							int_max_ = (*std::max_element(offsets.begin(), offsets.end()));
							vtu_file << "<DataArray type=\"Int64\" Name=\"offsets\" format=\"ascii\"";
							vtu_file << " RangeMin=\"" << int_min_ << "\" RangeMax=\"" << int_max_ << "\">" << std::endl;
							for(auto it = offsets.begin(); it != offsets.end(); it++ ) vtu_file << (*it) << " ";
							vtu_file << "</DataArray>" << std::endl;

							int_min_ = (*std::min_element(types.begin(), types.end()));
							int_max_ = (*std::max_element(types.begin(), types.end()));
							vtu_file << "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\"";
							vtu_file << " RangeMin=\"" << int_min_ << "\" RangeMax=\"" << int_max_ << "\">" << std::endl;
							for(auto it = types.begin(); it != types.end(); it++ ) vtu_file << (*it) << " ";
							vtu_file << "</DataArray>" << std::endl;

							vtu_file << "</Cells>" << std::endl;
							vtu_file << "</Piece>" << std::endl;
							vtu_file << "</UnstructuredGrid>" << std::endl;
							vtu_file << "</VTKFile>" << std::endl;
							for( auto it_node = parent_.begin();  it_node != parent_.end(); it_node++ ) {
								link_const_type node     = (*it_node);
								box_const_type box = node->_M_box;
								for (auto it_child = node->_M_child.begin(); it_child != node->_M_child.end(); it_child++ ) 
									if( (*it_child) != nullptr )
										child_.push_back(*it_child);
							}
							parent_.clear();
							std::swap(parent_, child_);
							vtu_file.close();
							level++;
							vtm_file << "</DataSet>" << std::endl;
						}
						vtm_file << "</Block>" << std::endl;
						vtm_file << "</vtkMultiBlockDataSet>" << std::endl;
						vtm_file << "</VTKFile>" << std::endl;
						vtm_file.close();
					return true;
				}
#endif
		};
}
#endif //INCLUDE_OCTTREE_OCTTREE_HPP
