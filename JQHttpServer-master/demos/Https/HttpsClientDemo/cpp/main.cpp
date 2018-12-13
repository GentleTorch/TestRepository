// Qt lib import
#include <QtCore>

// JQLibrary import
#include "JQNet.h"

int main(int argc, char* argv[])
{
    QCoreApplication a(argc, argv);

    QJsonObject obj;
    obj.insert("CameraSpin", true);
    obj.insert("CameraDetect", true);

    const auto&& reply = JQNet::HTTP::post("https://192.168.100.107:24684/pc_vas/CruiseControl", QJsonDocument(obj).toJson());
    //    const auto &&reply = JQNet::HTTP::get( "https://127.0.0.1:24684/TestUrl" );

    qDebug() << reply.first << reply.second;

    return 0;
}
