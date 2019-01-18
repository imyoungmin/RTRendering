#version 410 core

in vec3 position;
in vec3 normal;

uniform mat4 ModelView;									// Model-View transform takes points from world coordinates to camera coordinates.
uniform mat3 InvTransModelView;							// Inverse-transposed 3x3 principal submatrix of ModelView matrix.
uniform mat4 Projection;
uniform float pointSize;
uniform bool useBlinnPhong;

out vec3 vPosition;			// Position in view (camera) coordinates.
out vec3 vNormal;			// Normal vector in view coordinates.

void main( void )
{
	vec4 p = vec4( position.xyz, 1.0 );					// A point.
	gl_Position = Projection * ModelView * p;

	if( useBlinnPhong )
	{
		vPosition = (ModelView * p).xyz;				// Send vertex and normal to fragment shader in camera coodinates.
		vNormal = InvTransModelView * normal;
	}

	gl_PointSize = pointSize;
}