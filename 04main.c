// Подключение библиотек
#include "thread.h"
#include "periph/gpio.h"
#include "xtimer.h"
#include "msg.h"
#define debounce_delay 50000
#define max_bl 5
static uint32_t last_time_press = 0;
static int blink_count = 0;
// Выделение памяти под стеки двух тредов
char thread_one_stack[THREAD_STACKSIZE_DEFAULT];
char thread_two_stack[THREAD_STACKSIZE_DEFAULT];

// Глобально объявляется структура, которая будет хранить PID (идентификатор треда)
// Нам нужно знать этот идентификатор, чтобы посылать сообщения в тред
static kernel_pid_t thread_one_pid;


void btn_handler(void *arg)
{
  (void)arg;
  msg_t msg; 
  uint32_t now = xtimer_usec_from_ticks(xtimer_now());
  if ((now - last_time_press) >= debounce_delay){
    if(gpio_read(GPIO_PIN(PORT_A, 0))>0)
      {
        blink_count = (blink_count % max_bl)+1;
        msg.content.value = blink_count;
        msg_send(&msg, thread_one_pid);
        last_time_press = now;
      }
  }
}
// Первый поток
void *thread_one(void *arg)
{
  // Прием аргументов из главного потока
  (void) arg;
  // Создание объекта для хранения сообщения 
  msg_t msg;
  // Инициализация пина PC8 на выход
  gpio_init(GPIO_PIN(PORT_C,8),GPIO_OUT);
  while(1){
    // Ждем, пока придет сообщение
    // msg_receive - это блокирующая операция, поэтому задача зависнет в этом месте, пока не придет сообщение
  	msg_receive(&msg);
  	int count = msg.content.value;
  	for (int i = 0; i < count; i++)
  	{
  	  gpio_toggle(GPIO_PIN(PORT_C, 8));
  	  xtimer_usleep(500000);
  	  gpio_toggle(GPIO_PIN(PORT_C, 8));
  	  xtimer_usleep(500000);
  	}
  }
  // Хотя код до сюда не должен дойти, написать возврат NULL все же обязательно надо    
  return NULL;
}

// Второй поток
void *thread_two(void *arg)
{

  // Прием аргументов из главного потока
  (void) arg;
  // Инициализация пина PC9 на выход
  gpio_init(GPIO_PIN(PORT_C,9),GPIO_OUT);

  while(1){
    // Включаем светодиод
  	gpio_set(GPIO_PIN(PORT_C,9));
    // Задача засыпает на 333333 микросекунды
   	xtimer_usleep(333333);
    // Выключаем светодиод
        gpio_clear(GPIO_PIN(PORT_C,9));
    // Задача снова засыпает на 333333 микросекунды
        xtimer_usleep(333333);
  }    
  return NULL;
}


int main(void)
{
  // Инициализация прерывания с кнопки (подробнее в примере 02button)
	gpio_init_int(GPIO_PIN(PORT_A,0),GPIO_IN,GPIO_BOTH,btn_handler,NULL);

  // Создение потоков (подробнее в примере 03threads)
  // Для первого потока мы сохраняем себе то, что возвращает функция thread_create,
  // чтобы потом пользоваться этим как идентификатором потока для отправки ему сообщений
  thread_one_pid = thread_create(thread_one_stack, sizeof(thread_one_stack),
                  THREAD_PRIORITY_MAIN - 1, THREAD_CREATE_STACKTEST,
                  thread_one, NULL, "thread_one");
  // обратите внимание, что у потоков разный приоритет: на 1 и на 2 выше, чем у main
  thread_create(thread_two_stack, sizeof(thread_two_stack),
                  THREAD_PRIORITY_MAIN - 2, THREAD_CREATE_STACKTEST,
                  thread_two, NULL, "thread_two");

  while (1){

    }

    return 0;
}

/* 
  Задание 1: Добавьте в код подавление дребезга кнопки
  Задание 2: Сделайте так, чтобы из прерывания по нажатию кнопки в поток thread_one передавалось целое число,
              которое означает, сколько раз должен моргнуть светодиод на пине PC8 после нажатия кнопки.
              После каждого нажатия циклически инкрементируйте значение от 1 до 5.
              Передать значение в сообщении можно через поле msg.content.value
  Задание 3: Сделайте так, чтобы из прерывания по отпусканию кнопки в поток thread_two передавалось целое число,
              которое задает значение интервала между морганиями светодиода на пине PC9. 
              Моргание светодиода не должно останавливаться.
              После каждого нажатия циклически декрементируйте значение от 1000000 до 100000 с шагом 100000.
              Чтобы послать сообщение асинхронно и без блокирования принимающего потока, нужно воспользоваться очередью. 
              Под очередь нужно выделить память в начале программы так: static msg_t rcv_queue[8];
              Затем в принимающем потоке нужно ее инициализировать так: msg_init_queue(rcv_queue, 8);
              Поток может проверять, лежит ли что-то в очереди, это делается функцией msg_avail(), 
              которая возвращает количество элементов в очереди. 

  Что поизучать: https://riot-os.org/api/group__core__msg.html
*/

/*
ex 1
void btn_handler(void *arg)
{
  (void)arg;
  msg_t msg; 
  uint32_t now = xtimer_usec_from_ticks(xtimer_now())
  if ((now - last_time_press) >= debounce_delay) > 0){
    if(gpio_read(GPIO_PIN(PORT_A, 0))>0)
    // Отправка сообщения в тред по его PID
    // Сообщение остается пустым, поскольку нам сейчас важен только сам факт его наличия, и данные мы не передаем
    msg_send(&msg, thread_one_pid);    
  }
*/


/*
ex. 3
static msg_t rcv_queue[8]; 
static uint32_t interval = 10000000
void btn_handler(void *arg)
{
  (void)arg;
  if (gpio_read(GPIO_PIN(PORT_A, 0)) == 0)
  {
    interval -= 100000;
    if (interval < 100000)
    {
      interval = 1000000;
    }
    msg_t msg;
    msg.content.value = interval;
    msg_send(&msg, thread_one_pid);
  }
}
void *thread_one(void *arg)
{
  // Прием аргументов из главного потока
  (void) arg;
  // Создание объекта для хранения сообщения 
  msg_t msg;
  // Инициализация пина PC8 на выход
  gpio_init(GPIO_PIN(PORT_C,8),GPIO_OUT);
  while(1){
        msg_receive(&msg);
        gpio_toggle(GPIO_PIN(PORT_C,8));
  	}
  }
  // Хотя код до сюда не должен дойти, написать возврат NULL все же обязательно надо    
  return NULL;
}
void *thread_two(void *arg)
{

  // Прием аргументов из главного потока
  (void) arg;
  msg_init_queue(rcv_queue, 8);
  // Инициализация пина PC9 на выход
  gpio_init(GPIO_PIN(PORT_C,9),GPIO_OUT);
  uint32_t local_interval = interval;
  
  while(1){
    if (msg_avail())
    {
      msg_t msg;
      msg_receive(&msg);
      local_interview = msg.content.value;
    }
    gpio_set(GPIO_PIN(PORT_C, 9));
    xtimer_usleep(local_interval);
    gpio_clear(GPIO_PIN(PORT_C, 9));
  }    
  return NULL;
}
*/
