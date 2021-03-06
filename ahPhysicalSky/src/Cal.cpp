#include "Renderer.h"

#include <opencv2/opencv.hpp>


using namespace cv;
Vector3f * generateSky(float phi, float theta, float turbidity)
{
	Renderer render;

	// Options
	render.Width = 3328;
	render.Height = 1664;
	render.Colorspace = sRGB;
	render.SunSize = 0.02f;
	render.Turbidity = turbidity;
	render.SunTint = Vector3f(1);
	render.SkyTint = Vector3f(1);
	render.GroundAlbedo = Vector3f(0.3);
	render.SunColor = Vector3f(20000);
	render.bEnableSun = 1;
	render.Samples = 1;
	Vector3f dir(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));

	std::cout << ">>> Rendering Colorspaces, 0 - XYZ, 1 - sRGB, 2 - ACEScg, 3 = ACES 2065-1 " << std::endl;
	std::cout << ">>> Rendering with colorspace : " << render.Colorspace << std::endl;
	unsigned i = 0;

	char filename[1080 * 3];
	sprintf(filename, "./skydome.1%03d.exr", i);
	float angle = i / float(render.Samples - 1.0f) * M_PI * -0.6f;
	fprintf(stderr, ">>> Rendering Hosek-Wilkie Sky Model image %d. Angle = %0.2f\n", i, angle * 180.0f / M_PI);

	SkyModel sky;
	sky.SetupSky(dir,
		render.SunSize,
		render.SunColor,
		render.GroundAlbedo,
		render.Turbidity,
		render.Colorspace);
	Vector3f *skyImage = new Vector3f[render.Width * render.Height], *p = skyImage;
	memset(skyImage, 0x0, sizeof(Vector3f) * render.Width * render.Height);
	for (unsigned j = 0; j < render.Height; ++j) {
		float y = 2.0f * (j + 0.5f) / float(render.Height - 1) - 1.0f;
		for (unsigned i = 0; i < render.Width; ++i, ++p) {
			float x = 2.0f * (i + 0.5f) / float(render.Width - 1) - 1.0f;
			float z2 = x * x + y * y;
			if (j <= render.Height / 2) {
				float phi = i * 2 * M_PI / render.Width;//std::atan2(y, x);//0-2pi
				float theta = j * M_PI / render.Height;//std::acos(1 - z2);
				Vector3f dir(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
				// 1 meter above sea level
				*p = sky.Sample(dir, render.bEnableSun, render.SkyTint, render.SunTint);
			}
		}
	}
	return skyImage;


}

inline unsigned char ftouc(float f, float gamma)
{
	int i = static_cast<int>(255.0f * powf(f, 1.0f / gamma));
	if (i > 255) i = 255;
	if (i < 0) i = 0;

	return static_cast<unsigned char>(i);
}

double compute_SSE(cv::Mat Mat1, cv::Mat Mat2)
{

	cv::Mat M1 = Mat1.clone();
	cv::Mat M2 = Mat2.clone();

	int rows = M2.rows;
	int cols = M2.cols;
	// 确保它们的大小是一致的
	cv::resize(M1, M1, cv::Size(cols, rows));

	M1.convertTo(M1, CV_32F);
	M2.convertTo(M2, CV_32F);
	// compute PSNR
	Mat Diff;
	// Diff一定要提前转换为32F，因为uint8格式的无法计算成平方
	Diff.convertTo(Diff, CV_32F);
	cv::absdiff(M1, M2, Diff); //  Diff = | M1 - M2 |

	Diff = Diff.mul(Diff);            //  　　 | M1 - M2 |.^2
	Scalar S = cv::sum(Diff);  // 分别计算每个通道的元素之和

	double sse;   // square error
	sse = S.val[0] + S.val[1] + S.val[2];  // sum of all channels

	return sse;
}


/* ArithmeticOp.cpp */
#include "mex.h" //一定要包含这个头文件，这个头文件是MATLAB为MEX编译专门提供的头文件 
#include <cstdio>

// 被封装的函数声明
void Arithmetic_Operation(double* res, double b);

/*
* 请注意下面的这个mexFunction函数，如果需要使用mex编译C++函数，一定要使用这个函数
* mexFunction就像#include "mex.h"一样是必须的，相当于是MATLAB和C++的一个接口函数
* 这个函数的四个参数都是有用的，它们分别代表的意思是：
* nlhs: 输入参数个数(MATLAB调用语句中，等号左边的参数个数)  plhs: 输入参数列表
* nrhs: 输出参数个数(MATLAB调用语句中，等号右边的参数个数) prhs: 输处参数列表
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
	if (nlhs != 1 || nrhs != 2)// 判断调用参数数量是否正确
	{
		printf("Invalid Argument Usage!\n");
		exit(0);
	}

	// get input data
	double* a = mxGetPr(prhs[0]); // 输入参数列表，下标0是第一个输入参数
	double* result = (double *)malloc(4 * sizeof(double));

	// invoke 
	Arithmetic_Operation(result, *a);

	// output data
	plhs[0] = mxCreateDoubleMatrix(4, 1, mxREAL);// 用于创建一个double型的矩阵，三个参数分别是矩阵的行，列，类型
	double* output = (double *)mxGetPr(plhs[0]); // mexGetPr()函数是mex.h中提供的用于获取输入输出参数地址的函数

	for (int i = 0; i < 4; i++)
		output[i] = result[i];

	free(result);
	return;
}

/* 被封装的函数 */
void Arithmetic_Operation(double* res, double turbidity)
{
	float phi = 44.3496529;
	float theta = -143.0590763;
	auto h = generateSky(phi, theta, turbidity);
	Mat sky;
	//加载图片
	sky = imread("a.png", IMREAD_COLOR);
	Mat mask = imread("a.png", IMREAD_COLOR);
	Mat land = imread("tt.png", IMREAD_COLOR);
	int nl = sky.rows; //行数  
	int nc = sky.cols * sky.channels();
	for (int row = 0, i = 0; row < sky.rows; row++)
	{
		// data 是 uchar* 类型的, m.ptr(row) 返回第 row 行数据的首地址
		// 需要注意的是该行数据是按顺序存放的,也就是对于一个 3 通道的 Mat, 一个像素3个通道值, [B,G,R][B,G,R][B,G,R]... 
		// 所以一行长度为:sizeof(uchar) * m.cols * m.channels() 个字节 
		uchar* data = sky.ptr(row);
		uchar* maskData = mask.ptr(row);
		uchar* landData = land.ptr(row);
		for (int col = 0; col < sky.cols; col++, i++)
		{
			if (maskData[col * 3] != 0)
			{
				data[col * 3] = ftouc(h[i].z, 2.2);     //第row行的第col个像素点的第一个通道值 Blue

				data[col * 3 + 1] = ftouc(h[i].y, 2.2); // Green

				data[col * 3 + 2] = ftouc(h[i].x, 2.2); // Red
			}
			else
			{
				data[col * 3] = landData[col * 3];     //第row行的第col个像素点的第一个通道值 Blue

				data[col * 3 + 1] = landData[col * 3 + 1]; // Green

				data[col * 3 + 2] = landData[col * 3 + 2]; // Red
			}
		}
	}
	res[0] = compute_SSE(sky, land);

	return;
}
