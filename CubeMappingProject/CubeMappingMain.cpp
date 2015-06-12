/*
	If you built something upon this, consider sharing as well =)
	@tiiago11
*/

#include <stdio.h>
#include <math.h>
#include <vector>
#include <iostream>
#include "Vectors.h"
#include "Addon/glew.h"
#include "Addon/freeglut.h"
using namespace std;

// A triangle is composed by 3 vertex indexes
struct Triangle_t {
	int v1;
	int v2;
	int v3;
	Triangle_t() : v1(0), v2(0), v3(0) {}
	Triangle_t(int v1, int v2, int v3) : v1(v1), v2(v2), v3(v3) {}
	friend std::ostream& operator<<(std::ostream& os, const Triangle_t& t){
		os << "(" << t.v1 << ", " << t.v2 << ", " << t.v3 << ")";
		return os;
	}
};

/*---------------------------Global Variables-------------------------------*/
bool  g_Lighting = false;
float g_ObjMiddle[3] = { 0, 0, 0 };
float g_ObjRadius = 1;
bool  g_Wireframe = false;
Vector3 g_LowerBoundObj;

vector<Triangle_t> g_Triangles;
vector<Vector3>	   g_Vertexes;
vector<Vector3>    g_NormalsPerVertex;
GLuint             g_GroundTexId;
GLuint             g_CubeTexId;
int   g_CurrentModel = 0;
float g_Zoom = 1.;
int   g_MouseX;
int   g_MouseY;
float g_OrientationMatrix[16];
int   g_CurrentCubeMap = 0;
int	 g_SmoothingSteps = 0;

char *g_FileNames[4] = { "Models/bunny.ply", "Models/dragon.ply", "Models/happy.ply", "Models/cube.ply" };
int	  g_FileProperties[4 * 2] = { 35947, 69451, 437645, 871414, 543652, 1087716, 8, 12 };


char *g_FaceFile[6] = {
	"Textures/skybox_left.bmp", 
	"Textures/skybox_right.bmp", 
	"Textures/skybox_top.bmp", 
	"Textures/skybox_bottom.bmp", 
	"Textures/skybox_back.bmp", 
	"Textures/skybox_front.bmp"
	};

static GLenum g_FaceTarget[6] = {
	GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT,
	GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT,
	GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT
};

void SetupModelview()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(g_OrientationMatrix);
	glTranslatef(-g_ObjMiddle[0], -g_ObjMiddle[1], -g_ObjMiddle[2]);//Center the object!
}

void SetupProjection()
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-g_ObjRadius*g_Zoom, g_ObjRadius*g_Zoom, -g_ObjRadius*g_Zoom, g_ObjRadius*g_Zoom, -g_ObjRadius, g_ObjRadius*2.0f);
}

void InitMatrix(float *m)
{
	int i;
	for (i = 0; i < 16; i++)
		m[i] = 0;
	m[0] = m[5] = m[10] = m[15] = 1.;
}

void SetupLighting()
{
	float ambiant[4] = { 0.2, 0.2, 0.2, 1. };
	float diffuse[4] = { 0.7, 0.7, 0.7, 1. };
	float specular[4] = { 1, 1, 1, 1. };
	float exponent = 4;
	float lightDir[4] = { 0, 0, 1, 0 };

	glEnable(GL_COLOR);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_FRONT_AND_BACK);

	glEnable(GL_LIGHTING);
	glLightfv(GL_LIGHT0, GL_AMBIENT, ambiant);
	glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, specular);
	glLightf(GL_LIGHT0, GL_SPOT_EXPONENT, exponent);
	glLightfv(GL_LIGHT0, GL_SPOT_DIRECTION, lightDir);
	glEnable(GL_LIGHT0);
}

/*
*	Apply normal smoothing by averanging the adjacent normals;
*/
void smoothNormals(int steps){

	vector<Vector3> smoothNormals;
	for (int i = 0; i < g_Vertexes.size(); i++){
		smoothNormals.push_back(Vector3());
	}
	for (int i = 0; i < steps; i++){
		for (int j = 0; j < g_Triangles.size(); j++){
			smoothNormals.at(g_Triangles[j].v1) += (g_NormalsPerVertex.at(g_Triangles[j].v2) + g_NormalsPerVertex.at(g_Triangles[j].v3));
			smoothNormals.at(g_Triangles[j].v2) += (g_NormalsPerVertex.at(g_Triangles[j].v1) + g_NormalsPerVertex.at(g_Triangles[j].v3));
			smoothNormals.at(g_Triangles[j].v3) += (g_NormalsPerVertex.at(g_Triangles[j].v1) + g_NormalsPerVertex.at(g_Triangles[j].v2));
		}
	}
	for (int i = 0; i < smoothNormals.size(); i++)
		smoothNormals.at(i) = smoothNormals.at(i).normalize();
	g_NormalsPerVertex = smoothNormals;
}

