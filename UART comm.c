#include "LCD_display.h"
#include "driver/gpio.h"
#include "soc/gpio_reg.h"
#include <stdio.h> // Para snprintf
// Dá erro porque primeiro deve usar o CMake, se adicionar componente
// novo tem que usar project - clean

volatile int interruption_flag = 0;
volatile int count = 0;

void delay(int ms) {
   for (volatile int i = 0; i < ms * 13200; i++);
}

static void interruption(void* arg) {
	interruption_flag = 1;
}

void app_main(void)
{
	// Configurar pinos de inicialização
	gpio_config_t io_conf; // Inicializa uma instância da estrutura
	io_conf.intr_type = GPIO_INTR_POSEDGE; // Interrupcao na borda de subida
	io_conf.pin_bit_mask = (1 << 10); // Máscara para GPIO10
	io_conf.mode = GPIO_MODE_INPUT; // Configura a porta como entrada
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE; // Habilita o pull-down
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE; // Desabilita o pull-up
	gpio_config(&io_conf); // Atribui as configurações
	gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
	gpio_isr_handler_add(10, interruption, NULL);

	// Inicializa o LCD e indica ao usuário
	char string1 [17] = "Interruption";
	char string2 [17] = "Disable";
	LCD_inicializa_4_bits(8, 9, 0, 1, 2, 3);
	LCD_escreve_strings(string1, string2);

    while (1) {
    	if (interruption_flag) {
    		count = 0; // Zera o contador ao detectar a interrupção
    		interruption_flag = 0;
			LCD_escreve_strings("Interruption", "enable");
    	}
    	delay(1000); // Delay para atualização visual
    	count++;
    	string1 [17] = "Interruption";
    	// Converte o valor de count para string
    	snprintf(string2, 17, "Disable Time:%d", count);
    	// Exibe a contagem no LCD
    	LCD_escreve_strings(string1, string2); // Mostra a contagem na primeira linha e deixa a segunda linha em branco
    }
}


/*
 * REFERÊNCIA
 * https://docs.espressif.com/projects/esp-idf/en/v5.2.3/esp32c3/index.html
 * Para a biblioteca LCD_display, adicionar pasta "components" no projeto (junto
 * de build e main), então colocar a pasta LCD_display (no moodle) dentro
 *
 *
 * --- FUNÇÕES DE INTERRUPÇÃO ---
 * Devem ser rápidas, exceções, tratadas e resolvidas
 * Não usar delay ou printf nesses casos, caso seja algo demorado de tratar lidar
 * na função principal, pois interrupção para tudo, par ao fluxo ser rápido
 *
 * --- ARQUITETURAS DE MICROPROCESSADORES ---
 *
 * - VON NEUMANN: Um barramento para comunicar com dados e instruções
 * CPU <-> DADOS E INSTRUÇÕES
 * Prós: Pode controlar se uso mais dados e menos instruções, versátil
 * Problema: Mics em geral fazem várias instruções, adiantam as próximas, isso
 * não é possivel no caso
 *
 * - HARVARD
 * MEM INST. <-> CPU <-> MEM. DADOS
 * Prós: Pode ler próxima instrução sem parar dados
 * Problema: Valor fixo para armazenar ambos
 *
 *
 * --- TIPOS DE INTERRUPÇÃO ---
 *
 * POLLING: Vê sensor por sensor o estado
 * EXTERNA: Meio de hardware para, quanto ocorrer interrupção, parar e atendê-la
 * EXCEÇÕES: Overflow, falha de página (acessar memória inexistente)
 *
 *
 * --- CONTROLE DE ERROS ---
 *
 * BUTTON BOUNCE: Erro ao apertar o botão
 *
 *    :  BOUNCE : :  BUTTON  : :  BOUNCE :
 *    :_   _   _: :__________: :_   _   _:
 * ___| |_| |_| |_|          |_| |_| |_| |___
 *
 * RESISTOR DE PULL-DOWN: Quando conecto botão em 3.3V e placa e ele está aberto,
 * não sei que tensão vem da placa. O resistor funciona para "acertar" a tensão
 * no lado da placa, sendo GND
 *
 *  _______
 * | ESP32 |_____---___ +3.3V
 * |_______|   |
 *             |__/\/\/\__ GND
 *
 * RESISTOR DE PULL-UP: Ao contrário de pull-down, faz a referência ser positiva
 *
 *  _______
 * | ESP32 |_____---___ GND
 * |_______|   |
 *             |__/\/\/\__ +3.3V
 *
 */

