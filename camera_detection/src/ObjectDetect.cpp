#include "ObjectDetect.h"

ObjectDetect::ObjectDetect()
{
    graphPath = "./graph/frozen_inference_graph.pb";
    labelsPath = "../demo/ssd_mobilenet_v1_egohands/labels_map.pbtxt";

    // init session
    LOG(INFO) << "graphPath:" << graphPath;
    Status loadGraphStatus = loadGraph(graphPath, &session);
    if (!loadGraphStatus.ok()) {
        LOG(ERROR) << "loadGraph(): ERROR" << loadGraphStatus;
        initStatusOK = false;
        return;
    } else {
        LOG(INFO) << "loadGraph(): frozen graph loaded" << endl;
    }

    // init labelsMap
    labelsMap = std::map<int, std::string>();
    Status readLabelsMapStatus = readLabelsMapFile(labelsPath, labelsMap);
    if (!readLabelsMapStatus.ok()) {
        LOG(ERROR) << "readLabelsMapFile(): ERROR" << loadGraphStatus;
        initStatusOK = false;
        return;
    } else {
        LOG(INFO) << "readLabelsMapFile(): labels map loaded with "
                  << labelsMap.size() << " label(s)" << endl;
    }

    // init shape
    shape.AddDim(1);
    shape.AddDim((int64)frameHeight);
    shape.AddDim((int64)frameWidth);
    shape.AddDim(3);

    result = nullptr;
}

int ObjectDetect::runDetect(cv::Mat& frame)
{
    //FssLog::getInstance().WriteMethodLog("ObjectDetect", "runDetect", "");
    cvtColor(frame, frame, COLOR_BGR2RGB);

    // Convert mat to tensor
    tensor = Tensor(tensorflow::DT_FLOAT, shape);
    Status readTensorStatus = readTensorFromMat(frame, tensor);
    if (!readTensorStatus.ok()) {
        LOG(ERROR) << "Mat->Tensor conversion failed: " << readTensorStatus;
        return -1;
    }

    // Run the graph on tensor
    outputs.clear();
    Status runStatus = session->Run({ { inputLayer, tensor } }, outputLayer, {}, &outputs);
    if (!runStatus.ok()) {
        LOG(ERROR) << "Running model failed: " << runStatus;
        return -1;
    }

    // Extract results from the outputs vector
    tensorflow::TTypes<float>::Flat scores = outputs[1].flat<float>();
    tensorflow::TTypes<float>::Flat classes = outputs[2].flat<float>();
    tensorflow::TTypes<float>::Flat numDetections = outputs[3].flat<float>();
    tensorflow::TTypes<float, 3>::Tensor boxes = outputs[0].flat_outer_dims<float, 3>();

    vector<size_t> goodIdxs = filterBoxes(scores, boxes, thresholdIOU, thresholdScore);
    for (size_t i = 0; i < goodIdxs.size(); i++)
        LOG(INFO) << "score:" << scores(goodIdxs.at(i))
                  << ",class:" << labelsMap[classes(goodIdxs.at(i))] << " ("
                  << classes(goodIdxs.at(i)) << "), box:"
                  << "," << boxes(0, goodIdxs.at(i), 0) << ","
                  << boxes(0, goodIdxs.at(i), 1) << ","
                  << boxes(0, goodIdxs.at(i), 2) << ","
                  << boxes(0, goodIdxs.at(i), 3);

    // Draw bboxes and captions
    cvtColor(frame, frame, COLOR_BGR2RGB);
    drawBoundingBoxesOnImage(frame, scores, classes, boxes, labelsMap, goodIdxs);

    if (goodIdxs.size() > 0) {
        // imwrite("label.png",frame);
        if (nullptr != result) {
            delete result;
        }

        result = new QJsonObject();
        result->insert("plateNo", "suA88888");
        result->insert("pictureWidth", to_string(frameWidth).c_str());
        result->insert("pictureHeight", to_string(frameHeight).c_str());

        vector<uchar> img_content;
        imencode(".png", frame, img_content);
        uchar* pPngData = reinterpret_cast<uchar*>(img_content.data());
        QString src = QString::fromStdString(
            QByteArray::fromRawData((char*)pPngData, img_content.size())
                .toBase64()
                .toStdString());

        //        string str_img(img_content.begin(),img_content.end());
        //        QString src=QString::fromStdString(str_img);
        result->insert("pictureContent", src);

        QJsonArray arrLoc;
        for (size_t i = 0; i < goodIdxs.size(); i++) {
            QJsonObject obj;
            int yMin = boxes(0, goodIdxs.at(i), 0) * frame.rows;
            int xMin = boxes(0, goodIdxs.at(i), 1) * frame.cols;

            int yMax = boxes(0, goodIdxs.at(i), 2) * frame.rows;
            int xMax = boxes(0, goodIdxs.at(i), 3) * frame.cols;

            obj.insert("locationX", to_string(xMin).c_str());
            obj.insert("locationY", to_string(yMin).c_str());
            obj.insert("width", to_string(xMax - xMin).c_str());
            obj.insert("height", to_string(yMax - yMin).c_str());
            obj.insert("type", labelsMap[classes(goodIdxs.at(i))].c_str());

            arrLoc.push_back(obj);
        }

        result->insert("detectObjects", arrLoc);

        cout << "########Size : " << result->size() << " ##########" << endl;
        return 1;
    }

    return 0;
}

