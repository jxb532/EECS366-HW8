/*

EECS 366/466 COMPUTER GRAPHICS
Template for Assignment 8-MAPPING
Spring 2006

*/

#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <math.h>

#include <string.h>
#include "glut.h"
#include <windows.h>
#include <GL/glu.h>
#include <GL/gl.h>

#include "glprocs.h"
#include "main.h"
#include "read_tga.h"
#include "Vector3.h"

#define PI 3.14159265359
#define TWOPI 6.28318530718

#define PrintOpenGLError()::PrintOGLError(__FILE__, __LINE__)

using namespace std;

//object related information
int verts, faces, norms;    // Number of vertices, faces and normals in the system
point *vertList, *normList; // Vertex and Normal Lists
faceStruct *faceList;	    // Face List

//Illimunation and shading related declerations
char *shaderFileRead(char *fn);
GLuint vertex_shader,fragment_shader,p;
int illimunationMode = 0;
int shadingMode = 0;
int lightSource = 0;
int program=-1;

//Parameters for Copper (From: "Computer Graphics Using OpenGL" BY F.S. Hill, Jr.) 
GLfloat ambient_cont [] = {0.19125,0.0735,0.0225};
GLfloat diffuse_cont [] = {0.7038,0.27048,0.0828};
GLfloat specular_cont [] = {0.256777,0.137622,0.086014};
GLfloat exponent = 12.8;

//Projection, camera contral related declerations
int WindowWidth,WindowHeight;
bool LookAtObject = false;

float CameraRadius = 10;
float CameraTheta = PI / 2;
float CameraPhi = PI / 2;
int MouseX = 0;
int MouseY = 0;
bool MouseLeft = false;
bool MouseRight = false;

//0 texture mapping - plane, planar mapping
//1 texture mapping - sphere, planar mapping
//2 texture mapping - teapot, planar mapping
//3 texture mapping - sphere, spherical mapping
//4 texture mapping - teapot, spherical mapping
//5 environment mapping - sphere, sphere map
//6 environment mapping - teapot, sphere map
//7 bump mapping - plane
//8 environment mapping - sphere, cube map [EXTRA CREDIT]
//9 environment mapping - teapot, cube map [EXTRA CREDIT]
//10 bump mapping - sphere [EXTRA CREDIT]
int mapMode = 0;

float binormal_loc;
float tangent_loc;

TGA* getTGAForMode() {
	char* filename;

	switch (mapMode) {
	case 0:
	case 1:
	case 2:
		glutSetWindowTitle("Assignment 8 - Planar Texture Map");
		filename = "./planartexturemap/abstract2.tga";
		break;
	case 3:
	case 4:
		glutSetWindowTitle("Assignment 8 - Spherical Texture Map");
		filename = "./sphericaltexturemap/earth2.tga";
		break;
	case 5:
	case 6:
		glutSetWindowTitle("Assignment 8 - Spherical Environment Map");
		filename = "./sphericalenvironmentmap/house2.tga";
		break;
	case 7:
		glutSetWindowTitle("Assignment 8 - Planar Bump Map");
		filename = "./planarbumpmap/abstract2.tga";
		break;
	case 8:
	case 9:
		//TODO figure out how to do the other cube faces
		glutSetWindowTitle("Assignment 8 - Cubic Environment Map");
		filename = "./cubicenvironmentmap/cm_back2.tga";
		break;
	case 10:
		glutSetWindowTitle("Assignment 8 - Sphere Bump Map");
		filename = "./sphericalbumpmap/earth2.tga";
		break;
	default:
		filename = "./planartexturemap/abstract2.tga";
		break;
	}
	return new TGA(filename);
}

