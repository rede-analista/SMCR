# Arquivos (LittleFS)

## Visão Geral
A interface de arquivos (menu: Serviços > Arquivos / LittleFS) permite gerenciar o conteúdo gravado na flash do ESP32 sem necessidade de recompilar o firmware.

## Funcionalidades
- Listagem de arquivos com contagem total (`fileCount`).
- Upload múltiplo sequencial (barra de progresso por arquivo + cancelamento em lote).
- Download múltiplo (abre cada arquivo em sequência, evitando sobrecarga imediata).
- Deleção múltipla sequencial com resumo final de sucesso/erro.
- Formatação completa da flash (apaga todos os arquivos LittleFS). Use com cautela.
- Exportação/Importação de NVS em página dedicada (chaves de configuração persistentes).
- Upload de firmware OTA em página específica.

## Aviso Importante (Operações em Massa)
⚠️ Executar uploads, downloads ou deleções de MUITOS arquivos em sequência pode:
- Aumentar uso de RAM e heap internamente.
- Gerar lentidão no servidor assíncrono.
- Ocasionar travamento temporário ou reinício automático do ESP32.

### Recomendações
1. Agrupe operações em LOTES pequenos (ex: 5–10 arquivos) em vez de enviar/deletar dezenas de uma vez.
2. Aguarde alguns segundos entre lotes maiores para liberar recursos.
3. Evite iniciar novos uploads enquanto downloads ou deleções múltiplas ainda estão ocorrendo.
4. Após um reinício inesperado, confirme integridade dos arquivos críticos (configs de pinos/ações).

## Fluxo de Upload Múltiplo
1. Selecione múltiplos arquivos no campo de upload.
2. Cada arquivo é enviado em POST individual para `/api/files/upload`.
3. Progresso exibido; é possível cancelar o lote antes de terminar.

## Download e Deleção Múltiplos
- Seleção via checkboxes.
- Botões: Selecionar Todos, Limpar Seleção, Baixar Selecionados, Deletar Selecionados.
- Processamento sequencial (evita estouro de recursos simultâneos).

## Boas Práticas
- Mantenha apenas arquivos necessários para reduzir uso de flash e tempo de inicialização.
- Remova versões antigas de páginas HTML após atualizações.
- Faça backup de configurações antes de formatação da flash.

## Endpoints Principais
- `GET /api/files/list` – JSON com lista e `fileCount`.
- `POST /api/files/upload` – Upload de um arquivo (multi-upload usa vários POSTs).
- `GET /api/files/download?name=<arquivo>` – Download individual.
- (Múltiplo download: frontend abre cada URL em sequência.)
- `POST /api/files/delete` – Deleta arquivo (multi-delete faz vários POSTs).
- `POST /api/files/format` – Formata LittleFS.

## Recuperação
Se ocorrer travamento/reinício durante operação em massa:
- Acesse o dashboard para verificar uptime e heap livre.
- Refaça apenas operações pendentes (evite repetir uploads bem sucedidos).
- Verifique se arquivos de configuração essenciais permanecem íntegros.

---
Atualizado em: 28/11/2025
