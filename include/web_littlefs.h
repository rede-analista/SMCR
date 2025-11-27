#ifndef WEB_LITTLEFS_H
#define WEB_LITTLEFS_H

#include <Arduino.h>

// Página de Arquivos LittleFS
const char web_littlefs_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="pt-BR">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SMCR - Arquivos LittleFS</title>
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
        <h1>SMCR - Arquivos LittleFS</h1>
        
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

        <!-- Arquivos LittleFS -->
        <div class="section">
            <div class="section-title">📁 Arquivos LittleFS</div>
            <div class="info-box">
                <strong>ℹ️ Informação:</strong> Arquivos armazenados no sistema de arquivos da flash.
            </div>
            <button onclick="loadFilesList()" class="btn btn-primary">🔄 Recarregar Lista</button>
            <button onclick="formatFlash()" class="btn btn-warning" style="margin-left: 10px;">⚠️ Formatar Flash</button>
            <div id="files-list" class="file-list" style="background: #f8f9fa;border: 1px solid #dee2e6;border-radius: 4px;padding: 15px;margin: 10px 0;">
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
            loadFilesList();
        });

        function loadFilesList() {
            fetch('/api/files/list')
                .then(response => response.json())
                .then(data => {
                    const filesList = document.getElementById('files-list');
                    filesList.innerHTML = '';
                    
                    if (data.files && data.files.length > 0) {
                        data.files.forEach(file => {
                            const fileDiv = document.createElement('div');
                            fileDiv.style.cssText = 'padding:10px;margin:5px 0;background:white;border:1px solid #ddd;border-radius:4px;display:flex;justify-content:space-between;align-items:center;';
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

        // === SISTEMA DE MONITORAMENTO DE PERFORMANCE ===
        const pageStartTime = performance.now();
        
        // Função para mostrar indicador de performance
        function showPerformanceIndicator() {
            const loadTime = performance.now() - pageStartTime;
            const indicator = document.createElement('div');
            indicator.style.cssText = 'position: fixed; bottom: 10px; right: 10px; background: rgba(0,0,0,0.7); color: white; padding: 8px 12px; border-radius: 6px; font-size: 12px; font-family: monospace;';
            indicator.innerHTML = `Tempo de Carregamento: ${loadTime.toFixed(2)}ms`;
            document.body.appendChild(indicator);
            
            console.log(`[PERFORMANCE] LittleFS carregado em ${loadTime.toFixed(2)}ms`);
        }
        
        // Mostrar indicador após carregamento
        window.addEventListener('load', function() {
            setTimeout(showPerformanceIndicator, 100);
        });
    </script>
</body>
</html>
)rawliteral";

#endif
