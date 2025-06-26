#include "Renderer.h"
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>

// === 모델 데이터 구조체 정의 ===
struct Model {
    Vertex* vertex;
    Vertex* vertex_color;
    Vertex* vertex_normals;
    MMesh* mesh;
    int num_vertices, num_texcoords, num_normals, num_faces;
    float posX, posY, posZ;
    float scale;
};

std::vector<Model> models;

Model loadOBJ(const char* filename, float posX, float posY, float posZ, float user_scale) {
    Model m;
    m.vertex = new Vertex[10000000];
    m.vertex_color = new Vertex[10000000];
    m.vertex_normals = new Vertex[10000000];
    m.mesh = new MMesh[10000000];
    m.num_vertices = m.num_texcoords = m.num_normals = m.num_faces = 0;
    m.posX = posX;
    m.posY = posY;
    m.posZ = posZ;
    m.scale = 1.0f; 

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        printf("Failed to open %s\n", filename);
        return m;
    }

    char buf[256];
    float* tempX = new float[10000000];
    float* tempY = new float[10000000];
    float* tempZ = new float[10000000];

    while (fgets(buf, sizeof(buf), fp)) {
        if (strncmp(buf, "v ", 2) == 0) {
            float x, y, z;
            sscanf(buf, "v %f %f %f", &x, &y, &z);
            tempX[m.num_vertices] = x;
            tempY[m.num_vertices] = y;
            tempZ[m.num_vertices] = z;
            m.num_vertices++;
        }
        else if (strncmp(buf, "vt ", 3) == 0) {
            float u, v;
            sscanf(buf, "vt %f %f", &u, &v);
            m.vertex_color[m.num_texcoords++] = { u, v, 0 };
        }
        else if (strncmp(buf, "vn ", 3) == 0) {
            float x, y, z;
            sscanf(buf, "vn %f %f %f", &x, &y, &z);
            m.vertex_normals[m.num_normals++] = { x, y, z };
        }
		else if (strncmp(buf, "f ", 2) == 0) {
			int v[4], t[4], n[4];
			int matches = sscanf(buf, "f %d/%d/%d %d/%d/%d %d/%d/%d",
								&v[0], &t[0], &n[0],
								&v[1], &t[1], &n[1],
								&v[2], &t[2], &n[2]);
			if (matches == 9) {
				m.mesh[m.num_faces++] = { v[0], t[0], n[0], v[1], t[1], n[1], v[2], t[2], n[2] };
				continue;
			}

			matches = sscanf(buf, "f %d//%d %d//%d %d//%d %d//%d",
							&v[0], &n[0],
							&v[1], &n[1],
							&v[2], &n[2],
							&v[3], &n[3]);
			if (matches == 8) {
				// 사각형을 삼각형 2개로 쪼갬
				m.mesh[m.num_faces++] = { v[0], 0, n[0], v[1], 0, n[1], v[2], 0, n[2] };
				m.mesh[m.num_faces++] = { v[0], 0, n[0], v[2], 0, n[2], v[3], 0, n[3] };
				continue;
			}

			matches = sscanf(buf, "f %d//%d %d//%d %d//%d",
							&v[0], &n[0],
							&v[1], &n[1],
							&v[2], &n[2]);
			if (matches == 6) {
				m.mesh[m.num_faces++] = { v[0], 0, n[0], v[1], 0, n[1], v[2], 0, n[2] };
				continue;
			}

			matches = sscanf(buf, "f %d %d %d", &v[0], &v[1], &v[2]);
			if (matches == 3) {
				m.mesh[m.num_faces++] = { v[0], 0, 0, v[1], 0, 0, v[2], 0, 0 };
				continue;
			}
		}
    }
    fclose(fp);

    // --- 중심 이동 + 스케일 정규화 ---
    float minX = 1e9, maxX = -1e9;
    float minY = 1e9, maxY = -1e9;
    float minZ = 1e9, maxZ = -1e9;

    for (int i = 0; i < m.num_vertices; i++) {
        if (tempX[i] < minX) minX = tempX[i];
        if (tempX[i] > maxX) maxX = tempX[i];
        if (tempY[i] < minY) minY = tempY[i];
        if (tempY[i] > maxY) maxY = tempY[i];
        if (tempZ[i] < minZ) minZ = tempZ[i];
        if (tempZ[i] > maxZ) maxZ = tempZ[i];
    }

    float centerX = (minX + maxX) / 2.0f;
    float centerY = (minY + maxY) / 2.0f;
    float centerZ = (minZ + maxZ) / 2.0f;

    float sizeX = maxX - minX;
    float sizeY = maxY - minY;
    float sizeZ = maxZ - minZ;
    float maxSize = std::max({ sizeX, sizeY, sizeZ });

    float scaleFactor = (user_scale > 0.0f) ? user_scale / maxSize : 2.0f / maxSize;

    for (int i = 0; i < m.num_vertices; i++) {
        m.vertex[i].X = (tempX[i] - centerX) * scaleFactor;
        m.vertex[i].Y = (tempY[i] - centerY) * scaleFactor;
        m.vertex[i].Z = (tempZ[i] - centerZ) * scaleFactor;
    }

    delete[] tempX;
    delete[] tempY;
    delete[] tempZ;

    printf("Loaded %s: %d vertices, %d texcoords, %d normals, %d faces\n", filename, m.num_vertices, m.num_texcoords, m.num_normals, m.num_faces);
    return m;
}