void mapTexture(point &p, float &u, float &v, uint width, uint height, Vector3 reflection) {
	float radius = sqrtf(p.x * p.x + p.y * p.y + p.z * p.z);
	switch (mapMode) {
	case 0:
	case 1:
	case 2:
		// planar
		u = p.x;
		v = p.y;
		break;
	case 3:
	case 4:
		// spherical texture
		//u = acosf(p.z / sqrtf(p.x * p.x + p.y * p.y + p.z * p.z));
		//v = atanf(p.y / p.x);
		v = acos(p.z/radius) / PI;
		if (p.y >= 0)
			u = acos(p.x/(radius * sin(PI * v))) / TWOPI;
		else
			u = (PI + acos(p.x/(radius * sin(PI * v)))) / TWOPI;
		break;
	case 5:
	case 6:
		// spherical environment
		float m = 2.0 * sqrtf(reflection[0] * reflection[0] + reflection[1] * reflection[1] + (reflection[2] + 1.0) * (reflection[2] + 1.0));
		u = (reflection[0] / m + 0.5);
		v = (reflection[1] / m + 0.5);
		break;
	}
}

void bump(TGA *tga, point &p, point &n) {
	uint w = tga->GetWidth();
	uint h = tga->GetHeigth();
	uint u = (uint)((uint)(p.x - (w / 2)) % w);
	uint v = (uint)((uint)(p.y - (h / 2)) % h);
	uint pu = (u > 0) ? u - 1 : w;
	uint pv = (v > 0) ? v - 1 : h;
	byte *b = tga->GetPixels();
	byte i = b[w * u + v];
	byte piu = b[w * pu + v];
	byte piv = b[w * u + pv];
	byte du = (i - piu) * w;
	byte dv = (i - piv) * h;

	// TODO: Perturb

}


void DisplayFunc(void) 
{
    GLuint id ;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	//load projection and viewing transforms
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
        
	gluPerspective(60,(GLdouble) WindowWidth/WindowHeight,0.01,10000);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(CameraRadius*cos(CameraTheta)*sin(CameraPhi),
			  CameraRadius*cos(CameraPhi),
			  CameraRadius*sin(CameraTheta)*sin(CameraPhi),
			  0,0,0,
			  0,1,0);

	glEnable(GL_DEPTH_TEST);	
	glEnable(GL_TEXTURE_2D);

	setParameters(program);

	// Load image from tga file
	TGA *TGAImage = getTGAForMode();

	// Use to dimensions of the image as the texture dimensions
	uint width	= TGAImage->GetWidth();
	uint height	= TGAImage->GetHeigth();
	
	// The parameters for actual textures are changed

	glGenTextures(1, &id);

	glBindTexture(GL_TEXTURE_2D, id);


	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);

	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);


	// Finaly build the mipmaps
	glTexImage2D (GL_TEXTURE_2D, 0, 4, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, TGAImage->GetPixels());

	gluBuild2DMipmaps(GL_TEXTURE_2D, 4, width, height, GL_RGBA, GL_UNSIGNED_BYTE, TGAImage->GetPixels());

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	glEnable( GL_TEXTURE_2D );

	glBindTexture (GL_TEXTURE_2D, id); 

    delete TGAImage;

	point camera = point();
	camera.x = CameraRadius*cos(CameraTheta)*sin(CameraPhi);
	camera.y = CameraRadius*cos(CameraPhi);
	camera.z = CameraRadius*sin(CameraTheta)*sin(CameraPhi);

	TGA *bumpMap = NULL;
	if (mapMode == 7) {
		bumpMap = new TGA("./planarbumpmap/abstract_gray2.tga");
	}

	for (int i = 0; i < faces; i++)
	{
		
		glBegin(GL_TRIANGLES);
			point v1, v2, v3, n1, n2, n3;
			v1 = vertList[faceList[i].v1];
			v2 = vertList[faceList[i].v2];
			v3 = vertList[faceList[i].v3];
			n1 = vertList[faceList[i].v1];
			n2 = vertList[faceList[i].v2];
			n3 = vertList[faceList[i].v3];
			float u, v;

			if (bumpMap) {
				bump(bumpMap, v1, n1);
				bump(bumpMap, v2, n2);
				bump(bumpMap, v2, n2);
			}

			glVertexAttrib3fARB(tangent_loc, 1.0, 0.0, 0.0);
			glVertexAttrib3fARB(binormal_loc, 0.0, 1.0, 0.0);
			glNormal3f(n1.x, n1.y, n1.z);
			Vector3 view = Vector3(v1.x - camera.x, v1.y - camera.y, v1.z - camera.z);
			Vector3 normal = Vector3(n1.x, n1.y, n1.z);
			float d = normal.dot(view);
			Vector3 reflection = view - (normal * 2.0 * d);
			mapTexture(v1, u, v, width, height, reflection);
			glTexCoord2f (u, v);
			glVertex3f(v1.x, v1.y, v1.z);

			glVertexAttrib3fARB(tangent_loc, 1.0, 0.0, 0.0);
			glVertexAttrib3fARB(binormal_loc, 0.0, 1.0, 0.0);
			glNormal3f(n2.x, n2.y, n2.z);
			view = Vector3(v2.x - camera.x, v2.y - camera.y, v2.z - camera.z);
			normal = Vector3(n2.x, n2.y, n2.z);
			d = normal.dot(view);
			reflection = view - (normal * 2.0 * d);
			mapTexture(v2, u, v, width, height, reflection);
			glTexCoord2f (u, v);
			glVertex3f(v2.x, v2.y, v2.z);

			glVertexAttrib3fARB(tangent_loc, 1.0, 0.0, 0.0);
			glVertexAttrib3fARB(binormal_loc, 0.0, 1.0, 0.0);
			glNormal3f(n3.x, n3.y, n3.z);
			view = Vector3(v3.x - camera.x, v3.y - camera.y, v3.z - camera.z);
			normal = Vector3(n3.x, n3.y, n3.z);
			d = normal.dot(view);
			reflection = view - (normal * 2.0 * d);
			mapTexture(v3, u, v, width, height, reflection);
			glTexCoord2f (u, v);
			glVertex3f(v3.x, v3.y, v3.z);
		glEnd();

	}

	//glutSolidTeapot(1);
	setParameters(program);
	glutSwapBuffers();
}

