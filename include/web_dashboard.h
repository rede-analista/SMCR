// Conteúdo de web_dashboard.h
#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// Dashboard principal de status (Acessado quando conectado ao Wi-Fi local)
const char web_dashboard_html[] PROGMEM = R"raw_string(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Central de Controle</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background-color: #f3f4f6;
        }
        .card {
            background-color: white;
            padding: 1.5rem;
            border-radius: 0.75rem;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -2px rgba(0, 0, 0, 0.1);
            transition: transform 0.2s;
        }
        .card:hover { transform: translateY(-3px); }
    </style>
</head>
<body class="min-h-screen">

    <!-- Menu Superior (Navbar) -->
    <header class="bg-white shadow-md">
        <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4 flex justify-between items-center">
            <h1 class="text-xl font-bold text-gray-800">SMCR - Central de Controle</h1>
            <nav class="space-x-4 text-sm font-medium">
                <a href="/" class="text-blue-600 hover:text-blue-800">Status</a>
                <a href="/configuracao" class="text-gray-600 hover:text-blue-600">Configurações</a>
                <a href="/pinos" class="text-gray-600 hover:text-blue-600">Pinos/Relés</a>
                <a href="/mqtt" class="text-gray-600 hover:text-blue-600">MQTT/Serviços</a>
                <a href="/reset" class="text-red-600 hover:text-red-800">Reset</a>
            </nav>
        </div>
    </header>

    <!-- Conteúdo Principal -->
    <main class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8 w-full">
        <h2 class="text-2xl font-semibold text-gray-700 mb-6">Status do Módulo</h2>

        <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mb-8">
            
            <!-- Card 1: Rede Wi-Fi -->
            <div id="wifi-card" class="card border-t-4 border-green-500">
                <h3 class="font-semibold text-xl text-gray-800 mb-3">Rede Wi-Fi</h3>
                <ul class="text-gray-600 space-y-1 text-sm">
                    <li>Status: <span id="wifi_status" class="font-medium text-green-500">Carregando...</span></li>
                    <li>SSID: <span id="wifi_ssid">Carregando...</span></li>
                    <li>IP Local: <span id="wifi_ip_local" class="font-mono">Carregando...</span></li>
                    <li>IP do Usuário: <span id="wifi_ip_client" class="font-mono">Carregando...</span></li>
                </ul>
            </div>

            <!-- Card 2: Sistema (AGORA COM DATA/HORA) -->
            <div id="system-card" class="card border-t-4 border-blue-500">
                <h3 class="font-semibold text-xl text-gray-800 mb-3">Sistema</h3>
                <ul class="text-gray-600 space-y-1 text-sm">
                    <li>Hostname: <span id="sys_hostname" class="font-medium">Carregando...</span></li>
                    <!-- NOVO CAMPO: Data/Hora -->
                    <li>Data/Hora Atual: <span id="sys_current_time" class="font-medium">Carregando...</span></li>
                    <li>Tempo de Atividade (Uptime): <span id="sys_uptime" class="font-medium">Carregando...</span></li>
                    <li>Memória Livre (Heap): <span id="sys_heap" class="font-mono">Carregando...</span></li>
                </ul>
            </div>

            <!-- Card 3: Sincronização (FOCADO APENAS EM STATUS DE SERVIÇOS) -->
            <div id="sync-card" class="card border-t-4 border-yellow-500">
                <h3 class="font-semibold text-xl text-gray-800 mb-3">Sincronização</h3>
                <ul class="text-gray-600 space-y-1 text-sm">
                    <li>NTP Status: <span id="sync_ntp" class="font-medium">Carregando...</span></li>
                    <li>MQTT Status: <span id="sync_mqtt" class="font-medium">Desabilitado</span></li>
                    <!-- Última atualização movida para o Card Sistema (Data/Hora) -->
                </ul>
            </div>
        </div>

        <!-- Informações de Pinos -->
        <div class="card border-t-4 border-gray-400 mt-8">
            <h3 class="text-lg font-semibold text-gray-700 mb-2">Informações de Pinos (Ainda Não Implementadas)</h3>
            <p class="text-gray-500 text-sm">Esta seção será preenchida com o status em tempo real dos seus relés e sensores.</p>
        </div>
    </main>

    <!-- Rodapé (Footer) -->
    <footer class="text-center text-xs text-gray-500 py-4 mt-8">
        &copy; 2025 SMCR - Sistema de Controle e Monitoramento Remoto
    </footer>

    <!-- Script de Atualização -->
    <script>
        // Função utilitária para formatar bytes em KB, MB, etc.
        function formatBytes(bytes, decimals = 1) {
            if (bytes === 0) return '0 Bytes';
            if (bytes === 1) return '1 Byte';

            const k = 1024;
            const dm = decimals < 0 ? 0 : decimals;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];

            const i = Math.floor(Math.log(bytes) / Math.log(k));

            return parseFloat((bytes / Math.pow(k, i)).toFixed(dm)) + ' ' + sizes[i];
        }

        // Função para atualizar o Dashboard com dados JSON reais
        function updateStatus(data) {
            // console.log("Dados recebidos:", data);

            // 1. Rede Wi-Fi
            document.getElementById('wifi_status').textContent = data.wifi.status;
            document.getElementById('wifi_ssid').textContent = data.wifi.ssid;
            document.getElementById('wifi_ip_local').textContent = data.wifi.local_ip;
            document.getElementById('wifi_ip_client').textContent = data.system.client_ip || 'N/A';
            
            // 2. Sistema (Incluindo a Data/Hora)
            document.getElementById('sys_hostname').textContent = data.system.hostname;
            document.getElementById('sys_current_time').textContent = data.system.current_time || 'N/A';
            document.getElementById('sys_uptime').textContent = data.system.uptime;
            document.getElementById('sys_heap').textContent = formatBytes(data.system.free_heap);
            
            // 3. Sincronização
            document.getElementById('sync_ntp').textContent = data.sync.ntp_status;
            document.getElementById('sync_mqtt').textContent = data.sync.mqtt_status;
            
            // Lógica visual (cores)
            const wifiStatusElement = document.getElementById('wifi_status');
            if (data.wifi.status === 'Conectado') {
                wifiStatusElement.classList.remove('text-red-700');
                wifiStatusElement.classList.add('text-green-500');
            } else {
                wifiStatusElement.classList.remove('text-green-500');
                wifiStatusElement.classList.add('text-red-700');
            }
        }

        // Função para buscar dados REALES do ESP32 via rota /status/json
        async function fetchStatusData() {
            try {
                const response = await fetch('/status/json');
                
                if (!response.ok) {
                    throw new Error(`Erro HTTP: ${response.status}`);
                }
                
                const data = await response.json();
                updateStatus(data); 
                
            } catch (error) {
                console.error("Erro ao carregar status:", error);
                document.getElementById('sys_heap').textContent = 'Erro';
            }
        }

        // Atualiza conforme o tempo de refresh configurado (padrão 15 segundos)
        let refreshInterval = 15000; // Padrão
        
        window.onload = function() {
            fetchStatusData();
            
            // Busca o tempo de refresh configurado
            fetch('/config/json')
                .then(response => response.json())
                .then(config => {
                    if (config.tempo_refresh) {
                        refreshInterval = config.tempo_refresh * 1000; // Converte para ms
                    }
                    setInterval(fetchStatusData, refreshInterval);
                })
                .catch(error => {
                    console.error("Erro ao carregar configurações:", error);
                    setInterval(fetchStatusData, refreshInterval); // Usa padrão se houver erro
                });
        };
    </script>
</body>
</html>
)raw_string";

#endif // WEB_DASHBOARD_H