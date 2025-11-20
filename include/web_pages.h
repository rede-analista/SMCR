// Conteúdo de web_pages.h
#ifndef WEB_PAGES_H
#define WEB_PAGES_H


#include <Arduino.h>

// Rota de configuração inicial (HTML simples)
const char web_config_html[] PROGMEM = R"raw_string(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR Configuração Inicial</title>
    <link href="https://fonts.googleapis.com/css2?family=Inter:wght@400;500;700&display=swap" rel="stylesheet">
    <script src="https://cdn.tailwindcss.com"></script>
    <style>
        body { font-family: 'Inter', sans-serif; }
    </style>
</head>
<body class="bg-gray-100 text-gray-800 flex items-center justify-center min-h-screen p-4">
    <div class="bg-white rounded-2xl shadow-xl p-8 w-full max-w-sm">
        <h1 class="text-2xl font-bold text-center mb-2">Configuração Inicial</h1>
        <p class="text-sm text-gray-500 text-center mb-6">Insira as informações da rede Wi-Fi para o dispositivo se conectar.</p>
        <form id="wifiForm" class="flex flex-col gap-4">
            <div class="flex flex-col">
                <label for="hostname" class="font-medium text-sm mb-1">Hostname (Nome do Dispositivo)</label>
                <input type="text" id="hostname" name="hostname" required value="modular01" class="p-3 border border-gray-300 rounded-lg bg-gray-50 focus:outline-none focus:ring-2 focus:ring-blue-500">
            </div>
            <div class="flex flex-col">
                <label for="ssid" class="font-medium text-sm mb-1">SSID da Rede (Wi-Fi)</label>
                <input type="text" id="ssid" name="ssid" required value="iot24" class="p-3 border border-gray-300 rounded-lg bg-gray-50 focus:outline-none focus:ring-2 focus:ring-blue-500">
            </div>
            <div class="flex flex-col">
                <label for="password" class="font-medium text-sm mb-1">Senha da Rede (Wi-Fi)</label>
                <input type="password" id="password" name="password" required class="p-3 border border-gray-300 rounded-lg bg-gray-50 focus:outline-none focus:ring-2 focus:ring-blue-500">
            </div>
            <button type="submit" class="bg-blue-600 text-white font-bold py-3 px-6 rounded-lg hover:bg-blue-700 transition duration-200 shadow-md">Salvar e Conectar</button>
            <div id="statusMessage" class="status-message hidden mt-4 p-3 rounded-lg text-center font-semibold"></div>
        </form>
    </div>
    <script>
        document.getElementById('wifiForm').addEventListener('submit', async (event) => {
            event.preventDefault();
            const statusMessage = document.getElementById('statusMessage');
            statusMessage.classList.remove('hidden', 'bg-green-100', 'text-green-800', 'bg-red-100', 'text-red-800');
            statusMessage.classList.add('bg-gray-200', 'text-gray-700');
            statusMessage.textContent = 'Salvando configurações e reiniciando o dispositivo...';
            
            const formData = new FormData(event.target);
            const data = Object.fromEntries(formData.entries());
            const params = new URLSearchParams(data).toString();

            try {
                // Rota configurada para receber via GET
                const response = await fetch(`/save_config?${params}`, { method: 'GET' });

                if (response.ok) {
                    statusMessage.classList.remove('bg-gray-200', 'text-gray-700');
                    statusMessage.classList.add('bg-green-100', 'text-green-800');
                    statusMessage.textContent = 'Configurações salvas! O dispositivo está reiniciando para se conectar à nova rede.';
                    setTimeout(() => window.location.reload(), 3000);
                } else {
                    const errorText = await response.text();
                    statusMessage.classList.remove('bg-gray-200', 'text-gray-700');
                    statusMessage.classList.add('bg-red-100', 'text-red-800');
                    statusMessage.textContent = \`Erro: \${errorText}\`;
                }
            } catch (error) {
                statusMessage.classList.remove('bg-gray-200', 'text-gray-700');
                statusMessage.classList.add('bg-red-100', 'text-red-800');
                statusMessage.textContent = \`Erro de conexão: \${error.message}. Verifique o IP do AP.\`;
            }
        });
    </script>
</body>
</html>
)raw_string";

#endif // WEB_PAGES_H