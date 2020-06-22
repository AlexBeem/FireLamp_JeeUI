/*
Copyright © 2020 Dmytro Korniienko (kDn)
JeeUI2 lib used under MIT License Copyright (c) 2019 Marsel Akhkamov

    This file is part of FireLamp_JeeUI.

    FireLamp_JeeUI is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    FireLamp_JeeUI is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with FireLamp_JeeUI.  If not, see <https://www.gnu.org/licenses/>.

  (Этот файл — часть FireLamp_JeeUI.

   FireLamp_JeeUI - свободная программа: вы можете перераспространять ее и/или
   изменять ее на условиях Стандартной общественной лицензии GNU в том виде,
   в каком она была опубликована Фондом свободного программного обеспечения;
   либо версии 3 лицензии, либо (по вашему выбору) любой более поздней
   версии.

   FireLamp_JeeUI распространяется в надежде, что она будет полезной,
   но БЕЗО ВСЯКИХ ГАРАНТИЙ; даже без неявной гарантии ТОВАРНОГО ВИДА
   или ПРИГОДНОСТИ ДЛЯ ОПРЕДЕЛЕННЫХ ЦЕЛЕЙ. Подробнее см. в Стандартной
   общественной лицензии GNU.

   Вы должны были получить копию Стандартной общественной лицензии GNU
   вместе с этой программой. Если это не так, см.
   <https://www.gnu.org/licenses/>.)
*/

#include "main.h"
#include "effects.h"
#include "ui.h"

void setEffectParams(EFFECT *curEff);

#define SETPARAM(key, call) if (data->containsKey(F(key))) { \
    call; \
    jee.var(F(key), (*data)[F(key)]); \
}

#ifdef AUX_PIN
void AUX_toggle(bool key)
{
    if (key)
    {
        digitalWrite(AUX_PIN, AUX_LEVEL);
        jee.var(F("AUX"), (F("true")));
    }
    else
    {
        digitalWrite(AUX_PIN, !AUX_LEVEL);
        jee.var(F("AUX"), (F("false")));
    }
    //myLamp.sendStringToLamp(String(digitalRead(AUX_PIN) == AUX_LEVEL ? F("AUX ON") : F("AUX OFF")).c_str(), CRGB::White);
}
#endif


void event_worker(const EVENT *event) // обработка эвентов лампы
{
    LOG(printf_P, PSTR("%s - %s\n"), ((EVENT *)event)->getName().c_str(), myLamp.timeProcessor.getFormattedShortTime().c_str());

    String filename;
    String tmpStr = jee.param(F("txtColor"));
    tmpStr.replace(F("#"),F("0x"));
    CRGB::HTMLColorCode color = (CRGB::HTMLColorCode)strtol(tmpStr.c_str(),NULL,0);
    EFFECT *curEff = myLamp.effects.getCurrent();

    switch (event->event)
    {
    case EVENT_TYPE::ON :
        myLamp.setOnOff(true);
        jee.var(F("ONflag"), (myLamp.isLampOn()?F("true"):F("false")));
        myLamp.switcheffect(SW_SPECIFIC, myLamp.getFaderFlag(), curEff->eff_nb);
        break;
    case EVENT_TYPE::OFF :
        myLamp.disableEffectsUntilText();
        myLamp.setOffAfterText();
        //if(event->message) myLamp.sendStringToLamp(event->message,color);
        jee.var(F("ONflag"), (F("false")));
        //return;
        break;
    case EVENT_TYPE::ALARM :
        myLamp.startAlarm();
        //return; // если не нужен вывод текста - раскомментировать
        break;
    case EVENT_TYPE::DEMO_ON :
        if(myLamp.getMode() != MODE_DEMO || !myLamp.isLampOn())
            myLamp.startDemoMode();
        break;
    case EVENT_TYPE::LAMP_CONFIG_LOAD :
        filename=String(F("/glb/"));
        filename.concat(event->message);
        if(event->message)
            jee.load(filename.c_str());
        break;
    case EVENT_TYPE::EFF_CONFIG_LOAD :
        filename=String(F("/cfg/"));
        filename.concat(event->message);
        if(event->message)
            myLamp.effects.loadConfig(filename.c_str());
        break;
    case EVENT_TYPE::EVENTS_CONFIG_LOAD :
        filename=String(F("/evn/"));
        filename.concat(event->message);
        if(event->message)
            myLamp.events.loadConfig(filename.c_str());
        break;
    case EVENT_TYPE::SEND_TEXT :
        if(event->message==nullptr)
            break;
        if(!myLamp.isLampOn()){
            myLamp.disableEffectsUntilText(); // будем выводить текст, при выкюченной матрице
            myLamp.setOffAfterText();
            myLamp.setOnOff(true);
            myLamp.setBrightness(1,false,false); // выводить будем минимальной яркостью myLamp.getNormalizedLampBrightness()
            myLamp.sendStringToLamp(event->message,color);
        } else {
            myLamp.sendStringToLamp(event->message,color);
        }
        return;
    case EVENT_TYPE::PIN_STATE : {
            if(event->message==nullptr)
                break;
            //LOG(printf_P, PSTR("TEST: %s\n"),src);
            String tmpS(event->message);
            tmpS.replace(F("'"),F("\"")); // так делать не красиво, но шопаделаешь...
            DynamicJsonDocument doc(128);
            deserializeJson(doc,tmpS);
            JsonArray arr = doc.as<JsonArray>();
            for (size_t i=0; i<arr.size(); i++) {
                JsonObject item = arr[i];
                uint8_t pin = item[F("pin")].as<int>();
                String action = item[F("act")].as<String>();
                //LOG(printf_P, PSTR("text: %s, pin: %d - %s\n"), tmpS.c_str(), pin, action.c_str());
                pinMode(pin, OUTPUT);
                switch(action.c_str()[0]){
                    case 'H':
                        digitalWrite(pin, HIGH); // LOW
                        break;
                    case 'L':
                        digitalWrite(pin, LOW); // LOW
                        break;
                    case 'T':
                        digitalWrite(pin, !digitalRead(pin)); // inverse
                        break;
                    default:
                        break;
                }
                char tmpbuffer[32];
                sprintf_P(tmpbuffer, PSTR("Set PIN: %d to %s"), pin, action.c_str());
                myLamp.sendStringToLamp(tmpbuffer,color);
                return;
            }
        }
        break;
#ifdef AUX_PIN
    case EVENT_TYPE::AUX_ON:
        AUX_toggle(true);

        break;
    case EVENT_TYPE::AUX_OFF:
        AUX_toggle(false);

        break;
    case EVENT_TYPE::AUX_TOGGLE:
        digitalWrite(AUX_PIN, !digitalRead(AUX_PIN));
        jee.var(F("AUX"), (digitalRead(AUX_PIN) == AUX_LEVEL ? F("true") : F("false")));
        //return;
        break;
#endif
    default:
        break;
    }
    if(event->message) myLamp.sendStringToLamp(event->message,color);
    jee.refresh();
}

