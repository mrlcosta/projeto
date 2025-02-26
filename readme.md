# Jogo da Memória com Raspberry Pi Pico

Este projeto implementa um jogo da memória interativo utilizando a placa Raspberry Pi Pico, uma matriz de LEDs WS2812, um joystick analógico, um display OLED e um buzzer. O jogo desafia o jogador a memorizar e repetir uma sequência crescente de LEDs que piscam, controlando a seleção dos LEDs através do joystick e confirmando com um botão.

## Funcionalidades

* Geração de sequências aleatórias de LEDs.
* Exibição visual da sequência na matriz de LEDs WS2812.
* Navegação precisa do cursor na matriz de LEDs através do joystick analógico.
* Confirmação da seleção do LED através de um botão.
* Feedback visual através do display OLED (pontuação, mensagens).
* Feedback sonoro através do buzzer (acertos, erros, início do jogo).
* Botão A para iniciar o jogo.
* Aumento gradual da dificuldade com sequências mais longas.

## Hardware Utilizado

* Raspberry Pi Pico
* Matriz de LEDs WS2812 (5x5)
* Joystick Analógico
* Display OLED (SSD1306)
* Buzzer
* 2 Botões

## Pinagem

* WS2812: Pino 7
* Joystick X: Pino 27 (ADC0)
* Joystick Y: Pino 26 (ADC1)
* Botão de confirmação: pino 6
* Display OLED SDA: Pino 14
* Display OLED SCL: Pino 15
* Buzzer: pino 21
* Botão A: pino 5
* Botão do Joystick: pino 22

## Dependências

* Raspberry Pi Pico SDK
* Biblioteca SSD1306 para display OLED (incluída no código)

## Como Usar

1.  Conecte os componentes conforme a pinagem.
2.  Compile e carregue o código para a Raspberry Pi Pico utilizando o ambiente de desenvolvimento de sua preferência.
3.  Pressione o botão A para iniciar o jogo.
4.  Utilize o joystick para mover o cursor e o botão de confirmação para selecionar os LEDs.
5.  Siga as instruções exibidas no display OLED.

## Estrutura do Código

* `main.c`: Contém a lógica principal do jogo, inicialização dos periféricos e tratamento de eventos.
* `ws2812.pio`: Programa PIO para controle dos LEDs WS2812.
* `lib/ssd1306.h`: Biblioteca para controle do display OLED SSD1306.

## Funções Principais

* `init_ws2812()`: Inicializa o PIO e os LEDs WS2812.
* `play_sound(int frequency, int duration)`: Toca um som no buzzer.
* `set_led_color(int x, int y, uint32_t color)`: Define a cor de um LED na matriz.
* `update_leds(PIO pio, uint sm)`: Atualiza a matriz de LEDs WS2812.
* `read_joystick(int pin)`: Lê o valor do joystick.
* `move_led_with_joystick(int *x, int *y)`: Move o LED com base no joystick.
* `show_x()`: Mostra um "X" na matriz de LEDs.
* `game_loop()`: Lógica principal do jogo.
* `init_buzzer()`: Inicializa o buzzer.
* `init_display_oled()`: Inicializa o display OLED.
* `update_display(char *linha1, char *linha2)`: Atualiza o display OLED.
* `debounce_bt(uint pino, absolute_time_t *ultimo_tempo)`: Função de debounce para os botões.
* `gpio_irq_handler(uint gpio, uint32_t events)`: Callback da interrupção do botão A.
* `config_button_a_irq()`: Configura a interrupção do botão A.


## Vídeo Demonstrativo:

[![Assista ao vídeo demonstrativo](https://i.ytimg.com/vi/m4CvZlHNlw0/hq720_2.jpg?sqp=-oaymwEoCJUDENAFSFryq4qpAxoIARUAAIhC0AEB2AEB4gEKCBgQAhgGOAFAAQ==&rs=AOn4CLDEAiMRf0h5bnH_93G-STIvsmkGAQ)](https://www.youtube.com/shorts/m4CvZlHNlw0)
