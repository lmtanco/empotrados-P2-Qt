#include "guipanel.h"
#include "ui_guipanel.h"
#include <QSerialPort>      // Comunicacion por el puerto serie
#include <QSerialPortInfo>  // Comunicacion por el puerto serie
#include <QMessageBox>

#include <QJsonObject>
#include <QJsonDocument>

#include<stdint.h>      // Cabecera para usar tipos de enteros con tamaño
#include<stdbool.h>     // Cabecera para usar booleanos


GUIPanel::GUIPanel(QWidget *parent) :  // Constructor de la clase
    QWidget(parent),
    ui(new Ui::GUIPanel)               // Indica que guipanel.ui es el interfaz grafico de la clase
  , transactionCount(0)
{
    ui->setupUi(this);                // Conecta la clase con su interfaz gráfico.
    ui->leHost->setText("192.168.2.3"); // valor por defecto
    ui->pub_topic_intro->setText("casa/luces");
    setWindowTitle(tr("Control Domótico")); // Título de la ventana


    _client=new QMQTT::Client(QHostAddress::LocalHost, 1883); //localhost y lo otro son valores por defecto


    connect(_client, SIGNAL(connected()), this, SLOT(onMQTT_Connected()));
    connect(_client, SIGNAL(received(const QMQTT::Message &)), this, SLOT(onMQTT_Received(const QMQTT::Message &)));
    connect(_client, SIGNAL(subscribed(const QString &)), this, SLOT(onMQTT_subscribed(const QString &)));
    fMQTTconnected=false;                 // Todavía no hemos establecido la conexión MQTT

    ui->intrusion->setVisible(false);
    ui->c_iluminacion->setEnabled(false);
}

GUIPanel::~GUIPanel() // Destructor de la clase
{
    delete ui;   // Borra el interfaz gráfico asociado a la clase
}


// Establecimiento de la comunicación USB serie a través del interfaz seleccionado en la comboBox, tras pulsar el
// botón RUN del interfaz gráfico. Se establece una comunicacion a 9600bps 8N1 y sin control de flujo en el objeto
// 'serial' que es el que gestiona la comunicación USB serie en el interfaz QT
void GUIPanel::startClient()
{
    _client->setHostName(ui->leHost->text());
    _client->setPort(1883);
    _client->setKeepAlive(300);
    _client->setCleanSession(true);
    _client->connectToHost();

}

// Funcion auxiliar de procesamiento de errores de comunicación (usada por startSlave)
void GUIPanel::processError(const QString &s)
{
    activateRunButton(); // Activa el botón RUN
    // Muestra en la etiqueta de estado la razón del error (notese la forma de pasar argumentos a la cadena de texto)
    ui->statusLabel->setText(tr("Estado: No se ejecuta, %1.").arg(s));
}

// Funcion de habilitacion del boton de inicio/conexion
void GUIPanel::activateRunButton()
{
    ui->runButton->setEnabled(true);
}

// SLOT asociada a pulsación del botón RUN
void GUIPanel::on_runButton_clicked()
{
    startClient();
}


void GUIPanel::onMQTT_subscribed(const QString &topic)
{
    ui->statusLabel->setText(tr("suscrito a %1").arg(topic));
}


void GUIPanel::on_pushButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}


void GUIPanel::onMQTT_Received(const QMQTT::Message &message)
{
    bool previousblockinstate,checked;
    if (fMQTTconnected)
    {

        QJsonParseError error;
        QJsonDocument mensaje=QJsonDocument::fromJson(message.payload(),&error);

        if ((error.error==QJsonParseError::NoError)&&(mensaje.isObject()))
        { //Tengo que comprobar que el mensaje es del tipo adecuado y no hay errores de parseo...

            QJsonObject objeto_json=mensaje.object();
            QJsonValue entrada=objeto_json["luz1_r"]; //Obtengo la entrada redLed. Esto lo puedo hacer porque el operador [] está sobrecargado
            // Ejemplo de actualizacion de checkbox de control de luz por cambio en otro interfaz
            if (entrada.isBool()){   //Compruebo que es booleano...

                checked=entrada.toBool(); //Leo el valor de objeto (si fuese entero usaria toInt(), toDouble() si es doble....
                previousblockinstate=ui->controlLuz1_r->blockSignals(true);   //Esto es para evitar que el cambio de valor
                //provoque otro envio al topic por el que he recibido

                ui->controlLuz1_r-> setChecked(checked);
                if(checked)
                    ui->luz1_r->setPixmap(QPixmap(":/images/BulbOn.png"));
                else
                    ui->luz1_r->setPixmap(QPixmap(":/images/BulbOff.png"));

                ui->controlLuz1_r->blockSignals(previousblockinstate);
            }
            entrada=objeto_json["luz2_y"];
            if (entrada.isBool()){
                checked=entrada.toBool();
                previousblockinstate=ui->controlLuz2_y->blockSignals(true);
                ui->controlLuz2_y->setChecked(checked);
                if(checked)
                    ui->luz2_y->setPixmap(QPixmap(":/images/BulbOn.png"));
                else
                    ui->luz2_y->setPixmap(QPixmap(":/images/BulbOff.png"));
                ui->controlLuz2_y->blockSignals(previousblockinstate);
            }
            entrada=objeto_json["luz3_g"];
            if (entrada.isBool()){
                checked=entrada.toBool();
                previousblockinstate=ui->controlLuz3_g->blockSignals(true);
                ui->controlLuz3_g->setChecked(checked);
                if(checked)
                    ui->luz3_g->setPixmap(QPixmap(":/images/BulbOn.png"));
                else
                    ui->luz3_g->setPixmap(QPixmap(":/images/BulbOff.png"));
                ui->controlLuz3_g->blockSignals(previousblockinstate);
            }
        }
    }
}