void pubCallback(Interface *interf){
    interf->json_frame_value();
    interf->value(F("pTime"), myLamp.timeProcessor.getFormattedShortTime(), true);
    interf->value(F("pMem"), String(ESP.getFreeHeap()), true);
    interf->json_frame_flush();
}

void block_menu(Interface *interf, JsonObject *data){
    // создаем меню
    interf->json_section_menu();

    interf->option(F("effects"), F("Эффекты"));
    interf->option(F("lamp"), F("Лампа"));
    interf->option(F("settings"), F("Настройки"));

    interf->json_section_end();
}

static EFFECT *confEff = nullptr;
void block_effects_config_param(Interface *interf, JsonObject *data){
    if (!confEff) return;

    interf->json_section_begin(F("eff_conf_prm"));
    interf->checkbox(F("canBeSelected"), confEff->canBeSelected? F("true") : F("false"), F("В списке выбора"), false);
    interf->checkbox(F("isFavorite"), confEff->isFavorite? F("true") : F("false"), F("В списке демо"), false);
    interf->text(F("param"), confEff->getParam(), F("Доп. параметры"));

    interf->button_submit(F("eff_conf_prm"), F("Сохранить"), F("gray"));
    interf->json_section_end();
}

void show_effects_config_param(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_effects_config_param(interf, data);
    interf->json_frame_flush();
}

void set_effects_config_param(Interface *interf, JsonObject *data){
    if (!confEff || !data) return;

    SETPARAM("param", confEff->updateParam((*data)[F("param")]));
    SETPARAM("canBeSelected", confEff->canBeSelected = ((*data)[F("canBeSelected")] == true));
    SETPARAM("isFavorite", confEff->isFavorite = ((*data)[F("isFavorite")] == true));

    String filename = String(F("/cfg/"))+jee.param(F("fileName"));
    if (filename.length() > 4 && SPIFFS.begin()) {
        myLamp.effects.saveConfig(filename.c_str());
    }
}

void block_effects_config(Interface *interf, JsonObject *data){
    interf->json_section_main(F("effects_config"), F("Конфигурирование"));

    EFFECT enEff; enEff.setNone();
    confEff = myLamp.effects.enumNextEffect(&enEff);

    interf->select(F("effListConf"), String((int)enEff.eff_nb), F("Эффект"), true);
    while ((enEff = *myLamp.effects.enumNextEffect(&enEff)).eff_nb != EFF_NONE) {
        interf->option(String((int)enEff.eff_nb), FPSTR(enEff.eff_name));
    }
    interf->json_section_end();

    block_effects_config_param(interf, nullptr);

    interf->spacer();
    interf->button(F("effects"), F("Выход"));
    interf->json_section_end();
}

void show_effects_config(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_effects_config(interf, data);
    interf->json_frame_flush();
}

void set_effects_config_list(Interface *interf, JsonObject *data){
    if (!data) return;
    EFF_ENUM num = (EFF_ENUM)(*data)[F("effListConf")];
    confEff = myLamp.effects.getEffectBy(num);
    show_effects_config_param(interf, data);
}

void block_effects_param(Interface *interf, JsonObject *data){
    EFFECT *curEff = myLamp.effects.getCurrent();
    if (!curEff) return;
    interf->json_section_begin(F("effects_param"));
    interf->range(F("bright"), curEff->brightness, 1, 255, 1, F("Яркость"), true);
    interf->range(F("speed"), curEff->speed, 1, 255, 1, F("Скорость"), true);
    interf->range(F("scale"), curEff->scale, 1, 255, 1, F("Масштаб"), true);

    String v = curEff->getValue(curEff->param, F("R"));
    if (!v.isEmpty()) {
        interf->range(F("extraR"), (int)v.toInt(), 1, 255, 1, F("Доп. регулятор"), true);
    }
    interf->json_section_end();
}

void show_effects_param(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_effects_param(interf, data);
    interf->json_frame_flush();
}

void set_effects_list(Interface *interf, JsonObject *data){
    if (!data) return;
    EFF_ENUM num = (EFF_ENUM)(*data)[F("effList")];
    EFFECT *curEff = myLamp.effects.getEffectBy(num);
    if (!curEff) return;

    if (!myLamp.isLampOn()) {
        myLamp.effects.moveBy(curEff->eff_nb); // переходим на выбранный эффект для начальной инициализации
    } else {
        myLamp.switcheffect(SW_SPECIFIC, myLamp.getFaderFlag(), curEff->eff_nb);
        setEffectParams(curEff);
    }
    myLamp.setLoading();
    jee.var(F("effList"), (*data)[F("effList")]);
    jee.save();

    show_effects_param(interf, data);
}

void set_effects_param(Interface *interf, JsonObject *data){
    EFFECT *curEff = myLamp.effects.getCurrent();
    if (!curEff || !data) return;

    SETPARAM("bright", curEff->brightness = (*data)[F("bright")]);
    SETPARAM("speed", curEff->speed = (*data)[F("speed")]);
    SETPARAM("scale", curEff->speed = (*data)[F("speed")]);
    SETPARAM("extraR", curEff->setValue(curEff->param, F("R"), (*data)[F("extraR")].as<String>().c_str()));

    String filename = String(F("/cfg/"))+jee.param(F("fileName"));
    if (filename.length() > 4 && SPIFFS.begin()) {
        myLamp.effects.saveConfig(filename.c_str());
    }
}

