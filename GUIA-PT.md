# Guia rápido: criar o Handkerchief para o PS Vita

Você mesmo cria o jogo para o Vita, no seu computador, a partir da **sua própria cópia** do Handkerchief. Nada do jogo é enviado para a internet nem distribuído. Não precisa instalar Python, WSL nem usar terminal.

## 1. Baixe o programa certo para o seu computador

| Seu computador | Baixe este arquivo (na página de [Releases](../../releases/latest)) | Depois |
| --- | --- | --- |
| **Windows** (10 ou 11, 64 bits) | **`HandkerchiefVitaBuilder-WINDOWS.zip`** | extraia e dê dois cliques em `HandkerchiefVitaBuilder.exe` |
| **Linux** (64 bits) | **`HandkerchiefVitaBuilder-LINUX.tar.gz`** | extraia e dê dois cliques em `HandkerchiefVitaBuilder` |

No Windows pode aparecer o aviso "O Windows protegeu o computador" (o programa não tem assinatura digital paga). Clique em **Mais informações** e depois em **Executar assim mesmo**.

## 2. Use a janela (3 passos)

1. **Escolha a pasta do seu Handkerchief**: a pasta onde está o `HDKWIN.EXE`. O programa tenta achá-la sozinho e avisa se faltar algo ou se a versão do jogo não for suportada. Duas versões funcionam: a **original (archive.org)** e a **com o fix da comunidade**. Se você escolher a pasta de fora (a que contém `HDKWIN/`), o programa acha a subpasta sozinho. Não tem o jogo? Ele está preservado no archive.org: <https://archive.org/details/Handkerchief> (há um botão para abrir a página).
2. **(Opcional) Escolha a pasta da tradução em português**: tradução feita por fãs, do Patryck (<https://patryckpo.com/traducoes/windows/handkerchief/downloads/>). Baixe, execute o arquivo para extrair e escolha a pasta extraída. Sem isso, o jogo fica em inglês.
3. **Criar o jogo para o Vita**: espere a barra de progresso. Na primeira vez leva de 5 a 20 minutos e baixa cerca de 250 MB de ferramentas gratuitas (precisa de internet e de uns 3 GB livres no disco). Das próximas vezes leva cerca de 1 minuto.

## 3. Instale no Vita

Quando aparecer **Pronto!**, a pasta do resultado se abre (por padrão `Handkerchief-Vita`, na Área de Trabalho). Nela:

1. Copie **`handkerchief.vpk`** para o Vita (cabo USB ou FTP) e instale com o **VitaShell** (abra o arquivo e aperte X).
2. Copie a pasta inteira **`handkerchief`** para **`ux0:data/`** no Vita. Deve ficar `ux0:data/handkerchief/HDKWIN.EXE`. Ao atualizar, copie a pasta por cima da antiga: seus saves (pasta `USER`) são mantidos, porque o construtor não inclui essa pasta; o jogo a cria sozinho na primeira execução.
3. Abra a bolha **Handkerchief** na tela LiveArea e divirta-se.

O Vita precisa de firmware customizado (HENkaku/Ensō) para instalar VPKs.

## Se algo der errado

- A janela explica o problema e o que fazer. O botão **Abrir a pasta de logs** mostra os detalhes para quem for ajudar você.
- Falha de download: verifique a internet/antivírus e clique em Criar de novo (o que já foi baixado é aproveitado).
- "Versão não suportada": use o `HDKWIN.EXE` da versão original (archive.org) ou da versão com o fix da comunidade, sem outros patches, tradução ou crack.
