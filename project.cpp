///////////////////////////////////////////////////
// Name: 	Jacob French
// Course: 	CMPS 3480 Final Project
// Title: 	Changing Flag
// Controls: Press 'R' key to show flag.
// Press 'F' key to change flag texture.
///////////////////////////////////////////////////


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include "ppm.h"
#include "fonts.h"

typedef float Flt;
typedef Flt Vec[3];
#define rnd() (float)rand() / (float)RAND_MAX
#define PI 3.14159265358979323846264338327950
//X Windows variables
Display *dpy;
Window win;
GLXContext glc;

void initXWindows(void);
void init_opengl(void);
void cleanupXWindows(void);
void check_resize(XEvent *e);
void check_keys(XEvent *e);
void physics(void);
void render(void);
void change_flag();


int done=0;
int xres=500, yres=300;
float pos[3]={20.0,200.0,0.0};
float vel[3]={3.0,0.0,0.0};

const int MAX_POINTS = 10000;
const int MAX_CONTROL_POINTS = 2000;

GLfloat LightAmbient[]  = {0.0f, 0.0f, 0.0f,  1.0f };
GLfloat LightDiffuse[]  = {1.0f, 1.0f, 1.0f,  1.0f };
GLfloat LightSpecular[] = {0.5f, 0.5f, 0.5f,  1.0f };
GLfloat LightPosition[] = {-400.0f, 200.0f, -800.0f, 1.0f};
// GLfloat LightPosition[] = {0.0f, 300.0f, -1600.0f, 0.10f};


void setup_springs();

class Image{
public:
	int width, height;
	unsigned char *data;
	GLuint texId1;
	GLuint texId2;


	void loadImage(char filePath[80]){
		char str[80];
		strcpy(str, "convert ");
		strcat(str, filePath);
		strcat(str, " flag.ppm");
		puts(str);

		system(str);
		FILE *fpi = fopen("flag.ppm","r");
		if(fpi){
			char line[200];
			fgets(line, 200, fpi);
			fgets(line, 200, fpi);
			while(line[0] == '#')
				fgets(line, 200, fpi);

			sscanf(line, "%i %i", &width, &height);
			fgets(line, 200, fpi);

			//get pixel data
			int n = width * height * 3;
			data = new unsigned char[n];
			for(int i=0; i<n; i++) 
				data[i] = fgetc(fpi);

			fclose(fpi);

		}else{
			printf("ERROR opening input flag.ppm\n");
			exit(0);
		}

		unlink("flag.ppm");

	}
	~Image(){ delete [] data; }

}img;


int main(void){
	setup_springs();	
	initXWindows();
	init_opengl();

	//Do this to allow fonts
	glEnable(GL_TEXTURE_2D);
	initialize_fonts();
	while(!done) {
		while(XPending(dpy)) {
			XEvent e;
			XNextEvent(dpy, &e);
			check_resize(&e);
			check_keys(&e);
		}
		physics();
		render();
		glXSwapBuffers(dpy, win);
	}
	cleanupXWindows();
	cleanup_fonts();
	return 0;
}

void cleanupXWindows(void){
	XDestroyWindow(dpy, win);
	XCloseDisplay(dpy);
}

void set_title(void){
	//Set the window title bar.
	XMapWindow(dpy, win);
	XStoreName(dpy, win, "Jacob French - CMPS 3480 Final Project");
}

void setup_screen_res(const int w, const int h){
	xres = w;
	yres = h;
}

void initXWindows(void){
	Window root;
	GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, GLX_DOUBLEBUFFER, None };
	//GLint att[] = { GLX_RGBA, GLX_DEPTH_SIZE, 24, None };
	XVisualInfo *vi;
	Colormap cmap;
	XSetWindowAttributes swa;

	setup_screen_res(1200, 700);
	dpy = XOpenDisplay(NULL);
	if(dpy == NULL) {
		printf("\n\tcannot connect to X server\n\n");
		exit(EXIT_FAILURE);
	}
	root = DefaultRootWindow(dpy);
	vi = glXChooseVisual(dpy, 0, att);
	if(vi == NULL) {
		printf("\n\tno appropriate visual found\n\n");
		exit(EXIT_FAILURE);
	}
	//else {
	//	// %p creates hexadecimal output like in glxinfo
	//	printf("\n\tvisual %p selected\n", (void *)vi->visualid);
	//}
	cmap = XCreateColormap(dpy, root, vi->visual, AllocNone);
	swa.colormap = cmap;
	swa.event_mask = ExposureMask | KeyPressMask | KeyReleaseMask |
						StructureNotifyMask | SubstructureNotifyMask;
	win = XCreateWindow(dpy, root, 0, 0, xres, yres, 0,
							vi->depth, InputOutput, vi->visual,
							CWColormap | CWEventMask, &swa);
	set_title();
	glc = glXCreateContext(dpy, vi, NULL, GL_TRUE);
	glXMakeCurrent(dpy, win, glc);
}

