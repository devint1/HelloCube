//
// Created by Devin Tuchsen on 8/6/16.
//
// Texture source: https://www.filterforge.com/filters/6903.html
//

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <math.h>
#include <stdlib.h>

#pragma pack(0)
typedef struct _tex_hdr {
	uint32_t width, height;
	uint32_t format,  type;
} tex_hdr;

const static GLchar *VERTEX_SOURCE =
	"precision mediump float;"
	"uniform mat4 u_MVPMatrix;"
	"uniform mat4 u_MVMatrix;"
	"attribute vec4 a_Position;"
	"attribute vec2 a_TexCoordinate;"
	"varying vec3 v_Position;"
	"varying vec2 v_TexCoordinate;"
	"void main() {"
		"v_Position = vec3(u_MVMatrix * a_Position);"
		"v_TexCoordinate = a_TexCoordinate;"
		"gl_Position = u_MVPMatrix * a_Position;"
	"}";

const static GLchar *FRAGMENT_SOURCE =
	"precision mediump float;"
	"uniform sampler2D u_Texture;"
	"uniform sampler2D u_NormalMap;"
	"varying vec3 v_Position;"
	"varying vec2 v_TexCoordinate;"
	"void main() {"
		"vec3 lightPos = vec3(1.0, 1.0, 1.0);"
		"vec3 lightDirection = normalize(lightPos - v_Position);"
		"vec3 eyePos = vec3(0.0, 0.0, 0.0);"
		"vec3 eyeDirection = normalize(eyePos - v_Position);"
		"vec3 lightColor = vec3(1.0, 1.0, 1.0);"
		"vec3 ambient = vec3(0.25, 0.25, 0.25);"
		"vec3 diffuseCoeff = vec3(0.6, 0.6, 0.6);"
		"vec3 specCoeff = vec3(2.0, 2.0, 2.0);"
		"float specExp = 150.0;"
		"vec3 normal = normalize(vec3(texture2D(u_NormalMap, v_TexCoordinate)));"
		"float distance = length(lightPos - v_Position);"
		"float attenuation = (1.0 / (1.0 + (0.25 * distance * distance)));"
		"float diffuseComponent, specComponent;"
		"if (dot(lightDirection, normal) <= 0.0) {"
			"diffuseComponent = specComponent = 0.0;"
		"} else {"
			"diffuseComponent = max(dot(lightDirection, normal), 0.0);"
			"specComponent = max(pow(dot(reflect(lightDirection, normal), eyeDirection), specExp), 0.0);"
		"}"
		"gl_FragColor = vec4((diffuseComponent * diffuseCoeff + specComponent * specCoeff) * attenuation * lightColor + ambient, 1.0) * texture2D(u_Texture, v_TexCoordinate);"
	"}";

static GLuint texture, normalMap;
static GLuint vertexShader, fragmentShader;
static GLuint program;
static GLuint mvpMatrixLoc, mvMatrixLoc, positionLoc, texCoordLoc, texLoc, normalMapLoc;
static GLfloat mvMatrix[16];
static GLfloat pMatrix[16];
static GLfloat mvpMatrix[16];

