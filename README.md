
# Projeto de Controle de LEDs e Joystick com Raspberry Pi Pico

Este projeto utiliza a placa Raspberry Pi Pico (com o microcontrolador RP2040) para controlar LEDs e interagir com um joystick, além de exibir informações em um display OLED. Ele também permite a manipulação de LEDs e o controle de dispositivos via interrupções de botões.

## Requisitos

Antes de começar, você precisará de alguns itens e ferramentas:

- **Placa Raspberry Pi Pico** (BitDogLab no caso do Embarcatech)
- **Cabo micro-USB** para conexão à placa
- **Joysticks analógicos** para controle
- **LEDs** para teste
- **Display OLED** (por exemplo, baseado no SSD1306)
- **Ferramentas de desenvolvimento**:
  - `git` (para clonar o repositório)
  - `CMake` (para gerar os arquivos de compilação)
  - `Make` (para compilar o código)
  - `SDK do Raspberry Pi Pico` (Raspberry Pi Pico SDK)

## Clonando o Repositório

Primeiro, clone o repositório do projeto para o seu computador:

```bash
git clone <url-do-repositorio>
cd <nome-do-repositorio>
```

## Configuração do Ambiente de Desenvolvimento

Antes de compilar o código, você precisará configurar o ambiente de desenvolvimento. Este projeto utiliza o **SDK do Raspberry Pi Pico** e **CMake** para compilar o código. Siga os passos abaixo para configurar:

### 1. Criar o diretório de build

Primeiro, crie um diretório para a compilação do projeto:

```bash
mkdir build
cd build
```

### 2. Configurar o CMake

Dentro do diretório `build`, configure o projeto com o CMake:

```bash
cmake ..
```

Este comando irá gerar os arquivos de compilação necessários.

### 3. Compilar o código

Agora, compile o projeto com o `make`:

```bash
make
```

Isso irá compilar o código e gerar o arquivo binário para o Raspberry Pi Pico.

### 4. Carregar o código na Placa

Após a compilação, você precisará carregar o código na sua placa Raspberry Pi Pico. Para isso, pressione o botão **BOOTSEL** na placa enquanto a conecta ao computador via USB. Isso fará com que a placa seja reconhecida como um dispositivo de armazenamento em massa.

Copie o arquivo `.uf2` gerado pelo comando `make` para a memória da placa Raspberry Pi Pico. Após copiar o arquivo, a placa será reiniciada automaticamente e começará a executar o código.

## Testando o Projeto

Após carregar o código na sua Raspberry Pi Pico, siga os passos abaixo para testar a funcionalidade do projeto:

1. **Controle de LEDs com o Joystick**:
   - O joystick tem três direções de controle:
     - **Eixo X (direção horizontal)**: Ao mover o joystick para a direção **X**, o LED **vermelho** acende, com a intensidade de brilho controlada pela posição do joystick.
     - **Eixo Y (direção vertical)**: Ao mover o joystick para a direção **Y**, o LED **azul** acende, com o brilho ajustado da mesma forma.
     - **Botão do Joystick**: Quando o botão do joystick é pressionado, o LED **verde** acende ou apaga, alternando entre os dois estados.

2. **Botões**:
   - O projeto possui dois botões configurados para realizar as seguintes ações:
     - **Botão A**: Alterna entre ligar/desligar os LEDs.
     - **Joystick**: Trabalha com as interações no display OLED e os leds.

3. **Display OLED**:
   - O display OLED irá mostrar um objeto que se move de acordo com os valores lidos do joystick.

4. **Testar a Funcionalidade**:
   - Mova o joystick e verifique se os LEDs e o objeto no display OLED respondem corretamente.
   - Pressione os botões para testar as interrupções e se a lógica de controle dos LEDs e do sistema funciona corretamente.


## Autoria

Este projeto foi desenvolvido por Diêgo Farias de freitass.

