
#include <stdio.h>
#include <string.h>
#include "math/mathc.h"
#include "buzzer.h"


/* XDCtools Header files */
#include <xdc/std.h>
#include <xdc/runtime/System.h>
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/drivers/I2C.h>
#include <ti/drivers/i2c/I2CCC26XX.h>
#include <ti/drivers/PIN.h>
#include <ti/drivers/pin/PINCC26XX.h>
#include <ti/mw/display/Display.h>

/* Board Header files */
#include "Board.h"
#include "sensors/bmp280.h"
#include "sensors/mpu9250.h"
#include "wireless/comm_lib.h"
#include "wireless/address.h"

#define STACKSIZE 2048
char taskStack[STACKSIZE];
char taskStack0[STACKSIZE];
char taskStack1[STACKSIZE];
char taskStack2[STACKSIZE];
char taskStack3[STACKSIZE];

enum state { MENU=1, DEFAULT=2, READ_SENSOR=3, UPDATE=4, NEW_MSG=5 };
enum direction { UP, DOWN, RIGHT, LEFT };
enum menu {PLAY=1, OPTIONS=2, EXIT=3};
enum gameState {GAME=1, WIN=2, LOST_GAME=3};

/*Global variables*/
enum state myState = MENU;
enum state prevState = MENU;
enum menu menuState = PLAY;
enum direction latestDir = UP;
enum gameState currentGameState = GAME;
bool inMenu = true;
bool dirUnused = false;
int turn = 0;


char event_up[16] = "event:UP";
char event_down[16] = "event:DOWN";
char event_left[16] = "event:LEFT";
char event_right[16] = "event:RIGHT";
char str[16];
char win[16] = "32,WIN";
char lost[16] = "32,LOST GAME";



static PIN_Handle hMpuPin;
static PIN_State MpuPinState;
static PIN_Config MpuPinConfig[] = {
    Board_MPU_POWER  | PIN_GPIO_OUTPUT_EN | PIN_GPIO_HIGH | PIN_PUSHPULL | PIN_DRVSTR_MAX,
    PIN_TERMINATE
};

static const I2CCC26XX_I2CPinCfg i2cMPUCfg = {
    .pinSDA = Board_I2C0_SDA1,
    .pinSCL = Board_I2C0_SCL1
};

Void menuFxn(UArg arg0){
    
    while (1) {
        
        if(myState == MENU && prevState == MENU){
            System_printf("MENU => UPDATE\n");
            System_printf("%d currently selected\n", menuState);
            System_flush();
            myState = UPDATE;
        }
        
        else if(myState == MENU && prevState == READ_SENSOR){
            
            switch (latestDir){
            
                case UP:
                    
                    menuState -= 1;
                    if(menuState < 1) menuState = 3;
                    
                    prevState = MENU;
                    myState = UPDATE;
                    
                    break;
            
                case DOWN:
                    
                    menuState += 1;
                    if(menuState > 3) menuState = 1;
                    
                    prevState = MENU;
                    myState = UPDATE;
                    
                    break;
            
                case LEFT:
                    
                    switch(menuState){
                        
                        case PLAY:
                            System_printf("MENU SELECT\n");
                            System_printf("MENU => DEFAULT\n");
                            System_flush();
                            
                            inMenu = false;
                            myState = DEFAULT;
                            break;
                            
                        case OPTIONS:
                            prevState = MENU;
                            myState = DEFAULT;
                            break;
                            
                        case EXIT:
                            prevState = MENU;
                            myState = DEFAULT;
                            break;
                            
                        default:
                            System_printf("Error in menu!");
                            System_flush();
                    }
                    
                    break;
                
                case RIGHT:
                    
                    switch(menuState){
                        
                        case PLAY:
                            System_printf("MENU SELECT\n");
                            System_printf("MENU => DEFAULT\n");
                            System_flush();
                            
                            inMenu = false;
                            myState = DEFAULT;
                            break;
                            
                        case OPTIONS:
                            prevState = MENU;
                            myState = DEFAULT;
                            break;
                            
                        case EXIT:
                            prevState = MENU;
                            myState = DEFAULT;
                            break;
                            
                        default:
                            System_printf("Error in menu!");
                            System_flush();
                    }
                    
                    break;
                    
                default:
                    System_printf("Error in display task!");
                    System_flush();
            }
        }
        
        Task_sleep(100000 / Clock_tickPeriod);
    }
}