void ReshapeFunc(int x,int y)
{
    glViewport(0,0,x,y);
    WindowWidth = x;
    WindowHeight = y;
}


void MouseFunc(int button,int state,int x,int y)
{
	MouseX = x;
	MouseY = y;

    if(button == GLUT_LEFT_BUTTON)
		MouseLeft = !(bool) state;
	if(button == GLUT_RIGHT_BUTTON)
		MouseRight = !(bool) state;
}

void MotionFunc(int x, int y)
{
	if(MouseLeft)
	{
        CameraTheta += 0.01*PI*(MouseX - x);
		CameraPhi += 0.01*PI*(MouseY - y);
		if (CameraPhi > (PI - 0.01))
			CameraPhi = PI - 0.01;
		if (CameraPhi < 0.01)
			CameraPhi = 0.01;
	}
	if(MouseRight)
	{
        CameraRadius += 0.2*(MouseY-y);
		if(CameraRadius <= 0)
			CameraRadius = 0.2;
	}
    
	MouseX = x;
	MouseY = y;

	glutPostRedisplay();
}





//Motion and camera controls
void KeyboardFunc(unsigned char key, int x, int y)
{
    switch(key)
	{
	case 'A':
	case 'a':
		//change if Extra Credit is implemented
		mapMode = (mapMode + 1) % 8;
		//mapMode = (mapMode + 1) % 11;

		//0 texture mapping - plane, planar mapping
		//1 texture mapping - sphere, planar mapping
		//2 texture mapping - teapot, planar mapping
		//3 texture mapping - sphere, spherical mapping
		//4 texture mapping - teapot, spherical mapping
		//5 environment mapping - sphere, sphere map
		//6 environment mapping - teapot, sphere map
		//7 bump mapping - plane
		//8 environment mapping - sphere, cube map [EXTRA CREDIT]
		//9 environment mapping - teapot, cube map [EXTRA CREDIT]
		//10 bump mapping - sphere [EXTRA CREDIT]

		switch(mapMode) {
		case 0:
		case 7:
			meshReader("plane.obj", 1);
			break;
		case 1:
		case 3:
		case 5:
		case 8:
		case 10:
			meshReader("sphere.obj", 1);
			break;
		case 2:
		case 4:
		case 6:
		case 9:
			meshReader("teapot.obj", 1);
			break;
		}
		break;
	case 'Q':
	case 'q':
		exit(1);
		break;
	case 'w':
	case 'W':
		if (illimunationMode == 0)
		{
			illimunationMode = 1;
		}
		else
		{
			illimunationMode = 0;
		}
		break;
	case 'e':
	case 'E':
		if (shadingMode == 0)
		{
			shadingMode =1;
		}
		else
		{
			shadingMode =0;
		}
		break;
	case 'd':
	case 'D':
		if (lightSource == 0)
		{
			lightSource =1;
		}
		else
		{
			lightSource =0;
		}
		break;
	case 'f':
	case 'F':
		if (lightSource == 1)
		{
			//change color of the secondary light source at each key press, 
			//light color cycling through pure red, green, blue, and white.
		}
		break;

    default:
		break;
    }

	glutPostRedisplay();
}

