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
const char *password = "TU_CONTRASEÑA_WIFI";

// Configuración del correo
ESP_Mail_Session session;
SMTPSession smtp;
SMTP_Message message;

const char *email_sender = "TU_EMAIL@gmail.com";  // Cambiar a tu correo electrónico
const char *email_password = "TU_CONTRASEÑA_APP"; // Contraseña de aplicación, obtener desde la configuración de seguridad de tu cuenta
const char *smtp_server = "smtp.gmail.com";
const int smtp_port = 465;

const char *email_recipient = "DESTINATARIO_EMAIL"; // Correo del destinatario

// Configuración del pin GPIO
const int alarmaPin = 13;
bool alarmaActiva = false;
bool estadoAnterior = LOW; // Guarda el último estado leído
unsigned long ultimaVezCorreo = 0;
const unsigned long tiempoEspera = 300000; // 5 minutos (300000 ms)

// Configuración NTP
const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -10800;   // UTC-3 (ajustar según tu zona horaria)
const int daylightOffset_sec = 3600; // Cambio horario, ajusta si es necesario

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

    // Inicializar la pantalla
    tft.init(135, 240);
    tft.setRotation(1);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE, ST77XX_BLACK);
    tft.setTextSize(2.5);
    tft.println("Iniciando...");

    // Configurar el tiempo
    configurarHora();

    // Configurar el pin como entrada con pull-down para evitar lecturas erróneas
    pinMode(alarmaPin, INPUT_PULLDOWN);

    // Conectar a WiFi
    mostrarEnPantalla("Conectando a WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
        mostrarEnPantalla("Conectando...");
    }
    mostrarEnPantalla("\nWiFi conectado.");
    Serial.println("\nConexión WiFi establecida.");
}

void loop()
{
    int estadoActual = digitalRead(alarmaPin);

    // Detectar el cambio de estado de LOW -> HIGH (activación)
    if (estadoActual == HIGH && estadoAnterior == LOW)
    {
        if (!alarmaActiva)
        {
            alarmaActiva = true;
            ultimaVezCorreo = millis();
            enviarCorreo();
            mostrarEnPantalla("¡Alarma activada!");
            Serial.println("¡Alarma activada!");
        }
    }
    // Detectar el cambio de estado de HIGH -> LOW (desactivación)
    else if (estadoActual == LOW && estadoAnterior == HIGH)
    {
        alarmaActiva = false;
        mostrarEnPantalla("Alarma inactiva...");
        Serial.println("Alarma inactiva...");
    }

    estadoAnterior = estadoActual; // Actualizar el estado anterior
}

void enviarCorreo()
{
    Serial.println("Enviando correo...");

    smtp.debug(1); // Activa la depuración para ver errores

    // Configurar sesión SMTP
    session.server.host_name = smtp_server;
    session.server.port = smtp_port;
    session.login.email = email_sender;
    session.login.password = email_password;
    session.login.user_domain = "";

    // Limpiar destinatarios antes de agregar uno nuevo
    message.clearRecipients();

    // Configurar los datos del mensaje
    message.sender.name = "Alarma Incendios";
    message.sender.email = email_sender;
    message.subject = "¡Alarma activada!";
    message.addRecipient("Contacto", email_recipient);
    message.text.content = "La alarma de incendios ha sido activada. Por favor, verifica la situación.";
    message.text.charSet = "utf-8";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;
    message.priority = esp_mail_smtp_priority_high;

    // Conectar al servidor SMTP
    if (!smtp.connect(&session))
    {
        Serial.println("Error al conectar con el servidor SMTP: " + smtp.errorReason());
        return;
    }

    // Enviar el correo
    if (!MailClient.sendMail(&smtp, &message))
    {
        Serial.println("Error al enviar el correo: " + smtp.errorReason());
    }
    else
    {
        Serial.println("Correo enviado con éxito");
    }
}

// Función para mostrar mensajes en la pantalla
void mostrarEnPantalla(String mensaje)
{
    tft.fillScreen(ST77XX_BLACK);
    tft.setCursor(10, 10);
    tft.println(mensaje);
}

void mostrarEnPantallaAlerta(String mensaje)
{
    tft.fillScreen(ST77XX_WHITE);
    tft.setCursor(10, 10);
    tft.println(mensaje);
}
