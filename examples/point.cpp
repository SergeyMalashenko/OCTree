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
typedef OCTree::OCTree<3, WRAPPER_CLASS> OCTREE;

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

void fill (OCTREE* tree, const std::vector<WRAPPER_CLASS>& objects) {
	for ( auto object : objects ) tree->insert(object); 
	return;
}

void optimize(OCTREE* tree) {
	tree->optimize();
	return;
}

void check(OCTREE* tree, const std::vector<WRAPPER_CLASS>& objects) {
	for(auto object : objects) {
		std::cout << "Lalala" << std::endl;
	}	
	return;
}

int main() {
	const size_t num_threads = 4;
	const size_t num_objects = 20;
	std::vector<std::thread>   threads;
	std::vector<WRAPPER_CLASS> objects;
	
	for(size_t i = 0; i <= num_objects; ++i) {
		for(size_t j = 0; j <= num_objects; ++j) {
			for(size_t k = 0; k <= num_objects; ++k) {
				const double x = 2*(i - num_objects/2)/num_objects;
				const double y = 2*(j - num_objects/2)/num_objects;
				const double z = 2*(k - num_objects/2)/num_objects;

				WRAPPER_CLASS    temp;
				temp.object    = new POINT();
				temp.object->x = x; temp.object->y = y; temp.object->z = z;
				objects.push_back(temp);
			}
		}
	}

	OCTREE* tree = new OCTREE( OCTREE::box_type( -1, 1, -1, 1, -1, 1) );
	//Fill the tree in multithreaded mode
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&fill    , tree, objects));
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
	std::cout << "Hello world" << std::endl;
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
