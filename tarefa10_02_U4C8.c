#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "src/font.h"
#include "src/ssd1306.h"

// Definições de pinos e parâmetros
#define PORTA_I2C i2c1
#define ENDERECO_OLED 0x3C
#define PINO_I2C_DADOS 14
#define PINO_I2C_CLK 15

#define LED_PINO_VERDE 11
#define LED_PINO_AZUL 12
#define LED_PINO_VERMELHO 13

#define BOTAO_PINO_A 5

#define JOYSTICK_PINO_X 27
#define JOYSTICK_PINO_Y 26
#define PINO_BOTAO 22
#define LARGURA_TELA 128
#define ALTURA_TELA 64
#define RESOLUCAO_ADC 4096
#define CENTRO_ADC 2048
#define LIMITE_JOYSTICK 100

// Variáveis de controle
static volatile bool estado_botao_joystick = true;
static volatile bool estado_led_verde = false;
static volatile uint32_t ultimo_tempo_botao_a = 0;
static volatile uint32_t ultimo_tempo_botao_joystick = 0;
static volatile uint pwm_slice_verde;
static volatile uint pwm_slice_azul;
static volatile uint pwm_slice_vermelho;
static volatile bool linha_na_parte_inferior = false;
static volatile int modo_brilho = 1; 
static volatile bool pwm_ativo = true;
static volatile bool linhas_verticais_visiveis = true;

// Funções auxiliares
void configurar_pinos_e_perifericos(ssd1306_t *display);
void ler_joystick(uint16_t *x, uint16_t *y);
void mapear_joystick_para_tela(uint16_t x_raw, uint16_t y_raw, float *x, float *y);
void tratar_pressao_botao(uint pino, uint32_t eventos);

// Função principal
int main() {
    uint16_t joystick_x_raw, joystick_y_raw;
    float posicao_x, posicao_y;
    bool inverter_cores = true;

    ssd1306_t display;

    stdio_init_all(); // Inicializa a comunicação serial para debug

    // Configuração inicial dos pinos e periféricos
    configurar_pinos_e_perifericos(&display);
    
    // Configura interrupções para os botões
    gpio_set_irq_enabled_with_callback(PINO_BOTAO, GPIO_IRQ_EDGE_FALL, true, &tratar_pressao_botao);
    gpio_set_irq_enabled_with_callback(BOTAO_PINO_A, GPIO_IRQ_EDGE_FALL, true, &tratar_pressao_botao);

    // Loop principal
    while (true) {
        // Lê os valores do joystick
        ler_joystick(&joystick_x_raw, &joystick_y_raw);
        // Mapeia os valores do joystick para a tela
        mapear_joystick_para_tela(joystick_x_raw, joystick_y_raw, &posicao_x, &posicao_y);

        // Exibe os valores do joystick e o estado do botão no console
        printf("Joystick X: %u, Y: %u, Botão: %d\n", joystick_x_raw, joystick_y_raw, estado_botao_joystick);

        // Controle dos LEDs com base no joystick
        if (joystick_x_raw < CENTRO_ADC - LIMITE_JOYSTICK) {
            pwm_set_gpio_level(LED_PINO_VERMELHO, RESOLUCAO_ADC * (joystick_x_raw / (float)RESOLUCAO_ADC) * modo_brilho);
        } else {
            if (joystick_x_raw > CENTRO_ADC + LIMITE_JOYSTICK) {
                pwm_set_gpio_level(LED_PINO_VERMELHO, RESOLUCAO_ADC * (joystick_x_raw / (float)RESOLUCAO_ADC) * modo_brilho);
            } else {
                pwm_set_gpio_level(LED_PINO_VERMELHO, 0);
            }
        }

        if (joystick_y_raw < CENTRO_ADC - LIMITE_JOYSTICK) {
            pwm_set_gpio_level(LED_PINO_AZUL, RESOLUCAO_ADC * (joystick_y_raw / (float)RESOLUCAO_ADC) * modo_brilho);
        } else {
            if (joystick_y_raw > CENTRO_ADC + LIMITE_JOYSTICK) {
                pwm_set_gpio_level(LED_PINO_AZUL, RESOLUCAO_ADC * (joystick_y_raw / (float)RESOLUCAO_ADC) * modo_brilho);
            } else {
                pwm_set_gpio_level(LED_PINO_AZUL, 0);
            }
        }

        // Atualização do display
        ssd1306_fill(&display, false); // Limpa o display

        // Desenha as linhas verticais apenas se 'linhas_verticais_visiveis' for true
        if (linhas_verticais_visiveis) {
            ssd1306_vline(&display, 0, 0, display.height - 1, true); // Linha na lateral esquerda
            ssd1306_vline(&display, display.width - 1, 0, display.height - 1, true); // Linha na lateral direita
        }

        // Desenha um retângulo na posição mapeada do joystick
        ssd1306_rect(&display, (int)posicao_y, (int)posicao_x, 8, 8, inverter_cores, true);
        ssd1306_send_data(&display); // Envia os dados para o display

        sleep_ms(100); // Aguarda 100ms para a próxima iteração
    }
}

