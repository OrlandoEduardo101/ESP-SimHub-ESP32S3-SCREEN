# Minimapa de Pista — PAGE_MAP

## O que é

A **PAGE_MAP** (página 6 do display) exibe o layout real da pista em miniatura com os carros do jogador, carro à frente e carro atrás posicionados dinamicamente via dados do SimHub. Para pistas sem SVG cadastrado, exibe um oval genérico como fallback.

```
┌──────────────────────────────────────────────────────────────┐
│ MAP  Interlagos                 P3     2.3          FUEL LAPS│
├──────────────────────────────┬───────────────────────────────┤
│                              │ DELTA   -0.312                │
│    [polyline da pista]       │ GAP ↑   2.541                 │
│         ●  (jogador verde)   │ GAP ↓   8.102                 │
│      ● A   (frente vermelho) │ SPD  287  G  6                │
│                      ● B    │ BB   54.2%                    │
│          (atrás azul)        │ TC  3  ABS  4                 │
│                              │ LAP  1:32.441                 │
└──────────────────────────────┴───────────────────────────────┘
        220×160 px (PROGMEM)           Painel estratégico
```

---

## Pistas disponíveis

| Pista | Strings de match (trackId do SimHub) |
|---|---|
| Circuit of the Americas | `americas`, `cota` |
| Autodromo di Monza | `monza` |
| Circuit Gilles Villeneuve | `villeneuve`, `montreal` |
| Autodromo Interlagos | `interlagos` |
| Circuit de Monaco | `monaco` |
| Circuit Paul Ricard | `paul_ricard`, `ricard`, `castellet` |
| Red Bull Ring | `red_bull`, `spielberg` |
| Sepang International | `sepang` |
| Silverstone | `silverstone` |
| Sochi Autodrom | `sochi` |
| Suzuka | `suzuka` |

O match é feito por **substring case-insensitive**: se o `trackId` enviado pelo SimHub contém qualquer uma das strings acima, a pista correspondente é usada.
Exemplo: `"Autodromo_Nazionale_Monza"` → match com `"monza"`.

Para pistas fora dessa lista, o oval genérico é exibido automaticamente.

---

## Arquitetura

| Arquivo | Papel |
|---|---|
| `tracks_svg/*.svg` | Arquivos fonte dos layouts das pistas |
| `scripts/convert_tracks.py` | Script Python que converte SVGs → header C |
| `src/TrackMaps.h` | Header gerado automaticamente (arrays PROGMEM) |
| `src/SHCustomProtocol.h` | Funções de renderização e lookup |

### Fluxo de dados

```
SVG (curvas Bézier)
    │
    ▼ convert_tracks.py
    │  1. Extrai path data do SVG
    │  2. Achata Bézier → 80 pontos equidistantes (arc-length)
    │  3. Normaliza para 220×160 px
    │  4. Gera int16_t PROGMEM arrays
    ▼
TrackMaps.h  ──────────────────────►  SHCustomProtocol.h
                                       findTrackMap(trackId)
                                       drawTrackPolyline()
                                       getPositionOnTrack()
```

### PROGMEM

Cada pista ocupa `80 pontos × 2 coords × 2 bytes = 320 bytes`.
Total de 11 pistas: **~3.5 KB Flash** (negligível).

---

## Como adicionar uma nova pista

### Pré-requisitos

```bash
# Criar virtualenv com dependência (só na primeira vez)
python3 -m venv /tmp/svg_conv
source /tmp/svg_conv/bin/activate
pip install svgpathtools
```

### Passo 1 — Obter o SVG

