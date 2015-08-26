#include <iostream>
#include <vector>
#include <array>
#include <limits>
#include <thread>

#include "octree.hpp"

const double double_max = std::numeric_limits<double>::max();

struct POINT {
	double x, y, z;
};

struct  WRAPPER_CLASS;
typedef OCTree::OCTree<3, WRAPPER_CLASS, OCTree::mutex_sync_object> OCTREE;
typedef OCTree::_Node <3, WRAPPER_CLASS, OCTree::mutex_sync_object> NODE;

struct  WRAPPER_CLASS {
  typedef double value_type;
  typedef OCTREE::box_const_type box_const_type;
  inline bool operator () (box_const_type& box) const;
  inline bool operator  < (const WRAPPER_CLASS& data) const { return object  < data.object; }
  POINT* object;
};

bool WRAPPER_CLASS::operator () (box_const_type& box) const {
  const double& x = object->x;
  const double& y = object->y;
  const double& z = object->z;

  bool flag = true;
  flag &= (x >= box._M_low_bounds[0]) && (x <= box._M_high_bounds[0]);
  flag &= (y >= box._M_low_bounds[1]) && (y <= box._M_high_bounds[1]);
  flag &= (z >= box._M_low_bounds[2]) && (z <= box._M_high_bounds[2]);
  
  return flag;
}

void fill (OCTREE* tree, const std::vector<WRAPPER_CLASS>& objects, const int& thread_num) {
	for ( auto object : objects ) tree->insert(object);
	return;
}

void optimize(OCTREE* tree) {
	tree->optimize();
	return;
}

struct functor {
	bool operator( )( const NODE& node ) const {
		auto box  = node._M_box;
		
		bool flag = true;
		flag &= ( 0.5 >= box._M_low_bounds[0] ) && ( -0.5 <= box._M_high_bounds[0] );
  		flag &= ( 0.5 >= box._M_low_bounds[1] ) && ( -0.5 <= box._M_high_bounds[1] );
  		flag &= ( 0.5 >= box._M_low_bounds[2] ) && ( -0.5 <= box._M_high_bounds[2] );
  
  		return flag;
	}	
};

void check(OCTREE* tree, const std::vector<WRAPPER_CLASS>& objects) {
	
	OCTREE::query_type query_point = {{ 0.0, 0.0, 0.0}};
	
	std::vector<WRAPPER_CLASS> find_exact     = tree->find_exact    (query_point);
	std::vector<WRAPPER_CLASS> find_nearest   = tree->find_nearest  (query_point);
	std::vector<WRAPPER_CLASS> find_nearest_s = tree->find_nearest_s(query_point);
	std::vector<WRAPPER_CLASS> find_if        = tree->find_if       (  functor());

	return;
}

int main() {
	const int num_threads = 8;
	const int num_objects = 10;
	std::vector<std::thread>   threads;
	std::vector<WRAPPER_CLASS> objects;
	
	for(int i = 0; i <= num_objects; ++i) {
		for(int j = 0; j <= num_objects; ++j) {
			for(int k = 0; k <= num_objects; ++k) {
				const double x = 2.*(i - num_objects/2.)/num_objects;
				const double y = 2.*(j - num_objects/2.)/num_objects;
				const double z = 2.*(k - num_objects/2.)/num_objects;

				WRAPPER_CLASS    temp;
				temp.object    = new POINT();
				temp.object->x = x; temp.object->y = y; temp.object->z = z;
				objects.push_back(temp);
			}
		}
	}
	//Allocate memory
	OCTREE* tree = new OCTREE( OCTREE::box_type( -1, 1, -1, 1, -1, 1) );
	//Fill the tree in multithreaded mode
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&fill    , tree, objects, i));
	for (size_t i = 0; i < num_threads; ++i)
		threads[i].join();
	threads.clear();
	//Output the tree
	std::cout << *tree << std::endl;	
	//Optimize the tree
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&optimize, tree));
	for (size_t i = 0; i < num_threads; ++i)
		threads[i].join();
	threads.clear();
	//Output the tree
	std::cout << *tree << std::endl;	
	//Check the tree in multithreaded mode
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&check   , tree, objects));
	for (size_t i = 0; i < num_threads; ++i)
		threads[i].join();
	threads.clear();
	//Dump the tree
	tree->dump("point");	
	
	delete tree;
	for (auto object : objects) delete object.object;
}
