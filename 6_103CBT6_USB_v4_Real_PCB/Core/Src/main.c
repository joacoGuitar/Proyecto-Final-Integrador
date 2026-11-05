/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usb_device.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

// #include <stdio.h>
#include <string.h>
#include "usbd_hid.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef union {
    struct __attribute__((packed)) {
        uint8_t square     :1;
        uint8_t x          :1;
        uint8_t o          :1;
        uint8_t triangle   :1;
        uint8_t L1         :1;
        uint8_t R1         :1;
        uint8_t L2         :1;
        uint8_t R2         :1;

        uint8_t select     :1;
        uint8_t start      :1;
        uint8_t L3         :1;
        uint8_t R3         :1;
        uint8_t home       :1; // este lo usaré de Analog_ON
        uint8_t            :3;

        uint8_t hat_switch :4;
        uint8_t            :4;

        uint16_t X;
        uint16_t Y;
        uint16_t Z;
        uint16_t Rz;
    };
    uint8_t val[11];
} Joystick_Report;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

#define ADC_NUM_CHANNELS 4

#define HAT_SWITCH_AR       0x0
#define HAT_SWITCH_AR_DER	0x1
#define HAT_SWITCH_DER      0x2
#define HAT_SWITCH_AB_DER 	0x3
#define HAT_SWITCH_AB       0x4
#define HAT_SWITCH_AB_IZQ 	0x5
#define HAT_SWITCH_IZQ 		0x6
#define HAT_SWITCH_AR_IZQ 	0x7
#define HAT_SWITCH_NULL     0x8

#define DEBUG_1 0

// #define CANT_BOTONES 11 // Medidos en la STM
#define CANT_BOTONES 18 // Medidos en la STM
#define CANT_BOTONES_TOTALES 18
#define DEBOUNCE_TIME_MS 10

#define BUFFER_SIZE_RX 20 // > al necesario por las dudas
#define REPORT_BUFFER_SIZE 11 // tamaño del Report struct
// #define RX_SIZE 8 // Recibo 6 botones (uso 1 byte por bit por facilidad) + el ID
#define RX_SIZE 3 // Recibo 7 botones (en 1 byte) + el ID
#define TX_SIZE 13 // envio el struct + ID

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim3;
TIM_HandleTypeDef htim4;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
DMA_HandleTypeDef hdma_usart1_tx;

/* USER CODE BEGIN PV */


extern USBD_HandleTypeDef hUsbDeviceFS;

volatile uint16_t ADC_Buffer[ADC_NUM_CHANNELS] = {0};
// volatile uint32_t Flag_ADC = 0;
volatile uint32_t Flag_TIM1 = 0;
volatile uint32_t Flag_TIM2 = 0;
volatile uint32_t Flag_TIM4 = 0;
volatile uint32_t Flag_Analog_State = 0;
// volatile uint32_t Flag_Rx = 0;
volatile uint32_t Flag_ESP_Buttons = 0;
volatile uint32_t Flag_ESP_Sent_Report = 0;

Joystick_Report Joystick_Report_Struct;


uint32_t dt, t1 = 0, dt2, t2 = 0;

// los inicializo en la main()
volatile uint32_t button_last_time[CANT_BOTONES_TOTALES];
volatile uint8_t button_state[CANT_BOTONES_TOTALES];
volatile uint8_t button_debounced[CANT_BOTONES_TOTALES];

uint32_t USB_State;



// Pines de botones (ajusta según tu configuración)
const uint16_t BUTTON_PINS[CANT_BOTONES] = {
	// Action PAD
    Triang_Pin,
    Square_Pin,
    XX_Pin,
    Circle_Pin,

	// D-PAD
	Izq_Pin,
	Ar_Pin,
	Ab_Pin,
	Der_Pin,

	// Analogs SW
	Joy_Izq_Pin,
	Joy_Der_Pin,

	// Medium PAD
	V_USB_Sensing_Pin,
	Analog_ON_Pin, // Analog_ON_Pin

	// Triggers
    L1_Pin,
    R1_Pin,
    L2_Pin,
    R2_Pin,
    // L3_Pin,
    // R3_Pin,

	// Rest of Medium PAD
	Select_Pin,
	Start_Pin,

};
// Analog L1 RI1 L2 R2 Select Start

