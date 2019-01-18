#version 410 core

uniform vec4 lightPosition;								// In camera coordinates.
uniform vec4 ambientProd, diffuseProd, specularProd;	// The [r,g,b,a] element-wise products k_a*I, k_d*I, and k_s*I for ambient, diffuse, and specular lighting, respectively.
uniform float shininess;
uniform bool useBlinnPhong;
uniform bool drawPoint;

in vec3 vPosition;			// Position in view (camera) coordinates.
in vec3 vNormal;			// Normal vector in view coordinates.

out vec4 color;

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

    // Final fragment color.
    vec3 totalColor = ambient + diffuse + specular;
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