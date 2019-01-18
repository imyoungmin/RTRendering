#include "OpenGL.h"

//////////////////////////////////////////// Lighting and material variables ///////////////////////////////////////////

OpenGL::Lighting OpenGL::material = 					// Material properties (which change when we set color).
{
	{ 0.8, 0.8, 0.8, 1.0 },								// Ambient: k_a.
	{ 0.8, 0.8, 0.8, 1.0 },								// Diffuse: k_d.
	{ 0.8, 0.8, 0.8, 1.0 },								// Specular: k_s.
	64.0												// Shininess.
};

const vec4 OpenGL::lightColor =	{ 0.9, 0.9, 0.9, 1.0 };	// Light source properties (constant).
const vec4 OpenGL::lightPosition = { -2, 7, 10, 1 };

///////////////////////////////////////////// OpenGL rendering variables ///////////////////////////////////////////////

GLuint OpenGL::renderingProgram = 0;					// Shader's program.
GLuint OpenGL::vao = 0;									// Vertex array object.

OpenGL::GeometryBuffer* OpenGL::cube = nullptr;
OpenGL::GeometryBuffer* OpenGL::sphere = nullptr;
OpenGL::GeometryBuffer* OpenGL::cylinder = nullptr;
OpenGL::GeometryBuffer* OpenGL::prism = nullptr;
OpenGL::GeometryBuffer* OpenGL::path = nullptr;

bool OpenGL::usingUniformScaling = true;

map<string, Object3D> OpenGL::objectModels;				// Collection of 3D object models.

//////////////////////////////////////////////// FreeType variables ////////////////////////////////////////////////////

FT_Library OpenGL::ft = nullptr;
FT_Face OpenGL::face = nullptr;

GLuint OpenGL::glyphsProgram = 0;						// Glyphs shaders program.
GLuint OpenGL::glyphsVao = 0;							// Glyphs vertex array object.
GLuint OpenGL::glyphsBufferID = 0;						// Glyphs buffer ID.

Atlas* OpenGL::atlas48 = nullptr;						// Text atlas object pointers.
Atlas* OpenGL::atlas24 = nullptr;
Atlas* OpenGL::atlas12 = nullptr;

/**
 * Release resources.
 */
void OpenGL::finish()
{
	glDeleteVertexArrays( 1, &vao );
	glDeleteProgram( renderingProgram );

	glDeleteVertexArrays( 1, &glyphsVao );
	glDeleteProgram( glyphsProgram );
}

/**
 * Initialize the OpenGL engine.
 */
void OpenGL::init()
{
	// Initialize glyphs via FreeType.
	initGlyphs();

	// Initialize shaders for geom/sequence drawing program.  To use the program, get its object from the caller and invoke glUseProgram(.).
	cout << "Initializing geoms and sequence shaders... ";
	Shaders shaders;
	renderingProgram = shaders.compile( conf::SHADERS_FOLDER + "shader.vert", conf::SHADERS_FOLDER + "shader.frag" );
	glGenVertexArrays( 1, &vao );
	cout << "Done!" << endl;
}

/**
 * Initialize OpenGL objects for glyphs.
 */
