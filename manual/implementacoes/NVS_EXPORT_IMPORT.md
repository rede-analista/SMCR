# Exportação/Importação do NVS e Correção de Listagem

Data: 26/11/2025

## Resumo
- Adicionados recursos de exportar e importar todas as chaves do NVS (todas as namespaces) pela interface web e via API.
- Opções de segurança para mascarar segredos por padrão e incluir segredos sob confirmação.
- Opção de "apagar namespaces antes de importar" para restauração limpa.
- Correção de crash (LoadProhibited) ao abrir o menu Arquivos causada por iteração incorreta do NVS.

## Motivação
- Facilitar backup, clonagem e restauração de configurações entre módulos.
- Possibilitar diagnósticos: visualização completa do NVS, inclusive outras namespaces além de `smcrconf`.
- Endereçar reinicialização ao acessar o menu Arquivos.

## Solução
- Enumeração de chaves do NVS com `nvs_entry_find("nvs", NULL, NVS_TYPE_ANY)` (partição explícita) e `nvs_entry_next`.
- Máscara automática de chaves sensíveis (contendo `pass`, `senha`, `token`, `secret`) como `[OCULTO]`.
- Exportação:
  - `JSON` (padrão) com estrutura: `preferences[{ namespace, key, type, value }]`.
  - `Texto` com linhas: `namespace:key [TYPE]=value`.
  - Parâmetro opcional `include_secrets=1` para exportar segredos em claro.
- Importação:
  - `POST` com JSON no mesmo formato do export (BLOB/UNKNOWN ignorados; `[OCULTO]` ignorado).
  - Parâmetro opcional `clear=1` para apagar todas as chaves das namespaces presentes no arquivo antes de gravar.
  - Estatísticas de retorno: `ok`, `skipped`, `errors`, `cleared`.

## API Endpoints
- `GET /api/nvs/list` — lista todas as chaves do NVS (todas namespaces). Campos:
  - `preferences[]`: `key` (também já inclui `namespace:key`), `type`, `value`, `namespace`.
  - `count`.
- `GET /api/nvs/export` — exporta como arquivo (attachment).
  - Parâmetros:
    - `format=json|text` (padrão: `json`)
    - `include_secrets=1` para incluir segredos.
- `POST /api/nvs/import` — importa de JSON (mesmo formato do export JSON).
  - Parâmetros:
    - `clear=1` para apagar namespaces antes da importação.
  - Retorno: `{ success, ok, skipped, errors, cleared }`.

## Mudanças na Interface (Página Arquivos)
- Seção "Preferências NVS":
  - Botões "Exportar NVS (JSON)" e "Exportar NVS (Texto)".
  - Checkbox "Incluir segredos (senha/token)" com confirmação.
  - Formulário "Importar NVS (JSON)" com input de arquivo e checkbox "Apagar namespaces antes de importar".
  - Mensagens de status e recarregamento automático após importar.

## Segurança
- Por padrão, valores sensíveis são mascarados como `[OCULTO]` na listagem e exportação.
- A inclusão de segredos exige opção explícita e confirmação na UI.
- Import ignora valores `[OCULTO]` e tipos `BLOB`/`UNKNOWN`.

## Correção de Crash
- Causa: enumeração usando `nvs_entry_find(NULL, ...)` levando a ponteiro inválido em algumas condições.
- Correção: uso explícito da partição "nvs" e checagem nula antes de `nvs_release_iterator`.

## Arquivos Modificados
- `src/servidor_web.cpp`
  - `fV_handleNVSList`: enumeração completa do NVS, máscara de segredos e correção da partição.
  - `fV_handleNVSExport`: export JSON/Text, `include_secrets`, correção de partição.
  - `fV_handleNVSImport`: import JSON, opção `clear=1`, estatísticas.
  - Rotas: `/api/nvs/list`, `/api/nvs/export`, `/api/nvs/import`.
- `include/web_files.h`
  - UI: botões de exportação JSON/Text com opção de segredos.
  - UI: formulário de import com opção de apagar namespaces e confirmações.

## Teste/Validação
- Build via PlatformIO: ok.
- Acesso a `/arquivos`: listagem do NVS carrega sem reset.
- Export JSON/Text: arquivos baixados com nome `nvs_export_<HOSTNAME>_<millis>.(json|txt)`.
- Import JSON:
  - Sem `clear`: mescla valores.
  - Com `clear=1`: apaga chaves das namespaces presentes e restaura; resposta mostra `cleared`.

## Benefícios
- Backup e restauração rápidos.
- Clonagem de módulos simplificada.
- Diagnóstico mais completo do estado do NVS.
- Estabilidade da página Arquivos corrigida.
