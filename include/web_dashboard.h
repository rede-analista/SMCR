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
    <title>SMCR | Dashboard de Status</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body { font-family: 'Inter', sans-serif; }
        .card { transition: transform 0.2s; }
        .card:hover { transform: translateY(-3px); box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1); }
    </style>
</head>
<body class="bg-gray-50 text-gray-800">
    <div class="min-h-screen flex flex-col">
        <!-- Menu Superior (Navbar) -->
        <header class="bg-white shadow-md">
            <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-3 flex justify-between items-center">
                <div class="text-xl font-bold text-blue-600">SMCR - Central de Controle</div>
                <nav class="hidden md:flex space-x-4">
                    <a href="/" class="text-gray-600 hover:text-blue-600 font-medium">Status</a>
                    <a href="/config" class="text-gray-600 hover:text-blue-600 font-medium">Configurações Gerais</a>
                    <a href="/pinos" class="text-gray-600 hover:text-blue-600 font-medium">Pinos/Relés</a>
                    <a href="/mqtt" class="text-gray-600 hover:text-blue-600 font-medium">MQTT/Serviços</a>
                    <a href="/reset" class="text-red-500 hover:text-red-700 font-medium">Reset</a>
                </nav>
                <!-- Botão Menu Mobile (hidden) -->
            </div>
        </header>

        <!-- Conteúdo Principal -->
        <main class="flex-grow max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-8 w-full">
            <h1 class="text-3xl font-extrabold mb-6">Status do Módulo</h1>

            <div class="grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-6 mb-8">
                <!-- Card 1: Status da Rede -->
                <div class="card bg-white p-6 rounded-xl shadow-lg border-b-4 border-green-500">
                    <h2 class="text-lg font-semibold mb-3 flex items-center">
                        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5 mr-2 text-green-500"><path d="M12 21.5s-4-3-4-9c0-4.4 3.6-8 8-8s8 3.6 8 8c0 6-4 9-4 9z"/><path d="M12 11v-4"/><path d="M12 15h0"/></svg>
                        Rede Wi-Fi
                    </h2>
                    <p class="text-sm text-gray-600 mb-1">Status: <span id="wifi_status" class="font-bold text-green-700">Conectado</span></p>
                    <p class="text-sm text-gray-600 mb-1">SSID: <span id="wifi_ssid" class="font-medium">Carregando...</span></p>
                    <p class="text-sm text-gray-600">IP Local: <span id="local_ip" class="font-medium text-blue-600">Carregando...</span></p>
                </div>

                <!-- Card 2: Status do Sistema -->
                <div class="card bg-white p-6 rounded-xl shadow-lg border-b-4 border-blue-500">
                    <h2 class="text-lg font-semibold mb-3 flex items-center">
                        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5 mr-2 text-blue-500"><path d="M12 21c-4.4 0-8-3.6-8-8V3h16v10c0 4.4-3.6 8-8 8z"/><path d="M12 3v10"/><path d="M15 17h-6"/></svg>
                        Sistema
                    </h2>
                    <p class="text-sm text-gray-600 mb-1">Hostname: <span id="sys_hostname" class="font-medium">Carregando...</span></p>
                    <p class="text-sm text-gray-600 mb-1">Tempo de Atividade (Uptime): <span id="sys_uptime" class="font-medium">Carregando...</span></p>
                    <p class="text-sm text-gray-600">Memória Livre (Heap): <span id="sys_heap" class="font-medium">Carregando...</span></p>
                </div>

                <!-- Card 3: Serviços -->
                <div class="card bg-white p-6 rounded-xl shadow-lg border-b-4 border-yellow-500">
                    <h2 class="text-lg font-semibold mb-3 flex items-center">
                        <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round" class="w-5 h-5 mr-2 text-yellow-500"><path d="M12 21v-4"/><path d="M5 12H2"/><path d="M22 12h-3"/><path d="M15.5 4.5l-2.4 2.4"/><path d="M8.5 19.5l2.4-2.4"/><path d="M19.5 8.5l-2.4 2.4"/><path d="M4.5 15.5l2.4-2.4"/><circle cx="12" cy="12" r="3"/></svg>
                        Sincronização
                    </h2>
                    <p class="text-sm text-gray-600 mb-1">NTP: <span id="ntp_status" class="font-medium">Desabilitado</span></p>
                    <p class="text-sm text-gray-600 mb-1">MQTT: <span id="mqtt_status" class="font-medium">Desabilitado</span></p>
                    <p class="text-sm text-gray-600">Última Atualização: <span id="last_update" class="font-medium">Carregando...</span></p>
                </div>
            </div>

            <div class="mt-8 p-6 bg-white rounded-xl shadow-lg">
                <h2 class="text-2xl font-bold mb-4">Informações de Pinos (Ainda Não Implementadas)</h2>
                <p class="text-gray-600">Esta seção será preenchida com o status em tempo real dos seus relés e sensores.</p>
            </div>
        </main>

        <!-- Rodapé (Footer) -->
        <footer class="bg-gray-200 py-4 text-center text-sm text-gray-600 mt-4">
            &copy; 2025 SMCR - Sistema de Controle e Monitoramento Remoto
        </footer>
    </div>

    <!-- Script de Atualização (Placeholder) -->
    <script>
        function updateStatus() {
            // Em uma proxima etapa, faremos uma chamada AJAX para o ESP32 (ex: /status/json)
            // para obter dados em tempo real.
            console.log("Atualizando status...");
            // Exemplo de como usar a variavel global de configuracao (se disponivel no C++)
            // document.getElementById('sys_hostname').textContent = 'Valor de vSt_mainConfig.vS_hostname';
            
            // Placeholder:
            document.getElementById('sys_uptime').textContent = '2d 4h 32m';
            document.getElementById('sys_heap').textContent = `${ESP.getFreeHeap()} bytes`; 
            document.getElementById('wifi_ssid').textContent = 'IoT24';
            document.getElementById('local_ip').textContent = '192.168.1.100';
            document.getElementById('last_update').textContent = new Date().toLocaleTimeString();
        }

        // Simula o refresh a cada 5 segundos
        // setInterval(updateStatus, 5000); 
        // updateStatus();
    </script>
</body>
</html>
)raw_string";

#endif // WEB_DASHBOARD_H