var ump_control_blocks = [];

function terminal_settings_set(block)
{
	if (!block.terminal_settings_formpanel) return;
	var baud_rate = block.terminal_settings_formpanel.getComponent('baud_rate_combobox').getValue();
	var data_bits = block.terminal_settings_formpanel.getComponent('data_bits_combobox').getValue();
	var stop_bits = block.terminal_settings_formpanel.getComponent('stop_bits_combobox').getValue();
	var parity    = block.terminal_settings_formpanel.getComponent('parity_combobox').getValue();
	var sw_flow   = block.terminal_settings_formpanel.getComponent('sw_flow_combobox').getValue();
	console.log('setting baud_rate', baud_rate, 'data_bits', data_bits, 'stop bits', stop_bits, 'parity', parity, 'sw_flow', sw_flow);

	var terminal_output = block.terminal_output;
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_conf_set',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id,
			dev_terminal_baud_rate: baud_rate,
			dev_terminal_data_bits: data_bits,
			dev_terminal_stop_bits: stop_bits,
			dev_terminal_parity: parity,
			dev_terminal_sw_flow: sw_flow
		},
		success: function (response, opts) {
			var json;
			try {
				json = JSON.parse(response.responseText);
			} catch(e) {
				return;
			}
			if (json.error_code !== undefined) {
				if (json.error_text !== undefined) {
					Ext.MessageBox.alert('Ошибка при установке настроек терминала', 'Ошибка при установке настроек терминала: ' + json.error_text);
				} else {
					Ext.MessageBox.alert('Ошибка при установке настроек терминала', 'Ошибка при установке настроек терминала: команда не выполнена');
				}
			} else {
				if (json.control_test !== undefined) {
					Ext.MessageBox.alert('Установка настроек терминала', json.control_text);
				} else {
					Ext.MessageBox.alert('Установка настроек терминала', 'Настройки установлены');
				}
			}
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при установке настроек терминала', 'Ошибка при установке настроек терминала: ' + response.status);
		}
	});
};

function terminal_settings_get(block)
{
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_conf_get',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id,
		},
		success: function (response, opts) {
			var json;
			try {
				json = JSON.parse(response.responseText);
			} catch(e) {
				return;
			}
			if (json.error_code !== undefined) {
				if (json.error_text !== undefined) {
					Ext.MessageBox.alert('Ошибка при запросе настроек терминала', 'Ошибка при запросе настроек терминала: ' + json.error_text);
				} else {
					Ext.MessageBox.alert('Ошибка при запросе настроек терминала', 'Ошибка при запросе настроек терминала: команда не выполнена');
				}
			} else {
				if (json.control_test !== undefined) {
					Ext.MessageBox.alert('Установка настроек терминала', json.control_text);
				} else {
					Ext.MessageBox.alert('Установка настроек терминала', 'Настройки установлены');
				}
				if (!block.terminal_settings_formpanel) return;
				var baud_rate_combobox = block.terminal_settings_formpanel.getComponent('baud_rate_combobox');
				var data_bits_combobox = block.terminal_settings_formpanel.getComponent('data_bits_combobox');
				var stop_bits_combobox = block.terminal_settings_formpanel.getComponent('stop_bits_combobox');
				var parity_combobox = block.terminal_settings_formpanel.getComponent('parity_combobox');
				var sw_flow_combobox = block.terminal_settings_formpanel.getComponent('sw_flow_combobox');
				baud_rate_combobox.setValue(json.dev_terminal_baud_rate);
				data_bits_combobox.setValue(json.dev_terminal_data_bits);
				stop_bits_combobox.setValue(json.dev_terminal_stop_bits);
				parity_combobox.setValue(json.dev_terminal_parity);
				sw_flow_combobox.setValue(json.dev_terminal_sw_flow);
			}
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при запросе настроек терминала', 'Ошибка при запросе настроек терминала: ' + response.status);
		}
	});
};

function terminal_settings_formpanel_gen(block)
{
	var baud_rate_array = [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200];
	var stop_bits_array = [1, 2];
	var data_bits_array = [5, 6, 7, 8];
	var parity_array = [
		[UMP_BLOCK_TERMINAL_PARITY_NONE, 'Без контроля'],
		[UMP_BLOCK_TERMINAL_PARITY_ODD,  'Нечетность'],
		[UMP_BLOCK_TERMINAL_PARITY_EVEN, 'Четность']
	];
	var sw_flow_array = [
		[UMP_BLOCK_TERMINAL_SW_FLOW_OFF, 'Отключен'],
		[UMP_BLOCK_TERMINAL_SW_FLOW_ON,  'Включен']
	];

	var default_combobox_settings = {
		mode: 'local',
		allowBlank: false,
		blankText: 'Поле является обязтельным для ввода',
		forceSelection: true,
		triggerAction: 'all',
		emptyText:'Выберите значение...',
		selectOnFocus: true,
	};

	var baud_rate_combo = new Ext.form.ComboBox(Object.assign({
		store: baud_rate_array,
		fieldLabel: 'Скорость (бод)',
		itemId: 'baud_rate_combobox'
	}, default_combobox_settings));

	var data_bits_combo = new Ext.form.ComboBox(Object.assign({
		store: data_bits_array,
		fieldLabel: 'Биты данных',
		itemId: 'data_bits_combobox'
	}, default_combobox_settings));

	var stop_bits_combo = new Ext.form.ComboBox(Object.assign({
		store: stop_bits_array,
		fieldLabel: 'Стоповые биты',
		itemId: 'stop_bits_combobox'
	}, default_combobox_settings));

	var parity_combo = new Ext.form.ComboBox(Object.assign({
		store: parity_array,
		fieldLabel: 'Контроль четности',
		itemId: 'parity_combobox'
	}, default_combobox_settings));

	var sw_flow_combo = new Ext.form.ComboBox(Object.assign({
		store: sw_flow_array,
		fieldLabel: 'Программный контроль потока',
		itemId: 'sw_flow_combobox'
	}, default_combobox_settings));

	var formpanel = new Ext.form.FormPanel({
		bodyStyle: 'margin: 0px; padding: 5px 3px;',
		anchor: '100%, 10%',
		hidden: ump_blocks_terminal_allow_settings != true,
		collapsible: true,
		collapsed: true,
		region: 'east',
		title: 'Настройки терминала',
		items: [
			baud_rate_combo,
			data_bits_combo,
			stop_bits_combo,
			parity_combo,
			sw_flow_combo
		],
		buttons: [
			{
				text: 'Сохранить',
				handler: function () {
					var baud_rate_valid = baud_rate_combo.isValid();
					var data_bits_valid = data_bits_combo.isValid();
					var stop_bits_valid = stop_bits_combo.isValid();
					var parity_valid = parity_combo.isValid();
					var sw_flow_valid = sw_flow_combo.isValid();
					if (
						!baud_rate_valid ||
						!data_bits_valid ||
						!stop_bits_valid ||
						!parity_valid ||
						!sw_flow_valid)
					{
						return;
					}
					terminal_settings_set(block);
				}
			}
		]
	});
	return formpanel;
}

