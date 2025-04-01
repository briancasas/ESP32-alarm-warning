#include <WiFi.h>
#include <ESP_Mail_Client.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "time.h"

// Configuración de pantalla TFT ST7789
#define LCD_MOSI 23
#define LCD_SCLK 18
#define LCD_CS 15
#define LCD_DC 2
#define LCD_RST 4
#define LCD_BLK 32

Adafruit_ST7789 tft = Adafruit_ST7789(LCD_CS, LCD_DC, LCD_RST);

// Configuración WiFi
const char *ssid = "TU_SSID";
const char *password = "TU_PASSWORD";

// Configuración del correo
ESP_Mail_Session session;
SMTPSession smtp;
SMTP_Message message;

const char *email_sender = "TU_CORREO";
const char *email_password = "TU_CONTRASEÑA";
const char *smtp_server = "smtp.gmail.com";
const int smtp_port = 465;
const char *email_recipient = "DESTINATARIO_CORREO";

// Configuración del pin GPIO
const int alarmaPin = 10;
bool alarmaActiva = false;
bool estadoAnterior = LOW;
unsigned long ultimaVezCorreo = 0;
const unsigned long tiempoEspera = 300000;

// Configuración NTP
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;
const int daylightOffset_sec = 3600;

void configurarHora()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        Serial.println("Error al obtener la hora");
        return;
    }
    Serial.println("Hora sincronizada correctamente");
}

void setup()
{
    Serial.begin(115200);
    tft.init(135, 240);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2.5);
    tft.println("Iniciando...");

    configurarHora();
    pinMode(alarmaPin, INPUT_PULLDOWN);

    mostrarEnPantalla("Conectando a WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        mostrarEnPantalla(".");
    }
    mostrarEnPantalla("\nWiFi conectado.");
    Serial.println("\nConexión WiFi establecida.");
}

void loop()
{
    int estadoActual = digitalRead(alarmaPin);

    if (estadoActual == HIGH && estadoAnterior == LOW)
    {
        if (!alarmaActiva)
        {
            alarmaActiva = true;
            ultimaVezCorreo = millis();
            enviarCorreo();
            mostrarEnPantalla("\n\n\n\n\n\n\n\nALERTA\nALARMA ACTIVADA");
            Serial.println("\nALARMA ACTIVADA");
        }
    }
    else if (estadoActual == LOW && estadoAnterior == HIGH)
    {
        alarmaActiva = false;
        mostrarEnPantalla("Alarma inactiva...");
        Serial.println("Alarma inactiva...");
    }
    estadoAnterior = estadoActual;
}

void enviarCorreo()
{
    Serial.println("Enviando correo...");
    smtp.debug(1);

    session.server.host_name = smtp_server;
    session.server.port = smtp_port;
    session.login.email = email_sender;
    session.login.password = email_password;
    session.login.user_domain = "";

    message.clearRecipients();
    message.sender.name = "Alarma";
    message.sender.email = email_sender;
    message.subject = "¡Alarma activada!";
    message.addRecipient("Contacto", email_recipient);
    message.text.content = "Se ha activado la alarma. Verifique la situación.";
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority_high;

    if (!smtp.connect(&session))
    {
        Serial.println("Error al conectar con el servidor SMTP: " + smtp.errorReason());
        return;
    }

    if (!MailClient.sendMail(&smtp, &message))
    {
        Serial.println("Error al enviar el correo: " + smtp.errorReason());
    }
    else
    {
        Serial.println("Correo enviado con éxito");
    }
}

void mostrarEnPantalla(String mensaje)
{
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.println(mensaje);
}