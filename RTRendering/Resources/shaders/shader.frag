#version 410 core

uniform vec4 lightPosition0;							// In camera coordinates.
uniform vec3 lightColor0;								// Only RGB.
uniform vec4 ambient, diffuse, specular;				// The [r,g,b,a] ambient, diffuse, and specular material properties, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool useTexture;
uniform bool drawPoint;

uniform sampler2D shadowMap0;							// Shadow map textures for ith light.
uniform sampler2D objectTexture;						// 3D object texture.

in vec3 vPosition;										// Position in view (camera) coordinates.
in vec3 vNormal;										// Normal vector in view coordinates.
in vec2 oTexCoords;

in vec4 fragPosLightSpace0;								// Position of fragment in light space (need w component for manual perspective division).

out vec4 color;

///////////////////////////////////////// Percentage Closer Soft Shadows ///////////////////////////////////////////////

#define BLOCKER_SEARCH_NUM_SAMPLES  31
#define PCF_NUM_SAMPLES 			62
#define NEAR_PLANE 					0.01
#define LIGHT_WORLD_SIZE 			4.0
#define LIGHT_FRUSTUM_WIDTH 		30.0
#define LIGHT_SIZE_UV 				(LIGHT_WORLD_SIZE / LIGHT_FRUSTUM_WIDTH)	// Assuming that LIGHT_FRUSTUM_WIDTH = LIGHT_FRUSTUM_HEIGHT.

// Used for sampling the depth/shadow map.
const vec2 poissonDisk[BLOCKER_SEARCH_NUM_SAMPLES] = vec2[BLOCKER_SEARCH_NUM_SAMPLES](
																					vec2( 0.19483898, -0.04906884),
																					vec2(-0.20130208, -0.1598451 ),
																					vec2(-0.17657379,  0.27214397),
																					vec2(-0.18434988,  0.71673575),
																					vec2(0.19165408,  0.89863354),
																					vec2(-0.58109718,  0.63340388),
																					vec2(0.05128605,  0.51784401),
																					vec2(0.63354365,  0.5275925 ),
																					vec2(-0.51379029,  0.1419501 ),
																					vec2(0.69485186,  0.13043893),
																					vec2(-0.88038342,  0.3311689 ),
																					vec2(0.10940117, -0.63964106),
																					vec2(0.34065359,  0.36201536),
																					vec2(-0.85766868, -0.19484638),
																					vec2(0.68075919,  0.9383508 ),
																					vec2(0.98666111,  0.70115573),
																					vec2(0.93154108,  0.40278772),
																					vec2(0.50832938, -0.35158726),
																					vec2(-0.55546935, -0.39379395),
																					vec2(-0.38763822,  0.96256016),
																					vec2(-0.81259892, -0.80838567),
																					vec2(-0.99363224,  0.79686303),
																					vec2(-0.4778399 , -0.95310737),
																					vec2(-0.75429473,  0.98810816),
																					vec2(0.81623723, -0.72246694),
																					vec2(0.05209179, -0.96911855),
																					vec2(0.10548147, -0.33657712),
																					vec2(-0.20686522, -0.49231428),
																					vec2(0.97070736, -0.24451268),
																					vec2(0.44274798, -0.77235927),
																					vec2(-0.83493721, -0.50384213)
);

