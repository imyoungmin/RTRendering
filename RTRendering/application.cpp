/**
 * OpenGL main application.
 */

#include <iostream>
#include <armadillo>
#include <OpenGL/gl3.h>
#include <string>
#include "GLFW/glfw3.h"
#include "ArcBall/Ball.h"
#include "OpenGL.h"
#include "Transformations.h"

using namespace std;
using namespace std::chrono;
using namespace arma;

// Perspective projection matrix.
mat44 Proj;

// Text scaling.
float gTextScaleX;
float gTextScaleY;

// Camera controls globals.
vec3 gPointOfInterest;
vec3 gEye;
vec3 gUp;

bool gLocked;							// Track if mouse button is pressed down.
bool gUsingArrowKey;					// Track if we are using the arrow keys for rotating scene.
float gZoom;							// Camera zoom.
const float ZOOM_IN = 1.015;
const float ZOOM_OUT = 0.985;
BallData* gArcBall;						// Arc ball.

// Framebuffer size metrics.
int fbWidth;
int fbHeight;
float gRetinaRatio;						// How many screen dots exist per OpenGL pixel.

OpenGL ogl;								// Initialize application OpenGL.

// Frame rate variables and functions.
static const int NUM_FPS_SAMPLES = 64;
float gFpsSamples[NUM_FPS_SAMPLES];
unsigned char gCurrentSample = 0;		// Should start storing from gCurrentSample >= 1.

/**
 * Calculate the number of frames per second using a window.
 * @param dt Amount of seconds for current frame.
 * @return Frames per second.
 */
float calculateFPS( float dt )
{
	gCurrentSample++;
	gCurrentSample = static_cast<unsigned char>( max( 1, static_cast<int>( gCurrentSample ) ) );
	if( dt <= 0 )
		cout << "error" << endl;
	gFpsSamples[(gCurrentSample - 1) % NUM_FPS_SAMPLES] = 1.0f / dt;
	float fps = 0;
	int i = 0;
	for( i = 0; i < min( NUM_FPS_SAMPLES, static_cast<int>( gCurrentSample ) ); i++ )
		fps += gFpsSamples[i];
	fps /= i;
	return fps;
}

/**
 * Reset rotation and zoom.
 */
void resetArcBall()
{
	Ball_Init( gArcBall );
	Ball_Place( gArcBall, qOne, 0.75 );
}

/**
 * GLFW error callback.
 * @param error Error code.
 * @param description Error description.
 */
void errorCallback( int error, const char* description )
{
	cerr << error << ": " << description << endl;
}

/**
 * Rotate scene in x or y direction.
 * @param x Rotation amount in x direction, usually in the range [-1,1].
 * @param y Rotation amount in y direction, usually in the range [-1,1].
 */
void rotateWithArrowKey( float x, float y )
{
	if( gLocked )		// Do not rotate scene with arrow key if it's currently rotating with mouse.
		return;

	gUsingArrowKey = true;						// Start blocking rotation with mouse button.
	HVect arcballCoordsStart, arcballCoordsEnd;

	arcballCoordsStart.x = 0.0;					// Start rotation step.
	arcballCoordsStart.y = 0.0;
	Ball_Mouse( gArcBall, arcballCoordsStart );
	Ball_BeginDrag( gArcBall );

	arcballCoordsEnd.x = x;						// End rotation step.
	arcballCoordsEnd.y = y;
	Ball_Mouse( gArcBall, arcballCoordsEnd );
	Ball_Update( gArcBall );
	Ball_EndDrag( gArcBall );
	gUsingArrowKey = false;						// Exiting conflicting block for rotating with arrow keys.
}

/**
 * GLFW keypress callback.
 * @param window GLFW window.
 * @param key Which key was pressed.
 * @param scancode Unique code for key.
 * @param action Key action: pressed, etc.
 * @param mods Modifier bits: shift, ctrl, alt, super.
 */
