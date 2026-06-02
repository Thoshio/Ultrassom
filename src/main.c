#include <zephyr/kernel.h>             // Funções básicas do Zephyr (ex: k_msleep, k_thread, etc.)
#include <zephyr/device.h>             // API para obter e utilizar dispositivos do sistema
#include <zephyr/drivers/gpio.h>       // API para controle de pinos de entrada/saída (GPIO)
#include <pwm_z402.h>                  // Biblioteca personalizada com funções de controle do TPM (Timer/PWM Module)

// Define o valor do registrador MOD do TPM para configurar o período do PWM
#define TPM_MODULE 30                // Define a frequência do PWM fpwm = (TPM_CLK / (TPM_MODULE * PS)) para um periodo de 40us
#define DUTY_CYCLE TPM_MODULE/4      // Define duty_cycle em 50%  ->  pulsos duram 20us 

#define TPM_IRQ_LINE TPM1_IRQn  // relaciona a interrupção ao timer TPM1
#define TPM_IRQ_PRIORITY 1      // define a prioridade da interrupção

#define VELOCIDADE_SOM 34300      // cm/s

volatile uint16_t captured = 0;
volatile uint16_t captured_subida = 0; 
volatile bool esperando_descida = false; // Controla se estamos medindo subida ou descida
volatile bool nova_leitura_pronta = false; // Avisa o main que há um dado novo
volatile uint16_t delta = 0; 


void tpm1_isr(void *arg)
{   
    TPM1->STATUS |= TPM_STATUS_CH0F_MASK; // zerra a flag que gerou a interrupção

    captured = TPM1->CONTROLS[0].CnV; // coloca o valor atual do timer na variável "captured"

    if (!esperando_descida) {
        captured_subida = captured;
        esperando_descida = true;
    } else {
        delta = captured-captured_subida;
        nova_leitura_pronta = true;
        esperando_descida = false;
    }
}


int main(void)
{

    //trigger TPM2 Ch:0 PTB2
    pwm_tpm_Init(TPM2, TPM_PLLFLL, TPM_MODULE, TPM_CLK, PS_128, EDGE_PWM);
    pwm_tpm_Ch_Init(TPM2, 0, TPM_PWM_H, GPIOB, 2);
    pwm_tpm_CnV(TPM2, 0, DUTY_CYCLE);

    // Conecta a interrupção via Zephyr
    IRQ_CONNECT(TPM_IRQ_LINE, TPM_IRQ_PRIORITY, tpm1_isr, NULL, 0);
    irq_enable(TPM_IRQ_LINE);
 
    // Inicializa TPM1 com módulo e prescaler desejado
    pwm_tpm_Init(TPM1, TPM_PLLFLL, 65535, TPM_CLK, PS_128, EDGE_PWM);

    // Configura TPM1_CH0 como input capture na borda de subida e descida em PTB0
    pwm_tpm_Ch_Init(TPM1, 0, TPM_INPUT_CAPTURE_BOTH|TPM_CHANNEL_INTERRUPT, GPIOB, 0);

    //////////////////////////////////////////////////////////////////////////////////////////

    for(;;) {
        // Imprime o valor no console
        if (nova_leitura_pronta) {
            nova_leitura_pronta = false;
            float tempo_segundos = (float)delta * 128.0f / 48000000;
            float dist = tempo_segundos * VELOCIDADE_SOM / 2;
            printk("Objeto a: %f cm\n", (double)dist);
        }
        k_msleep(3000);
        
    }
    
    return 0;
}