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
#define BOTAO_PINO_B 6

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
    ssd1306_t display;
    bool inverter_cores = true;

    stdio_init_all();

    // Configuração inicial
    configurar_pinos_e_perifericos(&display);
    
    gpio_set_irq_enabled_with_callback(PINO_BOTAO, GPIO_IRQ_EDGE_FALL, true, &tratar_pressao_botao);
    gpio_set_irq_enabled_with_callback(BOTAO_PINO_A, GPIO_IRQ_EDGE_FALL, true, &tratar_pressao_botao);
    gpio_set_irq_enabled_with_callback(BOTAO_PINO_B, GPIO_IRQ_EDGE_FALL, true, &tratar_pressao_botao);

    // Loop principal
    // Loop principal
    while (true) {
        ler_joystick(&joystick_x_raw, &joystick_y_raw);
        mapear_joystick_para_tela(joystick_x_raw, joystick_y_raw, &posicao_x, &posicao_y);

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
        ssd1306_fill(&display, false);

        // Desenha as linhas verticais apenas se 'linhas_verticais_visiveis' for true
        if (linhas_verticais_visiveis) {
            // Linha na lateral esquerda
            ssd1306_vline(&display, 0, 0, display.height - 1, true); // Lateral esquerda

            // Linha na lateral direita
            ssd1306_vline(&display, display.width - 1, 0, display.height - 1, true); // Lateral direita
        }

        ssd1306_rect(&display, (int)posicao_y, (int)posicao_x, 8, 8, inverter_cores, true);
        ssd1306_send_data(&display);

        sleep_ms(100);
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

    gpio_init(BOTAO_PINO_B);
    gpio_set_dir(BOTAO_PINO_B, GPIO_IN);
    gpio_pull_up(BOTAO_PINO_B);

    gpio_init(PINO_BOTAO);
    gpio_set_dir(PINO_BOTAO, GPIO_IN);
    gpio_pull_up(PINO_BOTAO);

    // Configura comunicação I2C
    i2c_init(PORTA_I2C, 400 * 1000);
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
    adc_select_input(1); // Eixo X
    *x = adc_read();
    adc_select_input(0); // Eixo Y
    *y = adc_read();
}

// Mapeia os valores do joystick para a tela usando if-else
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
    uint32_t tempo_atual = to_us_since_boot(get_absolute_time());

    if (pino == PINO_BOTAO && (tempo_atual - ultimo_tempo_botao_joystick > 250000)) {
        ultimo_tempo_botao_joystick = tempo_atual;

        estado_botao_joystick = !estado_botao_joystick;

        if (!estado_led_verde) {
            pwm_set_gpio_level(LED_PINO_VERDE, CENTRO_ADC);
            estado_led_verde = true;
        } else {
            pwm_set_gpio_level(LED_PINO_VERDE, 0);
            estado_led_verde = false;
        }
        linhas_verticais_visiveis = !linhas_verticais_visiveis;
    } else if (pino == BOTAO_PINO_A && (tempo_atual - ultimo_tempo_botao_a > 250000)) {
        ultimo_tempo_botao_a = tempo_atual;

        if (!pwm_ativo) {
            pwm_set_gpio_level(LED_PINO_VERDE, 0);
            pwm_set_gpio_level(LED_PINO_AZUL, 0);
            pwm_set_gpio_level(LED_PINO_VERMELHO, 0);
        }

        pwm_set_enabled(pwm_slice_verde, !pwm_ativo);
        pwm_set_enabled(pwm_slice_azul, !pwm_ativo);
        pwm_set_enabled(pwm_slice_vermelho, !pwm_ativo);

        pwm_ativo = !pwm_ativo;
    } 
}


