#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "ws2812.pio.h" // Necessário para controle de LEDs WS2812
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "hardware/i2c.h"
#include "lib/ssd1306.h"

#define I2C_PORT i2c1
#define SDA_PIN 14
#define SCL_PIN 15
#define OLED_ADDR 0x3C
ssd1306_t ssd;
#define WS2812_PINO 7 // Pino de controle do WS2812
#define JOYSTICK_X 27 // Pino analógico para movimentação X
#define JOYSTICK_Y 26 // Pino analógico para movimentação Y
#define BUTTON_PIN 6  // Pino digital para o botão de confirmação
#define QTD_LEDS 25   // Total de LEDs na matriz 5x5 (5*5 = 25)
#define BUZZER_PIN 21 // Pino do buzzer A
#define PULSO_MIN 500
#define PULSO_MEIO 1470
#define PULSO_MAX 2400

#define VALOR_TOP 20000
#define DIVISOR_CLK 10.0

#define BOTAO_A 5
#define BT_JOYSTICK 22
absolute_time_t debounce_A;
absolute_time_t debounce_B;
bool iniciar = 0;

static const uint8_t mapeamento_led[5][5] = {
    {24, 23, 22, 21, 20},
    {15, 16, 17, 18, 19},
    {14, 13, 12, 11, 10},
    {5, 6, 7, 8, 9},
    {4, 3, 2, 1, 0}};

// Buffer de LEDs para armazenar o estado de cada LED
uint32_t buffer_led[QTD_LEDS];

void update_display(char *linha1, char *linha2);

// Função para inicializar o PIO e os LEDs WS2812
void init_ws2812()
{
    PIO pio = pio0;
    uint sm = 0;

    // Carregar o programa WS2812 no PIO
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, WS2812_PINO, 800000, false);
}

// Função para tocar um som em um determinado tom e duração
void play_sound(int frequency, int duration)
{

    uint slice_pwm = pwm_gpio_to_slice_num(BUZZER_PIN);
    gpio_set_function(BUZZER_PIN, GPIO_FUNC_PWM); // Define função PWM para o pino

    pwm_set_enabled(slice_pwm, false); // desliga o PWM antes da configuração

    pwm_set_wrap(slice_pwm, VALOR_TOP * frequency); // Define o valor do contador
    pwm_set_clkdiv(slice_pwm, DIVISOR_CLK);         // Ajusta o divisor do clock para 1MHz

    pwm_set_chan_level(slice_pwm, pwm_gpio_to_channel(BUZZER_PIN), 1000); // inicia com duty cycle
    pwm_set_enabled(slice_pwm, true);                                     // liga o PWM depois de configurar
    sleep_ms(duration);
    pwm_set_enabled(slice_pwm, false); // desliga pwm
}

// Funções para eventos
void sound_game_start()
{
    for (int i = 0; i < 3; i++)
    {
        play_sound(1, 500);
        sleep_ms(250);
    }
}

void sound_game_win()
{
    for (int i = 0; i < 2; i++)
    {
        play_sound(1, 500);
        sleep_ms(100);
    }
    for (int i = 0; i < 3; i++)
    {
        play_sound(2, 200);
        sleep_ms(200);
    }
    for (int i = 0; i < 2; i++)
    {
        play_sound(4, 300);
        sleep_ms(100);
    }
}

void sound_button_press()
{
    play_sound(2, 200);
}

void sound_game_over()
{
    play_sound(100, 1000);
}

// Função para acender ou apagar um LED na posição (x, y) com cor
void set_led_color(int x, int y, uint32_t color)
{
    uint16_t led_index = mapeamento_led[x][y]; // Usa o mapeamento específico da matriz
    buffer_led[led_index] = color;
}

// Função para atualizar a matriz de LEDs WS2812
void update_leds(PIO pio, uint sm)
{
    // Envia os valores de 'buffer_led' para o hardware de LED
    for (int i = 0; i < QTD_LEDS; i++)
    {
        pio_sm_put_blocking(pio, sm, buffer_led[i]);
    }
}

// Função para ler o valor do joystick (simulação simples, valor em 0-1023)
int read_joystick(int pin)
{
    if (pin == JOYSTICK_X)
    {
        adc_select_input(0);    // Seleciona o canal ADC para o eixo X (pino 27)
        int value = adc_read(); // Lê o valor do pino X
        return value;
    }
    else if (pin == JOYSTICK_Y)
    {
        adc_select_input(1); // Seleciona o canal ADC para o eixo Y (pino 26)
        return adc_read();   // Lê o valor do pino Y
    }
    return 0;
}