function terminal_reset(block)
{
	var terminal_output = block.terminal_output;
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_reset',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id
		},
		success: function (response, opts) {
			var html = '', state;
			try {
				state = JSON.parse(response.responseText);
			} catch(e) {
				terminal_output.waiting_restore = 0;
//				terminal_input.waiting_restore = 0;
				return;
			}

			terminal_output.html = '';
			terminal_output.update(terminal_output.html)

//			terminal_output.el.dom.scrollTop = terminal_output.el.dom.scrollHeight;
			terminal_output.waiting_restore = 0;
			terminal_output.el.scroll('down', 10000);
			
//			terminal_input.html = '';
//			terminal_input.update(terminal_input.html)
			
//			terminal_input.el.dom.scrollTop = terminal_input.el.dom.scrollHeight;
//			terminal_input.waiting_restore = 0;
			
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при сбросе терминала', 'Ошибка при сбросе терминала: ' + response.status);
			terminal_output.waiting_restore = 0;
//			terminal_input.waiting_restore = 0;
		}
	});
}


function terminal_restore(block)
{
	var terminal_output = block.terminal_output;
	terminal_output.waiting_restore = 1;
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_restore',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id
		},
		success: function (response, opts) {
			var html = '', state;
			try {
				state = JSON.parse(response.responseText);
			} catch(e) {
				terminal_output.waiting_restore = 0;
				return;
			}
			if (state.dev_terminal_text){
				html += state.dev_terminal_text;
				if (!terminal_output.html) terminal_output.html = html;
				else terminal_output.html += html;
				terminal_output.update(terminal_output.html)
			}
//			terminal_output.el.dom.scrollTop = terminal_output.el.dom.scrollHeight;
			terminal_output.el.scroll('down', 10000);
			terminal_output.waiting_restore = 0;
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при восстановлении истории терминала', 'Ошибка при восстановлении истории терминала: ' + response.status);
			terminal_output.waiting_restore = 1;
		}
	});
}

function terminal_get(block)
{
	var terminal_output = block.terminal_output;
	if (terminal_output.waiting_restore) return;
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_get',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id
		},
		success: function (response, opts) {
			var html = '', state;
			try {
				state = JSON.parse(response.responseText);
			} catch(e) {
				return;
			}
			if (state.dev_terminal_text){
				html += state.dev_terminal_text;
				if (!terminal_output.html) terminal_output.html = html;
				else terminal_output.html += html;
				terminal_output.update(terminal_output.html)
			}
			terminal_output.el.dom.scrollTop = terminal_output.el.dom.scrollHeight;
			terminal_output.el.scroll('down', 10000);
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при запросе данных терминала', 'Ошибка при запросе данных терминала: ' + response.status);
		}
	});
}

function terminal_put(block, input) {
//	alert('terminal_input');
	var terminal_output = block.terminal_output;
	var cmd_line = ''
/*	
	if (input) {
		var lines = input.split('\n')
		lines.forEach(function(element) {
			//element = element.replace("__switch_control", "")
			//element = element.replace("__force_switch_control", "")
			if (element.length > 0) {
				cmd_line += '> '
				cmd_line += element
				cmd_line += '\n'
			}
		});
		if (!terminal_output.html) terminal_output.html = cmd_line;
		else {
			var last_char = terminal_output.html.slice(-1);
			if (last_char != '\n') {
				terminal_output.html += '\n';
			}
			terminal_output.html += cmd_line;
		}
		terminal_output.update(terminal_output.html)
	}
*/	
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/dev_terminal_put',
		params: {
			dev_terminal_id: 0,
			dev_id: block.dev_id,
			dev_terminal_text: input
		},
		success: function (response, opts) {
			var html = '', state;
			try {
				state = JSON.parse(response.responseText);
			} catch(e) {
				return;
			}
			if(state.dev_terminal_text){
				html += state.dev_terminal_text;
				if(!terminal_output.html) terminal_output.html = html;
				else terminal_output.html += html;
				terminal_output.update(terminal_output.html)
			}
//			terminal_output.el.dom.scrollTop = terminal_output.el.dom.scrollHeight;
			terminal_output.el.scroll('down', 10000);
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка при отправке данных терминала', 'Ошибка при отправке данных терминала: ' + response.status);
		}
	});
};

