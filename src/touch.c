#include <stdio.h>
#include <time.h>

#include "mgos.h"
#include "tft.h"

#include "button.h"

/***************************************************************************************
***************************************************************************************/



#if USE_TOUCH == TOUCH_TYPE_XPT2046
uint32_t tp_calx = TP_CALX_XPT2046;
uint32_t tp_caly = TP_CALY_XPT2046;
#elif USE_TOUCH == TOUCH_TYPE_STMPE610
uint32_t tp_calx = TP_CALX_STMPE610;
uint32_t tp_caly = TP_CALY_STMPE610;
#else
uint32_t tp_calx = TP_CALX_XPT2046;
uint32_t tp_caly = TP_CALY_XPT2046;
#endif


static struct tft_button_list b_list;
static int bid=0;
static mgos_timer_id TFT_Touch_read_timer_id;


static int _lcd_width = DEFAULT_TFT_DISPLAY_WIDTH;
static int _lcd_height = DEFAULT_TFT_DISPLAY_HEIGHT;
static int _lcd_orientation = PORTRAIT;


// ============= Touch panel functions =========================================

#if USE_TOUCH == TOUCH_TYPE_XPT2046
//-------------------------------------------------------
static int tp_get_data_xpt2046(uint8_t type, int samples)
{
	if (ts_spi == (void *)NULL) return 0;

	int n, result, val = 0;
	int avg = 0;
	uint32_t i = 0;
//	uint32_t vbuf[18];
	uint32_t minval, maxval, dif=0;

    if (samples < 3) samples = 1;
    if (samples > 18) samples = 18;

    // one dummy read
    result = touch_get_data(type);
    avg = result >> 3;

    // read data
	while (i < 10) {
    	minval = 5000;
    	maxval = 0;
		// get values
		for (n=0;n<samples;n++) {
		    result = touch_get_data(type) >> 3;
		    avg = ((avg * 3) + result) / 4;

			if (result < 0) break;

//			vbuf[n] = result;
			if (result < minval) minval = result;
			if (result > maxval) maxval = result;
		}
		if (result < 0) break;
		dif = maxval - minval;
		if (dif < 40) break;
		i++;
    }

	if (result < 0) return -1;
	val = avg;
/*
	if (samples > 2) {
		// remove one min value
		for (n = 0; n < samples; n++) {
			if (vbuf[n] == minval) {
				vbuf[n] = 5000;
				break;
			}
		}
		// remove one max value
		for (n = 0; n < samples; n++) {
			if (vbuf[n] == maxval) {
				vbuf[n] = 5000;
				break;
			}
		}
		for (n = 0; n < samples; n++) {
			if (vbuf[n] < 5000) val += vbuf[n];
		}
		val /= (samples-2);
	}
	else val = vbuf[0];
*/
    return val;
}


//-----------------------------------------------
static int TFT_read_touch_xpt2046(int *x, int* y)
{
	int res = 0, result = -1;

	if (spi_lobo_device_select(ts_spi, 0) != ESP_OK) return 0;

    result = tp_get_data_xpt2046(0xB0, 3);  // Z; pressure; touch detect
	if (result <= 50)  goto exit;

	// touch panel pressed
	result = tp_get_data_xpt2046(0xD0, 10);
	if (result < 0)  goto exit;

	*x = result;

	result = tp_get_data_xpt2046(0x90, 10);
	if (result < 0)  goto exit;

	*y = result;
	res = 1;
exit:
	spi_lobo_device_deselect(ts_spi);
	return res;
}
#endif