//modify
Status ObjectDetect::loadGraph(const string& graph_file_name,
    unique_ptr<tensorflow::Session>* session)
{
    tensorflow::GraphDef graph_def;
    Status load_graph_status = ReadBinaryProto(tensorflow::Env::Default(), graph_file_name, &graph_def);
    if (!load_graph_status.ok()) {
        return tensorflow::errors::NotFound("Failed to load compute graph at '",
            graph_file_name, "'");
    }
    session->reset(tensorflow::NewSession(tensorflow::SessionOptions()));
    Status session_create_status = (*session)->Create(graph_def);
    if (!session_create_status.ok()) {
        return session_create_status;
    }
    return Status::OK();
}

Status ObjectDetect::readLabelsMapFile(const string& fileName,
    map<int, string>& labelsMap)
{
    labelsMap.insert(pair<int, string>(1, "Nest"));
    return Status::OK();
}

/** Convert Mat image into tensor of shape (1, height, width, d) where last
 * three dims are equal to the original dims.
 */
Status ObjectDetect::readTensorFromMat(const Mat& mat, Tensor& outTensor)
{
    auto root = tensorflow::Scope::NewRootScope();
    using namespace ::tensorflow::ops;

    // Trick from https://github.com/tensorflow/tensorflow/issues/8033
    float* p = outTensor.flat<float>().data();
    Mat fakeMat(mat.rows, mat.cols, CV_32FC3, p);
    mat.convertTo(fakeMat, CV_32FC3);

    auto input_tensor = Placeholder(root.WithOpName("input"), tensorflow::DT_FLOAT);
    vector<pair<string, tensorflow::Tensor>> inputs = { { "input", outTensor } };
    auto uint8Caster = Cast(root.WithOpName("uint8_Cast"), outTensor, tensorflow::DT_UINT8);

    // This runs the GraphDef network definition that we've just constructed, and
    // returns the results in the output outTensor.
    tensorflow::GraphDef graph;
    TF_RETURN_IF_ERROR(root.ToGraphDef(&graph));

    vector<Tensor> outTensors;
    unique_ptr<tensorflow::Session> session(
        tensorflow::NewSession(tensorflow::SessionOptions()));

    TF_RETURN_IF_ERROR(session->Create(graph));
    TF_RETURN_IF_ERROR(session->Run({ inputs }, { "uint8_Cast" }, {}, &outTensors));

    outTensor = outTensors.at(0);
    return Status::OK();
}

/** Draw bounding box and add caption to the image.
 *  Boolean flag _scaled_ shows if the passed coordinates are in relative units
 * (true by default in tensorflow detection)
 */
