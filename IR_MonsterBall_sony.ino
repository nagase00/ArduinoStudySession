// リモコンからデータを受信してその内容を送信して見る
// スイッチ1を押してリモコンから受信、スイッチ2で送信する
// スイッチ1でLED点灯し、受信したら消灯する
#define TIME_T        600         // 600[u sec]   
#define ZERO_USEC     TIME_T      // '0'
#define ONE_USEC      TIME_T * 2  // '1'
#define READ_H_USEC   TIME_T * 4  // reader
#define LOW_USEC      TIME_T      // 消えている
#define LED_FREQUENCY 39000       // 39KHz

#define TONE_OVERHEAD_CONV  0.380 //Tone関数オーバーヘッド（この割合をかける）
#define DELAY_OVERHEAD_CONV 0.988 //waitMicrosec関数オーバーヘッド（この割合をかける）

#define TRANSMISSION_TIME   45000 //繰り返し間隔（readerからreaderまで）

#define LED_PINNO    7     // LEDの接続しているピン番号
#define SW1_PINNO    11    // スイッチ１の接続しているピン番号
#define SW2_PINNO    12    // スイッチ２の接続しているピン番号
#define SND_PINNO    10    // 赤外線送信LED接続ピン番号
#define RCV_PINNO    8     // 赤外線受信モジュール接続ピン番号
#define RCV_PIN    (PINB & 0x1)    // デジタル8番ピン(PORTB:PB0)

#define SENC_3D_X    0  //x
#define SENC_3D_Y    1  //y
#define SENC_3D_Z    2  //z

#define SIZE_OF_IR_BIT_BUFFER  256
char IRbitData[SIZE_OF_IR_BIT_BUFFER] ;       // 受信データを格納する変数('0'/'1'で格納)
int  IRtimeUsecData[SIZE_OF_IR_BIT_BUFFER] ;  // 受信データをtime格納する変数([usec]で格納)
int  IRbitLen ;            // 受信データの長さ
int  transmissionTimeLength;   //受信データの繰り返し間隔(reader data間隔)

////////////////////////////////////////////////////////////////////////
//    setup
//
void setup()
{
  Serial.begin(9600) ;               // シリアル通信の初期化
  pinMode(SW1_PINNO, INPUT) ; // スイッチ１
  pinMode(SW2_PINNO, INPUT) ; // スイッチ２
  pinMode(RCV_PINNO, INPUT) ;        // 赤外線受信モジュール
  pinMode(SND_PINNO, OUTPUT) ;       // 赤外線送信LED
  pinMode(LED_PINNO, OUTPUT) ;       // LED
  IRbitLen = 0 ;

  int i;
  for ( i = 0; i < SIZE_OF_IR_BIT_BUFFER; i++) {
    IRbitData[i] = 0;
    IRtimeUsecData[i] = 0;
  }
}

////////////////////////////////////////////////////////////////////////
//  メイン処理
//
void loop()
{
  int i;

  // スイッチ1が押されたらリモコンより受信するまで待つ
  if (digitalRead(SW1_PINNO) == LOW) {
    digitalWrite(LED_PINNO, HIGH) ;         // LED点灯
    ReceiveData() ;                         // 受信する
    DspData(IRbitLen, IRbitData) ;          // 表示する
    DspTimeData(IRbitLen, IRtimeUsecData);
    digitalWrite(LED_PINNO, LOW) ;          // LED消灯
  }

  // スイッチ２が押されたら受信内容の送信と表示を行う
  //または，投げられたら
  if( (digitalRead(SW2_PINNO) == LOW) 
   || (isThrowed() == true ) ){
    for (i = 0; i < 3; i++) {
      SendData(IRbitLen, IRbitData) ;       // 送信する
      //DspData(IRbitLen, IRbitData) ;      // 表示する
    }
    delay(1000) ;                           // チャタリング防止
  }
}

