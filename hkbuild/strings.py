"""User-visible texts (English and Brazilian Portuguese). t(key, **kw) formats with the active language."""
import locale, os

LANG = "en"


def detect():
    global LANG
    code = os.environ.get("HK_LANG", "")
    if not code:
        try:
            code = locale.getdefaultlocale()[0] or ""
        except Exception:
            code = ""
        if not code and os.name == "nt":
            try:
                import ctypes
                code = locale.windows_locale.get(ctypes.windll.kernel32.GetUserDefaultUILanguage(), "")
            except Exception:
                code = ""
    LANG = "pt" if code.lower().startswith("pt") else "en"
    return LANG


def t(key, **kw):
    table = S.get(key)
    if table is None:
        return key
    return table.get(LANG, table["en"]).format(**kw)


S = {
 "title": {"en": "Handkerchief Vita Builder — {plat}", "pt": "Handkerchief Vita Builder — {plat}"},
 "plat_windows": {"en": "Windows", "pt": "Windows"},
 "plat_linux": {"en": "Linux", "pt": "Linux"},
 "intro": {"en": "Make the PS Vita version of Handkerchief from your own copy of the game. Nothing from the game is "
                 "uploaded or shared; everything happens on this computer.",
           "pt": "Crie a versão de PS Vita do Handkerchief a partir da sua própria cópia do jogo. Nada do jogo é "
                 "enviado ou compartilhado; tudo acontece neste computador."},
 "step1": {"en": "1. Choose your Handkerchief folder", "pt": "1. Escolha a pasta do seu Handkerchief"},
 "step1_hint": {"en": "The folder that contains HDKWIN.EXE (and the folders BITMAP, SYSTEM, WAV). "
                       "Both the original release (archive.org) and the one with the community fix work.",
                "pt": "A pasta que contém o HDKWIN.EXE (e as pastas BITMAP, SYSTEM, WAV). "
                       "Tanto a versão original (archive.org) quanto a com o fix da comunidade funcionam."},
 "step2": {"en": "2. (Optional) Choose the Portuguese translation folder",
           "pt": "2. (Opcional) Escolha a pasta da tradução em português"},
 "step2_hint": {"en": "Fan translation by Patryck. Download and extract it first, then pick the extracted folder. "
                      "Leave empty for the original English game.",
                "pt": "Tradução feita por fãs, do Patryck. Baixe e extraia primeiro, depois escolha a pasta extraída. "
                      "Deixe vazio para o jogo original em inglês."},
 "step2_link": {"en": "Open the translation page", "pt": "Abrir a página da tradução"},
 "step3": {"en": "3. Build", "pt": "3. Criar"},
 "choose": {"en": "Choose folder...", "pt": "Escolher pasta..."},
 "clear": {"en": "Clear", "pt": "Limpar"},
 "none": {"en": "(nothing chosen)", "pt": "(nada escolhido)"},
 "build_btn": {"en": "Build my Vita game", "pt": "Criar o jogo para o Vita"},
 "cancel": {"en": "Cancel", "pt": "Cancelar"},
 "first_time": {"en": "The first time takes about 5 to 20 minutes and downloads about 250 MB of free tools. "
                      "Next times take about a minute.",
                "pt": "A primeira vez leva de 5 a 20 minutos e baixa cerca de 250 MB de ferramentas gratuitas. "
                      "As próximas levam cerca de um minuto."},
 "game_ok": {"en": "Game found and verified ({edition}).", "pt": "Jogo encontrado e verificado ({edition})."},
 "ed_cd": {"en": "with the community fix", "pt": "com o fix da comunidade"},
 "ed_archive": {"en": "original release (archive.org)", "pt": "versão original (archive.org)"},
 "no_game": {"en": "Don't have the game? Download it from:",
             "pt": "Não tem o jogo? Baixe em:"},
 "open_mab": {"en": "1. My Abandonware", "pt": "1. My Abandonware"},
 "open_archive": {"en": "2. archive.org", "pt": "2. archive.org"},
 "autodetected": {"en": "Found automatically: {path}", "pt": "Encontrado automaticamente: {path}"},
 "pick_game_title": {"en": "Select the Handkerchief folder", "pt": "Selecione a pasta do Handkerchief"},
 "pick_tr_title": {"en": "Select the extracted translation folder", "pt": "Selecione a pasta da tradução extraída"},
 "pick_out_title": {"en": "Where should the result be saved?", "pt": "Onde salvar o resultado?"},
 "err_no_exe": {"en": "HDKWIN.EXE was not found in this folder.\nChoose the folder where Handkerchief is installed "
                      "(the one that contains HDKWIN.EXE).",
                "pt": "O HDKWIN.EXE não foi encontrado nesta pasta.\nEscolha a pasta onde o Handkerchief está "
                      "instalado (a que contém o HDKWIN.EXE)."},
 "err_missing": {"en": "Some game folders are missing: {names}.\nUse a complete copy of the game (not just the .exe).",
                 "pt": "Faltam pastas do jogo: {names}.\nUse uma cópia completa do jogo (não apenas o .exe)."},
 "err_hash": {"en": "This HDKWIN.EXE is not the supported version.\nOnly the original, untouched Handkerchief releases (original or with the community fix) "
                    "work. Patched, translated or cracked executables do not.\n(SHA-256 found: {found})",
              "pt": "Este HDKWIN.EXE não é a versão suportada.\nSó funciona a versão original do Handkerchief, sem "
                    "modificações. Executáveis com patch, traduzidos ou crackeados não funcionam.\n"
                    "(SHA-256 encontrado: {found})"},
 "err_tr": {"en": "HDKWINBR.EXE was not found in the translation folder.\nExtract the translation archive first and "
                  "choose the extracted folder.",
            "pt": "O HDKWINBR.EXE não foi encontrado na pasta da tradução.\nExtraia o arquivo da tradução primeiro e "
                  "escolha a pasta extraída."},
 "err_nogame": {"en": "Choose your Handkerchief folder first (step 1).", "pt": "Escolha primeiro a pasta do Handkerchief (passo 1)."},
 "err_platform": {"en": "This download is for {want}, but your computer is {have}. Please download the right file "
                        "from the release page:\n  Windows: HandkerchiefVitaBuilder-WINDOWS.zip\n"
                        "  Linux: HandkerchiefVitaBuilder-LINUX.tar.gz\n(64-bit PCs only.)",
                  "pt": "Este download é para {want}, mas o seu computador é {have}. Baixe o arquivo certo na "
                        "página de releases:\n  Windows: HandkerchiefVitaBuilder-WINDOWS.zip\n"
                        "  Linux: HandkerchiefVitaBuilder-LINUX.tar.gz\n(somente PCs de 64 bits.)"},
 "err_space": {"en": "Not enough free disk space: about {need} GB are needed in {path}, {have} GB are free.",
               "pt": "Sem espaço livre em disco: são necessários cerca de {need} GB em {path}, há {have} GB livres."},
 "err_download": {"en": "Could not download {name}. Check your internet connection (and that a firewall or antivirus "
                        "is not blocking this program), then press Build again: finished downloads are kept.\n({why})",
                  "pt": "Não foi possível baixar {name}. Verifique sua conexão com a internet (e se um firewall ou "
                        "antivírus não está bloqueando o programa) e clique em Criar de novo: o que já foi baixado "
                        "é mantido.\n({why})"},
 "err_sha": {"en": "The downloaded file {name} is corrupt or was changed (checksum mismatch). Press Build again.",
             "pt": "O arquivo baixado {name} está corrompido ou foi alterado (checksum diferente). Clique em Criar de novo."},
 "err_step": {"en": "The step \"{step}\" failed.\nDetails were saved in: {log}\n\n{tail}",
              "pt": "A etapa \"{step}\" falhou.\nOs detalhes foram salvos em: {log}\n\n{tail}"},
 "err_novpk": {"en": "The build ended without producing a VPK. Details: {log}",
               "pt": "A compilação terminou sem gerar o VPK. Detalhes: {log}"},
 "err_cancel": {"en": "Cancelled.", "pt": "Cancelado."},
 "err_generic": {"en": "Unexpected error: {why}", "pt": "Erro inesperado: {why}"},
 "st_verify": {"en": "Checking your game files...", "pt": "Verificando os arquivos do jogo..."},
 "st_dl": {"en": "Downloading tools (once, {size}): {name}...", "pt": "Baixando ferramentas (só uma vez, {size}): {name}..."},
 "st_extract": {"en": "Unpacking tools (once): {name}...", "pt": "Descompactando ferramentas (só uma vez): {name}..."},
 "st_recomp": {"en": "Converting the game (about a minute)...", "pt": "Convertendo o jogo (cerca de 1 minuto)..."},
 "st_config": {"en": "Preparing the Vita build...", "pt": "Preparando a compilação para o Vita..."},
 "st_compile": {"en": "Building the VPK ({done}/{total})...", "pt": "Criando o VPK ({done}/{total})..."},
 "st_compile0": {"en": "Building the VPK...", "pt": "Criando o VPK..."},
 "st_copy": {"en": "Copying the game files...", "pt": "Copiando os arquivos do jogo..."},
 "st_cached": {"en": "Already prepared, skipping.", "pt": "Já preparado, pulando."},
 "elapsed": {"en": "Elapsed: {m}:{s:02d}", "pt": "Tempo: {m}:{s:02d}"},
 "done_title": {"en": "Done! Your Vita game is ready.", "pt": "Pronto! Seu jogo para o Vita está pronto."},
 "done_folder": {"en": "Saved in: {path}", "pt": "Salvo em: {path}"},
 "done1": {"en": "1. Copy  handkerchief.vpk  to your Vita (USB cable or FTP) and install it with VitaShell "
                 "(open the file, press X).",
           "pt": "1. Copie o  handkerchief.vpk  para o Vita (cabo USB ou FTP) e instale com o VitaShell "
                 "(abra o arquivo e aperte X)."},
 "done2": {"en": "2. Copy the whole folder  handkerchief  to  ux0:data/  on the Vita.\n"
                 "   Result: ux0:data/handkerchief/HDKWIN.EXE (and BITMAP, SYSTEM, WAV...).\n"
                 "   Updating? Copy the folder over the old one: your saves (the USER folder) are kept.",
           "pt": "2. Copie a pasta inteira  handkerchief  para  ux0:data/  no Vita.\n"
                 "   Resultado: ux0:data/handkerchief/HDKWIN.EXE (e BITMAP, SYSTEM, WAV...).\n"
                 "   Atualizando? Copie a pasta por cima da antiga: seus saves (pasta USER) são mantidos."},
 "done3": {"en": "3. Open the \"Handkerchief\" bubble on the LiveArea screen and play!",
           "pt": "3. Abra a bolha \"Handkerchief\" na tela LiveArea e divirta-se!"},
 "done_note": {"en": "Needs a Vita with custom firmware (HENkaku/Ensō) so unsigned VPKs can be installed.",
               "pt": "Precisa de um Vita com firmware customizado (HENkaku/Enso) para instalar VPKs."},
 "open_out": {"en": "Open the result folder", "pt": "Abrir a pasta do resultado"},
 "again": {"en": "Back", "pt": "Voltar"},
 "close": {"en": "Close", "pt": "Fechar"},
 "failed_title": {"en": "Something went wrong", "pt": "Algo deu errado"},
 "open_logs": {"en": "Open the log folder", "pt": "Abrir a pasta de logs"},
 "cli_done": {"en": "Done. Install handkerchief.vpk with VitaShell, then copy the folder 'handkerchief' to ux0:data/ on the Vita.",
              "pt": "Pronto. Instale o handkerchief.vpk com o VitaShell e copie a pasta 'handkerchief' para ux0:data/ no Vita."},
}