void draw_center(void)
{
	glBegin(GL_LINES);
	glColor3f(1.0f, 0.0f, 0.0f); /* R */
	glVertex3f(0.0f, 0.0f, 0.0f);
	glVertex3f(0.2f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.2f, 0.0f, 0.0f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'x');

	glBegin(GL_LINES);
	glColor3f(0.0f, 1.0f, 0.0f); /* G */
	glVertex3f(0.0f, 0.2f, 0.0f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.0f, 0.2f, 0.0f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'y');

	glBegin(GL_LINES);
	glColor3f(0.0f, 0.0f, 1.0f); /* B */
	glVertex3f(0.0f, 0.0f, -0.2f);
	glVertex3f(0.0f, 0.0f, 0.0f);
	glEnd();
	glRasterPos3f(0.0f, 0.0f, -0.2f);
	glutBitmapCharacter(GLUT_BITMAP_9_BY_15, 'z');
}

void idle() {
	static GLuint previousClock = glutGet(GLUT_ELAPSED_TIME);
	static GLuint currentClock = glutGet(GLUT_ELAPSED_TIME);
	static GLfloat deltaT;

	currentClock = glutGet(GLUT_ELAPSED_TIME);
	deltaT = currentClock - previousClock;
	if (deltaT < 1000.0 / 20.0) { return; }
	else { previousClock = currentClock; }

	//char buff[256];
	//sprintf_s(buff, "Frame Rate = %f", 1000.0 / deltaT);
	//frameRate = buff;

	glutPostRedisplay();
}

void close()
{
	glDeleteTextures(1, &dispBindIndex);
	glutLeaveMainLoop();
	CloseHandle(hMutex);
}

void add_quats(float q1[4], float q2[4], float dest[4])
{
	static int count = 0;
	float t1[4], t2[4], t3[4];
	float tf[4];

	vcopy(q1, t1);
	vscale(t1, q2[3]);

	vcopy(q2, t2);
	vscale(t2, q1[3]);

	vcross(q2, q1, t3);
	vadd(t1, t2, tf);
	vadd(t3, tf, tf);
	tf[3] = q1[3] * q2[3] - vdot(q1, q2);

	dest[0] = tf[0];
	dest[1] = tf[1];
	dest[2] = tf[2];
	dest[3] = tf[3];

	if (++count > RENORMCOUNT) {
		count = 0;
		normalize_quat(dest);
	}
}

void reshape(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(58, (double)width / height, 0.1, 100);
	glMatrixMode(GL_MODELVIEW);
}

