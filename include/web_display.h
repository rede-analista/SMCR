// Conteúdo de web_display.h
#ifndef WEB_DISPLAY_H
#define WEB_DISPLAY_H

#include <Arduino.h>

// Página Display - Servida de LittleFS (data/web_display.html)
const char web_display_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Display</title>
    <style>
        * { box-sizing: border-box; }
        body { font-family: Arial, sans-serif; margin: 0; padding: 16px; background: #f0f0f0; }
        .container { max-width: 600px; margin: 0 auto; background: white; padding: 16px 20px; border-radius: 8px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }
        .header { display: flex; align-items: center; justify-content: space-between; margin-bottom: 16px; padding-bottom: 10px; border-bottom: 2px solid #007bff; }
        h1 { margin: 0; color: #333; font-size: 18px; font-weight: bold; }
        .btn-back { text-decoration: none; color: #333; padding: 7px 14px; border-radius: 6px; border: 1px solid #dee2e6; background: #fff; font-size: 13px; }
        .pin-item { padding: 10px 14px; margin: 5px 0; border-radius: 6px; display: flex; align-items: center; gap: 12px; border: 1px solid #e9ecef; }
        .pin-indicator { width: 12px; height: 12px; border-radius: 50%; flex-shrink: 0; }
        .pin-description { font-size: 14px; color: #222; }
        .footer { text-align: center; font-size: 11px; color: #aaa; margin-top: 14px; }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>Display &mdash; [<span id="title-hostname">-</span>]</h1>
            <a href="/" class="btn-back">&#8592; Voltar</a>
        </div>
        <div id="pins-list"><p style="text-align:center;color:#666;">Carregando...</p></div>
        <div class="footer">Atualiza a cada <span id="refresh-seconds">15</span>s</div>
    </div>
    <script>
        let refreshTimer = null, currentInterval = 15000;
        const PIN_TYPE_ANALOG = 192, PIN_TYPE_PWM = 193, PIN_TYPE_REMOTE_ANALOG = 65533;
        function isAlerta(pin) {
            const tipo = pin.tipo || 0, val = pin.status_atual !== undefined ? pin.status_atual : 0;
            if (tipo === PIN_TYPE_ANALOG || tipo === PIN_TYPE_PWM || tipo === PIN_TYPE_REMOTE_ANALOG)
                return val >= (pin.nivel_acionamento_min || 0) && val <= (pin.nivel_acionamento_max || 4095);
            return val === (pin.nivel_acionamento_min || 0);
        }
        function updateDisplay() {
            fetch('/status/json').then(r => r.json()).then(data => {
                const sys = data.system || {}, pins = (data.pins || []).filter(p => p.exibir_display === true);
                const hostname = sys.hostname || '-', corAlerta = sys.cor_com_alerta || '#dc3545';
                const corOk = sys.cor_sem_alerta || '#28a745', tempoRefresh = sys.tempo_refresh || 15;
                document.getElementById('title-hostname').textContent = hostname;
                document.title = 'SMCR - Display [' + hostname + ']';
                document.getElementById('refresh-seconds').textContent = tempoRefresh;
                const novoInterval = tempoRefresh * 1000;
                if (novoInterval !== currentInterval) { currentInterval = novoInterval; if (refreshTimer) clearInterval(refreshTimer); refreshTimer = setInterval(updateDisplay, currentInterval); }
                const list = document.getElementById('pins-list');
                if (!pins.length) { list.innerHTML = '<p style="text-align:center;color:#666;">Nenhum pino configurado para exibição no Display</p>'; return; }
                list.innerHTML = '';
                pins.forEach(pin => {
                    const cor = isAlerta(pin) ? corAlerta : corOk, desc = pin.description || 'Pino';
                    const item = document.createElement('div'); item.className = 'pin-item';
                    item.innerHTML = '<div class="pin-indicator" style="background:' + cor + ';box-shadow:0 0 4px ' + cor + '88;"></div><span class="pin-description">' + desc + '</span>';
                    list.appendChild(item);
                });
            }).catch(() => {});
        }
        updateDisplay(); refreshTimer = setInterval(updateDisplay, currentInterval);
    </script>
</body>
</html>
)rawliteral";

#endif