/**
* Computes the normal of a vertex by averaging the normals of the triangles it belongs
*/
void ComputeNormalsPerVertex()
{
	cout << "Calculating per vertex normals...";
	if (!g_NormalsPerVertex.empty())
		g_NormalsPerVertex.clear();

	//Initialize the array
	for (int i = 0; i < g_Vertexes.size(); i++){
		g_NormalsPerVertex.push_back(Vector3());
	}

	// calculates the normal per triangle and add to the respectives vertexes' normals
	for (int i = 0; i < g_Triangles.size(); i++){
		/// calculate the normal of this particular triangle
		Vector3 v1(g_Vertexes[g_Triangles[i].v1]);
		Vector3 v2(g_Vertexes[g_Triangles[i].v2]);
		Vector3 v3(g_Vertexes[g_Triangles[i].v3]);
		v1 = v1 - v3;
		v2 = v2 - v3;
		Vector3 normal = v1.cross(v2);
		normal = normal.normalize();

		// sums it into each vertex belonging to this triangle
		g_NormalsPerVertex.at(g_Triangles[i].v1) += normal;
		g_NormalsPerVertex.at(g_Triangles[i].v2) += normal;
		g_NormalsPerVertex.at(g_Triangles[i].v3) += normal;
	}
	// normalization loop
	for (int i = 0; i < g_NormalsPerVertex.size(); i++){
		g_NormalsPerVertex.at(i) = g_NormalsPerVertex.at(i).normalize();
	}
	cout << "OK" << endl;
}


void testMaxMin(float* max, float* min, Vector3 v){
	if (min[0] > v.x)
		min[0] = v.x;
	if (min[1] > v.y)
		min[1] = v.y;
	if (min[2] > v.z)
		min[2] = v.z;

	if (max[0] < v.x)
		max[0] = v.x;
	if (max[1] < v.y)
		max[1] = v.y;
	if (max[2] < v.z)
		max[2] = v.z;
}

void ReadFile(char *name, int nbOfVertex, int nbOfTriangles)
{
	InitMatrix(g_OrientationMatrix);
	system("cls");
	cout << "Loading model : " << name << "....";
	FILE *f; //Using stdio but other libraries available as well
	float aux1, aux2;
	int polygonNb;
	float min[3] = { 99999, 99999, 99999 };
	float max[3] = { -99999, -99999, -99999 };

	if (!g_Triangles.empty())
		g_Triangles.clear();
	if (!g_Vertexes.empty())
		g_Vertexes.clear();

	f = fopen(name, "r");
	if (f == NULL) //File not found
	{
		printf("File not found\n");
		return;
	}

	float x, y, z;
	if (g_CurrentModel == 0){
		for (int i = 0; i < nbOfVertex; i++){
			fscanf(f, "%f %f %f %f %f\t", &x, &y, &z, &aux1, &aux2);
			g_Vertexes.push_back(Vector3(x, y, z));
			testMaxMin(max, min, g_Vertexes.at(i)); // computes max/min during the reading, saving a loop = one less O(n) loop
		}
	}
	else{
		for (int i = 0; i < nbOfVertex; i++){
			fscanf(f, "%f %f %f\t", &x, &y, &z);
			g_Vertexes.push_back(Vector3(x, y, z));
			testMaxMin(max, min, g_Vertexes.at(i)); // computes max/min during the reading, saving a loop = one less O(n) loop
		}
	}
	int v1, v2, v3;
	for (int i = 0; i < nbOfTriangles; i++){
		fscanf(f, "%d %d %d %d\t", &polygonNb, &v1, &v2, &v3);
		g_Triangles.push_back(Triangle_t(v1, v2, v3));
	}
	fclose(f);

	// find the middle point
	for (int j = 0; j < 3; j++) //Loop on each axis
		g_ObjMiddle[j] = (min[j] + max[j]) / 2.;

	g_ObjRadius = sqrt((min[0] - g_ObjMiddle[0])*(min[0] - g_ObjMiddle[0])
		+ (min[1] - g_ObjMiddle[1])*(min[1] - g_ObjMiddle[1])
		+ (min[2] - g_ObjMiddle[2])*(min[2] - g_ObjMiddle[2]));

	g_LowerBoundObj.set(min[0], min[1], min[2]);

	cout << "OK" << endl;
	cout << "Approximate Radius: " << g_ObjRadius << endl;
	cout << "Triangles: " << g_Triangles.size() << endl;
	cout << "Vertexes:  " << g_Vertexes.size() << endl;
	ComputeNormalsPerVertex();
}

