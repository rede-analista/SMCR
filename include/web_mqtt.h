// Conteúdo de web_mqtt.h
#ifndef WEB_MQTT_H
#define WEB_MQTT_H

#include <Arduino.h>

// Página de configuração de MQTT
const char web_mqtt_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Configurações MQTT</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #f0f0f0; 
        }
        .container { 
            max-width: 800px; 
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
        .actions { 
            text-align: center; 
            margin: 20px 0; 
            padding: 15px; 
            background: #e9ecef; 
            border-radius: 4px; 
        }
        .actions button { 
            margin: 0 10px; 
            padding: 8px 16px; 
            border: none; 
            border-radius: 4px; 
            cursor: pointer; 
            font-weight: bold; 
        }
        .btn-apply { background: #17a2b8; color: white; }
        .btn-save { background: #28a745; color: white; }
        .btn-reboot { background: #fd7e14; color: white; }
        .config-table { 
            width: 100%; 
            border-collapse: collapse; 
            margin: 20px 0; 
            background: white; 
        }
        .config-table td { 
            padding: 12px; 
            border: 1px solid #ddd; 
            vertical-align: middle; 
        }
        .config-table td:first-child { 
            font-weight: bold; 
            color: #555; 
            width: 30%; 
            background: #f8f9fa; 
        }
        .config-table input, .config-table select { 
            width: 100%; 
            padding: 8px; 
            border: 1px solid #ccc; 
            border-radius: 4px; 
        }
        .alert { 
            padding: 15px; 
            margin: 20px 0; 
            border: 1px solid; 
            border-radius: 4px; 
        }
        .alert-warning { 
            background: #fff3cd; 
            border-color: #ffeaa7; 
            color: #856404; 
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
        <div class="loading-text">Carregando configurações MQTT...</div>
    </div>
    <div class="container">
        <h1>SMCR - Configurações MQTT</h1>
        
        <div class="menu">
            <a href="/">Status</a>
            <a href="/configuracao">Configuracoes Gerais</a>
            <a href="/pinos">Pinos/Reles</a>
            <a href="/acoes">Acoes</a>
            <a href="/mqtt">MQTT/Servicos</a>
            <a href="/arquivos">Arquivos</a>
            <a href="/reset">Reset</a>
        </div>

        <div class="actions">
            <button type="button" class="btn-apply" onclick="applyMqttConfig()">Aplicar</button>
        </div>

        <div class="alert alert-warning">
            <strong>⚠️ Funcionalidade em desenvolvimento</strong><br>
            Esta página será implementada em versões futuras para permitir configuração de serviços MQTT.
        </div>

        <form id="mqttForm">
            <h3 style="color: #333; margin: 20px 0 15px 0; border-bottom: 1px solid #ddd; padding-bottom: 8px;">Configurações MQTT</h3>
            <table class="config-table">
                <tr>
                    <td>Habilitar MQTT:</td>
                    <td>
                        <select name="mqtt_enabled" disabled>
                            <option value="false">Desabilitado</option>
                            <option value="true">Habilitado</option>
                        </select>
                    </td>
                </tr>
                <tr>
                    <td>Servidor MQTT:</td>
                    <td><input type="text" name="mqtt_server" placeholder="broker.mqtt.com" disabled></td>
                </tr>
                <tr>
                    <td>Porta:</td>
                    <td><input type="number" name="mqtt_port" value="1883" min="1" max="65535" disabled></td>
                </tr>
                <tr>
                    <td>Usuário (opcional):</td>
                    <td><input type="text" name="mqtt_user" placeholder="Deixe em branco se não usar autenticação" disabled></td>
                </tr>
                <tr>
                    <td>Senha (opcional):</td>
                    <td><input type="password" name="mqtt_pass" placeholder="Deixe em branco se não usar autenticação" disabled></td>
                </tr>
                <tr>
                    <td>Tópico Base:</td>
                    <td><input type="text" name="mqtt_topic" placeholder="smcr/modulo1" disabled></td>
                </tr>
            </table>
            <p style="font-size: 12px; color: #666; margin-top: 10px;">
                <strong>Tópico Base:</strong> usado para publicação de status e recebimento de comandos.<br>
                Exemplo: com tópico "smcr/modulo1", será usado "smcr/modulo1/status" para publicar status.
            </p>
        </form>
    </div>

    <script>
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
        });

        function applyMqttConfig() {
            if (confirm('Aplicar configuracoes MQTT? O ESP sera reiniciado.')) {
                showLoading('Aplicando configuracoes MQTT e reiniciando...');
                
                // Simular salvamento (MQTT ainda não implementado)
                setTimeout(() => {
                    showStatus('Configuracoes MQTT aplicadas! Reiniciando ESP...', 'success');
                    
                    // Reiniciar ESP após 2 segundos
                    setTimeout(() => {
                        fetch('/restart', { method: 'POST' })
                            .then(() => {
                                showStatus('ESP reiniciado com sucesso!', 'success');
                                // Recarregar página após reinicialização
                                setTimeout(() => {
                                    window.location.reload();
                                }, 3000);
                            })
                            .catch(() => {
                                showStatus('ESP reiniciado. Recarregue a página manualmente.', 'success');
                            });
                    }, 2000);
                }, 1000);
            }
        }

        function showStatus(message, type) {
            // Criar elemento de status se não existir
            let statusDiv = document.getElementById('status-message');
            if (!statusDiv) {
                statusDiv = document.createElement('div');
                statusDiv.id = 'status-message';
                statusDiv.style.cssText = 'margin: 20px 0; padding: 10px; border-radius: 4px; text-align: center;';
                document.querySelector('.container').appendChild(statusDiv);
            }
            
            statusDiv.textContent = message;
            
            // Aplicar estilos baseados no tipo
            if (type === 'success') {
                statusDiv.style.background = '#d4edda';
                statusDiv.style.color = '#155724';
                statusDiv.style.border = '1px solid #c3e6cb';
            } else if (type === 'error') {
                statusDiv.style.background = '#f8d7da';
                statusDiv.style.color = '#721c24';
                statusDiv.style.border = '1px solid #f5c6cb';
            }
            
            statusDiv.style.display = 'block';
            
            setTimeout(() => {
                statusDiv.style.display = 'none';
            }, 5000);
        }

        // === SISTEMA DE MONITORAMENTO DE PERFORMANCE ===
        const pageStartTime = performance.now();
        
        // Função para mostrar indicador de performance
        function showPerformanceIndicator() {
            const loadTime = performance.now() - pageStartTime;
            const indicator = document.createElement('div');
            indicator.style.cssText = 'position: fixed; bottom: 10px; right: 10px; background: rgba(0,0,0,0.7); color: white; padding: 8px 12px; border-radius: 6px; font-size: 12px; font-family: monospace;';
            indicator.innerHTML = `Tempo de Carregamento: ${loadTime.toFixed(2)}ms`;
            document.body.appendChild(indicator);
            
            console.log(`[PERFORMANCE] MQTT carregado em ${loadTime.toFixed(2)}ms`);
        }

        // Mostrar indicador após carregamento
        window.addEventListener('load', function() {
            hideLoading();
            setTimeout(showPerformanceIndicator, 100);
        });
    </script>
</body>
</html>
)rawliteral";

#endif