int main(int argc, char **argv) 
{			  

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowPosition(100,100);
	glutInitWindowSize(320,320);
	glutCreateWindow("Assignment 8");

	glutDisplayFunc(DisplayFunc);
	glutReshapeFunc(ReshapeFunc);
	glutMouseFunc(MouseFunc);
    glutMotionFunc(MotionFunc);
    glutKeyboardFunc(KeyboardFunc);

	
	setShaders();
	
	meshReader("teapot.obj", 1);

	glutMainLoop();

	return 0;
}

/*************************************************************
Shader related methods,
Setting the shader files
Setting the shader variables
*************************************************************/

void error_exit(int status, char *text)
{

	// Print error message

	fprintf(stderr,"Internal Error %i: ", status);
	fprintf(stderr,text);
	printf("\nTerminating as Result of Internal Error.\nPress Enter to exit.\n");

	// Keep the terminal open

	int anyKey = getchar();

	// Exit program

	exit(status);
}

int PrintOGLError(char *file, int line)
{
    GLenum glErr;
    int    retCode = 0;

    glErr = glGetError();
    while (glErr != GL_NO_ERROR)
    {
        printf("glError in file %s @ line %d: %s\n", file, line, gluErrorString(glErr));
        retCode = 1;
        glErr = glGetError();
    }
    return retCode;
}


void setShaders() 
{

	char *vs = NULL,*fs = NULL;

	//create the empty shader objects and get their handles
	vertex_shader = glCreateShaderObjectARB(GL_VERTEX_SHADER_ARB);
	fragment_shader = glCreateShaderObjectARB(GL_FRAGMENT_SHADER_ARB);
	

	//read the shader files and store the strings in corresponding char. arrays.
	vs = shaderFileRead("shader.vert");
	fs = shaderFileRead("shader.frag");

	const char * vv = vs;
	const char * ff = fs;

	GLint       vertCompiled, fragCompiled;

	//set the shader's source code by using the strings read from the shader files.
	glShaderSourceARB(vertex_shader, 1, &vv,NULL);
	glShaderSourceARB(fragment_shader, 1, &ff,NULL);

	free(vs);free(fs);

	//Compile the shader objects
	glCompileShaderARB(vertex_shader);
	glCompileShaderARB(fragment_shader);

	glGetObjectParameterivARB(fragment_shader, GL_OBJECT_COMPILE_STATUS_ARB, &fragCompiled);
	glGetObjectParameterivARB(vertex_shader, GL_OBJECT_COMPILE_STATUS_ARB, &vertCompiled);
    if (!vertCompiled || !fragCompiled)
	{
        cout<<"not compiled"<<endl;
	}
	
	//create an empty program object to attach the shader objects
	p = glCreateProgramObjectARB();

	program =p;
	//attach the shader objects to the program object
	glAttachObjectARB(p,vertex_shader);
	glAttachObjectARB(p,fragment_shader);

	/*
	**************
	Programming Tip:
	***************
	Delete the attached shader objects once they are attached.
	They will be flagged for removal and will be freed when they are no more used.
	*/
	glDeleteObjectARB(vertex_shader);
	glDeleteObjectARB(fragment_shader);

	//Link the created program.
	/*
	**************
	Programming Tip:
	***************
	You can trace the status of link operation by calling 
	"glGetObjectParameterARB(p,GL_OBJECT_LINK_STATUS_ARB)"
	*/
	glLinkProgramARB(p);


	//Start to use the program object, which is the part of the current rendering state
	glUseProgramObjectARB(p);

	    
	setParameters(p);

}