void keyCallback( GLFWwindow* window, int key, int scancode, int action, int mods )
{
	if( action != GLFW_PRESS && action != GLFW_REPEAT )
		return;
	
	const float rotationStep = 0.0025;
	
	switch( key )
	{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose( window, GL_TRUE );
			break;
		case GLFW_KEY_LEFT:
			rotateWithArrowKey( -rotationStep, 0 );
			break;
		case GLFW_KEY_RIGHT:
			rotateWithArrowKey( +rotationStep, 0 );
			break;
		case GLFW_KEY_UP:
			rotateWithArrowKey( 0, +rotationStep );
			break;
		case GLFW_KEY_DOWN:
			rotateWithArrowKey( 0, -rotationStep );
			break;
		case GLFW_KEY_R:
			resetArcBall();
			gZoom = 1.0;
			break;
		default: return;
	}
}

/**
 * GLFW mouse button callback.
 * @param window GLFW window.
 * @param button Which mouse button has been actioned.
 * @param action Mouse action: pressed or released.
 * @param mode Modifier bits: shift, ctrl, alt, supper.
 */
void mouseButtonCallback( GLFWwindow* window, int button, int action, int mode )
{
	if( button != GLFW_MOUSE_BUTTON_LEFT )		// Ignore mouse button other than left one.
		return;

	if( gUsingArrowKey )						// Wait for arrow keys to stop being used as rotation mechanism.
		return;

	if( action == GLFW_PRESS )
	{
		//glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_DISABLED );
		HVect arcballCoords;
		double x, y;
		int w, h;
		glfwGetWindowSize( window, &w, &h );
		glfwGetCursorPos( window, &x, &y );
		arcballCoords.x = static_cast<float>( 2.0*x/static_cast<float>(w) - 1.0 );
		arcballCoords.y = static_cast<float>( -2.0*y/static_cast<float>(h) + 1.0 );
		Ball_Mouse( gArcBall, arcballCoords );
		Ball_BeginDrag( gArcBall );
		gLocked = true;							// Determines whether the mouse movement is used for rotating the object.
	}
	else
	{
		//glfwSetInputMode( window, GLFW_CURSOR, GLFW_CURSOR_NORMAL );
		Ball_EndDrag( gArcBall );
		gLocked = false;						// Go back to normal.
	}
}

/**
 * GLFW mouse motion callback.
 * @param window GLFW window.
 * @param x Mouse x-position.
 * @param y Mouse y-position.
 */
void mousePositionCallback( GLFWwindow* window, double x, double y )
{
	if( glfwGetMouseButton( window, GLFW_MOUSE_BUTTON_LEFT ) == GLFW_PRESS && gLocked )
	{
		HVect arcballCoords;
		int w, h;
		glfwGetWindowSize( window, &w, &h );
		arcballCoords.x = static_cast<float>( 2.0*x/static_cast<float>(w) - 1.0 );
		arcballCoords.y = static_cast<float>( -2.0*y/static_cast<float>(h) + 1.0 );
		Ball_Mouse( gArcBall, arcballCoords );
		Ball_Update( gArcBall );
	}
}

/**
 * GLFW mouse scroll callback.
 * @param window GLFW window.
 * @param xOffset X offset.
 * @param yOffset Y offset.
 */
void mouseScrollCallback( GLFWwindow* window, double xOffset, double yOffset )
{
	gZoom *= (yOffset > 0)? ZOOM_IN: ZOOM_OUT;
	gZoom = max( 0.5f, min( gZoom, 2.5f ) );
}

/**
 * GLFW frame buffer resize callback.
 * @param window GLFW window.
 * @param w New frame buffer width.
 * @param h New frame buffer height.
 */
void resizeCallback( GLFWwindow* window, int w, int h )
{
	fbWidth = w;		// w and h are width and height of framebuffer, not window.
	fbHeight = h;

	//Proj = Tx::frustrum( -0.5, 0.5, -0.5, 0.5, 1.0, 100 );

	// Projection used for 3D.
	double ratio = static_cast<double>(w)/static_cast<double>(h);
	Proj = Tx::perspective( M_PI/3.0, ratio, 0.01, 1000.0 );

	// Projection used for text rendering.
	int windowW, windowH;
	glfwGetWindowSize( window, &windowW, &windowH );
	gTextScaleX = 1.0f / windowW;
	gTextScaleY = 1.0f / windowH;
}

