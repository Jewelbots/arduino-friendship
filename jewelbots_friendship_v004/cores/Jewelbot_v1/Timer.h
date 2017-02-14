#ifndef __TIMER_H__
#define __TIMER_H__

  void arduino_timer_init(void);

  class Timer {
  public:
    Timer();
    ~Timer();
    void pause(uint32_t milliseconds);
    uint32_t runtime_ms(void);

  };

#endif
