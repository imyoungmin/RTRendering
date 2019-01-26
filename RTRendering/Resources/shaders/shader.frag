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

///////////////////////////////////////// Percentage Closer Soft Shadows ///////////////////////////////////////////////

const float W_LIGHT = 127.0;
const float NEAR = 0.01;

/**
 * Get the average blocker depth values that are closer to the light than current depth.
 * @param projFrag Fragment position in normalized coordinates [0, 1].
 * @param texelSize Size of a texel.
 * @return Average blocker depth or -1 if no blockers were found.
 */
float blockerSearch( vec3 projFrag, vec2 texelSize )
{
	float currentDepth = projFrag.z;							// Receiver distance from light.
	float blockerSearchSize = W_LIGHT * ( currentDepth - NEAR ) / currentDepth;
	int b = int( max( 1.0, blockerSearchSize / 2.0 ) );			// Boundary for search window.
	float sumBlockerDepth = 0.0;
	int blockerTexelsCount = 0;
	for( int x = -b; x <= b; x++ )								// Iterate over the cells of the search window.
	{
		for( int y = -b; y <=b; y++ )
		{
			float d = texture( shadowMap, projFrag.xy + vec2( x, y ) * texelSize ).r;	// Depth to test.
			if( currentDepth > d )								// A blocker? Closer to light.
			{
				sumBlockerDepth += d;							// Accumulate blockers depth.
				blockerTexelsCount++;
			}
		}
	}
	
	return ( blockerTexelsCount > 0 )? sumBlockerDepth / blockerTexelsCount : -1.0;
}

/**
 * Determine if current fragment is in shadow by comparing its depth value with the one from the light's point of view.
 * @param fpLightSpace Fragment position in light space: Proj_light * View_light * Model * position.
 * @param incidence Dot product between normal and light direction unit vectors.
 * @return A shadow percentage.
 */
float shadowCalculation( vec4 fpLightSpace, float incidence )
{
	vec3 projFrag = fpLightSpace.xyz / fpLightSpace.w;			// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;							// Normalize fragment position to [0, 1].
	
	if( projFrag.z > 1.0 )
		return 0.0;												// Anything farther than the light frustrum should be lit.
	
	float currentDepth = projFrag.z;

	float bias = max( 0.006 * ( 1.0 - incidence ), 0.0015 );

	float shadow = 0.0;											// Accumulate shadow evaluations.
	vec2 texelSize = 1.0 / textureSize( shadowMap, 0 );			// Retrieve the size of a texel.
	float dBlocker = blockerSearch( projFrag, texelSize );
	if( dBlocker > 0 )											// Blockers detected?
	{
		int wPenumbra = int( ( currentDepth - dBlocker ) / dBlocker * W_LIGHT );
//		int wPenumbra = 15;
		int wB = int( max( 1.0, wPenumbra / 2.0 ) );
		for( int x = -wB; x <= wB; x++ )						// Iterate over a square window of texels center at the current one.
		{
			for( int y = -wB; y <= wB; y++ )
			{
				float pcfDepth = texture( shadowMap, projFrag.xy + vec2( x, y ) * texelSize ).r;
				shadow += ( currentDepth - pcfDepth > bias )? 1.0 : 0.0;
			}
		}
		shadow /= ( pow( 2.0 * wB + 1.0, 2.0 ) );
	}

	return shadow;
}

/**
 * Main function.
 */
void main( void )
{
    vec3 ambient, diffuse, specular;
    float alpha = ambientProd.a;
    float shadow;

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

        shadow = shadowCalculation( fragPosLightSpace, incidence );
    }
    else
    {
        ambient = ambientProd.rgb;
        diffuse = diffuseProd.rgb;
        specular = vec3( 0.0, 0.0, 0.0 );
        shadow = shadowCalculation( fragPosLightSpace, 1 );
    }
	
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
