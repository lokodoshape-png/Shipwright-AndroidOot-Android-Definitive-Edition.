# Testes da versão 0.8.0

Base: `main` no commit `c28198a` (0.7.1, versionCode 17). Branch: `fix/ootde-0.8-stability`.

Só está marcado como feito o que foi realmente executado. Nenhum teste em celular foi feito durante o desenvolvimento
(não havia aparelho Android disponível); esses ficam na lista de pendências abaixo.

## Testes automáticos (rodam no computador e no GitHub Actions antes do build)

| Script | O que testa | Resultado |
| --- | --- | --- |
| `scripts/tests/run_java_tests.sh` | `SafeFiles.java`: CRC32C (vetor do RFC 3720 e comparação com uma implementação independente em Python), conversão .v64/.n64 igual à do extrator, ROM recusada por CRC, tamanho, compactação, cabeçalho e cópia curta; troca segura de arquivo (mantém o antigo se a troca falhar); `soh.otr` válido, cortado e inválido; mods `.otr`/`.o2r` válidos e cortados; detecção de mod duplicado e nome livre "nome (2).otr" | 25 de 25 passaram |
| `scripts/tests/run_qm_encode_test.sh` | `QM_Encode` copiado do código do jogo, com AddressSanitizer: limite de saída para todo tamanho de 0 a 64, letra de dois códigos no fim do buffer, terminador, letras do português, UTF-8 quebrado, e os 449 textos de `questmenu_data.c` | todos passaram; nenhum texto cortado |
| `scripts/tests/run_otr_verify_test.sh` | `otr_add_custom.py --verify`: o `soh.otr` do repositório passa; cópia ausente, cortada, aleatória ou corrompida é recusada | 5 de 5 passaram |

Também foi conferido:

- O código antigo de `QM_Encode` (0.7.1) escreve fora do buffer: com "ABÃ" e espaço para 3 códigos ele devolve 4 e
  escreve a 4ª posição. O novo para em 2.
- O Java do app (`MainActivity`, `SafeFiles`, `AssetCopyUtil`, `SDLActivity`) compila com `javac` contra as classes do
  Android 13 (com substitutos simples para `androidx.core` e `R`). Isso só confere tipos e sintaxe; o build completo é
  o do GitHub Actions.
- `CONFIGURE_DEPENDS` testado com CMake 3.28 num projeto pequeno: um `.c` criado depois da configuração é compilado no
  próximo `make` sem reconfigurar à mão.

## Matriz de testes no celular (pendentes)

| Teste | Resultado esperado | Situação |
| --- | --- | --- |
| Instalação limpa | Pede a permissão, prepara os arquivos e o jogo inicia | Pendente (precisa de celular) |
| Permissão negada | Aparece "Permissão necessária"; "Sair" fecha sem travar | Pendente |
| Permissão concedida depois | Ao voltar para o app a preparação começa sozinha | Pendente |
| Preparação incompleta (sem espaço, ou app fechado no meio) | "Falha na preparação" com o motivo; o jogo não inicia; "Tentar novamente" funciona; na próxima abertura tudo é copiado de novo | Pendente |
| Importação de ROM válida | ROM conferida e o extrator cria o `oot.otr` | Pendente (o teste automático usa ROMs falsas; nenhuma ROM real foi usada) |
| Importação interrompida ou ROM errada | Mensagem com o motivo; o `OOT.z64` anterior continua igual | Pendente |
| Instalação de mod | Arquivo completo na pasta `mods`, aparece no menu de mods | Pendente |
| Mod com nome duplicado | Pergunta "Substituir / Manter os dois / Cancelar"; nada é sobrescrito sem escolha | Pendente |
| Abrir e fechar o Diário de Quests várias vezes | Sem travamentos; animações e volta ao jogo normais | Pendente |
| Marcar Ganondorf | A (ou toque) marca e desmarca depois que "Flechas de Luz" está concluída; a marca fica só naquele save | Pendente |
| Conversão UTF-8 | Acentos iguais aos da 0.7.1 | Coberto pelo teste automático; conferir na tela |
| Controle Bluetooth e toque no Diário | Navegação igual à 0.7.1 | Pendente (código de navegação não mudou, exceto o botão A) |
| Android 11 a 16 | Permissão "acesso a todos os arquivos" e pasta `OOT-DE` funcionando | Avaliado no código (veja abaixo); pendente no aparelho |
| Atualização 0.7.1 → 0.8.0 | Saves, mods, `oot.otr` e configurações mantidos; `soh.otr` e `assets/` trocados | Pendente |

## Compatibilidade Android avaliada no código

- Android 11 a 16 (API 30 a 36): o app usa `MANAGE_EXTERNAL_STORAGE`, que continua valendo para apps instalados fora da
  Play Store. O jogo abre saves, mods, `soh.otr` e `oot.otr` por caminho de arquivo, por isso essa permissão não pode
  ser trocada pelo Storage Access Framework sem mudar o libultraship. ROM e mods já são escolhidos pelo Storage Access
  Framework (`ACTION_OPEN_DOCUMENT`).
- `targetSdk` continua 33. Subir para 35 ou mais ativa o desenho de ponta a ponta (edge-to-edge) e outras mudanças que
  precisam ser testadas num aparelho antes.
- Android 10: `requestLegacyExternalStorage` adicionado. Android 7 a 9: permissões de leitura e escrita pedidas na
  hora, agora com resposta tratada (antes nada acontecia depois de aceitar).
