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

#define BLOCKER_SEARCH_NUM_SAMPLES  16
#define PCF_NUM_SAMPLES 			16
#define NEAR_PLANE 					0.01
#define LIGHT_WORLD_SIZE 			7.0
#define LIGHT_FRUSTUM_WIDTH 		30.0
#define LIGHT_SIZE_UV 				(LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)	// Assuming that LIGHT_FRUSTUM_WIDTH = LIGHT_FRUSTUM_HEIGHT.

// Used for sampling the depth/shadow map.
const vec2 poissonDisk[BLOCKER_SEARCH_NUM_SAMPLES] = vec2[BLOCKER_SEARCH_NUM_SAMPLES](
	vec2( -0.94201624, -0.39906216 ),
	vec2( 0.94558609, -0.76890725 ),
	vec2( -0.094184101, -0.92938870 ),
	vec2( 0.34495938, 0.29387760 ),
	vec2( -0.91588581, 0.45771432 ),
	vec2( -0.81544232, -0.87912464 ),
	vec2( -0.38277543, 0.27676845 ),
	vec2( 0.97484398, 0.75648379 ),
	vec2( 0.44323325, -0.97511554 ),
	vec2( 0.53742981, -0.47373420 ),
	vec2( -0.26496911, -0.41893023 ),
	vec2( 0.79197514, 0.19090188 ),
	vec2( -0.24188840, 0.99706507 ),
	vec2( -0.81409955, 0.91437590 ),
	vec2( 0.19984126, 0.78641367 ),
	vec2( 0.14383161, -0.14100790 )
);

float penumbraSize( float zReceiver, float zBlocker ) //Parallel plane estimation
{
	return ( zReceiver - zBlocker ) / zBlocker;
}

float findBlockerDepth( vec2 uv, float zReceiver )
{
	// Uses similar triangles to compute what area of the shadow map we should search.
	float searchWidth = LIGHT_SIZE_UV * ( zReceiver - NEAR_PLANE ) / ( zReceiver * 2.5 );
	float blockerSum = 0, numBlockers = 0;
	
	for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++ )
	{
		float shadowMapDepth = texture( shadowMap, uv + poissonDisk[i] * searchWidth ).r;
		if( zReceiver > shadowMapDepth )					// A blocker? Closer to light.
		{
			blockerSum += shadowMapDepth;					// Accumulate blockers depth.
			numBlockers++;
		}
	}
	
	return ( numBlockers > 0 )? blockerSum / numBlockers : -1.0;
}

float applyPCFilter( vec2 uv, float zReceiver, float filterRadiusUV, float incidence )
{
	float shadow = 0;
	float bias = max( 0.006 * ( 1.0 - incidence ), 0.0015 );
	for( int i = 0; i < PCF_NUM_SAMPLES; i++ )
	{
		vec2 offset = poissonDisk[i] * filterRadiusUV;
		float pcfDepth = texture( shadowMap, uv + offset ).r;
		shadow += ( zReceiver - pcfDepth > bias )? 1.0 : 0.0;
	}
	return shadow / PCF_NUM_SAMPLES;
}

float PCSS( vec4 coords, float incidence )
{
	vec3 projFrag = coords.xyz / coords.w;			// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;				// Normalize fragment position to [0, 1].
	
	vec2 uv = projFrag.xy;
	float zReceiver = projFrag.z;
	
	if( zReceiver > 1.0 )							// Anything farther than the light frustrum should be lit.
		return 0;
	
	// Step 1: Blocker search.
	float avgBlockerDepth = findBlockerDepth( uv, zReceiver );
	if( avgBlockerDepth < 0 )						// There are no occluders so early out (this saves filtering).
		return 0;
	
	// Step 2: Penumbra size.
	float penumbraRatio = penumbraSize( zReceiver, avgBlockerDepth );
	float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / zReceiver;
	
	// Step 3: Filtering.
	return applyPCFilter( uv, zReceiver, filterRadiusUV, incidence );
}

/**
 * Get the average blocker depth values that are closer to the light than current depth.
 * @param projFrag Fragment position in normalized coordinates [0, 1].
 * @param texelSize Size of a texel.
 * @return Average blocker depth or -1 if no blockers were found.
 */
/*float blockerSearch( vec3 projFrag, vec2 texelSize )
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
/*float shadowCalculation( vec4 fpLightSpace, float incidence )
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

        shadow = PCSS( fragPosLightSpace, incidence );
    }
    else
    {
        ambient = ambientProd.rgb;
        diffuse = diffuseProd.rgb;
        specular = vec3( 0.0, 0.0, 0.0 );
        shadow = PCSS( fragPosLightSpace, 1 );
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
