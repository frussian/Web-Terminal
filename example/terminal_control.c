/*
 * terminal_control.c
 *
 *  Created on: 5 апр. 2021 г.
 *      Author: andrey
 */


#include "module.h"
#include "module_control.h"


#define FLOW_CONTROL_REG			0x13
#define DISABLE_FLOW_CONTROL		0x00

#define ENABLE_FLOW_CONTROL			0xA8
#define FLOW_LVL_REG				0x0F
#define XON1_REG					0x14
#define XOFF1_REG					0x16

#define TX_FIFO_DATA_SIZE			0x11

#define RX_FIFO_DATA_SIZE			0x12
#define RX_FIFO_BUFF				0x00
#define LSR_REG						0x04

#define BRGCONFIG_REG				0x1B
#define DIVLSB_REG					0x1C
#define DIVMSB_REG					0x1D
#define LCR_REG						0x0B

#define DISABLE_CLOCK			0x40
#define ENABLE_CLOCK			0x00

const unsigned char adrs[MAX_PORTS_NUM] = {0x6C, 0x5C, 0x2C, 0x1C, 0x64, 0x54, 0x24, 0x14};

int module_terminal_configure(cfgnode_t cfg, struct TERMINAL_CONFIG*tcfg)
{
	tcfg->term_bauds = cfgparser_get_int(cfg, "term_bauds", 115200);
	tcfg->term_bits = cfgparser_get_int(cfg, "term_bits", 8);
	tcfg->term_parity = cfgparser_get_int(cfg, "term_parity", 0);
	tcfg->term_stop = cfgparser_get_int(cfg, "term_stop", 1);
	tcfg->term_flow = cfgparser_get_int(cfg, "term_flow", 1);
	tcfg->term_work = cfgparser_get_bool(cfg, "term_work", 1);

	return 0;
}

int module_terminal_buf_reset(struct MODULE_STATE*state, int dev_ind)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	tb->len_read = tb->len_act = tb->pos_read = 0;
	tb->cur_x = tb->cur_y = 0;
	tb->max_x = 80;
	tb->max_y = 25;
	return 0;
}

int module_terminal_buf_write_reset(struct MODULE_STATE*state, int dev_ind)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	tb->len_write = 0;
	return 0;
}

int module_terminal_term(struct MODULE_STATE*state)
{
	return 0;
}

int module_terminal_init(struct MODULE_STATE*state)
{
	int r, div, i;

	for(i=0; i<MAX_PORTS_NUM; i++){
		state->term_state.terminal_mode[i] = 0;
		state->term_state.terminal_mode_write[i] = 0;

		module_terminal_buf_reset(state, i);
		module_terminal_buf_write_reset(state, i);
	}
	state->term_state.client_id = 0;

	r = module_select_i2c_bus(state, DEVICE_I2C_BUS_UART);
	if (r < 0) return r;

	div = 3686400 / 16 / state->term_cfg.term_bauds;
	DEBUG(NOTIFY, "divisor = %d, 0x%04X", div, div);

	DEBUG(NOTIFY, "configure port baud rate...");
	for(i=0; i<MAX_PORTS_NUM; i++){
		r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], BRGCONFIG_REG, DISABLE_CLOCK); // disable clock
		if (r) return 6;
		r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], DIVLSB_REG, div & 255);
		if (r) return 6;
		r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], DIVMSB_REG, div >> 8);
		if (r) return 6;

		{
			unsigned char reg = 0;
			DEBUG(NOTIFY, "configure port mode...");
			switch (state->term_cfg.term_bits) {
			case 5: break;
			case 6: reg |= 1; break;
			case 7: reg |= 2; break;
			case 8: reg |= 3; break;
			default: ERROR(5, "bad bits: %d", state->term_cfg.term_bits); return 7;
			}
			if (state->term_cfg.term_stop > 1) reg |= 4;
			switch (state->term_cfg.term_parity) {
			case 0: break;
			case 1: reg |= 0x08;
			case 2: reg |= 0x18;
			default: ERROR(5, "bad parity: %d", state->term_cfg.term_parity); return 7;
			}
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], LCR_REG, reg); // configure mode
			if (r) return 6;
		}

		if (state->term_cfg.term_flow) {
			DEBUG(NOTIFY, "configure flow control...");
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], FLOW_LVL_REG, 0x01); // flow levels
			if (r) return 6;
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], XON1_REG, 0x11); // XON
			if (r) return 6;
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], XOFF1_REG, 0x13); // XOFF
			if (r) return 6;
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], FLOW_CONTROL_REG, ENABLE_FLOW_CONTROL); // enable software flow control
			if (r) return 6;
		} else {
			DEBUG(NOTIFY, "disable flow control...");
			r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], FLOW_CONTROL_REG, DISABLE_FLOW_CONTROL); // disable flow control
			if (r) return 6;
		}
		r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[i], BRGCONFIG_REG, ENABLE_CLOCK); // enable clock
		if (r) return 6;
	}
	return 0;
}


