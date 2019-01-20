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
	Lighting material = {						// Material properties (to be changed).
		{ 0.8, 0.8, 0.8, 1.0 },					// Ambient: k_a.
		{ 0.8, 0.8, 0.8, 1.0 },					// Diffuse: k_d.
		{ 0.8, 0.8, 0.8, 1.0 },					// Specular: k_s.
		64.0									// Shininess.
	};
	vec4 lightColor;							// Light properties.
	vec4 lightPosition;
	
	//////////////////////////////////////////// OpenGL rendering variables ////////////////////////////////////////////

	struct GeometryBuffer
	{
		GLuint bufferID;						// Buffer ID given by OpenGL.
		GLuint verticesCount;					// Number of vertices stored in buffer.
	};
	enum GeometryTypes { CUBE, SPHERE, CYLINDER, PRISM };
	
	GLuint renderingProgram;					// Geom/sequence full color renderer's shader program.
	GLuint vao;									// Vertex array object.
	
	GeometryBuffer* cube = nullptr;				// Buffers for solids.
	GeometryBuffer* sphere = nullptr;
	GeometryBuffer* cylinder = nullptr;
	GeometryBuffer* prism = nullptr;
	GeometryBuffer* path = nullptr;				// Buffer for dots and paths (sequences).

	bool usingUniformScaling = true;			// True if only uniform scaling is used.

	map<string, Object3D> objectModels;			// Store 3D object models per kind.
	
	/////////////////////////////////////////////// FreeType variables /////////////////////////////////////////////////

	struct GlyphPoint {
		GLfloat x;
		GLfloat y;
		GLfloat s;
		GLfloat t;
	};
	
	FT_Library ft = nullptr;
	FT_Face face = nullptr;

	GLuint glyphsProgram;						// Glyphs shaders program.
	GLuint glyphsBufferID;						// Glyphs buffer ID.

	void sendShadingInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, bool usingBlinnPhong );
	GLint setSequenceInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, const vector<vec3>& vertices );
	void drawGeom( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, GeometryBuffer** G, GeometryTypes t );
	void initGlyphs();

public:
	Atlas* atlas48 = nullptr;					// Atlases (i.e. font texture maps).
	Atlas* atlas24 = nullptr;
	Atlas* atlas12 = nullptr;

	OpenGL();
	~OpenGL();
	void init( const vec3& lPosition, const vec3& lColor = { 0.9, 0.9, 0.9 } );
	void setColor( float r, float g, float b, float a = 1.0f );
	void drawCube( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix );
	void drawSphere( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix );
	void drawCylinder( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix );
	void drawPrism( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix );
	void drawPath( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, const vector<vec3>& vertices );
	void drawPoints( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, const vector<vec3>& vertices, float size = 10.0f );
	void render3DObject( const mat44& Projection, const mat44& Camera, const mat44& Model, const mat44& LightSpaceMatrix, const char* objectType );
	void renderText( const char* text, const Atlas* a, float x, float y, float sx, float sy, const float* color );
	GLuint getGlyphsProgram();
	void setUsingUniformScaling( bool u );
	void create3DObject( const char* name, const char* filename );
	void useProgram( GLuint program );
};

#endif /* OpenGL_h */






