void block_effects_main(Interface *interf, JsonObject *data){
    // Страница "Управление эффектами"
    interf->json_section_main(F("effects"), F("Эффекты"));

    interf->checkbox(F("ONflag"), myLamp.isLampOn()? F("true") : F("false"), F("Включение лампы"), true);

    EFFECT enEff; enEff.setNone();
    interf->select(F("effList"), F("Эффект"), true);
    while ((enEff = *myLamp.effects.enumNextEffect(&enEff)).eff_nb != EFF_NONE) {
        if (enEff.canBeSelected) {
            interf->option(String((int)enEff.eff_nb), FPSTR(enEff.eff_name));
        }
    }
    interf->json_section_end();

    block_effects_param(interf, data);

    interf->button(F("effects_config"), F("Конфигурирование"));

    interf->json_section_end();
}

void set_onflag(Interface *interf, JsonObject *data){
    EFFECT *curEff = myLamp.effects.getCurrent();
    bool newpower = (*data)[F("ONflag")] == true;
    if (newpower != myLamp.isLampOn()) {
        if (newpower) {
            // включаем через switcheffect, т.к. простого isOn недостаточно чтобы запустить фейдер и поменять яркость (при необходимости)
            myLamp.switcheffect(SW_SPECIFIC, myLamp.getFaderFlag(), curEff->eff_nb);
        } else {
            myLamp.setOnOff(newpower);
        }
    }
}

void set_demoflag(Interface *interf, JsonObject *data){
    bool newdemo = (*data)[F("Demo")] == true;
    switch (myLamp.getMode()) {
        case MODE_NORMAL:
            if (newdemo) myLamp.startDemoMode(); break;
        case MODE_DEMO:
            if (!newdemo) myLamp.startNormalMode(); break;
        default:;
    }
}

#ifdef OTA
void set_otaflag(Interface *interf, JsonObject *data){
    myLamp.startOTA();

    interf->json_frame_interface();
    interf->json_section_content();
    interf->button(F("OTA"), F("Обновление по ОТА-PIO"), F("grey"));
    interf->json_section_end();
    interf->json_frame_flush();
}
#endif

#ifdef AUX_PIN
void set_auxflag(Interface *interf, JsonObject *data){
    if (((*data)[F("AUX")] == true) != (digitalRead(AUX_PIN) == AUX_LEVEL ? true : false)) {
        AUX_toggle(!(digitalRead(AUX_PIN) == AUX_LEVEL ? true : false));

    }
    jee.save();
}
#endif

