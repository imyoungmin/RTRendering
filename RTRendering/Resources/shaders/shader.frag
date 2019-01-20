#version 410 core

uniform vec4 lightPosition;								// In camera coordinates.
uniform vec4 ambientProd, diffuseProd, specularProd;	// The [r,g,b,a] element-wise products k_a*I, k_d*I, and k_s*I for ambient, diffuse, and specular lighting, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool drawPoint;

uniform sampler2D shadowMap;							// Shadow map texture.

in vec3 vPosition;										// Position in view (camera) coordinates.
in vec3 vNormal;										// Normal vector in view coordinates.
in vec4 fragPosLightSpace;								// Position of fragment in light space (need w component for manual perspective division).

out vec4 color;

/**
 * Determine if current fragment is in shadow by comparing its depth value with the one from the light's point of view.
 * @param fpLightSpace Fragment position in light space: Proj_light * View_light * Model * position.
 * @return 1 - in shadow, 0 - not in shadow.
 */
float shadowCalculation( vec4 fpLightSpace )
{
	vec3 projFrag = fpLightSpace.xyz / fpLightSpace.w;			// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;							// Normalize fragment position to [0, 1].
	
	float closestDepth = texture( shadowMap, projFrag.xy ).r;	// Closest depth from light's point of view.
	float currentDepth = projFrag.z;
	
	float bias = 0.001;
	float shadow = ( currentDepth - closestDepth > bias )? 1.0: 0.0;
	
	return shadow;
}

void main( void )
{
    vec3 ambient, diffuse, specular;
    float alpha = ambientProd.a;

    if( useBlinnPhong )
    {
        vec3 N = normalize( vNormal );
        vec3 E = normalize( -vPosition.xyz );
        vec3 L = normalize( lightPosition.xyz - vPosition );

        vec3 H = normalize( L + E );
        float incidence = dot( N, L );

        // Diffuse component.
        float cDiff = max( incidence, 0.0 );
        diffuse = cDiff * diffuseProd.rgb;

        // Ambient component.
        ambient = ambientProd.rgb;

        // Specular component.
        if( incidence > 0 )
        {
        	float cSpec = pow( max( dot( N, H ), 0.0 ), shininess );
        	specular = cSpec * specularProd.rgb;
        }
        else
        	specular = vec3( 0.0, 0.0, 0.0 );
    }
    else
    {
        ambient = ambientProd.rgb;
        diffuse = diffuseProd.rgb;
        specular = vec3( 0.0, 0.0, 0.0 );
    }

	float shadow = shadowCalculation( fragPosLightSpace );
	
    // Final fragment color.
    vec3 totalColor = ambient + ( 1.0 - shadow ) * ( diffuse + specular );
    if( drawPoint )
    {
        if( dot( gl_PointCoord-0.5, gl_PointCoord - 0.5 ) > 0.25 )		// For rounded points.
        	discard;
        else
            color = vec4( totalColor, alpha );
    }
    else
        color = vec4( totalColor, alpha );
}