//  Useful Functions
void BMPReader(char *name, unsigned char **image, int &sizeX, int &sizeY)
{
	int i;
	FILE *f;
	int hdr[0x36];

	if (*image != NULL)
	{
		delete[] * image;
		*image = NULL;
	}
	f = fopen(name, "rb");
	if (f == NULL)
	{
		return;
	}
	fread(hdr, 1, 0x12, f);
	fread(&sizeX, 1, 4, f);
	fread(&sizeY, 1, 4, f);
	fread(hdr, 1, 0x1C, f);
	*image = new unsigned char[3 * sizeX*sizeY];
	fread(*image, 1, sizeX*sizeY * 3, f);
	for (i = 0; i < sizeX*sizeY; i++)
	{
		static unsigned char aux;
		aux = (*image)[3 * i];
		(*image)[3 * i] = (*image)[3 * i + 2];
		(*image)[3 * i + 2] = aux;
	}
	fclose(f);
}

/**
	Creates a cubemap according to the current cube
	*/
void makeCubeMap()
{
	int w;
	int h;

	glGenTextures(1, &g_CubeTexId);
	glBindTexture(GL_TEXTURE_2D, g_CubeTexId);

	//load each face
	for (int i = 0; i < 6; i++){
		unsigned char *image = NULL;
		BMPReader(g_FaceFile[g_CurrentCubeMap * 6 + i], &image, w, h);
		glTexImage2D(g_FaceTarget[i], 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
		free(image);
	}
	glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
	glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
	glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);

	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_CLAMP);
}

/**
	Creates a grass texture in a quad form.
	*/
void makeGround(){
	int w;
	int h;

	unsigned char *image = NULL;
	BMPReader("Textures/skybox_bottom.bmp", &image, w, h);

	glGenTextures(1, &g_GroundTexId);
	glBindTexture(GL_TEXTURE_2D, g_GroundTexId);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, w, h, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
	free(image);
}

/*---------------------------Glut Callback functions---------------------------------*/

void Idlefunc()
{
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMultMatrixf(g_OrientationMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, g_OrientationMatrix);
	glutPostRedisplay();
}

void print_help(){
	cout << "----------------------- Key Helper -------------------------" << endl;
	cout << " ' ' = change model" << endl;
	cout << "  w  = wireframe on/off" << endl;
	cout << "  m  = switch cube map" << endl;
	cout << "  s  = smooth model's normals" << endl;
	cout << " 1 to 4 = choose the model (1=bunny,2=dragon,3=happy,4=cube)" << endl;
	cout << " -------------------------------------------------------------" << endl;
}

void Key(unsigned char c, int mousex, int mousey)
{
	switch (c)
	{
	case 27://Escape
		glFinish();
		glutPostRedisplay();
		exit(0);
		break;
	case ' ':
		g_CurrentModel = (g_CurrentModel + 1) % 4;
		ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);
		glutPostRedisplay();
		break;
	case 'w':
		g_Wireframe = !g_Wireframe;
		cout << "Wireframe " << g_Wireframe << endl;
		///wireframe control
		if (g_Wireframe)
			glPolygonMode(GL_FRONT, GL_LINE);
		else
			glPolygonMode(GL_FRONT, GL_FILL);

		glutPostRedisplay();
		break;
	case 'm':
		g_CurrentCubeMap = (g_CurrentCubeMap + 1) % 4;
		makeCubeMap();
		glutPostRedisplay();
		break;
	case '1':
		g_CurrentModel = 0;
		ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);
		glutPostRedisplay();
		break;
	case '2':
		g_CurrentModel = 1;
		ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);
		glutPostRedisplay();
		break;
	case '3':
		g_CurrentModel = 2;
		ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);
		glutPostRedisplay();
		break;
	case '4':
		g_CurrentModel = 3;
		ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);
		glutPostRedisplay();
		break;
	case 's':
		g_SmoothingSteps++;
		smoothNormals(g_SmoothingSteps);
		cout << "Smoothing loops: " << g_SmoothingSteps << endl;
		break;
	case 'h':
		print_help();
	default:
		cout << "Need some help? press 'h' " << endl;
		break;
	}
}

//Record the mouse location when a button is pushed down
void mouse(int button, int state, int x, int y)
{
	g_MouseX = x;
	g_MouseY = y;
}

