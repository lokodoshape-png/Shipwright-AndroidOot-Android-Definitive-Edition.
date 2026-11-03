# Como as versões do OOT D.E. são organizadas

Mesma organização usada no fork 2Ship MM3D (lokodoshape-png/2ship2harkinian-Android).

## Branch
- **`main`** é a linha do OOT D.E. Todo o trabalho entra nela, de preferência por pull request.
- Cada pull request compila o APK no GitHub Actions e deixa o arquivo como artefato da execução, para testar antes
  de juntar.
- Não se criam branches por versão (`release/0.x...`). A versão fica registrada no código, no CHANGELOG e na tag
  de cada build.

## Número da versão
- A sequência é `0.1`, `0.2`, `0.3`, ... `1.0`, depois `1.1`, `1.2` e assim por diante.
- Ao começar uma versão nova, mude os dois arquivos juntos (a build falha se eles forem diferentes):
  - `Android/app/build.gradle`: `versionName` e `versionCode`;
  - `Android/app/src/main/AndroidManifest.xml`: `android:versionName` e `android:versionCode`.
- O `versionCode` sempre sobe e nunca é reaproveitado. Quando ele sobe, o app troca o `soh.otr` e a pasta
  `assets` do celular pelos da versão nova, sem tocar em saves, mods e `oot.otr`.
- Mantenha o `applicationId` `com.ootde.soh` e a mesma chave de assinatura (segredos `ANDROID_KEYSTORE_*` do
  repositório). Se algum dos dois mudar, o Android recusa instalar por cima e é preciso desinstalar.

## Builds e Releases
- Cada push na `main` gera uma release `android-<versão>-build-N` (N é o número da execução do Actions), marcada
  como a mais recente. Exemplo: `android-0.4-build-70`.
- Uma execução manual (Actions > Build Android APK > Run workflow) em outra branch publica a build como
  pré-lançamento.
- As notas da release vêm da seção da versão no `CHANGELOG.md`.

## CHANGELOG
- `CHANGELOG.md`, em português, com a versão mais nova no topo.
- Cada versão tem uma seção `## [x.y] — título`, com o que mudou para quem joga. Só entra o que está de fato
  naquela versão; o que falta vai em "Pendências".

## Tags antigas
- `v0.1-android.1` e `v0.2-android.1` são das versões feitas antes desta organização e ficam como estão.

## Pasta do app no celular
- Desde a 0.5 o app usa `/storage/emulated/0/OOT-DE`. O caminho está em dois lugares, que precisam ser iguais:
  `DATA_FOLDER` no `MainActivity.java` e o `patches/libultraship-ootde.patch` (o libultraship é um submódulo).
- O `docker/scripts/build_android_project.sh` aplica esse patch antes de compilar. Numa build pelo Android Studio,
  aplique antes com `git -C libultraship apply ../patches/libultraship-ootde.patch`.
