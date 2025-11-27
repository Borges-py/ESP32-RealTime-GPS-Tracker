#include <WiFi.h>
#include <WebServer.h>
#include <TinyGPSPlus.h>
#include <HardwareSerial.h>

const char* ssid = "nome_do_wifi";
const char* password = "senha_do_wifi";

const int ledPin = 2;
WebServer server(80);

#define RXD2 16
#define TXD2 17
#define GPS_BAUD 9600
HardwareSerial gpsSerial(2);
TinyGPSPlus gps;

const double DEST_LAT = -25.420000; 
const double DEST_LON = -49.270000;
const float GEOFENCE_RADIUS = 0.25; 
const int ALARM_HOUR = 2; 
const int ALARM_MINUTE = 38;

float currentLatitude = DEST_LAT; 
float currentLongitude = DEST_LON; 
String currentStatus = "Aguardando GPS...";


String alertMessage = ""; 
bool destinationReached = false; 


#define MAX_LOG_ENTRIES 10 

struct GpsLogEntry {
    double lat;
    double lon;
    char timeStr[15]; 
};

GpsLogEntry logHistory[MAX_LOG_ENTRIES];
int logIndex = 0; 
bool logFull = false; 

void addLogEntry() {
    if (gps.location.isValid() && gps.time.isValid()) {
        
        char timeBuffer[15];
        snprintf(timeBuffer, sizeof(timeBuffer), "%02d:%02d:%02d", 
                gps.time.hour(), gps.time.minute(), gps.time.second());

        logHistory[logIndex].lat = gps.location.lat();
        logHistory[logIndex].lon = gps.location.lng();
        strncpy(logHistory[logIndex].timeStr, timeBuffer, sizeof(logHistory[logIndex].timeStr) - 1);
        
        logHistory[logIndex].timeStr[sizeof(logHistory[logIndex].timeStr) - 1] = '\0';
                                                
        logIndex++;
        if (logIndex >= MAX_LOG_ENTRIES) {
            logIndex = 0; 
            logFull = true;
        }
    }
}