// Função que configura todos os pinos e periféricos de uma vez
void configurar_pinos_e_perifericos(ssd1306_t *display) {
    // Configura PWM para LEDs
    gpio_set_function(LED_PINO_VERDE, GPIO_FUNC_PWM);
    pwm_slice_verde = pwm_gpio_to_slice_num(LED_PINO_VERDE);
    pwm_set_wrap(pwm_slice_verde, RESOLUCAO_ADC);
    pwm_set_enabled(pwm_slice_verde, true);

    gpio_set_function(LED_PINO_AZUL, GPIO_FUNC_PWM);
    pwm_slice_azul = pwm_gpio_to_slice_num(LED_PINO_AZUL);
    pwm_set_wrap(pwm_slice_azul, RESOLUCAO_ADC);
    pwm_set_enabled(pwm_slice_azul, true);

    gpio_set_function(LED_PINO_VERMELHO, GPIO_FUNC_PWM);
    pwm_slice_vermelho = pwm_gpio_to_slice_num(LED_PINO_VERMELHO);
    pwm_set_wrap(pwm_slice_vermelho, RESOLUCAO_ADC);
    pwm_set_enabled(pwm_slice_vermelho, true);

    // Configura os botões
    gpio_init(BOTAO_PINO_A);
    gpio_set_dir(BOTAO_PINO_A, GPIO_IN);
    gpio_pull_up(BOTAO_PINO_A);

    gpio_init(PINO_BOTAO);
    gpio_set_dir(PINO_BOTAO, GPIO_IN);
    gpio_pull_up(PINO_BOTAO);

    // Configura comunicação I2C
    i2c_init(PORTA_I2C, 400 * 1000); // Inicializa I2C com 400kHz
    gpio_set_function(PINO_I2C_DADOS, GPIO_FUNC_I2C);
    gpio_set_function(PINO_I2C_CLK, GPIO_FUNC_I2C);
    gpio_pull_up(PINO_I2C_DADOS);
    gpio_pull_up(PINO_I2C_CLK);

    // Configura o display OLED
    ssd1306_init(display, LARGURA_TELA, ALTURA_TELA, false, ENDERECO_OLED, PORTA_I2C);
    ssd1306_config(display);
    ssd1306_send_data(display);
    ssd1306_fill(display, false);
    ssd1306_send_data(display);

    // Configura o ADC e o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_PINO_X);
    adc_gpio_init(JOYSTICK_PINO_Y);
}

// Lê os valores do joystick
void ler_joystick(uint16_t *x, uint16_t *y) {
    adc_select_input(1); // Seleciona o eixo X
    *x = adc_read(); // Lê o valor do eixo X
    adc_select_input(0); // Seleciona o eixo Y
    *y = adc_read(); // Lê o valor do eixo Y
}