O SVG deve conter um `<path d="...">` com o traçado completo da pista.
Fontes recomendadas:
- [Wikimedia Commons](https://commons.wikimedia.org/wiki/Category:Circuit_diagrams_of_Formula_One_circuits) — buscar "RaceCircuit + nome"
- Os SVGs existentes em `tracks_svg/` são um bom modelo de referência

Copiar o arquivo `.svg` para a pasta `tracks_svg/`:

```
tracks_svg/RaceCircuitSpa.svg
```

### Passo 2 — Registrar no script de conversão

Abrir `scripts/convert_tracks.py` e localizar o dicionário `TRACKS`:

```python
TRACKS = {
    'RaceCircuitAmericas':         ('AMERICAS',    ['americas', 'cota']),
    'RaceCircuitAutodromaDiMonza': ('MONZA',       ['monza']),
    # ...
}
```

Adicionar uma entrada com:
- **Chave**: nome do arquivo SVG sem extensão (exato)
- **C\_IDENTIFIER**: identificador C em maiúsculas (sem espaços)
- **match strings**: substrings que o SimHub pode enviar como `trackId` para essa pista (em minúsculas)

Exemplo para Spa-Francorchamps:

```python
'RaceCircuitSpa': ('SPA', ['spa', 'francorchamps']),
```

### Passo 3 — Rodar o script

```bash
cd /caminho/para/o/projeto
source /tmp/svg_conv/bin/activate
python scripts/convert_tracks.py
```

O script exibe um preview ASCII de cada pista para validação visual:

```
  RaceCircuitSpa.svg -> TRACK_SPA ... OK (80 pts)

  RaceCircuitSpa:
  +------------------------------------------------------------+
  |         #  #                                               |
  |       #      #  # # # #                                    |
  | # # #                  #  # #                              |
  ...
```

Se a forma parecer incorreta, verifique se o SVG tem `<g transform="translate(...)">` e se o path está no elemento esperado.

### Passo 4 — Compilar

```bash
pio run -e wt32-sc01-plus
```

O arquivo `src/TrackMaps.h` é atualizado automaticamente pelo script.
Nenhuma outra alteração no código C++ é necessária.

---

## Formato do SVG suportado

O script aceita dois formatos de SVG:

### Formato simples (1 path)
```xml
<svg viewBox="0 0 556 528">
  <path d="m542.8 6.5 c..." stroke="#000" fill="none"/>
</svg>
```

### Formato multi-layer (5 paths idênticos com estilos diferentes)
```xml
<svg viewBox="0 0 457 673">
  <g>
    <path d="M224 40 C..." fill="#006400" stroke-width="80"/>  <!-- fill verde -->
    <path d="M224 40 C..." stroke="#f00" stroke-width="26"/>   <!-- curbs vermelho -->
    <path d="M224 40 C..." stroke="#fff" stroke-dasharray="26,26"/>
    <path d="M224 40 C..." stroke="#141414" stroke-width="20"/>
    <path d="M224 40 C..." stroke="#fff" stroke-width="2"/>    <!-- center line -->
  </g>
</svg>
```

No formato multi-layer os 5 paths têm o **mesmo `d`** — o script extrai apenas um, ignorando os demais estilos visuais.

Transformações `<g transform="translate(x,y)">` são aplicadas automaticamente.

---

## Troubleshooting

**Pista não reconhecida no display (aparece oval)**

Verifique qual `trackId` o SimHub está enviando. Na página de telemetria do SimHub:
- `DataCorePlugin.GameData.NewData.TrackId`

O valor varia por jogo (ACC, iRacing, F1 24, etc.). Adicione a string exata como alias no dicionário `TRACKS` e regenere o header.

**Pista com forma distorcida no preview ASCII**

O SVG pode ter o traçado em sub-paths (`M ... Z M ... Z`). O script concatena múltiplos sub-paths em sequência. Se o resultado ainda parecer errado, edite manualmente eliminando sub-paths secundários (como o logo ou decorações internas, ex: Interlagos tem ilhas no interior que foram removidas automaticamente).

**Erro ao rodar o script**

```
ERROR: svgpathtools required
```
→ Ativar o virtualenv: `source /tmp/svg_conv/bin/activate`

```
ERROR: SVG directory not found
```
→ Rodar o script a partir da raiz do projeto: `cd /caminho/projeto && python scripts/convert_tracks.py`
