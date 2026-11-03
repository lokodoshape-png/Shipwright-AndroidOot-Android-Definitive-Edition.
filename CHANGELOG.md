# OOT D.E. — Histórico de versões

Este arquivo registra as alterações da edição OOT D.E. As versões seguem a sequência 0.x até 1.0 e, depois, 1.1, 1.2 e assim por diante. Cada publicação deve incrementar também o `versionCode` Android.

## [0.1] — Primeira versão da edição

### Identidade e Android
- Criado o identificador de aplicativo independente `com.ootde.soh`, permitindo instalar OOT D.E. ao lado do Ship of Harkinian original.
- Nome exibido no Android alterado para **OOT D.E**.
- Nome dos APKs padronizado como `ootde_<tipo>_v<versão>.apk`.
- Definida a versão inicial como `0.1` (`versionCode` 9).

### Menu em português do Brasil
- Traduzidas as abas principais e categorias da navegação do menu.
- Traduzidos diversos rótulos das configurações gerais, áudio, gráficos, aprimoramentos, aleatorizador e ferramentas de desenvolvedor.
- Mantidos os identificadores internos e sufixos de identificação do ImGui em inglês para preservar compatibilidade.
- Traduzidos textos de pesquisa e ações básicas do menu.

### Observações
- Esta versão é uma base inicial de desenvolvimento; a tradução de descrições, dicas de contexto, opções internas e demais telas continua em andamento.
- As atualizações instaláveis por cima dependem da manutenção do mesmo `applicationId` e da mesma chave de assinatura em todas as versões.
- O APK ainda precisa ser compilado e validado antes de ser considerado uma versão de teste pronta.

## Convenção de versionamento
- `0.1` → `0.2` → `0.3` ... → `1.0` → `1.1` → `1.2` ...
- Não reutilizar `versionCode`; incrementar em toda publicação Android.
- Registrar em cada seção apenas alterações efetivamente incluídas naquela versão.