function terminal_open(block)
{
	var terminal_title = '';
	if (block.dev_data && block.dev_data.dev_text) {
		terminal_title = block.dev_data.dev_text + ': терминал';
	} else if (block.orig_title) {
		terminal_title = block.orig_title + ': терминал';
	} else {
		terminal_title = 'Терминал';
	}
	
	var terminal_io_panel = new Ext.Panel({
		region: 'center',
		width: '100%',
		height: '100%',
		layout: 'anchor',
		items: [
				{
				xtype: 'textarea',
				readOnly: true,
				enableKeyEvents: true,
				itemId: 'terminal_output',
				allowBlank: true,
				autoScroll: true,
				width: '100%',
				anchor: '100% 90%',
				style: "font-family:monospace;",
				listeners: {
					keydown: function(el, e) {
						var k = e.getKey();
						if (e.ctrlKey && k > 0x40) {
							var s = String.fromCharCode(k - 0x40);
							terminal_put(block, s);
							e.preventDefault();
							e.stopEvent()
						}
						if (k == e.BACKSPACE) {
							terminal_put(block, '\b');
							e.stopEvent()
						} else if (k == e.TAB) {
							terminal_put(block, '\t');
							e.stopEvent()
						} else if (k == e.ENTER) {
							terminal_put(block, '\n');
							e.stopEvent()
						} else if (k == e.ESC) {
							terminal_put(block, '\x1B');// not works?
							e.stopEvent()
						}
					},
					keypress: function(el, e) {
						var c = e.getCharCode();
						var s = String.fromCharCode(c);
						terminal_put(block, s);
						e.stopEvent()
					},
					afterrender: function() {
						this.focus(false, 100);
					},
				},
			},
			{
				xtype: 'textfield',
				itemId: 'terminal_input',
				allowBlank: true,
				width: '100%',
				height: 20,
				anchor: '100% 10%',
				enableKeyEvents : true,
				listeners: {
					keypress : function(field, e) {
						var key = e.getKey();
						if (key === e.ENTER) {
							if (e.ctrlKey) {
								//enter a new line
							} else {
								terminal_put(block, block.terminal_input.getValue() + '\n');
								block.terminal_input.setValue('');
								e.stopEvent()
							}
						}
						if (key === e.ESC) {
							terminal_close(block);
						}
					}
				}
			},
		],
	});

	var terminal_settings_formpanel = terminal_settings_formpanel_gen(block);
	block.terminal_io_panel = terminal_io_panel;
	block.terminal_input = terminal_io_panel.getComponent('terminal_input');
	block.terminal_output = terminal_io_panel.getComponent('terminal_output');

	var terminal_window = new Ext.Window({
		width: '75%',
		block: block,
		modal: true,
		buttonAlign: 'left',
		closable: false,
		layout: 'border',
		title: terminal_title,
		height: '450',
		items: [
			terminal_io_panel,
			terminal_settings_formpanel
		],
		buttons: [
			{
				text: 'Отправить',
				icon: '/main/icons/right.png',
				handler: function () {
					terminal_put(block, block.terminal_input.getValue());
				}
			},
			{
				text: 'Очистить',
				icon: '/main/icons/refresh.png',
				handler: function () {
					terminal_reset(block);
				}
			},
			{
				text: 'Прервать',
				icon: '/main/icons/stop.png',
				handler: function () {
					terminal_put(block, '\x03');
				}
			},
			{
				text: 'Закрыть',
				icon: '/main/icons/cross.png',
				handler: function() {
					terminal_close(block);
				}
			}
		],
		renderTo: document.body,
	});
	

	terminal_window.block = block;
	terminal_io_panel.block = block;
	block.terminal_window = terminal_window;
	block.terminal_settings_formpanel = terminal_settings_formpanel;
	terminal_restore(block);
	terminal_window.show();
	params = {
		dev_terminal_id: 0,
		dev_id: block.dev_id
	};
	var upd = terminal_window.body.getUpdater();
		upd.setRenderer({
			render: function(el, response, scripts, callback) {
				var state;
				var html = '';
				try {
					state = JSON.parse(response.responseText);
				} catch (e) {
					return true; // do nothing
				}
				if (state.dev_terminal_text) {
					html += state.dev_terminal_text;
					if (!block.terminal_output.html) block.terminal_output.html = html;
					else block.terminal_output.html += html;
					block.terminal_output.update(block.terminal_output.html)
				}
				block.terminal_output.el.scroll('down', 10000);
//				terminal_window.el.dom.scrollTop = terminal_window.el.dom.scrollHeight;
			}
		});
		upd.showLoading = function() { }
//		upd.on('failure', function(el, response) {
//			if (response.status == 403) { // forbidden
//				document.location.reload(true);
//			}
//		});
	upd.timeout = 0.25;
	upd.startAutoRefresh(0.3, '/' + ump_ctl_point + '/dev_terminal_get', params, null, true);
}

function terminal_close(block)
{
	if (block.terminal_window) {
		block.terminal_window.body.getUpdater().stopAutoRefresh();
		block.terminal_window.close();
		block.terminal_window = undefined;
	}
}

function ump_block_control_send(block, cmd_name, params = null)
{
	Ext.Ajax.request({
		url: '/' + ump_ctl_point + '/' + cmd_name,
		params: Object.assign({
			dev_id: block.dev_id,
		}, params),
/*
		success: function (response, opts) {
			var json = Ext.decode(response.responseText);
			if (json.error_code !== undefined) {
				if (json.error_text !== undefined) {
					Ext.MessageBox.alert('Ошибка выполнения команды', json.error_text);
				} else {
					Ext.MessageBox.alert('Ошибка выполнения команды', 'Во время выполнения команды произошла ошибка');
				}
			} else {
				if (json.control_test !== undefined) {
					Ext.MessageBox.alert('Выполнение команды', json.control_text);
				} else {
					Ext.MessageBox.alert('Выполнение команды', 'Команды принята к выполнению');
				}
			}
		},
		failure: function (response, opts) {
			Ext.MessageBox.alert('Ошибка выполнения команды', 'Ошибка во время отправки команды: ' + response.status);
		}
*/
	});
}

function ump_block_control_send_reset(block)
{
	return ump_block_control_send(block, 'dev_reset');
}

function ump_block_control_send_power_on(block)
{
	return ump_block_control_send(block, 'dev_power_on');
}

function ump_block_control_send_power_off(block)
{
	return ump_block_control_send(block, 'dev_power_off');
}

function ump_block_control_send_power_cycle(block)
{
	return ump_block_control_send(block, 'dev_power_cycle');
}


function ump_block_fan_control_send_low(block)
{
	return ump_block_control_send(block, 'dev_fan_set_speed_low');
}

function ump_block_fan_control_send_medium(block)
{
	return ump_block_control_send(block, 'dev_fan_set_speed_medium');
}

function ump_block_fan_control_send_high(block)
{
	return ump_block_control_send(block, 'dev_fan_set_speed_high');
}

function ump_block_fan_control_send_custom(block, value)
{
	return ump_block_control_send(block, 'dev_fan_set_speed_custom', { 'dev_fan_speed': value });
}

function ump_block_control_button(button_text, button_handler, block, icon='', confirm = false, xparams = null)
{
	button = new Ext.Button(Object.assign({
		width: '80%',
		//itemId: 'ump_control_reset',
		xtype: 'button',
		icon: icon,
		iconAlign: 'right',
		text: button_text,
		style: { 'text-align': 'left', 'margin': 'auto' },
		handler: function(btn) {
			if (!confirm) return button_handler.call(btn, block, btn);
			Ext.MessageBox.show({
				icon: Ext.MessageBox.QUESTION,
				title: 'Подтверждение',
				msg: 'Выполнить ' + btn.text + '?',
				buttons: {yes: 'Да', no: 'Нет'},
				fn: function(btn, text) {
					if (btn == 'yes') button_handler.call(btn, block, btn);
				}
			});
		}
	}, xparams));
	return button;
}


function ump_block_control_toggle(button_text, button_handler, block, icon='', confirm = false, xparams = null)
{
	button = new Ext.Button(Object.assign({
		width: '80%',
		xtype: 'button',
		icon: icon,
		iconAlign: 'left',
		enableToggle: true,
		text: button_text,
		style: { 'text-align': 'left', 'margin': 'auto' },
		listeners: {
		toggle: function(ctl, pressed) {
			if (!confirm) return button_handler.call(ctl, block, ctl, pressed);
			Ext.MessageBox.show({
				icon: Ext.MessageBox.QUESTION,
				title: 'Подтверждение',
				msg: 'Выполнить действие?',
				buttons: {yes: 'Да', no: 'Нет'},
				fn: function(btn, text) {
					if (btn == 'yes') button_handler.call(ctl, block, ctl, pressed);
				}
			});
		},
		},
	}, xparams));
	return button;
}