// Função para inicializar o joystick e o botão
void init_joystick_button()
{
    adc_init();
    adc_gpio_init(JOYSTICK_X);
    adc_gpio_init(JOYSTICK_Y);
    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN); // Ativa o pull-up no botão
}

// Função para mover o LED vermelho na tela com base no joystick
void move_led_with_joystick(int *x, int *y)
{
    int joystick_x = read_joystick(JOYSTICK_X); // Ler o eixo X
    int joystick_y = read_joystick(JOYSTICK_Y); // Ler o eixo Y

    // Mapear os valores do joystick para a matriz 5x5
    *x = 4 - (joystick_x * 5) / 4096; // Mapeia para valores entre 0 e 4 (X)

    *y = (joystick_y * 5) / 4096; // Mapeia para valores entre 0 e 4 (Y)

    // Ajuste para garantir que a posição final da matriz corresponda corretamente
    if (*x < 0)
        *x = 0;
    if (*x > 4)
        *x = 4;

    if (*y < 0)
        *y = 0;
    if (*y > 4)
        *y = 4;
}

// Função para mostrar um "X" na matriz de LEDs
void show_x()
{
    // Desenhar um "X" nas diagonais da matriz
    for (int i = 0; i < 5; i++)
    {
        set_led_color(i, i, 0x00FF00 << 8);     // Diagonal principal
        set_led_color(i, 4 - i, 0x00FF00 << 8); // Diagonal secundária
    }
    update_leds(pio0, 0); // Atualiza a matriz com os LEDs acesos
    sleep_ms(1000);       // Pausa para o "X" ser visível
    // Apaga os LEDs após a exibição do "X"
    for (int i = 0; i < 5; i++)
    {
        set_led_color(i, i, 0x000000 << 8);     // Apaga diagonal principal
        set_led_color(i, 4 - i, 0x000000 << 8); // Apaga diagonal secundária
    }
    update_leds(pio0, 0); // Atualiza a matriz para apagar os LEDs
}