void OpenGL::initGlyphs()
{
	// Initialize the FreeType2 library.
	if( FT_Init_FreeType( &ft ) )
	{
		cerr << "Could not init FreeType Library" << endl;
		exit( EXIT_FAILURE );
	}

	// Create the font face object.
	FT_Error ft_error = FT_New_Face( ft, string(conf::FONTS_FOLDER + "cmunbmr.ttf").c_str(), 0, &face );
	if( ft_error != FT_Err_Ok )
	{
		cerr << "Could not open font!" << endl;
		exit( EXIT_FAILURE );
	}

	// Initialize shaders for glyphs drawing program.
	Shaders shaders;
	cout << "Initializing glyph shaders... " << endl;
	glGenVertexArrays( 1, &glyphsVao );
	glBindVertexArray( glyphsVao );
	glyphsProgram = shaders.compile( conf::SHADERS_FOLDER + "glyphs.vert", conf::SHADERS_FOLDER + "glyphs.frag" );
	if( glyphsProgram == 0 )
	{
		cerr << "Failed to compile glyphs shaders proglram!" << endl;
		exit( EXIT_FAILURE );
	}

	GLint attribute_coord = glGetAttribLocation( glyphsProgram, "coord" );	// Reading locations off the shader.
	GLint uniform_tex = glGetUniformLocation( glyphsProgram, "tex" );
	GLint uniform_color = glGetUniformLocation( glyphsProgram, "color" );

	if( attribute_coord == -1 || uniform_tex == -1 || uniform_color == -1 )
	{
		cerr << "Failed to read off shader locations!" << endl;
		exit( EXIT_FAILURE );
	}

	glGenBuffers( 1, &glyphsBufferID );		// Create the vertex buffer object

	// Create texture atlasses for several font sizes.
	atlas48 = new Atlas( face, 48, uniform_color, attribute_coord, uniform_color );
	atlas24 = new Atlas( face, 24, uniform_color, attribute_coord, uniform_color );
	atlas12 = new Atlas( face, 12, uniform_color, attribute_coord, uniform_color );

	// Done with freetype: free resources.
	FT_Done_Face( face );
	FT_Done_FreeType( ft );

	cout << "Done!" << endl;
}

/**
 * Change material color.
 * @param r Red component in [0,1].
 * @param g Green component in [0,1].
 * @param b Blue component in [0,1].
 * @param a Alpha value in [0,1].
 */
void OpenGL::setColor( float r, float g, float b, float a )
{
	// Clamp components to valid values.
	r = fmax( 0.0f, fmin( r, 1.0f ) );
	g = fmax( 0.0f, fmin( g, 1.0f ) );
	b = fmax( 0.0f, fmin( b, 1.0f ) );
	a = fmax( 0.0f, fmin( a, 1.0f ) );

	material.diffuse = { r, g, b, a };
	material.ambient = material.diffuse * 0.1;
	material.specular[3] = material.ambient[3] = a;
}

/**
 * Draw a unit cube at the origin.
 * Creates a unit cube at the origin. If the cube is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawCube( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &cube, CUBE );
}

/**
 * Draw a unit sphere at the origin.
 * Creates a unit sphere at the origin. If the sphere is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawSphere( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &sphere, SPHERE );
}

/**
 * Draw a unit-length cylinder, with unit radius, from z=0 to z=1.
 * Creates a unit cylinder. If the cylinder is not yet created, it fills out a buffer and
 * leaves its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawCylinder( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &cylinder, CYLINDER );
}

/**
 * Draw a unit prism.
 * Creates a 8-sided prism whose first apex is at the orgin, and the second apex is at (0,0,1).
 * The pyramid bases join at the plane z=0.3 and consist of a square inscribed in
 * a circle of unit radius. If the prism is not yet created, it fills out a buffer and leaves
 * its vertex and normal data there for future calls, making it drawing in OpenGL more efficient.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera transformation matrix.
 * @param Model The 4x4 model transformation matrix.
 */
void OpenGL::drawPrism( const mat44& Projection, const mat44& Camera, const mat44& Model )
{
	drawGeom( Projection, Camera, Model, &prism, PRISM );
}

/**
 * Draw an open path.
 * This function combines the functionality of the private function drawGeometry with the public draw* functions,
 * but for a path, which doesn't consider the normals.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of vec3 elements containing position information.
 */
void OpenGL::drawPath( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices )
{
	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending for transparency.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	GLuint posL = setSequenceInformation( Projection, Camera, Model, vertices );		// Prepare drawing by sending shading information to shaders.

	// Draw connected line segments.
	glDrawArrays( GL_LINE_STRIP, 0, path->verticesCount );

	// Disable vertex attribute array position we sent in the setSequenceInformation function.
	glDisableVertexAttribArray( posL );

	if( material.ambient[3] < 1.0 )		// Restore blending if necessary.
		glDisable( GL_BLEND );
};