Void clkFxn(UArg arg0) {

    if (myState == DEFAULT) {
        
        System_printf("DEFAULT => READ_SENSOR\n");
        System_flush();
    
        myState = READ_SENSOR;
    }
}

Void sensorTask(UArg arg0, UArg arg1) {
    
    float ax, ay, az, gx, gy, gz;

	I2C_Handle i2cMPU;
	I2C_Params i2cMPUParams;
    I2C_Params_init(&i2cMPUParams);
    i2cMPUParams.bitRate = I2C_400kHz;
    i2cMPUParams.custom = (uintptr_t)&i2cMPUCfg;

    
    i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
    if (i2cMPU == NULL) {
        System_abort("Error Initializing I2CMPU\n");
    }

    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_ON);

	Task_sleep(100000 / Clock_tickPeriod);
    System_printf("MPU9250: Power ON\n");
    System_flush();

    
	System_printf("MPU9250: Setup and calibration...\n");
	System_flush();

	mpu9250_setup(&i2cMPU);

	System_printf("MPU9250: Setup and calibration OK\n");
	System_flush();

    I2C_close(i2cMPU);
    

	while (1) {
	    
	    if(myState == READ_SENSOR){
	        
	        i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
        	if (i2cMPU == NULL) {
        	System_abort("Error Initializing I2CMPU\n");
        	}
        	mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
        	az++;
        		
        	I2C_close(i2cMPU);
        	
        	struct vec3 vec_1 = {ax, ay, az};
        	
        	Task_sleep(10000 / Clock_tickPeriod);
        	
        	i2cMPU = I2C_open(Board_I2C, &i2cMPUParams);
        	if (i2cMPU == NULL) {
        	System_abort("Error Initializing I2CMPU\n");
        	}
        	
        	mpu9250_get_data(&i2cMPU, &ax, &ay, &az, &gx, &gy, &gz);
            az++;
            
            I2C_close(i2cMPU);
        	    
            struct vec3 vec_2 = {ax, ay, az};
                
            Task_sleep(10000 / Clock_tickPeriod);
            
            struct vec3 tmp;
            tmp = svec3_subtract(vec_1, vec_2);
            float len = svec3_length(tmp);
        
            if(len > 0.65f){
                char stmp[8];
                struct vec3 dir;
                dir = svec3_divide_f(svec3_add(vec_1, vec_2), 2.0f);
                struct vec3 absvec;
                absvec = svec3_abs(dir);
                
                if(absvec.z > absvec.x && absvec.z > absvec.y)
                {
                    if(dir.z > 0)
                    {
                        //strcpy(stmp, "z+\n");
                        //DOWN!!!!!
                        latestDir = DOWN;
                        dirUnused = true;
                        if(prevState == MENU && inMenu == true){
                            System_printf("READ_SENSOR => MENU\n");
                            
                            myState = MENU;
                            prevState = READ_SENSOR;
                        }
                        else{
                            myState = UPDATE;
                            System_printf("READ_SENSOR => UPDATE\n");
                        }
                    }
                    else
                    {
                        //strcpy(stmp, "z-\n");
                        //UP!!!!!
                        latestDir = UP;
                        dirUnused = true;
                        if(prevState == MENU && inMenu == true){
                            System_printf("READ_SENSOR => MENU\n");
                            
                            myState = MENU;
                            prevState = READ_SENSOR;
                        }
                        else{
                            myState = UPDATE;
                            System_printf("READ_SENSOR => UPDATE\n");
                        }
                    }
                }
                else if (absvec.x > absvec.z && absvec.x > absvec.y)
                {
                    if(dir.x > 0)
                    {
                        //strcpy(stmp, "x+\n");
                        //LEFT!!!!!
                        latestDir = LEFT;
                        dirUnused = true;
                        if(prevState == MENU && inMenu == true){
                            System_printf("READ_SENSOR => MENU\n");
                            System_flush();
                            myState = MENU;
                            prevState = READ_SENSOR;
                        }
                        else{
                            myState = UPDATE;
                            System_printf("READ_SENSOR => UPDATE\n");
                        }
                    }
                    else
                    {
                        //strcpy(stmp, "x-\n");
                        //RIGHT!!!!!!
                        latestDir = RIGHT;
                        dirUnused = true;
                        if(prevState == MENU && inMenu == true){
                            System_printf("READ_SENSOR => MENU\n");
                            System_flush();
                            myState = MENU;
                            prevState = READ_SENSOR;
                        }
                        else{
                            myState = UPDATE;
                            System_printf("READ_SENSOR => UPDATE\n");
                        }
                    }
                    
                }
                else
                {
                    //strcpy(stmp, "yyy\n");
                    
                }
                
                
                
                //System_printf(stmp);
                System_flush();
            }
	    }
	    Task_sleep(10000 / Clock_tickPeriod);
	}

    PIN_setOutputValue(hMpuPin,Board_MPU_POWER, Board_MPU_POWER_OFF);
}

