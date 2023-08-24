#include <AGL/agl.hpp>

bool loadOBJ(
	const char * path, 
	std::vector<agl::Vec<float, 3>> & out_vertices, 
	std::vector<agl::Vec<float, 2>> & out_uvs, 
	std::vector<agl::Vec<float, 3>> & out_normals
);
