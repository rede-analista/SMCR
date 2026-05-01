// Conteúdo de web_dashboard.h
#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

// Dashboard principal - Servido de LittleFS (data/web_dashboard.html)
// Fallback básico com botão para buscar arquivos do cloud
const char web_dashboard_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Configuracao Inicial</title>
    <style>
        *{box-sizing:border-box;margin:0;padding:0}
        body{font-family:Arial,sans-serif;background:linear-gradient(135deg,#667eea,#764ba2);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}
        .card{background:#fff;border-radius:12px;box-shadow:0 10px 40px rgba(0,0,0,.3);padding:36px;max-width:560px;width:100%;text-align:center}
        h1{color:#333;font-size:24px;margin-bottom:6px}
        .sub{color:#666;font-size:14px;margin-bottom:24px}
        .warn{background:#fff3cd;border:2px solid #ffc107;color:#856404;padding:16px;border-radius:8px;margin-bottom:20px;text-align:left}
        .warn b{display:block;margin-bottom:6px}
        label{display:block;text-align:left;font-size:13px;font-weight:bold;color:#444;margin-bottom:4px}
        input[type=text]{width:100%;padding:10px;border:1px solid #ccc;border-radius:6px;font-size:14px;margin-bottom:14px}
        .row{display:flex;gap:10px}
        .row input{margin-bottom:14px}
        .col2{flex:0 0 110px}
        .col1{flex:1}
        .btn{display:block;width:100%;padding:13px;border:none;border-radius:6px;font-size:15px;font-weight:bold;cursor:pointer;margin-bottom:10px}
        .btn-primary{background:#007bff;color:#fff}
        .btn-primary:hover{background:#0056b3}
        .btn-secondary{background:#6c757d;color:#fff;text-decoration:none;display:inline-block;width:auto;padding:10px 20px}
        .btn-secondary:hover{background:#545b62}
        #status{margin-top:12px;padding:12px;border-radius:6px;font-size:14px;display:none}
        .ok{background:#d4edda;color:#155724}
        .err{background:#f8d7da;color:#721c24}
        .loading{background:#d1ecf1;color:#0c5460}
        .divider{border-top:1px solid #eee;margin:20px 0;padding-top:16px}
    </style>
</head>
<body>
<div class="card">
    <h1>SMCR - Sistema Modular</h1>
    <p class="sub">Configuracao inicial do dispositivo</p>

    <div class="warn">
        <b>Paginas HTML nao encontradas no sistema de arquivos.</b>
        Se o Cloud/HA ja esta configurado, clique em "Buscar Arquivos" para baixa-las automaticamente.
    </div>

    <label>URL do Cloud / HA</label>
    <input type="text" id="cloud_url" placeholder="smcr.pensenet.com.br">

    <div class="row">
        <div class="col1">
            <label>Porta</label>
            <input type="text" id="cloud_port" placeholder="80">
        </div>
        <div class="col2">
            <label>HTTPS</label>
            <select id="cloud_https" style="width:100%;padding:10px;border:1px solid #ccc;border-radius:6px;font-size:14px;margin-bottom:14px">
                <option value="0">Nao</option>
                <option value="1">Sim</option>
            </select>
        </div>
    </div>

    <button class="btn btn-primary" onclick="buscarArquivos()">Buscar Arquivos do Cloud</button>
    <div id="status"></div>

    <div class="divider">
        <a href="/arquivos/littlefs" class="btn btn-secondary">Gerenciador de Arquivos (upload manual)</a>
    </div>
</div>
<script>
function showStatus(msg, type) {
    var s = document.getElementById('status');
    s.textContent = msg;
    s.className = type;
    s.style.display = 'block';
}
function buscarArquivos() {
    var url = document.getElementById('cloud_url').value.trim();
    var port = document.getElementById('cloud_port').value.trim();
    var https = document.getElementById('cloud_https').value;
    if (!url) { showStatus('Informe a URL do Cloud.', 'err'); return; }
    showStatus('Buscando arquivos... aguarde.', 'loading');
    fetch('/start-fetch-cloud-files?cloud_url=' + encodeURIComponent(url) + '&cloud_port=' + encodeURIComponent(port) + '&cloud_https=' + https)
        .then(function(r){ return r.text(); })
        .then(function(){ pollStatus(); })
        .catch(function(){ showStatus('Erro ao iniciar download.', 'err'); });
}
function pollStatus() {
    fetch('/fetch-cloud-files-status')
        .then(function(r){ return r.json(); })
        .then(function(d){
            if (d.done) {
                if (d.error) { showStatus('Erro: ' + d.status, 'err'); }
                else { showStatus('Concluido! Recarregando...', 'ok'); setTimeout(function(){ location.reload(); }, 2000); }
            } else {
                showStatus(d.status || 'Aguardando...', 'loading');
                setTimeout(pollStatus, 2000);
            }
        })
        .catch(function(){ setTimeout(pollStatus, 3000); });
}
// Preenche URL do cloud salvo no dispositivo
fetch('/config/json').then(function(r){ return r.json(); }).then(function(d){
    if (d.cloud_url) document.getElementById('cloud_url').value = d.cloud_url;
    if (d.cloud_port) document.getElementById('cloud_port').value = d.cloud_port;
    if (d.cloud_use_https !== undefined) document.getElementById('cloud_https').value = d.cloud_use_https ? '1' : '0';
}).catch(function(){});
</script>
</body>
</html>
)rawliteral";

#endif
