# ESP32-RealTime-GPS-Tracker

🛰️ ESP32 Real-Time GPS Tracker
Este projeto demonstra a criação de um dispositivo de rastreamento robusto utilizando o ESP32 como microcontrolador principal. O objetivo é capturar dados de geolocalização (latitude e longitude) de um módulo GPS e transmiti-los em tempo real para um frontend interativo através de uma conexão Wi-Fi.

O sistema é construído sobre um Servidor Web leve, hospedado diretamente no ESP32, que serve uma página HTML com recursos de visualização e controle.

✨ Tecnologias e Componentes Principais
Microcontrolador: ESP32 (Gerenciamento da lógica, Wi-Fi, Servidor Web).
2 PPROTOBARDs 
GPS: Módulo GPS GY-NEO6MV2 para aquisição das coordenadas.
Jumpers para fazer a ligação do módulo GPS ao Esp-32

Linguagens de Programação: C++ (Código do ESP32 / Arduino Framework), HTML, CSS, e JavaScript (Interface Web).

Protocolo: Wi-Fi (para comunicação entre ESP32 e o dispositivo de visualização).

Visualização: Utilização de bibliotecas de mapa Leaflet.js no frontend para plotar a localização.

🗺️ Funcionalidades Implementadas
Rastreamento GPS em Tempo Real:

Leitura contínua das coordenadas GPS e atualização na interface web.

Exibição da localização atual em um mapa interativo.

Servidor Web Embarcado:

Hospedagem da interface do usuário (UI) diretamente no ESP32.

Comunicação bidirecional para receber comandos e enviar dados de telemetria.

Controle Remoto de Periféricos:

Opção na interface web para Ligar/Desligar um LED (atuando como um atuador de exemplo).

Sistema de Log de Coordenadas:

Registro e exibição de um log das coordenadas mais recentes.

Definição de Ponto de Destino e Alarme (Geofence Simples):

Capacidade de definir um ponto de destino alvo.

Implementação de uma lógica simples de alarme visual que é acionada quando o você não chega no ponto em determinado tempo.

⚙️ Como Utilizar
Conectar o módulo GPS e o LED aos pinos definidos no código C++.

Configurar as credenciais do Wi-Fi no arquivo de código.

Fazer o upload do código para o ESP32.

Acessar o endereço IP do ESP32 pelo navegador para visualizar a interface web.