/**
 * Render the scene.
 * @param program OpenGL program ID that contains the shaders to use for rendering the scene.
 * @param Projection The 4x4 projection matrix to use.
 * @param View The 4x4 view matrix.
 * @param Model Any previously built 4x4 model matrix (usually containing current zoom and scene rotation as provided by arcball).
 * @param LightSpaceMatrix The 4x4 Proj_light * View_light transformation matrix.
 * @param currentTime Current step.
 */
void renderScene( GLuint program, const mat44& Projection, const mat44& View, const mat44& Model, const mat44& LightSpaceMatrix, double currentTime )
{
	ogl.useProgram( program );							// Render using the shaders defined for input program.
	glEnable( GL_CULL_FACE );
	
	ogl.setColor( 1.0, 1.0, 1.0 );						// A 3D object.
	ogl.render3DObject( Projection, View, Model * Tx::translate( 0.25, 0.24, 0 ) * Tx::rotate( -0.01, Tx::Z_AXIS ) * Tx::scale( 0.75 ), LightSpaceMatrix, "bunny" );
	
	ogl.setColor( 0.0, 1.0, 0.0 );						// A green sphere.
	ogl.drawSphere( Projection, View, Model * Tx::translate( 4, 0.5, 0 ) * Tx::scale( 0.5 ), LightSpaceMatrix );
	
	ogl.setColor( 0.0, 0.0, 1.0 );						// A blue cylinder.
	ogl.drawCylinder( Projection, View, Model * Tx::translate( -4, 0.5, -0.5 ) * Tx::scale( 0.5, 0.5, 1.0 ), LightSpaceMatrix );
	
	ogl.setColor( 0.9, 0.9, 1.0 );						// Ground.
	ogl.drawCube( Projection, View, Model * Tx::translate( 0, -0.005, 0 ) * Tx::scale( 20, 0.01, 20 ), LightSpaceMatrix );
	
	double theta = 2.0 * M_PI/6.0;
	double r = 3;
	vector<vec3> points;								// A yellow hexagon.
	for( int i = 0; i <= 6; i++ )
		points.emplace_back( vec3( { r * cos( i * theta + currentTime * 0.2 ) * 0.75, r * sin( i * theta + currentTime * 0.2 ) * 0.75, 0 } ) );
	ogl.setColor( 1.0, 1.0, 0.0 );
	ogl.drawPath( Projection, View, Model * Tx::translate( 0, 2, -1 ) * Tx::rotate( M_PI/4.0, Tx::X_AXIS ), LightSpaceMatrix, points );
	
	ogl.setColor( 0.0, 1.0, 1.0, 0.5 );					// A semi-transparent cyan set of points.
	vector<vec3>::const_iterator first = points.begin();
	vector<vec3>::const_iterator last = points.end() - 1;
	ogl.drawPoints( Projection, View, Model * Tx::translate( 0, 2, -1 ) * Tx::rotate( M_PI/4.0, Tx::X_AXIS ), LightSpaceMatrix, vector<vec3>( first, last ), 20 );
}

/**
 * Application main function.
 * @param argc Number of input arguments.
 * @param argv Input arguments.
 * @return Exit code.
 */