static const GLfloat vertices[] =  {
		// f 1/1 2/2 3/3
		0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, 0.5f,
		-0.5f, -0.5f, 0.5f,
		// f 8/4 7/5 6/2
		-0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, 0.5f,
		0.5f, 0.5f, 0.5f,
		// f 5/6 6/7 2/2
		0.5f, 0.5f, -0.5f,
		0.5f, 0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		// f 2/1 6/6 7/7
		0.5f, -0.5f, 0.5f,
		0.5f, 0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		// f 3/7 7/6 8/1
		-0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		-0.5f, 0.5f, -0.5f,
		// f 1/2 4/7 8/6
		0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f,
		// f 4/6 1/1 3/3
		-0.5f, -0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		// f 5/1 8/4 6/2
		0.5f, 0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f,
		0.5f, 0.5f, 0.5f,
		// f 1/1 5/6 2/2
		0.5f, -0.5f, -0.5f,
		0.5f, 0.5f, -0.5f,
		0.5f, -0.5f, 0.5f,
		// f 3/2 2/1 7/7
		-0.5f, -0.5f, 0.5f,
		0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, 0.5f,
		// f 4/2 3/7 8/1
		-0.5f, -0.5f, -0.5f,
		-0.5f, -0.5f, 0.5f,
		-0.5f, 0.5f, -0.5f,
		// f 5/1 1/2 8/6
		0.5f, 0.5f, -0.5f,
		0.5f, -0.5f, -0.5f,
		-0.5f, 0.5f, -0.5f
};
static const GLfloat texCoords[] = {
		// f 1/1 2/2 3/3
		0.0f, 0.0f,
		1.0f, 0.0f,
		1.0f, 1.0f,
		// f 8/4 7/5 6/2
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		// f 5/6 6/7 2/2
		0.0f, 1.0f,
		1.0f, 1.0f,
		1.0f, 0.0f,
		// f 2/1 6/6 7/7
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 1.0f,
		// f 3/7 7/6 8/1
		1.0f, 1.0f,
		0.0f, 1.0f,
		0.0f, 0.0f,
		// f 1/2 4/7 8/6
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 1.0f,
		// f 4/6 1/1 3/3
		0.0f, 1.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		// f 5/1 8/4 6/2
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		// f 1/1 5/6 2/2
		0.0f, 0.0f,
		0.0f, 1.0f,
		1.0f, 0.0f,
		// f 3/2 2/1 7/7
		1.0f, 0.0f,
		0.0f, 0.0f,
		1.0f, 1.0f,
		// f 4/2 3/7 8/1
		1.0f, 0.0f,
		1.0f, 1.0f,
		0.0f, 0.0f,
		// f 5/1 1/2 8/6
		0.0f, 0.0f,
		1.0f, 0.0f,
		0.0f, 1.0f,
};

void multMatrix(GLfloat *result, const GLfloat *srcA, const GLfloat *srcB)
{
	int i, j, k;
	GLfloat tmp[16];

	memset(tmp, 0, sizeof(tmp));
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			for (k = 0; k < 4; ++k) {
				tmp[i * 4 + j] += srcA[i * 4 + k] * srcB[k * 4 + j];
			}
		}
	}
	memcpy(result, tmp, sizeof(tmp));
}

void translateMatrix(GLfloat *result, GLfloat x, GLfloat y, GLfloat z) {
	GLfloat trans[16];

	memset(trans, 0, sizeof(trans));
	trans[0] = 1.0f;
	trans[12] = x;
	trans[5] = 1.0f;
	trans[13] = y;
	trans[10] = 1.0f;
	trans[14] = z;
	trans[15] = 1.0f;

	multMatrix(result, result, trans);
}

void frustumMatrix(GLfloat *result, GLfloat left, GLfloat right, GLfloat bottom, GLfloat top,
				   GLfloat nearZ, GLfloat farZ) {
	GLfloat deltaX = right - left;
	GLfloat deltaY = top - bottom;
	GLfloat deltaZ = farZ - nearZ;
	GLfloat frust[16];

	if ((nearZ <= 0.0f) || (farZ <= 0.0f) ||
		(deltaX <= 0.0f) || (deltaY <= 0.0f) || (deltaZ <= 0.0f))
		return;

	frust[0] = 2.0f * nearZ / deltaX;
	frust[1] = 0.0f;
	frust[2] = 0.0f;
	frust[3] = 0.0f;

	frust[4] = 0.0f;
	frust[5] = 2.0f * nearZ / deltaY;
	frust[6] = 0.0f;
	frust[7] = 0.0f;

	frust[8] = (right + left) / deltaX;
	frust[9] = (top + bottom) / deltaY;
	frust[10] = -(nearZ + farZ) / deltaZ;
	frust[11] = -1.0f;

	frust[12] = 0.0f;
	frust[13] = 0.0f;
	frust[14] = -2.0f * nearZ * farZ / deltaZ;
	frust[15] = 0.0f;

	multMatrix(result, result, frust);
}

