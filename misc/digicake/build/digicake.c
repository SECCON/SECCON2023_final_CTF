#include "pico/stdlib.h"
#include "hardware/pwm.h"
#include <stdlib.h>

#define PIN_LED  PICO_DEFAULT_LED_PIN
#define PIN_SEL_1 3
#define PIN_SEL_2 4
#define PIN_SEL_3 5
#define PIN_SEL_4 6
#define PIN_SEG_A 14
#define PIN_SEG_B 13
#define PIN_SEG_C 12
#define PIN_SEG_D 11
#define PIN_SEG_E 10
#define PIN_SEG_F 9
#define PIN_SEG_G 8
#define PIN_SEG_P 7
#define PIN_SND 15
#define PIN_CLK 17
#define PIN_RST 16
#define PIN_LED_RED  1
#define PIN_LED_BLUE 2
#define PIN_S1  18
#define PIN_S2  19
#define PIN_START_SIG 20

#define BIT_A 0b10000000
#define BIT_B 0b01000000
#define BIT_C 0b00100000
#define BIT_D 0b00010000
#define BIT_E 0b00001000
#define BIT_F 0b00000100
#define BIT_G 0b00000010
#define BIT_P 0b00000001

const uint8_t font[12] = {
  BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F,       // 0
  BIT_B|BIT_C,                               // 1
  BIT_A|BIT_B|BIT_G|BIT_E|BIT_D,             // 2
  BIT_A|BIT_B|BIT_G|BIT_C|BIT_D,             // 3
  BIT_F|BIT_B|BIT_G|BIT_C,                   // 4
  BIT_A|BIT_F|BIT_G|BIT_C|BIT_D,             // 5
  BIT_A|BIT_F|BIT_E|BIT_D|BIT_C|BIT_G,       // 6
  BIT_A|BIT_B|BIT_C,                         // 7
  BIT_A|BIT_B|BIT_C|BIT_D|BIT_E|BIT_F|BIT_G, // 8
  BIT_A|BIT_B|BIT_C|BIT_D|BIT_F|BIT_G,       // 9
  0,                                         // blank
  BIT_G,                                     // -
};
const uint8_t pin_dig[4] = {PIN_SEL_1, PIN_SEL_2, PIN_SEL_3, PIN_SEL_4};
const uint8_t pin_seg[8] = {
  PIN_SEG_A, PIN_SEG_B, PIN_SEG_C, PIN_SEG_D,
  PIN_SEG_E, PIN_SEG_F, PIN_SEG_G, PIN_SEG_P,
};

volatile int16_t timer;
volatile uint8_t digit[4];
volatile uint8_t sel = 0;
uint channel;

void init_gpio(pwm_config *pconfig, uint *pslice) {
  gpio_init(PIN_LED);
  gpio_set_dir(PIN_LED, GPIO_OUT);
  gpio_init_mask         (0b11111111111111111110);
  gpio_set_dir_in_masked (0b11100000000000000000);
  gpio_set_dir_out_masked(0b00111111111111111110);
  for (int i = 0; i < 22; i++)
    gpio_put(i, 0);
  for (int i=0; i <8; i++)
    gpio_pull_up(pin_seg[i]);

  gpio_pull_down(PIN_LED_RED);
  gpio_pull_down(PIN_LED_BLUE);
  gpio_pull_down(PIN_S1);
  gpio_pull_down(PIN_S2);
  gpio_pull_down(PIN_CLK);
  gpio_pull_down(PIN_START_SIG);

  gpio_set_function(PIN_SND, GPIO_FUNC_PWM);
  channel = pwm_gpio_to_channel(PIN_SND);
  *pslice = pwm_gpio_to_slice_num(PIN_SND);
  *pconfig = pwm_get_default_config();
  pwm_set_wrap(*pslice, 3);
  pwm_set_chan_level(*pslice, channel, 0);
  pwm_set_enabled(*pslice, false);

  digit[0] = digit[1] = digit[2] = digit[3] = BIT_G;
}

void set_frequency(pwm_config *pconfig, uint slice, uint freq) {
  uint count = 125000000 * 16 / freq;
  uint div = count / 60000;
  pconfig->div = div;
  pconfig->top = count / div;
  pwm_init(slice, pconfig, true);
  pwm_set_chan_level(slice, channel, pconfig->top / 2);
}