void motion(int x, int y)
{
	GLfloat spin_quat[4];
	float gain;
	gain = 2.0; /* trackball gain */

	if (drag_state == GLUT_DOWN)
	{
		if (button_state == GLUT_LEFT_BUTTON)
		{
			trackball(spin_quat,
				(gain * rot_x - 500) / 500,
				(500 - gain * rot_y) / 500,
				(gain * x - 500) / 500,
				(500 - gain * y) / 500);
			add_quats(spin_quat, quat, quat);
		}
		else if (button_state == GLUT_RIGHT_BUTTON)
		{
			t[0] -= (((float)trans_x - x) / 500);
			t[1] += (((float)trans_y - y) / 500);
		}
		else if (button_state == GLUT_MIDDLE_BUTTON)
			t[2] -= (((float)trans_z - y) / 500 * 4);
		else if (button_state == 3 || button_state == 4) // scroll
		{

		}
		//glutPostRedisplay();
	}

	rot_x = x;
	rot_y = y;

	trans_x = x;
	trans_y = y;
	trans_z = y;
}

void mouse(int button, int state, int x, int y)
{
	if (state == GLUT_DOWN)
	{
		if (button == GLUT_LEFT_BUTTON)
		{
			rot_x = x;
			rot_y = y;

			//t[0] = t[0] + 1;


		}
		else if (button == GLUT_RIGHT_BUTTON)
		{
			trans_x = x;
			trans_y = y;
		}
		else if (button == GLUT_MIDDLE_BUTTON)
		{
			//trcon = trcon + 1;
			trans_z = y;
		}
		else if (button == 3 || button == 4)
		{
			const float sign = (static_cast<float>(button)-3.5f) * 2.0f;
			t[2] -= sign * 500 * 0.00015f;
		}
	}

	drag_state = state;
	button_state = button;
}

void vzero(float* v)
{
	v[0] = 0.0f;
	v[1] = 0.0f;
	v[2] = 0.0f;
}

void vset(float* v, float x, float y, float z)
{
	v[0] = x;
	v[1] = y;
	v[2] = z;
}

void vsub(const float *src1, const float *src2, float *dst)
{
	dst[0] = src1[0] - src2[0];
	dst[1] = src1[1] - src2[1];
	dst[2] = src1[2] - src2[2];
}

void vcopy(const float *v1, float *v2)
{
	register int i;
	for (i = 0; i < 3; i++)
		v2[i] = v1[i];
}

void vcross(const float *v1, const float *v2, float *cross)
{
	float temp[3];

	temp[0] = (v1[1] * v2[2]) - (v1[2] * v2[1]);
	temp[1] = (v1[2] * v2[0]) - (v1[0] * v2[2]);
	temp[2] = (v1[0] * v2[1]) - (v1[1] * v2[0]);
	vcopy(temp, cross);
}

float vlength(const float *v)
{
	return sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
}

void vscale(float *v, float div)
{
	v[0] *= div;
	v[1] *= div;
	v[2] *= div;
}

void vnormal(float *v)
{
	vscale(v, 1.0f / vlength(v));
}

float vdot(const float *v1, const float *v2)
{
	return v1[0] * v2[0] + v1[1] * v2[1] + v1[2] * v2[2];
}

void vadd(const float *src1, const float *src2, float *dst)
{
	dst[0] = src1[0] + src2[0];
	dst[1] = src1[1] + src2[1];
	dst[2] = src1[2] + src2[2];
}

void trackball(float q[4], float p1x, float p1y, float p2x, float p2y)
{
	float a[3]; /* Axis of rotation */
	float phi;  /* how much to rotate about axis */
	float p1[3], p2[3], d[3];
	float t;

	if (p1x == p2x && p1y == p2y) {
		/* Zero rotation */
		vzero(q);
		q[3] = 1.0;
		return;
	}

	/*
	 * First, figure out z-coordinates for projection of P1 and P2 to
	 * deformed sphere
	 */
	vset(p1, p1x, p1y, tb_project_to_sphere(TRACKBALLSIZE, p1x, p1y));
	vset(p2, p2x, p2y, tb_project_to_sphere(TRACKBALLSIZE, p2x, p2y));

	/*
	 *  Now, we want the cross product of P1 and P2
	 */
	vcross(p2, p1, a);

	/*
	 *  Figure out how much to rotate around that axis.
	 */
	vsub(p1, p2, d);
	t = vlength(d) / (2.0f*TRACKBALLSIZE);

	/*
	 * Avoid problems with out-of-control values...
	 */
	if (t > 1.0) t = 1.0;
	if (t < -1.0) t = -1.0;
	phi = 2.0f * asin(t);

	axis_to_quat(a, phi, q);
}