Void displayTask(UArg arg0, UArg arg1) {
   
   Display_Params params;
   Display_Params_init(&params);
   params.lineClearMode = DISPLAY_CLEAR_BOTH;

   Display_Handle displayHandle = Display_open(Display_Type_LCD, &params);
   

    while (1) {
        
        if (myState == UPDATE) {
            
            if(currentGameState != GAME){
                
                Display_clear(displayHandle);
                
                if(currentGameState == WIN){
                    Display_print0(displayHandle, 5, 6, "YOU WIN!");
                    Display_print0(displayHandle, 6, 7, str);
                    
                    Task_sleep(500000 / Clock_tickPeriod);
                    
                    prevState = MENU;
                    inMenu = true;
                    myState = MENU;
                    turn = 0;
                }
                else if(currentGameState == LOST_GAME){
                    Display_print0(displayHandle, 5, 6, "YOU LOST!");
                    Display_print0(displayHandle, 6, 7, str);
                    
                    Task_sleep(500000 / Clock_tickPeriod);
                    
                    prevState = MENU;
                    inMenu = true;
                    myState = MENU;
                    turn = 0;
                }
                
            }
            
            else if(inMenu == true){
                
                Display_clear(displayHandle);
                
                switch(menuState){
                        
                        case PLAY:
                            Display_print0(displayHandle, 5, 3, "-->PLAY<--");
                            Display_print0(displayHandle, 6, 5, "OPTIONS");
                            Display_print0(displayHandle, 7, 6, "EXIT");
                            break;
                            
                        case OPTIONS:
                            Display_print0(displayHandle, 5, 6, "PLAY");
                            Display_print0(displayHandle, 6, 2, "-->OPTIONS<--");
                            Display_print0(displayHandle, 7, 6, "EXIT");
                            break;
                            
                        case EXIT:
                            Display_print0(displayHandle, 5, 6, "PLAY");
                            Display_print0(displayHandle, 6, 5, "OPTIONS");
                            Display_print0(displayHandle, 7, 3, "-->EXIT<--");
                            break;
                            
                        default:
                            System_printf("Error in menu!");
                            System_flush();
                    }
                    
                myState = DEFAULT;
                
            }
            
            else{
                
                turn += 1;
                
                switch (latestDir){
                
                case UP:
                    System_printf("UP\n");
                    System_flush();
                    
                    Display_clear(displayHandle);
                    Display_print0(displayHandle, 6, 7, "UP");
                    
                    Send6LoWPAN(IEEE80154_SERVER_ADDR, event_up, strlen(event_up));
                    StartReceive6LoWPAN();
                    break;
                
                case DOWN:
                    System_printf("DOWN\n");
                    System_flush();
                    
                    Display_clear(displayHandle);
                    Display_print0(displayHandle, 6, 6, "DOWN");
                    
                    Send6LoWPAN(IEEE80154_SERVER_ADDR, event_down, strlen(event_down));
                    StartReceive6LoWPAN();
                    break;
                
                case LEFT:
                    System_printf("LEFT\n");
                    System_flush();
                    
                    Display_clear(displayHandle);
                    Display_print0(displayHandle, 6, 6, "LEFT");
                    
                    Send6LoWPAN(IEEE80154_SERVER_ADDR, event_left, strlen(event_left));
                    StartReceive6LoWPAN();
                    break;
                
                case RIGHT:
                    System_printf("RIGHT\n");
                    System_flush();
                    
                    Display_clear(displayHandle);
                    Display_print0(displayHandle, 6, 6, "RIGHT");
                    
                    Send6LoWPAN(IEEE80154_SERVER_ADDR, event_right, strlen(event_right));
                    StartReceive6LoWPAN();
                    break;
                    
                default:
                    System_printf("Error in display task!");
                    System_flush();
                
            }
            sprintf(str, "Turn: %d", turn);
            Display_print0(displayHandle, 1, 1, str);
            
            System_printf("UPDATE => DEFAULT\n");
            System_flush();
            myState = DEFAULT;
                
            }
        }
    
        Task_sleep(1000000 / Clock_tickPeriod);
    }
}