////////////////////////////////////////////////////////////////////////
//  受信処理
//
void ReceiveData() {
  unsigned long t ;
  int i , cnt ;
  unsigned long start_t, end_t;

  //Serial.println("Please press the remote control.") ;
  // 受信するまでループ
  while (1) {
    t = 0 ;
    if (RCV_PIN == LOW) {
      // リーダ部のチェックを行う
      t = micros() ;                     // 現在の時刻(us)を得る
      start_t = t;
      while (RCV_PIN == LOW) ;           // HIGHになるまで待つ(消えるまで)
      t = micros() - t ;                 // LOWの部分をはかる
    }
    // リーダ部有りなら処理する
    if (t >= READ_H_USEC) {
      Serial.print("reader time : ");
      Serial.println(t, DEC);
      
      i = 0 ;

      // データ部の読み込み
      while (1) {
        while (RCV_PIN == HIGH) ;     // 消えている
        t = micros() ;
        cnt = 0 ;
        while (RCV_PIN == LOW) {      // 点いている
          delayMicroseconds(10) ;
          cnt++ ;
        }
        t = micros() - t ;
        if (t >= READ_H_USEC){
          end_t = micros();
          transmissionTimeLength = start_t - end_t + t;
          Serial.print("transmission time : ");
          Serial.println(transmissionTimeLength, DEC);
          break ;       // readerをストップデータとする
        }

        if (t >= (ZERO_USEC + 100) ) {       // 誤差100usecを取る
          IRbitData[i] = (char)0x31 ;  // '1'
        } else {
          IRbitData[i] = (char)0x30 ;  // '0'
        }
        IRtimeUsecData[i] = cnt;
        i++ ;
      }
      // データ有りなら内容を保存し終了
      if (i != 0) {
        IRbitData[i] = 0 ;
        IRbitLen     = i ;
        //Serial.println("Was received.") ;            // 受信しました
        break ;
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////
//  Debug
//
void DspData(int num, char *data)
{
  int i , j , x , dt ;

  Serial.print(data) ;              // ビットデータで表示
  Serial.write(" ( ") ;
  x = num / 8 ;
  // ビット文字列データから数値に変換する
  for (j = 0 ; j < x ; j++) {
    dt = 0 ;
    for (i = 0 ; i < 8 ; i++) {
      if (*data++ == 0x31) bitSet(dt, i) ;
    }
    Serial.print(dt, HEX) ;      // ＨＥＸ(16進数)で表示
    Serial.write(' ') ;
  }
  Serial.println(')') ;
}

////////////////////////////////////////////////////////////////////////
//  Debug
//
void DspTimeData(int num, int *data) {
  int i;
  Serial.write(" ( ") ;
  for ( i = 0; i < num; i++) {
    Serial.print(data[i], DEC);
    Serial.write(',');
  }
  Serial.println(')') ;
}

////////////////////////////////////////////////////////////////////////
//  送信処理
//
void SendData(int num, char *data)
{
  unsigned long start_t, end_t ,t;
  start_t = micros() ;

  // リーダ部を送る
  PalseHigh(READ_H_USEC * TONE_OVERHEAD_CONV + 80) ;  //誤差込み

  // データを送る
  int i ;
  for (i = 0 ; i < num ; i++) {
    //t = micros();
    delayMicroseconds(LOW_USEC * DELAY_OVERHEAD_CONV ) ;  //誤差込み
    //Serial.print(micros()-t,DEC);
    //Serial.print("/");
    
    //t = micros();
    if (*data == '1') {
      PalseHigh(ONE_USEC * TONE_OVERHEAD_CONV + 80) ;  //誤差込み
    } else {
      PalseHigh(ZERO_USEC * TONE_OVERHEAD_CONV) ;  //誤差込み
    }
    //Serial.print(micros()-t,DEC);
    //Serial.print(", ");
    data++ ;
  }
  // 1サイクル時間経つまで待つ
  while ( micros() - start_t < TRANSMISSION_TIME );
  Serial.println(micros() - start_t,DEC);
}

////////////////////////////////////////////////////////////////////////
//  ＨＩＧＨのパルスを作る（めっちゃオーバーヘットがある）
//
void PalseHigh(int cnt)
{
  tone(SND_PINNO, LED_FREQUENCY) ; // 40KHzのON開始
  delayMicroseconds(cnt) ;
  noTone(SND_PINNO) ;              // 40KHzの終了
}


////////////////////////////////////////////////////////////////////////
//  投げられたかどうか，加速度センサの値から判断する
//
bool isThrowed(){
  int x = analogRead(SENC_3D_X);
  int y = analogRead(SENC_3D_Y);
  int z = analogRead(SENC_3D_Z);

  //一定値より上回ったか，下回っらた
  if(  x < 50 || 1200 < x
    || y < 50 || 1200 < y
    || z < 50 || 1200 < z ){
    return( true );
  }
  return( false );
}

