#version 410 core

in vec3 position;

uniform mat4 Model;										// Model transform takes points from model into world coordinates.
uniform mat4 View;										// View matrix takes points from world into camera coordinates.
uniform mat4 Projection;

void main( void )
{
	gl_Position = Projection * View * Model * vec4( position, 1.0 );		// Transforming all scene vertices to light space.
}
