// Constantes de Mapa e Marcadores
var destMarker; 
var geofenceCircle; 
var trackPolyline;
var map;
var currentMarker;

// Configuracao inicial do mapa (Valores Iniciais)
var initialLat = 0.000000;
var initialLon = 0.000000;
var initialRadius = 0.25; // Raio de 250m
var initialDestLat = -25.420000; 
var initialDestLon = -49.270000;

document.addEventListener('DOMContentLoaded', (event) => {
    // Inicializa o mapa apos o DOM carregar
    map = L.map('mapid').setView([initialLat, initialLon], 15);
    L.tileLayer('https://tile.openstreetmap.org/{z}/{x}/{y}.png',{maxZoom: 19, attribution:'© OpenStreetMap'}).addTo(map); 
    
    // Marcador atual
    currentMarker = L.marker([initialLat, initialLon]).addTo(map).bindPopup('Localizacao Atual').openPopup();

    // Inicializa marcadores de Destino e Geofence
    initializeMapMarkers(initialDestLat, initialDestLon, initialRadius);

    // Inicia a primeira chamada de dados
    updateData();
    // Configura o refresh
    setInterval(updateData, 5000); 
});

function initializeMapMarkers(lat, lon, radius) {
    var destLatLon = [lat, lon];

    // Marcador de Destino (icone vermelho)
    destMarker = L.marker(destLatLon, {
        icon: L.icon({
            iconUrl: 'https://raw.githubusercontent.com/pointhi/leaflet-color-markers/master/img/marker-icon-2x-red.png', 
            shadowUrl: 'https://cdnjs.cloudflare.com/ajax/libs/leaflet/0.7.7/images/marker-shadow.png', 
            iconSize: [25, 41], 
            iconAnchor: [12, 41], 
            popupAnchor: [1, -34], 
            shadowSize: [41, 41]
        })
    }).addTo(map).bindPopup('DESTINO');
    
    // Circulo Geofence (raio em metros)
    geofenceCircle = L.circle(destLatLon, {
        color: 'red', 
        fillColor: '#f03', 
        fillOpacity: 0.2, 
        radius: radius * 1000
    }).addTo(map); 
    
    // Polilinha de Rastreamento
    trackPolyline = L.polyline([], {
        color: '#8AB4F8', 
        weight: 4
    }).addTo(map); 
}

function updateData() {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            var data = JSON.parse(this.responseText);
            
            // 1. Atualiza dados de status
            document.getElementById('gps-lat').innerHTML = data.latitude;
            document.getElementById('gps-lon').innerHTML = data.longitude;
            document.getElementById('gps-status').innerHTML = '<b>Status:</b> ' + data.status;
            
            // 2. Atualiza dados do Alarme/Chegada
            var alertBox = document.querySelector('.alert-box');
            document.getElementById('alert-message').innerHTML = data.alert_message;
            
            // Atualiza classe do Alert Box
            var alertClass = 'alert-ok';
            if (data.alert_message.startsWith('⚠️ ALARME!')) {
                alertClass = 'alert-danger';
            } else if (data.alert_message.startsWith('A caminho')) {
                alertClass = 'alert-warning';
            }
            alertBox.className = 'alert-box ' + alertClass;

            // 3. Atualiza Display de Monitoramento
            document.getElementById('geofence-radius-display').innerHTML = data.geofence_radius;
            
            // 4. Atualiza LED
            var ledSwitch = document.getElementById('ledSwitch');
            var ledStatusText = document.getElementsByClassName('led-status')[0];
            ledSwitch.checked = data.led_on;
            ledStatusText.innerHTML = data.led_on ? 'LIGADO' : 'DESLIGADO';
            ledStatusText.style.color = data.led_on ? "#4CAF50" : "#F44336";

            // 5. Atualiza o Mapa (Current Location)
            var newLatLon = [data.latitude, data.longitude];
            currentMarker.setLatLng(newLatLon);
            map.setView(newLatLon, 15);

            // 6. Atualiza Polilinha (Historico de Coordenadas)
            trackPolyline.setLatLngs(data.log_coords);
            
            // 7. Atualiza Marcadores de Destino (apenas se for necessario mudar)
            var destLatLon = [data.dest_lat, data.dest_lon];
            destMarker.setLatLng(destLatLon);
            geofenceCircle.setLatLng(destLatLon).setRadius(data.geofence_radius * 1000); // Raio em metros

            // 8. Injeta o HTML do Log
            document.getElementById('log-history').innerHTML = data.log_html;
        }
    };
    xhttp.open('GET', '/data', true); 
    xhttp.send();
}