void motion(int x, int y)
{
	//Perform a rotation. The new orientation matrix should be  m_orientation=m_orientation*m_mouse_interaction
	//Where m_mouse_interaction is representative of the mouse interactions in the image plane
	glMatrixMode(GL_MODELVIEW);//Use OpenGL capabilites to multiply matrices. The modelview Matrix is used here
	glLoadIdentity();
	glRotatef(sqrt((float)((x - g_MouseX)*(x - g_MouseX) + (y - g_MouseY)*(y - g_MouseY))), (y - g_MouseY), (x - g_MouseX), 0); //Rotation around an inclined axis
	glMultMatrixf(g_OrientationMatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, g_OrientationMatrix); //Get the multiplied matrix
	g_MouseX = x;	//Reinitialize the new position	
	g_MouseY = y;
	glutPostRedisplay();// Generates a refresh event (which will call draw)
}

/*
* Draw the trinangles using the normal calculated by the average of the adjacents shared faces' normals
* Uses the smooth normals
*/
void drawModel(){


	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_CUBE_MAP_EXT);
	glEnable(GL_TEXTURE_GEN_S);
	glEnable(GL_TEXTURE_GEN_T);
	glEnable(GL_TEXTURE_GEN_R);

	glBindTexture(GL_TEXTURE_2D, g_CubeTexId);

	for (int i = 0; i < g_Triangles.size(); i++){
		Triangle_t t = g_Triangles.at(i);
		glBegin(GL_TRIANGLES);
		glNormal3f(g_NormalsPerVertex.at(t.v1).x, g_NormalsPerVertex.at(t.v1).y, g_NormalsPerVertex.at(t.v1).z);
		glVertex3f(g_Vertexes.at(t.v1).x, g_Vertexes.at(t.v1).y, g_Vertexes.at(t.v1).z);
		glNormal3f(g_NormalsPerVertex.at(t.v2).x, g_NormalsPerVertex.at(t.v2).y, g_NormalsPerVertex.at(t.v2).z);
		glVertex3f(g_Vertexes.at(t.v2).x, g_Vertexes.at(t.v2).y, g_Vertexes.at(t.v2).z);
		glNormal3f(g_NormalsPerVertex.at(t.v3).x, g_NormalsPerVertex.at(t.v3).y, g_NormalsPerVertex.at(t.v3).z);
		glVertex3f(g_Vertexes.at(t.v3).x, g_Vertexes.at(t.v3).y, g_Vertexes.at(t.v3).z);
		glEnd();
	}

	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_CUBE_MAP_EXT);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
	glDisable(GL_TEXTURE_GEN_R);
}
/*
*
* Draw the texture in a side by side square
*/
void drawGround(void)
{
	float side = 0.25;
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, g_GroundTexId);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f);
	glVertex3f(-side, g_LowerBoundObj.y, -side);
	glTexCoord2f(1.0f, 0.0f);
	glVertex3f(side, g_LowerBoundObj.y, -side);
	glTexCoord2f(1.0f, 1.0f);
	glVertex3f(side, g_LowerBoundObj.y, side);
	glTexCoord2f(0.0f, 1.0f);
	glVertex3f(-side, g_LowerBoundObj.y, side);
	glEnd();

	glDisable(GL_TEXTURE_2D);
}

// draw callback
void Draw()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SetupModelview();
	SetupProjection();

	glRotated(10.0f, 1, 0, 0);
	drawGround();
	drawModel();

	glutSwapBuffers();
}

void mouseWheelCB(int a, int state, int x, int y){
	if (state > 0)
		g_Zoom *= 0.8;
	else if (state < 0)
		g_Zoom *= 1.25;
}

void Init_GL(int sizex, int sizey)
{
	glutInitWindowPosition(0, 0);
	glutInitWindowSize(sizex, sizey);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("CubeMappingDemo - tiiago11");
	glutIdleFunc(Idlefunc); //Setup a function that is called after every rendering
	glutDisplayFunc(Draw);
	glutKeyboardFunc(Key);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutMouseWheelFunc(mouseWheelCB);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LEQUAL);
}

void initTextures(){
	makeGround();
	makeCubeMap();
}

int main(int argc, char **argv)
{
	ReadFile(g_FileNames[g_CurrentModel], g_FileProperties[2 * g_CurrentModel], g_FileProperties[2 * g_CurrentModel + 1]);

	glutInit(&argc, argv);
	Init_GL(700, 700);
	SetupLighting();
	InitMatrix(g_OrientationMatrix);

	if (glewInit() != GLEW_OK)
	{
		fprintf(stderr, "INFO: OpenGL 2.0 not supported. Exit\n");
		return EXIT_FAILURE;
	}
	initTextures();

	glutMainLoop();
	system("pause");
}