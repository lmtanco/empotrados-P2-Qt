#ifndef GUIPANEL_H
#define GUIPANEL_H

#include <QWidget>
#include <QtSerialPort/qserialport.h>
#include "qmqtt.h"

namespace Ui {
class GUIPanel;
}

//QT4:QT_USE_NAMESPACE_SERIALPORT

class GUIPanel : public QWidget
{
    Q_OBJECT
    
public:
    //GUIPanel(QWidget *parent = 0);
    explicit GUIPanel(QWidget *parent = 0);
    ~GUIPanel(); // Da problemas
    
private slots:

    void on_runButton_clicked();
    void on_pushButton_clicked();

    void onMQTT_Received(const QMQTT::Message &message);
    void onMQTT_Connected(void);

    void onMQTT_subscribed(const QString &topic);

    void on_controlLuz1_r_stateChanged(int arg1);
    
    void on_statusButton_clicked();

    void on_pruebaCamara_pressed();

    void on_pruebaCamara_released();

    void on_controlLuz2_y_stateChanged(int arg1);

    void on_controlLuz3_g_stateChanged(int arg1);

    void on_pushButton_sendRGB_clicked();

private: // funciones privadas
//    void pingDevice();
    void startClient();
    void processError(const QString &s);
    void activateRunButton();
private:
    Ui::GUIPanel *ui;
    int transactionCount;
    QMQTT::Client *_client;
    bool fMQTTconnected;
    QString pub_topic;
};

#endif // GUIPANEL_H
