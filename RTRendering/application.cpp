/**
 * OpenGL application.
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
const float ZOOM_IN = 1.03;
const float ZOOM_OUT = 0.97;
BallData* gArcBall;						// Arc ball.

// Framebuffer size metrics.
int fbWidth;
int fbHeight;
float gRetinaRatio;						// How many screen dots exist per OpenGL pixel.

// Frame rate variables and functions.
static const int NUM_FPS_SAMPLES = 64;
float gFpsSamples[NUM_FPS_SAMPLES];
int gCurrentSample = 0;

/**
 * Calculate the number of frames per second using a window.
 * @param dt Amount of seconds for current frame.
 * @return Frames per second.
 */
float calculateFPS( float dt )
{
	gFpsSamples[gCurrentSample % NUM_FPS_SAMPLES] = 1.0f / dt;
	float fps = 0;
	int i = 0;
	for( i = 0; i < min( NUM_FPS_SAMPLES, gCurrentSample ); i++ )
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
	
	const float rotationStep = 0.01;
	
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
}

/**
 * GLFW frame buffer resize callback.
 * @param window GLFW window.
 * @param w New frame buffer width.
 * @param h New frame buffer height.
 */
void resizeCallback( GLFWwindow* window, int w, int h )
{
	fbWidth = w;
	fbHeight = h;
	glViewport( 0, 0, fbWidth, fbHeight );		// w and h are width and height of framebuffer, not window.

	//Proj = Tx::frustrum( -0.5, 0.5, -0.5, 0.5, 1.0, 100 );

	// Projection used for 3D.
	double ratio = static_cast<double>(w)/static_cast<double>(h);
	Proj = Tx::perspective( M_PI/4.0, ratio, 0.01, 1000.0 );

	// Projection used for text rendering.
	int windowW, windowH;
	glfwGetWindowSize( window, &windowW, &windowH );
	gTextScaleX = 1.0f / windowW;
	gTextScaleY = 1.0f / windowH;
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
	gEye = { 0, 0, 10 };
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
	int windowWidth = 1080;
	int windowHeight = 720;
	window = glfwCreateWindow( windowWidth, windowHeight, "OpenGL", nullptr, nullptr );

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
	
	gArcBall = new BallData;						// Initialize arc ball.
	resetArcBall();
	
	OpenGL::init();									// Initialize application OpenGL.
	
	double currentTime = 0.0;
	const double timeStep = 0.01;
	const float color[] = { 0.1, 0.1, 0.11, 1.0 };
	const float one = 1.0;
	const float white[] = {1.0, 1.0, 1.0, 1.0};
	
	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );
	glFrontFace( GL_CCW );

	// Frame rate variables.
	long gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
	long gOldTicks = gNewTicks;
	float transcurredTimePerFrame;
	string FPS = "FPS: ";

	OpenGL::setUsingUniformScaling( true );						// Important! We'll be using uniform scaling in the following scene rendering.
	OpenGL::create3DObject( "bunny", "bunny.obj" );		// Create a 3D object model.

	// Rendering loop.
	while( !glfwWindowShouldClose( window ) )
	{
		glfwPollEvents();

		glClearBufferfv( GL_COLOR, 0, color );
		glClearBufferfv( GL_DEPTH, 0, &one );
		
		////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
		mat44 Camera = Tx::lookAt( gEye, gPointOfInterest, gUp );
		HMatrix abr;
		Ball_Value( gArcBall, abr );
		mat44 ArcBall = {
			{ abr[0][0], abr[0][1], abr[0][2], abr[0][3] },
			{ abr[1][0], abr[1][1], abr[1][2], abr[1][3] },
			{ abr[2][0], abr[2][1], abr[2][2], abr[2][3] },
			{ abr[3][0], abr[3][1], abr[3][2], abr[3][3] } };
		mat44 Model = ArcBall.t() * Tx::scale( gZoom );

		/////////////////////////////////////////////// Rendering scene ////////////////////////////////////////////////

		glUseProgram( OpenGL::getRenderingProgram() );		// Enable geom/sequence rendering.
		glBindVertexArray( OpenGL::getRenderingVao() );
		glEnable( GL_CULL_FACE );

		OpenGL::setColor( 1.0, 1.0, 1.0 );					// A 3D object.
		OpenGL::render3DObject( Proj, Camera, Model * Tx::translate( 0.25, -0.5, 0 ) * Tx::scale( 0.75 ), "bunny" );

		OpenGL::setColor( 0.0, 1.0, 0.0 );					// A green sphere.
		OpenGL::drawSphere( Proj, Camera, Model * Tx::translate( 2, 0, 0 ) * Tx::scale( 0.5 ) );

		OpenGL::setColor( 0.0, 0.0, 1.0 );					// A blue cylinder.
		OpenGL::drawCylinder( Proj, Camera, Model * Tx::translate( -2, 0, -0.5 ) * Tx::scale( 0.5, 0.5, 1.0 ) );

		double theta = 2.0 * M_PI/6.0;
		double r = 3;
		vector<vec3> points;								// A yellow hexagon.
		for( int i = 0; i <= 6; i++ )
			points.emplace_back( vec3( { r * cos( i * theta + currentTime ) * 1.1, r * sin( i * theta + currentTime ) * 1.1, 0 } ) );
		OpenGL::setColor( 1.0, 1.0, 0.0 );
		OpenGL::drawPath( Proj, Camera, Model, points );

		vector<vec3> points2;								// A magenta hexagon.
		double phase = theta / 2.0;
		for( int i = 0; i <= 6; i++ )
			points2.emplace_back( vec3( { r * cos( i * theta + phase ), r * sin( i * theta + phase ), 0 } ) );
		OpenGL::setColor( 1.0, 0.0, 1.0 );
		OpenGL::drawPath( Proj, Camera, Model, points2 );

		OpenGL::setColor( 0.0, 1.0, 1.0, 0.5 );				// A semi-transparent cyan set of points.
		vector<vec3>::const_iterator first = points.begin();
		vector<vec3>::const_iterator last = points.end() - 1;
		OpenGL::drawPoints( Proj, Camera, Model, vector<vec3>( first, last ), 20 );

		/////////////////////////////////////////////// Rendering text /////////////////////////////////////////////////

		glUseProgram( OpenGL::getGlyphsProgram() );		// Switch to text rendering.
		glBindVertexArray( OpenGL::getGluphsVao() );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_CULL_FACE );

		gNewTicks = duration_cast<milliseconds>( system_clock::now().time_since_epoch() ).count();
		transcurredTimePerFrame = (gNewTicks - gOldTicks) / 1000.0f;
		ostringstream fpsStr;
		gCurrentSample++;
		fpsStr << FPS << calculateFPS( transcurredTimePerFrame );
		gOldTicks = gNewTicks;

		OpenGL::renderText( fpsStr.str().c_str(), OpenGL::atlas24, -1 + 10 * gTextScaleX, -1 + 10 * gTextScaleY, gTextScaleX, gTextScaleY, white );

		glDisable( GL_BLEND );

		glfwSwapBuffers( window );
		
		currentTime += timeStep;
	}
	
	OpenGL::finish();								// Release OpenGL resources.
	
	glfwDestroyWindow( window );
	glfwTerminate();
	
	return 0;
}