int main( int argc, const char * argv[] )
{
	gPointOfInterest = { 0, 0, 0 };		// Camera controls globals.
	gEye = { 3, 4, 9 };
	gUp = Tx::Y_AXIS;
	
	gLocked = false;					// Track if mouse button is pressed down.
	gUsingArrowKey = false;				// Track pressing action of arrow keys.
	gZoom = 1.0;						// Camera zoom.
	
	GLFWwindow* window;
	glfwSetErrorCallback( errorCallback );
	
	if( !glfwInit() )
		exit( EXIT_FAILURE );
	
	// Indicate GLFW which version will be used and the OpenGL core only.
	glfwWindowHint( GLFW_SAMPLES, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MAJOR, 4 );
	glfwWindowHint( GLFW_CONTEXT_VERSION_MINOR, 1 );
	glfwWindowHint( GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE );
	glfwWindowHint( GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE );
	
	cout << glfwGetVersionString() << endl;

	// Create window object (with screen-dependent size metrics).
	int windowWidth = 1280;
	int windowHeight = 920;
	window = glfwCreateWindow( windowWidth, windowHeight, "Real-Time Rendering", nullptr, nullptr );

	if( !window )
	{
		glfwTerminate();
		exit( EXIT_FAILURE );
	}
	
	glfwMakeContextCurrent( window );
	glfwSwapInterval( 1 );
	
	// Hook up callbacks.
	glfwSetFramebufferSizeCallback( window, resizeCallback );
	glfwSetKeyCallback( window, keyCallback );
	glfwSetMouseButtonCallback( window, mouseButtonCallback );
	glfwSetCursorPosCallback( window, mousePositionCallback );
	glfwSetScrollCallback( window, mouseScrollCallback );

	// Initialize projection matrices and viewport.
	glfwGetFramebufferSize( window, &fbWidth, &fbHeight );
	gRetinaRatio = static_cast<float>( fbWidth ) / windowWidth;
	cout << "Retina pixel ratio: " << gRetinaRatio << endl;
	resizeCallback( window, fbWidth, fbHeight );
	
	gArcBall = new BallData;						// Initialize arcball.
	resetArcBall();
	
	///////////////////////////////////// Intialize OpenGL and rendering shaders ///////////////////////////////////////
	
	const vec3 lightPosition = { -2, 12, 12 };
	ogl.init( lightPosition );
	
	// Initialize shaders for geom/sequence drawing program.
	cout << "Initializing rendering shaders... ";
	Shaders shaders;
	GLuint renderingProgram = shaders.compile( conf::SHADERS_FOLDER + "shader.vert", conf::SHADERS_FOLDER + "shader.frag" );		// Usual rendering.
	cout << "Done!" << endl;
	
	// Initialize shaders program for shadow mapping.
	cout << "Initializing shadow mapping shaders... ";
	GLuint shadowMapProgram = shaders.compile( conf::SHADERS_FOLDER + "shadow.vert", conf::SHADERS_FOLDER + "shadow.frag" );
	cout << "Done!" << endl;
	
	/////////////////////////////////////////// Setting up shadow mapping //////////////////////////////////////////////
	
	GLuint depthMapFBO;											// Create a framebuffer for rendering the shadow map.
	glGenFramebuffers( 1, &depthMapFBO );
	
	const auto SHADOW_WIDTH = static_cast<GLuint>( fbWidth ),
		SHADOW_HEIGHT = static_cast<GLuint>( fbHeight );							// Texture size.
	
	GLuint depthMap;
	glGenTextures( 1, &depthMap );													// Generate texture and properties.
	glBindTexture( GL_TEXTURE_2D, depthMap );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER );		// By doing this, anything farther than the shadow map will appear in light.
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER );
	float borderColor[] = { 1.0f, 1.0f, 1.0f, 1.0f };								// Depth = 1.0.  So the rendering of the normal scene will produce something larger than this.
	glTexParameterfv( GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor );
	
	glBindFramebuffer( GL_FRAMEBUFFER, depthMapFBO );			// Attach texture as the framebuffer in the depth buffer.
	glFramebufferTexture2D( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthMap, 0 );
	glDrawBuffer( GL_NONE );									// We won't render any color.
	glReadBuffer( GL_NONE );
	glBindFramebuffer( GL_FRAMEBUFFER, 0 );						// Unbind.
	
	float nearPlane = 0.01f, farPlane = 1000.0f;				// Setting up the light projection matrix.
	//mat44 LightProjection = Tx::ortographic( -10, 10, -10, 10, nearPlane, farPlane );
	mat44 LightProjection = Tx::perspective( M_PI/2.0, static_cast<float>( SHADOW_WIDTH )/static_cast<float>( SHADOW_HEIGHT ), nearPlane, farPlane );
	
	int shadowMap_location = glGetUniformLocation( renderingProgram, "shadowMap" );
	glUniform1i( shadowMap_location, 0 );						// Texture will be associated to unit GL_TEXTURE0.
	
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	double currentTime = 0.0;
	const double timeStep = 0.01;
	const float textColor[] = { 0.0, 0.8, 1.0, 1.0 };
	char text[128];
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glFrontFace( GL_CCW );

	// Frame rate variables.
	long gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
	long gOldTicks = gNewTicks;
	float transcurredTimePerFrame;
	string FPS = "FPS: ";

	ogl.setUsingUniformScaling( true );							// Important! We'll be using uniform scaling in the following scene rendering.
	ogl.create3DObject( "bunny", "bunny.obj" );					// Create a 3D object model.

	// Rendering loop.
	while( !glfwWindowShouldClose( window ) )
	{
		glClearColor( 0.1f, 0.1f, 0.11f, 1.0f );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		HMatrix abr;
		Ball_Value( gArcBall, abr );
		mat44 ArcBall = {
			{ abr[0][0], abr[0][1], abr[0][2], abr[0][3] },
			{ abr[1][0], abr[1][1], abr[1][2], abr[1][3] },
			{ abr[2][0], abr[2][1], abr[2][2], abr[2][3] },
			{ abr[3][0], abr[3][1], abr[3][2], abr[3][3] } };
		mat44 Model = ArcBall.t() * Tx::scale( gZoom );
		
		//////////////////////////////////// First pass: render scene to depth map /////////////////////////////////////
		
		mat44 LightView = Tx::lookAt( lightPosition, gPointOfInterest, Tx::Y_AXIS );
		mat44 LightSpaceMatrix = LightProjection * LightView;
		
		glViewport( 0, 0, SHADOW_WIDTH, SHADOW_HEIGHT );
		glBindFramebuffer( GL_FRAMEBUFFER, depthMapFBO );
		glClear( GL_DEPTH_BUFFER_BIT );

		renderScene( shadowMapProgram, LightProjection, LightView, Model, LightSpaceMatrix, currentTime );
		glBindFramebuffer( GL_FRAMEBUFFER, 0 );				// Unbind: return control to normal draw framebuffer.

		//////////////////////////////// Second pass: render scene with shadow mapping /////////////////////////////////

		mat44 Camera = Tx::lookAt( gEye, gPointOfInterest, gUp );
		
		glViewport( 0, 0, fbWidth, fbHeight );
		glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
		
		// Enable shadow mapping texture sampler.
		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, depthMap );
		renderScene( renderingProgram, Proj, Camera, Model, LightSpaceMatrix, currentTime );

		/////////////////////////////////////////////// Rendering text /////////////////////////////////////////////////

		glUseProgram( ogl.getGlyphsProgram() );				// Switch to text rendering.  The text rendering is the only program created within the OpenGL class.

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_CULL_FACE );

		gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
		transcurredTimePerFrame = (gNewTicks - gOldTicks) / 1000.0f;
		sprintf( text, "FPS: %.2f", ( ( transcurredTimePerFrame <= 0 )? -1 : calculateFPS( transcurredTimePerFrame ) ) );
		gOldTicks = gNewTicks;

		ogl.renderText( text, ogl.atlas48, -1 + 10 * gTextScaleX, 1 - 30 * gTextScaleY, static_cast<float>( gTextScaleX * 0.6 ),
						static_cast<float>( gTextScaleY * 0.6 ), textColor );

		glDisable( GL_BLEND );

		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		glfwSwapBuffers( window );
		glfwPollEvents();
		
		currentTime += timeStep;
	}
	
	glfwDestroyWindow( window );
	glfwTerminate();
	
	// Delete OpenGL programs.
	glDeleteProgram( renderingProgram );
	
	return 0;
}

