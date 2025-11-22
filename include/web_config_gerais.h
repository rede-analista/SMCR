// Conteúdo de web_config_gerais.h
#ifndef WEB_CONFIG_GERAIS_H
#define WEB_CONFIG_GERAIS_H

#include <Arduino.h>

// Página de Configurações Gerais
const char web_config_gerais_html[] PROGMEM = R"raw_string(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Configurações Gerais</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background-color: #f3f4f6;
        }
        .form-section {
            background-color: white;
            padding: 1.5rem;
            border-radius: 0.75rem;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -2px rgba(0, 0, 0, 0.1);
            margin-bottom: 1.5rem;
        }
        .form-group {
            margin-bottom: 1rem;
        }
        .form-group label {
            display: block;
            font-weight: 600;
            margin-bottom: 0.5rem;
            color: #374151;
        }
        .form-group input, .form-group select {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #d1d5db;
            border-radius: 0.5rem;
            background-color: #f9fafb;
            color: #111827;
        }
        .form-group input:focus, .form-group select:focus {
            outline: none;
            border-color: #2563eb;
            box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.2);
        }
        .checkbox-group {
            display: flex;
            align-items: center;
            gap: 0.5rem;
        }
        .checkbox-group input[type="checkbox"] {
            width: auto;
            margin: 0;
        }
        .btn-primary {
            background-color: #2563eb;
            color: white;
            padding: 0.75rem 1.5rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
        }
        .btn-primary:hover {
            background-color: #1d4ed8;
        }
        .btn-success {
            background-color: #059669;
            color: white;
            padding: 0.75rem 1.5rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
        }
        .btn-success:hover {
            background-color: #047857;
        }
        .btn-warning {
            background-color: #d97706;
            color: white;
            padding: 0.75rem 1.5rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
        }
        .btn-warning:hover {
            background-color: #b45309;
        }
        .btn-secondary {
            background-color: #6b7280;
            color: white;
            padding: 0.75rem 1.5rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
            text-decoration: none;
            display: inline-block;
        }
        .btn-secondary:hover {
            background-color: #4b5563;
        }
        .status-message {
            margin-top: 1rem;
            padding: 1rem;
            border-radius: 0.5rem;
            text-align: center;
            font-weight: 500;
            display: none;
        }
        .status-success {
            background-color: #d1fae5;
            color: #065f46;
        }
        .status-error {
            background-color: #fee2e2;
            color: #991b1b;
        }
        .status-info {
            background-color: #dbeafe;
            color: #1e40af;
        }
        .status-warning {
            background-color: #fef3c7;
            color: #92400e;
        }
    </style>
