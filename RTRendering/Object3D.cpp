//
// Created by Im YoungMin on 2019-01-10.
//

#include "Object3D.h"

/**
 * Default constructor.
 */
Object3D::Object3D() = default;

/**
 * 3D model constructor.
 * @param type Unique kind name for this model.
 * @param filename OBJ filename.
 */
Object3D::Object3D( const char* type, const char* filename )
{
	kind = string( type );

	// Load the 3D model from the provided filename.
	cout << "Loading 3D model \"" << kind << "\" from file: \"" << filename << "\"... " << endl;
	vector<vec3> vertices, normals;		// Output vectors.
	vector<vec2> uvs;
	loadOBJ( filename, vertices, uvs, normals );

	// Allocate a buffer and load data into it.
	glGenBuffers( 1, &(bufferID) );
	glBindBuffer( GL_ARRAY_BUFFER, bufferID );
	vector<float> vertexPositions;
	vector<float> textureCoordinates;
	vector<float> normalComponents;
	verticesCount = getData( vertices, uvs, normals, vertexPositions, textureCoordinates, normalComponents );

	// So far we don't allocate texture coordinates.
	const size_t size = sizeof(float) * vertexPositions.size();				// Size of arrays in bytes.
	glBufferData( GL_ARRAY_BUFFER, 2 * size, nullptr, GL_STATIC_DRAW );
	glBufferSubData( GL_ARRAY_BUFFER, 0, size, vertexPositions.data() );	// Copy positions and normals data.
	glBufferSubData( GL_ARRAY_BUFFER, size, size, normalComponents.data() );
}

/**
 * Read the 3D object vertices, uv coordinates, and vector normals.
 * @param filename 3D object filename.
 * @param outVertices Vector of 3D vertices.
 * @param outUVs Vector of 2D texture coordinates.
 * @param outNormals Vector of 3D normals.
 */
void Object3D::loadOBJ( const char* filename, vector<vec3 >& outVertices, vector<vec2>& outUVs, vector<vec3>& outNormals ) const
{
	vector<int> vertexIndices, uvIndices, normalIndices;	// Auxiliary variables.
	vector<vec3> temp_vertices;
	vector<vec2> temp_uvs;
	vector<vec3> temp_normals;

	ifstream objFile( conf::OBJECTS_FOLDER + string( filename ) );
	if( objFile.is_open() )
	{
		string line;
		regex ws_re( "\\s+" ), slash_re("/");		// Tokenization regular expressions.
		size_t nFaces = 0;
		while( getline( objFile, line ) )
		{
			line = trim( line );					// Remove trailing spaces and split line into tokens.
			if( line.empty() )
				continue;
			vector<string> tokens{ sregex_token_iterator( line.begin(), line.end(), ws_re, -1 ), {} };

			// Actions depending on headline.
			if( tokens[0] == "v" )					// A vertex? v -1.000000 1.000000 -1.000000
			{
				vec3 vertex = { stof(tokens[1]), stof(tokens[2]), stof(tokens[3]) };
				temp_vertices.push_back( vertex );
			}
			else
			{
				if( tokens[0] == "vt" )				// Texture coordinate? vt 0.748953 0.250920
				{
					vec2 uv = { stof(tokens[1]), stof(tokens[2]) };
					temp_uvs.push_back( uv );
				}
				else
				{
					if( tokens[0] == "vn" )			// A normal vector? vn -0.000000 -1.000000 0.000000
					{
						vec3 normal = { stof(tokens[1]), stof(tokens[2]), stof(tokens[3]) };
						temp_normals.push_back( normal );
					}
					else
					{
						if( tokens[0] == "f" )		// A face? f 5/1/1 1/2/1 4/3/1
						{
							// Split indices information.
							vector<string> i1{ sregex_token_iterator( tokens[1].begin(), tokens[1].end(), slash_re, -1 ), {} };
							vector<string> i2{ sregex_token_iterator( tokens[2].begin(), tokens[2].end(), slash_re, -1 ), {} };
							vector<string> i3{ sregex_token_iterator( tokens[3].begin(), tokens[3].end(), slash_re, -1 ), {} };

							if( i1.size() != 3 || i2.size() != 3 || i3.size() != 3 )	// Check appropriate format of a face.
							{
								cerr << "Face format is not of the form vi/uvi/ni" << endl;
								exit( EXIT_FAILURE );
							}

							if( i1[0].empty() || i2[0].empty() || i3[0].empty() )		// Check we are receiving the vertex coordinates.
							{
								cerr << "Face f " << tokens[1] << " " << tokens[2] << " " << tokens[3] << " is missing verteices index information" << endl;
								exit( EXIT_FAILURE );
							}

							if( i1[2].empty() || i2[2].empty() || i3[2].empty() )		// Check we are receiving the normals.
							{
								cerr << "Face f " << tokens[1] << " " << tokens[2] << " " << tokens[3] << " is missing normals index information" << endl;
								exit( EXIT_FAILURE );
							}

							// Loading vertex index information.
							vertexIndices.push_back( stoi( i1[0] ) );
							vertexIndices.push_back( stoi( i2[0] ) );
							vertexIndices.push_back( stoi( i3[0] ) );

							// We can live without texture information.
							if( !( i1[1].empty() || i2[1].empty() || i3[1].empty() ) )
							{
								uvIndices.push_back( stoi( i1[1] ) );
								uvIndices.push_back( stoi( i2[1] ) );
								uvIndices.push_back( stoi( i3[1] ) );
							}

							// Loading normal index information.
							normalIndices.push_back( stoi( i1[2] ) );
							normalIndices.push_back( stoi( i2[2] ) );
							normalIndices.push_back( stoi( i3[2] ) );

							nFaces++;
						}
					}
				}
			}
		}

		objFile.close();

		if( uvIndices.size() != nFaces )		// Did we read an inconsistent number of UV texture indices?
		{
			cout << "WARNING! The UV information is incomplete or missing -- it'll be ignored" << endl;
			uvIndices.clear();
			temp_uvs.clear();
		}

		// Now processing the data we read from the object file.
		// For each vertex of each triangle:
		for( size_t i = 0; i < vertexIndices.size(); i++ )
		{
			int vertexIndex = vertexIndices[i];				// Building vertices.
			vec3 vertex = temp_vertices[ vertexIndex-1 ];
			outVertices.push_back( vertex );

			if( !uvIndices.empty() )
			{
				int uvIndex = uvIndices[i];					// Building texture coordinates.
				vec2 uv = temp_uvs[ uvIndex-1 ];
				outUVs.push_back( uv );
			}

			int normalIndex = normalIndices[i];				// Building normals.
			vec3 normal = temp_normals[ normalIndex-1 ];
			outNormals.push_back( normal );

		}

		cout << "Finished loading " << nFaces << " triangles!" << endl;
	}
	else
	{
		cerr << "Unable to open file " << filename << endl;
		exit( EXIT_FAILURE );
	}
}