/**
 * Draw a sequence of points.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of vec3 elements containing vertex positions.
 * @param size Pixel size for points.
 */
void OpenGL::drawPoints( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices, float size )
{
	if( size < 0 )
		size = 10.0;

	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending for transparency.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	GLuint posL = setSequenceInformation( Projection, Camera, Model, vertices );		// Prepare drawing by sending shading information to shaders.

	// Overriding the point size set by the sendShadingInformation() function in vertex shader.
	int pointSize_location = glGetUniformLocation( renderingProgram, "pointSize" );
	glUniform1f( pointSize_location, size );

	// Specify we are drawing a point --setSequenceInformation (via sendShadingInformation) sent a false, but here we'll override it with a 1.
	int drawPoint_location = glGetUniformLocation( renderingProgram, "drawPoint" );
	glUniform1i( drawPoint_location, true );

	glEnable( GL_PROGRAM_POINT_SIZE );
	glDrawArrays( GL_POINTS, 0, path->verticesCount );
	glDisable( GL_PROGRAM_POINT_SIZE );

	// Disable vertex attribute array position we sent in the setSequenceInformation function.
	glDisableVertexAttribArray( posL );

	if( material.ambient[3] < 1.0 )		// Restore blending mode.
		glDisable( GL_BLEND );
};

/**
 * Auxiliary function to draw any geometry.
 * This function considers the right geometry buffer has been bound (active) and executes all
 * necessary commands to finish drawing the active geometry.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param G A pointer to the geometry data structure.
 * @param t Type of geometry to be drawn.
 */
void OpenGL::drawGeom( const mat44& Projection, const mat44& Camera, const mat44& Model, GeometryBuffer** G, GeometryTypes t )
{
	if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending.
	{
		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	}

	if( *G == nullptr )					// No data yet loaded into the buffer?
	{
		*G = new GeometryBuffer();
		glGenBuffers( 1, &((*G)->bufferID) );
		glBindBuffer( GL_ARRAY_BUFFER, (*G)->bufferID );
		
		OpenGLGeometry geom;
		switch( t )						// Create a geometry vertices and normals according to requested type.
		{
			case CUBE: geom.createCube(); break;
			case SPHERE: geom.createSphere(); break;
			case CYLINDER: geom.createCylinder(); break;
			case PRISM: geom.createPrism(); break;
		}

		vector<float> vertexPositions;
		vector<float> normals;
		(*G)->verticesCount = geom.getData( vertexPositions, normals );
		
		// Allocate space for the buffer.
		const size_t size = sizeof(float) * vertexPositions.size();				// Size of arrays in bytes.
		glBufferData( GL_ARRAY_BUFFER, 2 * size, nullptr, GL_STATIC_DRAW );
		glBufferSubData( GL_ARRAY_BUFFER, 0, size, vertexPositions.data() );	// Copy actual position and normal data.
		glBufferSubData( GL_ARRAY_BUFFER, size, size, normals.data() );
	}
	else									// Data is already there; just make geom bufferID the active buffer.
		glBindBuffer( GL_ARRAY_BUFFER, (*G)->bufferID );
	
	// Set up our vertex attributes.
	auto position_location = static_cast<GLuint>( glGetAttribLocation( renderingProgram, "position" ) );
	glEnableVertexAttribArray( position_location );
	glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );
	
	auto normal_location = static_cast<GLuint>( glGetAttribLocation( renderingProgram, "normal" ) );
	glEnableVertexAttribArray( normal_location );
	size_t offset = sizeof(float) * (*G)->verticesCount * ELEMENTS_PER_VERTEX;
	glVertexAttribPointer( normal_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( offset ) );

	sendShadingInformation( Projection, Camera, Model, true );
	
	// Draw triangles.
	glDrawArrays( GL_TRIANGLES, 0, (*G)->verticesCount );

	// Disable attribute arrays for position and normals.
	glDisableVertexAttribArray( position_location );
	glDisableVertexAttribArray( normal_location );

	if( material.ambient[3] < 1.0 )
		glDisable( GL_BLEND );
}

