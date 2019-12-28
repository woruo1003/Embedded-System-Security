#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#define TUBE_IOCTROL  0x11
#define DOT_IOCTROL   0x12
#define BACK_PW "endhh"//back up stored encrypted password
#define CRC 0x6f //xor

//TUBE_LED: represent 0-9
unsigned char LEDCODE[10]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};

//dd_data[16][10] information
unsigned char dd_data[16][10]={{0xff,0,0,0,0,0,0,0,0,0},
{0,0xff,0,0,0,0,0,0,0,0},
{0,0,0xff,0,0,0,0,0,0,0},
{0,0,0,0xff,0,0,0,0,0,0},
{0,0,0,0,0xff,0,0,0,0,0},
{0,0,0,0,0,0xff,0,0,0,0},
{0,0,0,0,0,0,0xff,0,0,0},
{0,0,0,0,0,0,0,0xff,0,0},
{0x1,0x1,0x1,0x1,0x1,0x1,0x1,0x1,0,0},
{0x2,0x2,0x2,0x2,0x2,0x2,0x2,0x2,0,0},
{0x4,0x4,0x4,0x4,0x4,0x4,0x4,0x4,0,0},
{0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0,0},
{0x10,0x10,0x10,0x10,0x10,0x10,0x10,0x10,0,0},
{0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,0,0},
{0x40,0x40,0x40,0x40,0x40,0x40,0x40,0x40,0,0},
{0x80,0x80,0x80,0x80,0x80,0x80,0x80,0x80,0,0},
};

/*dd_lock[10]：display when you are entering the pw, the word "in"
0 0 0 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 1 0 0 0 0 0
0 0 0 0 0 0 0 0
0 0 1 0 1 0 1 0
0 0 1 0 1 1 0 1
0 0 1 0 1 0 0 1
0 0 1 0 1 0 0 1
*/
unsigned char dd_lock[10]={0,0,0xf4,0,0xf0,0x20,0x10,0xe0,0,0};


//delay
void jmdelay(int n) {
    int i,j,k;
    for (i=0;i<n;i++)
        for (j=0;j<100;j++)
            for (k=0;k<100;k++);
}


//encrypt the input password,simple algorithm
void en_pw(char *data)
{
    int i;
    for(i=0;i<strlen(data);i++)
        data[i]=data[i]+i; 
}

//check whether the input password is ture
int check_pw(char* enter_pw,char* THE_PW){
    en_pw(enter_pw);    //encrypt the input password

    jmdelay(rand()%255);    //delay considering that attackers can figure out the password according to the actual time spent 
    
    if(strcmp(enter_pw,THE_PW)==0)    //verified successfully
    {
        return 1;
    }
    else
        return 0;
}

//test whether the pw is changed or not. If changed, try to use the back_up pw to restore it.
//If back_up pw is also invalid, failed.
int samepw_check(char* THE_PW,char* BACK_PW)
{
    crc=THE_PW[0]^THE_PW[1]^THE_PW[2]^THE_PW[3]^THE_PW[4]; 

    if(CRC!=crc) //if the stored en_password was changed
    {
        crc=BACK_PW[0]^BACK_PW[1]^BACK_PW[2]^BACK_PW[3]^BACK_PW[4];
        if(CRC!=crc) //if back_up en_pw is also invalid, failed!!!!!
        {
            return 0;
        }
        else
        {
            memcpy(THE_PW,BACK_PW,8);  //use the back up pw to restore the original pw
            
        }
    }
    else
        memcpy(BACK_PW,THE_PW,8);  //use the original pw to update the back up pw to ensure the security
}





//open all lights, the process of initialization
int initialization(unsigned int *LEDWORD,int fd)
{
    //int ret;
    int i;
    unsigned char tmp[10];

    printf("TUBE LED  ,please wait .............. \n"); 
    *LEDWORD=0x0000; //all bright
    ioctl(fd,0x12,*LEDWORD);
    sleep(1);

    printf("DIG LED  ,please wait .............. \n");
    
    for (i=0;i<10;i++)
        tmp[i] = 255;
    write(fd,tmp,10); //all bright
    sleep(1);

    printf("you can enter 0 if you find there are errors, otherwise you should enter 1:")
    scanf("%d",&ret);
    return ret;
}


//firstly, judge whether your original and back_up pws are valid. If something wrong, try to restore.
//secondly, judge whether your input is true
int module_pw(unsigned int *LEDWORD,int fd,int *flag,char *enter_pw,char *THE_PW)
{
    int i,j,ret=0;
    if(!samepw_check(THE_PW)) //fail to use the back_up to restore the pw
        return ret;  

    //DIG_LED hint
    printf("please enter the pw to show the message: \n");
    write(fd,dd_lock,10);//display "in"
    sleep(1);
    
    if(scanf("%s",enter_pw))
    {
        ret=1;
        *flag=check_pw(enter_pw,THE_PW);  // wrong flag=0, right flag=1
    }
    
    return ret;
}
    
    

int main() {
    int fd;
    int i,j,k;
    int flag=2;//flag=0 pw is wrong，flag=1 pw is true，flag=2 pw hasn't been input
    int check1,check2,check3;
    unsigned int LEDWORD;
    //unsigned int MLEDA[8];
    char enter_pw[50];
    char THE_PW[50];
    unsigned char tmp[10];  //dark
    unsigned char a[10];    //random
    memset(enter_pw,0x00,50);
    memset(THE_PW,0x00,50);
    memcpy(THE_PW,BACK_PW,8);   //two copies

    fd=open("/dev/s3c2410_led0",O_RDWR);
    if (fd < 0) {
        printf("Led device open fail!!!!!!\n");
        return (-1);
    }

    //begin to work
    check1=initialization(&LEDWORD,fd);
    if(check1==1)
        printf("device was successfully initialized\n");
    else
    {
        printf("failure!!!!!!\n");
        close(fd);
        return (-1); 
    }


    //verify the input
    check2=module_pw(&LEDWORD,fd,&flag,enter_pw,THE_PW);
    if(check2)
        printf("checking the pw now .............. \n");
    else
    {
        printf("failure!!!!!!\n");
        close(fd);
        return (-1); 
    }
        
    //3、显示模块MODULE_SHOW：根据密码输入正确与否显示乱码/秘密信息
    if(flag!=1) //input is wrong
    {
        LEDWORD=0xffff;         //all dark
        ioctl(fd,0x12,LEDWORD);
        printf("pw is wrong!!!!!!!\n");
        while(1){               //DIG_LED乱码
            //unsigned char a[10];
            for (i=0;i<10;i++)
                a[i] = rand() % 255;
            write(fd,a,10);
            jmdelay(500);
        }
    }
    else
    {
        LEDWORD=0x0000;         //all bright
        ioctl(fd,0x12,LEDWORD);
        printf("pw is right, and display the information\n");
        for (i=0;i<16;i++) {    //DIG_LED显示信息
            write(fd,dd_data[i],10);
            jmdelay(1000);
        }
        check3=1; //sucessfully display all information 
    }

    if(check3) //successfully exit
    {
        LEDWORD=0xffff;     
        ioctl(fd,0x12,LEDWORD);
        sleep(1);
        
        for (i=0;i<10;i++)
            tmp[i] = 0;
        write(fd,tmp,10);
        sleep(1);
        close(fd);
        return 0;
    }
    else //any interrupt
    {
        printf("failure!!!!!!!\n");
        close(fd);
        return (-1); 
    }
    
}