String generateLogHistoryHTML_New() {
    String html = "<ul>";
    
    int count = logFull ? MAX_LOG_ENTRIES : logIndex;
    
    for (int i = 0; i < count; i++) {
        int current = (logIndex - 1 - i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;

        html += "<li>";
        
        html += "<span class='log-time'>";
        html += logHistory[current].timeStr;
        html += "</span>";
        
        html += "<span class='log-coords'>";
        html += "Lat: " + String(logHistory[current].lat, 6);
        html += ", Lon: " + String(logHistory[current].lon, 6);
        html += "</span>";
        
        html += "</li>";
    }
    html += "</ul>";
    
    if (count == 0) {
        return "<p style='color: #BBBBBB; padding: 10px; border: 1px dashed #444; border-radius: 4px;'>Aguardando primeira localizacao valida do GPS...</p>";
    }
    
    return html;
}

void checkAlertStatus() {
    if (!gps.location.isValid() || !gps.time.isValid()) {
        alertMessage = "Aguardando localizacao e hora do GPS para verificar o alarme.";
        return; 
    }

    double distanceKm = 
        gps.distanceBetween(
            gps.location.lat(), gps.location.lng(), 
            DEST_LAT, DEST_LON
        ) / 1000.0; 

    if (distanceKm <= GEOFENCE_RADIUS) {
        destinationReached = true;
        alertMessage = "CHEGOU NO DESTINO! Distancia: " + String(distanceKm, 2) + " km.";
        return;
    } 
    
    if (destinationReached) {
        alertMessage = "CHEGOU NO DESTINO (Dist. Max: " + String(GEOFENCE_RADIUS, 1) + "km). Alerta desativado.";
        return;
    }

    int currentHour = gps.time.hour();
    int currentMinute = gps.time.minute();

    if (currentHour > ALARM_HOUR || (currentHour == ALARM_HOUR && currentMinute >= ALARM_MINUTE)) {
        
        char alarmTimeStr[6]; 
        snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", ALARM_HOUR, ALARM_MINUTE);

        alertMessage = "⚠️ ALARME! NAO CHEGOU no destino ate as " + String(alarmTimeStr) + ". Distancia: " + String(distanceKm, 2) + " km.";
        
    } else {
        alertMessage = "A caminho. Ainda no prazo. Distancia: " + String(distanceKm, 2) + " km.";
    }
}

void handleData() {
    bool isLedOn = digitalRead(ledPin);

    String logCoordsJson = "[";
    int count = logFull ? MAX_LOG_ENTRIES : logIndex;

    for (int i = 0; i < count; i++) {
        int current = (logIndex - count + i + MAX_LOG_ENTRIES) % MAX_LOG_ENTRIES;

        logCoordsJson += "[";
        logCoordsJson += String(logHistory[current].lat, 6);
        logCoordsJson += ",";
        logCoordsJson += String(logHistory[current].lon, 6);
        logCoordsJson += "]";

        if (i < count - 1) {
            logCoordsJson += ",";
        }
    }
    logCoordsJson += "]";

    String json = "{";
    json += "\"latitude\":" + String(currentLatitude, 6) + ",";
    json += "\"longitude\":" + String(currentLongitude, 6) + ",";
    json += "\"status\":\"" + currentStatus + "\",";
    json += "\"alert_message\":\"" + alertMessage + "\",";
    json += "\"led_on\":" + String(isLedOn ? "true" : "false") + ",";
    
    json += "\"dest_lat\":" + String(DEST_LAT, 6) + ",";
    json += "\"dest_lon\":" + String(DEST_LON, 6) + ",";
    json += "\"geofence_radius\":" + String(GEOFENCE_RADIUS, 2) + ",";
    
    json += "\"log_coords\":" + logCoordsJson;
    
    json += "}";

    server.send(200, "application/json", json);
}

void handleRoot() {
    bool isLedOn = digitalRead(ledPin);
    
    String html = "<!DOCTYPE html><html><head><title>PONTO A PONTO</title>";
    
    html += "<meta charset='UTF-8'>"; 
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<link rel='stylesheet' href='https://unpkg.com/leaflet@1.9.4/dist/leaflet.css'/>";
    
    html += "<link href='https://fonts.googleapis.com/css2?family=Montserrat:wght@400;600;700&display=swap' rel='stylesheet'>";
    
    html += "<style>";
    
    html += "body { font-family: 'Montserrat', sans-serif; background-color: #121212; color: #E0E0E0; margin: 0; padding: 0; display: flex; justify-content: center; align-items: flex-start; min-height: 100vh; }";
    html += ".container { width: 95%; max-width: 900px; margin: 20px auto; background: #1E1E1E; padding: 30px; border-radius: 12px; box-shadow: 0 5px 15px rgba(0,0,0,0.5); }";
    
    html += "h1, h2 { font-weight: 700; color: #FFFFFF; border-bottom: 2px solid #333333; padding-bottom: 10px; margin-top: 0; text-transform: uppercase; }";
    html += "p { font-weight: 400; }";
    html += "b { font-weight: 600; color: #FFFFFF; }";

    html += ".switch-container { display: flex; align-items: center; margin: 20px 0; padding: 15px; background-color: #282828; border-radius: 8px; }";
    html += ".switch-label { margin-right: 15px; font-size: 1.1em; }";
    html += ".switch { position: relative; display: inline-block; width: 60px; height: 34px; }";
    html += ".switch input { opacity: 0; width: 0; height: 0; }";
    html += ".slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: #ccc; -webkit-transition: .4s; transition: .4s; border-radius: 34px; }";
    html += ".slider:before { position: absolute; content: ''; height: 26px; width: 26px; left: 4px; bottom: 4px; background-color: white; -webkit-transition: .4s; transition: .4s; border-radius: 50%; }";
    html += "input:checked + .slider { background-color: #4CAF50; }";
    html += "input:focus + .slider { box-shadow: 0 0 1px #4CAF50; }";
    html += "input:checked + .slider:before { -webkit-transform: translateX(26px); -ms-transform: translateX(26px); transform: translateX(26px); }";
    
    html += ".led-status { margin-left: 15px; font-weight: 600; color: " + String(isLedOn ? "#4CAF50" : "#F44336") + "; }";
    
    html += ".alert-box { padding: 15px; border-radius: 8px; margin-bottom: 20px; font-weight: 700; text-align: center; font-size: 1.2em; }";
    html += ".alert-ok { background-color: #1A531A; border: 2px solid #4CAF50; color: #E8F5E9; }"; 
    html += ".alert-warning { background-color: #5C4000; border: 2px solid #FFC107; color: #FFF8E1; }"; 
    html += ".alert-danger { background-color: #7B0000; border: 2px solid #F44336; color: #FFEBEE; }"; 

    html += "#mapid { height: 400px; width: 100%; border-radius: 8px; margin-top: 15px; border: 1px solid #333333; }";
    html += ".status-box { padding: 10px; background-color: #282828; border-left: 5px solid #8AB4F8; margin-bottom: 20px; text-align: left; }";
    html += ".status-box p { margin: 5px 0; font-size: 1.1em; }";
    
    html += "#log-container { margin-top: 30px; padding-top: 20px; border-top: 1px solid #333333; text-align: left; }";
    html += "#log-history { max-height: 300px; overflow-y: auto; padding-right: 15px; }"; 
    html += "#log-history ul { list-style-type: none; padding: 0; margin: 0; }";
    html += "#log-history li { margin-bottom: 8px; padding: 10px 15px; border-radius: 6px; background-color: #242424; box-shadow: 0 1px 3px rgba(0,0,0,0.2); transition: background-color 0.2s; border-left: 4px solid #8AB4F8; display: flex; justify-content: space-between; align-items: center; }";
    html += "#log-history li:hover { background-color: #333333; }";
    html += ".log-time { font-weight: 700; color: #FFFFFF; min-width: 90px; }";
    html += ".log-coords { font-family: 'Montserrat', monospace; font-size: 0.9em; color: #FFFFFF; }";
    
    html += "p[style*='color: #888'] { color: #BBBBBB !important; border-color: #444 !important; }";

    html += "</style></head><body><div class='container'>";
    
    
    html += "<h1>PONTO A PONTO</h1>"; 
    
    html += "<h2>STATUS DE CHEGADA</h2>"; 

    String alertClass = "alert-ok";
    if (alertMessage.startsWith("⚠️ ALARME!")) {
        alertClass = "alert-danger"; 
    } else if (alertMessage.startsWith("A caminho")) {
        alertClass = "alert-warning"; 
    }

    html += "<div class='alert-box " + alertClass + "'>";
    html += "<p id='alert-message'>" + alertMessage + "</p>";
    html += "</div>";

    char alarmTimeStr[6];
    snprintf(alarmTimeStr, sizeof(alarmTimeStr), "%02d:%02d", ALARM_HOUR, ALARM_MINUTE);

    html += "<h2>MONITORAMENTO</h2>"; 
    html += "<div class='status-box' style='border-left: 5px solid #FFC107;'>"; 
    html += "<p><b>Prazo Limite (UTC):</b> <span style='font-size: 1.1em; font-weight: 700;' id='alarm-time-display'>" + String(alarmTimeStr) + "</span></p>";
    html += "<p><b>Raio de Chegada:</b> " + String(GEOFENCE_RADIUS, 1) + " km</p>";
    html += "</div>";
    
    html += "<h2>CONTROLE DO LED</h2>"; 
    html += "<div class='switch-container'>";
    html += "<span class='switch-label'>LED Status:</span>";
    html += "<label class='switch'>";
    html += "<input type='checkbox' id='ledSwitch' " + String(isLedOn ? "checked" : "") + " onchange=\"window.location.href = this.checked ? '/ligar' : '/desligar'\">";
    html += "<span class='slider'></span>";
    html += "</label>";
    html += "<span class='led-status'>" + String(isLedOn ? "LIGADO" : "DESLIGADO") + "</span>";
    html += "</div>";

    html += "<h2>LOCALIZACAO ATUAL</h2>"; 
    html += "<div class='status-box'>";
    html += "<p id='gps-status'><b>Status:</b> " + currentStatus + "</p>";
    html += "<p><b>Latitude:</b> <span id='gps-lat'>" + String(currentLatitude, 6) + "</span></p>";
    html += "<p><b>Longitude:</b> <span id='gps-lon'>" + String(currentLongitude, 6) + "</span></p>";
    html += "</div>";
    html += "<div id='mapid'></div>";

    html += "<div id='log-container'>";
    html += "<h2>HISTORICO DE COORDENADAS (ULTIMAS 10)</h2>"; 
    
    html += generateLogHistoryHTML_New(); 
    
    html += "</div>";
    
    html += "<script src='https://unpkg.com/leaflet@1.9.4/dist/leaflet.js'></script>";
    html += "<script>";
    
    html += "var destMarker;"; 
    html += "var geofenceCircle;"; 
    html += "var trackPolyline;";

    html += "var map = L.map('mapid').setView([" + String(currentLatitude, 6) + "," + String(currentLongitude, 6) + "], 15);";
    html += "L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom: 19, attribution:'© OpenStreetMap'}).addTo(map);"; 
    html += "var marker = L.marker([" + String(currentLatitude, 6) + "," + String(currentLongitude, 6) + "]).addTo(map).bindPopup('Localizacao Atual').openPopup();";

    html += "function initializeMapMarkers(lat, lon, radius) {";
    html += " 	var destLatLon = [lat, lon];";

    html += " 	destMarker = L.marker(destLatLon, {icon: L.icon({iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-red.png', shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png', iconSize: [25, 41], iconAnchor: [12, 41], popupAnchor: [1, -34], shadowSize: [41, 41]})}).addTo(map).bindPopup('DESTINO');";
    
    html += " 	geofenceCircle = L.circle(destLatLon, {color: 'red', fillColor: '#f03', fillOpacity: 0.2, radius: radius * 1000}).addTo(map);"; 
    html += "}";
    html += " 	trackPolyline = L.polyline([], {color: '#8AB4F8', weight: 4}).addTo(map);"; 	

    html += "initializeMapMarkers(" + String(DEST_LAT, 6) + ", " + String(DEST_LON, 6) + ", " + String(GEOFENCE_RADIUS, 2) + ");";

    html += "function updateData() {";
    html += " 	var xhttp = new XMLHttpRequest();";
    html += " 	xhttp.onreadystatechange = function() {";
    html += " 	 	if (this.readyState == 4 && this.status == 200) {";
    html += " 	 	 	var data = JSON.parse(this.responseText);";
    
    html += " 	 	 	document.getElementById('gps-lat').innerHTML = data.latitude;";
    html += " 	 	 	document.getElementById('gps-lon').innerHTML = data.longitude;";
    html += " 	 	 	document.getElementById('gps-status').innerHTML = '<b>Status:</b> ' + data.status;";
    
    html += " 	 	 	var alertBox = document.querySelector('.alert-box');";
    html += " 	 	 	document.getElementById('alert-message').innerHTML = data.alert_message;";
    html += " 	 	 	alertBox.className = 'alert-box ' + (data.alert_message.startsWith('⚠️ ALARME!') ? 'alert-danger' : (data.alert_message.startsWith('A caminho') ? 'alert-warning' : 'alert-ok'));";

    html += " 	 	 	var newLatLon = [data.latitude, data.longitude];";
    html += " 	 	 	marker.setLatLng(newLatLon);";
    html += " 	 	 	map.setView(newLatLon, 15);";

    html += " 	 	 	trackPolyline.setLatLngs(data.log_coords);";

    html += " 	 	 	var destLatLon = [data.dest_lat, data.dest_lon];";
    html += " 	 	 	destMarker.setLatLng(destLatLon);";
    html += " 	 	 	geofenceCircle.setLatLng(destLatLon).setRadius(data.geofence_radius * 1000);";
    
    html += " 	 	 	var ledSwitch = document.getElementById('ledSwitch');";
    html += " 	 	 	var ledStatusText = document.getElementsByClassName('led-status')[0];";
    html += " 	 	 	ledSwitch.checked = data.led_on;";
    html += " 	 	 	ledStatusText.innerHTML = data.led_on ? 'LIGADO' : 'DESLIGADO';";
    
    html += " 	 	}";
    html += " 	};";
    html += " 	xhttp.open('GET', '/data', true);"; 
    html += " 	xhttp.send();";
    html += "}";
    
    html += "setInterval(updateData, 5000);"; 

    html += "</script></div></body></html>";

    server.send(200, "text/html", html);
}

void handleLigado() {
    digitalWrite(ledPin, HIGH);
    server.sendHeader("Location", "/"); 
    server.send(303);
}

void handleDesligado() {
    digitalWrite(ledPin, LOW);
    server.sendHeader("Location", "/"); 
    server.send(303);
}

void setup() {
    delay(2000); 

    Serial.begin(115200);
    

    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);

    gpsSerial.begin(GPS_BAUD, SERIAL_8N1, RXD2, TXD2);
    

    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        
    }
    
    
    server.on("/", handleRoot);
    server.on("/ligar", handleLigado);
    server.on("/desligar", handleDesligado);
    server.on("/data", handleData); 
    server.begin();
    
}

void loop() {
    server.handleClient(); 

    while (gpsSerial.available() > 0) {
        char gpsByte = gpsSerial.read();
        
        if (gps.encode(gpsByte)) {
            if (gps.location.isValid()) {
                currentLatitude = gps.location.lat();
                currentLongitude = gps.location.lng();
                currentStatus = "Localizacao Ok! (Hora: " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second()) + ")";
                
                addLogEntry();
                
                checkAlertStatus(); 
                
            } else {
                currentStatus = "Aguardando 'fix' (sinal de satelite).";
                checkAlertStatus(); 
            }
        }
    }
}