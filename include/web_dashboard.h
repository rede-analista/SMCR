// Conteúdo de web_dashboard.h
#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// Dashboard principal - Servido de LittleFS (data/web_dashboard.html)
// Fallback básico com link para upload de arquivos
const char web_dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Configuração Inicial</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        .container { 
            max-width: 600px; 
            background: white; 
            padding: 40px; 
            border-radius: 12px; 
            box-shadow: 0 10px 40px rgba(0,0,0,0.3);
            text-align: center;
        }
        h1 { 
            color: #333; 
            margin-bottom: 20px;
            font-size: 28px;
        }
        .warning {
            background: #fff3cd;
            border: 2px solid #ffc107;
            color: #856404;
            padding: 20px;
            border-radius: 8px;
            margin: 20px 0;
        }
        .warning h2 {
            margin-top: 0;
            color: #856404;
        }
        .btn {
            display: inline-block;
            padding: 15px 30px;
            margin: 10px;
            background: #007bff;
            color: white;
            text-decoration: none;
            border-radius: 6px;
            font-weight: bold;
            font-size: 16px;
            transition: background 0.3s;
        }
        .btn:hover {
            background: #0056b3;
        }
        .btn-success {
            background: #28a745;
        }
        .btn-success:hover {
            background: #1e7e34;
        }
        .info {
            background: #e7f3ff;
            border-left: 4px solid #2196f3;
            padding: 15px;
            margin: 20px 0;
            text-align: left;
        }
        .info p {
            margin: 8px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🚀 SMCR - Sistema Modular</h1>
        
        <div class="warning">
            <h2>⚠️ Configuração Inicial Necessária</h2>
            <p>As páginas HTML ainda não foram carregadas no sistema de arquivos.</p>
        </div>

        <div class="info">
            <p><strong>Para configurar o sistema:</strong></p>
            <p>1. Acesse a página de gerenciamento de arquivos</p>
            <p>2. Faça o upload dos arquivos HTML da pasta <code>data/</code></p>
            <p>3. Após o upload, todas as páginas funcionarão normalmente</p>
        </div>

        <a href="/arquivos/littlefs" class="btn btn-success">📂 Acessar Gerenciador de Arquivos</a>
        
        <div style="margin-top: 30px; padding-top: 20px; border-top: 1px solid #ddd;">
            <p style="color: #666; font-size: 14px;">
                <strong>Arquivos necessários:</strong><br>
                web_dashboard.html, web_pins.html, web_actions.html, web_config_gerais.html,<br>
                web_files.html, web_mqtt.html, web_reset.html, web_preferencias.html,<br>
                web_firmware.html, web_serial.html, web_intermod.html
            </p>
        </div>
    </div>
</body>
</html>
)rawliteral";

#endif