//Gets the location of the uniform variable given with "name" in the memory
//and tests whether the process was successfull.
//Returns the location of the queried uniform variable
int getUniformVariable(GLuint program,char *name)
{
	int location = glGetUniformLocationARB(program, name);
	
	if (location == -1)
	{
		printf("%s\n", name);
 		error_exit(1007, "No such uniform variable");
	}
	//PrintOpenGLError();
	return location;
}

void update_Light_Position()
{
	
	// Create light components
	GLfloat light_position[] = { CameraRadius*cos(CameraTheta)*sin(CameraPhi),			  
			  CameraRadius*cos(CameraPhi) , 
			  CameraRadius*sin(CameraTheta)*sin(CameraPhi),0.0 };
	glLightfv(GL_LIGHT0, GL_POSITION, light_position);
	

}

//Sets the light positions, etc. parameters for the shaders
void setParameters(GLuint program)
{
	int light_loc;
	int ambient_loc,diffuse_loc,specular_loc;
	int exponent_loc, mapMode_loc, baseMap_loc;

	//sample variable used to demonstrate how attributes are used in vertex shaders.
	//can be defined as gloabal and can change per vertex
	//float tangent = 0.0;
	//float tangent_loc;

	update_Light_Position();

	//Access uniform variables in shaders
	ambient_loc = getUniformVariable(program, "AmbientContribution");	
	glUniform3fvARB(ambient_loc,1, ambient_cont);

	diffuse_loc = getUniformVariable(program, "DiffuseContribution");
	glUniform3fvARB(diffuse_loc,1, diffuse_cont);

	specular_loc = getUniformVariable(program, "SpecularContribution");
	glUniform3fvARB(specular_loc,1,specular_cont);

	exponent_loc = getUniformVariable(program, "exponent");
	glUniform1fARB(exponent_loc,exponent);

	mapMode_loc = getUniformVariable(program, "mapMode");
	glUniform1iARB(mapMode_loc, mapMode);

	baseMap_loc = getUniformVariable(program, "baseMap");
    glUniform1iARB(baseMap_loc, 0);

	//Access attributes in vertex shader
	//tangent_loc = glGetAttribLocationARB(program,"tang");
	//glVertexAttrib1fARB(tangent_loc,tangent);

	tangent_loc = glGetAttribLocationARB(program,"rm_Tangent");
	binormal_loc = glGetAttribLocationARB(program,"rm_Binormal");

}


/****************************************************************
Utility methods:
shader file reader
mesh reader for objectt
****************************************************************/
//Read the shader files, given as parameter.
char *shaderFileRead(char *fn) {


	FILE *fp = fopen(fn,"r");
	if(!fp)
	{
		cout<< "Failed to load " << fn << endl;
		return " ";
	}
	else
	{
		cout << "Successfully loaded " << fn << endl;
	}
	
	char *content = NULL;

	int count=0;

	if (fp != NULL) 
	{
		fseek(fp, 0, SEEK_END);
		count = ftell(fp);
		rewind(fp);

		if (count > 0) 
		{
			content = (char *)malloc(sizeof(char) * (count+1));
			count = fread(content,sizeof(char),count,fp);
			content[count] = '\0';
		}
		fclose(fp);
	}
	return content;
}

