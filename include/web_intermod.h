#ifndef WEB_INTERMOD_H
#define WEB_INTERMOD_H
#include <Arduino.h>


const char WEB_HTML_INTERMOD[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Inter-Módulos</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Oxygen, Ubuntu, Cantarell, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            padding: 20px;
            min-height: 100vh;
        }
        .container {
            max-width: 1000px;
            margin: 0 auto;
            background: white;
            border-radius: 16px;
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            overflow: hidden;
        }
        .header {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 30px;
            text-align: center;
        }
        h1 {
            font-size: 28px;
            font-weight: 600;
        }
        .menu {
            background: #f8f9fa;
            padding: 0;
            display: flex;
            flex-wrap: wrap;
            justify-content: center;
            border-bottom: 2px solid #e9ecef;
        }
        .menu a {
            padding: 15px 20px;
            text-decoration: none;
            color: #495057;
            font-weight: 500;
            transition: all 0.3s;
            border-bottom: 3px solid transparent;
        }
        .menu a:hover {
            background: #e9ecef;
            color: #667eea;
            border-bottom-color: #667eea;
        }
        .content {
            padding: 30px;
        }
        .section {
            margin-bottom: 30px;
            background: #f8f9fa;
            padding: 20px;
            border-radius: 8px;
            border-left: 4px solid #667eea;
        }
        .section-title {
            font-size: 20px;
            font-weight: 600;
            color: #495057;
            margin-bottom: 15px;
        }
        .info-box {
            background: #e7f3ff;
            border-left: 4px solid #2196F3;
            padding: 15px;
            margin: 15px 0;
            border-radius: 4px;
        }
        .warning-box {
            background: #fff3cd;
            border-left: 4px solid #ffc107;
            padding: 15px;
            margin: 15px 0;
            border-radius: 4px;
        }
        .btn {
            padding: 12px 24px;
            border: none;
            border-radius: 6px;
            font-size: 14px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s;
            text-decoration: none;
            display: inline-block;
            margin: 5px;
        }
        .btn-primary {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
        }
        .btn-primary:hover {
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.4);
        }
        @media (max-width: 768px) {
            .menu {
                flex-direction: column;
            }
            .menu a {
                border-bottom: 1px solid #dee2e6;
                border-left: 3px solid transparent;
            }
            .menu a:hover {
                border-bottom-color: #dee2e6;
                border-left-color: #667eea;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔗 SMCR - Inter-Módulos</h1>
        </div>
        <nav class="navbar" style="background:#f8f9fa;border-bottom:1px solid #e9ecef;padding:10px 14px;display:flex;gap:12px;flex-wrap:wrap;align-items:center;">
            <a href="/" style="text-decoration:none;color:#333;padding:8px 12px;border-radius:6px;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Status</a>
            <a href="/configuracao" style="text-decoration:none;color:#333;padding:8px 12px;border-radius:6px;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Configurações</a>
            <a href="/pinos" style="text-decoration:none;color:#333;padding:8px 12px;border-radius:6px;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Pinos</a>
            <a href="/acoes" style="text-decoration:none;color:#333;padding:8px 12px;border-radius:6px;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Ações</a>
            <details style="position:relative;">
                <summary style="list-style:none;cursor:pointer;padding:8px 12px;border-radius:6px;color:#333;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Serviços</summary>
                <div style="position:absolute;background:#fff;border:1px solid #e9ecef;box-shadow:0 8px 20px rgba(0,0,0,0.1);border-radius:8px;padding:8px;display:flex;flex-direction:column;gap:6px;z-index:10;">
                    <a href="/mqtt" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">MQTT</a>
                    <a href="/intermod" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">Inter-Módulos</a>
                </div>
            </details>
            <details style="position:relative;">
                <summary style="list-style:none;cursor:pointer;padding:8px 12px;border-radius:6px;color:#333;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Arquivos</summary>
                <div style="position:absolute;background:#fff;border:1px solid #e9ecef;box-shadow:0 8px 20px rgba(0,0,0,0.1);border-radius:8px;padding:8px;display:flex;flex-direction:column;gap:6px;z-index:10;">
                    <a href="/arquivos/firmware" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">Firmware</a>
                    <a href="/arquivos/preferencias" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">Preferências</a>
                    <a href="/arquivos/littlefs" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">LittleFS</a>
                </div>
            </details>
            <details style="position:relative;">
                <summary style="list-style:none;cursor:pointer;padding:8px 12px;border-radius:6px;color:#333;border:1px solid #dee2e6;background:#fff;transition:background 0.2s;" onmouseover="this.style.background='#e9ecef'" onmouseout="this.style.background='#fff'">Util</summary>
                <div style="position:absolute;background:#fff;border:1px solid #e9ecef;box-shadow:0 8px 20px rgba(0,0,0,0.1);border-radius:8px;padding:8px;display:flex;flex-direction:column;gap:6px;z-index:10;">
                    <a href="/serial" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">Web Serial</a>
                    <a href="/reset" style="text-decoration:none;color:#333;padding:6px 8px;border-radius:4px;">Reset</a>
                </div>
            </details>
        </nav>

        <div class="content">
            <div class="section">
                <div class="section-title">🔗 Comunicação Inter-Módulos</div>
                <div class="info-box">
                    <strong>ℹ️ Funcionalidade em Desenvolvimento</strong><br>
                    Esta seção permitirá a configuração de comunicação entre múltiplos módulos SMCR na rede local.
                </div>
            </div>

            <div class="section">
                <div class="section-title">📋 Recursos Planejados</div>
                <ul style="margin-left: 20px; line-height: 1.8;">
                    <li>Descoberta automática de módulos na rede</li>
                    <li>Envio de comandos entre módulos</li>
                    <li>Sincronização de estados</li>
                    <li>Acionamento remoto de pinos</li>
                    <li>Notificações entre dispositivos</li>
                </ul>
            </div>

            <div class="warning-box">
                <strong>⚠️ Aviso:</strong> Esta funcionalidade será implementada em versões futuras do firmware.
            </div>
        </div>
    </div>
    <script>
        // === SISTEMA DE MONITORAMENTO DE PERFORMANCE ===
        const pageStartTime = performance.now();
        
        // Função para mostrar indicador de performance
        function showPerformanceIndicator() {
            const loadTime = performance.now() - pageStartTime;
            const indicator = document.createElement('div');
            indicator.style.cssText = 'position: fixed; bottom: 10px; right: 10px; background: rgba(0,0,0,0.7); color: white; padding: 8px 12px; border-radius: 6px; font-size: 12px; font-family: monospace;';
            indicator.innerHTML = `Tempo de Carregamento: ${loadTime.toFixed(2)}ms`;
            document.body.appendChild(indicator);
            
            console.log(`[PERFORMANCE] Inter-Módulos carregado em ${loadTime.toFixed(2)}ms`);
        }
        
        // Mostrar indicador após carregamento
        window.addEventListener('load', function() {
            setTimeout(showPerformanceIndicator, 100);
        });
    </script>
</body>
</html>
)rawliteral";

#endif // WEB_INTERMOD_H
