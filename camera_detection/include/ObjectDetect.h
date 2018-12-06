/*
**定义TensorFlow目标检测类
**
** Authour: leaf
*/

#ifndef _OBJECT_DETECT_H
#define _OBJECT_DETECT_H


#include <cv.h>
#include <fstream>
#include <highgui.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <string>
#include <utility>
#include <vector>

#include "tensorflow/cc/ops/const_op.h"
#include "tensorflow/cc/ops/image_ops.h"
#include "tensorflow/cc/ops/standard_ops.h"
#include "tensorflow/core/framework/graph.pb.h"
#include "tensorflow/core/graph/default_device.h"
#include "tensorflow/core/graph/graph_def_builder.h"
#include "tensorflow/core/lib/core/threadpool.h"
#include "tensorflow/core/lib/io/path.h"
#include "tensorflow/core/lib/strings/stringprintf.h"
#include "tensorflow/core/platform/init_main.h"
#include "tensorflow/core/public/session.h"
#include "tensorflow/core/util/command_line_flags.h"

#include <cv.hpp>
#include <opencv2/core/mat.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <time.h>
#include <QJsonObject>
#include <QJsonArray>

using tensorflow::Flag;
using tensorflow::int32;
using tensorflow::Status;
using tensorflow::string;
using tensorflow::Tensor;

using namespace cv;
using namespace std;

class ObjectDetect {
public:
    ObjectDetect();
 
    /*
    **运行检测
    ** frame: 输入帧数据　Mat格式
    ** 返回: 
    **      1: 检测到物体
    **      0: 未检测到物体
    **     -1: 运行出错
    */
    int runDetect(cv::Mat& frame);

    /*
    ** 获取初始化状态
    */
    bool getInitStatus() { return initStatusOK; }

    /*
    ** 获取JsonObject 返回检测结果
    */
    QJsonObject* getJsonResult() { return result; }

private:

    /*
    ** 加载模型计算图
    ** graph_file_name: 模型计算图路径
    ** session: 执行计算图会话
    ** 返回：
    **      Status 状态值
    */
    Status loadGraph(const string& graph_file_name,
        std::unique_ptr<tensorflow::Session>* session);

    /*
    ** 从文件中读取标签类别数据
    ** fileName: 文件路径
    ** labelsMap: 存储id:类别键值对
    ** 返回：
    **      Status 状态值
    */
    Status readLabelsMapFile(const string& fileName,
        std::map<int, string>& labelsMap);

    /*
    ** 从opencv　Mat转换成Tensor张量
    ** mat: 帧源数据
    ** outTensor: 输出张量
    ** 返回：
    **      Status 状态值
    */
    Status readTensorFromMat(const cv::Mat& mat, Tensor& outTensor);

    /*
    ** 在图片上画一个框
    ** image: 原始帧数据
    ** xMin: 左上角X坐标
    ** yMin: 左上角Ｙ坐标
    ** xMax: 右下角X坐标
    ** yMax: 右下角Y坐标
    ** score: 当前物体属于某个类别的可信度
    ** label: 物体类别标签
    ** scaled: 是否需要放大
    ** 返回:
    **      空
    */
    void drawBoundingBoxOnImage(cv::Mat& image, double xMin,
        double yMin, double xMax, double yMax, double score, std::string label, bool scaled);

    /*
    ** 在图片上画多个框
    ** image: 原始帧数据
    ** scores: 多个物体类别可信度
    ** classes: 多个物体类别
    ** boxes: 多个包围盒
    ** labelsMap: id对应的类别
    ** idxs: 包围盒对应的坐标
    ** 返回:
    **      空
    */
    void drawBoundingBoxesOnImage(cv::Mat& image,
        tensorflow::TTypes<float>::Flat& scores,
        tensorflow::TTypes<float>::Flat& classes,
        tensorflow::TTypes<float, 3>::Tensor& boxes,
        std::map<int, string>& labelsMap,
        std::vector<size_t>& idxs);

    /*
    ** 计算两个矩形交叠率
    ** box1: 矩形坐标
    ** box2: 矩形坐标
    ** 返回:
    **      double 交叠率
    */
    double IOU(Rect2f box1, Rect2f box2);

    /*
    ** 根据分数阀值和交叠率阀值过滤包围盒
    ** scores: 各个物体属于类别的可信度
    ** boxes:  包围盒
    ** thresholdIOU: 交叠率阀值
    ** thresholdScore: 分数阀值
    ** 返回:
    **      vector<size_t> 过滤后的包围盒序号
    */
    std::vector<size_t> filterBoxes(tensorflow::TTypes<float>::Flat& scores,
        tensorflow::TTypes<float, 3>::Tensor& boxes,
        double thresholdIOU, double thresholdScore);

private:
    string graphPath;                                           //模型存储路径
    string labelsPath;                                          //检测物体类别存储路径

    string inputLayer = "image_tensor:0";                       //输入层，图片张量
    vector<string> outputLayer = {  "detection_boxes:0",
                                    "detection_scores:0",
                                    "detection_classes:0",
                                    "num_detections:0" };       //输出结果层，包围盒，检测分数，检测类别，检测物体数目
    unique_ptr<tensorflow::Session> session;                    //模型计算会话
    std::map<int, std::string> labelsMap;                       //数字类别标签映射

    Tensor tensor;                                              //张量
    std::vector<Tensor> outputs;                                //输出张量
    double thresholdScore = 0.5;                                //分数阀值
    double thresholdIOU = 0.8;                                  //包围盒相互交叉的面积阀值

    tensorflow::TensorShape shape = tensorflow::TensorShape();  //张量维度
    bool initStatusOK = true;                                   //初始化状态
    int frameHeight = 480;                                      //帧图片高度
    int frameWidth = 600;                                       //帧图片宽度

    QJsonObject* result;                                        //返回检测结果JsonObject
};

#endif