// Mapeia os valores do joystick para a tela
void mapear_joystick_para_tela(uint16_t x_raw, uint16_t y_raw, float *x, float *y) {
    if (x_raw < 0) {
        *x = 0;
    } else if (x_raw > RESOLUCAO_ADC) {
        *x = LARGURA_TELA;
    } else {
        *x = LARGURA_TELA * (float)x_raw / RESOLUCAO_ADC;
    }

    if (y_raw < 0) {
        *y = 0;
    } else if (y_raw > RESOLUCAO_ADC) {
        *y = ALTURA_TELA;
    } else {
        *y = ALTURA_TELA * (1 - (float)y_raw / RESOLUCAO_ADC);
    }

    // Limite para que o objeto não saia da tela
    if (*x > LARGURA_TELA - 8) {
        *x = LARGURA_TELA - 8;
    }
    if (*y > ALTURA_TELA - 8) {
        *y = ALTURA_TELA - 8;
    }
}

// Manipula as interrupções dos botões
void tratar_pressao_botao(uint pino, uint32_t eventos) {
    // Obtém o tempo atual em microssegundos desde o boot do sistema
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    // Verifica se o botão pressionado foi o botão do joystick (PINO_BOTAO)
    // e se o tempo desde a última pressão foi maior que 250ms (debounce)
    if (pino == PINO_BOTAO && (tempo_atual - ultimo_tempo_botao_joystick > 250000)) {
        // Atualiza o tempo da última pressão do botão do joystick
        ultimo_tempo_botao_joystick = tempo_atual;

        // Inverte o estado do botão do joystick (toggle)
        estado_botao_joystick = !estado_botao_joystick;

        // Verifica o estado do LED verde para alternar seu estado
        if (!estado_led_verde) {
            // Se o LED verde estiver apagado, acende com intensidade definida por CENTRO_ADC
            pwm_set_gpio_level(LED_PINO_VERDE, CENTRO_ADC);
            estado_led_verde = true; // Atualiza o estado do LED verde para "aceso"
        } else {
            // Se o LED verde estiver aceso, apaga
            pwm_set_gpio_level(LED_PINO_VERDE, 0);
            estado_led_verde = false; // Atualiza o estado do LED verde para "apagado"
        }

        // Alterna a visibilidade das linhas verticais no display
        linhas_verticais_visiveis = !linhas_verticais_visiveis;
    }
    // Verifica se o botão pressionado foi o botão A (BOTAO_PINO_A)
    // e se o tempo desde a última pressão foi maior que 250ms (debounce)
    else if (pino == BOTAO_PINO_A && (tempo_atual - ultimo_tempo_botao_a > 250000)) {
        // Atualiza o tempo da última pressão do botão A
        ultimo_tempo_botao_a = tempo_atual;

        // Se o PWM estiver desativado, apaga todos os LEDs antes de alternar o estado do PWM
        if (!pwm_ativo) {
            pwm_set_gpio_level(LED_PINO_VERDE, 0); // Apaga o LED verde
            pwm_set_gpio_level(LED_PINO_AZUL, 0);  // Apaga o LED azul
            pwm_set_gpio_level(LED_PINO_VERMELHO, 0); // Apaga o LED vermelho
        }

        // Alterna o estado do PWM (ativa/desativa) para todos os LEDs
        pwm_set_enabled(pwm_slice_verde, !pwm_ativo);    // Alterna o PWM do LED verde
        pwm_set_enabled(pwm_slice_azul, !pwm_ativo);      // Alterna o PWM do LED azul
        pwm_set_enabled(pwm_slice_vermelho, !pwm_ativo);  // Alterna o PWM do LED vermelho

        // Inverte o estado do PWM (ativo/inativo)
        pwm_ativo = !pwm_ativo;
    }
}