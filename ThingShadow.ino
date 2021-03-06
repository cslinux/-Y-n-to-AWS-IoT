/*
 * Copyright 2010-2016 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include <aws_iot_mqtt.h>
#include <aws_iot_version.h>
#include "aws_iot_config.h"
#include <string.h>

aws_iot_mqtt_client myClient;
char JSON_buf[100];
int cnt = 0;
int rc = 1;
bool success_connect = false;

//LED Setup
const int NUM_PINS=10;
int ledPins[NUM_PINS] = {4,5,6,7,8,9,10,11,12,13};
int i = 0;

bool print_log(const char* src, int code) {
  bool ret = true;
  if(code == 0) {
    #ifdef AWS_IOT_DEBUG
      Serial.print(F("[LOG] command: "));
      Serial.print(src);
      Serial.println(F(" completed."));
    #endif
    ret = true;
  }
  else {
    #ifdef AWS_IOT_DEBUG
      Serial.print(F("[ERR] command: "));
      Serial.print(src);
      Serial.print(F(" code: "));
     Serial.println(code);
    #endif
    ret = false;
  }
  Serial.flush();
  return ret;
}

void toggle_light(uint8_t state)
{
  int i = 0;
  for (i = 0; i < NUM_PINS; i++)
  {
    digitalWrite(ledPins[i], state);
  }
}

void msg_callback_delta(char* src, unsigned int len, Message_status_t flag) {
  if(flag == STATUS_NORMAL) {
    // Get delta for light key
    print_log("getDeltaKeyValue", myClient.getDeltaValueByKey(src, "light", JSON_buf, 100));
    Serial.println(JSON_buf);
    if (strcmp(JSON_buf, "on") == 0)
        toggle_light(HIGH);
    else
        toggle_light(LOW); 
        
    String payload = "{\"state\":{\"reported\":";
    payload += "{\"light\":\"";
    payload += JSON_buf;
    payload += "\"}}}";
    payload.toCharArray(JSON_buf, 100);
    Serial.println(JSON_buf);
    print_log("update thing shadow", myClient.shadow_update(AWS_IOT_MY_THING_NAME, JSON_buf, strlen(JSON_buf), NULL, 5));
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);

  int i = 0;
  for (i = 0; i < NUM_PINS; i++)
  {
    pinMode(ledPins[i], OUTPUT);
  }
  
  //Disable this line if connected to wall plug!
  //while(!Serial);

  char curr_version[80];
  snprintf_P(curr_version, 80, PSTR("AWS IoT SDK Version(dev) %d.%d.%d-%s\n"), VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH, VERSION_TAG);
  Serial.println(curr_version);

  if(print_log("setup...", myClient.setup(AWS_IOT_CLIENT_ID, true, MQTTv311, true))) {
    if(print_log("config", myClient.config(AWS_IOT_MQTT_HOST, AWS_IOT_MQTT_PORT, AWS_IOT_ROOT_CA_PATH, AWS_IOT_PRIVATE_KEY_PATH, AWS_IOT_CERTIFICATE_PATH))) {
      if(print_log("connect", myClient.connect())) {
        success_connect = true;
        
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        delay(500);
        digitalWrite(LED_BUILTIN, HIGH);
        delay(500);
        digitalWrite(LED_BUILTIN, LOW);
        
        print_log("shadow init", myClient.shadow_init(AWS_IOT_MY_THING_NAME));
        print_log("register thing shadow delta function", myClient.shadow_register_delta_func(AWS_IOT_MY_THING_NAME, msg_callback_delta));
      }
    }
  }
}

void loop() {
  if(success_connect) {
    if(myClient.yield()) {
      Serial.println(F("Yield failed."));
    }
    delay(1000); // check for incoming delta per 100 ms
  }
}
