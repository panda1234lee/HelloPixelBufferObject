// HelloPBO.cpp : 定义控制台应用程序的入口点。
//
#include "stdafx.h"

#include <time.h>
#include <gl/glew.h>
#include <gl/freeglut.h>
#include<soil.h>


#define WIN_W 640
#define WIN_H 480
#define WIN_X 0	// ☆
#define WIN_Y 0
#define CAPTURE 1
//#define IO 2

// PBO开关
#define PBO_TEST 1

GLuint  texName;
#if PBO_TEST
GLuint  pboName[2];
#endif

void add(unsigned char *src, int width, int height, int shift, unsigned char *dst);

// 自定义的方法
bool saveBMP(const char *lpFileName)
{
    GLint viewport[4];
    glGetIntegerv(GL_VIEWPORT, viewport);

    int width = viewport[2];
    int height = viewport[3];

    // 设置解包像素的对齐格式——Word对齐(4字节)
    glPixelStorei(GL_PACK_ALIGNMENT, 4);
    // 计算对齐后的真实宽度
    int nAlignWidth = (width * 24 + 31) / 32;

#if PBO_TEST
    static int index = 0;
    index = (index + 1) % 2;
    int next_index = (index + 1) % 2;

    // PBO
    glReadBuffer(GL_FRONT);
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[index]);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, 0);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[next_index]);
    unsigned char *src = (unsigned char *)glMapBufferRange(GL_PIXEL_PACK_BUFFER, 0, nAlignWidth * height * 4, GL_MAP_READ_BIT);
    unsigned char *pdata = NULL;
    if (src)
    {
        pdata = new unsigned char[nAlignWidth * height * 4];
        memcpy(pdata, src, nAlignWidth * height * 4);
        glUnmapBuffer(GL_PIXEL_PACK_BUFFER);
    }

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#else
    // 分配缓冲区
    unsigned char *pdata = new unsigned char[nAlignWidth * height * 4];
    memset(pdata, 0, nAlignWidth * height * 4);
    //从当前绑定的 frame buffer 读取 pixels
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pdata);
#endif
    // ============================================

    if (!pdata)
    {
        printf("pdata is NULL! \n");
        return FALSE;
    }

	unsigned char *dst = new unsigned char[nAlignWidth * height * 4];

	// 耗时操作
	add(pdata, nAlignWidth, height, 0, dst);

#ifdef IO
    // 以下就是为了 BMP 格式 做准备
    //由RGB变BGR
    for (int i = 0; i < width * height * 3; i += 3)
    {
        unsigned char tmpRGB;
        tmpRGB = pdata[i];
        pdata[i] = pdata[i + 2];
        pdata[i + 2] = tmpRGB;
    }

    // 设置 BMP 的文件头
    BITMAPFILEHEADER Header;
    BITMAPINFOHEADER HeaderInfo;
    Header.bfType = 0x4D42;
    Header.bfReserved1 = 0;
    Header.bfReserved2 = 0;
    Header.bfOffBits = (DWORD)(
                           sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER));
    Header.bfSize = (DWORD)(
                        sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER)
                        + nAlignWidth * height * 4);
    HeaderInfo.biSize = sizeof(BITMAPINFOHEADER);
    HeaderInfo.biWidth = width;
    HeaderInfo.biHeight = height;
    HeaderInfo.biPlanes = 1;
    HeaderInfo.biBitCount = 24;
    HeaderInfo.biCompression = 0;
    HeaderInfo.biSizeImage = 4 * nAlignWidth * height;
    HeaderInfo.biXPelsPerMeter = 0;
    HeaderInfo.biYPelsPerMeter = 0;
    HeaderInfo.biClrUsed = 0;
    HeaderInfo.biClrImportant = 0;


    // 写入字节数据
    FILE *pfile;
    if (!(pfile = fopen(lpFileName, "wb+")))
    {
        printf("保存图像失败!\n");
        return FALSE;
    }
    fwrite(&Header, 1, sizeof(BITMAPFILEHEADER), pfile);
    fwrite(&HeaderInfo, 1, sizeof(BITMAPINFOHEADER), pfile);
    fwrite(pdata, 1, HeaderInfo.biSizeImage, pfile);
    fclose(pfile);
#endif

#ifndef PBO_TEST
    delete[] pdata;
#else
    pdata = NULL;
#endif
	delete[] dst;

    return TRUE;
}

GLuint loadGLTexture(const char *file_path)
{
    // 切记要初始化 OpenGL Context 和 GLEW ！
    /* load an image file directly as a new OpenGL texture */
    GLuint  tex_2d = SOIL_load_OGL_texture
                     (
                         file_path,
                         SOIL_LOAD_AUTO,
                         SOIL_CREATE_NEW_ID,
                         SOIL_FLAG_MIPMAPS | SOIL_FLAG_INVERT_Y | SOIL_FLAG_NTSC_SAFE_RGB | SOIL_FLAG_COMPRESS_TO_DXT
                     );

    /* check for an error during the load process */
    if (0 == tex_2d)
    {
        printf("SOIL loading error: '%s'\n", SOIL_last_result());
    }

    return tex_2d;
}