const vec2 poissonDisk2[PCF_NUM_SAMPLES] = vec2[PCF_NUM_SAMPLES](
																vec2( -0.41894735,  0.28361862 ),
																vec2( -0.24148383,  0.05461596 ),
																vec2( -0.58308818, -0.0852975  ),
																vec2( -0.91874502,  0.05268024 ),
																vec2( -0.04767359,  0.21626245 ),
																vec2( -0.15423918, -0.21534419 ),
																vec2(  0.04333529, -0.53046382 ),
																vec2( -0.37547078, -0.35958815 ),
																vec2( -0.63740364,  0.27292947 ),
																vec2(  0.21069004,  0.43992648 ),
																vec2( -0.73911603, -0.35945667 ),
																vec2( -0.59656022,  0.51906849 ),
																vec2(  0.34975911, -0.50447884 ),
																vec2(  0.17560473, -0.03183017 ),
																vec2( -0.89441073,  0.41783122 ),
																vec2( -0.24461117, -0.69711611 ),
																vec2( -0.97468813, -0.44716258 ),
																vec2(  0.41110872,  0.12066805 ),
																vec2(  0.40341027, -0.14889646 ),
																vec2( -0.77569146,  0.66697568 ),
																vec2( -0.96399972, -0.18848533 ),
																vec2( -0.35779514, -0.12931476 ),
																vec2( -0.02976176,  0.56043754 ),
																vec2(  0.53670582,  0.52940395 ),
																vec2( -0.17354002, -0.45539534 ),
																vec2(  0.10326228,  0.85196112 ),
																vec2( -0.69624443, -0.5855882  ),
																vec2(  0.1881653 , -0.32044992 ),
																vec2(  0.71666727,  0.21630712 ),
																vec2( -0.45256602, -0.5994996  ),
																vec2( -0.03826519, -0.02934504 ),
																vec2( -0.47159284, -0.8340908  ),
																vec2(  0.27824399,  0.65858582 ),
																vec2(  0.54459319, -0.38588128 ),
																vec2( -0.95978971, -0.82648911 ),
																vec2( -0.2054803 ,  0.82165196 ),
																vec2(  0.97567113,  0.41926589 ),
																vec2(  0.38543113, -0.75498238 ),
																vec2( -0.75175631, -0.98538239 ),
																vec2(  0.72957447, -0.02317733 ),
																vec2( -0.78320064,  0.95469406 ),
																vec2(  0.94696986, -0.18339134 ),
																vec2( -0.23056447,  0.44452964 ),
																vec2( -0.50502084,  0.92455126 ),
																vec2(  0.19367209,  0.20346628 ),
																vec2( -0.21444   , -0.92398083 ),
																vec2(  0.9990128 ,  0.07870851 ),
																vec2(  0.80038941, -0.40781422 ),
																vec2(  0.92947054,  0.62736324 ),
																vec2( -0.95951865,  0.82246055 ),
																vec2(  0.9262233 , -0.73090587 ),
																vec2(  0.84220073, -0.94588656 ),
																vec2(  0.64896845, -0.7283036  ),
																vec2(  0.05390996, -0.81932572 ),
																vec2(  0.54171418, -0.97042362 ),
																vec2(  0.37663224,  0.9652776  ),
																vec2(  0.25923189, -0.93416817 ),
																vec2(  0.52885537,  0.78992843 ),
																vec2(  0.87417148,  0.87986249 ),
																vec2( -0.42900654,  0.70587327 ),
																vec2(  0.75055635,  0.51088842 ),
																vec2(  0.64325796,  0.98972451 )
);

/**
 * Parallel plane estimatino of penumbra size.
 * @param zReceiver Current fragment depth in normalized coordinates [0, 1].
 * @param zBlocker Average blocker depth.
 * @return Triangle similarity estimation of proportion for penumbra size.
 */
float penumbraSize( float zReceiver, float zBlocker ) //Parallel plane estimation
{
	return ( zReceiver - zBlocker ) / zBlocker;
}

/**
 * Get the average blocker depth values that are closer to the light than current depth.
 * @param shadowMap Shadow map texture sampler to use for current search.
 * @param uv Fragment position in normalized coordinates [0, 1].
 * @param zReceiver Depth of current fragment in normalized coordinates [0, 1].
 * @param bias If given, evaluation bias to prevent 'depth' acne.
 * @return Average blocker depth or -1 if no blockers were found.
 */
float findBlockerDepth( sampler2D shadowMap, vec2 uv, float zReceiver, float bias )
{
	// Uses similar triangles to compute what area of the shadow map we should search.
	float searchWidth = LIGHT_SIZE_UV * ( zReceiver - NEAR_PLANE ) / 1.5;
	float blockerSum = 0, numBlockers = 0;
	
	for( int i = 0; i < BLOCKER_SEARCH_NUM_SAMPLES; i++ )
	{
		float shadowMapDepth = texture( shadowMap, uv + poissonDisk[i] * searchWidth ).r;
		if( zReceiver - shadowMapDepth > bias )				// A blocker? Closer to light.
		{
			blockerSum += shadowMapDepth;					// Accumulate blockers depth.
			numBlockers++;
		}
	}
	
	return ( numBlockers > 0 )? blockerSum / numBlockers : -1.0;
}

/**
 * Apply percentage closer filter to a set of samples around the current fragment (in the shadow map).
 * @param shadowMap Shadow map sampler to use for current filtering operation.
 * @param uv Fragment position in normalized coordinates [0 ,1] with respect to light projected space.
 * @param zReceiver Fragment's depth value in light projected space.
 * @param filterRadiusUV Sampling radius around the fragment's position in shadow map.
 * @param bias Evaluation bias to prevent shadow acne.
 * @return Percentage of shadow to be assigned to fragment.
 */