function ump_block_control_slider(slider_text, slider_handler, block)
{
	slider = {
		width: '95%',
		//itemId: 'ump_control_reset',
		xtype: 'slider',
		fieldLabel: slider_text,
		value: 25,
		minValue: 0,
		maxValue: 100,
		style: { 'margin': 'auto' },
		tipText: function(thumb){
			return String(thumb.value) + '%';
		},
		/*
		do_reset: function() {
			this.setValue((this.data.audio_param_value == undefined)? this.data.audio_param_values[this.index]: this.data.audio_param_value);
			this.upd_label();
		},
		do_default: function() {
			if (this.data.audio_param_def != undefined) this.setValue(this.data.audio_param_def);
			else this.setValue((this.data.audio_param_min + this.data.audio_param_max) >> 1);
			this.upd_label();
		},
		get_param: function() {
			var val = this.getValue();
			if (val == this.data.audio_param_value) return null;
			return {audio_param_id: this.data.audio_param_id, audio_param_index: this.index, audio_param_value: val};
		},
		upd_label: function(v) {
			var pct = ((v == undefined? this.value: v) - this.minValue) * 100.0 / (this.maxValue - this.minValue);
			this.label.update(this.t + ' [' + pct.toFixed(1) + '%]:');
		},
		*/
		listeners: {
			change: function(slider, newValue, thumb) {
				slider_handler(block, newValue);
				//sl.upd_label(n);
			},
			afterrender: function(sl) {
				//sl.upd_label();
			},
		},

	};
	return slider;
}

function ump_block_init(panel)
{
	panel.removeAll();
	panel.enable();
	panel.update('');
}

function ump_block_uncontrollable_control(panel, code)
{
	if (panel.prev_type == code) return;
	panel.prev_type = code;

	ump_block_init(panel);

	panel.update('Блок не имеет уникальных команд управления'.italics());

	panel.doLayout();
}

function ump_block_power_220v_control(panel)
{
	ump_block_update_widgets(panel, 0);
	return ump_block_uncontrollable_control(panel, UMP_BLOCK_TYPE_POWER_220V);
}

function ump_block_power_48v_control(panel)
{
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_ON | UMP_BLOCK_CONTROL_BUTTON_OFF | UMP_BLOCK_CONTROL_BUTTON_CYCLE);
	return ump_block_uncontrollable_control(panel, UMP_BLOCK_TYPE_POWER_48V);
}

function ump_block_digital_control(panel)
{
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_ALL);
	return ump_block_uncontrollable_control(panel, UMP_BLOCK_TYPE_DIGITAL);
}

function ump_block_empty_control(panel)
{
	if (panel.prev_type == UMP_BLOCK_TYPE_EMPTY) return;
	panel.prev_type = UMP_BLOCK_TYPE_EMPTY;

	ump_block_init(panel);

	panel.disable();

	ump_block_update_widgets(panel, 0);

	panel.doLayout();
}

function ump_block_unsupported_control(panel, type)
{
	if (panel.prev_type == type) return;
	panel.prev_type = type;

	ump_block_init(panel);

	panel.update('Не поддерживается'.italics());

	panel.doLayout();
}

function ump_block_fan_control(panel)
{
	if (panel.prev_type == UMP_BLOCK_TYPE_FAN) return;
	panel.prev_type = UMP_BLOCK_TYPE_FAN;

	ump_block_init(panel);

	panel.add(ump_block_control_button('Установить низкую скорость', ump_block_fan_control_send_low, panel, 'main/icons/fan_speed_low.png'));
	panel.add(ump_block_control_button('Установить среднюю скорость', ump_block_fan_control_send_medium, panel, 'main/icons/fan_speed_medium.png'));
	panel.add(ump_block_control_button('Установить высокую скорость', ump_block_fan_control_send_high, panel, 'main/icons/fan_speed_high.png'));
	panel.add(ump_block_control_slider('Задать скорость', ump_block_fan_control_send_custom, panel));

	ump_block_update_widgets(panel, 0);

	panel.doLayout();
}

function ump_block_pstn_play(block, ctl, p)
{
	if (ctl.audio === undefined) {
		ctl.audio = null;
		ctl.initialConfig.enableToggle = true;
		ctl.start_audio = function() {
			if (this.audio != null) return;
			this.audio = block.el.insertFirst({
				tag: 'audio',
//				src: '/'+this.node+'/get_data?probe_cont_format=wav&data_code=audio&dev_slot='+block.dev_id+'&dev_port='+this.port_num+'&snd_format=alaw&snd_rate=8000&_dc=' + new Date().getTime(),
				src: '/'+this.node+'/get_data?probe_cont_format=wav&data_code=audio&dev_slot='+block.dev_id+'&dev_port='+this.port_num+'&_dc=' + new Date().getTime(),
				preload: 'none',
//				controls: false,
				autoplay: true,
			});
			this.audio.ctl = ctl;
			this.audio.on('error', function() {
				this.ctl.toggle(false);
			});
			this.audio.on('ended', function() {
				this.ctl.reload_audio();
			});
		}
		ctl.stop_audio = function() {
			if (this.audio != null) {
				this.audio.dom.src = 'about:blank';
				this.audio.remove();
			}
			this.audio = null;
		}
		ctl.reload_audio = function() {
			if (!this.pressed && (this.OwnerCt !== undefined && !this.OwnerCt.collapsed)) {
				this.stop_audio();
				this.start_audio();
			}
		}
	}
	if (p) {
		ctl.start_audio();
	} else {
		ctl.stop_audio();
	}
	return 0;
}


function pstn_an_parser(state)
{
	var html = '';
//	if (state.error_code == 'ST') return null;
	if (state.error_text != undefined) {
		html += '<div class=status-error>Ошибка: ' + state.error_text + '</div>';
	} else if (state.status_text != undefined) {
		html += state.status_text;
	}
	return '<font size=+1>'+html+'</font>';
}

function ump_block_pstn_an(block, ctl, p)
{
	if (ctl.analysis === undefined) {
		ctl.analysis = null;
		ctl.initialConfig.enableToggle = true;
		ctl.start_analysis = function() {
			var pos;
			if (this.analysis != null) return;
			pos = viewport.nextWindowPos(600, 100);
			this.analysis = new Ext.Window({
				title: 'Анализ сигнала ('+block.dev_id+':'+this.port_num+')',
				collapsible: true,
				closable: true,
				autoScroll: true,
				minWidth: 100,
				minHeight: 100,
				shadow: false,
				ctl: ctl,
				x: pos.x,
				y: pos.y,
				listeners: {
					close: function() {
						this.ctl.toggle(false);
					},
				},
			});
			viewport.add(this.analysis);
			setAutoRefreshStatus(this.analysis, 0.5, '/'+this.node+'/status_an', {dev_slot: block.dev_id, dev_port: this.port_num}, pstn_an_parser, true, false);
			this.analysis.show();
		}
		ctl.stop_analysis = function() {
			if (this.analysis != null) {
				this.analysis.close();
			}
			this.analysis = null;
		}
	}
	if (p) {
		ctl.start_analysis();
	} else {
		ctl.stop_analysis();
	}
	return 0;
}