Void commTask(UArg arg0, UArg arg1) {
    
    char payload[16]; // viestipuskuri
    uint16_t senderAddr;
    
    // Radio alustetaan vastaanottotilaan
    int32_t result = StartReceive6LoWPAN();
    if(result != true) {
        System_abort("Wireless receive start failed");
    }
    
    System_printf("Wireless receive start success");
    System_flush();
    

    while (1) {
        
        if (GetRXFlag() == TRUE && myState == DEFAULT) {

            myState = NEW_MSG;
            
            memset(payload,0,16);
            
            Receive6LoWPAN(&senderAddr, payload, 16);
            
            
            if(strcmp(payload, win) == 0){
                System_printf("WIN!\n");
                System_flush();
                
                currentGameState = WIN;
                
                myState = UPDATE;
            }
            else if(strcmp(payload, lost) == 0){
                System_printf("YOU LOST!\n");
                System_flush();
                
                currentGameState = LOST_GAME;
                
                myState = UPDATE;
            }
            else{
                System_printf("Message error!\n");
                System_printf("NEW_MSG -> DEFAULT\n");
                System_flush();
                myState = DEFAULT;
            }
            
            System_printf(payload);
            System_flush();
        }
    }
}

int main(void) {
    
    Board_initGeneral();
    
    Board_initI2C();
    
    Init6LoWPAN();
    
    Clock_Handle clkHandle;
    Clock_Params clkParams;
    
    Clock_Params_init(&clkParams);
    clkParams.period = 1000000 / Clock_tickPeriod;
    clkParams.startFlag = TRUE;

    hMpuPin = PIN_open(&MpuPinState, MpuPinConfig);
    if (hMpuPin == NULL) {
    	System_abort("Pin open failed!");
    }
    
    clkHandle = Clock_create((Clock_FuncPtr)clkFxn, 1000000 / Clock_tickPeriod, &clkParams, NULL);
    if (clkHandle == NULL) {
        System_abort("Clock create failed");
        
    }
    
	Task_Handle task;
	Task_Params taskParams;
	
    Task_Params_init(&taskParams);
    taskParams.stackSize = STACKSIZE;
    taskParams.stack = &taskStack;
    taskParams.priority = 2;

    task = Task_create((Task_FuncPtr)clkFxn, &taskParams, NULL);
    if (task == NULL) {
    	System_abort("Task create failed!");
    }
    
    Task_Handle task0;
	Task_Params taskParams0;
	
    Task_Params_init(&taskParams0);
    taskParams0.stackSize = STACKSIZE;
    taskParams0.stack = &taskStack0;
    taskParams0.priority = 2;

    task0 = Task_create((Task_FuncPtr)sensorTask, &taskParams0, NULL);
    if (task0 == NULL) {
    	System_abort("Task create failed!");
    }
    
    Task_Handle task1;
	Task_Params taskParams1;
	
    Task_Params_init(&taskParams1);
    taskParams1.stackSize = STACKSIZE;
    taskParams1.stack = &taskStack1;
    taskParams1.priority = 2;

    task1 = Task_create((Task_FuncPtr)displayTask, &taskParams1, NULL);
    if (task1 == NULL) {
    	System_abort("Task create failed!");
    }
    
    Task_Handle task2;
	Task_Params taskParams2;
	
    Task_Params_init(&taskParams2);
    taskParams2.stackSize = STACKSIZE;
    taskParams2.stack = &taskStack2;
    taskParams2.priority = 2;

    task2 = Task_create((Task_FuncPtr)menuFxn, &taskParams2, NULL);
    if (task2 == NULL) {
    	System_abort("Task create failed!");
    }
    
    Task_Handle task3;
	Task_Params taskParams3;
	
    Task_Params_init(&taskParams3);
    taskParams3.stackSize = STACKSIZE;
    taskParams3.stack = &taskStack3;
    taskParams3.priority = 1;

    task3 = Task_create((Task_FuncPtr)commTask, &taskParams3, NULL);
    if (task3 == NULL) {
    	System_abort("Task create failed!");
    }
    
    BIOS_start();

    return (0);
}