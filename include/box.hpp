#ifndef INCLUDE_OCTTREE_REGION_HPP
#define INCLUDE_OCTTREE_REGION_HPP

namespace OCTree {
	template <size_t const __K, typename _Val> struct _Box;
	template <size_t const __K, typename _Val> struct _Sphere;
	template <size_t const __K, typename _Val>  
	using _QueryPoint = std::array<_Val, __K>;

	//Service static functions
	template <size_t const __K, typename _Val>
	static _Val _shortest_distance(_Box<__K, _Val> const& _box, _QueryPoint<__K, _Val> const& _point) {
		_Val distance = 0;
		for (size_t __i = 0; __i != __K; ++__i) {
			if (
				(_point[__i] < _box._M_low_bounds[__i]) || (_point[__i] > _box._M_high_bounds[__i])
				) {
				_Val temp = std::min(std::abs(_point[__i] - _box._M_low_bounds[__i]), std::abs(_point[__i] - _box._M_high_bounds[__i]));
				distance += temp*temp;
			}
		}
		return distance;
	}
	template <size_t const __K, typename _Val>
	static _Val _longest_distance(_Box<__K, _Val> const& _box, _QueryPoint<__K, _Val> const& _point) {
		_Val distance = 0;
		for (size_t __i = 0; __i != __K; ++__i) {
			_Val temp = std::max(std::abs(_point[__i] - _box._M_low_bounds[__i]), std::abs(_point[__i] - _box._M_high_bounds[__i]));
			distance += temp*temp;
		}
		return distance;
	}
	template <size_t const __K, typename _Val>
	static bool _intersects_with(_Box<__K, _Val> const& _box, _Sphere<__K, _Val> const& _sphere) {
		_Val radius2 = _shortest_distance(_box, _sphere._M_center);
		return radius2 <= _sphere._M_radius2;
	}
	template <size_t const __K, typename _Val>
	static bool _intersects_with(_Box<__K, _Val> const& _left, _Box<__K, _Val> const& _right) {
		for (size_t __i = 0; __i != __K; ++__i) {
			if (
				(_right._M_high_bounds[__i] < _left._M_low_bounds[__i]) || (_right._M_low_bounds[__i] > _left._M_high_bounds[__i])
				) return false;
		}
		return true;
	}
	template <size_t const __K, typename _Val>
	static bool _is_inside(_Sphere<__K, _Val> const& _sphere, _QueryPoint<__K, _Val> const& point) {
		_Val distance2 = 0;
		for (size_t __i = 0; __i != __K; ++__i) {
			_Val temp = point[__i] - _sphere._M_center[__i];
			distance2 += temp*temp;
		}
		return distance2 <= _sphere._M_radius2;
	}
	template <size_t const __K, typename _Val>
	static bool _is_inside(_Box<__K, _Val> const& _box, _QueryPoint<__K, _Val> const& _point) {
		for (size_t __i = 0; __i != __K; ++__i) {
			if (
				(_point[__i] < _box._M_low_bounds[__i]) || (_point[__i] > _box._M_high_bounds[__i])
			) return false;
		}
		return true;
	}
	//////////////////////////////////////////////////////////////////////////////
	template <size_t const __K, typename _Val>
	struct _Sphere {
		typedef       _Val                               value_type;
		typedef const _Val                         value_const_type;
		