void reshape_window(int width, int height){
	//window has been resized.
	setup_screen_res(width, height);
	//
	glViewport(0, 0, (GLint)width, (GLint)height);
	glMatrixMode(GL_PROJECTION); glLoadIdentity();
	glMatrixMode(GL_MODELVIEW); glLoadIdentity();
	set_title();
}


void init_opengl(void){
	//OpenGL initialization
	int selection = 3;
	switch(selection) {

		case 0:
		case 1:
			glViewport(0, 0, xres, yres);
			glDepthFunc(GL_LESS);
			glDisable(GL_DEPTH_TEST);
			//Initialize matrices
			glMatrixMode(GL_PROJECTION); glLoadIdentity();
			glMatrixMode(GL_MODELVIEW); glLoadIdentity();
			//This sets 2D mode (no perspective)
			glOrtho(0, xres, 0, yres, -1, 1);
			//Clear the screen
			glClearColor(1.0, 1.0, 1.0, 1.0);
			glClear(GL_COLOR_BUFFER_BIT);
			break;
		default:
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glClearDepth(1.0);
			glDepthFunc(GL_LESS);
			glEnable(GL_DEPTH_TEST);
			glShadeModel(GL_SMOOTH);
			glMatrixMode(GL_PROJECTION);
			glLoadIdentity();
			//change perspective
			gluPerspective(45.0f,(GLfloat)xres/(GLfloat)yres, 0.1f,10000.0f);
			glMatrixMode(GL_MODELVIEW);
			//Enable this so material colors are the same as vert colors.
			glEnable(GL_COLOR_MATERIAL);
			glEnable( GL_LIGHTING );
			glLightfv(GL_LIGHT0, GL_AMBIENT, LightAmbient);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, LightDiffuse);
			glLightfv(GL_LIGHT0, GL_SPECULAR, LightSpecular);
			glLightfv(GL_LIGHT0, GL_POSITION,LightPosition);
			glEnable(GL_LIGHT0);
			break;
	}
}

void check_resize(XEvent *e){
	//The ConfigureNotify is sent by the
	//server if the window is resized.
	if (e->type != ConfigureNotify)
		return;
	XConfigureEvent xce = e->xconfigure;
	if (xce.width != xres || xce.height != yres) {
		//Window size did change.
		reshape_window(xce.width, xce.height);
	}
}



void check_keys(XEvent *e){
	//Was there input from the keyboard?
	if (e->type == KeyPress) {
		int key = XLookupKeysym(&e->xkey, 0);
		switch(key) {
			case XK_r:
				setup_springs();
				init_opengl();
				break;
			case XK_f:
				change_flag();
				break;
			case XK_Escape:
				done=1;
				break;
		}
	}
}


struct Point {
	float x, y;
};

struct Spring {
	int mass[2];
	Flt length;
	Flt stiffness;
	Flt force[2];
	Flt vel[2];
} spring[MAX_CONTROL_POINTS*4];
int nsprings=0;



class Global {
public:
	int xres, yres;
	int mode;
	Point point[MAX_POINTS];
	int npoints;
	Point center;
	float radius;
	float matrix[2][2];

	//flag-vars
	int gravity;
	int jello;
	int wind;
	int rows;
	int cols;
	int flagIdx;
	Flt ylen;
	Flt xlen;
	Flt xstart;
	Flt ystart;
	Flt zstart;
	char *currentFlag;
	char *nextFlag;

	Global() {
		srand((unsigned)time(NULL));
		xres = 500;
		yres = 300;
		npoints = 5;
		center.x = xres/2;
		center.y = yres/2;
		radius = 100.0;
		gravity = 1;
		jello = 0;
		wind = 1;
		rows = 25;
		cols = 50;
		ylen = 15.0f;
		xstart = -100.0f;
		ystart = 200.0f;
		xstart = -300.0f;
		zstart = -600.0;
		flagIdx = 0;
		mode = 1;
	}
} g;

