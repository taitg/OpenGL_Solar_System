// ==========================================================================
// Vertex program for barebones GLFW boilerplate
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================
#version 410

// location indices for these attributes correspond to those specified in the
// InitializeGeometry() function of the main program
layout(location = 0) in vec3 VertexPosition; //===
layout(location = 1) in vec3 VertexColour;
layout(location = 2) in vec3 VertexTexture;

// output to be interpolated between vertices and passed to the fragment stage
out vec3 Colour;
out vec3 texCoords;
out vec3 normal;
out vec3 point;

// uniforms
uniform mat4 starsModel;
uniform mat4 sunModel;
uniform mat4 earthModel;
uniform mat4 moonModel;
uniform mat4 view;
uniform mat4 proj;

void main()
{
	mat4 mod = mat4(1.0);

	// set appropriate model matrix
	if (VertexTexture.z == 0) mod = earthModel;
	else if (VertexTexture.z == 1) mod = starsModel;
	else if (VertexTexture.z == 2) mod = moonModel;
	else if (VertexTexture.z == 3) mod = sunModel;

	// determine new position
	vec4 newPos = mod * vec4(VertexPosition, 1.0);
    gl_Position = proj * view * newPos;

	// determine surface normal
	vec4 c = mod * vec4(0.0, 0.0, 0.0, 1.0);
	normal = normalize(newPos.xyz - c.xyz);
	point = newPos.xyz;

    // assign output colour to be interpolated
    Colour = VertexColour;
	texCoords = VertexTexture;
}