GPIO_TypeDef* BUTTON_PORTS[CANT_BOTONES] = {
	// Action PAD
    Triang_GPIO_Port,
    Square_GPIO_Port,
    XX_GPIO_Port,
    Circle_GPIO_Port,
	// D-PAD
	Izq_GPIO_Port, 	// 4
	Ar_GPIO_Port,	// 5
	Ab_GPIO_Port,	// 6
	Der_GPIO_Port,	// 7
	// Analogs SW
	Joy_Izq_GPIO_Port, // 8
	Joy_Der_GPIO_Port, // 9
	// Medium PAD
	V_USB_Sensing_GPIO_Port, // 10
	Analog_ON_GPIO_Port, // 11

	// Triggers
    L1_GPIO_Port, // 12
    R1_GPIO_Port, // 13
    L2_GPIO_Port, // 14
    R2_GPIO_Port, // 15

	Select_GPIO_Port, // 16
	Start_GPIO_Port, // 17
};

typedef enum {
    TRIANG = 0,        // EXTI 11
    SQUARE = 1,        // EXTI 10
    XX = 2,            // EXTI 12
    CIRCLE = 3,        // EXTI 13
    IZQ = 4,           // EXTI 6
    AR = 5,            // EXTI 7
    AB = 6,            // EXTI 0
    DER = 7,           // EXTI 1
    JOY_IZQ = 8,       // EXTI 2
    JOY_DER = 9,       // EXTI 5
    V_USB_SENSING = 10,// GPIO Input
    ANALOG_ON = 11,    // EXTI 4
    L1 = 12,           // EXTI 8
    R1 = 13,           // EXTI 15
    L2 = 14,           // EXTI 9
    R2 = 15,           // EXTI 14
    SELECT = 16,       // GPIO Input
    START = 17         // EXTI 3
} Button_t;

// uint32_t Flag_ADC = 0;

// const uint8_t USB_ID = 0x55;
// const uint8_t Buttons_ID[2] = {0x55, 0x55};
const uint8_t BT_ID[2] = {0xAA, 0xAA};
// uint8_t COM_ID; // Toma el valor del ID según la alimentación

uint8_t buffer_Rx[BUFFER_SIZE_RX];
uint8_t Rx;
uint32_t pos_Rx = 0;

volatile uint32_t Analog_last = 0; // Para detemrinar si se está con analógicos ON u OFF

uint32_t to = 0;
uint32_t Flag_Latch_In = 0;
uint32_t cont_to = 0;


/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_ADC1_Init(void);
static void MX_TIM3_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_TIM1_Init(void);
static void MX_TIM2_Init(void);
static void MX_TIM4_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {

	/*
	if (GPIO_Pin == V_USB_Sensing_Pin)
		USB_State = HAL_GPIO_ReadPin(V_USB_Sensing_GPIO_Port, V_USB_Sensing_Pin);
*/

	uint32_t i;
	for (i = 0; i < CANT_BOTONES; i++) {
		if (GPIO_Pin == BUTTON_PINS[i]) {
			button_last_time[i] = HAL_GetTick();
			button_state[i] = HAL_GPIO_ReadPin(BUTTON_PORTS[i], BUTTON_PINS[i]);
		}

		/*
		if(GPIO_Pin == Circle_Pin)
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
		*/
	}
}

