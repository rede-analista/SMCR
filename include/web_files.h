// Conteúdo de web_files.h
#ifndef WEB_FILES_H
#define WEB_FILES_H

#include <Arduino.h>

// Página de Gerenciamento de Arquivos
const char web_files_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Arquivos</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 0; 
            padding: 20px; 
            background: #f0f0f0; 
        }
        .container { 
            max-width: 1000px; 
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
        .section { 
            margin: 30px 0; 
        }
        .section-title { 
            font-size: 18px; 
            font-weight: bold; 
            color: #444; 
            margin-bottom: 15px; 
            padding: 8px 0; 
            border-bottom: 1px solid #ddd; 
        }
        .config-table { 
            width: 100%; 
            border-collapse: collapse; 
            margin-bottom: 20px; 
            font-size: 13px;
        }
        .config-table th,
        .config-table td { 
            padding: 8px; 
            border: 1px solid #ddd; 
            text-align: left;
        }
        .config-table th {
            background: #f8f9fa;
            font-weight: bold;
            color: #555;
        }
        .config-table tr:nth-child(even) {
            background: #f9f9f9;
        }
        .btn { 
            margin: 5px; 
            padding: 10px 15px; 
            border: none; 
            border-radius: 4px; 
            cursor: pointer; 
            font-size: 14px; 
            font-weight: bold; 
            text-decoration: none; 
            display: inline-block; 
        }
        .btn-primary { background: #007bff; color: white; }
        .btn-primary:hover { background: #0056b3; }
        .btn-success { background: #28a745; color: white; }
        .btn-success:hover { background: #1e7e34; }
        .btn-warning { background: #ffc107; color: #212529; }
        .btn-warning:hover { background: #e0a800; }
        .file-list {
            background: #f8f9fa;
            border: 1px solid #dee2e6;
            border-radius: 4px;
            padding: 15px;
            margin: 10px 0;
        }
        .file-item {
            padding: 10px;
            margin: 5px 0;
            background: white;
            border: 1px solid #ddd;
            border-radius: 4px;
            display: flex;
            justify-content: space-between;
            align-items: center;
        }
        .upload-area {
            border: 2px dashed #007bff;
            border-radius: 4px;
            padding: 30px;
            text-align: center;
            background: #f8f9fa;
            margin: 20px 0;
        }
        input[type="file"] {
            margin: 10px 0;
        }
        .loading-overlay {
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(255, 255, 255, 0.9);
            display: none;
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
        .info-box {
            background: #e7f3ff;
            border-left: 4px solid #2196f3;
            padding: 15px;
            margin: 20px 0;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div id="loadingOverlay" class="loading-overlay">
        <div class="loading-spinner"></div>
        <div class="loading-text">Carregando...</div>
    </div>

    <div class="container">
        <h1>SMCR - Gerenciamento de Arquivos</h1>
        
        <div class="menu">
            <a href="/">Status</a>
            <a href="/configuracao">Configurações Gerais</a>
            <a href="/pins">Pinos/Relés</a>
            <a href="/mqtt">MQTT/Serviços</a>
            <a href="/arquivos">Arquivos</a>
            <a href="/reset">Reset</a>
        </div>

        <!-- Preferências NVS -->
        <div class="section">
            <div class="section-title">📋 Preferências NVS (Non-Volatile Storage)</div>
            <div class="info-box">
                <strong>ℹ️ Informação:</strong> Estes são os parâmetros salvos na memória não-volátil do ESP32.
            </div>
            <button onclick="loadNVSPreferences()" class="btn btn-primary">🔄 Recarregar Preferências</button>
            <div id="nvs-content">
                <table class="config-table" id="nvs-table">
                    <thead>
                        <tr>
                            <th>Parâmetro</th>
                            <th>Valor</th>
                            <th>Tipo</th>
                        </tr>
                    </thead>
                    <tbody id="nvs-tbody">
                        <tr><td colspan="3" style="text-align: center; color: #666;">Carregando...</td></tr>
                    </tbody>
                </table>
            </div>
        </div>

        <!-- Arquivos LittleFS -->
        <div class="section">
            <div class="section-title">📁 Arquivos LittleFS</div>
            <div class="info-box">
                <strong>ℹ️ Informação:</strong> Arquivos armazenados no sistema de arquivos da flash.
            </div>
            <button onclick="loadFilesList()" class="btn btn-primary">🔄 Recarregar Lista</button>
            <button onclick="formatFlash()" class="btn btn-warning" style="margin-left: 10px;">⚠️ Formatar Flash</button>
            <div id="files-list" class="file-list">
                <p style="text-align: center; color: #666;">Carregando arquivos...</p>
            </div>
        </div>

        <!-- Upload de Arquivos -->
        <div class="section">
            <div class="section-title">📤 Upload de Arquivos</div>
            <div class="upload-area">
                <h3>Enviar arquivo para o ESP32</h3>
                <p>Selecione um arquivo para fazer upload para a flash</p>
                <form id="upload-form" enctype="multipart/form-data">
                    <input type="file" id="file-input" name="file" required>
                    <br>
                    <button type="submit" class="btn btn-success">📤 Fazer Upload</button>
                </form>
            </div>
        </div>

        <div id="status" class="status-message"></div>
    </div>

    <script>
        function showLoading(text = 'Carregando...') {
            document.getElementById('loadingOverlay').style.display = 'flex';
            document.querySelector('.loading-text').textContent = text;
        }
        
        function hideLoading() {
            document.getElementById('loadingOverlay').style.display = 'none';
        }
        
        window.addEventListener('load', function() {
            hideLoading();
            loadNVSPreferences();
            loadFilesList();
        });

        function loadNVSPreferences() {
            showLoading('Carregando preferências NVS...');
            fetch('/api/nvs/list')
                .then(response => response.json())
                .then(data => {
                    hideLoading();
                    const tbody = document.getElementById('nvs-tbody');
                    tbody.innerHTML = '';
                    
                    if (data.preferences && data.preferences.length > 0) {
                        data.preferences.forEach(pref => {
                            const row = tbody.insertRow();
                            row.innerHTML = `
                                <td><strong>${pref.key}</strong></td>
                                <td>${escapeHtml(pref.value)}</td>
                                <td><span style="background: #e3f2fd; padding: 2px 6px; border-radius: 3px; font-size: 11px;">${pref.type}</span></td>
                            `;
                        });
                    } else {
                        tbody.innerHTML = '<tr><td colspan="3" style="text-align: center; color: #666;">Nenhuma preferência encontrada</td></tr>';
                    }
                })
                .catch(error => {
                    hideLoading();
                    showStatus('Erro ao carregar preferências NVS', 'error');
                    console.error('Erro:', error);
                });
        }

        function loadFilesList() {
            fetch('/api/files/list')
                .then(response => response.json())
                .then(data => {
                    const filesList = document.getElementById('files-list');
                    filesList.innerHTML = '';
                    
                    if (data.files && data.files.length > 0) {
                        data.files.forEach(file => {
                            const fileDiv = document.createElement('div');
                            fileDiv.className = 'file-item';
                            fileDiv.innerHTML = `
                                <div>
                                    <strong>${file.name}</strong>
                                    <span style="margin-left: 10px; color: #666; font-size: 12px;">${formatBytes(file.size)}</span>
                                </div>
                                <div>
                                    <button onclick="downloadFile('${file.name}')" class="btn btn-primary" style="padding: 5px 10px; font-size: 12px;">📥 Download</button>
                                    <button onclick="deleteFile('${file.name}')" class="btn btn-warning" style="padding: 5px 10px; font-size: 12px;">🗑️ Deletar</button>
                                </div>
                            `;
                            filesList.appendChild(fileDiv);
                        });
                        
                        // Adicionar informações de espaço
                        const infoDiv = document.createElement('div');
                        infoDiv.style.cssText = 'margin-top: 15px; padding: 10px; background: white; border-radius: 4px; font-size: 12px;';
                        infoDiv.innerHTML = `
                            <strong>Espaço:</strong> 
                            Usado: ${formatBytes(data.usedBytes)} | 
                            Total: ${formatBytes(data.totalBytes)} | 
                            Livre: ${formatBytes(data.freeBytes)}
                        `;
                        filesList.appendChild(infoDiv);
                    } else {
                        filesList.innerHTML = '<p style="text-align: center; color: #666;">Nenhum arquivo encontrado</p>';
                    }
                })
                .catch(error => {
                    showStatus('Erro ao carregar lista de arquivos', 'error');
                    console.error('Erro:', error);
                });
        }

        function downloadFile(filename) {
            window.location.href = `/api/files/download?filename=${encodeURIComponent(filename)}`;
        }

        function deleteFile(filename) {
            if (confirm(`Tem certeza que deseja deletar o arquivo "${filename}"?`)) {
                const formData = new FormData();
                formData.append('filename', filename);
                
                fetch('/api/files/delete', { 
                    method: 'POST',
                    body: formData
                })
                    .then(response => response.json())
                    .then(result => {
                        if (result.success) {
                            showStatus('Arquivo deletado com sucesso!', 'success');
                            loadFilesList();
                        } else {
                            showStatus(result.message || 'Erro ao deletar arquivo', 'error');
                        }
                    })
                    .catch(error => {
                        showStatus('Erro de comunicação', 'error');
                        console.error('Erro:', error);
                    });
            }
        }

        function formatFlash() {
            if (confirm('⚠️ ATENÇÃO: Esta ação irá APAGAR TODOS OS ARQUIVOS da flash LittleFS!\n\nIsso inclui:\n- Configurações de pinos\n- Arquivos enviados\n- Todos os dados armazenados\n\nEsta ação é IRREVERSÍVEL!\n\nDeseja realmente continuar?')) {
                if (confirm('Confirmação final: Tem ABSOLUTA CERTEZA que deseja formatar a flash e perder todos os dados?')) {
                    showLoading('Formatando flash...');
                    
                    fetch('/api/files/format', { method: 'POST' })
                        .then(response => response.json())
                        .then(result => {
                            hideLoading();
                            if (result.success) {
                                showStatus('Flash formatada com sucesso! Todos os arquivos foram removidos.', 'success');
                                loadFilesList();
                            } else {
                                showStatus(result.message || 'Erro ao formatar flash', 'error');
                            }
                        })
                        .catch(error => {
                            hideLoading();
                            showStatus('Erro de comunicação', 'error');
                            console.error('Erro:', error);
                        });
                }
            }
        }

        document.getElementById('upload-form').addEventListener('submit', function(e) {
            e.preventDefault();
            
            const formData = new FormData(this);
            const fileInput = document.getElementById('file-input');
            
            if (!fileInput.files.length) {
                showStatus('Selecione um arquivo', 'error');
                return;
            }
            
            showLoading('Fazendo upload...');
            
            fetch('/api/files/upload', {
                method: 'POST',
                body: formData
            })
            .then(response => response.json())
            .then(result => {
                hideLoading();
                if (result.success) {
                    showStatus('Upload realizado com sucesso!', 'success');
                    fileInput.value = '';
                    loadFilesList();
                } else {
                    showStatus('Erro no upload: ' + (result.message || 'Erro desconhecido'), 'error');
                }
            })
            .catch(error => {
                hideLoading();
                showStatus('Erro de comunicação', 'error');
            });
        });

        function formatBytes(bytes) {
            if (bytes === 0) return '0 Bytes';
            const k = 1024;
            const sizes = ['Bytes', 'KB', 'MB', 'GB'];
            const i = Math.floor(Math.log(bytes) / Math.log(k));
            return Math.round(bytes / Math.pow(k, i) * 100) / 100 + ' ' + sizes[i];
        }

        function escapeHtml(text) {
            const div = document.createElement('div');
            div.textContent = text;
            return div.innerHTML;
        }

        function showStatus(message, type) {
            const statusDiv = document.getElementById('status');
            statusDiv.textContent = message;
            statusDiv.className = 'status-message status-' + type;
            statusDiv.style.display = 'block';
            
            setTimeout(() => {
                statusDiv.style.display = 'none';
            }, 5000);
        }
    </script>
</body>
</html>
)rawliteral";

#endif
