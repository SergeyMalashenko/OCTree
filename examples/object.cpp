#include <iostream>
#include <vector>
#include <array>
#include <limits>
#include <thread>

#include "octree.hpp"

const double double_max = std::numeric_limits<double>::max();

struct TETRAHEDRON {
	double x0, y0, z0;
	double x1, y1, z1;
	double x2, y2, z2;
	double x3, y3, z3;
};

struct  WRAPPER_CLASS;
typedef OCTree::OCTree<3, WRAPPER_CLASS, OCTree::mutex_sync_object> OCTREE;

struct  WRAPPER_CLASS {
  typedef double value_type;
  typedef OCTREE::box_const_type box_const_type;
  inline bool operator () (box_const_type& box) const;
  inline bool operator  < (const WRAPPER_CLASS& data) const { return object  < data.object; }
  TETRAHEDRON* object;
};

bool WRAPPER_CLASS::operator () (box_const_type& box) const {
  double x_max = -double_max; double x_min =  double_max;
  double y_max = -double_max; double y_min =  double_max;
  double z_max = -double_max; double z_min =  double_max;

  x_max = std::max(x_max, object->x0); x_min = std::min(x_min, object->x0);
  x_max = std::max(x_max, object->x1); x_min = std::min(x_min, object->x1);
  x_max = std::max(x_max, object->x2); x_min = std::min(x_min, object->x2);
  x_max = std::max(x_max, object->x3); x_min = std::min(x_min, object->x3);
  
  y_max = std::max(y_max, object->y0); y_min = std::min(y_min, object->y0);
  y_max = std::max(y_max, object->y1); y_min = std::min(y_min, object->y1);
  y_max = std::max(y_max, object->y2); y_min = std::min(y_min, object->y2);
  y_max = std::max(y_max, object->y3); y_min = std::min(y_min, object->y3);
  
  z_max = std::max(z_max, object->z0); z_min = std::min(z_min, object->z0);
  z_max = std::max(z_max, object->z1); z_min = std::min(z_min, object->z1);
  z_max = std::max(z_max, object->z2); z_min = std::min(z_min, object->z2);
  z_max = std::max(z_max, object->z3); z_min = std::min(z_min, object->z3);
  

  bool flag = true;
  flag &= (x_max >= box._M_low_bounds[0]) && (x_min <= box._M_high_bounds[0]);
  flag &= (y_max >= box._M_low_bounds[1]) && (y_min <= box._M_high_bounds[1]);
  flag &= (z_max >= box._M_low_bounds[2]) && (z_min <= box._M_high_bounds[2]);
  return flag;
}

void fill (OCTREE* tree) {
	const size_t num_objects = 10;
	for(size_t i = 0; i < num_objects; ++i) {
		for(size_t j = 0; j < num_objects; ++j) {
			for(size_t k = 0; k < num_objects; ++k) {
				const double x = 2*(i - num_objects/2)/num_objects;
				const double y = 2*(j - num_objects/2)/num_objects;
				const double z = 2*(k - num_objects/2)/num_objects;

				WRAPPER_CLASS     temp;
				temp.object     = new TETRAHEDRON();
				temp.object->x0 = x + 0; temp.object->y0 = y + 0; temp.object->z0 = z + 0;
				temp.object->x1 = x + 1; temp.object->y1 = y + 0; temp.object->z1 = z + 0;
				temp.object->x2 = x + 0; temp.object->y2 = y + 1; temp.object->z2 = z + 0;
				temp.object->x3 = x + 0; temp.object->y3 = y + 0; temp.object->z3 = z + 1;
				tree->insert(temp);
			}
		}
	}
	return;
}

void optimize(OCTREE* tree) {
	tree->optimize();
	return;
}

void check(OCTREE* tree) {
		
	return;
}

int main() {
	const size_t num_threads = 4;
	std::vector<std::thread>   threads;

	OCTREE* tree = new OCTREE( OCTREE::box_type( -1, 1, -1, 1, -1, 1) );
	//Fill the tree in multithreaded mode	
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&fill    , tree));
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
	//Check the tree
	for (size_t i = 0; i < num_threads; ++i)
		threads.push_back(std::thread(&check   , tree));
	for (size_t i = 0; i < num_threads; ++i)
		threads[i].join();
	threads.clear();
	//Dump the tree
	tree->dump("object");	
	delete tree;
}