function ump_block_upn_play(block, ctl, p)
{
	if (ctl.audio === undefined) {
		ctl.audio = null;
		ctl.initialConfig.enableToggle = true;
		ctl.start_audio = function() {
			if (this.audio != null) return;
			this.audio = block.el.insertFirst({
				tag: 'audio',
				src: '/'+this.node+'/get_data?probe_cont_format=wav&data_code=audio&dev_slot='+block.dev_id+'&dev_port='+this.port_num+'&dev_line='+this.line_num+'&_dc=' + new Date().getTime(),
				preload: 'none',
//				controls: false,
				autoplay: true
			});
			this.audio.ctl = ctl;
			this.audio.on('error', function() {
				this.ctl.toggle(false);
			});
			this.audio.on('ended', function() {
				this.ctl.reload_audio();
			});
		}
		ctl.stop_audio = function() {
			if (this.audio != null) {
				this.audio.dom.src = 'about:blank';
				this.audio.remove();
			}
			this.audio = null;
		}
		ctl.reload_audio = function() {
			if (!this.pressed && (this.OwnerCt !== undefined && !this.OwnerCt.collapsed)) {
				this.stop_audio();
				this.start_audio();
			}
		}
	}
	if (p) {
		ctl.start_audio();
	} else {
		ctl.stop_audio();
	}
	return 0;
}


function ump_block_isdn_show(block, ctl)
{
	window.open('/main/container/?node=' + ctl.node + '&dev_slot=' + block.dev_id + '&dev_port='+ctl.port_num);
	return 0;
}