static int terminal_configure_flow(struct MODULE_STATE*state, int dev_ind)
{
	int r;
	if (!state->term_cfg.term_flow) return 0;
	r = module_select_i2c_bus(state, DEVICE_I2C_BUS_UART);
	if (r < 0) return r;
	r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[dev_ind], FLOW_CONTROL_REG, ENABLE_FLOW_CONTROL); // enable software flow control
	if (r) return r;
	return 0;
}

static int terminal_deconfigure_flow(struct MODULE_STATE*state, int dev_ind)
{
	int r;
	char xon_str[2] = {0x00, 0x11};
	if (!state->term_cfg.term_flow) return 0;
	r = module_select_i2c_bus(state, DEVICE_I2C_BUS_UART);
	if (r < 0) return r;
	r = i2c_bus_write_reg_byte(&state->i2c_state, adrs[dev_ind], FLOW_CONTROL_REG, DISABLE_FLOW_CONTROL); // disable software flow control
	if (r) return r;
	r = i2c_bus_write_buf(&state->i2c_state, adrs[dev_ind], xon_str, 2);
	if (r) return r;
	return 0;
}

#define MAX_NPAR	16
static int handle_csi_params(struct MODULE_STATE*state, int dev_ind, const int params[MAX_NPAR], int n_params, int cmd)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	char s[256];
	DEBUG(NOTIFY, "n_params = %d, cmd = %d (%c)", n_params, cmd, cmd);
	switch (cmd) {
	case 'n':
		if (n_params == 1) {
			switch (params[0]) {
			case 5:
				module_terminal_post(state, "\x1B[0n", dev_ind);
				break;
			case 6:
//resp:
				snprintf(s, sizeof(s), "\x1B[%d;%dR", tb->cur_y + 1, tb->cur_x + 1);
				module_terminal_post(state, s, dev_ind);
				break;
			}
		}
		break;
	case 'H': {
		int x, y;
		if (n_params == 2) { y = params[0]; x = params[1]; }
		else if (n_params == 1) { y = params[0]; x = 0; }
		else break;
		if (x < 1) x = 1;
		if (y < 1) y = 1;
		if (x >= tb->max_x) x = tb->max_x - 1;
		if (y >= tb->max_y) y = tb->max_y - 1;
		tb->cur_x = x;
		tb->cur_y = y;
		DEBUG(NOTIFY, "set pos to (%d, %d)", tb->cur_x, tb->cur_y);
//		goto resp;
		}
		break;
	}
	return 0;
}

static int handle_csi(struct MODULE_STATE*state, int dev_ind, const char*s, int *pl, char*d)
{
	int n_params = 0, ndig = 0;
	int params[MAX_NPAR] = { 0 };
	int ind = 0;

	if (*pl < 1) return -1;
	switch (*s) {
	case '[':
		if (*pl < 2) return -1;
		*pl = 2;
		return 0;
	case '?':
		++ind;
		break;
	}
	for (; ind < *pl; ++ind) {
		switch (s[ind]) {
		case ';':
			++n_params;
			if (n_params == MAX_NPAR) return 0; // ignore
			params[n_params] = 0;
			ndig = 0;
			break;
		case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
			params[n_params] *= 10;
			params[n_params] += s[ind] - '0';
			++ndig;
			break;
		default:
			*pl = ind + 1;
			if (ndig) ++n_params;
			return handle_csi_params(state, dev_ind, params, n_params, s[ind]);
		}
	}
	return -1; // incomplete
}

static int handle_osc(struct MODULE_STATE*state, int dev_ind, const char*s, int *pl, char*d)
{
	int ind;
	if (*pl < 2) return -1;
	for (ind = 0; ind < *pl; ++ind) {
		if (s[ind] == 0x07) { // BEL
			*pl = ind + 1;
			return 0;
		} else if (ind < *pl - 1 && s[ind] == 0x1B && s[ind + 1] == '\\') { // ESC ST
			*pl = ind + 2;
			return 0;
		}
	}
	return -1;
}

