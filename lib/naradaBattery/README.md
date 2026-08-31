# 나라다 리튬 축전지의 통신라이브러리  

## 디스플레이   
1. 디스플레이는 RS232통신으로 하며 모드버스 Slave로 작동하고 디스플레이는 Master로 작동한다  
1. Display part  232 TTL Level
    * 1 -> +5V  : Input
    * 2 -> TxD2(S3Wroom-1 U0Txd GPIO43)
    * 3 -> RxD2(S3Wroom-1 U0Rxd GPIO44)
    * 4 -> GND

1. Main BoardPart 
    * 9 ->  U0RxD(2*5P)  
    * 10 -> U0TxD(2*5P)
    * 4 -> GND(2*5P)
    * 3 -> Vcc(+5V)(4P) : Input
1. Cable  
    Dispay  MainBoard  Power   
    1   ---  3          +5V   
    2   ---  9          --  
    3   ---  10         --  
    4   ---  4          --  