void handle_button_press(uint8_t button_id, uint8_t state) {
	static uint32_t par = 0;

    switch(button_id) {
        case TRIANG: // Acción para botón 0
        	Joystick_Report_Struct.triangle = state;
            break;
        case SQUARE: // Acción para botón 1
        	Joystick_Report_Struct.square = state;
            break;
        case XX:
			Joystick_Report_Struct.x = state;
			break;
        case CIRCLE:
			Joystick_Report_Struct.o = state;
			break;
        // D-PAD
        case IZQ:
        case AR:
        case AB:
        case DER:
        	uint8_t left  = !button_debounced[IZQ];
        	uint8_t up    = !button_debounced[AR];
            uint8_t down  = !button_debounced[AB];
            uint8_t right = !button_debounced[DER];

            // Detección de diagonales primero (para prioridad)
            if (up && left)
            	Joystick_Report_Struct.hat_switch = HAT_SWITCH_AR_IZQ;
            else if (up && right)
            	Joystick_Report_Struct.hat_switch = HAT_SWITCH_AR_DER;
            else if (down && left)
            	Joystick_Report_Struct.hat_switch = HAT_SWITCH_AB_IZQ;
            else if (down && right)
            	Joystick_Report_Struct.hat_switch = HAT_SWITCH_AB_DER;

            else if (left && right){
            	if(button_last_time[4] > button_last_time[7]) // izq se presionó despues
            		Joystick_Report_Struct.hat_switch = HAT_SWITCH_IZQ;
            	else
            		Joystick_Report_Struct.hat_switch = HAT_SWITCH_DER;
            }
            else if (up && down){
            	if(button_last_time[5] > button_last_time[6]) // ar se presionó despues
            		Joystick_Report_Struct.hat_switch = HAT_SWITCH_AR;
            	else
            		Joystick_Report_Struct.hat_switch = HAT_SWITCH_AB;
            }

            else if (up)
				Joystick_Report_Struct.hat_switch = HAT_SWITCH_AR;
			else if (down)
				Joystick_Report_Struct.hat_switch = HAT_SWITCH_AB;
			else if (left)
				Joystick_Report_Struct.hat_switch = HAT_SWITCH_IZQ;
			else if (right)
				Joystick_Report_Struct.hat_switch = HAT_SWITCH_DER;

			else
				Joystick_Report_Struct.hat_switch = HAT_SWITCH_NULL;
            break;
        case JOY_IZQ:
        	Joystick_Report_Struct.L3 = state;
        	break;
        case JOY_DER:
        	Joystick_Report_Struct.R3 = state;
        	break;
        case V_USB_SENSING:
        	USB_State = !state;
        	// Joystick_Report_Struct.start = state;
        	break;
        case ANALOG_ON:
        	Joystick_Report_Struct.home = state;
        	if(state == 0){
				cont_to = 0;
				Flag_Latch_In = 0;
			}
        	else{
        		Flag_Latch_In = 1;
        	}

        	if(par == 0 || par == 2) {
        		// Flag_Analog_State = 1; // 0 indica que no mando data de los analógicos
        		par = 0;
        		if(Analog_last == 0){ // los analógicos estaban encendidos
        			Analog_last++; // indico que los analógicos se van a apagar (lógica invertida)

        			HAL_ADC_Stop_DMA(&hadc1);
        			HAL_TIM_Base_Stop(&htim3);


        			Joystick_Report_Struct.X = 2048;
        			Joystick_Report_Struct.Y = 2048;

        			Joystick_Report_Struct.Z = 2048;
        			Joystick_Report_Struct.Rz = 2048;

        			// HAL_UART_Receive_DMA(&huart1, &Rx, 1);

        			HAL_TIM_Base_Start_IT(&htim2);
  			  } else {
  				  Analog_last = 0;

  				  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_Buffer, ADC_NUM_CHANNELS);
  				  HAL_TIM_Base_Start(&htim3);
  				  HAL_TIM_Base_Stop_IT(&htim2);
  			  }
        	}
        	par++;

        	break;
        case L1:
        	Joystick_Report_Struct.L1 = state;
        	break;
        case R1:
        	Joystick_Report_Struct.R1 = state;
			break;
        case L2:
        	Joystick_Report_Struct.L2 = state;
			break;
        case R2:
        	Joystick_Report_Struct.R2 = state;
			break;
        case SELECT:
        	Joystick_Report_Struct.select = state;
			break;
        case START:
        	// USB_State = !state;
        	Joystick_Report_Struct.start = state;
        	break;

    }
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim){
	if(htim->Instance == TIM1){
		Flag_TIM1 = 1;
		/*
		static uint32_t debug = 0;
		debug++;
		if (debug > 100){
			HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			debug = 0;
		}
		*/
	}
	if(htim->Instance == TIM2)
		Flag_TIM2 = 1;
	if(htim->Instance == TIM4)
		Flag_TIM4 = 1;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* hadc) {

	if(Analog_last == 0){
		// dt = __HAL_TIM_GET_COUNTER(&htim1) - t1;
		// No vale la pena preguntar que ADC xq solo hay 1

		Joystick_Report_Struct.X = 4095 - ADC_Buffer[3];
		Joystick_Report_Struct.Y = 4095 - ADC_Buffer[2];

		Joystick_Report_Struct.Z = ADC_Buffer[1];
		Joystick_Report_Struct.Rz = 4095 - ADC_Buffer[0];


		// t1 = __HAL_TIM_GET_COUNTER(&htim1);
	}
	// Flag_ADC = 1;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

	if(huart == &huart1){

		buffer_Rx[pos_Rx] = Rx;
	  	pos_Rx++;
	  	pos_Rx %= BUFFER_SIZE_RX;

	  	uint32_t pos_aux = (pos_Rx - 2 + BUFFER_SIZE_RX)%BUFFER_SIZE_RX;

	  	// Buttons_ID = 0x5555
	  	/*
	  	if(buffer_Rx[pos_aux] == Buttons_ID[0] && buffer_Rx[pos_aux + 1] == Buttons_ID[1])
	  		Flag_ESP_Buttons = 1;

	  	// BT_ID = 0xAAAA
	  	else if(buffer_Rx[pos_aux] == BT_ID[0] && buffer_Rx[pos_aux + 1] == BT_ID[1])
	  	*/
	  	if(buffer_Rx[pos_aux] == BT_ID[0] && buffer_Rx[pos_aux + 1] == BT_ID[1])
	  		Flag_ESP_Sent_Report = 1;
	}
}


