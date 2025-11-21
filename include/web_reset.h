// Conteúdo de web_reset.h

#ifndef WEB_RESET_H
#define WEB_RESET_H

#include <Arduino.h>

// Página de Reset e Reinicialização do Sistema
const char web_reset_html[] PROGMEM = R"raw_string(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Reset do Sistema</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;600;700&display=swap" rel="stylesheet">
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background-color: #f3f4f6;
        }
        .reset-card {
            background-color: white;
            padding: 2rem;
            border-radius: 0.75rem;
            box-shadow: 0 4px 6px -1px rgba(0, 0, 0, 0.1), 0 2px 4px -2px rgba(0, 0, 0, 0.1);
            margin-bottom: 1.5rem;
            border-left: 4px solid #ef4444;
        }
        .warning-card {
            background-color: #fef3c7;
            border: 1px solid #f59e0b;
            border-radius: 0.5rem;
            padding: 1rem;
            margin-bottom: 1.5rem;
        }
        .btn-danger {
            background-color: #dc2626;
            color: white;
            padding: 0.75rem 1.5rem;
            border: none;
            border-radius: 0.5rem;
            font-weight: 600;
            cursor: pointer;
            transition: background-color 0.2s;
            width: 100%;
            margin-bottom: 0.75rem;
        }
        .btn-danger:hover {
            background-color: #b91c1c;
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
            width: 100%;
            margin-bottom: 0.75rem;
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
            text-align: center;
            width: 100%;
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
        .countdown {
            font-size: 1.25rem;
            font-weight: bold;
            color: #dc2626;
        }
    </style>
</head>
<body class="min-h-screen">
    <!-- Menu Superior (Navbar) -->
    <header class="bg-white shadow-md">
        <div class="max-w-7xl mx-auto px-4 sm:px-6 lg:px-8 py-4 flex justify-between items-center">
            <h1 class="text-xl font-bold text-gray-800">SMCR - Reset do Sistema</h1>
            <nav class="space-x-4 text-sm font-medium">
                <a href="/" class="text-gray-600 hover:text-blue-600">Status</a>
                <a href="/configuracao" class="text-gray-600 hover:text-blue-600">Configurações Gerais</a>
                <a href="/pinos" class="text-gray-600 hover:text-blue-600">Pinos/Relés</a>
                <a href="/mqtt" class="text-gray-600 hover:text-blue-600">MQTT/Serviços</a>
                <a href="/reset" class="text-red-600 hover:text-red-800">Reset</a>
            </nav>
        </div>
    </header>

    <!-- Conteúdo Principal -->
    <main class="max-w-4xl mx-auto px-4 sm:px-6 lg:px-8 py-8">
        
        <!-- Aviso de Segurança -->
        <div class="warning-card">
            <div class="flex items-center">
                <div class="flex-shrink-0">
                    <span class="text-2xl">⚠️</span>
                </div>
                <div class="ml-3">
                    <h3 class="text-sm font-medium text-yellow-800">Área de Reset do Sistema</h3>
                    <p class="mt-1 text-sm text-yellow-700">
                        As operações desta página são <strong>irreversíveis</strong>. Use com cuidado!
                    </p>
                </div>
            </div>
        </div>

        <!-- Reset Simples -->
        <div class="reset-card">
            <h3 class="text-lg font-semibold text-gray-700 mb-4">🔄 Reinicializar Sistema</h3>
            <p class="text-gray-600 mb-4">
                Reinicia o ESP32 mantendo todas as configurações salvas. Útil para resolver problemas temporários.
            </p>
            <ul class="text-sm text-gray-500 mb-4 list-disc list-inside">
                <li>Mantém todas as configurações</li>
                <li>Reconecta à rede WiFi</li>
                <li>Reinicia todos os serviços</li>
                <li>Tempo de reinício: ~15 segundos</li>
            </ul>
            <button type="button" id="softResetBtn" class="btn-warning">
                🔄 Reinicializar Sistema
            </button>
        </div>

        <!-- Reset de Fábrica -->
        <div class="reset-card">
            <h3 class="text-lg font-semibold text-gray-700 mb-4">🏭 Reset de Fábrica</h3>
            <p class="text-gray-600 mb-4">
                <strong>ATENÇÃO:</strong> Apaga <strong>TODAS</strong> as configurações e volta às configurações de fábrica.
            </p>
            <ul class="text-sm text-gray-500 mb-4 list-disc list-inside">
                <li><strong>Apaga:</strong> WiFi, hostname, configurações de interface</li>
                <li><strong>Apaga:</strong> Configurações de pinos e ações</li>
                <li><strong>Apaga:</strong> Configurações de NTP e watchdog</li>
                <li>ESP32 volta ao modo AP para nova configuração</li>
                <li>Será necessário reconfigurar tudo novamente</li>
            </ul>
            <button type="button" id="factoryResetBtn" class="btn-danger">
                🏭 Reset de Fábrica (CUIDADO!)
            </button>
        </div>

        <!-- Reset de Rede -->
        <div class="reset-card">
            <h3 class="text-lg font-semibold text-gray-700 mb-4">🌐 Reset de Rede</h3>
            <p class="text-gray-600 mb-4">
                Apaga apenas as configurações de rede (WiFi). Mantém outras configurações intactas.
            </p>
            <ul class="text-sm text-gray-500 mb-4 list-disc list-inside">
                <li><strong>Apaga:</strong> SSID e senha WiFi</li>
                <li><strong>Mantém:</strong> Hostname, cores, pinos</li>
                <li><strong>Mantém:</strong> NTP e watchdog</li>
                <li>ESP32 volta ao modo AP apenas para WiFi</li>
            </ul>
            <button type="button" id="networkResetBtn" class="btn-warning">
                🌐 Reset de Rede
            </button>
        </div>

        <!-- Voltar -->
        <div class="reset-card">
            <h3 class="text-lg font-semibold text-gray-700 mb-4">↩️ Voltar</h3>
            <p class="text-gray-600 mb-4">
                Retornar ao dashboard sem fazer nenhuma alteração.
            </p>
            <a href="/" class="btn-secondary">
                ↩️ Voltar ao Dashboard
            </a>
        </div>

        <!-- Status Messages -->
        <div id="status" class="status-message"></div>
    </main>

    <script>
        let countdownInterval;

        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.style.display = 'block';
            statusDiv.className = `status-message status-${type}`;
            statusDiv.innerHTML = message;
        }

        function startCountdown(seconds, callback) {
            let remaining = seconds;
            showStatus(`<span class="countdown">Executando em ${remaining} segundos...</span><br>Clique novamente para cancelar.`, 'info');
            
            countdownInterval = setInterval(() => {
                remaining--;
                if (remaining > 0) {
                    showStatus(`<span class="countdown">Executando em ${remaining} segundos...</span><br>Clique novamente para cancelar.`, 'info');
                } else {
                    clearInterval(countdownInterval);
                    callback();
                }
            }, 1000);
        }

        function cancelCountdown() {
            if (countdownInterval) {
                clearInterval(countdownInterval);
                countdownInterval = null;
                showStatus('❌ Operação cancelada.', 'info');
                setTimeout(() => {
                    document.getElementById('status').style.display = 'none';
                }, 3000);
                return true;
            }
            return false;
        }

        // Reinicialização Simples
        document.getElementById('softResetBtn').addEventListener('click', function() {
            if (cancelCountdown()) return;
            
            startCountdown(5, async () => {
                showStatus('🔄 Reinicializando sistema...', 'info');
                try {
                    const response = await fetch('/reset/soft', {
                        method: 'POST'
                    });
                    
                    if (response.ok) {
                        showStatus('✅ Sistema reinicializado! Aguarde reconexão...', 'success');
                        setTimeout(() => {
                            window.location.href = '/';
                        }, 10000);
                    } else {
                        showStatus('❌ Erro ao reinicializar sistema.', 'error');
                    }
                } catch (error) {
                    showStatus('✅ Sistema reinicializado! (Conexão perdida - esperado)', 'success');
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 10000);
                }
            });
        });

        // Reset de Fábrica
        document.getElementById('factoryResetBtn').addEventListener('click', function() {
            if (cancelCountdown()) return;
            
            startCountdown(10, async () => {
                showStatus('🏭 Executando reset de fábrica...', 'info');
                try {
                    const response = await fetch('/reset/factory', {
                        method: 'POST'
                    });
                    
                    if (response.ok) {
                        showStatus('✅ Reset de fábrica concluído! Conecte ao AP "esp32modularx" para reconfigurar.', 'success');
                    } else {
                        showStatus('❌ Erro no reset de fábrica.', 'error');
                    }
                } catch (error) {
                    showStatus('✅ Reset de fábrica executado! Conecte ao AP "esp32modularx".', 'success');
                }
            });
        });

        // Reset de Rede
        document.getElementById('networkResetBtn').addEventListener('click', function() {
            if (cancelCountdown()) return;
            
            startCountdown(5, async () => {
                showStatus('🌐 Resetando configurações de rede...', 'info');
                try {
                    const response = await fetch('/reset/network', {
                        method: 'POST'
                    });
                    
                    if (response.ok) {
                        showStatus('✅ Reset de rede concluído! Conecte ao AP para reconfigurar WiFi.', 'success');
                    } else {
                        showStatus('❌ Erro no reset de rede.', 'error');
                    }
                } catch (error) {
                    showStatus('✅ Reset de rede executado! Conecte ao AP para WiFi.', 'success');
                }
            });
        });
    </script>
</body>
</html>
)raw_string";

#endif // WEB_RESET_H