struct Mass {
	Flt mass, oomass;
	Flt pos[3];
	Flt vel[3];
	Flt force[3];
	int color[3];
} mass[MAX_CONTROL_POINTS];
int nmasses=0;




Flt apply_wind(Mass &mass1, Mass &mass2){
	Mass v;
	v.pos[0] = mass2.pos[0] - mass1.pos[0];
	v.pos[1] = mass2.pos[1] - mass1.pos[1];

	float angle = atan(v.pos[1]/v.pos[0]) * 180.0/PI;
	float wind = 0.0;

	if(angle < 0.0)
		angle = -angle;


		wind = 0.001;

	// return wind force in x direction
	return wind*angle;

}

void maintain_springs(){
	int i,m0,m1;
	Flt dist,oodist,factor;
	Flt springVec[3];
	Flt springforce[3];
	int rowEnd = 0;

	//Move the masses...
	for (i=0; i<nmasses; i++) {
		//anchor first mass in every row======================
		int yPos = g.ystart;
		int xPos = g.xstart;
		int zPos = g.zstart;
		int node = 0;
		for(int j=0; j<g.rows; j++){
				mass[node].pos[0] = xPos; //anchor mass xpos
				mass[node].pos[1] = yPos; //anchor mass ypos
				mass[node].pos[2] = zPos;
				node += g.cols;
				yPos -=g.ylen;
		}
		//====================================================

		mass[i].vel[0] += mass[i].force[0] * mass[i].oomass;
		mass[i].vel[1] += mass[i].force[1] * mass[i].oomass;
		mass[i].vel[2] += mass[i].force[2] * mass[i].oomass;
		mass[i].pos[0] += mass[i].vel[0];
		mass[i].pos[1] += mass[i].vel[1];
		mass[i].pos[2] += mass[i].vel[2];

		//apply wind force==========================================================
		if(g.wind){
			for(int j = rowEnd; j < g.cols+rowEnd; j++){
				mass[i].force[0] = apply_wind(mass[i], mass[i+1]);
				mass[i].force[1] = 0.0;

				if(mass[i].pos[2] >= -598.5)
					mass[i].force[2] = -0.01f;
				if(mass[i].pos[2] <= -600)
					mass[i].force[2] = 0.01f;

				

			}

		}
		else{
			mass[i].force[0] = 0.0;
			mass[i].force[1] = 0.0;
			mass[i].force[2] = 0.0;

		}

		rowEnd = (rowEnd >= g.rows*g.cols-g.cols) ? 0 : rowEnd+g.cols;
		//===========================================================================
		//Max velocity
		if (mass[i].vel[0] > 10.0)
			mass[i].vel[0] = 10.0;
		if (mass[i].vel[1] > 10.0)
			mass[i].vel[1] = 10.0;
		if (mass[i].vel[2] > 10.0)
			mass[i].vel[2] = 10.0;
		//Air resistance, or some type of damping
		mass[i].vel[0] *= 0.999;
		mass[i].vel[1] *= 0.999;
		mass[i].vel[2] *= 0.999;


		// printf("x=%f y=%f z=%f\n", mass[0].pos[0], mass[0].pos[1], mass[0].pos[2]);

	}


	//Resolve all springs...
	for (i=0; i<nsprings; i++) {
		m0 = spring[i].mass[0];
		m1 = spring[i].mass[1];
		//forces are applied here
		//vector between the two masses
		springVec[0] = mass[m0].pos[0] - mass[m1].pos[0];
		springVec[1] = mass[m0].pos[1] - mass[m1].pos[1];
		// //distance between the two masses
		dist = sqrt(springVec[0]*springVec[0] + springVec[1]*springVec[1]);
		if (dist == 0.0) dist = 0.1;
		oodist = 1.0 / dist; 
		springVec[0] *= oodist;
		springVec[1] *= oodist;
		//the spring force is added to the mass force
		factor = -(dist - spring[i].length) * spring[i].stiffness;
		springforce[0] = springVec[0] * factor;
		springforce[1] = springVec[1] * factor;
		//apply force and negative force to each end of the spring...
		mass[m0].force[0] += springforce[0];
		mass[m0].force[1] += springforce[1];
		mass[m1].force[0] -= springforce[0];
		mass[m1].force[1] -= springforce[1];
		//damping
		springforce[0] = (mass[m1].vel[0] - mass[m0].vel[0]) * 0.002;
		springforce[1] = (mass[m1].vel[1] - mass[m0].vel[1]) * 0.002;
		mass[m0].force[0] += springforce[0];
		mass[m0].force[1] += springforce[1];
		mass[m1].force[0] -= springforce[0];
		mass[m1].force[1] -= springforce[1];
	}
}