		_Sphere() : _M_center(), _M_radius2() {
			_M_radius2 = std::numeric_limits<value_type>::max();
			for (size_t __i = 0; __i != __K; ++__i)
				_M_center[__i] = 0.0;
		}
		bool intersects_with(_Box<__K, _Val> const& _box) const {
			return intersects_with(*this, _box);
		}
		bool is_inside(_QueryPoint<__K, _Val> const& _point) const {
			return is_inside(*this, _point);
		}
		_QueryPoint<__K, _Val>  _M_center;
		value_type              _M_radius2;
	};
	template <>
	struct _Sphere<3, double> {
		//Constructors
		_Sphere() : _M_center(), _M_radius2() {
			_M_radius2 = std::numeric_limits<double>::max();
			_M_center[0] = 0.0; _M_center[1] = 0.0; _M_center[2] = 0.0;
		}
		_Sphere(
			const double x_center, 
			const double y_center, 
			const double z_center,
			const double radius2
		) : _M_center(), _M_radius2(radius2) {
			_M_center[0] = x_center; _M_center[1] = y_center; _M_center[2] = z_center;
		}
		_Sphere(
			const _QueryPoint<3, double> center,
			const double radius2
			) : _M_center(center), _M_radius2(radius2) {
		}
		//Methods
		bool intersects_with(       _Box<3, double> const&   _box) const { return _intersects_with( _box, *this); }
		bool is_inside      (_QueryPoint<3, double> const& _point) const { return _is_inside      (*this, _point);}
		_QueryPoint<3, double>    _M_center;
		double					  _M_radius2;
	};
	template <>
	struct _Sphere<2, double> {
		//Constructors
		_Sphere() : _M_center(), _M_radius2() {
			_M_radius2 = std::numeric_limits<double>::max();
			_M_center[0] = 0.0; _M_center[1] = 0.0;
		}
		_Sphere(
			const double x_center,
			const double y_center,
			const double radius2
			) : _M_center(), _M_radius2(radius2) {
			_M_center[0] = x_center;
			_M_center[1] = y_center;
		}
		_Sphere(
			const _QueryPoint<2, double> center,
			const double radius2
			) : _M_center(center), _M_radius2(radius2) { }
		//Methods
		bool intersects_with(         _Box<2, double> const& _box) const { return _intersects_with( _box,  *this);	}
		bool is_inside      (_QueryPoint<2, double> const& _point) const { return _is_inside      (*this, _point);  }
		_QueryPoint<2, double>    _M_center;
		double					  _M_radius2;
	};
	template <>
	struct _Sphere<1, double> {
		//Constructors
		_Sphere() : _M_center(), _M_radius2() {
			_M_radius2 = std::numeric_limits<double>::max();
			_M_center[0] = 0.0;
		}
		_Sphere(
			const double x_center,
			const double radius2
			) : _M_center(), _M_radius2(radius2) {
			_M_center[0] = x_center;
		}
		_Sphere(
			const _QueryPoint<1, double> center,
			const double radius2
			) : _M_center(center), _M_radius2(radius2) { }
		//Methods
		bool intersects_with(       _Box<1, double> const&   _box) const { return _intersects_with(  _box, *this); }
		bool is_inside      (_QueryPoint<1, double> const& _point) const { return _is_inside      (*this, _point); }
		_QueryPoint<1, double>    _M_center;
		double					  _M_radius2;
	};
	/////////////////////////////////////////////////////////////////////
	template <size_t const __K, typename _Val>
	struct _Box {
		typedef       _Val                               value_type;
		typedef const _Val                         value_const_type;
		//Constructors
		_Box() {
			value_type _max = std::numeric_limits<value_type>::max();
			for (size_t __i = 0; __i != __K; ++__i) {
				_M_low_bounds [__i] =  _max;
				_M_high_bounds[__i] = -_max;
			}
		}
		//Methods
		//Checks intersection of regions
		bool intersects_with(_Sphere<__K, _Val> const& _sphere) const {
			return _intersects_with(*this, _sphere);
		}
		//Checks intersection of regions
		bool intersects_with(_Box const& _box) const {
			return intersects_with( *this, _box);
		}
		//Calculates the shortest distance between the query point and the region
		value_type shortest_distance(_QueryPoint<__K, _Val> const& _point) const {
			return _shortest_distance(*this, _point);
		}
		//Calculates the longest distance between the query point and the region
		//If the query point within the region it will return 0
		value_type longest_distance(_QueryPoint<__K, _Val> const& _point) const {
			return _longest_distance(*this, _point);
		}
		//Checks that the query point within the region
		bool is_inside(_QueryPoint<__K, _Val> const& _point) const {
			return _is_inside(*this, _point);
		}
		value_type _M_low_bounds[__K], _M_high_bounds[__K];
	};
	//Full specialization of the class is required to add new constructors
	template <>
	struct _Box<3, double> {
		typedef       double                               value_type;
		typedef const double                         value_const_type;
		//Constructors
		_Box() {
			value_type value_type_max = std::numeric_limits<value_type>::max();
			_M_low_bounds [0] = value_type_max;
			_M_low_bounds [1] = value_type_max;
			_M_low_bounds [2] = value_type_max;
			_M_high_bounds[0] = -value_type_max;
			_M_high_bounds[1] = -value_type_max;
			_M_high_bounds[2] = -value_type_max;
		}
		_Box( 
			const double x_min, 
			const double x_max,
			const double y_min,
			const double y_max,
			const double z_min,
			const double z_max
			) { 
			_M_low_bounds[0]  = x_min;
			_M_low_bounds[1]  = y_min;
			_M_low_bounds[2]  = z_min;
			_M_high_bounds[0] = x_max;
			_M_high_bounds[1] = y_max;
			_M_high_bounds[2] = z_max;
		}
		//Methods
		bool       intersects_with  (    _Sphere<3, double> const& _sphere) const { return _intersects_with(*this, _sphere); }
		bool       intersects_with  (       _Box<3, double> const&    _box) const { return _intersects_with(_box, *this); }
		value_type shortest_distance(_QueryPoint<3, double> const&  _point) const { return _shortest_distance(*this, _point); }
		value_type longest_distance (_QueryPoint<3, double> const&  _point) const { return _longest_distance(*this, _point); }
		bool       is_inside        (_QueryPoint<3, double> const&  _point) const { return _is_inside(*this, _point); }
		value_type _M_low_bounds[3], _M_high_bounds[3];
	};
	template <>
	struct _Box<2, double> {
		typedef       double                               value_type;
		typedef const double                         value_const_type;
		//Constructors
		_Box() {
			value_type value_type_max = std::numeric_limits<value_type>::max();
			_M_low_bounds [0] = value_type_max;
			_M_low_bounds [1] = value_type_max;
			_M_high_bounds[0] = -value_type_max;
			_M_high_bounds[1] = -value_type_max;
		}
		_Box(
			const double x_min,
			const double x_max,
			const double y_min,
			const double y_max
			) {
			_M_low_bounds[0] = x_min;
			_M_low_bounds[1] = y_min;
			_M_high_bounds[0] = x_max;
			_M_high_bounds[1] = y_max;
		}
		//Methods
		bool       intersects_with  (    _Sphere<2, double> const& _sphere) const { return _intersects_with(*this, _sphere); }
		bool       intersects_with  (       _Box<2, double> const&    _box) const { return _intersects_with( _box,   *this); }
		value_type shortest_distance(_QueryPoint<2, double> const&  _point) const { return _shortest_distance(*this,_point); }
		value_type longest_distance (_QueryPoint<2, double> const&  _point) const {	return _longest_distance(*this, _point); }
		bool       is_inside        (_QueryPoint<2, double> const&  _point) const {	return _is_inside(*this, _point); }
		value_type _M_low_bounds[2], _M_high_bounds[2];
	};
	template <>
	struct _Box<1, double> {
		typedef       double                               value_type;
		typedef const double                         value_const_type;
		//Constructors
		_Box() {
			value_type value_type_max = std::numeric_limits<value_type>::max();
			_M_low_bounds[0] = value_type_max;
			_M_high_bounds[0] = -value_type_max;
		}
		_Box(
			const double x_min,
			const double x_max
			) {
			_M_low_bounds[0] = x_min;
			_M_high_bounds[0] = x_max;
		}
		//Methods
		bool       intersects_with  (    _Sphere<1, double> const& _sphere) const { return _intersects_with(*this, _sphere); }
		bool       intersects_with  (       _Box<1, double> const&    _box) const { return _intersects_with(_box, *this); }
		value_type shortest_distance(_QueryPoint<1, double> const&  _point) const { return _shortest_distance(*this, _point); }
		value_type longest_distance (_QueryPoint<1, double> const&  _point) const { return _longest_distance(*this, _point); }
		bool       is_inside        (_QueryPoint<1, double> const&  _point) const { return _is_inside(*this, _point); }
		value_type _M_low_bounds[1], _M_high_bounds[1];
	};

	typedef _Box   <3, double> _Box3D;
    typedef _Box   <2, double> _Box2D;
	typedef _Box   <1, double> _Box1D;
    typedef _Sphere<3, double> _Sphere3D;
    typedef _Sphere<2, double> _Sphere2D;
	typedef _Sphere<1, double> _Sphere1D;
}
#endif //INCLUDE_OCTTREE_REGION_HPP