void block_lamp_config(Interface *interf, JsonObject *data){
    interf->json_section_hidden(F("lamp_config"), F("Конфигурации эффектов"));

    interf->json_section_begin(F("edit_lamp_config"));
    String cfg(F("Конфигурации")); cfg+=" ("; cfg+=jee.param(F("fileName")); cfg+=")";

    interf->select(F("fileName"), cfg);
    if(SPIFFS.begin()){
#ifdef ESP32
        File root = SPIFFS.open("/cfg");
        File file = root.openNextFile();
#else
        Dir dir = SPIFFS.openDir(F("/cfg"));
#endif
        String fn;
#ifdef ESP32
        while (file) {
            fn=file.name();
            if(!file.isDirectory()){
#else
        while (dir.next()) {
            fn=dir.fileName();
#endif

            fn.replace(F("/cfg/"),F(""));
            //LOG(println, fn);
            interf->option(fn, fn);
#ifdef ESP32
            file = root.openNextFile();
        }
#endif
        }
    }
    interf->json_section_end();

    interf->json_section_line();
    interf->button_submit_value(F("edit_lamp_config"), F("load"), F("Загрузить"), F("green"));
    interf->button_submit_value(F("edit_lamp_config"), F("del"), F("Удалить"), F("red"));
    interf->json_section_end();

    interf->json_section_end();

    interf->spacer();
    interf->json_section_begin(F("add_lamp_config"));
    interf->text(F("fileName"),F("Конфигурация"));
    interf->button_submit(F("add_lamp_config"), F("Создать"));
    interf->json_section_end();

    interf->json_section_end();
}

void show_lamp_config(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_lamp_config(interf, data);
    interf->json_frame_flush();
}

void add_lamp_config(Interface *interf, JsonObject *data){
    // Сохранение текущей конфигурации эффектов в ФС
    if (!data->containsKey(F("fileName"))) return;
    String name = (*data)[F("fileName")];

    String filename = String(F("/cfg/")) + name;
    if (SPIFFS.begin()) {
        // if(!SPIFFS.exists(F("/cfg")))
        //     SPIFFS.mkdir(F("/cfg"));
        myLamp.effects.saveConfig(filename.c_str());
        filename = String(F("/glb/")) + name;
        jee.save(filename.c_str());
        filename = String(F("/evn/")) + name;
        myLamp.events.saveConfig(filename.c_str());
    }
    show_lamp_config(interf, data);
}

void edit_lamp_config(Interface *interf, JsonObject *data){
    // Удаление конфигурации из ФС
    if (!data->containsKey(F("fileName")) || !data->containsKey(F("edit_lamp_config"))) return;
    String name = (*data)[F("fileName")];
    String act = (*data)[F("edit_lamp_config")];

    if (act == "del") {
        String filename = String(F("/cfg/")) + name;
        if (SPIFFS.begin()) SPIFFS.remove(filename);

        filename = String(F("/glb/")) + name;
        if (SPIFFS.begin()) SPIFFS.remove(filename);

        filename = String(F("/evn/")) + name;
        if (SPIFFS.begin()) SPIFFS.remove(filename);

    } else
    if (act == "load") {
        myLamp.effects.loadConfig(name.c_str());
        jee.var(F("fileName"), name);
        jee.save();
    }


    show_lamp_config(interf, data);
}

void block_lamp_textsend(Interface *interf, JsonObject *data){
    interf->json_section_begin(F("textsend"));

    interf->spacer(F("Вывести на лампе"));
    interf->text(F("msg"), F("Текст"));
    interf->color(F("txtColor"), F("Цвет сообщения"));
    interf->button_submit(F("textsend"), F("Отправить"), F("gray"));

    interf->json_section_end();
}

void set_lamp_textsend(Interface *interf, JsonObject *data){
    String tmpStr = (*data)[F("txtColor")];
    jee.var(F("txtColor"), tmpStr);
    jee.var(F("msg"), (*data)[F("msg")]);

    tmpStr.replace(F("#"), F("0x"));
    myLamp.sendStringToLamp((*data)[F("msg")], (CRGB::HTMLColorCode)strtol(tmpStr.c_str(), NULL, 0));
}

void block_lamp(Interface *interf, JsonObject *data){
    //Страница "Управление лампой"
    interf->json_section_main(F("lamp"), F("Лампа"));

    interf->json_section_line();
    interf->checkbox(F("ONflag"), myLamp.isLampOn()? F("true") : F("false"), F("Включение"), true);
    interf->checkbox(F("Demo"), (myLamp.getMode() == MODE_DEMO)? F("true") : F("false"), F("Демо режим"), true);
#ifdef AUX_PIN
    interf->checkbox(F("AUX"), F("Включение AUX"), true);
#endif
#ifdef MIC_EFFECTS
    interf->checkbox(F("Mic"), F("Микрофон"), true);
#endif
    interf->checkbox(F("Events"), myLamp.IsEventsHandled()? F("true") : F("false"), F("События"), true);
    interf->json_section_end();

    block_lamp_textsend(interf, data);
    block_lamp_config(interf, data);

    interf->json_section_end();
}

#ifdef MIC_EFFECTS
void block_settings_mic(Interface *interf, JsonObject *data){
    interf->json_section_main(F("set_mic"), F("Микрофон"));

    interf->checkbox(F("Mic"), F("Микрофон"), true);
    if (!iGLOBAL.isMicCal) {
        interf->number(F("micScale"), F("Коэф. коррекции нуля"));
        interf->number(F("micNoise"), F("Уровень шума, ед"));
        interf->range(F("micnRdcLvl"), 0, 4, 1, F("Шумодав"));
    }
    interf->button_submit(F("set_mic"), F("Сохранить"), F("grey"));
    interf->spacer();
    interf->button(F("mic_cal"), F("Калибровка микрофона"), iGLOBAL.isMicCal? F("grey") : F("red"));

    interf->spacer();
    interf->button(F("settings"), F("Выход"));

    interf->json_section_end();
}

void show_settings_mic(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_settings_mic(interf, data);
    interf->json_frame_flush();
}

void set_settings_mic(Interface *interf, JsonObject *data){
    SETPARAM("micScale", myLamp.setMicScale((*data)[F("micScale")].as<float>()));
    SETPARAM("micNoise", myLamp.setMicNoise((*data)[F("micNoise")].as<float>()));
    SETPARAM("micnRdcLvl", myLamp.setMicNoiseRdcLevel((MIC_NOISE_REDUCE_LEVEL)(*data)[F("micNoise")].as<long>()));
    jee.save();
}

void set_micflag(Interface *interf, JsonObject *data){
    SETPARAM("Mic", myLamp.setMicOnOff((*data)[F("isMicON")] == true));
    jee.save();
}

void set_settings_mic_calib(Interface *interf, JsonObject *data){
    if (!myLamp.isMicOnOff()) {
        myLamp.sendStringToLamp(String(F("Включите микрофон")).c_str(), CRGB::Red);
    } else
    if(!iGLOBAL.isMicCal) {
        myLamp.sendStringToLamp(String(F("Калибровка микрофона")).c_str(), CRGB::Red);
        myLamp.setMicCalibration();
        iGLOBAL.isMicCal = true;
    } else
    if (myLamp.isMicCalibration()) {
        myLamp.sendStringToLamp(String(F("... в процессе ...")).c_str(), CRGB::Red);
    } else {
        jee.var(F("micScale"), String(myLamp.getMicScale()));
        jee.var(F("micNoise"), String(myLamp.getMicNoise()));
        iGLOBAL.isMicCal = false;
        jee.save();
    }
}
#endif

void block_settings_wifi(Interface *interf, JsonObject *data){
    interf->json_section_main(F("settings_wifi"), F("WiFi"));
    // форма настроек Wi-Fi
    interf->json_section_hidden(F("set_wifi"), F("WiFi"));
    interf->text(F("ap_ssid"), F("AP/mDNS"));
    interf->text(F("ssid"), F("SSID"));
    interf->password(F("pass"), F("Password"));
    interf->button_submit(F("set_wifi"), F("Connect"), F("gray"));
    interf->json_section_end();

    // форма настроек MQTT
    interf->json_section_hidden(F("set_mqtt"), F("MQTT"));
    interf->text(F("m_host"), F("MQTT host"));
    interf->number(F("m_port"), F("MQTT port"));
    interf->text(F("m_user"), F("User"));
    interf->text(F("m_pass"), F("Password"));
    interf->number(F("mqtt_int"), F("Интервал mqtt сек."));
    interf->button_submit(F("set_mqtt"), F("Connect"), F("gray"));
    interf->json_section_end();

    interf->spacer();
    interf->button(F("settings"), F("Выход"));

    interf->json_section_end();
}

void show_settings_wifi(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_settings_wifi(interf, data);
    interf->json_frame_flush();
}

void set_settings_wifi(Interface *interf, JsonObject *data){
    SETPARAM("ap_ssid", 0);
    SETPARAM("ssid", 0);
    SETPARAM("pass", 0);

    jee.var(F("wifi"), F("STA"));
    jee.save();
    ESP.restart();
}

void set_settings_mqtt(Interface *interf, JsonObject *data){
    SETPARAM("m_host", 0);
    SETPARAM("m_port", 0);
    SETPARAM("m_user", 0);
    SETPARAM("m_pass", 0);
    SETPARAM("mqtt_int", iGLOBAL.mqtt_int = (*data)[F("mqtt_int")]);

    jee.save();
    ESP.restart();
}

void block_settings_other(Interface *interf, JsonObject *data){
    interf->json_section_main(F("set_other"), F("Разные"));

    interf->checkbox(F("isGLBbr"), F("Глобальная яркость"));
    interf->range(F("GlobBRI"), 1, 255, 1, F("Глобальная яркость"));
    interf->checkbox(F("MIRR_H"), F("Отзеркаливание H"));
    interf->checkbox(F("MIRR_V"), F("Отзеркаливание V"));
    interf->checkbox(F("isFaderON"), F("Плавное переключение эффектов"));
#ifdef ESP_USE_BUTTON
    interf->checkbox(F("isBtnOn"), F("Кнопка активна"));
#endif

    interf->spacer(F("Вывод текста"));
    interf->range(F("txtSpeed"),10,100,10,F("Задержка прокрутки текста"));
    interf->range(F("txtOf"),-1,10,1,F("Смещение вывода текста"));

    interf->select(F("perTime"), F("Периодический вывод времени"));
    interf->option(String(PERIODICTIME::PT_NOT_SHOW), F("Не выводить"));
    interf->option(String(PERIODICTIME::PT_EVERY_60), F("Каждый час"));
    interf->option(String(PERIODICTIME::PT_EVERY_30), F("Каждые полчаса"));
    interf->option(String(PERIODICTIME::PT_EVERY_15), F("Каждые 15 минут"));
    interf->option(String(PERIODICTIME::PT_EVERY_10), F("Каждые 10 минут"));
    interf->option(String(PERIODICTIME::PT_EVERY_5), F("Каждые 5 минут"));
    interf->option(String(PERIODICTIME::PT_EVERY_1), F("Каждую минуту"));
    interf->json_section_end();

    interf->spacer(F("Новогоднее поздравление"));
    interf->number(F("ny_period"), F("Период вывода новогоднего поздравления в минутах (0 - не выводить)"));
    interf->number(F("ny_unix"), F("UNIX дата/время нового года"));

    interf->button_submit(F("set_other"), F("Сохранить"), F("grey"));

    interf->spacer();
    interf->button(F("settings"), F("Выход"));

    interf->json_section_end();
}

void show_settings_other(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_settings_other(interf, data);
    interf->json_frame_flush();
}

void set_settings_other(Interface *interf, JsonObject *data){
    SETPARAM("isGLBbr", myLamp.setIsGlobalBrightness((*data)[F("isGLBbr")] == true));

    if (data->containsKey(F("GlobBRI"))) {
        if (myLamp.IsGlobalBrightness()) {
            myLamp.setGlobalBrightness((*data)[F("GlobBRI")].as<long>());
        }
        jee.var(F("GlobBRI"), (*data)[F("GlobBRI")]);
    }

    SETPARAM("MIRR_H", myLamp.setMIRR_H((*data)[F("MIRR_H")] == true));
    SETPARAM("MIRR_V", myLamp.setMIRR_V((*data)[F("isGLBbr")] == true));
    SETPARAM("isFaderON", myLamp.setFaderFlag((*data)[F("isGLBbr")] == true));

#ifdef ESP_USE_BUTTON
    SETPARAM("isBtnOn", myLamp.setButtonOn((*data)[F("isGLBbr")] == true));
#endif

    SETPARAM("txtSpeed", myLamp.setTextMovingSpeed((*data)[F("txtSpeed")]));
    SETPARAM("txtOf", myLamp.setTextOffset((*data)[F("txtOf")]));
    SETPARAM("perTime", myLamp.setPeriodicTimePrint((PERIODICTIME)(*data)[F("perTime")].as<long>()));
    SETPARAM("ny_period", myLamp.setNYMessageTimer((*data)[F("ny_period")]));
    SETPARAM("ny_unix", myLamp.setNYUnixTime((*data)[F("ny_unix")]));

    jee.save();
}

void block_settings_time(Interface *interf, JsonObject *data){
    interf->json_section_main(F("set_time"), F("Настройка времени"));

    interf->time(F("time"), F("Время"));
    interf->number(F("tm_offs"), F("Смещение времени в секундах для NTP"));
    interf->text(F("timezone"), F("Часовой пояс (http://worldtimeapi.org/api/timezone/)"));
    interf->checkbox(F("isTmSync"), F("Включить синхронизацию"));
    interf->button_submit(F("set_time"), F("Сохранить"), F("gray"));

    interf->spacer();
    interf->button(F("settings"), F("Выход"));

    interf->json_section_end();
}

void show_settings_time(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_settings_time(interf, data);
    interf->json_frame_flush();
}

void set_settings_time(Interface *interf, JsonObject *data){
    SETPARAM("tm_offs", myLamp.timeProcessor.SetOffset((*data)[F("tm_offs")]));
    SETPARAM("timezone", myLamp.timeProcessor.setTimezone((*data)[F("timezone")]));
    SETPARAM("time", myLamp.timeProcessor.setTime((*data)[F("time")]));
    SETPARAM("isTmSync", myLamp.timeProcessor.setIsSyncOnline((*data)[F("isTmSync")] == true));

    if (myLamp.timeProcessor.getIsSyncOnline()) {
        myLamp.refreshTimeManual(); // принудительное обновление времени
    }

    myLamp.sendStringToLamp(myLamp.timeProcessor.getFormattedShortTime().c_str(), CRGB::Green);
    jee.save();
}

void block_settings_update(Interface *interf, JsonObject *data){
    interf->json_section_hidden(F("update"), F("Обновление"));
#ifdef OTA
    interf->spacer(F("Обновление по ОТА-PIO"));
    if (myLamp.getMode() != MODE_OTA) {
        interf->button(F("OTA"), F("Начать"), F("blue"));
    } else {
        interf->button(F("OTA"), F("Включено"), F("grey"));
    }
#endif
    interf->spacer(F("Загрузка прошивки"));
    interf->file(F("update"), F("update"), F("Upload"));
    interf->json_section_end();
}

void block_settings_event(Interface *interf, JsonObject *data){
    interf->json_section_main(F("show_event"), F("События"));

    interf->checkbox(F("Events"), myLamp.IsEventsHandled()? F("true") : F("false"), F("События"), true);

    int num = 1;
    EVENT *next = nullptr;
    while ((next = myLamp.events.getNextEvent(next)) != nullptr) {
        String name = "evconf" + String(num);
        interf->json_section_begin(name, next->getName(), false, false, true);
        interf->hidden(F("event"), String(num));
        interf->button_submit_value(name, F("edit"), F("Редактировать"), F("green"));
        interf->button_submit_value(name, F("del"), F("Удалить"), F("red"));
        interf->json_section_end();
        ++num;
    }

    interf->button(F("event_conf"), F("Добавить событие"));

    interf->spacer();
    interf->button(F("settings"), F("Выход"));

    interf->json_section_end();
}

void show_settings_event(Interface *interf, JsonObject *data){
    interf->json_frame_interface();
    block_settings_event(interf, data);
    interf->json_frame_flush();
}

void set_eventflag(Interface *interf, JsonObject *data){
    myLamp.setIsEventsHandled((*data)[F("Events")]);
}

void set_event_conf(Interface *interf, JsonObject *data){
    EVENT event;
    String act;

    if (data->containsKey(F("event"))) {
        EVENT *curr = nullptr;
        int i = 1, num = (*data)[F("event")];
        while ((curr = myLamp.events.getNextEvent(curr)) && i != num) ++i;
        if (!curr) return;
        myLamp.events.delEvent(*curr);
    }

    if (data->containsKey(F("enabled"))) {
        event.isEnabled = ((*data)[F("enabled")] == true);
    } else {
        event.isEnabled = true;
    }

    event.d1 = ((*data)[F("d1")] == true);
    event.d2 = ((*data)[F("d2")] == true);
    event.d3 = ((*data)[F("d3")] == true);
    event.d4 = ((*data)[F("d4")] == true);
    event.d5 = ((*data)[F("d5")] == true);
    event.d6 = ((*data)[F("d6")] == true);
    event.d7 = ((*data)[F("d7")] == true);
    event.event = (EVENT_TYPE)(*data)[F("evList")].as<long>();
    event.repeat = (*data)[F("repeat")];
    event.stopat = (*data)[F("stopat")];

    tmElements_t tm;
    String tmEvent = (*data)[F("tmEvent")];
    tm.Year = atoi(tmEvent.substring(0,4).c_str()) - 1970;
    tm.Month = atoi(tmEvent.substring(5,7).c_str());
    tm.Day = atoi(tmEvent.substring(8,10).c_str());
    tm.Hour = atoi(tmEvent.substring(11,13).c_str());
    tm.Minute = atoi(tmEvent.substring(14,16).c_str());
    tm.Second = 0;

    event.unixtime = makeTime(tm);;
    String tmpMsg(jee.param(F("msg")));
    event.message = (char*)(tmpMsg.c_str());

    myLamp.events.addEvent(event);
    myLamp.events.saveConfig();
    show_settings_event(interf, data);
}

void show_event_conf(Interface *interf, JsonObject *data){
    EVENT event;
    String act;
    bool edit = false;
    int i = 1, num = 0;
    if (data->containsKey(F("event"))) {
        EVENT *curr = nullptr;
        num = (*data)[F("event")];
        while ((curr = myLamp.events.getNextEvent(curr)) && i != num) ++i;
        if (!curr) return;
        act = (*data)["evconf" + String(num)].as<String>();
        event = *curr;
        edit = true;
    }

    if (act == F("del")) {
        myLamp.events.delEvent(event);
        myLamp.events.saveConfig();
        show_settings_event(interf, data);
        return;
    } else
    if (data->containsKey(F("save"))) {
        set_event_conf(interf, data);
        return;
    }


    interf->json_frame_interface();

    if (edit) {
        interf->json_section_main(F("set_event"), F("Редактировать событие"));
        interf->constant(F("event"), String(num), event.getName());
        interf->checkbox(F("enabled"), (event.isEnabled? F("true") : F("false")), F("Активно"), false);
    } else {
        interf->json_section_main(F("set_event"), F("Добавить событие"));
    }

    interf->select(F("evList"), String(event.event), F("Тип события"), false);
    interf->option(String(EVENT_TYPE::ON), F("Включить лампу"));
    interf->option(String(EVENT_TYPE::OFF), F("Выключить лампу"));
    interf->option(String(EVENT_TYPE::DEMO_ON), F("Включить DEMO"));
    interf->option(String(EVENT_TYPE::ALARM), F("Будильник"));
    interf->option(String(EVENT_TYPE::LAMP_CONFIG_LOAD), F("Загрузка конф. лампы"));
    interf->option(String(EVENT_TYPE::EFF_CONFIG_LOAD), F("Загрузка конф. эффектов"));
    interf->option(String(EVENT_TYPE::EVENTS_CONFIG_LOAD), F("Загрузка конф. событий"));
    interf->option(String(EVENT_TYPE::SEND_TEXT), F("Вывести текст"));
    interf->option(String(EVENT_TYPE::PIN_STATE), F("Состояние пина"));
#ifdef AUX_PIN
    interf->option(String(EVENT_TYPE::AUX_ON), F("Включить AUX"));
    interf->option(String(EVENT_TYPE::AUX_OFF), F("Выключить AUX"));
    interf->option(String(EVENT_TYPE::AUX_TOGGLE), F("Переключить AUX"));
#endif
    interf->json_section_end();

    interf->datetime(F("tmEvent"), event.getDateTime(), F("Дата/время события"));
    interf->number(F("repeat"), event.repeat, F("Повтор, мин"));
    interf->number(F("stopat"), event.stopat, F("Остановить через, мин"));
    interf->text(F("msg"), String(event.message), F("Параметр (текст)"));

    interf->json_section_hidden(F("repeat"), F("Повтор"));
    interf->checkbox(F("d1"), (event.d1? F("true") : F("false")), F("Понедельник"), false);
    interf->checkbox(F("d2"), (event.d1? F("true") : F("false")), F("Вторник"), false);
    interf->checkbox(F("d3"), (event.d1? F("true") : F("false")), F("Среда"), false);
    interf->checkbox(F("d4"), (event.d1? F("true") : F("false")), F("Четверг"), false);
    interf->checkbox(F("d5"), (event.d1? F("true") : F("false")), F("Пятница"), false);
    interf->checkbox(F("d6"), (event.d1? F("true") : F("false")), F("Суббота"), false);
    interf->checkbox(F("d7"), (event.d1? F("true") : F("false")), F("Воскресенье"), false);
    interf->json_section_end();

    if (edit) {
        interf->hidden(F("save"), F("true"));
        interf->button_submit(F("set_event"), F("Обновить событие"));
    } else {
        interf->button_submit(F("set_event"), F("Добавить событие"), F("green"));
    }

    interf->spacer();
    interf->button(F("show_event"), F("Выход"));

    interf->json_section_end();
    interf->json_frame_flush();
}

void section_effects_frame(Interface *interf, JsonObject *data){
    interf->json_frame_interface(F(("Огненная лампа")));
    block_effects_main(interf, data);
    interf->json_frame_flush();
}

void section_lamp_frame(Interface *interf, JsonObject *data){
    interf->json_frame_interface(F(("Огненная лампа")));
    block_lamp(interf, data);
    interf->json_frame_flush();
}

void section_settings_frame(Interface *interf, JsonObject *data){
    interf->json_frame_interface(F(("Огненная лампа")));

    interf->json_section_main(F("settings"), F("Настройки"));

    interf->button(F("show_time"), F("Настройка времени"));

    interf->button(F("show_other"), F("Разные"));

    interf->button(F("show_wifi"), F("WiFi & MQTT"));
#ifdef MIC_EFFECTS
    interf->button(F("show_mic"), F("Микрофон"));
#endif

    interf->button(F("show_event"), F("События"));

    interf->spacer();
    block_settings_update(interf, data);

    interf->json_section_end();
    interf->json_frame_flush();
}

void section_main_frame(Interface *interf, JsonObject *data){ // функция в которой мф формируем веб интерфейс
    if(!jee.connected && !iGLOBAL.isAPMODE){
        // только для первого раза форсируем выбор вкладки настройки WiFi, дальше этого не делаем
        show_settings_wifi(interf, data);
        return;
    }

#ifdef MIC_EFFECTS
    myLamp.setMicAnalyseDivider(0); // отключить микрофон на время прорисовки интерфейса
#endif

    LOG(println, F("Внимание: Создание интерфейса! Такие вызовы должны быть минимизированы."));
    interf->json_frame_interface(F(("Огненная лампа")));

    block_menu(interf, data);
    block_effects_main(interf, data);

    interf->json_frame_flush();

#ifdef MIC_EFFECTS
    myLamp.setMicAnalyseDivider(1); // восстановить делитель, при любой активности (поскольку эффекты могут его перенастраивать под себя)
#endif
}


void create_parameters(){
    LOG(println, F("Создание дефолтных параметров"));
    // создаем дефолтные параметры для нашего проекта
    jee.var_create(F("wifi"), F("STA")); // режим работы WiFi по умолчанию ("STA" или "AP")  (параметр в энергонезависимой памяти)
    jee.var_create(F("ssid"), F("")); // имя точки доступа к которой подключаемся (параметр в энергонезависимой памяти)
    jee.var_create(F("pass"), F("")); // пароль точки доступа к которой подключаемся (параметр в энергонезависимой памяти)

    jee.var_create(F("m_host"), F("")); // Дефолтные настройки для MQTT
    jee.var_create(F("m_port"), F("1883"));
    jee.var_create(F("m_user"), F(""));
    jee.var_create(F("m_pass"), F(""));
    jee.var_create(F("m_pref"), F(""));

    // параметры подключения к MQTT
    jee.var_create(F("mqtt_int"), F("30")); // интервал отправки данных по MQTT в секундах (параметр в энергонезависимой памяти)

    jee.var_create(F("effList"), F("1"));

    jee.var_create(F("fileName"),F("cfg1"));

    jee.var_create(F("MIRR_H"), F("false"));
    jee.var_create(F("MIRR_V"), F("false"));
#ifdef AUX_PIN
    jee.var_create(F("AUX"), F("false"));
#endif
    jee.var_create(F("msg"), F(""));
    jee.var_create(F("txtColor"), F("#ffffff"));
    jee.var_create(F("txtSpeed"), F("100"));
    jee.var_create(F("txtOf"), F("0"));
    jee.var_create(F("perTime"), F("1"));

    jee.var_create(F("GlobBRI"), F("127"));

    jee.var_create(F("time"), F("00:00"));
    jee.var_create(F("timezone"), F(""));
    jee.var_create(F("tm_offs"), F("0"));

    jee.var_create(F("isTmSync"), F("true"));

    jee.var_create(F("isGLBbr"),F("false"));

    jee.var_create(F("ny_period"), F("0"));
    jee.var_create(F("ny_unix"), F("1609459200"));

# ifdef MIC_EFFECTS
    jee.var_create(F("micScale"),F("1.28"));
    jee.var_create(F("micNoise"),F("0.00"));
    jee.var_create(F("micnRdcLvl"),F("0"));
    jee.var_create(F("Mic"),F("true"));
#endif

    jee.var_create(F("isFaderON"),(FADE==true?F("true"):F("false")));

#ifdef ESP_USE_BUTTON
    jee.var_create(F("isBtnOn"), F("true"));
#endif
    //-----------------------------------------------

    jee.section_handle_add(F("main"), section_main_frame);

    jee.section_handle_add(F("effects"), section_effects_frame);
    jee.section_handle_add(F("effects_param"), show_effects_param);
    jee.section_handle_add(F("effList"), set_effects_list);
    jee.section_handle_add(F("bright"), set_effects_param);
    jee.section_handle_add(F("speed"), set_effects_param);
    jee.section_handle_add(F("scale"), set_effects_param);
    jee.section_handle_add(F("extraR"), set_effects_param);

    jee.section_handle_add(F("effects_config"), show_effects_config);
    jee.section_handle_add(F("effListConf"), set_effects_config_list);
    jee.section_handle_add(F("eff_conf_prm"), set_effects_config_param);

    jee.section_handle_add(F("ONflag"), set_onflag);
    jee.section_handle_add(F("Demo"), set_demoflag);
#ifdef OTA
    jee.section_handle_add(F("OTA"), set_otaflag);
#endif
#ifdef AUX_PIN
    jee.section_handle_add(F("AUX"), set_auxflag);
#endif

    jee.section_handle_add(F("lamp"), section_lamp_frame);
    jee.section_handle_add(F("textsend"), set_lamp_textsend);
    jee.section_handle_add(F("add_lamp_config"), add_lamp_config);
    jee.section_handle_add(F("edit_lamp_config"), edit_lamp_config);

    jee.section_handle_add(F("settings"), section_settings_frame);
    jee.section_handle_add(F("show_wifi"), show_settings_wifi);
    jee.section_handle_add(F("set_wifi"), set_settings_wifi);
    jee.section_handle_add(F("set_mqtt"), set_settings_mqtt);
    jee.section_handle_add(F("show_other"), show_settings_other);
    jee.section_handle_add(F("set_other"), set_settings_other);
    jee.section_handle_add(F("show_time"), show_settings_time);
    jee.section_handle_add(F("set_time"), set_settings_time);
#ifdef MIC_EFFECTS
    jee.section_handle_add(F("show_mic"), show_settings_mic);
    jee.section_handle_add(F("set_mic"), set_settings_mic);
    jee.section_handle_add(F("Mic"), set_micflag);
    jee.section_handle_add(F("mic_cal"), set_settings_mic_calib);
#endif

    jee.section_handle_add(F("show_event"), show_settings_event);
    jee.section_handle_add(F("event_conf"), show_event_conf);
    jee.section_handle_add(F("evconf*"), show_event_conf);
    jee.section_handle_add(F("set_event"), set_event_conf);
    // jee.section_handle_add(F("evconf*"), set_event_conf);
    jee.section_handle_add(F("Events"), set_eventflag);

}

void setEffectParams(EFFECT *curEff){
    if (!curEff) {
        // все еще ломается по неведомому закону... попробую обойти так
        LOG(println, F("Обнаружен нулевой указатель!"));
        ESP.restart();
        return;
    }

    LOG(println,F("Обновление через setEffectParams"));

}

void update(){ // функция выполняется после ввода данных в веб интерфейсе. получение параметров из веб интерфейса в переменные
    LOG(println, F("In update..."));

    myLamp.demoTimer(T_RESET);  // при любом изменении UI сбрасываем таймер ДЕМО режима и начинаем отсчет снова

    EFFECT *curEff = myLamp.effects.getCurrent();
/*
    if(curEff->eff_nb!=EFF_NONE){ // для служебного "пустого" эффекта EFF_NONE вообще ничего не делаем
        //LOG(printf_P, PSTR("curEff: %p iGLOBAL.prevEffect: %p\n"), curEff, iGLOBAL.prevEffect);
        if(curEff!= && iGLOBAL.prevEffect!=nullptr){ // Если эффект поменялся или требуется обновление UI, при этом не первый вход в процедуру после перезагрузки

        } else { // эффект не менялся, либо MQTT, либо первый вход - обновляем текущий эффект значениями из UI/MQTT

            myLamp.setLampBrightness(jee.param(F("bright")).toInt());
            if(myLamp.isLampOn()) {
                // только если включена, поскольку этот вызов при перезагрузке зажжет лампу, даже если она выключена в конфиге
                // два вызова выглядят коряво, но встраивать setBrightness в setLampBrightness нельзя,
                // т.к. это корежит фэйдер и отложенную смену эфектов, можно попробовать наоборот сделать setBrightness будет менять яркость в конфиге эффекта
                myLamp.setBrightness(myLamp.getNormalizedLampBrightness(), myLamp.getFaderFlag());
            }


            if(iGLOBAL.prevEffect!=nullptr){
                if(!myLamp.effects.autoSaveConfig()){ // отложенная запись, не чаще чем однократно в 30 секунд
                    myLamp.ConfigSaveSetup(60*1000); //через минуту сработает еще попытка записи и так до успеха
                }
            }

        }
    }
*/
    jee.setDelayedSave(30000); // отложенное сохранение конфига, раз в 30 секунд относительно последнего изменения
#ifdef MIC_EFFECTS
    myLamp.setMicAnalyseDivider(1); // восстановить делитель, при любой активности (поскольку эффекты могут его перенастраивать под себя)
#endif
}

void updateParm() // передача параметров в UI после нажатия сенсорной или мех. кнопки
{
    LOG(println, F("Обновляем параметры после нажатия кнопки..."));
    EFFECT *curEff = myLamp.effects.getCurrent();
    setEffectParams(curEff);

    if(myLamp.getMode() != MODE_DEMO) jee.save(); // Cохранить конфиг
    jee.refresh(); // форсировать перерисовку интерфейсов клиентов
}

void httpCallback(const char *param, const char *value)
{
    EFFECT *curEff = myLamp.effects.getCurrent();

    LOG(printf_P, "HTTP: %s - %s\n", param, value);
    if(!strcmp_P(param,PSTR("on"))){
        myLamp.setOnOff(true);
        jee.var(F("ONflag"), (myLamp.isLampOn()?F("true"):F("false")));
        myLamp.switcheffect(SW_SPECIFIC, myLamp.getFaderFlag(), curEff->eff_nb);
    } else if(!strcmp_P(param,PSTR("off"))){
        myLamp.setOnOff(false);
        jee.var(F("ONflag"), (myLamp.isLampOn()?F("true"):F("false")));
    } else if(!strcmp_P(param,PSTR("demo"))){
        myLamp.startDemoMode();
    } else if(!strcmp_P(param,PSTR("msg"))){
        myLamp.sendStringToLamp(value,CRGB::Green);
    } else if(!strcmp_P(param,PSTR("bright"))){
        if(atoi(value)>0){
            if(myLamp.getMode() == MODE_DEMO || myLamp.IsGlobalBrightness()){
                jee.var(F("GlobBRI"), value);
                //myLamp.setGlobalBrightness(atoi(value));
            } else {
                jee.var(F("bright"), value);
            }
            myLamp.setLampBrightness(atoi(value));
            myLamp.fadelight(myLamp.getNormalizedLampBrightness());
            //myLamp.setLoading(true); // перерисовать эффект
        }
    } else if(!strcmp_P(param,PSTR("speed"))){
        if(atoi(value)>0){
            jee.var(F("speed"), value);
            curEff->speed = atoi(value);
            myLamp.setLoading(true); // перерисовать эффект
        }
    } else if(!strcmp_P(param,PSTR("scale"))){
        if(atoi(value)>0){
            jee.var(F("scale"), value);
            curEff->scale = atoi(value);
            myLamp.setLoading(true); // перерисовать эффект
        }
    } else if(!strcmp_P(param,PSTR("effect"))){
        if(atoi(value)>0){
            jee.var(F("effList"), value);
            myLamp.switcheffect(SW_SPECIFIC, myLamp.getFaderFlag(), (EFF_ENUM)atoi(value));
        }
    } else if(!strcmp_P(param,PSTR("move_next"))){
        myLamp.switcheffect(SW_NEXT);
    } else if(!strcmp_P(param,PSTR("move_prev"))){
        myLamp.switcheffect(SW_PREV);
    } else if(!strcmp_P(param,PSTR("move_rnd"))){
        myLamp.switcheffect(SW_RND);
    } else if(!strcmp_P(param,PSTR("reboot"))){
        ESP.restart(); // так лучше :)
    } else if(!strcmp_P(param,PSTR("OTA"))){
        #ifdef OTA
            myLamp.startOTA();
        #endif
    }
#ifdef AUX_PIN
    else if (!strcmp_P(param, PSTR("aux_on")))
    {
        AUX_toggle(true);
    }
    else if (!strcmp_P(param, PSTR("aux_off")))
    {
        AUX_toggle(false);
    }
    else if (!strcmp_P(param, PSTR("aux_toggle")))
    {
        AUX_toggle(!digitalRead(AUX_PIN));
    }
#endif
    jee.refresh();
}