/**
 * Send shading information to GPU.
 * @param Projection 4x4 Projection matrix.
 * @param Camera 4x4 Camera matrix.
 * @param Model 4x4 Model matrix.
 * @param usingBlinnPhong Whether use phong model of flat coloring of geoms.
 */
void OpenGL::sendShadingInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, bool usingBlinnPhong )
{
	// Send the model-view and projection matrices.
	float model_matrix[ELEMENTS_PER_MATRIX];
	float view_matrix[ELEMENTS_PER_MATRIX];
	float proj_matrix[ELEMENTS_PER_MATRIX];
	int model_location = glGetUniformLocation( renderingProgram, "Model" );
	int view_location = glGetUniformLocation( renderingProgram, "View");
	int proj_location = glGetUniformLocation( renderingProgram, "Projection" );
	Tx::toOpenGLMatrix( model_matrix, Model );
	Tx::toOpenGLMatrix( view_matrix, Camera );
	Tx::toOpenGLMatrix( proj_matrix, Projection );
	glUniformMatrix4fv( model_location, 1, GL_FALSE, model_matrix );
	glUniformMatrix4fv( view_location, 1, GL_FALSE, view_matrix );
	glUniformMatrix4fv( proj_location, 1, GL_FALSE, proj_matrix );

	if( usingBlinnPhong )
	{
		float itmv_matrix[9];
		mat33 InvTransMV = Tx::getInvTransModelView( Camera * Model, usingUniformScaling );		// The inverse transpose of the upper left 3x3 matrix in the Model View matrix.
		Tx::toOpenGLMatrix( itmv_matrix, InvTransMV );
		int itmv_location = glGetUniformLocation( renderingProgram, "InvTransModelView" );
		glUniformMatrix3fv( itmv_location, 1, GL_FALSE, itmv_matrix );
	}

	// Specify if we will use phong lighting model.
	int useBlinnPhong_location = glGetUniformLocation( renderingProgram, "useBlinnPhong" );
	glUniform1i( useBlinnPhong_location, usingBlinnPhong );

	// Specify we are not drawing points.
	int drawPoint_location = glGetUniformLocation( renderingProgram, "drawPoint" );
	glUniform1i( drawPoint_location, false );

	// Set up shading.
	int lightSource_location = glGetUniformLocation( renderingProgram, "lightPosition" );
	float ls_vector[HOMOGENEOUS_VECTOR_SIZE];
	Tx::toOpenGLMatrix( ls_vector, Camera*lightPosition );
	glUniform4fv( lightSource_location, 1, ls_vector );

	// Set up material shading.
	int shininess_location = glGetUniformLocation( renderingProgram, "shininess" );
	glUniform1f( shininess_location, material.shininess );

	float ambientProd_vector[HOMOGENEOUS_VECTOR_SIZE];
	float diffuseProd_vector[HOMOGENEOUS_VECTOR_SIZE];
	float specularProd_vector[HOMOGENEOUS_VECTOR_SIZE];

	Tx::toOpenGLMatrix( ambientProd_vector, material.ambient % lightColor );
	Tx::toOpenGLMatrix( diffuseProd_vector, material.diffuse % lightColor );
	Tx::toOpenGLMatrix( specularProd_vector, material.specular % lightColor );

	int ambientProd_location = glGetUniformLocation( renderingProgram, "ambientProd" );
	int diffuseProd_location = glGetUniformLocation( renderingProgram, "diffuseProd" );
	int specularProd_location = glGetUniformLocation( renderingProgram, "specularProd" );

	glUniform4fv( ambientProd_location, 1, ambientProd_vector );
	glUniform4fv( diffuseProd_location, 1, diffuseProd_vector );
	glUniform4fv( specularProd_location, 1, specularProd_vector );
}

