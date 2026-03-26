// Conteúdo de web_modoap.h
#ifndef WEB_MODOAP_H
#define WEB_MODOAP_H

#include <Arduino.h>

// Rota de configuração inicial (HTML para a primeira execução)
// Variável para armazenar o conteúdo do HTML da página de configuração.
const char web_config_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuracao Inicial SMCR</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            background: #f0f0f0; 
            margin: 0; 
            padding: 20px; 
            display: flex; 
            justify-content: center; 
            align-items: center; 
            min-height: 100vh; 
        }
        .config-container { 
            background: white; 
            padding: 30px; 
            border-radius: 8px; 
            box-shadow: 0 2px 10px rgba(0,0,0,0.1); 
            width: 100%; 
            max-width: 500px; 
        }
        h1 { 
            text-align: center; 
            color: #333; 
            margin-bottom: 10px; 
            font-size: 24px; 
        }
        .subtitle { 
            text-align: center; 
            color: #666; 
            margin-bottom: 30px; 
            font-size: 14px; 
        }
        .config-table { 
            width: 100%; 
            border-collapse: collapse; 
            margin-bottom: 20px; 
        }
        .config-table td { 
            padding: 10px; 
            vertical-align: middle; 
        }
        .config-table td:first-child { 
            font-weight: bold; 
            color: #444; 
            width: 40%; 
            text-align: right; 
            padding-right: 15px; 
        }
        .config-table input { 
            width: 100%; 
            padding: 8px 12px; 
            border: 2px solid #ddd; 
            border-radius: 4px; 
            font-size: 14px; 
            box-sizing: border-box; 
        }
        .config-table input:focus { 
            outline: none; 
            border-color: #007bff; 
            box-shadow: 0 0 5px rgba(0,123,255,0.3); 
        }
        .btn-save { 
            width: 100%; 
            padding: 12px; 
            background: #007bff; 
            color: white; 
            border: none; 
            border-radius: 4px; 
            font-size: 16px; 
            font-weight: bold; 
            cursor: pointer; 
            margin-top: 10px; 
        }
        .btn-save:hover { 
            background: #0056b3; 
        }
        .status-message { 
            margin-top: 15px; 
            padding: 10px; 
            border-radius: 4px; 
            text-align: center; 
            display: none; 
        }
        .status-success { 
            background: #d4edda; 
            color: #155724; 
            border: 1px solid #c3e6cb; 
        }
        .status-error { 
            background: #f8d7da; 
            color: #721c24; 
            border: 1px solid #f5c6cb; 
        }
    </style>
</head>
<body>
    <div class="config-container">
        <h1>Configuracao Inicial</h1>
        <p class="subtitle">Insira as informacoes da rede Wi-Fi para o dispositivo se conectar:</p>
        
        <form id="configForm" action="/save_config" method="POST">
            <table class="config-table">
                <tr>
                    <td>Hostname:</td>
                    <td><input type="text" name="hostname" id="hostname" placeholder="esp32modularx" maxlength="32" required></td>
                </tr>
                <tr>
                    <td>SSID da Rede:</td>
                    <td><input type="text" name="ssid" id="ssid" placeholder="Nome da sua rede Wi-Fi" maxlength="32" required></td>
                </tr>
                <tr>
                    <td>Senha da Rede:</td>
                    <td><input type="password" name="password" id="password" placeholder="Senha do Wi-Fi" maxlength="64" required></td>
                </tr>
            </table>
            
            <button type="submit" class="btn-save">Salvar e Conectar</button>
        </form>
        
        <div id="status" class="status-message"></div>
    </div>

    <script>
        document.getElementById('configForm').addEventListener('submit', async function(e) {
            e.preventDefault();
            
            const statusDiv = document.getElementById('status');
            const submitBtn = document.querySelector('.btn-save');
            
            const hostname = document.getElementById('hostname').value.trim();
            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value;
            
            if (!hostname || !ssid || !password) {
                statusDiv.className = 'status-message status-error';
                statusDiv.style.display = 'block';
                statusDiv.textContent = 'Preencha todos os campos obrigatorios';
                return;
            }
            
            submitBtn.textContent = 'Salvando...';
            submitBtn.disabled = true;
            
            try {
                const formData = new FormData(this);
                const response = await fetch(this.action, {
                    method: 'POST',
                    body: formData
                });
                
                if (response.ok) {
                    statusDiv.className = 'status-message status-success';
                    statusDiv.style.display = 'block';
                    statusDiv.textContent = 'Configuracao salva com sucesso! Reiniciando...';
                    
                    setTimeout(() => {
                        window.location.href = '/';
                    }, 3000);
                } else {
                    throw new Error('Erro ao salvar configuracao');
                }
            } catch (error) {
                statusDiv.className = 'status-message status-error';
                statusDiv.style.display = 'block';
                statusDiv.textContent = 'Erro ao salvar configuracao. Tente novamente.';
                
                submitBtn.textContent = 'Salvar e Conectar';
                submitBtn.disabled = false;
            }
        });
    </script>