// Função principal do jogo da memória
void game_loop()
{
    sound_game_start(); // Toca o som ao iniciar o jogo
    int sequence[20];   // Armazena a sequência dos LEDs piscando
    int sequence_length = 1;
    int x = 0, y = 0;

    while ( iniciar)
    {
        printf("inicio\n");
        if (sequence_length > 1 && sequence_length < 20)
        {
            char buffer[30];
            sprintf(buffer, "%d pontos", (sequence_length - 1) * 10);
            update_display("voce possui ", buffer);
        }
        else if (sequence_length >= 20)
        {
            update_display("voce venceu ", "parabens");
            sound_game_win();
            
            sleep_ms(1000);
            break;
        }
        else
        {
            update_display("repita a", "sequencia ");
        }
        // Gerar sequência de LEDs aleatórios
        sequence[sequence_length - 1] = rand() % QTD_LEDS;
        int seq_x;
        int seq_y;

        // Mostrar sequência de LEDs piscando
        for (int i = 0; i < sequence_length; i++)
        {
            int led_index = sequence[i];
            seq_x = 4 - led_index / 5;
            seq_y = led_index % 5;

            if (seq_x == 0 || seq_x % 2 == 0)
            {
                seq_y = 4 - seq_y;
            }

            set_led_color(seq_x, seq_y, 0xFF00FF << 8); // Pisca o LED com cor vermelha
            update_leds(pio0, 0);                       // Atualiza a matriz de LEDs
            sleep_ms(500);                              // Pausa entre os LEDs
            set_led_color(seq_x, seq_y, 0x000000 << 8); // Apaga o LED
            update_leds(pio0, 0);                       // Atualiza novamente a matriz de LEDs
            sleep_ms(500);                              // Pausa entre os LEDs
        }

        // Agora o jogador precisa inserir a sequência completa
        int player_sequence[10]; // Sequência que o jogador escolheu
        int player_index = 0;    // Índice do jogador na sequência

        while (player_index < sequence_length)
        {
            move_led_with_joystick(&x, &y);

            // Apaga todos os LEDs (se necessário)
            for (int i = 0; i < QTD_LEDS; i++)
            {
                buffer_led[i] = 0x000000 << 8; // Cor apagada (preto)
            }

            // Mapeando a posição do joystick para LED
            set_led_color(x, y, 0xFF0000 << 8); // Coloca o LED verde na posição
            update_leds(pio0, 0);

            printf("pressionou %d %d %d %d\n", x, y, mapeamento_led[x][y], sequence[sequence_length - 1]);

            // Esperar o botão ser pressionado para confirmar
            if (gpio_get(BUTTON_PIN) == 0)
            {
                sound_button_press();
                for (int i = 0; i < QTD_LEDS; i++)
                {
                    buffer_led[i] = 0x000000 << 8; // Cor apagada (preto)
                }
                update_leds(pio0, 0);
                // sleep_ms(100);

                // Armazenar a escolha do jogador
                player_sequence[player_index] = mapeamento_led[x][y];

                // Verificar se o LED selecionado corresponde ao da sequência
                if (player_sequence[player_index] == sequence[player_index])
                {
                    player_index++;
                }
                else
                {
                    update_display("game over", "Resetando jogo");
                    printf("game over.\n");
                    sound_game_over(); // Toca o som de erro
                    iniciar= 0;

                    // Se o jogador errar a sequência, reiniciar o jogo
                    show_x();            // Mostrar "X" antes de reiniciar
                    sequence_length = 1; // Reiniciar o jogo
                    break;
                }
            }
        }

        // Se o jogador acertar a sequência, aumentar o tamanho da sequência
        if (player_index == sequence_length)
        {
            sequence_length++; // Aumenta a sequência
            printf("Acertou a sequência! Nova sequência com %d LEDs.\n", sequence_length);
        }

        sleep_ms(1000); // Pausa entre os turnos
    }
}
void init_buzzer()
{
    gpio_init(BUZZER_PIN);              // Inicializa o pino do buzzer
    gpio_set_dir(BUZZER_PIN, GPIO_OUT); // Define o pino como saída
}
void init_display_oled()
{
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    ssd1306_init(&ssd, 128, 64, false, OLED_ADDR, I2C_PORT);
    ssd1306_fill(&ssd, false);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);
}
void update_display(char *linha1, char *linha2)
{
    if (linha1 == NULL)
    {
        linha1 = "";
    }
    if (linha2 == NULL)
    {
        linha2 = "";
    }

    ssd1306_fill(&ssd, false); // limpa buffer
    ssd1306_draw_string(&ssd, linha1, 2, 20);
    ssd1306_draw_string(&ssd, linha2, 2, 40);
    ssd1306_rect(&ssd, 0, 0, 127, 63, 1, false);
    ssd1306_send_data(&ssd); // envia
}
bool debounce_bt(uint pino, absolute_time_t *ultimo_tempo)
{
    absolute_time_t agora = get_absolute_time();
    if (absolute_time_diff_us(*ultimo_tempo, agora) >= 200000)
    {
        *ultimo_tempo = agora;
        return (gpio_get(pino) == 0);
    }
    return false;
}

void gpio_irq_handler(uint gpio, uint32_t events) // callback da interrupção
{
    printf("gpio_irq_handler");
    if (events & GPIO_IRQ_EDGE_FALL) // borda de decida
    {

        if (gpio == BOTAO_A)
        {
            if (debounce_bt(BOTAO_A, &debounce_A))
            {
                iniciar = 1;
                printf("game loop");
            }
        }
    }
    printf("gpio_irq_handler2");
}

void config_button_a_irq()
{
    // inicia e configura o botão A
    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);
    gpio_set_irq_enabled_with_callback(BOTAO_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler); // configura a interrupção do botão e define o callback
}

int main()
{
    stdio_init_all();
    config_button_a_irq();
    init_ws2812();
    init_joystick_button();
    init_display_oled();
    update_display("Bem vindo", "iniciando jogo");
    debounce_A = get_absolute_time();
    debounce_B = get_absolute_time();
   
    // Inicializa todos os LEDs apagados no buffer
    for (int i = 0; i < QTD_LEDS; i++)
    {
        buffer_led[i] = 0x000000 << 8; // Cor apagada (preto)
    }
    sleep_ms(1000);
    update_display("pressione a", "para iniciar");
    while(1){
        if (iniciar)
        {
            game_loop();
        }else{
            update_display("pressione a", "para iniciar");
        }
        
    }
    

    return 0;
}
