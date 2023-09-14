#include "CustomizedShaderProgram.hpp"

#include "gl_compile_program.hpp"
#include "gl_errors.hpp"

Scene::Drawable::Pipeline customized_shader_program_pipeline;

Load< CustomizedShaderProgram > customized_shader_program(LoadTagEarly, []() -> CustomizedShaderProgram const * {
	CustomizedShaderProgram *ret = new CustomizedShaderProgram();

	//----- build the pipeline template -----
	customized_shader_program_pipeline.program = ret->program;

	customized_shader_program_pipeline.OBJECT_TO_CLIP_mat4 = ret->OBJECT_TO_CLIP_mat4;
	customized_shader_program_pipeline.OBJECT_TO_LIGHT_mat4x3 = ret->OBJECT_TO_LIGHT_mat4x3;
	customized_shader_program_pipeline.NORMAL_TO_LIGHT_mat3 = ret->NORMAL_TO_LIGHT_mat3;

	/* This will be used later if/when we build a light loop into the Scene:
	customized_shader_program_pipeline.LIGHT_TYPE_int = ret->LIGHT_TYPE_int;
	customized_shader_program_pipeline.LIGHT_LOCATION_vec3 = ret->LIGHT_LOCATION_vec3;
	customized_shader_program_pipeline.LIGHT_DIRECTION_vec3 = ret->LIGHT_DIRECTION_vec3;
	customized_shader_program_pipeline.LIGHT_ENERGY_vec3 = ret->LIGHT_ENERGY_vec3;
	customized_shader_program_pipeline.LIGHT_CUTOFF_float = ret->LIGHT_CUTOFF_float;
	*/

	//make a 1-pixel white texture to bind by default:
	GLuint tex;
	glGenTextures(1, &tex);

	glBindTexture(GL_TEXTURE_2D, tex);
	std::vector< glm::u8vec4 > tex_data(1, glm::u8vec4(0xff));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);


	customized_shader_program_pipeline.textures[0].texture = tex;
	customized_shader_program_pipeline.textures[0].target = GL_TEXTURE_2D;

	return ret;
});