static int handle_esc(struct MODULE_STATE*state, int dev_ind, const char*s, int *pl, char*d)
{
	int r;
	if (*pl < 1) return -1;
	switch (*s) {
	case '[':
		*pl = *pl - 1;
		r = handle_csi(state, dev_ind, s + 1, pl, d);
		if (r < 0) return r;
		*pl = *pl + 1;
		return r;
	case ']':
		*pl = *pl - 1;
		r = handle_osc(state, dev_ind, s + 1, pl, d);
		if (r < 0) return r;
		*pl = *pl + 1;
		return r;
	case '(':
	case ')':
	case '%':
	case '#':
		if (*pl < 2) return -1;
		*pl = 2;
		return 0;
//	case 'Z':
//		module_terminal_post(state, "\x1B[?6c", dev_ind); // VT102 answer
//		goto def;
	default:
//def:		
		*pl = 1;
		return 0;
	}
}

static int handle_char(struct MODULE_STATE*state, int dev_ind, int ch, int *pl, char*d)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	switch (ch) {
	case '\r':
		tb->cur_x = 0;
		break;
	case '\n':
		if (tb->cur_y < tb->max_y - 1) tb->cur_y++;
		tb->cur_x = 0;
		break;
	default:
		if (tb->cur_x < tb->max_x - 1) tb->cur_x++;
		else {
			tb->cur_x = 0;
			if (tb->cur_y < tb->max_y - 1) tb->cur_y++;
		}
	}
	*d = ch;
	*pl = 1;
	return 1;
}

static int terminal_escapes(struct MODULE_STATE*state, int dev_ind)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	const char *s = tb->term_buff + tb->len_act;
	char *d = tb->term_buff + tb->len_act;
	int l = tb->len_act, w = tb->len_act;
	DEBUG(NOTIFY, "len_read = %d, len_act = %d", tb->len_read, tb->len_act);
	for (; l < tb->len_read;) {
		int r, rl;
//		DEBUG(NOTIFY, "l = %d, w = %d, *s = %c", l, w, *s);
		switch (*s) {
		case 0x1B:
			rl = tb->len_read - l - 1;
			r = handle_esc(state, dev_ind, s + 1, &rl, d);
			++rl;
			break;
		case 0x9B:
			rl = tb->len_read - l - 1;
			r = handle_csi(state, dev_ind, s + 1, &rl, d);
			++rl;
			break;
		default:
			rl = tb->len_read - l;
			r = handle_char(state, dev_ind, *s, &rl, d);
			break;
		}
		if (r == -1) { // incomplete sequence
			if (l < tb->len_read) {
				memmove(d, s, tb->len_read - l);
				tb->len_read = w + tb->len_read - l;
				goto end;
			}
		}
		s += rl;
		l += rl;
		d += r;
		w += r;
		tb->len_act += r;
	}
	tb->len_read = w;
end:
	tb->len_act = w;
	DEBUG(NOTIFY, "resulting len_read = %d, len_act = %d", tb->len_read, tb->len_act);
	return 0;
}

static int terminal_read(struct MODULE_STATE*state, int mode, int dev_ind)
{
	int r, nr = 0;
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;

	r = module_select_i2c_bus(state, DEVICE_I2C_BUS_UART);
	if (r < 0) return r;

	nr = i2c_bus_read_reg_byte(&state->i2c_state, adrs[dev_ind], RX_FIFO_DATA_SIZE); // rx fifo data size
	if (nr < 0) return 6;
	if (nr > 0) {
		if (nr == 128 && !state->term_cfg.term_flow) { // overflow is possible
			r = i2c_bus_read_reg_byte(&state->i2c_state, adrs[dev_ind], LSR_REG); // read LSR
			if (r < 0) return 6;
			if (r & 2) {
				DEBUG(IMPORTANT, "TERMINAL#%d: RX overflow", dev_ind + 1);
			}
			if (r & 4) {
				DEBUG(IMPORTANT, "TERMINAL#%d: Parity Error", dev_ind + 1);
			}
			if (r & 8) {
				DEBUG(IMPORTANT, "TERMINAL#%d: Frame Error", dev_ind + 1);
			}
			if (r & 0x20) {
				DEBUG(IMPORTANT, "TERMINAL#%d: Rx Noise", dev_ind + 1);
			}
		}
		if (tb->len_read + nr > sizeof(tb->term_buff)) {
			int d = tb->len_read + nr - sizeof(tb->term_buff);
			tb->len_read -= d;
			if (tb->pos_read > d) tb->pos_read -= d;
			else tb->pos_read = 0;
			if (tb->len_act > d) tb->len_act -= d;
			else tb->len_act = 0;
			memmove(tb->term_buff, tb->term_buff + d, tb->len_read);
		}
		r = i2c_bus_read_reg_buf(&state->i2c_state, adrs[dev_ind], RX_FIFO_BUFF, tb->term_buff + tb->len_read, nr);
		if (r<0) return 6;
		DEBUG_DATA(NOTIFY, tb->term_buff + tb->len_read, nr, "TERMINAL#%d: READ", dev_ind + 1);
		if (tb->skip_read) {
			--tb->skip_read;
		} else {
			tb->len_read += nr;
			terminal_escapes(state, dev_ind);
		}
	}

	return 0;
}

