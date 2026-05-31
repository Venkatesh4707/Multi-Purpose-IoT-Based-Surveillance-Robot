#include "esp_camera.h"
#include <WiFi.h>
#include <ESP_Mail_Client.h>

#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

/* ================= WIFI ================= */
const char* ssid = "Mr perfect";
const char* password = "perfect4707";

/* ================= EMAIL ================= */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

#define AUTHOR_EMAIL "venkateshsagar4707@gmail.com"
#define AUTHOR_PASSWORD "gflqrgdyrfuodqbz"
#define RECIPIENT_EMAIL "venkateshsagar0446@gmail.com"

SMTPSession smtp;
bool emailSent = false;

/* ================= EMAIL FUNCTION ================= */

void sendAlertEmail()
{
  SMTP_Message message;

  message.sender.name = "SMART SURVEILLANCE ROBOT";
  message.sender.email = AUTHOR_EMAIL;
  message.subject = "🚨 HUMAN DETECTED!";
  message.addRecipient("Owner", RECIPIENT_EMAIL);

  message.text.content =
  "Human detected by ESP32-CAM Surveillance Robot.";

  ESP_Mail_Session session;

  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = AUTHOR_EMAIL;
  session.login.password = AUTHOR_PASSWORD;

  smtp.debug(1);

  if (!smtp.connect(&session)) {
    Serial.println("SMTP FAILED");
    return;
  }

  if (MailClient.sendMail(&smtp, &message))
    Serial.println("EMAIL SENT ✅");
  else
    Serial.println("EMAIL FAILED");

  smtp.closeSession();
}

/* ================= CAMERA SERVER ================= */

#include "esp_http_server.h"
httpd_handle_t stream_httpd = NULL;

static esp_err_t stream_handler(httpd_req_t *req)
{
  camera_fb_t * fb = NULL;
  esp_err_t res = ESP_OK;

  res = httpd_resp_set_type(req,
        "multipart/x-mixed-replace;boundary=frame");

  while(true)
  {
    fb = esp_camera_fb_get();
    if(!fb){
      Serial.println("Camera capture failed");
      return ESP_FAIL;
    }

    /* ==== SIMPLE HUMAN TRIGGER ==== */
    if(!emailSent)
    {
      Serial.println("Human Detected!");
      sendAlertEmail();
      emailSent = true;
    }

    char part[64];
    size_t hlen = snprintf(part,64,
      "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n",
      fb->len);

    httpd_resp_send_chunk(req, part, hlen);
    httpd_resp_send_chunk(req,
      (const char*)fb->buf, fb->len);
    httpd_resp_send_chunk(req,"\r\n",2);

    esp_camera_fb_return(fb);
    delay(100);
  }
  return res;
}

void startCameraServer()
{
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();

  httpd_uri_t stream_uri = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = stream_handler,
    .user_ctx = NULL
  };

  httpd_start(&stream_httpd, &config);
  httpd_register_uri_handler(stream_httpd, &stream_uri);
}

/* ================= SETUP ================= */

void setup()
{
  Serial.begin(115200);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href  = HREF_GPIO_NUM;

  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn  = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size   = FRAMESIZE_QVGA;
  config.jpeg_quality = 10;
  config.fb_count     = 2;

  if (esp_camera_init(&config) != ESP_OK)
  {
    Serial.println("Camera Init Failed");
    return;
  }

  WiFi.begin(ssid,password);

  Serial.print("Connecting WiFi");
  while(WiFi.status()!=WL_CONNECTED){
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi Connected");

  startCameraServer();

  Serial.print("Camera Ready: http://");
  Serial.println(WiFi.localIP());
}

void loop()
{
  delay(10000);
}