void meshReader (char *filename,int sign)
{
  float x,y,z,len;
  int i;
  char letter;
  point v1,v2,crossP;
  int ix,iy,iz;
  int *normCount;
  FILE *fp;

  fp = fopen(filename, "r");
  if (fp == NULL) { 
    printf("Cannot open %s\n!", filename);
    exit(0);
  }

  verts = 0;
  faces = 0;

  // Count the number of vertices and faces
  while(!feof(fp))
   {
      fscanf(fp,"%c %f %f %f\n",&letter,&x,&y,&z);
      if (letter == 'v')
	  {
		  verts++;
	  }
      else
	  {
		  faces++;
	  }
   }

  fclose(fp);

  printf("verts : %d\n", verts);
  printf("faces : %d\n", faces);

  // Dynamic allocation of vertex and face lists
  faceList = (faceStruct *)malloc(sizeof(faceStruct)*faces);
  vertList = (point *)malloc(sizeof(point)*verts);
  normList = (point *)malloc(sizeof(point)*verts);

  fp = fopen(filename, "r");

  // Read the veritces
  for(i = 0;i < verts;i++)
    {
      fscanf(fp,"%c %f %f %f\n",&letter,&x,&y,&z);
      vertList[i].x = x;
      vertList[i].y = y;
      vertList[i].z = z;
    }

  // Read the faces
  for(i = 0;i < faces;i++)
    {
      fscanf(fp,"%c %d %d %d\n",&letter,&ix,&iy,&iz);
      faceList[i].v1 = ix - 1;
      faceList[i].v2 = iy - 1;
      faceList[i].v3 = iz - 1;
    }
  fclose(fp);


  // The part below calculates the normals of each vertex
  normCount = (int *)malloc(sizeof(int)*verts);
  for (i = 0;i < verts;i++)
    {
      normList[i].x = normList[i].y = normList[i].z = 0.0;
      normCount[i] = 0;
    }

  for(i = 0;i < faces;i++)
    {
      v1.x = vertList[faceList[i].v2].x - vertList[faceList[i].v1].x;
      v1.y = vertList[faceList[i].v2].y - vertList[faceList[i].v1].y;
      v1.z = vertList[faceList[i].v2].z - vertList[faceList[i].v1].z;
      v2.x = vertList[faceList[i].v3].x - vertList[faceList[i].v2].x;
      v2.y = vertList[faceList[i].v3].y - vertList[faceList[i].v2].y;
      v2.z = vertList[faceList[i].v3].z - vertList[faceList[i].v2].z;

      crossP.x = v1.y*v2.z - v1.z*v2.y;
      crossP.y = v1.z*v2.x - v1.x*v2.z;
      crossP.z = v1.x*v2.y - v1.y*v2.x;

      len = sqrt(crossP.x*crossP.x + crossP.y*crossP.y + crossP.z*crossP.z);

      crossP.x = -crossP.x/len;
      crossP.y = -crossP.y/len;
      crossP.z = -crossP.z/len;

      normList[faceList[i].v1].x = normList[faceList[i].v1].x + crossP.x;
      normList[faceList[i].v1].y = normList[faceList[i].v1].y + crossP.y;
      normList[faceList[i].v1].z = normList[faceList[i].v1].z + crossP.z;
      normList[faceList[i].v2].x = normList[faceList[i].v2].x + crossP.x;
      normList[faceList[i].v2].y = normList[faceList[i].v2].y + crossP.y;
      normList[faceList[i].v2].z = normList[faceList[i].v2].z + crossP.z;
      normList[faceList[i].v3].x = normList[faceList[i].v3].x + crossP.x;
      normList[faceList[i].v3].y = normList[faceList[i].v3].y + crossP.y;
      normList[faceList[i].v3].z = normList[faceList[i].v3].z + crossP.z;
      normCount[faceList[i].v1]++;
      normCount[faceList[i].v2]++;
      normCount[faceList[i].v3]++;
    }
  for (i = 0;i < verts;i++)
    {
      normList[i].x = (float)sign*normList[i].x / (float)normCount[i];
      normList[i].y = (float)sign*normList[i].y / (float)normCount[i];
      normList[i].z = (float)sign*normList[i].z / (float)normCount[i];
    }

}