void physics(){
	int i;
	//gravity...
	if (g.gravity) {
		for (i=0; i<nmasses; i++) {
			mass[i].force[1] -= 0.009;
		}
	}

	maintain_springs();

}


void construct_spring(int curr, int next, int mode, Flt stiffness){
	double length = 0.0;
	if(mode == 0)
		length = mass[next].pos[0] - mass[curr].pos[0];
	else if (mode == 1)
		length = mass[next].pos[1] - mass[curr].pos[1];
	else
		length = (mass[next].pos[1] - mass[curr].pos[1])*sqrt(2.0);

	spring[nsprings].mass[0] = curr;
	spring[nsprings].mass[1] = next;
	spring[nsprings].length = length;
	spring[nsprings].stiffness = stiffness;
	nsprings++;
}

void setup_springs(){
	Flt x;
	Flt y;
	Flt z;
	Flt xlen, ylen;
	Flt vel[2];

	//position the masses
	x = g.xstart; //init first x pos
	y = g.ystart; //init first y pos
	z = g.zstart;
	nmasses = 0;


	xlen = 12.0f;
	ylen = g.ylen;


	//initialize masses in rows & columns
	int rowSize = g.rows;
	int colSize = g.cols;
	for (int i=0; i<rowSize; i++) {
		for(int j = 0; j < colSize; j++){
			mass[nmasses].mass = 1.0;
			mass[nmasses].oomass = 1.0 / mass[nmasses].mass;
			mass[nmasses].pos[0] = x;
			mass[nmasses].pos[1] = y;
			mass[nmasses].pos[2] = z;

			mass[nmasses].vel[0] = vel[0] + rnd() * 0.02-0.01;
			mass[nmasses].vel[1] = vel[1] + rnd() * 0.02-0.01;
			mass[nmasses].vel[2] = 0.0f;
			mass[nmasses].force[0] = 0.0;
			mass[nmasses].force[1] = 0.0;
			mass[nmasses].color[0] = 200 + rand() % 40;
			mass[nmasses].color[1] = 40 + rand() % 20;
			mass[nmasses].color[2] = 10 + rand() % 10;
			nmasses++;
			x += xlen;
		}

		x = g.xstart;
		y -= ylen;

	}


	//construct all flag springs===============================================
	//outer loop starts at first mass in each row
	//inner loop determines the current mass position in the current row
	nsprings = 0;
	Flt stiffness = 0.0;
	for(int i = 0; i < (rowSize*colSize); i+=colSize){
		for(int j = i; j < (i+colSize); j++){

			//horizontal springs
			stiffness = rnd() * 0.2 + 0.1;
			if(j < (i+colSize-1))
				construct_spring(j, j+1, 0, stiffness);
			if(j < (i+colSize-2))
				construct_spring(j, j+2, 0, stiffness);

			//vertical springs
			if(i < (rowSize*colSize)-colSize)
				construct_spring(j+colSize, j, 1, stiffness);

			if(i < (rowSize*colSize)-2*colSize)
				construct_spring(j+2*colSize, j, 1, stiffness);

			//diagonal springs (right to left)
			stiffness = rnd() * 0.2 + 0.01;
			if(i < (rowSize*colSize)-colSize && j < i+colSize-1)
				construct_spring(j+colSize+1, j, 3, stiffness);

		}
	}
	//end flag springs==========================================================
}

