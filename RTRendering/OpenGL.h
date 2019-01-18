#ifndef OpenGL_h
#define OpenGL_h

#define BUFFER_OFFSET(bytes) ((void*) (bytes))
#define ELEMENTS_PER_VERTEX		3
#define ELEMENTS_PER_MATRIX		16
#define HOMOGENEOUS_VECTOR_SIZE	4

#include <iostream>
#include <armadillo>
#include <OpenGL/gl3.h>
#include <vector>
#include <map>
#include "Shaders.h"
#include "OpenGLGeometry.h"
#include "Atlas.h"
#include "Object3D.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "Configuration.h"

using namespace std;
using namespace arma;

class OpenGL
{
private:
	///////////////////////////////////////// Lighting and material variables //////////////////////////////////////////

	struct Lighting
	{
		vec4 ambient;
		vec4 diffuse;
		vec4 specular;
		float shininess;
	};
	
	// Lighting variables.
	static Lighting material;  					// Material properties (to be changed).
	static const vec4 lightColor;				// Light color (constant).
	static const vec4 lightPosition;
	
	//////////////////////////////////////////// OpenGL rendering variables ////////////////////////////////////////////

	struct GeometryBuffer
	{
		GLuint bufferID;						// Buffer ID given by OpenGL.
		GLuint verticesCount;					// Number of vertices stored in buffer.
	};
	enum GeometryTypes { CUBE, SPHERE, CYLINDER, PRISM };
	
	static GLuint renderingProgram;				// Geom/sequence shader's program.
	static GLuint vao;							// Geom/sequence vertex array object.
	
	static GeometryBuffer* cube;				// Buffers for solids.
	static GeometryBuffer* sphere;
	static GeometryBuffer* cylinder;
	static GeometryBuffer* prism;
	static GeometryBuffer* path;				// Buffer for dots and paths (sequences).

	static bool usingUniformScaling;			// True if only uniform scaling is used.

	static map<string, Object3D> objectModels;	// Store 3D object models per kind.
	
	/////////////////////////////////////////////// FreeType variables /////////////////////////////////////////////////

	struct GlyphPoint {
		GLfloat x;
		GLfloat y;
		GLfloat s;
		GLfloat t;
	};
	
	static FT_Library ft;
	static FT_Face face;

	static GLuint glyphsProgram;				// Glyphs shaders program.
	static GLuint glyphsVao;					// Glyphs vertex array object.
	static GLuint glyphsBufferID;				// Glyphs buffer ID.

	static void sendShadingInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, bool usingBlinnPhong );
	static GLuint setSequenceInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices );
	static void drawGeom( const mat44& Projection, const mat44& Camera, const mat44& Model, GeometryBuffer** G, GeometryTypes t );
	static void initGlyphs();

public:
	static Atlas* atlas48;
	static Atlas* atlas24;
	static Atlas* atlas12;

	static void init();
	static void finish();
	static void setColor( float r, float g, float b, float a = 1.0f );
	static void drawCube( const mat44& Projection, const mat44& Camera, const mat44& Model );
	static void drawSphere( const mat44& Projection, const mat44& Camera, const mat44& Model );
	static void drawCylinder( const mat44& Projection, const mat44& Camera, const mat44& Model );
	static void drawPrism( const mat44& Projection, const mat44& Camera, const mat44& Model );
	static void drawPath( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices );
	static void drawPoints( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices, float size = 10.0f );
	static void render3DObject( const mat44& Projection, const mat44& Camera, const mat44& Model, const char* objectType );
	static void renderText( const char* text, const Atlas* a, float x, float y, float sx, float sy, const float* color );
	static GLuint getRenderingProgram();
	static GLuint getRenderingVao();
	static GLuint getGlyphsProgram();
	static GLuint getGluphsVao();
	static void setUsingUniformScaling( bool u );
	static void create3DObject( const char* name, const char* filename );
};

#endif /* OpenGL_h */






















