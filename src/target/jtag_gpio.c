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
#include "hello.h"

static const struct command_registration jtag_gpio_command_handlers[] = {
	{
		.name = "jtag_gpio",
		.mode = COMMAND_ANY,
		.help = "jtag_gpio target commands",

		.chain = hello_command_handlers,
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