/*
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	volatile uint32_t Flag_UART = 1;
}
*/
/*
void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef* hadc)
{

}
*/


/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */



  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_TIM3_Init();
  MX_USART1_UART_Init();
  MX_USB_DEVICE_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM4_Init();
  /* USER CODE BEGIN 2 */

/*
  HAL_GPIO_WritePin(Latch_Out_GPIO_Port, Latch_Out_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(Latch_v2_GPIO_Port, Latch_v2_Pin, GPIO_PIN_SET);
*/
/*
  USB_State = HAL_GPIO_ReadPin(V_USB_Sensing_GPIO_Port, V_USB_Sensing_Pin);

  if(!USB_State){
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  HAL_Delay(2000);
  }

  if(USB_State){
	  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
	  HAL_Delay(4000);
  }
*/


  volatile uint32_t i;
  for(i = 0; i < CANT_BOTONES_TOTALES; i++){
	  button_state[i] = 1;
	  button_debounced[i] = 1;
	  button_last_time[i] = 1;
  }

  // HAL_ADCEx_Calibration_Start(&hadc);
  HAL_ADC_Start_DMA(&hadc1, (uint32_t *)ADC_Buffer, ADC_NUM_CHANNELS);
  HAL_TIM_Base_Start(&htim3);
  Joystick_Report_Struct.hat_switch = HAT_SWITCH_NULL;

  // No necesito que estos arrays sean tan largos, pero facilita el procesamiento en un solo for en vez de en distintos pedazos.
  volatile uint32_t button_now_state[CANT_BOTONES_TOTALES];
  for(i = 0; i < CANT_BOTONES_TOTALES; i++)
	  button_now_state[i] = 1;
  volatile uint32_t button_last_state[CANT_BOTONES_TOTALES];
  for(i = 0; i < CANT_BOTONES_TOTALES; i++)
	  button_last_state[i] = 1;

  uint8_t buffer_Tx[TX_SIZE];

  // uint32_t init_buffer;


  USB_State = !HAL_GPIO_ReadPin(V_USB_Sensing_GPIO_Port, V_USB_Sensing_Pin);
  	  // Lo niego xq el NMOS me habilita el 0 cuando el USB está conectado

  if(USB_State){
	  HAL_TIM_Base_Start_IT(&htim1); // Comanda c/ cuanto enviar datos si la ESP no está involucrada
  }
  if(!USB_State){
	  HAL_UART_Receive_DMA(&huart1, &Rx, 1);
	  HAL_UART_Transmit(&huart1, BT_ID, 2, HAL_MAX_DELAY);
  }


  HAL_TIM_Base_Start_IT(&htim4);

  HAL_Delay(2000);
  HAL_GPIO_WritePin(Latch_Out_GPIO_Port, Latch_Out_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  // HAL_GPIO_WritePin(Latch_v2_GPIO_Port, Latch_v2_Pin, GPIO_PIN_SET);

  /*
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
  HAL_Delay(500);
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
   */
  // uint32_t debug = 0;

  // HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*
  if(USB_State){
	  COM_ID = USB_ID;
	  // HAL_UART_Transmit(&huart1, (uint8_t *)&COM_ID, 1, HAL_MAX_DELAY);

	  	  // *** Poner la Rx en un vlaor solo y en la interrupción acomodarlo para poder analizarlo
  }
  else{
	  COM_ID = BT_ID;
	  // HAL_UART_Transmit(&huart1, (uint8_t *)&BT_ID, 1, HAL_MAX_DELAY);
  }*/



  // uint8_t tx_data[26] = "Message from STM32 ... \r\n";


  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
	  // Si se está usando USB

	  // if(Flag_ADC || Flag_TIM2){ // se envia data con Flag_ADC (Analogs activados) Flag_TIM2 (Analogs desactivados)


		if(Flag_ESP_Buttons || Flag_TIM1){
			  // como recibí detengo contador de tiempo muerto y lo pongo en 0
			Flag_TIM1 = 0;

			HAL_TIM_Base_Stop_IT(&htim4);
			__HAL_TIM_SET_COUNTER(&htim4, 0);

			if(USB_State)
				__HAL_TIM_SET_AUTORELOAD(&htim4, 4);
			else
				__HAL_TIM_SET_AUTORELOAD(&htim4, 65);

			// dt2 = __HAL_TIM_GET_COUNTER(&htim1) - t2;
			// t2 = __HAL_TIM_GET_COUNTER(&htim1);

			Flag_ESP_Buttons = 0;
			// Flag_ADC = 0;
			Flag_TIM2 = 0;

			// ------ Sensado botones que no son EXTI
			// GPIO_Inputs_Update();
			// button_now_state[6] = HAL_GPIO_ReadPin(Ab_GPIO_Port, Ab_Pin);
			// button_now_state[7] = HAL_GPIO_ReadPin(Der_GPIO_Port, Der_Pin);
			// button_now_state[CIRCLE] = HAL_GPIO_ReadPin(Circle_GPIO_Port, Circle_Pin);
			button_now_state[V_USB_SENSING] = HAL_GPIO_ReadPin(V_USB_Sensing_GPIO_Port, V_USB_Sensing_Pin);
			USB_State = !HAL_GPIO_ReadPin(V_USB_Sensing_GPIO_Port, V_USB_Sensing_Pin);
			button_now_state[SELECT] = HAL_GPIO_ReadPin(Select_GPIO_Port, Select_Pin);
			/*
			if(button_now_state[CIRCLE] != button_last_state[CIRCLE]){
				button_last_state[CIRCLE] = button_now_state[CIRCLE];
				button_last_time[CIRCLE] = HAL_GetTick();
				button_state[CIRCLE] = button_now_state[CIRCLE];
			}
			*/
			if(button_now_state[V_USB_SENSING] != button_last_state[V_USB_SENSING]){
				button_last_state[V_USB_SENSING] = button_now_state[V_USB_SENSING];
				button_last_time[V_USB_SENSING] = HAL_GetTick();
				button_state[V_USB_SENSING] = button_now_state[V_USB_SENSING];
			}

			if(button_now_state[SELECT] != button_last_state[SELECT]){
				button_last_state[SELECT] = button_now_state[SELECT];
				button_last_time[SELECT] = HAL_GetTick();
				button_state[SELECT] = button_now_state[SELECT];
			}


			/*
			// Botones: Ab, Der
			for(i = 6; i < 8; i++){
				if(button_now_state[i] != button_last_state[i]){
					button_last_state[i] = button_now_state[i];
					button_last_time[i] = HAL_GetTick();
					button_state[i] = button_now_state[i];
				}
			}
			*/

			//
			/* Este debounce ahora lo hace la ESP siempre
			if(USB_State){
			  // Botones: Analog_ON, L1, R1, L2, R2, Select
				for(i = 11; i < 17; i++){
					if(button_now_state[i] != button_last_state[i]){
						button_last_state[i] = button_now_state[i];
						button_last_time[i] = HAL_GetTick();
						button_state[i] = button_now_state[i];
						// handle_button_press(11, 0); // El parámetro 0 en este caso no va a hacer nada
					}
				}
			}
			*/
			// el debounce de Analog_ON lo tengo que hacer si o si para que funcione
			/*
			if(USB_State){
				// si no le pongo esta condición me sobreescribe el hecho en BT
				init_buffer = (pos_Rx - RX_SIZE + BUFFER_SIZE_RX)%BUFFER_SIZE_RX;

				button_now_state[11] = !( (buffer_Rx[init_buffer] >> 5) & 0b01);
				if(button_now_state[11] != button_last_state[11]){
					button_last_state[11] = button_now_state[11];
					button_last_time[11] = HAL_GetTick();
					button_state[11] = button_now_state[11];
					// handle_button_press(11, 0); // El parámetro 0 en este caso no va a hacer nada
				}
			}
			*/

			  // Armado del Struct_Report
			for (i = 0; i < CANT_BOTONES_TOTALES; i++) {
				if ((HAL_GetTick() - button_last_time[i]) > DEBOUNCE_TIME_MS) { // se preisonó por más de 20 ms

					if (button_state[i] != button_debounced[i]) {
						button_debounced[i] = button_state[i];

						if (button_debounced[i] == GPIO_PIN_RESET) // Botón i válidamente presionado!
							handle_button_press(i, 1);
						else
							handle_button_press(i, 0);
					}
				}
			}

			if(USB_State){
				USBD_HID_SendReport(&hUsbDeviceFS, Joystick_Report_Struct.val, sizeof(Joystick_Report_Struct.val));

				/*
				debug++;
				if(debug > 1000){
					HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
					debug = 0;
				}
				*/

				// HAL_UART_Transmit(&huart1, Buttons_ID, 2, HAL_MAX_DELAY);
			}
			else{
				for(i = 0; i < REPORT_BUFFER_SIZE; i++)
					buffer_Tx[i + 2] = Joystick_Report_Struct.val[i];

				// BT_ID = 0xAAAA
				buffer_Tx[0] = BT_ID[0]; // COM_ID
				buffer_Tx[1] = BT_ID[1];
				  // memcpy(buffer_Tx, Joystick_Report_Struct.val, sizeof(Joystick_Report_Struct.val));
				HAL_UART_Transmit(&huart1, buffer_Tx, TX_SIZE, HAL_MAX_DELAY);
			}

			HAL_TIM_Base_Start_IT(&htim4);

		}

		// uint32_t cont_aux;
		if(Flag_ESP_Sent_Report){
			Flag_ESP_Sent_Report = 0;

			// ---------------------- DEBUG BEGIN -----------------------------
			/*
            if(cont_aux == 0){
				HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
			}
            cont_aux++;
            cont_aux %= 100;
            */
            // ---------------------- DEBUG END -----------------------------

			// HAL_UART_Transmit(&huart1, (uint8_t *)&Buttons_ID, 1, HAL_MAX_DELAY);

			// Analogs_ON
			/*
			init_buffer = (pos_Rx - 3 + BUFFER_SIZE_RX)%BUFFER_SIZE_RX;
			button_now_state[ANALO] = !buffer_Rx[init_buffer];
			if(button_now_state[11] != button_last_state[11]){
				button_last_state[11] = button_now_state[11];
				button_last_time[11] = HAL_GetTick();
				button_state[11] = button_now_state[11];
				// handle_button_press(11, 0); // El parámetro 0 en este caso no va a hacer nada
			}
			*/
			// vacio buffer
			for(i = 0; i < BUFFER_SIZE_RX; i++)
				buffer_Rx[i] = 0;

			// doy tiempo a que ESP pueda enviar por BT ya que tarda bastante
			HAL_Delay(50);
			Flag_ESP_Buttons = 1;
			// buffer_Rx[(pos_Rx - 2 + BUFFER_SIZE_RX)%BUFFER_SIZE_RX];
		}

		if(Flag_TIM4){
			Flag_TIM4 = 0;

			if(!USB_State)
				HAL_UART_Transmit(&huart1, BT_ID, 2, HAL_MAX_DELAY);
		}

		if(Flag_Latch_In){
			if(cont_to == 0){
				to = HAL_GetTick();
				cont_to++;
			}
			if((HAL_GetTick() - to ) > 2000){
				HAL_GPIO_WritePin(Latch_Out_GPIO_Port, Latch_Out_Pin, GPIO_PIN_RESET);
				// HAL_GPIO_WritePin(Latch_v2_GPIO_Port, Latch_v2_Pin, GPIO_PIN_RESET);
			}
		}




    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL6;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC|RCC_PERIPHCLK_USB;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV4;
  PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_PLL;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_EXTERNALTRIGCONV_T3_TRGO;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 4;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_4;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_8;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_9;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 48-1;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 1000;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 4800;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 100;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief TIM3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM3_Init(void)
{

  /* USER CODE BEGIN TIM3_Init 0 */

  /* USER CODE END TIM3_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM3_Init 1 */

  /* USER CODE END TIM3_Init 1 */
  htim3.Instance = TIM3;
  htim3.Init.Prescaler = 4800-1;
  htim3.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim3.Init.Period = 50;
  htim3.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim3) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim3, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_UPDATE;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim3, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM3_Init 2 */

  /* USER CODE END TIM3_Init 2 */

}