void axis_to_quat(float a[3], float phi, float q[4])
{
	vnormal(a);
	vcopy(a, q);
	vscale(q, sin(phi / 2.0f));
	q[3] = cos(phi / 2.0f);
}

float tb_project_to_sphere(float r, float x, float y)
{
	float d, t, z;

	d = sqrt(x*x + y*y);
	if (d < r * 0.70710678118654752440f) {    /* Inside sphere */
		z = sqrt(r*r - d*d);
	}
	else {           /* On hyperbola */
		t = r / 1.41421356237309504880f;
		z = t*t / d;
	}
	return z;
}

void normalize_quat(float q[4])
{
	int i;
	float mag;

	mag = (q[0] * q[0] + q[1] * q[1] + q[2] * q[2] + q[3] * q[3]);
	for (i = 0; i < 4; i++) q[i] /= mag;
}

void build_rotmatrix(float m[4][4], float q[4])
{
	m[0][0] = 1.0f - 2.0f * (q[1] * q[1] + q[2] * q[2]);
	m[0][1] = 2.0f * (q[0] * q[1] - q[2] * q[3]);
	m[0][2] = 2.0f * (q[2] * q[0] + q[1] * q[3]);
	m[0][3] = 0.0f;

	m[1][0] = 2.0f * (q[0] * q[1] + q[2] * q[3]);
	m[1][1] = 1.0f - 2.0f * (q[2] * q[2] + q[0] * q[0]);
	m[1][2] = 2.0f * (q[1] * q[2] - q[0] * q[3]);
	m[1][3] = 0.0f;

	m[2][0] = 2.0f * (q[2] * q[0] - q[1] * q[3]);
	m[2][1] = 2.0f * (q[1] * q[2] + q[0] * q[3]);
	m[2][2] = 1.0f - 2.0f * (q[1] * q[1] + q[0] * q[0]);
	m[2][3] = 0.0f;

	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void InitializeWindow(int argc, char* argv[])
{
	// initialize glut settings
	glutInit(&argc, argv);

	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitWindowSize(1000 / 2, 1000 / 2);

	glutInitWindowPosition(0, 0);

	dispWindowIndex = glutCreateWindow("3D Model");

	trackball(quat, 90.0, 0.0, 0.0, 0.0);

	glutIdleFunc(idle);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutSpecialFunc(special);
	glutMotionFunc(motion);
	glutMouseFunc(mouse);
	glutCloseFunc(close);
	//GLuint image = load   ("./my_texture.bmp");
	
	//glBindTexture(1,)

	glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);

	// bind textures
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glEnable(GL_DEPTH_TEST);

	reshape(1000, 1000);

	/*glGenTextures(1, &dispBindIndex);
	glBindTexture(GL_TEXTURE_2D, dispBindIndex);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);*/
}

void draw_starfield(int count = 300) {
    glDisable(GL_LIGHTING);
    glPointSize(1.5f);
    glBegin(GL_POINTS);
    glColor3f(1.0f, 1.0f, 1.0f);  // 흰색 별

    srand(42);  // 고정된 별 위치 (매번 동일)
    for (int i = 0; i < count; ++i) {
        float x = ((rand() % 2000) - 1000) / 50.0f;  // -20 ~ +20
        float y = ((rand() % 2000) - 1000) / 50.0f;
        float z = ((rand() % 2000) - 1000) / 50.0f;
        glVertex3f(x, y, z);
    }

    glEnd();
}

Vertex midpoint(const Vertex& a, const Vertex& b) {
    return {
        (a.X + b.X) / 2.0f,
        (a.Y + b.Y) / 2.0f,
        (a.Z + b.Z) / 2.0f
    };
}