void identityMatrix(GLfloat *result) {
	result[0] = 1.0f;
	result[1] = 0.0f;
	result[2] = 0.0f;
	result[3] = 0.0f;
	result[4] = 0.0f;
	result[5] = 1.0f;
	result[6] = 0.0f;
	result[7] = 0.0f;
	result[8] = 0.0f;
	result[9] = 0.0f;
	result[10] = 1.0f;
	result[11] = 0.0f;
	result[12] = 0.0f;
	result[13] = 0.0f;
	result[14] = 0.0f;
	result[15] = 1.0f;
}

void rotationMatrix(GLfloat *result, GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	GLfloat sinAngle, cosAngle;
	GLfloat mag = sqrtf(x * x + y * y + z * z);

	if (mag > 0.0f) {
		sinAngle = sinf(angle * (GLfloat) M_PI / 180.0f);
		cosAngle = cosf(angle * (GLfloat) M_PI / 180.0f);
		GLfloat xx, yy, zz, xy, yz, zx, xs, ys, zs;
		GLfloat oneMinusCos;
		GLfloat rotMat[16];

		x /= mag;
		y /= mag;
		z /= mag;

		xx = x * x;
		yy = y * y;
		zz = z * z;
		xy = x * y;
		yz = y * z;
		zx = z * x;
		xs = x * sinAngle;
		ys = y * sinAngle;
		zs = z * sinAngle;
		oneMinusCos = 1.0f - cosAngle;

		rotMat[0] = (oneMinusCos * xx) + cosAngle;
		rotMat[1] = (oneMinusCos * xy) - zs;
		rotMat[2] = (oneMinusCos * zz) + ys;
		rotMat[3] = 0.0f;

		rotMat[4] = (oneMinusCos * xy) + zs;
		rotMat[5] = (oneMinusCos * yy) + cosAngle;
		rotMat[6] = (oneMinusCos * yz) - xs;
		rotMat[7] = 0.0f;

		rotMat[8] = (oneMinusCos * zx) - ys;
		rotMat[9] = (oneMinusCos * yz) + xs;
		rotMat[10] = (oneMinusCos * zz) + cosAngle;
		rotMat[11] = 0.0f;

		rotMat[12] = 0.0f;
		rotMat[13] = 0.0f;
		rotMat[14] = 0.0f;
		rotMat[15] = 1.0f;

		multMatrix(result, result, rotMat);
	}
}

void scaleMatrix(GLfloat *result, GLfloat sx, GLfloat sy, GLfloat sz)
{
	result[0] *= sx;
	result[1] *= sx;
	result[2] *= sx;
	result[3] *= sx;

	result[4] *= sy;
	result[5] *= sy;
	result[6] *= sy;
	result[7] *= sy;

	result[8] *= sz;
	result[9] *= sz;
	result[10] *= sz;
	result[11] *= sz;
}

void Java_tuchsen_hellocube_GLView_drawFrame(JNIEnv *env, jobject obj) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glUseProgram(program);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);
	glUniform1i(texLoc, 0);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glUniform1i(normalMapLoc, 1);
	glEnableVertexAttribArray(positionLoc);
	glEnableVertexAttribArray(texCoordLoc);
	glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, texCoords);
	rotationMatrix(mvMatrix, 1.0f, 0.5f, 1.0f, 0.0f);
	translateMatrix(mvMatrix, 0.0f, 0.0f, -1.4f);
	multMatrix(mvpMatrix, mvMatrix, pMatrix);
	translateMatrix(mvMatrix, 0.0f, 0.0f, 1.4f);
	glUniformMatrix4fv(mvpMatrixLoc, 1, GL_FALSE, mvpMatrix);
	glUniformMatrix4fv(mvMatrixLoc, 1, GL_FALSE, mvMatrix);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glDisableVertexAttribArray(positionLoc);
	glDisableVertexAttribArray(texCoordLoc);
}

