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

static COMMAND_HELPER(handle_settings_args, const char **sep, const char **name)
{
	if (CMD_ARGC > 1)
		return ERROR_COMMAND_SYNTAX_ERROR;
	if (1 == CMD_ARGC) {
		*sep = " ";
		*name = CMD_ARGV[0];
	} else
		*sep = *name = "";

	return ERROR_OK;
}

COMMAND_HANDLER(handle_settings_command)
{
	const char *sep, *name;
	int retval = CALL_COMMAND_HANDLER(handle_settings_args, &sep, &name);
	if (ERROR_OK == retval)
		command_print(CMD_CTX, "Greetings%s%s!", sep, name);
	return retval;
}

COMMAND_HANDLER(handle_set_pin_command)
{
	const char *sep, *name;
	int retval = CALL_COMMAND_HANDLER(handle_settings_args, &sep, &name);
	if (ERROR_OK == retval)
		command_print(CMD_CTX, "Greetings%s%s!", sep, name);
	return retval;
}

COMMAND_HANDLER(handle_get_pin_command)
{
	const char *sep, *name;
	int retval = CALL_COMMAND_HANDLER(handle_settings_args, &sep, &name);
	if (ERROR_OK == retval)
		command_print(CMD_CTX, "Greetings%s%s!", sep, name);
	return retval;
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


static int jtag_gpio_init(struct command_context *cmd_ctx, struct target *target)
{
	return ERROR_OK;
}
static int jtag_gpio_poll(struct target *target)
{
	if ((target->state == TARGET_RUNNING) || (target->state == TARGET_DEBUG_RUNNING))
		target->state = TARGET_HALTED;
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
struct target_type jtag_gpio_target = {
	.name = "jtag_gpio",
	.commands = jtag_gpio_command_handlers,

	.init_target = &jtag_gpio_init,
	.poll = &jtag_gpio_poll,
	.halt = &jtag_gpio_halt,
	.assert_reset = &jtag_gpio_reset_assert,
	.deassert_reset = &jtag_gpio_reset_deassert,
};