Vertex normalize(const Vertex& v) {
    float length = sqrt(v.X * v.X + v.Y * v.Y + v.Z * v.Z);
    if (length > 1e-6) { // 0으로 나누는 것을 방지
        return { v.X / length, v.Y / length, v.Z / length };
    }
    return v; // 길이가 0이면 원래 벡터 반환
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // --- Projection ---
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, 0.1, 200);

    // --- ModelView ---
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 2.0, 8.0, 0, 0, 0, 0, 1.0, 0);

    GLfloat rot[4][4];
    build_rotmatrix(rot, quat);
    glMultMatrixf(&rot[0][0]);
    glTranslatef(t[0], t[1], t[2] - 1.0f);
    glScalef(1, 1, 1);

    // Lighting 설정
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL); // 조명과 색상 연결
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);  // 색상 반영 방식 설정
    glShadeModel(GL_SMOOTH);

    // 조명 속성 (기존과 동일)
    GLfloat light_pos[]     = { 5.0f, 5.0f, 5.0f, 1.0f };
    GLfloat light_ambient[] = { 0.2f, 0.2f, 0.2f, 1.0f };
    GLfloat light_diffuse[] = { 0.8f, 0.8f, 0.8f, 1.0f };
    GLfloat light_specular[]= { 1.0f, 1.0f, 1.0f, 1.0f };

    glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
    glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
    glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);

    GLfloat mat_specular[]  = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 64.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    // 배경 별
    draw_starfield();

    // --- 모델 렌더링 ---
    for (int i = 0; i < models.size(); ++i) {
        const auto& m = models[i];
        glPushMatrix();
        glTranslatef(m.posX, m.posY, m.posZ);
        glScalef(m.scale, m.scale, m.scale);

        glBegin(GL_TRIANGLES);
        for (int jj = 0; jj < m.num_faces; jj++) {
            const Vertex& v1 = m.vertex[m.mesh[jj].V1 - 1];
            const Vertex& v2 = m.vertex[m.mesh[jj].V2 - 1];
            const Vertex& v3 = m.vertex[m.mesh[jj].V3 - 1];

            const Vertex& n1 = m.vertex_normals[m.mesh[jj].N1 - 1];
            const Vertex& n2 = m.vertex_normals[m.mesh[jj].N2 - 1];
            const Vertex& n3 = m.vertex_normals[m.mesh[jj].N3 - 1];

            // Subdivision (삼각형 1 → 4)
            Vertex vm12 = midpoint(v1, v2);
            Vertex vm23 = midpoint(v2, v3);
            Vertex vm31 = midpoint(v3, v1);
            Vertex nm12 = normalize(midpoint(n1, n2));
            Vertex nm23 = normalize(midpoint(n2, n3));
            Vertex nm31 = normalize(midpoint(n3, n1));
            
            // 삼각형 1
            glColor3f((v1.X + 1.0f) / 2.0f, (v1.Y + 1.0f) / 2.0f, (v1.Z + 1.0f) / 2.0f);
            glNormal3f(n1.X, n1.Y, n1.Z); glVertex3f(v1.X, v1.Y, v1.Z);

            glColor3f((vm12.X + 1.0f) / 2.0f, (vm12.Y + 1.0f) / 2.0f, (vm12.Z + 1.0f) / 2.0f);
            glNormal3f(nm12.X, nm12.Y, nm12.Z); glVertex3f(vm12.X, vm12.Y, vm12.Z);
            
            glColor3f((vm31.X + 1.0f) / 2.0f, (vm31.Y + 1.0f) / 2.0f, (vm31.Z + 1.0f) / 2.0f);
            glNormal3f(nm31.X, nm31.Y, nm31.Z); glVertex3f(vm31.X, vm31.Y, vm31.Z);

            // 삼각형 2
            glColor3f((v2.X + 1.0f) / 2.0f, (v2.Y + 1.0f) / 2.0f, (v2.Z + 1.0f) / 2.0f);
            glNormal3f(n2.X, n2.Y, n2.Z); glVertex3f(v2.X, v2.Y, v2.Z);
            
            glColor3f((vm23.X + 1.0f) / 2.0f, (vm23.Y + 1.0f) / 2.0f, (vm23.Z + 1.0f) / 2.0f);
            glNormal3f(nm23.X, nm23.Y, nm23.Z); glVertex3f(vm23.X, vm23.Y, vm23.Z);
            
            glColor3f((vm12.X + 1.0f) / 2.0f, (vm12.Y + 1.0f) / 2.0f, (vm12.Z + 1.0f) / 2.0f);
            glNormal3f(nm12.X, nm12.Y, nm12.Z); glVertex3f(vm12.X, vm12.Y, vm12.Z);

            // 삼각형 3
            glColor3f((v3.X + 1.0f) / 2.0f, (v3.Y + 1.0f) / 2.0f, (v3.Z + 1.0f) / 2.0f);
            glNormal3f(n3.X, n3.Y, n3.Z); glVertex3f(v3.X, v3.Y, v3.Z);
            
            glColor3f((vm31.X + 1.0f) / 2.0f, (vm31.Y + 1.0f) / 2.0f, (vm31.Z + 1.0f) / 2.0f);
            glNormal3f(nm31.X, nm31.Y, nm31.Z); glVertex3f(vm31.X, vm31.Y, vm31.Z);
            
            glColor3f((vm23.X + 1.0f) / 2.0f, (vm23.Y + 1.0f) / 2.0f, (vm23.Z + 1.0f) / 2.0f);
            glNormal3f(nm23.X, nm23.Y, nm23.Z); glVertex3f(vm23.X, vm23.Y, vm23.Z);

            // 삼각형 4 (중앙)
            glColor3f((vm12.X + 1.0f) / 2.0f, (vm12.Y + 1.0f) / 2.0f, (vm12.Z + 1.0f) / 2.0f);
            glNormal3f(nm12.X, nm12.Y, nm12.Z); glVertex3f(vm12.X, vm12.Y, vm12.Z);
            
            glColor3f((vm23.X + 1.0f) / 2.0f, (vm23.Y + 1.0f) / 2.0f, (vm23.Z + 1.0f) / 2.0f);
            glNormal3f(nm23.X, nm23.Y, nm23.Z); glVertex3f(vm23.X, vm23.Y, vm23.Z);
            
            glColor3f((vm31.X + 1.0f) / 2.0f, (vm31.Y + 1.0f) / 2.0f, (vm31.Z + 1.0f) / 2.0f);
            glNormal3f(nm31.X, nm31.Y, nm31.Z); glVertex3f(vm31.X, vm31.Y, vm31.Z);
        }
        glEnd();

        glPopMatrix();
    }

    glutSwapBuffers();
}