static int terminal_write(struct MODULE_STATE*state, int dev_ind) {

	int nr;
	int r;

	r = module_select_i2c_bus(state, DEVICE_I2C_BUS_UART);
	if (r < 0) return r;

	r = i2c_bus_read_reg_byte(&state->i2c_state, adrs[dev_ind], TX_FIFO_DATA_SIZE); // tx fifo data size
	if (r < 0) {
		return r;
	}

	nr = 128 - r;
	if (nr > 0) {
		struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
		int l = tb->len_write;
		if (l > nr) l = nr;
		tb->term_buff_write[0] = 0; // register number
		r = i2c_bus_write_buf(&state->i2c_state, adrs[dev_ind], tb->term_buff_write, l + 1);
		if (r) {
			return r;
		}
//		DEBUG(NOTIFY, "TERMINAL#%d: written %d/%d byte(s)", dev_ind + 1, l, tb->len_write);
		DEBUG_DATA(NOTIFY, tb->term_buff_write + 1, l, "TERMINAL#%d: WRITE", dev_ind + 1);
		if (tb->len_write > l) {
			tb->len_write -= l;
			memmove(tb->term_buff_write + 1, tb->term_buff_write + l + 1, tb->len_write);
		} else {
			tb->len_write = 0;
		}
	}
	return 0;
}


static int terminal_activate(struct MODULE_STATE*state, int dev_ind)
{
	if(state->term_state.terminal_mode[dev_ind] == 0){
		state->term_state.terminal_mode[dev_ind] |= TERMINAL_GET_STR;
		DEBUG(MEDIUM, "Запущен терминал #%d", dev_ind + 1);
		state->term_buff[dev_ind].skip_read = 1;
	}
	module_loop_set_delay(state, 0);
	terminal_configure_flow(state, dev_ind);
	state->term_state.start_term_ts[dev_ind] = state->cur_ts;
	return 0;
}

static int terminal_deactivate(struct MODULE_STATE*state, int dev_ind)
{
	state->term_state.terminal_mode[dev_ind] &= ~TERMINAL_GET_STR;
	DEBUG(NOTIFY, "Терминал %d остановлен", dev_ind + 1);
	terminal_deconfigure_flow(state, dev_ind);
	module_loop_set_delay(state, 100);
	return 0;
}

int module_terminal_loop(struct MODULE_STATE*state)
{
	int i;
	for(i=0; i<MAX_PORTS_NUM; i++){

		if(state->term_state.terminal_mode[i] & TERMINAL_GET_STR){

			terminal_read(state, STR, i);
//			DEBUG(NOTIFY, "Чтение терминала = %d", i+1);

			if ((transport_get_difftime_msec(state->term_state.start_term_ts[i], state->cur_ts)) > TERMINAL_TIMEOUT) {
				terminal_deactivate(state, i);
			}
		}

		if(state->term_state.terminal_mode_write[i] & TERMINAL_WRITE){
			terminal_write(state, i);
//			DEBUG(NOTIFY, "Данные с WEB = %s", state->term_buff[i].term_buff_write);
			state->term_state.terminal_mode_write[i] &= ~TERMINAL_WRITE;
		}

	}

	return 0;
}

int module_terminal_report(struct MODULE_STATE*state, transport_packet_t resp, int mode, int dev_ind)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	terminal_activate(state, dev_ind);
	switch (mode) {
	case STR:
		transport_add_string(resp, SORM_DEV_TERMINAL_TEXT,  tb->term_buff + tb->pos_read, tb->len_act - tb->pos_read);
		tb->pos_read = tb->len_act;
		break;
	case BUFFER:
		transport_add_string(resp, SORM_DEV_TERMINAL_TEXT,  tb->term_buff, tb->len_act);
		break;
	}
	return 0;
}

int module_terminal_post(struct MODULE_STATE*state, const char*str, int dev_ind)
{
	struct TERMINAL_BUFF*tb = state->term_buff + dev_ind;
	int len;
	len = strlen(str);
	if (len + tb->len_write >= sizeof(tb->term_buff_write)) {
		WARN(NOTIFY, "Переполнение буфера терминала #%d", dev_ind + 1);
		len = sizeof(tb->term_buff_write) - tb->len_write - 1;
	}
	if (!len) return 0; // empty string
	memcpy(tb->term_buff_write + 1 + tb->len_write, str, len);
	tb->len_write += len;
	state->term_state.terminal_mode_write[dev_ind] |= TERMINAL_WRITE;
	terminal_activate(state, dev_ind);
	return 0;
}