/**
 * Set sequence of vertices information for a path.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param vertices A vector of 3D vertices.
 * @return The position attribute location in shader, so that the pointer can be disabled in the caller.
 */
GLuint OpenGL::setSequenceInformation( const mat44& Projection, const mat44& Camera, const mat44& Model, const vector<vec3>& vertices )
{
	if( path == nullptr )									// We haven't used this buffer before? Create it.
	{
		path = new GeometryBuffer;
		glGenBuffers( 1, &(path->bufferID) );
	}
	glBindBuffer( GL_ARRAY_BUFFER, path->bufferID );		// Make path buffer the current one.

	// Load vertices and (virtually no) normals.
	path->verticesCount = static_cast<GLuint>( vertices.size() );
	const int totalElements = ELEMENTS_PER_VERTEX * path->verticesCount;
	float vertexPositions[totalElements];
	for( int i = 0; i < path->verticesCount; i++ )
	{
		for( int j = 0; j < ELEMENTS_PER_VERTEX; j++ )
			vertexPositions[ELEMENTS_PER_VERTEX * i + j] = vertices[i][j];
	}

	// Allocate space for the buffer.
	const size_t size = sizeof(float) * totalElements;		// Size of arrays in bytes.
	glBufferData( GL_ARRAY_BUFFER, size, vertexPositions, GL_DYNAMIC_DRAW );

	// Set up our vertex attributes.
	auto position_location = static_cast<GLuint>( glGetAttribLocation( renderingProgram, "position" ) );
	glEnableVertexAttribArray( position_location );
	glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );

	sendShadingInformation( Projection, Camera, Model, false );			// Without using phong model.

	return position_location;
}

/**
 * Render a 3D object model of a selected type.
 * @param Projection The 4x4 projection matrix.
 * @param Camera The 4x4 camera matrix.
 * @param Model The 4x4 model transformation matrix.
 * @param objectType Type of object to be rendered.
 */