void change_flag(){
	glEnable(GL_TEXTURE_2D);
	if(g.flagIdx > 49){g.flagIdx = 0;}
	char flags[][50] = {
		"flags/american-flag-large.jpg", "flags/alabama-flag-large.jpg",
		"flags/alaska-flag-large.jpg", "flags/arizona-flag-large.jpg",
		"flags/arkansas-flag-large.jpg", "flags/california-flag-large.jpg",
		"flags/colorado-flag-large.jpg", "flags/connecticut-flag-large.jpg",
		"flags/delaware-flag-large.jpg", "flags/florida-flag-large.jpg",
		"flags/georgia-flag-large.jpg", "flags/hawaii-flag-large.jpg",
		"flags/idaho-flag-large.jpg", "flags/illinois-flag-large.jpg",
		"flags/indiana-flag-large.jpg", "flags/kansas-flag-large.jpg",
		"flags/kentucky-flag-large.jpg", "flags/louisiana-flag-large.jpg",
		"flags/maine-flag-large.jpg", "flags/maryland-flag-large.jpg",
		"flags/massachusetts-flag-large.jpg", "flags/michigan-flag-large.jpg",
		"flags/minnesota-flag-large.jpg", "flags/mississippi-flag-large.jpg",
		"flags/missouri-flag-large.jpg", "flags/montana-flag-large.jpg",
		"flags/nebraska-flag-large.jpg", "flags/nevada-flag-large.jpg",
		"flags/new-jersey-flag-large.jpg", "flags/new-mexico-flag-large.jpg",
		"flags/new-york-flag-large.jpg", "flags/north-carolina-flag-large.jpg",
		"flags/north-dakota-flag-large.jpg", "flags/ohio-flag-large.jpg",
		"flags/oklahoma-flag-large.jpg", "flags/oregon-flag-large.jpg",
		"flags/pennsylvania-flag-large.jpg", "flags/rhode-island-flag-large.jpg",
		"flags/south-carolina-flag-large.jpg", "flags/south-dakota-flag-large.jpg",
		"flags/tennessee-flag-large.jpg", "flags/texas-flag-large.jpg",
		"flags/utah-flag-large.jpg", "flags/vermont-flag-large.jpg",
		"flags/virginia-flag-large.jpg", "flags/washington-dc-flag-large.jpg",
		"flags/washington-flag-large.jpg", "flags/west-virginia-flag-large.jpg",
		"flags/wisconsin-flag-large.jpg", "flags/wyoming-flag-large.jpg"};

	//used for text on screen only
	g.currentFlag = flags[g.flagIdx];

	img.loadImage(flags[g.flagIdx]);
	g.flagIdx++;

   //texture mapping functionality
	glGenTextures(1, &img.texId1);

	int w = img.width;
	int h = img.height;
	glBindTexture(GL_TEXTURE_2D, img.texId1);

	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
	glTexImage2D(GL_TEXTURE_2D,0,3,w,h,0,GL_RGB,GL_UNSIGNED_BYTE,img.data);

	glClear(GL_COLOR_BUFFER_BIT);
}

void get_edge(Vec a, Vec b, Vec edge){
	edge[0] = b[0] - a[0];
	edge[1] = b[1] - a[1];
	edge[2] = b[2] - a[2];

}

void vecCrossProduct(Vec v0, Vec v1, Vec dest){
    dest[0] = v0[1]*v1[2] - v1[1]*v0[2];
    dest[1] = v0[2]*v1[0] - v1[2]*v0[0];
    dest[2] = v0[0]*v1[1] - v1[0]*v0[1];
}

void vecMake(Flt a, Flt b, Flt c, Vec v)
{
	v[0] = a;
	v[1] = b;
	v[2] = c;
}

Flt vecDotProduct(Vec v0, Vec v1){
	return v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
}

Flt vecLength(Vec v)
{
	return sqrt(vecDotProduct(v, v));
}

void vecNormalize(Vec v)
{
	Flt len = vecLength(v);
	if (len == 0.0) {
		vecMake(0,0,1,v);
		return;
	}
	len = 1.0 / len;
	v[0] *= len;
	v[1] *= len;
	v[2] *= len;
}