</head>
<body class="min-h-screen">
    <!-- Menu Superior (Navbar) -->
    <header class="bg-white shadow-md">
        <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4 flex justify-between items-center">
            <h1 class="text-xl font-bold text-gray-800">SMCR - Configurações Gerais</h1>
            <nav class="space-x-4 text-sm font-medium">
                <a href="/" class="text-gray-600 hover:text-blue-600">Status</a>
                <a href="/configuracao" class="text-blue-600 hover:text-blue-800">Configurações Gerais</a>
                <a href="/pinos" class="text-gray-600 hover:text-blue-600">Pinos/Relés</a>
                <a href="/mqtt" class="text-gray-600 hover:text-blue-600">MQTT/Serviços</a>
                <a href="/reset" class="text-red-600 hover:text-red-800">Reset</a>
            </nav>
        </div>
    </header>

    <!-- Conteúdo Principal -->
    <main class="max-w-5xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        <form id="configForm" class="space-y-6">
            
            <!-- Seção: Configurações de Rede -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações de Rede</h3>
                <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div class="form-group">
                        <label for="hostname">Hostname</label>
                        <input type="text" id="hostname" name="hostname" required>
                    </div>
                    <div class="form-group">
                        <label for="wifi_ssid">SSID da Rede WiFi</label>
                        <input type="text" id="wifi_ssid" name="wifi_ssid">
                    </div>
                    <div class="form-group">
                        <label for="wifi_pass">Senha da Rede WiFi</label>
                        <input type="password" id="wifi_pass" name="wifi_pass">
                    </div>
                    <div class="form-group">
                        <label for="wifi_attempts">Tentativas de Conexão WiFi</label>
                        <input type="number" id="wifi_attempts" name="wifi_attempts" min="1" max="60">
                    </div>
                </div>
            </div>

            <!-- Seção: Configurações de NTP -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações de Tempo (NTP)</h3>
                <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
                    <div class="form-group">
                        <label for="ntp_server1">Servidor NTP</label>
                        <input type="text" id="ntp_server1" name="ntp_server1" placeholder="pool.ntp.br">
                    </div>
                    <div class="form-group">
                        <label for="gmt_offset">Fuso Horário GMT (segundos)</label>
                        <input type="number" id="gmt_offset" name="gmt_offset" placeholder="-10800">
                    </div>
                    <div class="form-group">
                        <label for="daylight_offset">Horário de Verão (segundos)</label>
                        <input type="number" id="daylight_offset" name="daylight_offset" placeholder="0">
                    </div>
                </div>
            </div>

            <!-- Seção: Configurações da Interface -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações da Interface</h3>
                <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div class="form-group">
                        <div class="checkbox-group">
                            <input type="checkbox" id="status_pinos" name="status_pinos">
                            <label for="status_pinos">Status dos Pinos na Página Inicial</label>
                        </div>
                    </div>
                    <div class="form-group">
                        <div class="checkbox-group">
                            <input type="checkbox" id="inter_modulos" name="inter_modulos">
                            <label for="inter_modulos">Inter Módulos na Página Inicial</label>
                        </div>
                    </div>
                    <div class="form-group">
                        <label for="cor_com_alerta">Cor Status Com Alerta</label>
                        <input type="color" id="cor_com_alerta" name="cor_com_alerta" value="#ff0000">
                    </div>
                    <div class="form-group">
                        <label for="cor_sem_alerta">Cor Status Sem Alerta</label>
                        <input type="color" id="cor_sem_alerta" name="cor_sem_alerta" value="#00ff00">
                    </div>
                    <div class="form-group">
                        <label for="tempo_refresh">Tempo de Refresh (segundos)</label>
                        <input type="number" id="tempo_refresh" name="tempo_refresh" min="5" max="300">
                    </div>
                </div>
            </div>

            <!-- Seção: Configurações do Watchdog -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações do Watchdog</h3>
                <div class="grid grid-cols-1 md:grid-cols-3 gap-4">
                    <div class="form-group">
                        <div class="checkbox-group">
                            <input type="checkbox" id="watchdog_enabled" name="watchdog_enabled">
                            <label for="watchdog_enabled">Executar Watchdog</label>
                        </div>
                    </div>
                    <div class="form-group">
                        <label for="clock_esp32">Clock do ESP32 (MHz)</label>
                        <input type="number" id="clock_esp32" name="clock_esp32" min="80" max="240" placeholder="240">
                    </div>
                    <div class="form-group">
                        <label for="tempo_watchdog">Tempo para Watchdog (µs)</label>
                        <input type="number" id="tempo_watchdog" name="tempo_watchdog" min="1000000" placeholder="8000000">
                    </div>
                </div>
            </div>

            <!-- Seção: Configurações dos Pinos -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações dos Pinos</h3>
                <div class="form-group">
                    <label for="qtd_pinos">Quantidade Total de Pinos</label>
                    <input type="number" id="qtd_pinos" name="qtd_pinos" min="1" max="254" placeholder="16">
                </div>
            </div>

            <!-- Seção: Configurações do Servidor Web -->
            <div class="form-section">
                <h3 class="text-lg font-semibold text-gray-700 mb-4">Configurações do Servidor Web</h3>
                <div class="grid grid-cols-1 md:grid-cols-2 gap-4">
                    <div class="form-group">
                        <label for="web_port">Porta do Servidor Web</label>
                        <input type="number" id="web_port" name="web_port" min="80" max="65535" placeholder="8080">
                    </div>
                    <div class="form-group">
                        <div class="checkbox-group">
                            <input type="checkbox" id="auth_enabled" name="auth_enabled">
                            <label for="auth_enabled">Habilitar Autenticação Web</label>
                        </div>
                    </div>
                    <div class="form-group">
                        <label for="web_username">Usuário Administrador</label>
                        <input type="text" id="web_username" name="web_username" placeholder="admin">
                    </div>
                    <div class="form-group">
                        <label for="web_password">Senha Administrador</label>
                        <input type="password" id="web_password" name="web_password" placeholder="senha123">
                    </div>
                    <div class="form-group">
                        <div class="checkbox-group">
                            <input type="checkbox" id="dashboard_auth" name="dashboard_auth">
                            <label for="dashboard_auth">Dashboard Requer Autenticação</label>
                        </div>
                    </div>
                </div>
                <div class="bg-blue-50 border border-blue-200 rounded-md p-3 mt-4">
                    <p class="text-sm text-blue-700">
                        <strong>Nota:</strong> Quando a autenticação estiver habilitada, você pode escolher se o dashboard (página inicial) 
                        também requerá login. Se desabilitado, o dashboard funcionará como painel público de apresentação.
                    </p>
                </div>
            </div>

            <!-- Botões de Ação - Estilo Cisco -->
            <div class="flex gap-4 pt-4">
                <button type="button" id="applyBtn" class="btn-primary">Aplicar (Running-Config)</button>
                <button type="button" id="saveBtn" class="btn-success">Salvar na Flash (Startup-Config)</button>
                <button type="button" id="saveAndRebootBtn" class="btn-warning">Salvar e Reiniciar</button>
                <a href="/" class="btn-secondary">Voltar ao Dashboard</a>
            </div>

            <div id="status" class="status-message"></div>
        </form>
    </main>

    <script>
        // Carregar configurações atuais quando a página carrega
        async function carregarConfiguracoes() {
            console.log('Iniciando carregamento das configurações...');
            
            try {
                const response = await fetch('/config/json');
                console.log('Resposta recebida:', response.status, response.statusText);
                
                if (response.ok) {
                    const config = await response.json();
                    console.log('Configurações recebidas:', config);
                    
                    // Configurações de rede
                    if (config.hostname !== undefined) document.getElementById('hostname').value = config.hostname;
                    if (config.wifi_ssid !== undefined) document.getElementById('wifi_ssid').value = config.wifi_ssid;
                    if (config.wifi_pass !== undefined) document.getElementById('wifi_pass').value = config.wifi_pass;
                    if (config.wifi_attempts !== undefined) document.getElementById('wifi_attempts').value = config.wifi_attempts;
                    
                    // Configurações de NTP
                    if (config.ntp_server1 !== undefined) document.getElementById('ntp_server1').value = config.ntp_server1;
                    if (config.gmt_offset !== undefined) document.getElementById('gmt_offset').value = config.gmt_offset;
                    if (config.daylight_offset !== undefined) document.getElementById('daylight_offset').value = config.daylight_offset;
                    
                    // Configurações da interface
                    if (config.status_pinos !== undefined) document.getElementById('status_pinos').checked = config.status_pinos;
                    if (config.inter_modulos !== undefined) document.getElementById('inter_modulos').checked = config.inter_modulos;
                    if (config.cor_com_alerta !== undefined) document.getElementById('cor_com_alerta').value = config.cor_com_alerta;
                    if (config.cor_sem_alerta !== undefined) document.getElementById('cor_sem_alerta').value = config.cor_sem_alerta;
                    if (config.tempo_refresh !== undefined) document.getElementById('tempo_refresh').value = config.tempo_refresh;
                    
                    // Configurações do watchdog
                    if (config.watchdog_enabled !== undefined) document.getElementById('watchdog_enabled').checked = config.watchdog_enabled;
                    if (config.clock_esp32 !== undefined) document.getElementById('clock_esp32').value = config.clock_esp32;
                    if (config.tempo_watchdog !== undefined) document.getElementById('tempo_watchdog').value = config.tempo_watchdog;
                    
                    // Configurações dos pinos
                    if (config.qtd_pinos !== undefined) document.getElementById('qtd_pinos').value = config.qtd_pinos;
                    
                    // Configurações do servidor web
                    if (config.web_port !== undefined) document.getElementById('web_port').value = config.web_port;
                    if (config.auth_enabled !== undefined) document.getElementById('auth_enabled').checked = config.auth_enabled;
                    if (config.web_username !== undefined) document.getElementById('web_username').value = config.web_username;
                    if (config.web_password !== undefined) document.getElementById('web_password').value = config.web_password;
                    if (config.dashboard_auth !== undefined) document.getElementById('dashboard_auth').checked = config.dashboard_auth;
                    
                    console.log('Configurações carregadas e aplicadas com sucesso!');
                } else {
                    console.error('Erro na resposta:', response.status, response.statusText);
                    const errorText = await response.text();
                    console.error('Detalhes do erro:', errorText);
                }
            } catch (error) {
                console.error('Erro ao carregar configurações:', error);
                console.error('Stack trace:', error.stack);
            }
        }

        // Função para coletar dados do formulário
        function coletarDadosFormulario() {
            const form = document.getElementById('configForm');
            return new URLSearchParams(new FormData(form));
        }

        // Função para mostrar status
        function mostrarStatus(mensagem, tipo) {
            const statusDiv = document.getElementById('status');
            statusDiv.style.display = 'block';
            statusDiv.className = `status-message status-${tipo}`;
            statusDiv.textContent = mensagem;
        }

        // Botão APLICAR (Running-Config) - Estilo Cisco
        document.getElementById('applyBtn').addEventListener('click', async function() {
            const formData = coletarDadosFormulario();
            mostrarStatus('Aplicando configurações em memória (Running-Config)...', 'info');

            try {
                const response = await fetch('/apply_config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });

                if (response.ok) {
                    const text = await response.text();
                    mostrarStatus('✅ Running-Config aplicada! Teste as funcionalidades e depois salve se estiver OK.', 'success');
                } else {
                    mostrarStatus('❌ Erro ao aplicar configurações.', 'error');
                }
            } catch (error) {
                mostrarStatus(`❌ Erro de conexão: ${error.message}`, 'error');
            }
        });

        // Botão SALVAR na Flash (Startup-Config) - Estilo Cisco
        document.getElementById('saveBtn').addEventListener('click', async function() {
            mostrarStatus('Salvando Running-Config na Flash (Startup-Config)...', 'info');

            try {
                const response = await fetch('/save_to_flash', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: ''
                });

                if (response.ok) {
                    const text = await response.text();
                    mostrarStatus('✅ Startup-Config salva! Configurações persistidas na flash.', 'success');
                } else {
                    mostrarStatus('❌ Erro ao salvar na flash.', 'error');
                }
            } catch (error) {
                mostrarStatus(`❌ Erro de conexão: ${error.message}`, 'error');
            }
        });

        // Botão SALVAR e REINICIAR (Comportamento original)
        document.getElementById('saveAndRebootBtn').addEventListener('click', async function() {
            const formData = coletarDadosFormulario();
            mostrarStatus('Salvando e reiniciando módulo...', 'warning');

            try {
                const response = await fetch('/save_config', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                    body: formData
                });

                if (response.ok) {
                    mostrarStatus('✅ Configurações salvas! Módulo reiniciando...', 'success');
                } else {
                    mostrarStatus('❌ Erro ao salvar e reiniciar.', 'error');
                }
            } catch (error) {
                mostrarStatus(`❌ Erro de conexão: ${error.message}`, 'error');
            }
        });

        // Carrega as configurações quando a página é aberta
        window.onload = carregarConfiguracoes;
    </script>
</body>
</html>
)raw_string";

#endif // WEB_CONFIG_GERAIS_H