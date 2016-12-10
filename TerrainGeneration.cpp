#include <iostream>
#include <fstream>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>


#include <GL/glew.h>
#include <freeglut.h>
#include <freeglut_ext.h>

#pragma comment(lib, "glew32.lib") 

using namespace std;
using namespace glm;

// Size of the terrain
const int MAP_SIZE = 10;

const int SCREEN_WIDTH = 500;
const int SCREEN_HEIGHT = 500;

static mat3 normalMat = mat3(1.0);

struct Vertex
{
	float coords[4];
	vec3 normals;
	vec2 texCoords;
};

struct Matrix4x4
{
	float entries[16];
};

static mat4 projMat = mat4(1.0);

static const Matrix4x4 IDENTITY_MATRIX4x4 =
{
	{
		1.0, 0.0, 0.0, 0.0,
		0.0, 1.0, 0.0, 0.0,
		0.0, 0.0, 1.0, 0.0,
		0.0, 0.0, 0.0, 1.0
	}
};

struct BitMapFile
{
	int sizeX;
	int sizeY;
	unsigned char *data;
};

BitMapFile *getbmp(string filename);
struct Material
{
	vec4 ambRefl;
	vec4 difRefl;
	vec4 specRefl;
	vec4 emitCols;
	float shininess;
};

static const Material terrainFandB =
{
	vec4(0.0, 0.5, 0.5, 1.0),
	vec4(0.0, 0.5, 0.5, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 0.0, 0.0, 1.0),
	10.0
};
struct Light
{
	vec4 ambCols;
	vec4 difCols;
	vec4 specCols;
	vec4 coords;
};
static const Light light0 =
{
	vec4(0.0, 0.0, 0.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(1.0, 1.0, 1.0, 1.0),
	vec4(0.0, 5.0, 0.0, 0.0)
};

static enum buffer { TERRAIN_VERTICES };
static enum object { TERRAIN };

// Globals
static Vertex terrainVertices[MAP_SIZE*MAP_SIZE] = {};

static const vec4 globAmb = vec4(0.2, 0.2, 0.2, 1.0);

const int numStripsRequired = MAP_SIZE - 1;
const int verticesPerStrip = 2 * MAP_SIZE;

unsigned int terrainIndexData[numStripsRequired][verticesPerStrip];

static unsigned int
programId,
vertexShaderId,
fragmentShaderId,
modelViewMatLoc,
normalMatLoc,
projMatLoc,
buffer[1],
vao[1],
texture[1];

static BitMapFile *image;

void shaderCompileTest(GLuint shader)
{
	GLint result = GL_FALSE;
	int logLength;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &result);
	glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
	vector<GLchar> vertShaderError((logLength > 1) ? logLength : 1);
	glGetShaderInfoLog(shader, logLength, NULL, &vertShaderError[0]);
	cout << &vertShaderError[0] << endl;
}
// Function to read text file, used to read shader files
char* readTextFile(char* aTextFile)
{
	FILE* filePointer = fopen(aTextFile, "rb");
	char* content = NULL;
	long numVal = 0;

	fseek(filePointer, 0L, SEEK_END);
	numVal = ftell(filePointer);
	fseek(filePointer, 0L, SEEK_SET);
	content = (char*)malloc((numVal + 1) * sizeof(char));
	fread(content, 1, numVal, filePointer);
	content[numVal] = '\0';
	fclose(filePointer);
	return content;
}

