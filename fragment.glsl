
#version 410

const float PI = 3.1415926535897932384626433832795;

// interpolated colour received from vertex stage
in vec3 Colour;
in vec3 texCoords;
in vec3 point;
in vec3 normal;

// first output is mapped to the framebuffer's colour index by default
out vec4 FragmentColour;

// textures
uniform sampler2D tex0;	// earth
uniform sampler2D tex1;	// stars
uniform sampler2D tex2;	// moon
uniform sampler2D tex3;	// sun
uniform sampler2D tex4;	// clouds1
uniform sampler2D tex5;	// clouds2

// animation progress
uniform float animation;

// camera location
uniform vec3 camPoint;

// light source
uniform vec3 light;
uniform vec3 specColour;
uniform float ambient;
uniform float diffRatio;
uniform float intensity;
uniform float glowInt;
uniform float phong;
uniform float waterPhong;

uniform float cloudInt;


// apply lighting model
vec4 applyLighting(vec4 colour) {

	vec4 newColour = colour;
	vec3 lightDir = normalize(light - point);
	vec3 viewRay = normalize(point - camPoint);
	vec3 h = normalize(lightDir - viewRay);

	// ambient lighting
	newColour *= ambient;

	// diffuse lighting
	newColour += colour * diffRatio * intensity * max(0.0, dot(normal, lightDir));

	// specular lighting
	float p = phong;
	if (colour.b > colour.r + colour.g) p = waterPhong;
	float maxTerm = max(0.0, dot(normal, h));
	vec3 specular = specColour * intensity * pow(maxTerm, p);
	newColour += vec4(specular, 1.0);

	return newColour;
}


// apply special "glow" lighting/animation for the sun
vec4 applySunLighting(vec4 colour) {

	vec4 newColour = colour;

	if (length(newColour) > 1.7)
		newColour += newColour * glowInt * 0.7 *
					(sin(animation / 11.0) * cos(animation / 23.0) + 1.0);

	else if (length(newColour) > 1.5)
		newColour += newColour * glowInt * 1.1 *
					(cos(animation / 13.0) * sin(animation / 29.0) + 1.0);

	else if (length(newColour) > 1.4)
		newColour += newColour * glowInt * 1.5 *
					(sin(animation / 17.0) * cos(animation / 31.0) + 1.0);

	else newColour += newColour * glowInt * 2.0 *
					(cos(animation / 19.0) * sin(animation / 37.0) + 1.0);

	return newColour;
}


// get earth colour from earth and cloud textures
vec4 getEarthColour() {

	vec4 colour = texture(tex0, texCoords.xy);

	// cloud animation
	vec2 c1Coords, c2Coords;
	c1Coords.x = texCoords.x + 0.08 * 
					(texCoords.y - 0.5) *
					sin(animation * 2.0) *
					sin(animation * 2.0);
	c1Coords.y = texCoords.y + 0.01 * 
					cos(animation / 2.0);
	c2Coords.x = texCoords.x + 0.1 * 
					(texCoords.y - 0.5) *
					sin(animation * 2.0);
	c2Coords.y = texCoords.y + 0.01 * 
					cos(animation / 2.0);

	// switch between textures
	vec4 clouds1 = cloudInt *
					0.5 * (sin(animation / 2.0) + 1.0) * 
					texture(tex4, c1Coords);
	vec4 clouds2 = cloudInt *
					0.5 * (sin(animation / 2.0 + PI) + 1.0) * 
					0.5 * (sin(animation / 2.0 + PI) + 1.0) * 
					texture(tex5, c2Coords);
	clouds1.w = 1.0;
	clouds2.w = 1.0;

	return colour + clouds1 + clouds2;
}


// main function
void main(void)
{
	vec4 colour;

	// earth
	if (texCoords.z == 0)
		colour = applyLighting(getEarthColour());

	// stars
	else if (texCoords.z == 1)
		colour = (intensity + ambient) * texture(tex1, texCoords.xy);

	// moon
	else if (texCoords.z == 2)
		colour = applyLighting(texture(tex2, texCoords.xy));

	// sun
	else if (texCoords.z == 3) {
		vec2 sunCoords;

		// sun texture animation
		sunCoords.x = texCoords.x + 0.2 *
						(texCoords.y - 0.5) * 
						sin(animation / 71.0) * 
						cos(animation / 83.0);
		sunCoords.y = texCoords.y + 0.03 * 
						sin(animation / 61.0) * 
						cos(animation / 91.0);
		colour = applySunLighting(texture(tex3, sunCoords));
	}

	FragmentColour = colour;
}