void init(void)
{
    // ------------------------------------
	clock_t start = clock();
    // 比较耗时
    GLenum err = glewInit();
    //if (GLEW_OK != err)
    //{
    //    /* Problem: glewInit failed, something is seriously wrong. */
    //    fprintf(stderr, "Error: %s\n", glewGetErrorString(err));
    //}
    //fprintf(stdout, "Status: Using GLEW %s\n", glewGetString(GLEW_VERSION));
	if (glewIsSupported("GL_VERSION_2_0"))
		printf("Ready for OpenGL 2.0\n");
	else
	{
		printf("OpenGL 2.0 not supported\n");
		exit(1);
	}
	clock_t end = clock();
	double sec = double(end - start) / CLOCKS_PER_SEC;
	printf("glewInit: %fs\n", sec);
    // ------------------------------------

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);
    glEnable(GL_DEPTH_TEST);

    // ------------------------------------
    texName = loadGLTexture("../micky.png");

    // 按字节对齐
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // 区别1
    //glGenTextures(1, &texName);
    glBindTexture(GL_TEXTURE_2D, texName);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

    // 区别2
    //glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imageRec->sizeX, imageRec->sizeX,
    //	0, GL_RGB, GL_UNSIGNED_BYTE, imageRec->data);

    // ☆
#if PBO_TEST
    glGenBuffers(2, pboName);

    int nAlignWidth = (WIN_W * 24 + 31) / 32;
    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[0]);
    glBufferData(GL_PIXEL_PACK_BUFFER, nAlignWidth * WIN_H * 4, NULL, GL_DYNAMIC_READ);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, pboName[1]);
    glBufferData(GL_PIXEL_PACK_BUFFER, nAlignWidth * WIN_H * 4, NULL, GL_DYNAMIC_READ);

    glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
#endif
}

// 居然要 200+ms
void saveScreenShot(const char *img_path, int x, int y, int width, int height)
{
    int save_result = SOIL_save_screenshot
                      (
                          img_path,
                          SOIL_SAVE_TYPE_BMP,
                          x, y, width, height
                      );

}

void display(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);

    glBindTexture(GL_TEXTURE_2D, texName);

    glBegin(GL_QUADS);
    // 左面
    glTexCoord2f(0.0, 0.0);
    glVertex3f(-2.0, -1.0, 0.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(-2.0, 1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(0.0, 1.0, 0.0);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(0.0, -1.0, 0.0);
    // 右面
    glTexCoord2f(0.0, 0.0);
    glVertex3f(1.0, -1.0, 0.0);
    glTexCoord2f(0.0, 1.0);
    glVertex3f(1.0, 1.0, 0.0);
    glTexCoord2f(1.0, 1.0);
    glVertex3f(2.41421, 1.0, -1.41421);
    glTexCoord2f(1.0, 0.0);
    glVertex3f(2.41421, -1.0, -1.41421);
    glEnd();

#if CAPTURE

	static int count = 1;

    // 根据时间来命名位图
    clock_t time = clock();
    char img_path[100];
    sprintf(img_path, "../bmp/%ld.bmp", time);

    saveBMP(img_path);
    //saveScreenShot(img_path, WIN_X, WIN_Y, WIN_W, WIN_H);	// 太慢

    clock_t elapse = clock() - time;
    //printf("elapse = %dms\n", elapse);

	static long sum = 0.;
	sum += elapse;
	float avg = 1. * sum / count;
	count++;

	printf("average = %fms\n", avg);

# endif

    glFlush();	// 与单缓冲对应
    // glDisable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60.0, (GLfloat)w / (GLfloat)h, 1.0, 30.0);	// ☆
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0.0, 0.0, -3.6);
}

void processKeyboard(unsigned char key, int x, int y)
{
    // http://blog.sina.com.cn/s/blog_4a9aa55c0100p36b.html
    if (key == 27)
    {
        exit(0);
    }
    else if (key == 'r')
    {
        int mod = glutGetModifiers();
        if (mod == GLUT_ACTIVE_ALT)
            printf("Alt + r ! \n");
        else if(mod == GLUT_ACTIVE_CTRL)
            printf("Ctrl+ r ! \n");
        else if (mod == GLUT_ACTIVE_SHIFT)
            printf("Shift+ r ! \n");
    }
    else if (key == GLUT_KEY_F1)
    {
        int mod = glutGetModifiers();
        if (mod == (GLUT_ACTIVE_CTRL | GLUT_ACTIVE_ALT))
        {
            printf("Ctrl + Shift+ F1 ! \n");
        }
    }



}

// 测试：耗时操作
void add(unsigned char *src, int width, int height, int shift, unsigned char *dst)
{
    if (!src || !dst)
        return;

    int value;
    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; ++j)
        {
			//printf("(%d, %d)\n", i, j);
            value = *src + shift;
            if (value > 255) 
				*dst = (unsigned char)255;
            else            
				*dst = (unsigned char)value;
            ++src;
            ++dst;

            value = *src + shift;
            if (value > 255) 
				*dst = (unsigned char)255;
            else            
				*dst = (unsigned char)value;
            ++src;
            ++dst;

            value = *src + shift;
            if (value > 255) 
				*dst = (unsigned char)255;
            else            
				*dst = (unsigned char)value;
            ++src;
            ++dst;

            ++src;    // skip alpha
            ++dst;
        }
    }
}

int main(int argc, char **argv)
{
    glutInit(&argc, argv);
    // 单缓冲
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB/* | GLUT_DEPTH*/);
    glutInitWindowSize(WIN_W, WIN_H);
    glutInitWindowPosition(WIN_X, WIN_Y);
    glutCreateWindow("HelloPBO");

    init();

    glutDisplayFunc(display);
    glutIdleFunc(display);
    glutKeyboardFunc(&processKeyboard);
    glutReshapeFunc(reshape);
    glutMainLoop();

    return 0;
}