void Java_tuchsen_hellocube_GLView_init(JNIEnv *env, jobject obj, jobject am, jint w, jint h) {
	unsigned char *buf;
	AAssetManager *assetManager;
	AAsset *asset;
	GLfloat x0, x1, y0, y1;
	size_t npixels;
	tex_hdr hdr;

	// Shaders
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &VERTEX_SOURCE, NULL);
	glCompileShader(vertexShader);
	GLint status;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
	if (GL_FALSE == status) {
		GLchar log[256];
		glGetShaderInfoLog(vertexShader, 256, NULL, log);
		glDeleteShader(vertexShader);
	}
	fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &FRAGMENT_SOURCE, NULL);
	glCompileShader(fragmentShader);
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
	if (GL_FALSE == status) {
		GLchar log[256];
		glGetShaderInfoLog(fragmentShader, 256, NULL, log);
		glDeleteShader(fragmentShader);
	}

	// Program
	program = glCreateProgram();
	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)    {
		GLchar log[256];
		glGetProgramInfoLog(program, 256, NULL, log);

		glDeleteProgram(program);
		program = 0;
	}
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// Locations
	mvpMatrixLoc = (GLuint) glGetUniformLocation(program, "u_MVPMatrix");
	mvMatrixLoc = (GLuint) glGetUniformLocation(program, "u_MVMatrix");
	positionLoc = (GLuint) glGetAttribLocation(program, "a_Position");
	texCoordLoc = (GLuint) glGetAttribLocation(program, "a_TexCoordinate");
	texLoc = (GLuint) glGetUniformLocation(program, "u_Texture");
	normalMapLoc = (GLuint) glGetUniformLocation(program, "u_NormalMap");

	// Textures
	assetManager = AAssetManager_fromJava(env, am);
	asset = AAssetManager_open(assetManager, "brick.tex", AASSET_MODE_BUFFER);
	AAsset_read(asset, &hdr, sizeof(hdr));
	npixels = hdr.width * hdr.height * (hdr.format == GL_RGB ? 3 : 4);
	buf = malloc(npixels);
	AAsset_read(asset, buf, npixels);
	AAsset_close(asset);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexImage2D(GL_TEXTURE_2D, 0, hdr.format, hdr.width, hdr.height, 0, hdr.format, hdr.type, buf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	asset = AAssetManager_open(assetManager, "normal.tex", AASSET_MODE_BUFFER);
	AAsset_read(asset, &hdr, sizeof(hdr));
	npixels = hdr.width * hdr.height * (hdr.format == GL_RGB ? 3 : 4);
	buf = realloc(buf, npixels);
	AAsset_read(asset, buf, npixels);
	AAsset_close(asset);
	glGenTextures(1, &normalMap);
	glBindTexture(GL_TEXTURE_2D, normalMap);
	glTexImage2D(GL_TEXTURE_2D, 0, hdr.format, hdr.width, hdr.height, 0, hdr.format, hdr.type, buf);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	free(buf);

	// Matrices
	identityMatrix(pMatrix);
	y1 = 0.01f * tanf(75.0f * (float) M_PI / 360.0f);
	y0 = -y1;
	x0 = y0;
	x1 = y1;
	frustumMatrix(pMatrix, x0, x1, y0, y1, 0.01f, 10000.0f);
	if (w > h) {
		scaleMatrix(pMatrix, (float) h / (float) w, 1.0f, 1.0f);
	} else {
		scaleMatrix(pMatrix, 1.0f, (float) w / (float) h, 1.0f);
	}
	identityMatrix(mvMatrix);

	glViewport(0, 0, w, h);
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
}