//=============================================
//
int TFT_read_touch(int *x, int* y, uint8_t raw)
{
    *x = 0;
    *y = 0;
	if (ts_spi == (void *)NULL) return 0;
#if USE_TOUCH == TOUCH_TYPE_NONE
	return 0;
#else
	int result = -1;
    int X=0, Y=0;

    #if USE_TOUCH == TOUCH_TYPE_XPT2046
   	result = TFT_read_touch_xpt2046(&X, &Y);
   	if (result == 0) return 0;
    #elif USE_TOUCH == TOUCH_TYPE_STMPE610
    uint16_t Xx, Yy, Z=0;
    result = stmpe610_get_touch(&Xx, &Yy, &Z);
    if (result == 0) return 0;
    X = Xx;
    Y = Yy;
    #else
    return 0;
    #endif

    if (raw) {
    	*x = X;
    	*y = Y;
    	return result;
    }

    // Calibrate the result

	// tp_calx		tp_caly
	// LLLLRRRR 	TTTTBBBB
	int xmax = (tp_calx >> 16) & 0x3FFF;
	int xmin = tp_calx & 0x3FFF;
	int ymax = (tp_caly >> 16) & 0x3FFF;
	int ymin = tp_caly & 0x3FFF;

	int offset_x = X - xmin;
	int offset_y = Y - ymin;
	int range_x = xmax - xmin;
	int range_y = ymax - ymin;

	//
	// In 2.8" ILI3941+XPT2046 display X axis reads along the short side of display
	//
//	printf("RAW %d:%d | X: %d..%d : Y: %d..%d | W:%d H:%d\n", X, Y, xmin, xmax, ymin, ymax, _width, _height);
//	printf("offset %d:%d  %d:%d", offset_x, offset_y, (offset_x*1000)/(range_x), (offset_y*1000)/(range_y));

	if (((xmax - xmin) <= 0) || ((ymax - ymin) <= 0)) return 0;

    #if USE_TOUCH == TOUCH_TYPE_XPT2046
		int width = _lcd_width;
		int height = _lcd_height;

		switch (_lcd_orientation) {
			case PORTRAIT:
				X = xmax-offset_x;
				Y = Y;
				width = _height;
				height = _width;
				break;

			case PORTRAIT_FLIP:
				Y = ymax-offset_y;
				X = X;
				width = _height;
				height = _width;
				break;

			case LANDSCAPE:
				height = Y;
				Y = X;
				X = height;
				width = _height;
				height = _width;
				break;

			case LANDSCAPE_FLIP:
				Y = xmax-offset_x;
				X = ymax-offset_y;
				width = _height;
				height = _width;
				break;

			default:
				break;
        }

        X = ((X - xmin) * height) / (range_x);
        Y = ((Y - ymin) * width) / (range_y);

        if (X < 0) X = 0;
        if (X > height-1) X = height-1;
        if (Y < 0) Y = 0;
        if (Y > width-1) Y = width-1;

    #elif USE_TOUCH == TOUCH_TYPE_STMPE610
        int width = _width;
        int height = _height;
        if (_width > _height) {
            width = _height;
            height = _width;
        }
		X = ((X - xmax) * width) / (xmin - xmax);
		Y = ((Y - ymax) * height) / (ymin - ymax);

		if (X < 0) X = 0;
		if (X > width-1) X = width-1;
		if (Y < 0) Y = 0;
		if (Y > height-1) Y = height-1;

		switch (_lcd_orientation) {
			case PORTRAIT_FLIP:
				X = width - X - 1;
				Y = height - Y - 1;
				break;
			case LANDSCAPE:
				tmp = X;
				X = Y;
				Y = width - tmp -1;
				break;
			case LANDSCAPE_FLIP:
				tmp = X;
				X = height - Y -1;
				Y = tmp;
				break;
		}
    #endif
	*x = X;
	*y = Y;
	return result;
#endif // USE_TOUCH == TOUCH_TYPE_NONE
}


//
//
void TFT_Touch_XPT2046_read_timer_cb(void *arg)
{
	int tx, ty;
	int pin = (int)arg;
	bool touch_state = TFT_read_touch(&tx, &ty, false);

	if (touch_state) {
		TFT_drawCircle(tx, ty, 3, TFT_GREEN);
	} else {
		if (TFT_Touch_read_timer_id) {
			mgos_clear_timer(TFT_Touch_read_timer_id);
			TFT_Touch_read_timer_id = 0;
		}
		mgos_gpio_enable_int(pin);
	}

	TFT_Buttons_refresh(touch_state, tx, ty);
}


//
//
void TFT_Touch_XPT2046_intr_handler(const int pin, void *arg)
{
	const bool pin_state = mgos_gpio_read(pin);
	int touch_state = 0;
	int tx=0, ty=0;

	if (!pin_state)
	{
		mgos_gpio_disable_int(pin);

		if (!TFT_Touch_read_timer_id) {
			TFT_Touch_read_timer_id = mgos_set_timer(100, 1, TFT_Touch_XPT2046_read_timer_cb, (void *)pin);
		}

		if ((touch_state = TFT_read_touch(&tx, &ty, false))) {
			TFT_drawCircle(tx, ty, 5, TFT_MAGENTA);
		}
	}

	TFT_Buttons_refresh(touch_state, tx, ty);
}


//
//
bool TFT_Touch_intr_init(void)
{
	const int pin = mgos_sys_config_get_tft_t_irq_pin();

	printf("==== TFT_Touch_intr_init: init touch intr\r\n");

	if (-1 != pin) {
		LOG(LL_INFO, ("Set touch intr handler for pin '%d'", pin));
		mgos_gpio_set_mode(pin, MGOS_GPIO_MODE_INPUT);
		mgos_gpio_set_pull(pin, MGOS_GPIO_PULL_NONE);
		mgos_gpio_set_int_handler(pin, MGOS_GPIO_INT_EDGE_NEG, TFT_Touch_XPT2046_intr_handler, NULL);
		mgos_gpio_enable_int(pin);
	}
	return true;
}


// Mongoose-OS init
//
bool mgos_tft_touch_init(void)
{
	_lcd_height = mgos_sys_config_get_tft_height();
	_lcd_width = mgos_sys_config_get_tft_width();
	_lcd_orientation = mgos_sys_config_get_tft_orientation();

	printf("==== mgos_tft_touch_init: _\r\n");

	TFT_Touch_intr_init();

#if USE_TOUCH
    gpio_pad_select_gpio(PIN_NUM_TCS);
    gpio_set_direction(PIN_NUM_TCS, GPIO_MODE_OUTPUT);
#endif

	return true;
}
