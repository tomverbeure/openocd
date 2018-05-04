/***************************************************************************
 *   Copyright (C) 2009 Zachary T Welch <zw@superlucidity.net>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <helper/log.h>

#include "target.h"
#include "target_type.h"
#include "jtag/jtag.h"

#define NR_GPIOS            3

#define IR_SCAN_N           2
#define IR_EXTEST           3

struct jtag_gpio_info {
    struct jtag_tap * tap;

    uint8_t *pin_dir_output;
    uint8_t *pin_input_val;
    uint8_t *pin_output_val;
};

static int jtag_gpio_set_config(struct jtag_gpio_info *jgi);
static int jtag_gpio_get_config(struct jtag_gpio_info *jgi);
static int jtag_gpio_xfer_values(struct jtag_gpio_info *jgi);

COMMAND_HANDLER(handle_settings_command)
{
    int retval;

	if (CMD_ARGC != 2){
		LOG_WARNING("Need pin_nr and value");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	uint8_t pin_nr, value;

	COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], pin_nr);
	COMMAND_PARSE_NUMBER(u8, CMD_ARGV[1], value);

	command_print(CMD_CTX, "pin dir %d = %d (%s)", pin_nr, value, value ? "output" : "input");

    struct jtag_gpio_info *jgi = CMD_CTX->current_target->arch_info;

    retval = jtag_gpio_get_config(jgi);
    if (retval != ERROR_OK)
        return retval;

    jgi->pin_dir_output[pin_nr] = value;
        
    retval = jtag_gpio_set_config(jgi);

    return retval;
}

COMMAND_HANDLER(handle_set_pin_command)
{
    int retval;

	if (CMD_ARGC != 2){
		LOG_WARNING("Need pin_nr and value");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	uint8_t pin_nr, value;
	COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], pin_nr);
	COMMAND_PARSE_NUMBER(u8, CMD_ARGV[1], value);

	command_print(CMD_CTX, "pin %d = %d", pin_nr, value);

    struct jtag_gpio_info *jgi = CMD_CTX->current_target->arch_info;
    jgi->pin_output_val[pin_nr] = value;

    retval = jtag_gpio_xfer_values(jgi);

	return retval;
}

COMMAND_HANDLER(handle_get_pin_command)
{
    int retval;

	if (CMD_ARGC != 1){
		LOG_WARNING("Need pin_nr");
		return ERROR_COMMAND_SYNTAX_ERROR;
	}

	uint8_t pin_nr;
	COMMAND_PARSE_NUMBER(u8, CMD_ARGV[0], pin_nr);

    struct jtag_gpio_info *jgi = CMD_CTX->current_target->arch_info;
    retval = jtag_gpio_xfer_values(jgi);

    if (retval != ERROR_OK)
        return retval;

	command_print(CMD_CTX, "%d", jgi->pin_input_val[pin_nr]);

	return ERROR_OK;
}

static const struct command_registration jtag_gpio_command_handlers[] = {
	{
		.name = "settings",
        .handler = handle_settings_command,
		.mode = COMMAND_ANY,
		.help = "set GPIO pins as input or output",
        .usage = "<enable_mask>"
	},
	{
		.name = "set_pin",
        .handler = handle_set_pin_command,
		.mode = COMMAND_ANY,
		.help = "Set GPIO output value of pin nr",
        .usage = "<pin_nr> <value>"
	},
	{
		.name = "get_pin",
        .handler = handle_get_pin_command,
		.mode = COMMAND_ANY,
		.help = "Print GPIO input values of pin nr",
        .usage = "<pin_nr>"
	},
	COMMAND_REGISTRATION_DONE
};

static int jtag_gpio_target_create(struct target *target, Jim_Interp *interp)
{
    if (target->tap == NULL)
        return ERROR_FAIL;

    struct jtag_gpio_info *jgi = calloc(1, sizeof(struct jtag_gpio_info));

    target->arch_info = jgi;

    jgi->tap = target->tap;

    int nr_pins = 3;
    jgi->pin_dir_output = calloc(1, nr_pins);
    jgi->pin_input_val = calloc(1, nr_pins);
    jgi->pin_output_val = calloc(1, nr_pins);

    return ERROR_OK;
}

static void jtag_gpio_select_data_chain_nr(struct jtag_gpio_info *jgi, uint8_t chain_nr)
{
    struct scan_field fields[1];

    uint8_t ir_code = IR_SCAN_N;

    fields[0].num_bits = 4;
    fields[0].out_value = &ir_code;
    fields[0].in_value = NULL;

    jtag_add_ir_scan(jgi->tap, fields, TAP_IDLE);

    fields[0].num_bits = 1;
    fields[0].out_value = &chain_nr;
    fields[0].in_value = NULL;

    jtag_add_dr_scan(jgi->tap, 1, fields, TAP_IDLE);

    ir_code = IR_EXTEST;

    fields[0].num_bits = 4;
    fields[0].out_value = &ir_code;
    fields[0].in_value = NULL;

    jtag_add_ir_scan(jgi->tap, fields, TAP_IDLE);
}

static int jtag_gpio_get_config(struct jtag_gpio_info *jgi)
{
    int retval;

    // Select CONFIG chain
    jtag_gpio_select_data_chain_nr(jgi, 0);

    uint8_t gpio_config = 0;
    struct scan_field fields[1];

    fields[0].num_bits = NR_GPIOS+1;
    fields[0].out_value = NULL;
    fields[0].in_value = &gpio_config;

    jtag_add_dr_scan(jgi->tap, 1, fields, TAP_IDLE);

	retval = jtag_execute_queue();
	if (retval != ERROR_OK)
		return retval;

    for(int i=0;i<NR_GPIOS;++i){
        jgi->pin_dir_output[i] = (gpio_config >> i) & 1;
    }

    return retval;
}

static int jtag_gpio_set_config(struct jtag_gpio_info *jgi)
{
    int retval;

    // Select CONFIG chain
    jtag_gpio_select_data_chain_nr(jgi, 0);

    struct scan_field fields[1];
    uint8_t gpio_config = 0;

    for(int i=0;i<3;++i){
        gpio_config |= (jgi->pin_dir_output[i] << i);
    }
    gpio_config |= (1<<NR_GPIOS);

    fields[0].num_bits = NR_GPIOS+1;
    fields[0].out_value = &gpio_config;
    fields[0].in_value = NULL;

    jtag_add_dr_scan(jgi->tap, 1, fields, TAP_IDLE);

	retval = jtag_execute_queue();
	if (retval != ERROR_OK)
		return retval;

    return retval;
}


static int jtag_gpio_xfer_values(struct jtag_gpio_info *jgi)
{
    int retval;

    // Select DATA chain
    jtag_gpio_select_data_chain_nr(jgi, 1);

    struct scan_field fields[1];

    uint8_t gpio_input_value = 0;
    uint8_t gpio_output_value = 0;

    for(int i=0;i<NR_GPIOS;++i){
        gpio_output_value |= (jgi->pin_output_val[i] << i);
    }
    gpio_output_value |= (1<<NR_GPIOS);     // Set update flag

    fields[0].num_bits = NR_GPIOS+1;
    fields[0].out_value = &gpio_output_value;
    fields[0].in_value = &gpio_input_value;

    jtag_add_dr_scan(jgi->tap, 1, fields, TAP_IDLE);

	retval = jtag_execute_queue();
	if (retval != ERROR_OK)
		return retval;

    for(int i=0;i<NR_GPIOS;++i){
        jgi->pin_input_val[i] = (gpio_input_value >> i) & 1;
    }

    return retval;
}

static int jtag_gpio_init(struct command_context *cmd_ctx, struct target *target)
{
	return ERROR_OK;
}
static int jtag_gpio_poll(struct target *target)
{
	if ((target->state == TARGET_RUNNING) || (target->state == TARGET_DEBUG_RUNNING))
		target->state = TARGET_HALTED;

//    struct jtag_gpio_info *jgi = target->arch_info;

	return ERROR_OK;
}
static int jtag_gpio_halt(struct target *target)
{
	target->state = TARGET_HALTED;
	return ERROR_OK;
}
static int jtag_gpio_reset_assert(struct target *target)
{
	target->state = TARGET_RESET;
	return ERROR_OK;
}
static int jtag_gpio_reset_deassert(struct target *target)
{
	target->state = TARGET_RUNNING;
	return ERROR_OK;
}

static int jtag_gpio_arch_state(struct target *target)
{
    struct jtag_gpio_info *jgi = target->arch_info;

    uint32_t direction = 0;
    uint32_t output = 0;
    uint32_t input = 0;

    for(int i=0;i<NR_GPIOS;++i){
        direction |= (jgi->pin_dir_output[i] << i);
        output    |= (jgi->pin_output_val[i] << i);
        input     |= (jgi->pin_input_val[i] << i);
    }

    LOG_USER("direction: %02x, output: %02x, input: %02x", direction, output, input);

    return ERROR_OK;
}

struct target_type jtag_gpio_target = {
	.name = "jtag_gpio",
	.commands = jtag_gpio_command_handlers,

    .target_create = &jtag_gpio_target_create,
	.init_target = &jtag_gpio_init,
	.poll = &jtag_gpio_poll,
	.halt = &jtag_gpio_halt,
	.assert_reset = &jtag_gpio_reset_assert,
	.deassert_reset = &jtag_gpio_reset_deassert,
    .arch_state = &jtag_gpio_arch_state
};