int main(int argc, char* argv[]) {
    srand(static_cast<unsigned>(time(nullptr)));

    // 중심 우주정거장
    models.push_back(loadOBJ("spaceCenter.obj", 0.0f, 0.0f, 0.0f, 15.0f));

    // 주변을 떠다니는 모델들
    struct { 	
        const char* name;
        float scale;
    } modelData[] = {
        {"human.obj", 1.0f},
		{"01Mothership.obj", 1.0f},
        {"02BattleShip.obj", 1.0f},
        {"04BattleShip.obj", 1.0f},
        {"05BattleShip.obj", 1.0f},
        {"06BattleShip.obj", 1.0f},
        {"07Attacker.obj", 1.0f},
		{"08Monitor.obj", 1.0f},
		{"09spaceShip1.obj", 1.0f},
    };

    int num = sizeof(modelData) / sizeof(modelData[0]);
    for (int i = 0; i < num; ++i) {
        float angle = (float)(rand() % 360) * 3.14159f / 180.0f;   // 각도
        float radius = 3.0f + static_cast<float>(rand() % 300) / 100.0f;  // 3.0 ~ 6.0
        float height = -1.0f + static_cast<float>(rand() % 200) / 100.0f; // -1.0 ~ 1.0

        float x = radius * cos(angle);
        float z = radius * sin(angle);
        float y = height;

        models.push_back(loadOBJ(modelData[i].name, x, y, z, modelData[i].scale));
    }

    InitializeWindow(argc, argv);
    glutMainLoop();

    for (auto& m : models) {
        delete[] m.vertex;
        delete[] m.vertex_color;
        delete[] m.vertex_normals;
        delete[] m.mesh;
    }
    return 0;
}