void OpenGL::render3DObject( const mat44& Projection, const mat44& Camera, const mat44& Model, const char* objectType )
{
	try
	{
		Object3D o = objectModels.at( string( objectType ) );	// Retrieve object.

		if( material.ambient[3] < 1.0 )		// If alpha channel in current material color is not fully opaque, enable blending.
		{
			glEnable( GL_BLEND );
			glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		}

		glBindBuffer( GL_ARRAY_BUFFER, o.getBufferID() );

		// Set up our vertex attributes.
		auto position_location = static_cast<GLuint>( glGetAttribLocation( renderingProgram, "position" ) );
		glEnableVertexAttribArray( position_location );
		glVertexAttribPointer( position_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( 0 ) );

		auto normal_location = static_cast<GLuint>( glGetAttribLocation( renderingProgram, "normal" ) );
		glEnableVertexAttribArray( normal_location );
		size_t offset = sizeof(float) * o.getVerticesCount() * ELEMENTS_PER_VERTEX;
		glVertexAttribPointer( normal_location, ELEMENTS_PER_VERTEX, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET( offset ) );

		sendShadingInformation( Projection, Camera, Model, true );

		// Draw triangles.
		glDrawArrays( GL_TRIANGLES, 0, o.getVerticesCount() );

		// Disable attribute arrays for position and normals.
		glDisableVertexAttribArray( position_location );
		glDisableVertexAttribArray( normal_location );

		if( material.ambient[3] < 1.0 )
			glDisable( GL_BLEND );

	}
	catch( const out_of_range& oor )
	{
		cerr << "Attempting to render a nonexistent type of 3D object model!" << endl;
	}
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 * @param text Text to render.
 * @param a Glyphs atlas object.
 * @param x Start x-coordinate.
 * @param y Start y-coordinate.
 * @param sx Horizontal scaling.
 * @param sy Vertical scaling.
 * @param color Text color as 4-tuple [r,g,b,a].
 */
void OpenGL::renderText( const char* text, const Atlas* a, float x, float y, float sx, float sy, const float* color )
{
	const uint8_t *p;

	// Use the texture containing the atlas.
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, a->tex );
	glUniform1i( a->uniform_tex_loc, 0 );

	// Set up the VBO for our vertex data.
	glEnableVertexAttribArray( a->attribute_coord_loc );
	glBindBuffer( GL_ARRAY_BUFFER, glyphsBufferID );
	glVertexAttribPointer( a->attribute_coord_loc, 4, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(0) );

	// Set text color.
	glUniform4fv( a->uniform_color_loc, 1, color );

	GlyphPoint coords[6 * strlen(text)];
	int c = 0;

	// Loop through all characters.
	for( p = (const uint8_t *)text; *p; p++ )
	{
		// Calculate the vertex and texture coordinates.
		float x2 = x + a->c[*p].bl * sx;
		float y2 = -y - a->c[*p].bt * sy;
		float w = a->c[*p].bw * sx;
		float h = a->c[*p].bh * sy;

		// Advance the cursor to the start of the next character.
		x += a->c[*p].ax * sx;
		y += a->c[*p].ay * sy;

		// Skip glyphs that have no pixels.
		if( w <= 0 || h <= 0 )
			continue;

		coords[c++] = (GlyphPoint) { x2, -y2, a->c[*p].tx, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty + a->c[*p].bh / a->h };
		coords[c++] = (GlyphPoint) { x2 + w, -y2, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty };
		coords[c++] = (GlyphPoint) { x2, -y2 - h, a->c[*p].tx, a->c[*p].ty + a->c[*p].bh / a->h };
		coords[c++] = (GlyphPoint) { x2 + w, -y2 - h, a->c[*p].tx + a->c[*p].bw / a->w, a->c[*p].ty + a->c[*p].bh / a->h };
	}

	// Draw all the character on the screen in one go.
	glBufferData( GL_ARRAY_BUFFER, sizeof( coords ), coords, GL_DYNAMIC_DRAW );
	glDrawArrays( GL_TRIANGLES, 0, c );

	glDisableVertexAttribArray( a->attribute_coord_loc );
}

/**
 * Get the geom/sequence rendering program ID.
 * @return OpenGL rendering program ID.
 */
GLuint OpenGL::getRenderingProgram()
{
	return renderingProgram;
}

/**
 * Get the geom/sequence rendering vertex array object ID.
 * @return OpenGL VAO.
 */
GLuint OpenGL::getRenderingVao()
{
	return vao;
}

/**
 * Get the glyphs program ID.
 * @return OpenGL program ID.
 */
GLuint OpenGL::getGlyphsProgram()
{
	return glyphsProgram;
}

/**
 * Get the glyphs vertex array object ID.
 * @return OpenGL VAO.
 */
GLuint OpenGL::getGluphsVao()
{
	return glyphsVao;
}

/**
 * Set the uniform scaling flag to avoid computing inverses of the 3x3 principal submatrix of the model view matrix.
 * @param u True for using uniform scaling.
 */
void OpenGL::setUsingUniformScaling( bool u )
{
	usingUniformScaling = u;
}

/**
 * Load a new type of 3D object and allocate its necessary OpenGL rendering objects.
 * @param name User-defined object type name.
 * @param filename *.obj filename that contains the 3D triangular mesh.
 */
void OpenGL::create3DObject( const char* name, const char* filename )
{
	// Check if the object we want to create already exists.  If so, empty its buffer and recreate it.
	string sName = string( name );
	auto it = objectModels.find( sName );
	if( it != objectModels.end() )					// Element found?
	{
		Object3D o = it->second;
		cout << "WARNING!  You are attempting to create a new type of 3D object with an existing name.  The old one will be replaced!" << endl;
		GLuint bufferID = o.getBufferID();
		glDeleteBuffers( 1, &bufferID );			// Empty buffer.
	}

	objectModels[sName] = Object3D( name, filename );
	cout << "The 3D object of kind \"" << name << "\" has been successfully allocated!" << endl;
}















