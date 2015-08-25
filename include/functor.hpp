#ifndef INCLUDE_OCTTREE_FUNCTOR_HPP
#define INCLUDE_OCTTREE_FUNCTOR_HPP
#include "box.hpp"
#include "node.hpp"

namespace OCTree {
	template <size_t const __K, typename __Val> 
	class _SphereFunctor {
		static const double zero_;
		
		template <size_t __L> 
		using box_const_reference = const _Box<__L, __Val>&;
		typedef const _Sphere<__K, __Val>& sphere_const_reference;
		typedef __Val                      value_type;
		
		sphere_const_reference _sphere;
	public:
		_SphereFunctor(sphere_const_reference sphere ) : _sphere(sphere) {}
		template <size_t const __L>
		bool operator() (box_const_reference<__L> box) const {
			size_t           __i = 0;
			value_type distance2 = 0;
			for (__i = 0; __i != std::min(__K,__L); ++__i) {
				if ( ( _sphere._M_center[__i] < box._M_low_bounds[__i]) || ( _sphere._M_center[__i] > box._M_high_bounds[__i]) ) {
					value_type temp = 
						std::min(
							std::abs( _sphere._M_center[__i] - box._M_low_bounds [__i]),
							std::abs( _sphere._M_center[__i] - box._M_high_bounds[__i])
						);
					distance2 += temp*temp;
				}
			}
			for (; __i != __L; ++__i) 
				if ((box._M_high_bounds[__i] < zero_) || (box._M_low_bounds[__i] > zero_))   return false;
			return distance2 <= _sphere._M_radius2;
		}

		template <size_t _K, typename _Val, class _Sync> 
		bool operator() ( const _Node<_K, _Val,  _Sync>& node ) const {
			return true;
		}
	};
	template <size_t const __K, typename __Val>
	class _BoxFunctor {
		static const double zero_;

		template <size_t __L> 
		using box_const_reference = const _Box<__L, __Val>&;
		typedef const _Sphere<__K, __Val>& sphere_const_reference;
		typedef __Val                      value_type;
		
		box_const_reference<__K> _box;
	public:
		_BoxFunctor(box_const_reference<__K> box ) : _box(box) {}
		template <size_t const __L>
		bool operator() (box_const_reference<__L> box) const {
			size_t __i = 0;
			for (__i = 0; __i != std::min(__K, __L); ++__i) {
				if ( 
					   ( box._M_high_bounds[__i] < _box._M_low_bounds [__i])
					|| ( box._M_low_bounds [__i] > _box._M_high_bounds[__i])
					)
					return false;
			}
			for (; __i != __L; ++__i) 
				if ((box._M_high_bounds[__i] < zero_) || (box._M_low_bounds[__i] > zero_))   return false;
			return true;
		}
	
		template <size_t _K, typename _Val, class _Sync> 
		bool operator() ( const _Node<_K, _Val, _Sync>& node ) const {
			return true;
		}
	};
	template <size_t const __K, typename _Val>
		const double _SphereFunctor<__K, _Val>::zero_ = 0.0;
	template <size_t const __K, typename _Val>
		const double _BoxFunctor<__K, _Val>::zero_ = 0.0;
	
	class _TrueFunctor{
	public:
	template <size_t __K, typename __Val, class __Sync> 
		bool operator()( const _Node<__K,  __Val,  __Sync>& ) const { return true; } 
	};
	
}
#endif //INCLUDE_OCTTREE_FUNCTOR_HPP
