//导向滤波算法实现图片去雾
//参考网站 https://blog.csdn.net/s12244315/article/details/50292049
#include<iostream>
#include<opencv2/opencv.hpp>

using namespace std;
using namespace cv;

const double omega = 0.90;//空间颗粒杂质影响程度
const int windowsize = 15;//windowsize = 2r+1
const int radius = 7; //窗口半径r
const double t0 = 0.1;  //tr预估值(最小)
const double top = 0.001; //取top 求均值为A

void minFilter(Mat&,Mat&);

double max(double,double);

double getAtmosphere(Mat&,double);

int main(int argc,char**argv)
{
    double startTime=(double)getTickCount(); 
    Mat srcImage = imread("image/hazedImage/11.png");
    if(!srcImage.data)
    {
        cout<<"cannot read this picture,the filename may be wrong";
        return -1;
    }
    if(srcImage.cols>640)
    {
        resize(srcImage,srcImage,Size(640,480));
    }
     //resize(srcImage,srcImage,Size(640,480));

    Mat channels[3];
    split(srcImage,channels);
    //先创建最小化的channel
    Mat minRGB = Mat::zeros(srcImage.size(),CV_8UC1);
    
    int rows = srcImage.rows;
    int cols = srcImage.cols;

    //1.求暗通道
    for(int i = 0;i<rows;i++)
    {
        for(int j = 0;j<cols;j++)
        {
            int minValue = channels[0].at<uchar>(i,j);
            for(int c= 1;c<3;c++)
                minValue = minValue > channels[0].at<uchar>(i,j) ? channels[0].at<uchar>(i,j) : minValue;
            minRGB.at<uchar>(i,j) = minValue;
        }

    }

    //imshow("min BGR",minRGB);

    //求全球大气光值A top = 0.001
    
    double A = getAtmosphere(minRGB,top);//假定A是255

    cout<<"A: "<<A<<endl;

    Mat minFilterImage;
    minFilter(minRGB,minFilterImage);

    //imshow("minFilter",minFilterImage);
    waitKey();

    //求预估的投射图T(x)
    Mat T = Mat::zeros(minFilterImage.size(),CV_8UC1);
    for(int i = 0;i<T.rows;i++)
    {
        for(int j = 0;j<T.cols;j++)
        {
            T.at<uchar>(i,j) = saturate_cast<uchar>(A - omega*minFilterImage.at<uchar>(i,j));
        }
    }

    //imshow("预估投射图",T);
    
    Mat srcROI = srcImage(Rect(radius,radius,T.cols,T.rows));

    Mat finalImage = Mat::zeros(srcROI.size(),srcROI.type());

    for(int i = 0;i<T.rows;i++)
    {
        for(int j = 0;j<T.cols;j++)
        {
            double tempT = max(T.at<uchar>(i,j),255*t0);
            
            for(int c = 0;c<finalImage.channels();c++)
            {
                finalImage.at<Vec3b>(i,j)[c] = saturate_cast<uchar>(double(srcROI.at<Vec3b>(i,j)[c] - A)/tempT +A);
            }
            
        }
    }

    finalImage *= 1.08;
     //boxFilter(finalImage,finalImage,-1,Size(3,3));

    // resize(srcROI,srcROI,Size(700,800));
    // resize(finalImage,finalImage,Size(700,800));
    imshow("srcImage",srcROI);
    imshow("final image",finalImage);
    
    double time =double((getTickCount()-startTime)/getTickFrequency());
    cout<<"时间为: "<<time<<endl;

    while(waitKey()!='q');
    return 0;
}

//最小化滤波
void minFilter(Mat&src,Mat&dst)
{
    if(src.channels()!=1)
    {
        cout<<"channel 是"<<src.channels();
        return;
    }
    
    //在windowsize*windowsize的窗口内寻找 新建的图片少卷积不到的地方
    int cols = src.cols-windowsize+1;
    int rows = src.rows-windowsize+1;

    Mat minImage = Mat::zeros(Size(cols,rows),CV_8UC1);

    for(int i = 0,x = radius;i<rows;i++,x++)
    {
        for(int j =0,y=radius;j<cols;j++,y++)
        {
            int minValue = src.at<uchar>(x,y);//先得到中点

            for(int a = 0;a<radius;a++)
            {
                for(int b =0;b<radius;b++)
                {
                    if(minValue>src.at<uchar>(x-a,y-b)) minValue=src.at<uchar>(x-a,y-b);
                    if(minValue>src.at<uchar>(x+a,y-b)) minValue=src.at<uchar>(x+a,y-b);
                    if(minValue>src.at<uchar>(x-a,y+b)) minValue=src.at<uchar>(x-a,y+b);
                    if(minValue>src.at<uchar>(x+a,y+b)) minValue=src.at<uchar>(x+a,y+b);
                }
            }
            minImage.at<uchar>(i,j) = minValue;
        }
    }
    dst = minImage;
}

//返回最大值 并且归一化
double max(double a,double b)
{
    return a>b?a/255:b/255;
}


double getAtmosphere(Mat&img,double precison)
{
    const int num = img.rows*img.cols*precison;
    int tempNum = num;

    //设定取值范围
    float range[2] = {0, 255};
    const float* histRange[1] = {range};
    Mat hist;
    int histSize[1] = {256};
    calcHist(&img,1,0,Mat(),hist,1,histSize,histRange,true,false);
    
    int sum = 0;
    for(int i =255;i>0;i--)
    {
        if(hist.at<float>(i)==0) continue;
        if(hist.at<float>(i)<tempNum)
        {
            tempNum -= hist.at<float>(i);
            sum+=i*hist.at<float>(i);
        }else
        {
            sum+=i*tempNum;
            tempNum = 0;
            break;
        }
    }

    return (sum/num);
}