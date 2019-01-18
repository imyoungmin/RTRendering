//
// Created by Im YoungMin on 2019-01-10.
//

#ifndef OPENGL_OBJECT3D_H
#define OPENGL_OBJECT3D_H

#include <string>
#include <iostream>
#include <OpenGL/gl3.h>
#include <armadillo>
#include <regex>

#include "Configuration.h"

using namespace std;
using namespace arma;

/**
 * This class holds rendering information for a 3D model loaded from an .obj file.
 */
class Object3D
{
private:
	string kind;							// Object type (should be unique for multiple kinds of objects in a scene).
	GLuint bufferID;						// Buffer ID given by OpenGL.
	GLsizei verticesCount;					// Number of vertices stored in buffer.

	GLsizei getData( const vector<vec3>& inVs, const vector<vec2>& inUVs, const vector<vec3>& inNs, vector<float>& outVs, vector<float>& outUVs, vector<float>& outNs ) const;
	static string trim( const string& str, const string& whitespace = " " );

public:
	Object3D();
	Object3D( const char* type, const char* filename );
	void loadOBJ( const char* filename, vector<vec3 >& outVertices, vector<vec2>& outUVs, vector<vec3>& outNormals ) const;
	GLuint getBufferID() const;
	GLsizei getVerticesCount() const;
};


#endif //OPENGL_OBJECT3D_H
