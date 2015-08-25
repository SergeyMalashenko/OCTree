#ifndef INCLUDE_OCTTREE_NODE_HPP
#define INCLUDE_OCTTREE_NODE_HPP

#include <array>
#include <algorithm>
#include <iostream>
#include <vector>

#include "thread.hpp"

namespace OCTree {
	template<size_t i> struct power{ static const size_t result = 2 * power<i-1>::result; };
	template<> struct power<1> { static const size_t result = 2; };
	template<size_t D> struct cartesian_product { 
		static const std::array<std::array<int,D>, power<D>::result > product;
		typedef typename std::array<std::array<int,D>, power<D>::result >::iterator iterator;
		typedef typename std::array<std::array<int,D>, power<D>::result >::const_iterator const_iterator;
	};
	
	template <size_t __K, typename __Val, class __Sync>
		struct _Node {
		    typedef __Val                                                  			        object_type;
		    typedef const __Val                                                       object_const_type;
		    typedef __Val&                                                             object_reference;
		    typedef const __Val&                                                 object_const_reference;
			typedef typename __Val::value_type                                               value_type;
			typedef __Sync                                                             sync_object_type;
			typedef typename std::array<_Node*, power<__K>::result>::iterator             node_iterator;
			typedef typename std::array<_Node*, power<__K>::result>::const_iterator node_const_iterator;
			typedef typename std::vector<object_type>::iterator                           data_iterator;
			typedef typename std::vector<object_type>::const_iterator               data_const_iterator;
		 	typedef typename std::array<value_type, __K>                                     query_type; 
			//enum class STATE { M_DEFAULT, M_NO_ACTION, M_SPLIT_NODE, M_CLEAR_BRANCH, M_EMPTY_NODE };
			//std::atomic<STATE> _M_state; 
			struct STATE { 
				static const int M_DEFAULT      = 0;
				static const int M_NO_ACTION    = 1;
				static const int M_SPLIT_NODE   = 2;
				static const int M_CLEAR_BRANCH = 3;
				static const int M_EMPTY_NODE   = 4;
			};
			std::atomic<int> _M_state; 

			mutable sync_object_type                    _M_mutex;

			_Node*                                      _M_parent;                                
			std::array<_Node*, power<__K>::result> 		_M_child;
			std::vector<__Val>                          _M_data;
			_Box<__K, value_type>                       _M_box;
		private:
				_Node(const _Node&);
				_Node& operator=(const _Node&);
		public:
			_Node() : _M_state(),  _M_mutex(), _M_parent(), _M_child(), _M_data(), _M_box()  {
				_M_state  = STATE::M_DEFAULT;
				_M_parent = nullptr;
				std::fill( _M_child.begin(), _M_child.end(), nullptr);
			}
			~_Node() {
				std::unique_lock<sync_object_type>  lock( _M_mutex );
				for (node_iterator it_node = _M_child.begin(); it_node != _M_child.end(); it_node++ ) 
					delete *it_node;
				std::fill( _M_child.begin(), _M_child.end(), nullptr);
			}
			inline void Insert(object_const_reference __Object) {
				std::unique_lock<sync_object_type> lock( _M_mutex );
				_M_data.push_back(__Object);
				return;		
			}
			inline void sortData() {
				std::unique_lock<sync_object_type> lock(_M_mutex);
				std::sort(_M_data.begin(), _M_data.end());
				return;
			}
			inline void clearData() {
				std::unique_lock<sync_object_type> lock(_M_mutex);
				_M_data.clear();
				//std::vector<object_type>().swap(_Node->_M_data);
				return;
			}
			//Check that the node is a root node
			inline bool isRootNode     () const { 
				std::unique_lock<sync_object_type> lock( _M_mutex );
				return _M_parent   == nullptr;          
			}
			//Check that the node is a internal node
			inline bool isInternalNode () const { 
				std::unique_lock<sync_object_type> lock( _M_mutex );
				bool flag_root_node = ( _M_parent == nullptr );
				bool flag_leaf_node = true;
				node_const_iterator it_node;
				node_const_iterator begin_node = _M_child.begin();
				node_const_iterator end_node   = _M_child.end();
				for ( it_node = begin_node; it_node != end_node && flag_leaf_node ; ++it_node ) 
					flag_leaf_node &= (*it_node) == nullptr;
				return !flag_root_node && !flag_leaf_node;
				//return !isRootNode() && !isLeafNode();  
			}
			//Check that the node is an empty leaf node
			inline bool isEmptyLeafNode() const { 
				std::unique_lock<sync_object_type> lock( _M_mutex );
				bool flag_leaf_node = true;
				node_const_iterator it_node;
				node_const_iterator begin_node = _M_child.begin();
				node_const_iterator end_node   = _M_child.end();
				for ( it_node = begin_node; it_node != end_node && flag_leaf_node ; ++it_node ) 
					flag_leaf_node &= (*it_node) == nullptr;
				return flag_leaf_node && _M_data.empty();
				//return isLeafNode() && _M_data.empty(); 
			}
			//Check that the node is a leaf node
			inline bool isLeafNode() const {
				std::unique_lock<sync_object_type>  lock( _M_mutex );
				bool flag = true;
				node_const_iterator it_node;
				node_const_iterator begin_node = _M_child.begin();
				node_const_iterator end_node   = _M_child.end();
				for ( it_node = begin_node; it_node != end_node && flag ; ++it_node ) 
					flag &= (*it_node) == nullptr;
				return flag;
			}
#ifdef OCTTREE_DEFINE_OSTREAM_OPERATORS
			template <typename Char, typename Traits>
				friend
				std::basic_ostream<Char, Traits>& 
				operator<<(typename std::basic_ostream<Char, Traits>& out,
						   _Node const& node)
				{
					std::unique_lock<sync_object_type>  lock( node._M_mutex );
					out << &node;
					out << " parent: " << node._M_parent;
					out << "; childs: ";
					for(auto node_it = node._M_child.begin(); node_it != node._M_child.end(); node_it++ ) {
						out << *node_it << " ";  
					}
					out << std::endl;
					return out;
				}
#endif
		};
}
	
#endif //INCLUDE_OCTTREE_NODE_HPP