/**
  * @brief TIM4 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM4_Init(void)
{

  /* USER CODE BEGIN TIM4_Init 0 */

  /* USER CODE END TIM4_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM4_Init 1 */

  /* USER CODE END TIM4_Init 1 */
  htim4.Instance = TIM4;
  htim4.Init.Prescaler = 48000-1;
  htim4.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim4.Init.Period = 65;
  htim4.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim4.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim4) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim4, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim4, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM4_Init 2 */

  /* USER CODE END TIM4_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);
  /* DMA1_Channel4_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel4_IRQn);
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(Latch_Out_GPIO_Port, Latch_Out_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : Triang_Pin Square_Pin */
  GPIO_InitStruct.Pin = Triang_Pin|Square_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : XX_Pin Circle_Pin Joy_Der_Pin Start_Pin
                           Analog_ON_Pin Joy_Izq_Pin Ar_Pin */
  GPIO_InitStruct.Pin = XX_Pin|Circle_Pin|Joy_Der_Pin|Start_Pin
                          |Analog_ON_Pin|Joy_Izq_Pin|Ar_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : Select_Pin V_USB_Sensing_Pin */
  GPIO_InitStruct.Pin = Select_Pin|V_USB_Sensing_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : Der_Pin Ab_Pin Izq_Pin L2_Pin
                           L1_Pin R1_Pin R2_Pin */
  GPIO_InitStruct.Pin = Der_Pin|Ab_Pin|Izq_Pin|L2_Pin
                          |L1_Pin|R1_Pin|R2_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : Latch_Out_Pin */
  GPIO_InitStruct.Pin = Latch_Out_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(Latch_Out_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI0_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  HAL_NVIC_SetPriority(EXTI1_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI1_IRQn);

  HAL_NVIC_SetPriority(EXTI2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI2_IRQn);

  HAL_NVIC_SetPriority(EXTI3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI3_IRQn);

  HAL_NVIC_SetPriority(EXTI4_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI4_IRQn);

  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