float applyPCFilter( sampler2D shadowMap, vec2 uv, float zReceiver, float filterRadiusUV, float bias )
{
	float shadow = 0;
	for( int i = 0; i < PCF_NUM_SAMPLES; i++ )
	{
		vec2 offset = poissonDisk2[i] * max( bias/4, filterRadiusUV );
		float pcfDepth = texture( shadowMap, uv + offset ).r;
		shadow += ( zReceiver - pcfDepth > bias )? 1.0 : 0.0;
	}
	return shadow / PCF_NUM_SAMPLES;
}

/**
 * Percentage closer soft shadow method.
 * @param shadowMap Shadow map texture sampler to use for PCSS.
 * @param coords Fragment 3D position in projected light space.
 * @param incidence Dot product of light and normal vectors at fragment to be rendered.
 * @return Shadow percentage for fragment (1: Completely in shadow, 0: Completely lit).
 */
float PCSS( sampler2D shadowMap, vec4 coords, float incidence )
{
	vec3 projFrag = coords.xyz / coords.w;			// Perspective division: fragment is in [-1, +1].
	projFrag = projFrag * 0.5 + 0.5;				// Normalize fragment position to [0, 1].
	
	vec2 uv = projFrag.xy;
	float zReceiver = projFrag.z;
	
	if( zReceiver > 1.0 )							// Anything farther than the light frustrum should be lit.
		return 0;
	
	float bias = max( 0.005 * ( 1.0 - incidence ), 0.0004 );
	
	// Step 1: Blocker search.
	float avgBlockerDepth = findBlockerDepth( shadowMap, uv, zReceiver, 0.0 );
	if( avgBlockerDepth < 0 )						// There are no occluders so early out (this saves filtering).
		return 0;
	
	// Step 2: Penumbra size.
	float penumbraRatio = penumbraSize( zReceiver, avgBlockerDepth );
	float filterRadiusUV = penumbraRatio * LIGHT_SIZE_UV * NEAR_PLANE / zReceiver;
	
	// Step 3: Filtering.
	return applyPCFilter( shadowMap, uv, zReceiver, filterRadiusUV, bias );
}

/**
 * Apply color given a selected light and shadow map.
 * @param shadowMap Shadow map sampler to read depth values from.
 * @param fragPosLightSpace Fragment position in selected projected light space coordinates.
 * @param lightColor RGB color of light source.
 * @param lightPosition 3D coordinates of light source with respect to the camera.
 * @param N Normalized normal vector to current fragment (if using Blinn-Phong shading) in camera coordinates.
 * @param E Normalized view direction (if using Blinn-Phong shading) in camera coordinates.
 * @return Fragment color (minus ambient component).
 */
vec3 shade( sampler2D shadowMap, vec4 fragPosLightSpace, vec3 lightColor, vec3 lightPosition, vec3 N, vec3 E )
{
	vec3 diffuseColor = diffuse.rgb,
		 specularColor = specular.rgb;
	float shadow;
	
	if( useBlinnPhong )
	{
		vec3 L = normalize( lightPosition - vPosition );
		
		vec3 H = normalize( L + E );
		float incidence = dot( N, L );
		
		// Diffuse component.
		float cDiff = max( incidence, 0.0 );
		diffuseColor = cDiff * ( (useTexture)? texture( objectTexture, oTexCoords ).rgb * diffuseColor : diffuseColor );
		
		// Specular component.
		if( incidence > 0 && shininess > 0.0 )		// Negative shininess turns off specular component.
		{
			float cSpec = pow( max( dot( N, H ), 0.0 ), shininess );
			specularColor = cSpec * specularColor;
		}
		else
			specularColor = vec3( 0.0, 0.0, 0.0 );
		
		shadow = PCSS( shadowMap, fragPosLightSpace, incidence );
	}
	else
	{
		specularColor = vec3( 0.0, 0.0, 0.0 );
		shadow = PCSS( shadowMap, fragPosLightSpace, 1 );
	}
	
	// Fragment color with respect to this light (excluding ambient component).
	return ( 1.0 - shadow ) * ( diffuseColor + specularColor ) * lightColor;
}

/**
 * Main function.
 */
void main( void )
{
	vec3 ambientColor = ambient.rgb;		// Ambient component is constant across lights.
    float alpha = ambient.a;
	vec3 N, E;								// Unit-length normal and eye direction (only necessary for shading with Blinn-Phong reflectance model).
	
	if( useBlinnPhong )
	{
		N = normalize( vNormal );
		E = normalize( -vPosition );
	}
	
    // Final fragment color is the sum of light contributions.
    vec3 totalColor = ambientColor + shade( shadowMap0, fragPosLightSpace0, lightColor0, lightPosition0.xyz, N, E );	// Light 0.
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
