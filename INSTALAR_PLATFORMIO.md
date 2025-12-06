# Como Instalar PlatformIO no Mac

O PlatformIO não está instalado no seu sistema. Escolha uma das opções abaixo:

## Opção 1: Via VS Code (Mais Fácil - Recomendado) ⭐

1. **Instale o VS Code** (se não tiver):
   - Baixe em: https://code.visualstudio.com/

2. **Instale a extensão PlatformIO IDE**:
   - Abra o VS Code
   - Vá em Extensions (Cmd+Shift+X)
   - Procure por "PlatformIO IDE"
   - Clique em Install

3. **O PlatformIO será instalado automaticamente** em `~/.platformio/penv/bin/pio`

4. **Após instalar**, você pode usar:
   ```bash
   ~/.platformio/penv/bin/pio run -e wt32-sc01-plus -t upload
   ```

   Ou adicione ao PATH:
   ```bash
   echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc
   source ~/.zshrc
   ```

## Opção 2: Via pip (Python)

```bash
# Instalar
pip3 install platformio

# Ou para usuário atual apenas
pip3 install --user platformio

# Verificar instalação
pio --version
```

## Opção 3: Via Homebrew

```bash
# Instalar Homebrew (se não tiver)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Instalar PlatformIO
brew install platformio

# Verificar
pio --version
```

## Opção 4: Via Script Oficial

```bash
python3 -c "$(curl -fsSL https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py)"
```

## Após Instalar

Teste se funciona:

```bash
pio --version
```

Se funcionar, você pode usar o script:

```bash
./upload-mac.sh
```

Ou compilar manualmente:

```bash
pio run -e wt32-sc01-plus -t upload
```

## Adicionar ao PATH (Opcional)

Se o PlatformIO foi instalado mas não está no PATH, adicione ao `~/.zshrc`:

```bash
# Para VS Code extension
echo 'export PATH="$HOME/.platformio/penv/bin:$PATH"' >> ~/.zshrc

# Para pip --user
echo 'export PATH="$HOME/.local/bin:$PATH"' >> ~/.zshrc

# Recarregar
source ~/.zshrc
```