function ump_block_pstn_control(panel)
{
	if (panel.prev_type == UMP_BLOCK_TYPE_ANALOG) return;
	panel.prev_type = UMP_BLOCK_TYPE_ANALOG;

	ump_block_init(panel);

	panel.add(ump_block_control_toggle('Порт 1', ump_block_pstn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-pstn', port_num: 1}));
	panel.add(ump_block_control_toggle('Порт 2', ump_block_pstn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-pstn', port_num: 2}));
	panel.add(ump_block_control_toggle('Порт 3', ump_block_pstn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-pstn', port_num: 3}));
	panel.add(ump_block_control_toggle('Порт 4', ump_block_pstn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-pstn', port_num: 4}));

	panel.add(ump_block_control_toggle('Анализ порта 1', ump_block_pstn_an, panel, 'main/icons/properties.png', false, {node: 'nt-bc-pstn', port_num: 1}));
	panel.add(ump_block_control_toggle('Анализ порта 2', ump_block_pstn_an, panel, 'main/icons/properties.png', false, {node: 'nt-bc-pstn', port_num: 2}));
	panel.add(ump_block_control_toggle('Анализ порта 3', ump_block_pstn_an, panel, 'main/icons/properties.png', false, {node: 'nt-bc-pstn', port_num: 3}));
	panel.add(ump_block_control_toggle('Анализ порта 4', ump_block_pstn_an, panel, 'main/icons/properties.png', false, {node: 'nt-bc-pstn', port_num: 4}));
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_ALL_NO_TERMINAL);

	panel.doLayout();
}


function ump_block_upn_control(panel)
{
	if (panel.prev_type == UMP_BLOCK_TYPE_DIGITAL) return;
	panel.prev_type = UMP_BLOCK_TYPE_DIGITAL;

	ump_block_init(panel);

	panel.add(ump_block_control_toggle('Порт 1, контакты 4-5', ump_block_upn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-upn', port_num: 1, line_num: 1}));
	panel.add(ump_block_control_toggle('Порт 1, контакты 3-6', ump_block_upn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-upn', port_num: 1, line_num: 2}));
	panel.add(ump_block_control_toggle('Порт 2, контакты 4-5', ump_block_upn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-upn', port_num: 2, line_num: 1}));
	panel.add(ump_block_control_toggle('Порт 2, контакты 3-6', ump_block_upn_play, panel, 'main/icons/play.png', false, {node: 'nt-bc-upn', port_num: 2, line_num: 2}));
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_ALL_NO_TERMINAL);

	panel.doLayout();
}

function ump_block_isdn_control(panel)
{
	if (panel.prev_type == UMP_BLOCK_TYPE_DIGITAL) return;
	panel.prev_type = UMP_BLOCK_TYPE_DIGITAL;

	ump_block_init(panel);

	panel.add(ump_block_control_button('Порт 1', ump_block_isdn_show, panel, 'main/icons/right.png', false, {node: 'nt-bc-isdn', port_num: 1}));
	panel.add(ump_block_control_button('Порт 2', ump_block_isdn_show, panel, 'main/icons/right.png', false, {node: 'nt-bc-isdn', port_num: 2}));
	panel.add(ump_block_control_button('Порт 3', ump_block_isdn_show, panel, 'main/icons/right.png', false, {node: 'nt-bc-isdn', port_num: 3}));
	panel.add(ump_block_control_button('Порт 4', ump_block_isdn_show, panel, 'main/icons/right.png', false, {node: 'nt-bc-isdn', port_num: 4}));
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_ALL_NO_TERMINAL);

	panel.doLayout();
}



function ump_block_eth_control(panel, t)
{
	if (panel.prev_type == t) return;
	panel.prev_type = t;

	ump_block_init(panel);

	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_RESET);

	panel.doLayout();
}


function ump_block_bd_action(block, ctl, p)
{
	var par = {dev_slot: block.dev_id};
	if (this.dev_type) par.dev_type = this.dev_type;
	request_action(this.node + '/control_' + this.action, par);
}


function ump_block_bd_control(panel, t)
{
	if (panel.prev_type == t) return;
	panel.prev_type = t;

	ump_block_init(panel);

	panel.add(ump_block_control_button('Подключиться', ump_block_bd_action, panel, 'main/icons/play.png', false, {node: 'nt-bs-cmgr', action: 'connect'}));
	panel.add(ump_block_control_button('Отключиться', ump_block_bd_action, panel, 'main/icons/pause.png', false, {node: 'nt-bs-cmgr', action: 'disconnect'}));
	panel.add(ump_block_control_button('Переподключиться', ump_block_bd_action, panel, 'main/icons/refresh.png', false, {node: 'nt-bs-cmgr', action: 'reconnect'}));
	panel.add(ump_block_control_button('Выбрать для доставки', ump_block_bd_action, panel, 'main/icons/record.png', false, {node: 'nt-bs-cmgr', action: 'attach'}));
	panel.add(ump_block_control_button('Завершить доставку', ump_block_bd_action, panel, 'main/icons/cross.png', false, {node: 'nt-bs-cmgr', action: 'detach'}));
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_RESET | UMP_BLOCK_CONTROL_TERMINAL);

	panel.doLayout();
}


function ump_block_pnoi_control(panel, t)
{
	if (panel.prev_type == t) return;
	panel.prev_type = t;

	ump_block_init(panel);

	panel.add(ump_block_control_button('Подключиться', ump_block_bd_action, panel, 'main/icons/play.png', false, {node: 'nt-bs-cmgr', dev_type: 200, action: 'connect'}));
	panel.add(ump_block_control_button('Отключиться', ump_block_bd_action, panel, 'main/icons/pause.png', false, {node: 'nt-bs-cmgr', dev_type: 200, action: 'disconnect'}));
	panel.add(ump_block_control_button('Переподключиться', ump_block_bd_action, panel, 'main/icons/refresh.png', false, {node: 'nt-bs-cmgr', dev_type: 200, action: 'reconnect'}));
	panel.add(ump_block_control_button('Выбрать для доставки', ump_block_bd_action, panel, 'main/icons/record.png', false, {node: 'nt-bs-cmgr', dev_type: 200, action: 'attach'}));
	panel.add(ump_block_control_button('Завершить доставку', ump_block_bd_action, panel, 'main/icons/cross.png', false, {node: 'nt-bs-cmgr', dev_type: 200, action: 'detach'}));
	ump_block_update_widgets(panel, UMP_BLOCK_CONTROL_BUTTON_RESET | UMP_BLOCK_CONTROL_TERMINAL);

	panel.doLayout();
}




function ump_block_crossplate_control(panel, dev_data)
{
	var indicators_mask = dev_data.dev_indicator_bus_mask;
	var buttons_mask = dev_data.dev_button_bus_mask;
	var indicators_inact_mask = dev_data.dev_indicator_bus_inactive_mask;
	var buttons_inact_mask = dev_data.dev_button_bus_inactive_mask;

	var icon;
	var disabled;
	var cur_mask;
	var item;
	var tooltip = 'Состояние неизвестно';

	if (indicators_inact_mask == undefined) indicators_inact_mask = 0;
	if (buttons_inact_mask == undefined) buttons_inact_mask = 0;

	for (i=0; i< UMP_CROSSPLATE_N_BUTTONS; ++i) {
		cur_mask = (1 << (i));
		item = panel.crossplate_buttons.topToolbar.items.items[i];
		if (buttons_mask & cur_mask) { 
			icon = 'main/icons/button_pressed_rounded.png';
			tooltip = 'Кнопка нажата';
		} else {
			icon = 'main/icons/button_released_rounded.png';
			tooltip = 'Кнопка отпущена';
		}
		if (buttons_inact_mask & cur_mask) {
			tooltip = 'Кнопка неактивна';
			disabled = true;
		} else {
			disabled = false;
		}
		item.setTooltip(tooltip);
		item.setDisabled(disabled);
		item.setIcon(icon);
	}

	for (i=0; i< UMP_CROSSPLATE_N_INDICATORS; ++i) {
		cur_mask = (1 << (i));
		item = panel.crossplate_indicators.topToolbar.items.items[i];
		if (indicators_mask & cur_mask) {
			icon = 'main/icons/bulb_light_on.png';
			tooltip = 'Индикатор выключен';
		} else {
			icon = 'main/icons/bulb_light_off.png';
			tooltip = 'Индикатор выключен';
		}
		if (indicators_inact_mask & cur_mask) {
			disabled = true;
			tooltip = 'Индикатор неактивен';
		} else {
			disabled = false;
		}
		item.setTooltip(tooltip);
		item.setDisabled(disabled);
		item.setIcon(icon);
	}
}

function ump_block_control_widget(title, flags)
{
	var p;
	var par;

	if (typeof title == "object") par = title;
	else par = {
		title: title,
		collapsible: true,
		collapsed: false
	};

	p = new Ext.FormPanel(Object.assign(par));
	if (flags & UMP_BLOCK_CONTROL_BUTTON_RESET) p.reset_button = p.addButton(ump_block_control_button('Сброс', ump_block_control_send_reset, p, 'main/icons/reset.png', true));
	if (flags & UMP_BLOCK_CONTROL_BUTTON_ON)    p.on_button = p.addButton(ump_block_control_button('Вкл', ump_block_control_send_power_on, p, 'main/icons/power_on.png', true));
	if (flags & UMP_BLOCK_CONTROL_BUTTON_OFF)   p.off_button = p.addButton(ump_block_control_button('Выкл', ump_block_control_send_power_off, p, 'main/icons/power_off.png', true));
	if (flags & UMP_BLOCK_CONTROL_BUTTON_CYCLE) p.off_on_button = p.addButton(ump_block_control_button('Цикл. сброс', ump_block_control_send_power_cycle, p, 'main/icons/cycle.png', true));

	return p;
}

function ump_block_update_widgets(p, flags)
{
	var t;
	p.reset_button.setDisabled((flags & UMP_BLOCK_CONTROL_BUTTON_RESET) == 0);
	p.on_button.setDisabled((flags & UMP_BLOCK_CONTROL_BUTTON_ON) == 0);
	p.off_button.setDisabled((flags & UMP_BLOCK_CONTROL_BUTTON_OFF) == 0);
	p.off_on_button.setDisabled((flags & UMP_BLOCK_CONTROL_BUTTON_CYCLE) == 0);
	t = p.getTool('collapse');
	if (t != undefined) {
		t.setVisible((flags & UMP_BLOCK_CONTROL_TERMINAL) != 0);
	}
}

function ump_block_control_default(title, block_id, flags, coll = false)
{
	var tools = [], p;
	if (flags & UMP_BLOCK_CONTROL_TERMINAL) tools.push({
		xtype: 'button',
		id: 'collapse',
		qtip: 'Терминал устройства',
		handler: function(event, toolEl, panel) {
			if (!panel.terminal_window){
				terminal_open(panel);
			}
		}
	})

	p = ump_block_control_widget({
		block_id: id,
		dev_id: ump_block_id_to_dev_id(block_id),
		title: title,
		orig_title: title,
		collapsible: false,
		collapsed: coll,
		titleCollapse: true,
		autoScroll: true,
		tools: tools,
		frame: ump_blocks_view_use_frame
		//bodyStyle: 'margin: 0px; padding: 5px 3px;'
	}, flags);
	return p;
}

function ump_block_control_all(title, block_id, coll=false)
{
	return ump_block_control_default(title, block_id, UMP_BLOCK_CONTROL_BUTTON_ALL, coll);
}

function ump_block_control_all_no_terminal(title, block_id, coll=false)
{
	return ump_block_control_default(title, block_id, UMP_BLOCK_CONTROL_BUTTON_ALL_NO_TERMINAL, coll);
}

function ump_block_control_reset(title, block_id, coll=false)
{
	return ump_block_control_default(title, block_id, UMP_BLOCK_CONTROL_BUTTON_RESET, coll);
}

function update_control_block(dev_data)
{
	var dev_id = dev_data.dev_id;
	var block_id;
	var type = dev_data.dev_type;
	var block = null;
	var html = '';

	if (dev_id == UMP_BLOCK_CROSSPLATE_ID) type = UMP_BLOCK_TYPE_CROSSPLATE;
	block_id = ump_dev_id_to_block_id(dev_id);

	if (block_id == undefined) return;
	block = ump_control_blocks[block_id];
	if (block == null) return;

	block.dev_data = dev_data;

//	if (dev_data.dev_text != null) html += dev_data.dev_text;

//	if (dev_data.status_text != null) html += dev_data.status_text;

	var dev_state_text = '';

	if (dev_data.status_state != null) {
		dev_state_text = ump_dev_status_state_to_text(dev_data.status_state);
	}

	if (dev_data.dev_info) {
		var new_title = block.orig_title + ': ' + dev_data.dev_info;
		var new_tip = dev_data.dev_text;
		if (dev_state_text != '') new_title += ' (' + dev_state_text + ')';
		if (block.title != new_title) {
			var header_text_area = block.header.child('.x-panel-header-text');
			block.setTitle(new_title);
			if (header_text_area) {
				new Ext.ToolTip({
					target: header_text_area.id,
					html: new_tip
				});
			}
		}
	}

	switch (type) {
	case UMP_BLOCK_TYPE_CROSSPLATE: ump_block_crossplate_control(block, dev_data); break;
	case UMP_BLOCK_TYPE_ANALOG: ump_block_pstn_control(block, type); break;
	case UMP_BLOCK_TYPE_DIGITAL: ump_block_upn_control(block, type); break;
	case UMP_BLOCK_TYPE_E1: ump_block_isdn_control(block, type); break;
	case UMP_BLOCK_TYPE_ETHERNET: ump_block_eth_control(block, type); break;
	case UMP_BLOCK_TYPE_ETHERNET_EXTRA: ump_block_empty_control(block); break;
	case UMP_BLOCK_TYPE_STORAGE: ump_block_pnoi_control(block, type); break;
	case UMP_BLOCK_TYPE_TFOP: ump_block_bd_control(block, type); break;
	case UMP_BLOCK_TYPE_WIFI: ump_block_bd_control(block, type); break;
	case UMP_BLOCK_TYPE_LTE: ump_block_bd_control(block, type); break;
	case UMP_BLOCK_TYPE_POWER_220V: ump_block_power_220v_control(block, type); break;
	case UMP_BLOCK_TYPE_POWER_48V: ump_block_power_48v_control(block, type); break;
	case UMP_BLOCK_TYPE_FAN: ump_block_fan_control(block); break;
	case UMP_BLOCK_TYPE_EMPTY: ump_block_empty_control(block); break;
	case UMP_BLOCK_TYPE_DUMMY_SMALL: ump_block_uncontrollable_control(block, type); break;
	case UMP_BLOCK_TYPE_DUMMY_LARGE: ump_block_uncontrollable_control(block, type); break;
	case UMP_BLOCK_TYPE_EMPTY: ump_block_empty_control(block); break;
	default: ump_block_unsupported_control(block, type); break;
	}

	//block.scroll('down', 10000);
}

function setAutoRefreshControlBlocksAll(item, delay = null, url = null, params = null, parser = null, def_update = true)
{
	var coll;
	item.enable_update = function() {
		if (def_update == true) item.update(Ext.Updater.defaults.indicatorText);
		else if (def_update != false) item.update(def_update);
		this.body.getUpdater().startAutoRefresh(delay, url, params, null, true);
	}
	item.disable_update = function() {
		this.body.getUpdater().stopAutoRefresh();
	}
	if (item.rendered) {
		var upd = item.body.getUpdater();
		if (delay == null) {
			upd.stopAutoRefresh();
		} else {
			upd.timeout = delay / 2.0;
			upd.startAutoRefresh(delay, url, params, null, true);
		}
		return;
	}
	item.on('render', function() {
		var upd = item.body.getUpdater();
		upd.setRenderer({
			render: function(el, response, scripts, callback) {
				var state;
				var html = '';
				try {
					state = JSON.parse(response.responseText);
				} catch (e) {
					return true; // do nothing
				}
				dev_data_array = state.dev_data;
				if (dev_data_array != null) {
					for (var i = 0; i < dev_data_array.length; i++) {
						var dev_data = dev_data_array[i];
						update_control_block(dev_data);
					}
				}
				
				if (parser != null) {
					html = parser.call(item, state);
					if (html == null) return true;
				} else {
					if (state.error_text != undefined) {
						html += '<div class=status-error>Ошибка: ' + state.error_text + '</div>';
					} else if (state.status_text != undefined) {
						html += state.status_text;
					}
				}
				//el.update(html);
				//el.scroll('down', 10000);
			}
		});
		upd.showLoading = function() { }
		if (!item.collapsed && !item.hidden) {
			item.enable_update();
		}
		upd.on('failure', function(el, response) {
			if (response.status == 403) { // forbidden
				document.location.reload(true);
			}
		});
		coll = item.collapsed;
		item.ownerCt.on('beforeexpand', function () {
			if (!coll) item.enable_update();
		});
		item.ownerCt.on('beforecollapse', function() {
			coll = item.collapsed;
			item.disable_update();
		});
		item.ownerCt.on('beforeshow', function () {
			if (!coll) item.enable_update();
		});
		item.ownerCt.on('beforehide', function() {
			coll = item.collapsed;
			item.disable_update();
		});
		item.ownerCt.on('activate', function () {
			if (!coll) item.enable_update();
		});
		item.ownerCt.on('deactivate', function() {
			coll = item.collapsed;
			item.disable_update();
		});
		item.on('activate', function () {
			if (!coll) this.enable_update();
		});
		item.on('deactivate', function() {
			coll = item.collapsed;
			this.disable_update();
		});
		item.on('beforeshow', function () {
			this.enable_update();
		});
		item.on('beforehide', function() {
			this.disable_update();
		});
	});
	item.on('beforeexpand', function() {
		this.enable_update();
	});
	item.on('collapse', function() {
		this.disable_update();
	});
}

ump_control_blocks = [
	ump_block_control_reset('Кросс-плата', 0),
	ump_block_control_all('Слот 1', 1),
	ump_block_control_all('Слот 2', 2),
	ump_block_control_all('Слот 3', 3),
	ump_block_control_all('Слот 4', 4),
	ump_block_control_all('Слот 5', 5),
	ump_block_control_all('Слот 6', 6),
	ump_block_control_all('Слот 7', 7),
	ump_block_control_all('Слот 8', 8),
	ump_block_control_all_no_terminal('Коммутатор', 9)
];

function empty_button_handler()
{
	Ext.MessageBox.alert('Ошибка выполнения команды', 'Команда не реализована на стороне web-интерфейса');
}

function ump_crossplate_control_send_button_pressed(block, button_id)
{
	return ump_block_control_send(block, 'dev_button_bus_set_state_pressed', { dev_button_bus_id: button_id+1 });
}

function ump_crossplate_control_send_button_released(block, button_id)
{
	return ump_block_control_send(block, 'dev_button_bus_set_state_released', { dev_button_bus_id: button_id+1 });
}

function ump_crossplate_control_send_button_cycle(block, button_id)
{
	return ump_block_control_send(block, 'dev_button_bus_set_state_cycle', { dev_button_bus_id: button_id+1 });
}

function ump_crossplate_control_send_indicator_on(block, indicator_id)
{
	return ump_block_control_send(block, 'dev_indicator_bus_set_state_on', { dev_indicator_bus_id: indicator_id+1 });
}

function ump_crossplate_control_send_indicator_off(block, indicator_id)
{
	return ump_block_control_send(block, 'dev_indicator_bus_set_state_off', { dev_indicator_bus_id: indicator_id+1 });
}

function ump_crossplate_control_send_indicator_blink(block, indicator_id)
{
	return ump_block_control_send(block, 'dev_indicator_bus_set_state_blink', { dev_indicator_bus_id: indicator_id+1 });
}

function ump_crossplate_tool(icon, tip, handler, handler_arg, handler_arg_id)
{
	tool = {
		icon: icon,
		tooltip: tip,
		handler: function(b) {
			Ext.MessageBox.show({
				icon: Ext.MessageBox.QUESTION,
				title: 'Подтверждение',
				msg: 'Выполнить действие?',
				buttons: {yes: 'Да', no: 'Нет'},
				fn: function(btn, text) {
					if (btn == 'yes') handler(handler_arg, handler_arg_id);
				}
			});
		}
	};
	return tool;
}

function ump_crossplate_menubutton(icon, text, handler, handler_arg, handler_arg_id)
{
	menubutton = {
		icon: icon,
		text: text,
		handler: function(b) {
			Ext.MessageBox.show({
				icon: Ext.MessageBox.QUESTION,
				title: 'Подтверждение',
				msg: 'Выполнить действие?',
				buttons: {yes: 'Да', no: 'Нет'},
				fn: function(btn, text) {
					if (btn == 'yes') handler(handler_arg, handler_arg_id);
				}
			});
		}
	};
	return menubutton;
}

function ump_crossplate_button(crossplate_block, button_id)
{
	var crossplate_button = {
		text: (button_id + 1).toString(),
		tooltip: 'Состояние неизвестно',
		icon: 'main/icons/cycle.png',
		disabled: true,
		menu: {
			xtype: 'menu',
			plain: true,
			items: [
				ump_crossplate_menubutton('main/icons/button_pressed_rounded.png', 'Нажать кнопку', ump_crossplate_control_send_button_pressed, crossplate_block, button_id),
				ump_crossplate_menubutton('main/icons/button_released_rounded.png', 'Отпустить кнопку', ump_crossplate_control_send_button_released, crossplate_block, button_id),
				ump_crossplate_menubutton('main/icons/cycle.png', 'Цикл кнопки', ump_crossplate_control_send_button_cycle, crossplate_block, button_id)
			]
		}
	};

	return crossplate_button;
}

function ump_crossplate_prepend_buttons(crossplate_block)
{
	var i;
	var crossplate_buttons;
	var tbar = new Ext.Toolbar();

	for (i=0; i < UMP_CROSSPLATE_N_BUTTONS; i++) {
		tbar.addField(ump_crossplate_button(ump_control_blocks[0], i));
	}
	crossplate_buttons = new Ext.Panel({
		title: 'Кнопки',
		tbar: tbar
	});

	crossplate_block.crossplate_buttons = crossplate_buttons;
	crossplate_block.insert(0, crossplate_buttons);
}

function ump_crossplate_indicator(crossplate_block, indicator_id)
{
	var crossplate_indicator = {
		text: (indicator_id + 1).toString(),
		tooltip: 'Состояние неизвестно',
		icon: 'main/icons/cycle.png',
		disabled: true,
		menu: {
			xtype: 'menu',
			plain: true,
			items: [
				ump_crossplate_menubutton('main/icons/bulb_light_on.png', 'Включить индикатор', ump_crossplate_control_send_indicator_on, crossplate_block, indicator_id),
				ump_crossplate_menubutton('main/icons/bulb_light_off.png', 'Выключить индикатор', ump_crossplate_control_send_indicator_off, crossplate_block, indicator_id),
				ump_crossplate_menubutton('main/icons/cycle.png', 'Мигание индикатора', ump_crossplate_control_send_indicator_blink, crossplate_block, indicator_id)
			]
		}
	};

	return crossplate_indicator;
}

function ump_crossplate_prepend_indicators(crossplate_block)
{
	var i;
	var crossplate_indicators;
	var tbar = new Ext.Toolbar();

	for (i=0; i < UMP_CROSSPLATE_N_INDICATORS; i++) {
		tbar.addField(ump_crossplate_indicator(ump_control_blocks[0], i));
	}
	crossplate_indicators = new Ext.Panel({
		title: 'Индикаторы',
		tbar: tbar
	});

	crossplate_block.crossplate_indicators = crossplate_indicators;
	crossplate_block.insert(0, crossplate_indicators);
}

ump_crossplate_prepend_indicators(ump_control_blocks[0]);
ump_crossplate_prepend_buttons(ump_control_blocks[0]);

ump_control = ump_create_panel('Управление УМП', 'ump_control', ump_control_blocks);

setAutoRefreshControlBlocksAll(ump_control_blocks[0], 1.0, '/' + ump_ctl_point + '/status_dev', {dev_id: -1}, null, false);