void show_flag(){
	glBindTexture(GL_TEXTURE_2D, img.texId1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glLightfv(GL_LIGHT0, GL_POSITION, LightPosition);
	glColor3f(1.0f, 1.0f, 1.0f);

	//flag====================
	float stepx = 0.0f;
	float stepy = 0.0f;

	//mass vectors relitive to the current vector in the loop
	Vec current;
	Vec down;
	Vec right;
	Vec left;
	Vec down_right;
	Vec down_left;
	Vec up;
	Vec up_left;
	Vec up_right;

	Vec cross1;
	Vec cross2;
	Vec cross3;
	Vec cross4;
	Vec cross5;
	Vec cross6;
	Vec cross7;
	Vec cross8;
	Vec cross_sum;


	// int i = the start of each new row
	// int j = each mass in each row
	for(int i = 0; i < g.rows*g.cols-g.cols; i+=g.cols){

		glBegin(GL_TRIANGLE_STRIP);
		for(int j = 0; j < (g.cols-1); j++){
			//current mass
			current[0] = mass[j+i].pos[0];
			current[1] = mass[j+i].pos[1];
			current[2] = mass[j+i].pos[2];
			//down from current
			if(i < 1250){
				down[0] = mass[j+i+g.cols].pos[0];
				down[1] = mass[j+i+g.cols].pos[1];
				down[2] = mass[j+i+g.cols].pos[2];
			}
			
			//right from current	
			if(j != (g.cols-2)){
				//right
				right[0] = mass[j+i+1].pos[0];
				right[1] = mass[j+i+1].pos[1];
				right[2] = mass[j+i+1].pos[2];
				//up-right from current
				if(i > 48){
					up_right[0] = mass[j+i+1-g.cols].pos[0];
					up_right[1] = mass[j+i+1-g.cols].pos[1];
					up_right[2] = mass[j+i+1-g.cols].pos[2];

				}
				//down-right from current
				if(i < 1250){
					down_right[0] = mass[(j+i+1)+g.cols-1].pos[0];
					down_right[1] = mass[(j+i+1)+g.cols-1].pos[1];
					down_right[2] = mass[(j+i+1)+g.cols-1].pos[2];

					down_left[0] = mass[(j+i-1)+g.cols-1].pos[0];
					down_left[1] = mass[(j+i-1)+g.cols-1].pos[1];
					down_left[2] = mass[(j+i-1)+g.cols-1].pos[2];

				}

			}

			//up from current
			if(j > 48){
				up[0] = mass[j+i-g.cols].pos[0];
				up[1] = mass[j+i-g.cols].pos[1];
				up[2] = mass[j+i-g.cols].pos[2];
				//up-left from current
				if(j > 0){
					up_left[0] = mass[j+i-g.cols-1].pos[0];
					up_left[1] = mass[j+i-g.cols-1].pos[1];
					up_left[2] = mass[j+i-g.cols-1].pos[2];

					left[0] = mass[j+i-1].pos[0];
					left[1] = mass[j+i-1].pos[1];
					left[2] = mass[j+i-1].pos[2];

				}
			}

			vecCrossProduct(current, down, cross1);
			vecCrossProduct(current, right, cross2);
			vecCrossProduct(current, down_right, cross3);
			vecCrossProduct(current, up, cross4);
			vecCrossProduct(current, down_left, cross5);
			vecCrossProduct(current, up_left, cross6);
			vecCrossProduct(current, left, cross7);
			vecCrossProduct(current, up_right, cross8);

			cross_sum[0] = cross1[0] + cross2[0] + cross3[0] +
						   cross4[0] + cross5[0] + cross6[0] +
						   cross7[0] + cross8[0];
			cross_sum[1] = cross1[1] + cross2[1] + cross3[1] +
						   cross4[1] + cross5[1] + cross6[1] +
						   cross7[1] + cross8[1];
			cross_sum[2] = cross1[2] + cross2[2] + cross3[2] +
						   cross4[2] + cross5[2] + cross6[2] +
						   cross7[2] + cross8[2];

			vecNormalize(cross_sum);
			glNormal3fv(cross_sum);

			// current
			glTexCoord3f(stepx, stepy, 1.0f);
			glVertex3f(current[0], current[1], current[2]);

			//down from current
			glTexCoord3f(stepx, stepy+0.04f, 1.0f);
			glVertex3f(down[0], down[1], down[2]);

			stepx += 0.02f;

		}

		stepx = 0.0f;
		stepy += 0.04f;
		glEnd();

	}
}


void render(void){

	glBegin(GL_QUADS);
	glVertex3f(mass[0].pos[0], mass[0].pos[1], mass[0].pos[2]);
	glEnd();
	
	// Rect r;
	glClear(GL_COLOR_BUFFER_BIT);
	
	// r.bot = g.yres - 20;
	// r.left = 10;
	// r.center = 0;

	// ggprint8b(&r, 16, 0x00887766, "CMPS 3480 Final Project");
	// ggprint8b(&r, 16, 0x008877aa, "R - Show Flag");
	// ggprint8b(&r, 16, 0x008877aa, "F - Switch Texture");


	// glPushMatrix();
	show_flag();
	glPopMatrix();

}