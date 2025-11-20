// Conteúdo de web_pages.h
#ifndef WEB_MODOAP_H
#define WEB_MODOAP_H

#include <Arduino.h>

// Rota de configuração inicial (HTML para a primeira execução)
// Variável para armazenar o conteúdo do HTML da página de configuração.
const char web_config_html[] PROGMEM = R"raw_string(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Configuração SMCR</title>
    <!-- Incluindo Tailwind CSS para estilização moderna e responsiva -->
    <script src="https://cdn.tailwindcss.com"></script>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;700&display=swap" rel="stylesheet">
    <style>
        body {
            font-family: 'Inter', sans-serif;
            background-color: #f3f4f6;
            color: #1f2937;
            display: flex;
            justify-content: center;
            align-items: center;
            min-height: 100vh;
            padding: 1rem;
        }
        .container {
            background-color: #ffffff;
            border-radius: 1rem;
            padding: 2rem;
            box-shadow: 0 10px 15px -3px rgba(0, 0, 0, 0.1), 0 4px 6px -2px rgba(0, 0, 0, 0.05);
            width: 100%;
            max-width: 28rem;
        }
        h1 {
            text-align: center;
            font-size: 1.5rem;
            font-weight: 700;
            margin-bottom: 1.5rem;
        }
        p {
            text-align: center;
            font-size: 1rem;
            color: #6b7280;
            margin-bottom: 2rem;
        }
        .input-group {
            margin-bottom: 1rem;
        }
        .input-group label {
            display: block;
            font-weight: 500;
            margin-bottom: 0.5rem;
        }
        .input-group input {
            width: 100%;
            padding: 0.75rem;
            border: 1px solid #d1d5db;
            border-radius: 0.5rem;
            box-sizing: border-box;
            background-color: #f9fafb;
        }
        .input-group input:focus {
            outline: none;
            border-color: #2563eb;
            box-shadow: 0 0 0 3px rgba(37, 99, 235, 0.2);
        }
        button {
            width: 100%;
            padding: 0.75rem;
            background-color: #2563eb;
            color: #ffffff;
            font-weight: 700;
            border: none;
            border-radius: 0.5rem;
            cursor: pointer;
            transition: background-color 0.2s ease;
        }
        button:hover {
            background-color: #1d4ed8;
        }
        .status-message {
            margin-top: 1.5rem;
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
    </style>
</head>
<body>
    <div class="container">
        <h1>Configuração Inicial</h1>
        <p>Insira as informações da rede Wi-Fi para o dispositivo se conectar.</p>
        <form id="configForm" action="/save_config" method="POST">
            <div class="input-group">
                <label for="hostname">Hostname</label>
                <input type="text" id="hostname" name="hostname" required>
            </div>
            <div class="input-group">
                <label for="ssid">SSID da Rede</label>
                <input type="text" id="ssid" name="ssid" required>
            </div>
            <div class="input-group">
                <label for="password">Senha da Rede</label>
                <input type="password" id="password" name="password" required>
            </div>
            <button type="submit">Salvar e Conectar</button>
        </form>
        <div id="status" class="status-message"></div>
    </div>

    <script>
        document.getElementById('configForm').addEventListener('submit', async function(event) {
            event.preventDefault();
            const form = event.target;
            const statusDiv = document.getElementById('status');
            const button = form.querySelector('button');

            // Converte os dados do formulário para o formato URL-encoded para envio via POST
            const formData = new URLSearchParams(new FormData(form)).toString();

            statusDiv.style.display = 'block';
            statusDiv.className = 'status-message';
            statusDiv.textContent = 'Salvando configurações...';
            button.disabled = true;

            try {
                // A requisição fetch usará automaticamente o método POST e a URL de action="/save_config"
                const response = await fetch('/save_config', { // CORREÇÃO AQUI: Garante que o fetch va para /save_config
                    method: 'POST', 
                    headers: {
                        'Content-Type': 'application/x-www-form-urlencoded'
                    },
                    body: formData // Envia os dados no corpo
                });

                if (response.ok) {
                    statusDiv.className = 'status-message status-success';
                    statusDiv.textContent = 'Configurações salvas! O dispositivo está reiniciando para se conectar à nova rede.';
                    // O ESP32 irá reiniciar (delay(3000) e ESP.restart() em fV_handleSaveConfig)
                } else {
                    const errorText = await response.text();
                    statusDiv.className = 'status-message status-error';
                    statusDiv.textContent = \`Erro ao salvar: ${errorText || response.statusText}\`;
                    button.disabled = false;
                }
            } catch (error) {
                statusDiv.className = 'status-message status-error';
                statusDiv.textContent = \`Erro de conexão: ${error.message}\`;
                button.disabled = false;
            }
        });
    </script>
</body>
</html>
)raw_string";

#endif // WEB_WEB_MODOAP_H