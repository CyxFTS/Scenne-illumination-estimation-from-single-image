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
	// ȷ�����ǵĴ�С��һ�µ�
	cv::resize(M1, M1, cv::Size(cols, rows));

	M1.convertTo(M1, CV_32F);
	M2.convertTo(M2, CV_32F);
	// compute PSNR
	Mat Diff;
	// Diffһ��Ҫ��ǰת��Ϊ32F����Ϊuint8��ʽ���޷������ƽ��
	Diff.convertTo(Diff, CV_32F);
	cv::absdiff(M1, M2, Diff); //  Diff = | M1 - M2 |

	Diff = Diff.mul(Diff);            //  ���� | M1 - M2 |.^2
	Scalar S = cv::sum(Diff);  // �ֱ����ÿ��ͨ����Ԫ��֮��

	double sse;   // square error
	sse = S.val[0] + S.val[1] + S.val[2];  // sum of all channels

	return sse;
}


/* ArithmeticOp.cpp */
#include "mex.h" //һ��Ҫ�������ͷ�ļ������ͷ�ļ���MATLABΪMEX����ר���ṩ��ͷ�ļ� 
#include <cstdio>

// ����װ�ĺ�������
void Arithmetic_Operation(double* res, double b);

/*
* ��ע����������mexFunction�����������Ҫʹ��mex����C++������һ��Ҫʹ���������
* mexFunction����#include "mex.h"һ���Ǳ���ģ��൱����MATLAB��C++��һ���ӿں���
* ����������ĸ������������õģ����Ƿֱ�������˼�ǣ�
* nlhs: �����������(MATLAB��������У��Ⱥ���ߵĲ�������)  plhs: ��������б�
* nrhs: �����������(MATLAB��������У��Ⱥ��ұߵĲ�������) prhs: �䴦�����б�
*/
void mexFunction(int nlhs, mxArray *plhs[], int nrhs, mxArray *prhs[])
{
	if (nlhs != 1 || nrhs != 2)// �жϵ��ò��������Ƿ���ȷ
	{
		printf("Invalid Argument Usage!\n");
		exit(0);
	}

	// get input data
	double* a = mxGetPr(prhs[0]); // ��������б��±�0�ǵ�һ���������
	double* result = (double *)malloc(4 * sizeof(double));

	// invoke 
	Arithmetic_Operation(result, *a);

	// output data
	plhs[0] = mxCreateDoubleMatrix(4, 1, mxREAL);// ���ڴ���һ��double�͵ľ������������ֱ��Ǿ�����У��У�����
	double* output = (double *)mxGetPr(plhs[0]); // mexGetPr()������mex.h���ṩ�����ڻ�ȡ�������������ַ�ĺ���

	for (int i = 0; i < 4; i++)
		output[i] = result[i];

	free(result);
	return;
}

/* ����װ�ĺ��� */
void Arithmetic_Operation(double* res, double turbidity)
{
	float phi = 44.3496529;
	float theta = -143.0590763;
	auto h = generateSky(phi, theta, turbidity);
	Mat sky;
	//����ͼƬ
	sky = imread("a.png", IMREAD_COLOR);
	Mat mask = imread("a.png", IMREAD_COLOR);
	Mat land = imread("tt.png", IMREAD_COLOR);
	int nl = sky.rows; //����  
	int nc = sky.cols * sky.channels();
	for (int row = 0, i = 0; row < sky.rows; row++)
	{
		// data �� uchar* ���͵�, m.ptr(row) ���ص� row �����ݵ��׵�ַ
		// ��Ҫע����Ǹ��������ǰ�˳���ŵ�,Ҳ���Ƕ���һ�� 3 ͨ���� Mat, һ������3��ͨ��ֵ, [B,G,R][B,G,R][B,G,R]... 
		// ����һ�г���Ϊ:sizeof(uchar) * m.cols * m.channels() ���ֽ� 
		uchar* data = sky.ptr(row);
		uchar* maskData = mask.ptr(row);
		uchar* landData = land.ptr(row);
		for (int col = 0; col < sky.cols; col++, i++)
		{
			if (maskData[col * 3] != 0)
			{
				data[col * 3] = ftouc(h[i].z, 2.2);     //��row�еĵ�col�����ص�ĵ�һ��ͨ��ֵ Blue

				data[col * 3 + 1] = ftouc(h[i].y, 2.2); // Green

				data[col * 3 + 2] = ftouc(h[i].x, 2.2); // Red
			}
			else
			{
				data[col * 3] = landData[col * 3];     //��row�еĵ�col�����ص�ĵ�һ��ͨ��ֵ Blue

				data[col * 3 + 1] = landData[col * 3 + 1]; // Green

				data[col * 3 + 2] = landData[col * 3 + 2]; // Red
			}
		}
	}
	res[0] = compute_SSE(sky, land);

	return;
}