void ObjectDetect::drawBoundingBoxOnImage(Mat& image, double yMin, double xMin,
    double yMax, double xMax,
    double score, string label,
    bool scaled = true)
{
    cv::Point tl, br;
    if (scaled) {
        tl = cv::Point((int)(xMin * image.cols), (int)(yMin * image.rows));
        br = cv::Point((int)(xMax * image.cols), (int)(yMax * image.rows));
    } else {
        tl = cv::Point((int)xMin, (int)yMin);
        br = cv::Point((int)xMax, (int)yMax);
    }
    cv::rectangle(image, tl, br, cv::Scalar(0, 255, 255), 1);

    // Ceiling the score down to 3 decimals (weird!)
    float scoreRounded = floorf(score * 1000) / 1000;
    string scoreString = to_string(scoreRounded).substr(0, 5);
    string caption = label + " (" + scoreString + ")";

    // Adding caption of type "LABEL (X.XXX)" to the top-left corner of the
    // bounding box
    int fontCoeff = 12;
    cv::Point brRect = cv::Point(tl.x + caption.length() * fontCoeff / 1.6, tl.y + fontCoeff);
    cv::rectangle(image, tl, brRect, cv::Scalar(0, 255, 255), -1);
    cv::Point textCorner = cv::Point(tl.x, tl.y + fontCoeff * 0.9);
    cv::putText(image, caption, textCorner, FONT_HERSHEY_SIMPLEX, 0.4,
        cv::Scalar(255, 0, 0));
}

/** Draw bounding boxes and add captions to the image.
 *  Box is drawn only if corresponding score is higher than the _threshold_.
 */
void ObjectDetect::drawBoundingBoxesOnImage(
    Mat& image, tensorflow::TTypes<float>::Flat& scores,
    tensorflow::TTypes<float>::Flat& classes,
    tensorflow::TTypes<float, 3>::Tensor& boxes, map<int, string>& labelsMap,
    vector<size_t>& idxs)
{
    for (int j = 0; j < idxs.size(); j++)
        drawBoundingBoxOnImage(image, boxes(0, idxs.at(j), 0),
            boxes(0, idxs.at(j), 1), boxes(0, idxs.at(j), 2),
            boxes(0, idxs.at(j), 3), scores(idxs.at(j)),
            labelsMap[classes(idxs.at(j))]);
}

/** Calculate intersection-over-union (IOU) for two given bbox Rects.
 */
double ObjectDetect::IOU(Rect2f box1, Rect2f box2)
{
    float xA = max(box1.tl().x, box2.tl().x);
    float yA = max(box1.tl().y, box2.tl().y);
    float xB = min(box1.br().x, box2.br().x);
    float yB = min(box1.br().y, box2.br().y);

    float intersectArea = abs((xB - xA) * (yB - yA));
    float unionArea = abs(box1.area()) + abs(box2.area()) - intersectArea;

    return 1. * intersectArea / unionArea;
}

/** Return idxs of good boxes (ones with highest confidence score (>=
 * thresholdScore) and IOU <= thresholdIOU with others).
 */
vector<size_t> ObjectDetect::filterBoxes(
    tensorflow::TTypes<float>::Flat& scores,
    tensorflow::TTypes<float, 3>::Tensor& boxes, double thresholdIOU,
    double thresholdScore)
{
    vector<size_t> sortIdxs(scores.size());
    iota(sortIdxs.begin(), sortIdxs.end(), 0);

    // Create set of "bad" idxs
    set<size_t> badIdxs = set<size_t>();
    size_t i = 0;
    while (i < sortIdxs.size()) {
        if (scores(sortIdxs.at(i)) < thresholdScore)
            badIdxs.insert(sortIdxs[i]);
        if (badIdxs.find(sortIdxs.at(i)) != badIdxs.end()) {
            i++;
            continue;
        }

        Rect2f box1 = Rect2f(
            Point2f(boxes(0, sortIdxs.at(i), 1), boxes(0, sortIdxs.at(i), 0)),
            Point2f(boxes(0, sortIdxs.at(i), 3), boxes(0, sortIdxs.at(i), 2)));
        for (size_t j = i + 1; j < sortIdxs.size(); j++) {
            if (scores(sortIdxs.at(j)) < thresholdScore) {
                badIdxs.insert(sortIdxs[j]);
                continue;
            }
            Rect2f box2 = Rect2f(
                Point2f(boxes(0, sortIdxs.at(j), 1), boxes(0, sortIdxs.at(j), 0)),
                Point2f(boxes(0, sortIdxs.at(j), 3), boxes(0, sortIdxs.at(j), 2)));
            if (IOU(box1, box2) > thresholdIOU)
                badIdxs.insert(sortIdxs[j]);
        }
        i++;
    }

    // Prepare "good" idxs for return
    vector<size_t> goodIdxs = vector<size_t>();
    for (auto it = sortIdxs.begin(); it != sortIdxs.end(); it++)
        if (badIdxs.find(sortIdxs.at(*it)) == badIdxs.end())
            goodIdxs.push_back(*it);

    return goodIdxs;
}
