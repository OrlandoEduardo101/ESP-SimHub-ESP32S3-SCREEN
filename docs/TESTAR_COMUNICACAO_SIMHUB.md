# Como Testar Comunicação com SimHub

## Problema: SimHub detecta porta mas não conecta

Se o SimHub mostra a porta COM11 mas não consegue conectar, siga estes passos:

## 1. Verificar se o dispositivo está respondendo

### No Windows:

1. **Abra o Gerenciador de Dispositivos:**
   - Pressione `Win + X`
   - Selecione "Gerenciador de Dispositivos"
   - Verifique se aparece "USB JTAG/serial debug unit" (sem erros)

2. **Abra o Monitor Serial:**
   - Use o Arduino IDE ou PlatformIO
   - Configure para COM11, 19200 baud
   - Você deve ver logs do dispositivo (se houver)

## 2. Testar Comunicação Manual

### Usando um terminal serial (ex: PuTTY, Tera Term):

1. **Conecte na porta COM11:**
   - Velocidade: 19200
   - Data bits: 8
   - Stop bits: 1
   - Parity: None
   - Flow control: None

2. **Envie comandos do SimHub manualmente:**
   - O SimHub envia: `0x03` seguido do comando
   - Comando '1' (Hello): Envie `0x03 0x31`
   - O dispositivo deve responder com a versão ('j')

## 3. Verificar no SimHub

1. **No SimHub:**
   - Vá em "Arduino" → "Single Arduino"
   - Selecione COM11
   - Clique em "Scan"
   - Verifique se aparece algum erro

2. **Se aparecer "port not scanned":**
   - O dispositivo pode não estar respondendo
   - Verifique se o firmware foi carregado corretamente
   - Tente desconectar e reconectar o USB

## 4. Problemas Comuns

### Problema: "await arduino boot"
- **Causa**: O dispositivo não está respondendo ao comando Hello
- **Solução**:
  - Verifique se o Serial está inicializado corretamente
  - Verifique se há erros no código
  - Tente aumentar o delay no setup()

### Problema: "port not scanned"
- **Causa**: Comunicação serial não está funcionando
- **Solução**:
  - Verifique a velocidade (deve ser 19200)
  - Verifique se a porta está correta
  - Tente outra porta USB no PC

### Problema: Dispositivo não aparece
- **Causa**: Driver não instalado ou porta não reconhecida
- **Solução**:
  - Verifique se aparece no Gerenciador de Dispositivos
  - Instale drivers se necessário
  - Tente outro cabo USB

## 5. Debug no Código

Para adicionar debug e ver o que está acontecendo:

1. **Adicione logs no código:**
   ```cpp
   void setup() {
     Serial.begin(19200);
     delay(2000);
     Serial.println("ESP-SimHubDisplay iniciado");
     // ... resto do código
   }
   ```

2. **Monitore o Serial:**
   - Abra o monitor serial
   - Veja se aparecem mensagens
   - Verifique se há erros

## 6. Checklist

Antes de reportar problemas, verifique:

- [ ] Porta COM aparece no SimHub
- [ ] Porta aparece no Gerenciador de Dispositivos (sem erros)
- [ ] Velocidade está configurada para 19200
- [ ] Firmware foi carregado com sucesso
- [ ] Dispositivo foi desconectado e reconectado após upload
- [ ] Monitor serial mostra atividade (se houver logs)

## 7. Próximos Passos

Se ainda não funcionar:

1. **Compile e faça upload novamente:**
   ```bash
   ./upload-mac.sh
   ```

2. **Verifique os logs do monitor serial** (se houver)

3. **Teste com um terminal serial** para ver se o dispositivo responde

4. **Verifique se há erros de compilação** que possam estar causando problemas