</body>
</html>
)rawliteral";

// Página de setup inicial: baixar arquivos HTML da SMCR Cloud
const char web_setup_files_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Setup - SMCR</title>
    <style>
        body { font-family: Arial, sans-serif; background: #f0f0f0; margin: 0; padding: 20px;
               display: flex; justify-content: center; align-items: center; min-height: 100vh; }
        .box { background: white; padding: 30px; border-radius: 8px;
               box-shadow: 0 2px 10px rgba(0,0,0,0.1); width: 100%; max-width: 500px; }
        h1 { text-align: center; color: #333; font-size: 22px; margin-bottom: 6px; }
        .sub { text-align: center; color: #28a745; font-size: 14px; margin-bottom: 24px; }
        label { display: block; font-weight: bold; color: #444; margin-bottom: 6px; }
        input[type=text] { width: 100%; padding: 9px 12px; border: 2px solid #ddd;
                           border-radius: 4px; font-size: 14px; box-sizing: border-box; margin-bottom: 16px; }
        input:focus { outline: none; border-color: #007bff; }
        .btn { width: 100%; padding: 12px; background: #007bff; color: white; border: none;
               border-radius: 4px; font-size: 16px; font-weight: bold; cursor: pointer; }
        .btn:hover { background: #0056b3; }
        .btn:disabled { background: #999; cursor: not-allowed; }
        .status { margin-top: 16px; padding: 12px; border-radius: 4px; text-align: center;
                  font-size: 14px; display: none; }
        .info  { background: #d1ecf1; color: #0c5460; border: 1px solid #bee5eb; }
        .ok    { background: #d4edda; color: #155724; border: 1px solid #c3e6cb; }
        .err   { background: #f8d7da; color: #721c24; border: 1px solid #f5c6cb; }
        .skip  { display: block; text-align: center; margin-top: 14px; font-size: 13px;
                 color: #888; text-decoration: none; }
        .skip:hover { color: #555; }
    </style>
</head>
<body>
<div class="box">
    <h1>Setup Inicial - SMCR</h1>
    <p class="sub">✔ Wi-Fi conectado com sucesso!</p>

    <label for="cloudUrl">URL do servidor SMCR Cloud:</label>
    <input type="text" id="cloudUrl" value="smcr.pensenet.com.br"
           placeholder="smcr.pensenet.com.br">

    <button class="btn" id="btnFetch" onclick="fetchFiles()">
        ☁ Baixar arquivos HTML do servidor cloud
    </button>

    <div class="status info" id="statusBox" style="display:none;"></div>

    <a href="/arquivos/littlefs" class="skip">Pular este passo (fazer upload manual dos arquivos)</a>
</div>
<script>
    let polling = null;

    async function fetchFiles() {
        const url = document.getElementById('cloudUrl').value.trim();
        if (!url) { showStatus('Informe a URL do servidor cloud.', 'err'); return; }

        const btn = document.getElementById('btnFetch');
        btn.disabled = true;
        btn.textContent = 'Iniciando...';
        showStatus('Conectando ao servidor cloud...', 'info');

        try {
            const r = await fetch('/api/fetch_cloud_files', {
                method: 'POST',
                headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
                body: 'cloud_url=' + encodeURIComponent(url)
            });
            const d = await r.json();
            if (!d.ok) { showStatus('Erro: ' + d.error, 'err'); btn.disabled = false; btn.textContent = '☁ Baixar arquivos HTML do servidor cloud'; return; }
            startPolling();
        } catch(e) {
            showStatus('Erro de comunicação: ' + e.message, 'err');
            btn.disabled = false;
            btn.textContent = '☁ Baixar arquivos HTML do servidor cloud';
        }
    }

    function startPolling() {
        polling = setInterval(async () => {
            try {
                const r = await fetch('/api/fetch_cloud_files_status');
                const d = await r.json();
                showStatus(d.status, d.done ? (d.error ? 'err' : 'ok') : 'info');
                if (d.done) {
                    clearInterval(polling);
                    if (!d.error) {
                        showStatus(d.status + '<br><strong>Redirecionando para o dashboard...</strong>', 'ok');
                        setTimeout(() => window.location.href = '/', 2000);
                    } else {
                        document.getElementById('btnFetch').disabled = false;
                        document.getElementById('btnFetch').textContent = '☁ Baixar arquivos HTML do servidor cloud';
                    }
                }
            } catch(e) { /* ignora falha pontual de polling */ }
        }, 800);
    }

    function showStatus(msg, type) {
        const el = document.getElementById('statusBox');
        el.innerHTML = msg;
        el.className = 'status ' + type;
        el.style.display = 'block';
    }
</script>
</body>
</html>
)rawliteral";

#endif