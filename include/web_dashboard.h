// Conteúdo de web_dashboard.h
#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// Dashboard principal de status (Acessado quando conectado ao Wi-Fi local)
const char web_dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Central de Controle</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #f0f0f0; 
        }
        .container { 
            max-width: 1000px; 
            margin: 0 auto; 
            background: white; 
            padding: 20px; 
            border-radius: 8px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1); 
        }
        h1 { 
            text-align: center; 
            color: #333; 
            margin-bottom: 10px; 
            border-bottom: 2px solid #007bff; 
            padding-bottom: 10px; 
        }
        .menu { 
            text-align: center; 
            margin: 20px 0; 
            padding: 10px; 
            background: #f8f9fa; 
            border-radius: 4px; 
        }
        .menu a { 
            margin: 0 15px; 
            text-decoration: none; 
            color: #007bff; 
            font-weight: bold; 
        }
        .menu a:hover { 
            text-decoration: underline; 
        }
        .section { 
            margin: 30px 0; 
        }
        .section-title { 
            font-size: 18px; 
            font-weight: bold; 
            color: #444; 
            margin-bottom: 15px; 
            padding: 8px 0; 
            border-bottom: 1px solid #ddd; 
        }
        .info-table { 
            width: 100%; 
            border-collapse: collapse; 
            margin-bottom: 20px; 
            background: white; 
        }
        .info-table td { 
            padding: 10px; 
            border: 1px solid #ddd; 
            vertical-align: middle; 
        }
        .info-table td:first-child { 
            font-weight: bold; 
            color: #555; 
            width: 35%; 
            background: #f8f9fa; 
        }
        .status-ok { 
            color: #28a745; 
            font-weight: bold; 
        }
        .status-error { 
            color: #dc3545; 
            font-weight: bold; 
        }
        .status-warning { 
            color: #ffc107; 
            font-weight: bold; 
        }
        .pins-summary { 
            background: #f8f9fa; 
            padding: 15px; 
            border-radius: 4px; 
            border-left: 4px solid #007bff; 
        }
        .pins-list { 
            margin-top: 10px; 
        }
        .pin-item { 
            padding: 8px 12px; 
            margin: 5px 0; 
            background: white; 
            border: 1px solid #ddd; 
            border-radius: 4px; 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
        }
        .pin-name { 
            font-weight: bold; 
            color: #333; 
        }
        .pin-status { 
            font-size: 12px; 
            padding: 3px 8px; 
            border-radius: 3px; 
            color: white; 
        }
        .pin-high { 
            background: #28a745; 
        }
        .pin-low { 
            background: #6c757d; 
        }
        .loading-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(255, 255, 255, 0.9);
            display: flex;
            justify-content: center;
            align-items: center;
            z-index: 9999;
        }
        .loading-spinner {
            border: 4px solid #f3f3f3;
            border-top: 4px solid #007bff;
            border-radius: 50%;
            width: 40px;
            height: 40px;
            animation: spin 1s linear infinite;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
        .loading-text {
            margin-left: 15px;
            font-size: 16px;
            color: #007bff;
        }
    </style>
</head>
<body>
    <div id="loadingOverlay" class="loading-overlay">
        <div class="loading-spinner"></div>
        <div class="loading-text">Carregando status...</div>
    </div>
    <div class="container">
        <h1>SMCR - Central de Controle</h1>
        
        <div class="menu">
            <a href="/">Status</a>
            <a href="/configuracao">Configuracoes Gerais</a>
            <a href="/pins">Pinos/Reles</a>
            <a href="/mqtt">MQTT/Servicos</a>
            <a href="/arquivos">Arquivos</a>
            <a href="/reset">Reset</a>
        </div>

        <div class="section">
            <div class="section-title">Pinos Cadastrados</div>
            <div class="pins-summary">
                <div><strong>Total:</strong> <span id="pins-count">0</span> pinos de <span id="pins-max">-</span></div>
                <div id="pins-list" class="pins-list"></div>
            </div>
        </div>

        <div class="section">
            <div class="section-title">Status do Modulo</div>
            
            <div class="section-title" style="font-size: 16px; margin-top: 20px;">Rede Wi-Fi</div>
            <table class="info-table">
                <tr><td>Status:</td><td id="wifi-status" class="status-warning">Carregando...</td></tr>
                <tr><td>SSID:</td><td id="wifi-ssid">-</td></tr>
                <tr><td>IP Local:</td><td id="wifi-ip">-</td></tr>
                <tr><td>IP do Usuario:</td><td id="user-ip">-</td></tr>
            </table>

            <div class="section-title" style="font-size: 16px;">Sistema</div>
            <table class="info-table">
                <tr><td>Hostname:</td><td id="hostname">-</td></tr>
                <tr><td>Data/Hora Atual:</td><td id="current-time">-</td></tr>
                <tr><td>Tempo de Atividade (Uptime):</td><td id="uptime">-</td></tr>
                <tr><td>Memoria Livre (Heap):</td><td id="free-heap">-</td></tr>
            </table>

            <div class="section-title" style="font-size: 16px;">Sincronizacao</div>
            <table class="info-table">
                <tr><td>NTP Status:</td><td id="ntp-status" class="status-warning">-</td></tr>
                <tr><td>MQTT Status:</td><td class="status-error">Desabilitado</td></tr>
            </table>
        </div>
    </div>

    <!-- Indicador de Performance -->
    <div style="position: fixed; bottom: 10px; right: 10px; background: rgba(0,0,0,0.7); color: white; padding: 8px 12px; border-radius: 6px; font-size: 12px; font-family: monospace; display: none;" id="performance-indicator">
        Tempo de Carregamento: <span id="load-time">-</span>
    </div>

    <script>
        // === SISTEMA DE MONITORAMENTO DE PERFORMANCE ===
        const pageStartTime = performance.now();
        
        // Função para mostrar indicador de performance
        function showPerformanceIndicator() {
            const loadTime = performance.now() - pageStartTime;
            document.getElementById('load-time').textContent = loadTime.toFixed(2) + 'ms';
            document.getElementById('performance-indicator').style.display = 'block';
            
            console.log(`[PERFORMANCE] Dashboard carregado em ${loadTime.toFixed(2)}ms`);
        }
        
        // Controle do loading
        function showLoading(text = 'Carregando...') {
            document.getElementById('loadingOverlay').style.display = 'flex';
            document.querySelector('.loading-text').textContent = text;
        }
        
        function hideLoading() {
            document.getElementById('loadingOverlay').style.display = 'none';
        }
        
        // Esconder loading quando página carregar completamente
        window.addEventListener('load', function() {
            hideLoading();
            // Mostrar indicador de performance após carregamento completo
            setTimeout(showPerformanceIndicator, 100);
        });

        function formatBytes(bytes) {
            if (bytes === 0) return '0 B';
            const k = 1024;
            const sizes = ['B', 'KB', 'MB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return parseFloat((bytes / Math.pow(k, i)).toFixed(2)) + ' ' + sizes[i];
        }

        function updateStatus() {
            fetch('/status/json')
                .then(response => response.json())
                .then(data => {
                    // Rede Wi-Fi
                    const wifiStatus = document.getElementById('wifi-status');
                    if (data.wifi && data.wifi.status === 'Conectado') {
                        wifiStatus.textContent = data.wifi.status;
                        wifiStatus.className = 'status-ok';
                        document.getElementById('wifi-ssid').textContent = data.wifi.ssid || '-';
                        document.getElementById('wifi-ip').textContent = data.wifi.local_ip || '-';
                    } else {
                        wifiStatus.textContent = 'Desconectado';
                        wifiStatus.className = 'status-error';
                    }
                    
                    document.getElementById('user-ip').textContent = data.system.client_ip || '-';

                    // Sistema
                    document.getElementById('hostname').textContent = data.system.hostname || '-';
                    document.getElementById('current-time').textContent = data.system.current_time || '-';
                    document.getElementById('uptime').textContent = data.system.uptime || '-';
                    document.getElementById('free-heap').textContent = formatBytes(data.system.free_heap || 0);

                    // Sincronizacao
                    const ntpStatus = document.getElementById('ntp-status');
                    if (data.sync && data.sync.ntp_status === 'Sincronizado') {
                        ntpStatus.textContent = data.sync.ntp_status;
                        ntpStatus.className = 'status-ok';
                    } else {
                        ntpStatus.textContent = data.sync ? data.sync.ntp_status : 'Aguardando';
                        ntpStatus.className = 'status-warning';
                    }

                    // Pinos
                    const pinsCount = data.pins ? data.pins.length : 0;
                    const pinsMax = data.system ? data.system.max_pins || 16 : 16;
                    const showPinStatus = data.system ? data.system.show_pin_status : true;
                    const corAlerta = data.system ? data.system.cor_com_alerta || '#dc3545' : '#dc3545';
                    const corOk = data.system ? data.system.cor_sem_alerta || '#28a745' : '#28a745';
                    
                    document.getElementById('pins-count').textContent = pinsCount;
                    document.getElementById('pins-max').textContent = pinsMax;
                    
                    const pinsList = document.getElementById('pins-list');
                    
                    // Verificar se deve mostrar status dos pinos
                    if (!showPinStatus) {
                        pinsList.innerHTML = '<p style="text-align: center; color: #ff9800; margin: 10px 0; font-style: italic;">📊 Mostrar Status Desativado</p>';
                    } else if (pinsCount > 0) {
                        pinsList.innerHTML = '';
                        data.pins.forEach(pin => {
                            const pinDiv = document.createElement('div');
                            pinDiv.className = 'pin-item';
                            
                            // Determinar tipo de pino (Entrada/Saída)
                            let tipoStr = 'Desconhecido';
                            const modo = pin.modo || 0;
                            const tipo = pin.tipo || 0;
                            
                            // Constantes de modo (devem corresponder ao pin_manager.cpp)
                            const PIN_MODE_INPUT = 1;
                            const PIN_MODE_OUTPUT = 3;
                            const PIN_MODE_INPUT_PULLUP = 5;
                            const PIN_MODE_INPUT_PULLDOWN = 9;
                            const PIN_MODE_OUTPUT_OPEN_DRAIN = 12;
                            const PIN_TYPE_ANALOG = 192;
                            const PIN_TYPE_REMOTE = 65534;
                            
                            if (tipo === PIN_TYPE_REMOTE) {
                                tipoStr = 'Remoto';
                            } else if (tipo === PIN_TYPE_ANALOG) {
                                tipoStr = 'Analógico';
                            } else if (modo === PIN_MODE_OUTPUT || modo === PIN_MODE_OUTPUT_OPEN_DRAIN) {
                                tipoStr = 'Saída';
                            } else if (modo === PIN_MODE_INPUT || modo === PIN_MODE_INPUT_PULLUP || modo === PIN_MODE_INPUT_PULLDOWN) {
                                tipoStr = 'Entrada';
                            }
                            
                            // Valor atual do pino
                            const valorAtual = pin.status_atual !== undefined ? pin.status_atual : 0;
                            
                            // Determinar se está em alerta ou ok baseado no nível de acionamento
                            let isAlerta = false;
                            if (tipo === 192) {
                                // Pino analógico: verifica se está dentro do range configurado
                                const nivelMin = pin.nivel_acionamento_min || 0;
                                const nivelMax = pin.nivel_acionamento_max || 4095;
                                isAlerta = (valorAtual >= nivelMin && valorAtual <= nivelMax);
                            } else {
                                // Pino digital: verifica se corresponde ao nível configurado
                                const nivelAcionamento = pin.nivel_acionamento_min || 0;
                                isAlerta = (valorAtual === nivelAcionamento);
                            }
                            
                            const bgColor = isAlerta ? corAlerta : corOk;
                            
                            pinDiv.innerHTML = `
                                <div style="display: flex; justify-content: space-between; align-items: center; width: 100%;">
                                    <div>
                                        <span class="pin-name">GPIO ${pin.gpio} - ${pin.description || 'Pino'}</span>
                                        <span style="font-size: 11px; color: #666; margin-left: 10px;">(${tipoStr})</span>
                                    </div>
                                    <span class="pin-status" style="background-color: ${bgColor};">${valorAtual}</span>
                                </div>
                            `;
                            pinsList.appendChild(pinDiv);
                        });
                    } else {
                        pinsList.innerHTML = '<p style="text-align: center; color: #666; margin: 10px 0;">Nenhum pino cadastrado</p>';
                    }
                })
                .catch(error => {
                    console.error('Erro ao carregar status:', error);
                });
        }

        // Atualizar status a cada 15 segundos
        updateStatus();
        setInterval(updateStatus, 15000);
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_DASHBOARD_H