// Initialization routine.
void setup(void)
{
	// Initialise terrain - set values in the height map to 0
	float terrain[MAP_SIZE][MAP_SIZE] = {};
	for (int x = 0; x < MAP_SIZE; x++)
	{
		for (int z = 0; z < MAP_SIZE; z++)
		{
			terrain[x][z] = 0;
			if (z == 5 && x == 5) terrain[x][z] = 1.2;
		}
	}
	terrain[4][5] = 1;
	terrain[5][4] = 1;
	terrain[5][6] = 1;
	terrain[6][5] = 1;
	// TODO: Add your code here to calculate the height values of the terrain using the Diamond square algorithm
	

	// Intialise vertex array
	int i = 0;
	float fTextureS = float(MAP_SIZE)*0.1f;
	float fTextureT = float(MAP_SIZE)*0.1f;
	for (int z = 0; z < MAP_SIZE; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			// Set the coords (1st 4 elements) and a default colour of black (2nd 4 elements) 
			
			float fScaleC = float(x) / float(MAP_SIZE - 1);
			float fScaleR = float(z) / float(MAP_SIZE - 1);
			vec2 tex = vec2(1);
			tex.s = fScaleC*4.0f;
			tex.t = fScaleR*4.0f;
			terrainVertices[i] = { { (float)x, terrain[x][z], (float)z, 1.0 },{ 0.0, 1.0, 0.0 },tex };
			i++;
		}
	}
	//stackoverflow.com/questions/13983189/opengl-how-to-calculate-normals-in-a-terrain-height-grid 

	//TODO
	i = 0;
	for (int z = 0; z < MAP_SIZE; z++)
	{
		for (int x = 0; x < MAP_SIZE; x++)
		{
			// Set the coords (1st 4 elements) and a default colour of black (2nd 4 elements) 
			if ((z != 0) && (z != MAP_SIZE - 1) && (x != 0) && (x != MAP_SIZE - 1)) {
				float hL = terrain[x-1][z];
				float hR = terrain[x+1][z];
				float hD = terrain[x][z-1];
				float hU = terrain[x][z+1];
				vec3 N = vec3(0);
				// deduce terrain normal
				N.x = hL - hR;
				N.y = 2.0;
				N.z = hD - hU;
				N = normalize(N);
				terrainVertices[i].normals = N;
			}
			i++;
		}
	}

	//
	// Now build the index data 
	i = 0;
	for (int z = 0; z < MAP_SIZE - 1; z++)
	{
		i = z * MAP_SIZE;
		for (int x = 0; x < MAP_SIZE * 2; x += 2)
		{
			terrainIndexData[z][x] = i;
			i++;
		}
		for (int x = 1; x < MAP_SIZE * 2 + 1; x += 2)
		{
			terrainIndexData[z][x] = i;
			i++;
		}
	}

	
	glClearColor(1.0, 1.0, 1.0, 0.0);

	// Create shader program executable - read, compile and link shaders
	char* vertexShader = readTextFile("vertexShader.glsl");
	vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShaderId, 1, (const char**)&vertexShader, NULL);
	glCompileShader(vertexShaderId);
	cout << "VERTEX::" << endl;
	shaderCompileTest(vertexShaderId);

	char* fragmentShader = readTextFile("fragmentShader.glsl");
	fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShaderId, 1, (const char**)&fragmentShader, NULL);
	glCompileShader(fragmentShaderId);
	cout << "FRAGMENT::" << endl;
	shaderCompileTest(fragmentShaderId);

	programId = glCreateProgram();
	glAttachShader(programId, vertexShaderId);
	glAttachShader(programId, fragmentShaderId);
	glLinkProgram(programId);
	glUseProgram(programId);
	///////////////////////////////////////

	// Create vertex array object (VAO) and vertex buffer object (VBO) and associate data with vertex shader.
	glGenVertexArrays(1, vao);
	glGenBuffers(1, buffer);
	glBindVertexArray(vao[TERRAIN]);
	glBindBuffer(GL_ARRAY_BUFFER, buffer[TERRAIN_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, sizeof(terrainVertices), terrainVertices, GL_STATIC_DRAW);
	int t = sizeof(terrainVertices[0]);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)sizeof(terrainVertices[0].coords));
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(terrainVertices[0]), (GLvoid*)(sizeof(terrainVertices[0].coords)+sizeof(terrainVertices[0].normals)));
	glEnableVertexAttribArray(2);
	///////////////////////////////////////

	// Obtain projection matrix uniform location and set value.
	projMatLoc = glGetUniformLocation(programId, "projMat");
	projMat = perspective(radians(60.0), (double) SCREEN_WIDTH / (double)SCREEN_HEIGHT, 0.1, 100.0);
	glUniformMatrix4fv(projMatLoc, 1, GL_FALSE, value_ptr(projMat));

	///////////////////////////////////////

	// Obtain modelview matrix uniform location and set value.
	mat4 modelViewMat = mat4(1.0);
	// Move terrain into view - glm::translate replaces glTranslatef

	modelViewMat = translate(modelViewMat, vec3(-5.5f, -1.5f, -10.0f));
	modelViewMatLoc = glGetUniformLocation(programId, "modelViewMat");
	glUniformMatrix4fv(modelViewMatLoc, 1, GL_FALSE, value_ptr(modelViewMat));
	
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.ambRefl"), 1,
		&terrainFandB.ambRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.difRefl"), 1,
		&terrainFandB.difRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.specRefl"), 1,
		&terrainFandB.specRefl[0]);
	glUniform4fv(glGetUniformLocation(programId, "terrainFandB.emitCols"), 1,
		&terrainFandB.emitCols[0]);
	glUniform1f(glGetUniformLocation(programId, "terrainFandB.shininess"),
		terrainFandB.shininess);
	glUniform4fv(glGetUniformLocation(programId, "globAmb"), 1, &globAmb[0]);

	normalMatLoc = glGetUniformLocation(programId, "normalMat");
	normalMat = transpose(inverse(mat3(modelViewMat)));
	glUniformMatrix3fv(normalMatLoc, 1, GL_FALSE, value_ptr(normalMat));

	glUniform4fv(glGetUniformLocation(programId, "light0.ambCols"), 1,
		&light0.ambCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.difCols"), 1,
		&light0.difCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.specCols"), 1,
		&light0.specCols[0]);
	glUniform4fv(glGetUniformLocation(programId, "light0.coords"), 1,
		&light0.coords[0]);

	image = getbmp("Textures/grass.bmp");
	///////////////////////////////////////// Create texture ids.
	glGenTextures(1, texture);

	// Bind grass image.
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture[0]);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image->sizeX, image->sizeY, 0,
		GL_RGBA, GL_UNSIGNED_BYTE, image->data);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	int grassTexLoc = glGetUniformLocation(programId, "grassTex");
	glUniform1i(grassTexLoc, 0);
}