CustomizedShaderProgram::CustomizedShaderProgram() {
	//Compile vertex and fragment shaders using the convenient 'gl_compile_program' helper function:
	program = gl_compile_program(
		//vertex shader:
		"#version 330\n"
		"uniform mat4 OBJECT_TO_CLIP;\n"
		"uniform mat4x3 OBJECT_TO_LIGHT;\n"
		"uniform mat3 NORMAL_TO_LIGHT;\n"
		"in vec4 Position;\n"
		"in vec3 Normal;\n"
		"in vec4 Color;\n"
		"in vec2 TexCoord;\n"
		"out vec3 position;\n"
		"out vec3 normal;\n"
		"out vec4 color;\n"
		"out vec2 texCoord;\n"
		"void main() {\n"
		"	gl_Position = OBJECT_TO_CLIP * Position;\n"
		"	position = OBJECT_TO_LIGHT * Position;\n"
		"	normal = NORMAL_TO_LIGHT * Normal;\n"
		"	color = Color;\n"
		"	texCoord = TexCoord;\n"
		"}\n"
	,
		//fragment shader:
		// ref: http://amindforeverprogramming.blogspot.com/2013/07/random-floats-in-glsl-330.html
		"#version 330\n"
		"uniform sampler2D TEX;\n"
		"uniform int TIME;\n"
		"uniform int LIGHT_TYPE;\n"
		"uniform vec3 LIGHT_LOCATION;\n"
		"uniform vec3 LIGHT_DIRECTION;\n"
		"uniform vec3 LIGHT_ENERGY;\n"
		"uniform float LIGHT_CUTOFF;\n"
		"in vec3 position;\n"
		"in vec3 normal;\n"
		"in vec4 color;\n"
		"in vec2 texCoord;\n"
		"out vec4 fragColor;\n"
		"uint hash( uint x ) {\n"
		"	x += ( x << 10u );\n"
		"	x ^= ( x >>  6u );\n"
		"	x += ( x <<  3u );\n"
		"	x ^= ( x >> 11u );\n"
		"	x += ( x << 15u );\n"
		"	return x;\n"
		"}\n"
		"uint hash( uvec2 v ) {\n"
		"	return hash( v.x ^ hash(v.y) );\n"
		"}\n"
		"uint hash( uvec3 v ) {\n"
		"	return hash( v.x ^ hash(v.y) ^ hash(v.z) );\n"
		"}\n"
		"uint hash( uvec4 v ) {\n"
		"	return hash( v.x ^ hash(v.y) ^ hash(v.z) ^ hash(v.w) );\n"
		"}\n"
		"float random( float f ) {\n"
		"	const uint mantissaMask = 0x007FFFFFu;\n"
		"	const uint one          = 0x3F800000u;\n"
		"	uint h = hash( floatBitsToUint( f ) );\n"
		"	h &= mantissaMask;\n"
		"	h |= one;\n"
		"	float  r2 = uintBitsToFloat( h );\n"
		"	return r2 - 1.0;\n"
		"}\n"
		"void main() {\n"
		"	vec3 n = normalize(normal);\n"
		"	vec3 e;\n"
		"	if (LIGHT_TYPE == 0) { //point light \n"
		"		vec3 l = (LIGHT_LOCATION - position);\n"
		"		float dis2 = dot(l,l);\n"
		"		l = normalize(l);\n"
		"		float nl = max(0.0, dot(n, l)) / max(1.0, dis2);\n"
		"		e = nl * LIGHT_ENERGY;\n"
		"	} else if (LIGHT_TYPE == 1) { //hemi light \n"
		"		e = (dot(n,-LIGHT_DIRECTION) * 0.5 + 0.5) * LIGHT_ENERGY;\n"
		"	} else if (LIGHT_TYPE == 2) { //spot light \n"
		"		vec3 l = (LIGHT_LOCATION - position);\n"
		"		float dis2 = dot(l,l);\n"
		"		l = normalize(l);\n"
		"		float nl = max(0.0, dot(n, l)) / max(1.0, dis2);\n"
		"		float c = dot(l,-LIGHT_DIRECTION);\n"
		"		nl *= smoothstep(LIGHT_CUTOFF,mix(LIGHT_CUTOFF,1.0,0.1), c);\n"
		"		e = nl * LIGHT_ENERGY;\n"
		"	} else { //(LIGHT_TYPE == 3) //directional light \n"
		"		e = max(0.0, dot(n,-LIGHT_DIRECTION)) * LIGHT_ENERGY;\n"
		"	}\n"
		"	vec4 albedo = texture(TEX, texCoord) * color;\n"
		"	float r = random( hash(uvec4(position.xyz, TIME)) );\n"
		"	vec3 rColor = vec3(r,r,r);\n"
		"	fragColor = vec4((1-e)*albedo.rgb, albedo.a);\n"
		"	fragColor = vec4(e*(0.2*n+0.3*albedo.rgb+0.5*rColor), 1);\n"
		"}\n"
	);
	//As you can see above, adjacent strings in C/C++ are concatenated.
	// this is very useful for writing long shader programs inline.

	//look up the locations of vertex attributes:
	Position_vec4 = glGetAttribLocation(program, "Position");
	Normal_vec3 = glGetAttribLocation(program, "Normal");
	Color_vec4 = glGetAttribLocation(program, "Color");
	TexCoord_vec2 = glGetAttribLocation(program, "TexCoord");

	//look up the locations of uniforms:
	OBJECT_TO_CLIP_mat4 = glGetUniformLocation(program, "OBJECT_TO_CLIP");
	OBJECT_TO_LIGHT_mat4x3 = glGetUniformLocation(program, "OBJECT_TO_LIGHT");
	NORMAL_TO_LIGHT_mat3 = glGetUniformLocation(program, "NORMAL_TO_LIGHT");

	TIME_int = glGetUniformLocation(program, "TIME");
	LIGHT_TYPE_int = glGetUniformLocation(program, "LIGHT_TYPE");
	LIGHT_LOCATION_vec3 = glGetUniformLocation(program, "LIGHT_LOCATION");
	LIGHT_DIRECTION_vec3 = glGetUniformLocation(program, "LIGHT_DIRECTION");
	LIGHT_ENERGY_vec3 = glGetUniformLocation(program, "LIGHT_ENERGY");
	LIGHT_CUTOFF_float = glGetUniformLocation(program, "LIGHT_CUTOFF");


	GLuint TEX_sampler2D = glGetUniformLocation(program, "TEX");

	//set TEX to always refer to texture binding zero:
	glUseProgram(program); //bind program -- glUniform* calls refer to this program now

	glUniform1i(TEX_sampler2D, 0); //set TEX to sample from GL_TEXTURE0

	glUseProgram(0); //unbind program -- glUniform* calls refer to ??? now
}

CustomizedShaderProgram::~CustomizedShaderProgram() {
	glDeleteProgram(program);
	program = 0;
}