bool disp_callback(struct repeating_timer *_timer) {
  uint8_t n = sel;
  for (uint8_t i = 0; i < 4;i++)  // Select digit
    gpio_put(pin_dig[i], n == i ? 1 : 0);
  for (uint8_t i = 0; i < 8; i++) // Display number
    gpio_put(pin_seg[i], digit[n] & (0x80 >> i) ? 0 : 1);
  sel = (sel + 1) % 4;
  return true;
}

bool timer_callback(struct repeating_timer *_timer) {
  uint16_t min = timer / 60;
  uint16_t sec = timer % 60;
  digit[0] = font[min / 10];
  digit[1] = font[min % 10] | BIT_P;
  digit[2] = font[sec / 10];
  digit[3] = font[sec % 10];
  return --timer >= 0;
}

bool clock_callback(struct repeating_timer *_timer) {
  gpio_put(PIN_CLK, !gpio_get(PIN_CLK));
  gpio_put(PIN_LED, !gpio_get(PIN_LED));
  return true;
}

void play_ok(uint slice, pwm_config *pconf) {
  pwm_set_enabled(slice, true);
  set_frequency(pconf, slice, 494);
  sleep_ms(100);
  set_frequency(pconf, slice, 587);
  sleep_ms(100);
  set_frequency(pconf, slice, 784);
  sleep_ms(100);
  pwm_set_enabled(slice, false);
}

int main() {
  uint slice;
  pwm_config config;
  struct repeating_timer t1, t2, t3;

  init_gpio(&config, &slice);
  add_repeating_timer_ms(4, disp_callback, NULL, &t1);

  play_ok(slice, &config);

  /* Cut to start */
  gpio_put(PIN_RST, 0);
  sleep_ms(100);
  gpio_put(PIN_RST, 1);

  digit[0] = BIT_F | BIT_G | BIT_E | BIT_B | BIT_C ;        // H
  digit[1] = BIT_B | BIT_C | BIT_D | BIT_E | BIT_F ;        // U
  digit[2] = BIT_A | BIT_B | BIT_C | BIT_D | BIT_E | BIT_F; // O
  digit[3] = BIT_P;                                         // .

  int ctt_cnt = 0;
  while (ctt_cnt <= 3) {
    if (!gpio_get(PIN_START_SIG))
      ctt_cnt++;
    else
      ctt_cnt = 0;
    sleep_ms(200);
  }

  play_ok(slice, &config);

  /* Let's go */
  timer = 10 * 60;

  add_repeating_timer_ms(1000, timer_callback, NULL, &t2);
  add_repeating_timer_ms(10000, clock_callback, NULL, &t3);

  gpio_put(PIN_RST, 0);
  sleep_ms(100);
  gpio_put(PIN_RST, 1);

  int defuse_cnt = 0;
  while (defuse_cnt < 3 && timer >= 0) {
    if (gpio_get(PIN_S1) && gpio_get(PIN_S2))
      defuse_cnt++;
    else
      defuse_cnt = 0;
    sleep_ms(100);
  }

  cancel_repeating_timer(&t2);
  cancel_repeating_timer(&t3);
  if (defuse_cnt >= 3) {
    /* Show flag (success) */
    digit[0] = BIT_A | BIT_E | BIT_F | BIT_G;                 // F
    digit[1] = BIT_D | BIT_E | BIT_F;                         // L
    digit[2] = BIT_A | BIT_B | BIT_C | BIT_E | BIT_F | BIT_G; // A
    digit[3] = BIT_A | BIT_B | BIT_C | BIT_D | BIT_F | BIT_G; // g
    gpio_put(PIN_LED_BLUE, 1);
    while (true) tight_loop_contents();

  } else {
    /* Explode (fail) */
    gpio_put(PIN_LED_RED, 1);
    set_frequency(&config, slice, 500);
    pwm_set_enabled(slice, true);
    for (uint i = 0; i < 500*10; i += 10) {
      set_frequency(&config, slice, 500 + (i % 500));
      sleep_ms(10);
    }
    pwm_set_enabled(slice, false);

    while (true) {
      digit[0] = rand() & 0xff;
      digit[1] = rand() & 0xff;
      digit[2] = rand() & 0xff;
      digit[3] = rand() & 0xff;
      sleep_ms(100);
    }
  }
}