// Drawing routine.
void drawScene(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// For each row - draw the triangle strip
	for (int i = 0; i < MAP_SIZE - 1; i++)
	{
		glDrawElements(GL_TRIANGLE_STRIP, verticesPerStrip, GL_UNSIGNED_INT, terrainIndexData[i]);
	}

	glutSwapBuffers();
}

// OpenGL window reshape routine.
void resize(int w, int h)
{
	glViewport(0, 0, w, h);
}

// Keyboard input processing routine.
void keyInput(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27:
		exit(0);
		break;
	default:
		break;
	}
}

// Main routine.
int main(int argc, char* argv[])
{
	glutInit(&argc, argv);

	// Set the version of OpenGL (4.2)
	glutInitContextVersion(4, 2);
	// The core profile excludes all discarded features
	glutInitContextProfile(GLUT_CORE_PROFILE);
	// Forward compatibility excludes features marked for deprecation ensuring compatability with future versions
	glutInitContextFlags(GLUT_FORWARD_COMPATIBLE);

	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGBA);
	glutInitWindowSize(SCREEN_WIDTH, SCREEN_HEIGHT);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Procedural Terrain");

	// Set OpenGL to render in wireframe mode
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glutDisplayFunc(drawScene);
	glutReshapeFunc(resize);
	glutKeyboardFunc(keyInput);


	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	std::cout << glewGetErrorString(err);

	setup();

	glutMainLoop();
}
BitMapFile *getbmp(string filename)
{
	int offset, headerSize;

	// Initialize bitmap files for RGB (input) and RGBA (output).
	BitMapFile *bmpRGB = new BitMapFile;
	BitMapFile *bmpRGBA = new BitMapFile;

	// Read input bmp file name.
	ifstream infile(filename.c_str(), ios::binary);

	// Get starting point of image data in bmp file.
	infile.seekg(10);
	infile.read((char *)&offset, 4);

	// Get header size of bmp file.
	infile.read((char *)&headerSize, 4);

	// Get image width and height values from bmp file header.
	infile.seekg(18);
	infile.read((char *)&bmpRGB->sizeX, 4);
	infile.read((char *)&bmpRGB->sizeY, 4);

	// Determine the length of zero-byte padding of the scanlines 
	// (each scanline of a bmp file is 4-byte aligned by padding with zeros).
	int padding = (3 * bmpRGB->sizeX) % 4 ? 4 - (3 * bmpRGB->sizeX) % 4 : 0;

	// Add the padding to determine size of each scanline.
	int sizeScanline = 3 * bmpRGB->sizeX + padding;

	// Allocate storage for image in input bitmap file.
	int sizeStorage = sizeScanline * bmpRGB->sizeY;
	bmpRGB->data = new unsigned char[sizeStorage];

	// Read bmp file image data into input bitmap file.
	infile.seekg(offset);
	infile.read((char *)bmpRGB->data, sizeStorage);

	// Reverse color values from BGR (bmp storage format) to RGB.
	int startScanline, endScanlineImageData, temp;
	for (int y = 0; y < bmpRGB->sizeY; y++)
	{
		startScanline = y * sizeScanline; // Start position of y'th scanline.
		endScanlineImageData = startScanline + 3 * bmpRGB->sizeX; // Image data excludes padding.
		for (int x = startScanline; x < endScanlineImageData; x += 3)
		{
			temp = bmpRGB->data[x];
			bmpRGB->data[x] = bmpRGB->data[x + 2];
			bmpRGB->data[x + 2] = temp;
		}
	}

	// Set image width and height values and allocate storage for image in output bitmap file.
	bmpRGBA->sizeX = bmpRGB->sizeX;
	bmpRGBA->sizeY = bmpRGB->sizeY;
	bmpRGBA->data = new unsigned char[4 * bmpRGB->sizeX*bmpRGB->sizeY];

	// Copy RGB data from input to output bitmap files, set output A to 1.
	for (int j = 0; j < 4 * bmpRGB->sizeY * bmpRGB->sizeX; j += 4)
	{
		bmpRGBA->data[j] = bmpRGB->data[(j / 4) * 3];
		bmpRGBA->data[j + 1] = bmpRGB->data[(j / 4) * 3 + 1];
		bmpRGBA->data[j + 2] = bmpRGB->data[(j / 4) * 3 + 2];
		bmpRGBA->data[j + 3] = 0xFF;
	}

	return bmpRGBA;
}