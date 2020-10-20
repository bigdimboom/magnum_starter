
uniform mat4 uModelMat;
uniform mat4 uCamViewProjMat;
uniform vec3 uCamPos;

uniform float uGridStepSize;
uniform float uGridHeightBoosts;
uniform int uGridRez;
uniform sampler2D elevationMap;


vec2 toUV()
{
	float u = float(gl_VertexID % uGridRez) / float(uGridRez);
	float v = float(gl_VertexID / uGridRez) / float(uGridRez);
	return vec2(u, 1.0f - v);
}

vec4 terrainGrid(in float height, in float size)
{
	float x = gl_VertexID % uGridRez * size;
	float z = gl_VertexID / uGridRez * size;
	return vec4(x, height, z, 1.0f);
}

void main()
{
	gl_Position = uCamViewProjMat * uModelMat * terrainGrid(texture(elevationMap, toUV()).r * uGridHeightBoosts, uGridStepSize);
}