/**
 * Retrieve the buffer ID, which contains the rendering information for this kind of 3D object model.
 * @return OpenGL Buffer ID.
 */
GLuint Object3D::getBufferID() const
{
	return bufferID;
}

/**
 * Retrieve the number of vertices for this 3D object model.
 * @return Number of vertices.
 */
GLsizei Object3D::getVerticesCount() const
{
	return verticesCount;
}

/**
 * Trim trailing spaces from a string.
 * @param str String to trim.
 * @param whitespace Character considered whitespace.
 * @return Trimmed string.
 */
string Object3D::trim( const string& str, const string& whitespace )
{
	const auto strBegin = str.find_first_not_of( whitespace );
	if( strBegin == string::npos )
		return ""; // no content

	const auto strEnd = str.find_last_not_of( whitespace );
	const auto strRange = strEnd - strBegin + 1;

	return str.substr( strBegin, strRange );
}

/**
 * Collect the vertex, uv, and normal coordinates into linear vectors of scalars.
 * @param inVs Input 3D vertex positions.
 * @param inUVs Input 2D texture coordinates.
 * @param inNs Input 3D vertex normals.
 * @param outVs Flat x, y, and z vertex coordinates.
 * @param outUVs Flat u and v texture coordinates per vertex.
 * @param outNs Flat x, y, and z components of the normal vectors.
 * @return Number of processed vertices.
 */
GLsizei Object3D::getData( const vector<vec3>& inVs, const vector<vec2>& inUVs, const vector<vec3>& inNs, vector<float>& outVs, vector<float>& outUVs, vector<float>& outNs ) const
{
	GLsizei N = static_cast<GLsizei>( inVs.size() );

	for( int i = 0; i < N; i++ )
	{
		// Vertices.
		outVs.push_back( inVs[i][0] );			// X-coordinate.
		outVs.push_back( inVs[i][1] );			// Y-coordinate.
		outVs.push_back( inVs[i][2] );			// Z-coordinate.

		// Texture coordinates (if existent).
		if( !inUVs.empty() )
		{
			outUVs.push_back( inUVs[i][0] );	// U-coordinate.
			outUVs.push_back( inUVs[i][1] );	// V-coordinate.
		}

		// Normals.
		outNs.push_back( inNs[i][0] );			// X-coordinate.
		outNs.push_back( inNs[i][1] );			// Y-coordinate.
		outNs.push_back( inNs[i][2] );			// Z-coordinate.
	}

	return N;
}