/* -----------------------------------------------------------
 MQTT Client Slots
 -----------------------------------------------------------*/
void GUIPanel::onMQTT_Connected()
{

    ui->runButton->setEnabled(false);
    pub_topic=ui->pub_topic_intro->text(); // También sería posible poner un topic "fijo" (con una cadena simplemente: "/topic" )
    // Se indica que se ha realizado la conexión en la etiqueta 'statusLabel'
    ui->statusLabel->setText(tr("Ejecucion, conectado al servidor"));

    ui->c_iluminacion->setEnabled(true);

    fMQTTconnected=true;

    _client->subscribe(pub_topic,0);
}



void GUIPanel::on_controlLuz1_r_stateChanged(int arg1)
{
    QJsonObject objeto_json;
    //Añade un campo "luz1_r" al objeto JSON, con el valor (true o false) contenido en checked
    objeto_json["luz1_r"]=ui->controlLuz1_r->isChecked(); //Puedo hacer ["luz1_r"] porque el operador [] está sobrecargado.

    QJsonDocument mensaje(objeto_json); //crea un objeto QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    QMQTT::Message msg(0, pub_topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON
    _client->publish(msg); //     //Publica el mensaje

    if(Qt::Checked == arg1)
        ui->luz1_r->setPixmap(QPixmap(":/images/BulbOn.png"));
    else
        ui->luz1_r->setPixmap(QPixmap(":/images/BulbOff.png"));
}

void GUIPanel::on_statusButton_clicked()
{
    ui->statusLabel->setText(tr(""));
}


void GUIPanel::on_pruebaCamara_pressed()
{
    ui->camara->setPixmap(QPixmap(":/images/CamaraIAlarm.png"));
}


void GUIPanel::on_pruebaCamara_released()
{
    ui->camara->setPixmap(QPixmap(":/images/CamaraI.png"));
}


void GUIPanel::on_controlLuz2_y_stateChanged(int arg1)
{
    QJsonObject objeto_json;
    //Añade un campo "luz2_y" al objeto JSON, con el valor (true o false) contenido en checked
    objeto_json["luz2_y"]=ui->controlLuz2_y->isChecked(); //Puedo hacer ["luz2_y"] porque el operador [] está sobrecargado.

    QJsonDocument mensaje(objeto_json); //crea un objeto QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    QMQTT::Message msg(0, pub_topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON
    _client->publish(msg); //     //Publica el mensaje

    if(Qt::Checked == arg1)
        ui->luz2_y->setPixmap(QPixmap(":/images/BulbOn.png"));
    else
        ui->luz2_y->setPixmap(QPixmap(":/images/BulbOff.png"));
}



void GUIPanel::on_controlLuz3_g_stateChanged(int arg1)
{
    QJsonObject objeto_json;
    //Añade un campo "luz3_g" al objeto JSON, con el valor (true o false) contenido en checked
    objeto_json["luz3_g"]=ui->controlLuz3_g->isChecked(); //Puedo hacer ["luz3_g"] porque el operador [] está sobrecargado.

    QJsonDocument mensaje(objeto_json); //crea un objeto QJsonDocument conteniendo el objeto objeto_json (necesario para obtener el mensaje formateado en JSON)

    QMQTT::Message msg(0, pub_topic, mensaje.toJson()); //Crea el mensaje MQTT contieniendo el mensaje en formato JSON
    _client->publish(msg); //     //Publica el mensaje

    if(Qt::Checked == arg1)
        ui->luz3_g->setPixmap(QPixmap(":/images/BulbOn.png"));
    else
        ui->luz3_g->setPixmap(QPixmap(":/images/BulbOff.png"));
}


